// Copyright 2009 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Sockets for Linux

package net

import "syscall"

func maxListenerBacklog() int {
	fd, err := open("/proc/sys/net/core/somaxconn")
	if err != nil {
		return syscall.SOMAXCONN
	}
	defer fd.close()
	l, ok := fd.readLine()
	if !ok {
		return syscall.SOMAXCONN
	}
	f := getFields(l)
	n, _, ok := dtoi(f[0], 0)
	if n == 0 || !ok {
		return syscall.SOMAXCONN
	}
	return n
}

func listenerSockaddr(s, f int, la syscall.Sockaddr, toAddr func(syscall.Sockaddr) Addr) (syscall.Sockaddr, error) {
	a := toAddr(la)
	if a == nil {
		return la, nil
	}
	switch v := a.(type) {
	case *TCPAddr, *UnixAddr:
		err := setDefaultListenerSockopts(s)
		if err != nil {
			return nil, err
		}
	case *UDPAddr:
		if v.IP.IsMulticast() {
			err := setDefaultMulticastSockopts(s)
			if err != nil {
				return nil, err
			}
			switch f {
			case syscall.AF_INET:
				v.IP = IPv4zero
			case syscall.AF_INET6:
				v.IP = IPv6unspecified
			}
			return v.sockaddr(f)
		}
	}
	return la, nil
}
