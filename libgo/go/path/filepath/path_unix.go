// Copyright 2010 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build darwin freebsd linux netbsd openbsd

package filepath

import "strings"

// IsAbs returns true if the path is absolute.
func IsAbs(path string) bool {
	return strings.HasPrefix(path, "/")
}

// VolumeName returns the leading volume name on Windows.
// It returns "" elsewhere.
func VolumeName(path string) string {
	return ""
}

// HasPrefix exists for historical compatibility and should not be used.
func HasPrefix(p, prefix string) bool {
	return strings.HasPrefix(p, prefix)
}
