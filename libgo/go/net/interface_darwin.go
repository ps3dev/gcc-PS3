// Copyright 2011 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Network interface identification for Darwin

package net

import (
	"os"
	"syscall"
)

// If the ifindex is zero, interfaceMulticastAddrTable returns
// addresses for all network interfaces.  Otherwise it returns
// addresses for a specific interface.
func interfaceMulticastAddrTable(ifindex int) ([]Addr, error) {
	tab, err := syscall.RouteRIB(syscall.NET_RT_IFLIST2, ifindex)
	if err != nil {
		return nil, os.NewSyscallError("route rib", err)
	}

	msgs, err := syscall.ParseRoutingMessage(tab)
	if err != nil {
		return nil, os.NewSyscallError("route message", err)
	}

	var ifmat []Addr
	for _, m := range msgs {
		switch v := m.(type) {
		case *syscall.InterfaceMulticastAddrMessage:
			if ifindex == 0 || ifindex == int(v.Header.Index) {
				ifma, err := newMulticastAddr(v)
				if err != nil {
					return nil, err
				}
				ifmat = append(ifmat, ifma...)
			}
		}
	}
	return ifmat, nil
}

func newMulticastAddr(m *syscall.InterfaceMulticastAddrMessage) ([]Addr, error) {
	sas, err := syscall.ParseRoutingSockaddr(m)
	if err != nil {
		return nil, os.NewSyscallError("route sockaddr", err)
	}

	var ifmat []Addr
	for _, s := range sas {
		switch v := s.(type) {
		case *syscall.SockaddrInet4:
			ifma := &IPAddr{IP: IPv4(v.Addr[0], v.Addr[1], v.Addr[2], v.Addr[3])}
			ifmat = append(ifmat, ifma.toAddr())
		case *syscall.SockaddrInet6:
			ifma := &IPAddr{IP: make(IP, IPv6len)}
			copy(ifma.IP, v.Addr[:])
			// NOTE: KAME based IPv6 protcol stack usually embeds
			// the interface index in the interface-local or link-
			// local address as the kernel-internal form.
			if ifma.IP.IsInterfaceLocalMulticast() ||
				ifma.IP.IsLinkLocalMulticast() {
				// remove embedded scope zone ID
				ifma.IP[2], ifma.IP[3] = 0, 0
			}
			ifmat = append(ifmat, ifma.toAddr())
		}
	}
	return ifmat, nil
}
