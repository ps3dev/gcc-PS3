// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Garbage collector.

#include "runtime.h"
#include "arch.h"
#include "malloc.h"

#ifdef USING_SPLIT_STACK

extern void * __splitstack_find (void *, void *, size_t *, void **, void **,
				 void **);

extern void * __splitstack_find_context (void *context[10], size_t *, void **,
					 void **, void **);

#endif

enum {
	Debug = 0,
	PtrSize = sizeof(void*),
	DebugMark = 0,  // run second pass to check mark

	// Four bits per word (see #defines below).
	wordsPerBitmapWord = sizeof(void*)*8/4,
	bitShift = sizeof(void*)*8/4,
};

// Bits in per-word bitmap.
// #defines because enum might not be able to hold the values.
//
// Each word in the bitmap describes wordsPerBitmapWord words
// of heap memory.  There are 4 bitmap bits dedicated to each heap word,
// so on a 64-bit system there is one bitmap word per 16 heap words.
// The bits in the word are packed together by type first, then by
// heap location, so each 64-bit bitmap word consists of, from top to bottom,
// the 16 bitSpecial bits for the corresponding heap words, then the 16 bitMarked bits,
// then the 16 bitNoPointers/bitBlockBoundary bits, then the 16 bitAllocated bits.
// This layout makes it easier to iterate over the bits of a given type.
//
// The bitmap starts at mheap.arena_start and extends *backward* from
// there.  On a 64-bit system the off'th word in the arena is tracked by
// the off/16+1'th word before mheap.arena_start.  (On a 32-bit system,
// the only difference is that the divisor is 8.)
//
// To pull out the bits corresponding to a given pointer p, we use:
//
//	off = p - (uintptr*)mheap.arena_start;  // word offset
//	b = (uintptr*)mheap.arena_start - off/wordsPerBitmapWord - 1;
//	shift = off % wordsPerBitmapWord
//	bits = *b >> shift;
//	/* then test bits & bitAllocated, bits & bitMarked, etc. */
//
#define bitAllocated		((uintptr)1<<(bitShift*0))
#define bitNoPointers		((uintptr)1<<(bitShift*1))	/* when bitAllocated is set */
#define bitMarked		((uintptr)1<<(bitShift*2))	/* when bitAllocated is set */
#define bitSpecial		((uintptr)1<<(bitShift*3))	/* when bitAllocated is set - has finalizer or being profiled */
#define bitBlockBoundary	((uintptr)1<<(bitShift*1))	/* when bitAllocated is NOT set */

#define bitMask (bitBlockBoundary | bitAllocated | bitMarked | bitSpecial)

// Holding worldsema grants an M the right to try to stop the world.
// The procedure is:
//
//	runtime_semacquire(&runtime_worldsema);
//	m->gcing = 1;
//	runtime_stoptheworld();
//
//	... do stuff ...
//
//	m->gcing = 0;
//	runtime_semrelease(&runtime_worldsema);
//	runtime_starttheworld();
//
uint32 runtime_worldsema = 1;

// TODO: Make these per-M.
static uint64 nhandoff;

static int32 gctrace;

typedef struct Workbuf Workbuf;
struct Workbuf
{
	Workbuf *next;
	uintptr nobj;
	byte *obj[512-2];
};

typedef struct Finalizer Finalizer;
struct Finalizer
{
	void (*fn)(void*);
	void *arg;
	const struct __go_func_type *ft;
};

typedef struct FinBlock FinBlock;
struct FinBlock
{
	FinBlock *alllink;
	FinBlock *next;
	int32 cnt;
	int32 cap;
	Finalizer fin[1];
};

static G *fing;
static FinBlock *finq; // list of finalizers that are to be executed
static FinBlock *finc; // cache of free blocks
static FinBlock *allfin; // list of all blocks
static Lock finlock;
static int32 fingwait;

static void runfinq(void*);
static Workbuf* getempty(Workbuf*);
static Workbuf* getfull(Workbuf*);
static void	putempty(Workbuf*);
static Workbuf* handoff(Workbuf*);

static struct {
	Lock fmu;
	Workbuf	*full;
	Lock emu;
	Workbuf	*empty;
	uint32	nproc;
	volatile uint32	nwait;
	volatile uint32	ndone;
	Note	alldone;
	Lock	markgate;
	Lock	sweepgate;
	MSpan	*spans;

	Lock;
	byte	*chunk;
	uintptr	nchunk;
} work;

