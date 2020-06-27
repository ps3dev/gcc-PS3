// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build darwin freebsd netbsd openbsd

// Network interface identification for BSD variants

package net

import (
	"os"
	"syscall"
	"unsafe"
)

// If the ifindex is zero, interfaceTable returns mappings of all
// network interfaces.  Otherwise it returns a mapping of a specific
// interface.
func interfaceTable(ifindex int) ([]Interface, error) {
	tab, err := syscall.RouteRIB(syscall.NET_RT_IFLIST, ifindex)
	if err != nil {
		return nil, os.NewSyscallError("route rib", err)
	}

	msgs, err := syscall.ParseRoutingMessage(tab)
	if err != nil {
		return nil, os.NewSyscallError("route message", err)
	}

	var ift []Interface
	for _, m := range msgs {
		switch v := m.(type) {
		case *syscall.InterfaceMessage:
			if ifindex == 0 || ifindex == int(v.Header.Index) {
				ifi, err := newLink(v)
				if err != nil {
					return nil, err
				}
				ift = append(ift, ifi...)
			}
		}
	}
	return ift, nil
}

func newLink(m *syscall.InterfaceMessage) ([]Interface, error) {
	sas, err := syscall.ParseRoutingSockaddr(m)
	if err != nil {
		return nil, os.NewSyscallError("route sockaddr", err)
	}

	var ift []Interface
	for _, s := range sas {
		switch v := s.(type) {
		case *syscall.SockaddrDatalink:
			// NOTE: SockaddrDatalink.Data is minimum work area,
			// can be larger.
			m.Data = m.Data[unsafe.Offsetof(v.Data):]
			ifi := Interface{Index: int(m.Header.Index), Flags: linkFlags(m.Header.Flags)}
			var name [syscall.IFNAMSIZ]byte
			for i := 0; i < int(v.Nlen); i++ {
				name[i] = byte(m.Data[i])
			}
			ifi.Name = string(name[:v.Nlen])
			ifi.MTU = int(m.Header.Data.Mtu)
			addr := make([]byte, v.Alen)
			for i := 0; i < int(v.Alen); i++ {
				addr[i] = byte(m.Data[int(v.Nlen)+i])
			}
			ifi.HardwareAddr = addr[:v.Alen]
			ift = append(ift, ifi)
		}
	}
	return ift, nil
}

func linkFlags(rawFlags int32) Flags {
	var f Flags
	if rawFlags&syscall.IFF_UP != 0 {
		f |= FlagUp
	}
	if rawFlags&syscall.IFF_BROADCAST != 0 {
		f |= FlagBroadcast
	}
	if rawFlags&syscall.IFF_LOOPBACK != 0 {
		f |= FlagLoopback
	}
	if rawFlags&syscall.IFF_POINTOPOINT != 0 {
		f |= FlagPointToPoint
	}
	if rawFlags&syscall.IFF_MULTICAST != 0 {
		f |= FlagMulticast
	}
	return f
}

// If the ifindex is zero, interfaceAddrTable returns addresses
// for all network interfaces.  Otherwise it returns addresses
// for a specific interface.
func interfaceAddrTable(ifindex int) ([]Addr, error) {
	tab, err := syscall.RouteRIB(syscall.NET_RT_IFLIST, ifindex)
	if err != nil {
		return nil, os.NewSyscallError("route rib", err)
	}

	msgs, err := syscall.ParseRoutingMessage(tab)
	if err != nil {
		return nil, os.NewSyscallError("route message", err)
	}

	var ifat []Addr
	for _, m := range msgs {
		switch v := m.(type) {
		case *syscall.InterfaceAddrMessage:
			if ifindex == 0 || ifindex == int(v.Header.Index) {
				ifa, err := newAddr(v)
				if err != nil {
					return nil, err
				}
				ifat = append(ifat, ifa)
			}
		}
	}
	return ifat, nil
}

func newAddr(m *syscall.InterfaceAddrMessage) (Addr, error) {
	sas, err := syscall.ParseRoutingSockaddr(m)
	if err != nil {
		return nil, os.NewSyscallError("route sockaddr", err)
	}

	ifa := &IPNet{}
	for i, s := range sas {
		switch v := s.(type) {
		case *syscall.SockaddrInet4:
			switch i {
			case 0:
				ifa.Mask = IPv4Mask(v.Addr[0], v.Addr[1], v.Addr[2], v.Addr[3])
			case 1:
				ifa.IP = IPv4(v.Addr[0], v.Addr[1], v.Addr[2], v.Addr[3])
			}
		case *syscall.SockaddrInet6:
			switch i {
			case 0:
				ifa.Mask = make(IPMask, IPv6len)
				copy(ifa.Mask, v.Addr[:])
			case 1:
				ifa.IP = make(IP, IPv6len)
				copy(ifa.IP, v.Addr[:])
				// NOTE: KAME based IPv6 protcol stack usually embeds
				// the interface index in the interface-local or link-
				// local address as the kernel-internal form.
				if ifa.IP.IsLinkLocalUnicast() {
					// remove embedded scope zone ID
					ifa.IP[2], ifa.IP[3] = 0, 0
				}
			}
		}
	}
	return ifa, nil
}
