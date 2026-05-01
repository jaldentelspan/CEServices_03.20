#!/bin/sh
find . -follow -regex ".*\\.[chCH]" -print 2>/dev/null | grep -v -E "(build|.*#.*|/tcpip/|/lwip_tcpip/|/windows/|acl|ssh_lib|upnp|mitr_fpga)" | xargs ctags -a