// scanblock scans a block of n bytes starting at pointer b for references
// to other objects, scanning any it finds recursively until there are no
// unscanned objects left.  Instead of using an explicit recursion, it keeps
// a work list in the Workbuf* structures and loops in the main function
// body.  Keeping an explicit work list is easier on the stack allocator and
// more efficient.
static void
scanblock(byte *b, int64 n)
{
	byte *obj, *arena_start, *arena_used, *p;
	void **vp;
	uintptr size, *bitp, bits, shift, i, j, x, xbits, off, nobj, nproc;
	MSpan *s;
	PageID k;
	void **wp;
	Workbuf *wbuf;
	bool keepworking;

	if((int64)(uintptr)n != n || n < 0) {
		// runtime_printf("scanblock %p %lld\n", b, (long long)n);
		runtime_throw("scanblock");
	}

	// Memory arena parameters.
	arena_start = runtime_mheap.arena_start;
	arena_used = runtime_mheap.arena_used;
	nproc = work.nproc;

	wbuf = nil;  // current work buffer
	wp = nil;  // storage for next queued pointer (write pointer)
	nobj = 0;  // number of queued objects

	// Scanblock helpers pass b==nil.
	// The main proc needs to return to make more
	// calls to scanblock.  But if work.nproc==1 then
	// might as well process blocks as soon as we
	// have them.
	keepworking = b == nil || work.nproc == 1;

	// Align b to a word boundary.
	off = (uintptr)b & (PtrSize-1);
	if(off != 0) {
		b += PtrSize - off;
		n -= PtrSize - off;
	}

	for(;;) {
		// Each iteration scans the block b of length n, queueing pointers in
		// the work buffer.
		if(Debug > 1)
			runtime_printf("scanblock %p %lld\n", b, (long long) n);

		vp = (void**)b;
		n >>= (2+PtrSize/8);  /* n /= PtrSize (4 or 8) */
		for(i=0; i<(uintptr)n; i++) {
			obj = (byte*)vp[i];

			// Words outside the arena cannot be pointers.
			if((byte*)obj < arena_start || (byte*)obj >= arena_used)
				continue;

			// obj may be a pointer to a live object.
			// Try to find the beginning of the object.

			// Round down to word boundary.
			obj = (void*)((uintptr)obj & ~((uintptr)PtrSize-1));

			// Find bits for this word.
			off = (uintptr*)obj - (uintptr*)arena_start;
			bitp = (uintptr*)arena_start - off/wordsPerBitmapWord - 1;
			shift = off % wordsPerBitmapWord;
			xbits = *bitp;
			bits = xbits >> shift;

			// Pointing at the beginning of a block?
			if((bits & (bitAllocated|bitBlockBoundary)) != 0)
				goto found;

			// Pointing just past the beginning?
			// Scan backward a little to find a block boundary.
			for(j=shift; j-->0; ) {
				if(((xbits>>j) & (bitAllocated|bitBlockBoundary)) != 0) {
					obj = (byte*)obj - (shift-j)*PtrSize;
					shift = j;
					bits = xbits>>shift;
					goto found;
				}
			}

			// Otherwise consult span table to find beginning.
			// (Manually inlined copy of MHeap_LookupMaybe.)
			k = (uintptr)obj>>PageShift;
			x = k;
			if(sizeof(void*) == 8)
				x -= (uintptr)arena_start>>PageShift;
			s = runtime_mheap.map[x];
			if(s == nil || k < s->start || k - s->start >= s->npages || s->state != MSpanInUse)
				continue;
			p =  (byte*)((uintptr)s->start<<PageShift);
			if(s->sizeclass == 0) {
				obj = p;
			} else {
				if((byte*)obj >= (byte*)s->limit)
					continue;
				size = runtime_class_to_size[s->sizeclass];
				int32 i = ((byte*)obj - p)/size;
				obj = p+i*size;
			}

			// Now that we know the object header, reload bits.
			off = (uintptr*)obj - (uintptr*)arena_start;
			bitp = (uintptr*)arena_start - off/wordsPerBitmapWord - 1;
			shift = off % wordsPerBitmapWord;
			xbits = *bitp;
			bits = xbits >> shift;

		found:
			// Now we have bits, bitp, and shift correct for
			// obj pointing at the base of the object.
			// Only care about allocated and not marked.
			if((bits & (bitAllocated|bitMarked)) != bitAllocated)
				continue;
			if(nproc == 1)
				*bitp |= bitMarked<<shift;
			else {
				for(;;) {
					x = *bitp;
					if(x & (bitMarked<<shift))
						goto continue_obj;
					if(runtime_casp((void**)bitp, (void*)x, (void*)(x|(bitMarked<<shift))))
						break;
				}
			}

			// If object has no pointers, don't need to scan further.
			if((bits & bitNoPointers) != 0)
				continue;

			// If another proc wants a pointer, give it some.
			if(nobj > 4 && work.nwait > 0 && work.full == nil) {
				wbuf->nobj = nobj;
				wbuf = handoff(wbuf);
				nobj = wbuf->nobj;
				wp = (void**)(wbuf->obj + nobj);
			}

			// If buffer is full, get a new one.
			if(wbuf == nil || nobj >= nelem(wbuf->obj)) {
				if(wbuf != nil)
					wbuf->nobj = nobj;
				wbuf = getempty(wbuf);
				wp = (void**)(wbuf->obj);
				nobj = 0;
			}
			*wp++ = obj;
			nobj++;
		continue_obj:;
		}

		// Done scanning [b, b+n).  Prepare for the next iteration of
		// the loop by setting b and n to the parameters for the next block.

		// Fetch b from the work buffer.
		if(nobj == 0) {
			if(!keepworking) {
				putempty(wbuf);
				return;
			}
			// Emptied our buffer: refill.
			wbuf = getfull(wbuf);
			if(wbuf == nil)
				return;
			nobj = wbuf->nobj;
			wp = (void**)(wbuf->obj + wbuf->nobj);
		}
		b = *--wp;
		nobj--;

		// Ask span about size class.
		// (Manually inlined copy of MHeap_Lookup.)
		x = (uintptr)b>>PageShift;
		if(sizeof(void*) == 8)
			x -= (uintptr)arena_start>>PageShift;
		s = runtime_mheap.map[x];
		if(s->sizeclass == 0)
			n = s->npages<<PageShift;
		else
			n = runtime_class_to_size[s->sizeclass];
	}
}

