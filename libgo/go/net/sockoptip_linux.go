// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// IP-level socket options for Linux

package net

import (
	"os"
	"syscall"
)

func ipv4MulticastInterface(fd *netFD) (*Interface, error) {
	if err := fd.incref(false); err != nil {
		return nil, err
	}
	defer fd.decref()
	mreq, err := syscall.GetsockoptIPMreqn(fd.sysfd, syscall.IPPROTO_IP, syscall.IP_MULTICAST_IF)
	if err != nil {
		return nil, os.NewSyscallError("getsockopt", err)
	}
	if int(mreq.Ifindex) == 0 {
		return nil, nil
	}
	return InterfaceByIndex(int(mreq.Ifindex))
}

func setIPv4MulticastInterface(fd *netFD, ifi *Interface) error {
	var v int32
	if ifi != nil {
		v = int32(ifi.Index)
	}
	mreq := &syscall.IPMreqn{Ifindex: v}
	if err := fd.incref(false); err != nil {
		return err
	}
	defer fd.decref()
	err := syscall.SetsockoptIPMreqn(fd.sysfd, syscall.IPPROTO_IP, syscall.IP_MULTICAST_IF, mreq)
	if err != nil {
		return os.NewSyscallError("setsockopt", err)
	}
	return nil
}

func ipv4MulticastTTL(fd *netFD) (int, error) {
	if err := fd.incref(false); err != nil {
		return 0, err
	}
	defer fd.decref()
	v, err := syscall.GetsockoptInt(fd.sysfd, syscall.IPPROTO_IP, syscall.IP_MULTICAST_TTL)
	if err != nil {
		return -1, os.NewSyscallError("getsockopt", err)
	}
	return v, nil
}

func setIPv4MulticastTTL(fd *netFD, v int) error {
	if err := fd.incref(false); err != nil {
		return err
	}
	defer fd.decref()
	err := syscall.SetsockoptInt(fd.sysfd, syscall.IPPROTO_IP, syscall.IP_MULTICAST_TTL, v)
	if err != nil {
		return os.NewSyscallError("setsockopt", err)
	}
	return nil
}

func ipv4MulticastLoopback(fd *netFD) (bool, error) {
	if err := fd.incref(false); err != nil {
		return false, err
	}
	defer fd.decref()
	v, err := syscall.GetsockoptInt(fd.sysfd, syscall.IPPROTO_IP, syscall.IP_MULTICAST_LOOP)
	if err != nil {
		return false, os.NewSyscallError("getsockopt", err)
	}
	return v == 1, nil
}

func setIPv4MulticastLoopback(fd *netFD, v bool) error {
	if err := fd.incref(false); err != nil {
		return err
	}
	defer fd.decref()
	err := syscall.SetsockoptInt(fd.sysfd, syscall.IPPROTO_IP, syscall.IP_MULTICAST_LOOP, boolint(v))
	if err != nil {
		return os.NewSyscallError("setsockopt", err)
	}
	return nil
}

func ipv4ReceiveInterface(fd *netFD) (bool, error) {
	if err := fd.incref(false); err != nil {
		return false, err
	}
	defer fd.decref()
	v, err := syscall.GetsockoptInt(fd.sysfd, syscall.IPPROTO_IP, syscall.IP_PKTINFO)
	if err != nil {
		return false, os.NewSyscallError("getsockopt", err)
	}
	return v == 1, nil
}

func setIPv4ReceiveInterface(fd *netFD, v bool) error {
	if err := fd.incref(false); err != nil {
		return err
	}
	defer fd.decref()
	err := syscall.SetsockoptInt(fd.sysfd, syscall.IPPROTO_IP, syscall.IP_PKTINFO, boolint(v))
	if err != nil {
		return os.NewSyscallError("setsockopt", err)
	}
	return nil
}

func ipv6TrafficClass(fd *netFD) (int, error) {
	if err := fd.incref(false); err != nil {
		return 0, err
	}
	defer fd.decref()
	v, err := syscall.GetsockoptInt(fd.sysfd, syscall.IPPROTO_IPV6, syscall.IPV6_TCLASS)
	if err != nil {
		return 0, os.NewSyscallError("getsockopt", err)
	}
	return v, nil
}

func setIPv6TrafficClass(fd *netFD, v int) error {
	if err := fd.incref(false); err != nil {
		return err
	}
	defer fd.decref()
	err := syscall.SetsockoptInt(fd.sysfd, syscall.IPPROTO_IPV6, syscall.IPV6_TCLASS, v)
	if err != nil {
		return os.NewSyscallError("setsockopt", err)
	}
	return nil
}
