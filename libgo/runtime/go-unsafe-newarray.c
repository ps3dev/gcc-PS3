/* go-unsafe-newarray.c -- unsafe.NewArray function for Go.

   Copyright 2009 The Go Authors. All rights reserved.
   Use of this source code is governed by a BSD-style
   license that can be found in the LICENSE file.  */

#include "runtime.h"
#include "go-alloc.h"
#include "go-type.h"
#include "interface.h"

/* Implement unsafe_NewArray, called from the reflect package.  */

void *unsafe_NewArray (struct __go_empty_interface type, int n)
  asm ("libgo_reflect.reflect.unsafe_NewArray");

/* The dynamic type of the argument will be a pointer to a type
   descriptor.  */

void *
unsafe_NewArray (struct __go_empty_interface type, int n)
{
  const struct __go_type_descriptor *descriptor;

  if (((uintptr_t) type.__type_descriptor & reflectFlags) != 0)
    runtime_panicstring ("invalid interface value");

  /* FIXME: We should check __type_descriptor to verify that this is
     really a type descriptor.  */
  descriptor = (const struct __go_type_descriptor *) type.__object;
  return __go_alloc (descriptor->__size * n);
}