// debug_scanblock is the debug copy of scanblock.
// it is simpler, slower, single-threaded, recursive,
// and uses bitSpecial as the mark bit.
static void
debug_scanblock(byte *b, int64 n)
{
	byte *obj, *p;
	void **vp;
	uintptr size, *bitp, bits, shift, i, xbits, off;
	MSpan *s;

	if(!DebugMark)
		runtime_throw("debug_scanblock without DebugMark");

	if((int64)(uintptr)n != n || n < 0) {
		//runtime_printf("debug_scanblock %p %D\n", b, n);
		runtime_throw("debug_scanblock");
	}

	// Align b to a word boundary.
	off = (uintptr)b & (PtrSize-1);
	if(off != 0) {
		b += PtrSize - off;
		n -= PtrSize - off;
	}

	vp = (void**)b;
	n /= PtrSize;
	for(i=0; i<(uintptr)n; i++) {
		obj = (byte*)vp[i];

		// Words outside the arena cannot be pointers.
		if((byte*)obj < runtime_mheap.arena_start || (byte*)obj >= runtime_mheap.arena_used)
			continue;

		// Round down to word boundary.
		obj = (void*)((uintptr)obj & ~((uintptr)PtrSize-1));

		// Consult span table to find beginning.
		s = runtime_MHeap_LookupMaybe(&runtime_mheap, obj);
		if(s == nil)
			continue;


		p =  (byte*)((uintptr)s->start<<PageShift);
		if(s->sizeclass == 0) {
			obj = p;
			size = (uintptr)s->npages<<PageShift;
		} else {
			if((byte*)obj >= (byte*)s->limit)
				continue;
			size = runtime_class_to_size[s->sizeclass];
			int32 i = ((byte*)obj - p)/size;
			obj = p+i*size;
		}

		// Now that we know the object header, reload bits.
		off = (uintptr*)obj - (uintptr*)runtime_mheap.arena_start;
		bitp = (uintptr*)runtime_mheap.arena_start - off/wordsPerBitmapWord - 1;
		shift = off % wordsPerBitmapWord;
		xbits = *bitp;
		bits = xbits >> shift;

		// Now we have bits, bitp, and shift correct for
		// obj pointing at the base of the object.
		// If not allocated or already marked, done.
		if((bits & bitAllocated) == 0 || (bits & bitSpecial) != 0)  // NOTE: bitSpecial not bitMarked
			continue;
		*bitp |= bitSpecial<<shift;
		if(!(bits & bitMarked))
			runtime_printf("found unmarked block %p in %p\n", obj, vp+i);

		// If object has no pointers, don't need to scan further.
		if((bits & bitNoPointers) != 0)
			continue;

		debug_scanblock(obj, size);
	}
}

// Get an empty work buffer off the work.empty list,
// allocating new buffers as needed.
static Workbuf*
getempty(Workbuf *b)
{
	if(work.nproc == 1) {
		// Put b on full list.
		if(b != nil) {
			b->next = work.full;
			work.full = b;
		}
		// Grab from empty list if possible.
		b = work.empty;
		if(b != nil) {
			work.empty = b->next;
			goto haveb;
		}
	} else {
		// Put b on full list.
		if(b != nil) {
			runtime_lock(&work.fmu);
			b->next = work.full;
			work.full = b;
			runtime_unlock(&work.fmu);
		}
		// Grab from empty list if possible.
		runtime_lock(&work.emu);
		b = work.empty;
		if(b != nil)
			work.empty = b->next;
		runtime_unlock(&work.emu);
		if(b != nil)
			goto haveb;
	}

	// Need to allocate.
	runtime_lock(&work);
	if(work.nchunk < sizeof *b) {
		work.nchunk = 1<<20;
		work.chunk = runtime_SysAlloc(work.nchunk);
	}
	b = (Workbuf*)work.chunk;
	work.chunk += sizeof *b;
	work.nchunk -= sizeof *b;
	runtime_unlock(&work);

haveb:
	b->nobj = 0;
	return b;
}

static void
putempty(Workbuf *b)
{
	if(b == nil)
		return;

	if(work.nproc == 1) {
		b->next = work.empty;
		work.empty = b;
		return;
	}

	runtime_lock(&work.emu);
	b->next = work.empty;
	work.empty = b;
	runtime_unlock(&work.emu);
}

