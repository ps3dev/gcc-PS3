// $G $D/$F.go && $L $F.$A && ./$A.out 2>&1 | cmp - $D/$F.out

// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import "unsafe"

func main() {
	var p unsafe.Pointer
	println(p)
}