// Get a full work buffer off the work.full list, or return nil.
static Workbuf*
getfull(Workbuf *b)
{
	int32 i;
	Workbuf *b1;

	if(work.nproc == 1) {
		// Put b on empty list.
		if(b != nil) {
			b->next = work.empty;
			work.empty = b;
		}
		// Grab from full list if possible.
		// Since work.nproc==1, no one else is
		// going to give us work.
		b = work.full;
		if(b != nil)
			work.full = b->next;
		return b;
	}

	putempty(b);

	// Grab buffer from full list if possible.
	for(;;) {
		b1 = work.full;
		if(b1 == nil)
			break;
		runtime_lock(&work.fmu);
		if(work.full != nil) {
			b1 = work.full;
			work.full = b1->next;
			runtime_unlock(&work.fmu);
			return b1;
		}
		runtime_unlock(&work.fmu);
	}

	runtime_xadd(&work.nwait, +1);
	for(i=0;; i++) {
		b1 = work.full;
		if(b1 != nil) {
			runtime_lock(&work.fmu);
			if(work.full != nil) {
				runtime_xadd(&work.nwait, -1);
				b1 = work.full;
				work.full = b1->next;
				runtime_unlock(&work.fmu);
				return b1;
			}
			runtime_unlock(&work.fmu);
			continue;
		}
		if(work.nwait == work.nproc)
			return nil;
		if(i < 10)
			runtime_procyield(20);
		else if(i < 20)
			runtime_osyield();
		else
			runtime_usleep(100);
	}
}

static Workbuf*
handoff(Workbuf *b)
{
	int32 n;
	Workbuf *b1;

	// Make new buffer with half of b's pointers.
	b1 = getempty(nil);
	n = b->nobj/2;
	b->nobj -= n;
	b1->nobj = n;
	runtime_memmove(b1->obj, b->obj+b->nobj, n*sizeof b1->obj[0]);
	nhandoff += n;

	// Put b on full list - let first half of b get stolen.
	runtime_lock(&work.fmu);
	b->next = work.full;
	work.full = b;
	runtime_unlock(&work.fmu);

	return b1;
}

// Scanstack calls scanblock on each of gp's stack segments.
static void
scanstack(void (*scanblock)(byte*, int64), G *gp)
{
#ifdef USING_SPLIT_STACK
	M *mp;
	void* sp;
	size_t spsize;
	void* next_segment;
	void* next_sp;
	void* initial_sp;

	if(gp == runtime_g()) {
		// Scanning our own stack.
		sp = __splitstack_find(nil, nil, &spsize, &next_segment,
				       &next_sp, &initial_sp);
	} else if((mp = gp->m) != nil && mp->helpgc) {
		// gchelper's stack is in active use and has no interesting pointers.
		return;
	} else {
		// Scanning another goroutine's stack.
		// The goroutine is usually asleep (the world is stopped).

		// The exception is that if the goroutine is about to enter or might
		// have just exited a system call, it may be executing code such
		// as schedlock and may have needed to start a new stack segment.
		// Use the stack segment and stack pointer at the time of
		// the system call instead, since that won't change underfoot.
		if(gp->gcstack != nil) {
			sp = gp->gcstack;
			spsize = gp->gcstack_size;
			next_segment = gp->gcnext_segment;
			next_sp = gp->gcnext_sp;
			initial_sp = gp->gcinitial_sp;
		} else {
			sp = __splitstack_find_context(&gp->stack_context[0],
						       &spsize, &next_segment,
						       &next_sp, &initial_sp);
		}
	}
	if(sp != nil) {
		scanblock(sp, spsize);
		while((sp = __splitstack_find(next_segment, next_sp,
					      &spsize, &next_segment,
					      &next_sp, &initial_sp)) != nil)
			scanblock(sp, spsize);
	}
#else
	M *mp;
	byte* bottom;
	byte* top;

	if(gp == runtime_g()) {
		// Scanning our own stack.
		bottom = (byte*)&gp;
	} else if((mp = gp->m) != nil && mp->helpgc) {
		// gchelper's stack is in active use and has no interesting pointers.
		return;
	} else {
		// Scanning another goroutine's stack.
		// The goroutine is usually asleep (the world is stopped).
		bottom = (byte*)gp->gcnext_sp;
		if(bottom == nil)
			return;
	}
	top = (byte*)gp->gcinitial_sp + gp->gcstack_size;
	if(top > bottom)
		scanblock(bottom, top - bottom);
	else
		scanblock(top, bottom - top);
#endif
}

// Markfin calls scanblock on the blocks that have finalizers:
// the things pointed at cannot be freed until the finalizers have run.
static void
markfin(void *v)
{
	uintptr size;

	size = 0;
	if(!runtime_mlookup(v, (byte**)&v, &size, nil) || !runtime_blockspecial(v))
		runtime_throw("mark - finalizer inconsistency");

	// do not mark the finalizer block itself.  just mark the things it points at.
	scanblock(v, size);
}

static struct root_list* roots;

void
__go_register_gc_roots (struct root_list* r)
{
	// FIXME: This needs locking if multiple goroutines can call
	// dlopen simultaneously.
	r->next = roots;
	roots = r;
}

static void
debug_markfin(void *v)
{
	uintptr size;

	if(!runtime_mlookup(v, (byte**)&v, &size, nil))
		runtime_throw("debug_mark - finalizer inconsistency");
	debug_scanblock(v, size);
}

// Mark
static void
mark(void (*scan)(byte*, int64))
{
	struct root_list *pl;
	G *gp;
	FinBlock *fb;

	// mark data+bss.
	for(pl = roots; pl != nil; pl = pl->next) {
		struct root* pr = &pl->roots[0];
		while(1) {
			void *decl = pr->decl;
			if(decl == nil)
				break;
			scanblock(decl, pr->size);
			pr++;
		}
	}

	scan((byte*)&runtime_m0, sizeof runtime_m0);
	scan((byte*)&runtime_g0, sizeof runtime_g0);
	scan((byte*)&runtime_allg, sizeof runtime_allg);
	scan((byte*)&runtime_allm, sizeof runtime_allm);
	runtime_MProf_Mark(scan);
	runtime_time_scan(scan);

	// mark stacks
	for(gp=runtime_allg; gp!=nil; gp=gp->alllink) {
		switch(gp->status){
		default:
			runtime_printf("unexpected G.status %d\n", gp->status);
			runtime_throw("mark - bad status");
		case Gdead:
			break;
		case Grunning:
			if(gp != runtime_g())
				runtime_throw("mark - world not stopped");
			scanstack(scan, gp);
			break;
		case Grunnable:
		case Gsyscall:
		case Gwaiting:
			scanstack(scan, gp);
			break;
		}
	}

	// mark things pointed at by objects with finalizers
	if(scan == debug_scanblock)
		runtime_walkfintab(debug_markfin, scan);
	else
		runtime_walkfintab(markfin, scan);

	for(fb=allfin; fb; fb=fb->alllink)
		scanblock((byte*)fb->fin, fb->cnt*sizeof(fb->fin[0]));

	// in multiproc mode, join in the queued work.
	scan(nil, 0);
}

static bool
handlespecial(byte *p, uintptr size)
{
	void (*fn)(void*);
	const struct __go_func_type *ft;
	FinBlock *block;
	Finalizer *f;
	
	if(!runtime_getfinalizer(p, true, &fn, &ft)) {
		runtime_setblockspecial(p, false);
		runtime_MProf_Free(p, size);
		return false;
	}

	runtime_lock(&finlock);
	if(finq == nil || finq->cnt == finq->cap) {
		if(finc == nil) {
			finc = runtime_SysAlloc(PageSize);
			finc->cap = (PageSize - sizeof(FinBlock)) / sizeof(Finalizer) + 1;
			finc->alllink = allfin;
			allfin = finc;
		}
		block = finc;
		finc = block->next;
		block->next = finq;
		finq = block;
	}
	f = &finq->fin[finq->cnt];
	finq->cnt++;
	f->fn = fn;
	f->ft = ft;
	f->arg = p;
	runtime_unlock(&finlock); 
	return true;
}

// Sweep frees or collects finalizers for blocks not marked in the mark phase.
// It clears the mark bits in preparation for the next GC round.
static void
sweep(void)
{
	M *m;
	MSpan *s;
	int32 cl, n, npages;
	uintptr size;
	byte *p;
	MCache *c;
	byte *arena_start;
	int64 now;

	m = runtime_m();
	arena_start = runtime_mheap.arena_start;
	now = runtime_nanotime();

	for(;;) {
		s = work.spans;
		if(s == nil)
			break;
		if(!runtime_casp(&work.spans, s, s->allnext))
			continue;

		// Stamp newly unused spans. The scavenger will use that
		// info to potentially give back some pages to the OS.
		if(s->state == MSpanFree && s->unusedsince == 0)
			s->unusedsince = now;

		if(s->state != MSpanInUse)
			continue;

		p = (byte*)(s->start << PageShift);
		cl = s->sizeclass;
		if(cl == 0) {
			size = s->npages<<PageShift;
			n = 1;
		} else {
			// Chunk full of small blocks.
			size = runtime_class_to_size[cl];
			npages = runtime_class_to_allocnpages[cl];
			n = (npages << PageShift) / size;
		}

		// Sweep through n objects of given size starting at p.
		// This thread owns the span now, so it can manipulate
		// the block bitmap without atomic operations.
		for(; n > 0; n--, p += size) {
			uintptr off, *bitp, shift, bits;

			off = (uintptr*)p - (uintptr*)arena_start;
			bitp = (uintptr*)arena_start - off/wordsPerBitmapWord - 1;
			shift = off % wordsPerBitmapWord;
			bits = *bitp>>shift;

			if((bits & bitAllocated) == 0)
				continue;

			if((bits & bitMarked) != 0) {
				if(DebugMark) {
					if(!(bits & bitSpecial))
						runtime_printf("found spurious mark on %p\n", p);
					*bitp &= ~(bitSpecial<<shift);
				}
				*bitp &= ~(bitMarked<<shift);
				continue;
			}

			// Special means it has a finalizer or is being profiled.
			// In DebugMark mode, the bit has been coopted so
			// we have to assume all blocks are special.
			if(DebugMark || (bits & bitSpecial) != 0) {
				if(handlespecial(p, size))
					continue;
			}

			// Mark freed; restore block boundary bit.
			*bitp = (*bitp & ~(bitMask<<shift)) | (bitBlockBoundary<<shift);

			c = m->mcache;
			if(s->sizeclass == 0) {
				// Free large span.
				runtime_unmarkspan(p, 1<<PageShift);
				*(uintptr*)p = 1;	// needs zeroing
				runtime_MHeap_Free(&runtime_mheap, s, 1);
			} else {
				// Free small object.
				if(size > sizeof(uintptr))
					((uintptr*)p)[1] = 1;	// mark as "needs to be zeroed"
				c->local_by_size[s->sizeclass].nfree++;
				runtime_MCache_Free(c, p, s->sizeclass, size);
			}
			c->local_alloc -= size;
			c->local_nfree++;
		}
	}
}

void
runtime_gchelper(void)
{
	// Wait until main proc is ready for mark help.
	runtime_lock(&work.markgate);
	runtime_unlock(&work.markgate);
	scanblock(nil, 0);

	// Wait until main proc is ready for sweep help.
	runtime_lock(&work.sweepgate);
	runtime_unlock(&work.sweepgate);
	sweep();

	if(runtime_xadd(&work.ndone, +1) == work.nproc-1)
		runtime_notewakeup(&work.alldone);
}

// Initialized from $GOGC.  GOGC=off means no gc.
//
// Next gc is after we've allocated an extra amount of
// memory proportional to the amount already in use.
// If gcpercent=100 and we're using 4M, we'll gc again
// when we get to 8M.  This keeps the gc cost in linear
// proportion to the allocation cost.  Adjusting gcpercent
// just changes the linear constant (and also the amount of
// extra memory used).
static int32 gcpercent = -2;

static void
stealcache(void)
{
	M *m;

	for(m=runtime_allm; m; m=m->alllink)
		runtime_MCache_ReleaseAll(m->mcache);
}

static void
cachestats(void)
{
	M *m;
	MCache *c;
	uint32 i;
	uint64 stacks_inuse;
	uint64 stacks_sys;

	stacks_inuse = 0;
	stacks_sys = 0;
	for(m=runtime_allm; m; m=m->alllink) {
		runtime_purgecachedstats(m);
		// stacks_inuse += m->stackalloc->inuse;
		// stacks_sys += m->stackalloc->sys;
		c = m->mcache;
		for(i=0; i<nelem(c->local_by_size); i++) {
			mstats.by_size[i].nmalloc += c->local_by_size[i].nmalloc;
			c->local_by_size[i].nmalloc = 0;
			mstats.by_size[i].nfree += c->local_by_size[i].nfree;
			c->local_by_size[i].nfree = 0;
		}
	}
	mstats.stacks_inuse = stacks_inuse;
	mstats.stacks_sys = stacks_sys;
}

void
runtime_gc(int32 force)
{
	M *m;
	int64 t0, t1, t2, t3;
	uint64 heap0, heap1, obj0, obj1;
	const byte *p;
	bool extra;

	// Make sure all registers are saved on stack so that
	// scanstack sees them.
	__builtin_unwind_init();

	// The gc is turned off (via enablegc) until
	// the bootstrap has completed.
	// Also, malloc gets called in the guts
	// of a number of libraries that might be
	// holding locks.  To avoid priority inversion
	// problems, don't bother trying to run gc
	// while holding a lock.  The next mallocgc
	// without a lock will do the gc instead.
	m = runtime_m();
	if(!mstats.enablegc || m->locks > 0 || runtime_panicking)
		return;

	if(gcpercent == -2) {	// first time through
		p = runtime_getenv("GOGC");
		if(p == nil || p[0] == '\0')
			gcpercent = 100;
		else if(runtime_strcmp((const char*)p, "off") == 0)
			gcpercent = -1;
		else
			gcpercent = runtime_atoi(p);

		p = runtime_getenv("GOGCTRACE");
		if(p != nil)
			gctrace = runtime_atoi(p);
	}
	if(gcpercent < 0)
		return;

	runtime_semacquire(&runtime_worldsema);
	if(!force && mstats.heap_alloc < mstats.next_gc) {
		runtime_semrelease(&runtime_worldsema);
		return;
	}

	t0 = runtime_nanotime();
	nhandoff = 0;

	m->gcing = 1;
	runtime_stoptheworld();

	cachestats();
	heap0 = mstats.heap_alloc;
	obj0 = mstats.nmalloc - mstats.nfree;

	runtime_lock(&work.markgate);
	runtime_lock(&work.sweepgate);

	extra = false;
	work.nproc = 1;
	if(runtime_gomaxprocs > 1 && runtime_ncpu > 1) {
		runtime_noteclear(&work.alldone);
		work.nproc += runtime_helpgc(&extra);
	}
	work.nwait = 0;
	work.ndone = 0;

	runtime_unlock(&work.markgate);  // let the helpers in
	mark(scanblock);
	if(DebugMark)
		mark(debug_scanblock);
	t1 = runtime_nanotime();

	work.spans = runtime_mheap.allspans;
	runtime_unlock(&work.sweepgate);  // let the helpers in
	sweep();
	if(work.nproc > 1)
		runtime_notesleep(&work.alldone);
	t2 = runtime_nanotime();

	stealcache();
	cachestats();

	mstats.next_gc = mstats.heap_alloc+mstats.heap_alloc*gcpercent/100;
	m->gcing = 0;

	m->locks++;	// disable gc during the mallocs in newproc
	if(finq != nil) {
		// kick off or wake up goroutine to run queued finalizers
		if(fing == nil)
			fing = __go_go(runfinq, nil);
		else if(fingwait) {
			fingwait = 0;
			runtime_ready(fing);
		}
	}
	m->locks--;

	cachestats();
	heap1 = mstats.heap_alloc;
	obj1 = mstats.nmalloc - mstats.nfree;

	t3 = runtime_nanotime();
	mstats.last_gc = t3;
	mstats.pause_ns[mstats.numgc%nelem(mstats.pause_ns)] = t3 - t0;
	mstats.pause_total_ns += t3 - t0;
	mstats.numgc++;
	if(mstats.debuggc)
		runtime_printf("pause %llu\n", (unsigned long long)t3-t0);

	if(gctrace) {
		runtime_printf("gc%d(%d): %llu+%llu+%llu ms %llu -> %llu MB %llu -> %llu (%llu-%llu) objects %llu handoff\n",
			mstats.numgc, work.nproc, (unsigned long long)(t1-t0)/1000000, (unsigned long long)(t2-t1)/1000000, (unsigned long long)(t3-t2)/1000000,
			(unsigned long long)heap0>>20, (unsigned long long)heap1>>20, (unsigned long long)obj0, (unsigned long long)obj1,
			(unsigned long long) mstats.nmalloc, (unsigned long long)mstats.nfree,
			(unsigned long long) nhandoff);
	}
	
	runtime_MProf_GC();
	runtime_semrelease(&runtime_worldsema);

	// If we could have used another helper proc, start one now,
	// in the hope that it will be available next time.
	// It would have been even better to start it before the collection,
	// but doing so requires allocating memory, so it's tricky to
	// coordinate.  This lazy approach works out in practice:
	// we don't mind if the first couple gc rounds don't have quite
	// the maximum number of procs.
	runtime_starttheworld(extra);

	// give the queued finalizers, if any, a chance to run	
	if(finq != nil)	
		runtime_gosched();

	if(gctrace > 1 && !force)
		runtime_gc(1);
}

void runtime_ReadMemStats(MStats *)
  __asm__("libgo_runtime.runtime.ReadMemStats");

void
runtime_ReadMemStats(MStats *stats)
{
	M *m;

	// Have to acquire worldsema to stop the world,
	// because stoptheworld can only be used by
	// one goroutine at a time, and there might be
	// a pending garbage collection already calling it.
	runtime_semacquire(&runtime_worldsema);
	m = runtime_m();
	m->gcing = 1;
	runtime_stoptheworld();
	cachestats();
	*stats = mstats;
	m->gcing = 0;
	runtime_semrelease(&runtime_worldsema);
	runtime_starttheworld(false);
}

static void
runfinq(void* dummy __attribute__ ((unused)))
{
	G* gp;
	Finalizer *f;
	FinBlock *fb, *next;
	uint32 i;

	gp = runtime_g();
	for(;;) {
		// There's no need for a lock in this section
		// because it only conflicts with the garbage
		// collector, and the garbage collector only
		// runs when everyone else is stopped, and
		// runfinq only stops at the gosched() or
		// during the calls in the for loop.
		fb = finq;
		finq = nil;
		if(fb == nil) {
			fingwait = 1;
			gp->status = Gwaiting;
			gp->waitreason = "finalizer wait";
			runtime_gosched();
			continue;
		}
		for(; fb; fb=next) {
			next = fb->next;
			for(i=0; i<(uint32)fb->cnt; i++) {
				void *params[1];

				f = &fb->fin[i];
				params[0] = &f->arg;
				runtime_setblockspecial(f->arg, false);
				reflect_call(f->ft, (void*)f->fn, 0, 0, params, nil);
				f->fn = nil;
				f->arg = nil;
			}
			fb->cnt = 0;
			fb->next = finc;
			finc = fb;
		}
		runtime_gc(1);	// trigger another gc to clean up the finalized objects, if possible
	}
}

// mark the block at v of size n as allocated.
// If noptr is true, mark it as having no pointers.
void
runtime_markallocated(void *v, uintptr n, bool noptr)
{
	uintptr *b, obits, bits, off, shift;

	// if(0)
		// runtime_printf("markallocated %p+%p\n", v, n);

	if((byte*)v+n > (byte*)runtime_mheap.arena_used || (byte*)v < runtime_mheap.arena_start)
		runtime_throw("markallocated: bad pointer");

	off = (uintptr*)v - (uintptr*)runtime_mheap.arena_start;  // word offset
	b = (uintptr*)runtime_mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;

	for(;;) {
		obits = *b;
		bits = (obits & ~(bitMask<<shift)) | (bitAllocated<<shift);
		if(noptr)
			bits |= bitNoPointers<<shift;
		if(runtime_singleproc) {
			*b = bits;
			break;
		} else {
			// more than one goroutine is potentially running: use atomic op
			if(runtime_casp((void**)b, (void*)obits, (void*)bits))
				break;
		}
	}
}

// mark the block at v of size n as freed.
void
runtime_markfreed(void *v, uintptr n)
{
	uintptr *b, obits, bits, off, shift;

	// if(0)
		// runtime_printf("markallocated %p+%p\n", v, n);

	if((byte*)v+n > (byte*)runtime_mheap.arena_used || (byte*)v < runtime_mheap.arena_start)
		runtime_throw("markallocated: bad pointer");

	off = (uintptr*)v - (uintptr*)runtime_mheap.arena_start;  // word offset
	b = (uintptr*)runtime_mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;

	for(;;) {
		obits = *b;
		bits = (obits & ~(bitMask<<shift)) | (bitBlockBoundary<<shift);
		if(runtime_singleproc) {
			*b = bits;
			break;
		} else {
			// more than one goroutine is potentially running: use atomic op
			if(runtime_casp((void**)b, (void*)obits, (void*)bits))
				break;
		}
	}
}

// check that the block at v of size n is marked freed.
void
runtime_checkfreed(void *v, uintptr n)
{
	uintptr *b, bits, off, shift;

	if(!runtime_checking)
		return;

	if((byte*)v+n > (byte*)runtime_mheap.arena_used || (byte*)v < runtime_mheap.arena_start)
		return;	// not allocated, so okay

	off = (uintptr*)v - (uintptr*)runtime_mheap.arena_start;  // word offset
	b = (uintptr*)runtime_mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;

	bits = *b>>shift;
	if((bits & bitAllocated) != 0) {
		runtime_printf("checkfreed %p+%p: off=%p have=%p\n",
			v, (void*)n, (void*)off, (void*)(bits & bitMask));
		runtime_throw("checkfreed: not freed");
	}
}

// mark the span of memory at v as having n blocks of the given size.
// if leftover is true, there is left over space at the end of the span.
void
runtime_markspan(void *v, uintptr size, uintptr n, bool leftover)
{
	uintptr *b, off, shift;
	byte *p;

	if((byte*)v+size*n > (byte*)runtime_mheap.arena_used || (byte*)v < runtime_mheap.arena_start)
		runtime_throw("markspan: bad pointer");

	p = v;
	if(leftover)	// mark a boundary just past end of last block too
		n++;
	for(; n-- > 0; p += size) {
		// Okay to use non-atomic ops here, because we control
		// the entire span, and each bitmap word has bits for only
		// one span, so no other goroutines are changing these
		// bitmap words.
		off = (uintptr*)p - (uintptr*)runtime_mheap.arena_start;  // word offset
		b = (uintptr*)runtime_mheap.arena_start - off/wordsPerBitmapWord - 1;
		shift = off % wordsPerBitmapWord;
		*b = (*b & ~(bitMask<<shift)) | (bitBlockBoundary<<shift);
	}
}

// unmark the span of memory at v of length n bytes.
void
runtime_unmarkspan(void *v, uintptr n)
{
	uintptr *p, *b, off;

	if((byte*)v+n > (byte*)runtime_mheap.arena_used || (byte*)v < runtime_mheap.arena_start)
		runtime_throw("markspan: bad pointer");

	p = v;
	off = p - (uintptr*)runtime_mheap.arena_start;  // word offset
	if(off % wordsPerBitmapWord != 0)
		runtime_throw("markspan: unaligned pointer");
	b = (uintptr*)runtime_mheap.arena_start - off/wordsPerBitmapWord - 1;
	n /= PtrSize;
	if(n%wordsPerBitmapWord != 0)
		runtime_throw("unmarkspan: unaligned length");
	// Okay to use non-atomic ops here, because we control
	// the entire span, and each bitmap word has bits for only
	// one span, so no other goroutines are changing these
	// bitmap words.
	n /= wordsPerBitmapWord;
	while(n-- > 0)
		*b-- = 0;
}

bool
runtime_blockspecial(void *v)
{
	uintptr *b, off, shift;

	if(DebugMark)
		return true;

	off = (uintptr*)v - (uintptr*)runtime_mheap.arena_start;
	b = (uintptr*)runtime_mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;

	return (*b & (bitSpecial<<shift)) != 0;
}

void
runtime_setblockspecial(void *v, bool s)
{
	uintptr *b, off, shift, bits, obits;

	if(DebugMark)
		return;

	off = (uintptr*)v - (uintptr*)runtime_mheap.arena_start;
	b = (uintptr*)runtime_mheap.arena_start - off/wordsPerBitmapWord - 1;
	shift = off % wordsPerBitmapWord;

	for(;;) {
		obits = *b;
		if(s)
			bits = obits | (bitSpecial<<shift);
		else
			bits = obits & ~(bitSpecial<<shift);
		if(runtime_singleproc) {
			*b = bits;
			break;
		} else {
			// more than one goroutine is potentially running: use atomic op
			if(runtime_casp((void**)b, (void*)obits, (void*)bits))
				break;
		}
	}
}

void
runtime_MHeap_MapBits(MHeap *h)
{
	// Caller has added extra mappings to the arena.
	// Add extra mappings of bitmap words as needed.
	// We allocate extra bitmap pieces in chunks of bitmapChunk.
	enum {
		bitmapChunk = 8192
	};
	uintptr n;

	n = (h->arena_used - h->arena_start) / wordsPerBitmapWord;
	n = (n+bitmapChunk-1) & ~(bitmapChunk-1);
	if(h->bitmap_mapped >= n)
		return;

	runtime_SysMap(h->arena_start - n, n - h->bitmap_mapped);
	h->bitmap_mapped = n;
}
