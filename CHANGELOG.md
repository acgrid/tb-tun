# CHANGE LOG #

## [r5](https://code.google.com/p/tb-tun/source/detail?r=5) ##

The earliest version, but have some errors when compiling (Described in [Issue 1](https://code.google.com/p/tb-tun/issues/detail?id=1)).

## [r14](https://code.google.com/p/tb-tun/source/detail?r=14) ##

Fixed compile errors in [r5](https://code.google.com/p/tb-tun/source/detail?r=5) (Described in [Issue 1](https://code.google.com/p/tb-tun/issues/detail?id=1)). This is the stable version for TunnelBroker and have been used by many OpenVZ VPS users. But have trouble in 6to4 mode when transmitting packages between 6to4 networks.(As described in [Issue 3](https://code.google.com/p/tb-tun/issues/detail?id=3), Drop package when incoming source IPv6 address is 2002::/16)

## [r15](https://code.google.com/p/tb-tun/source/detail?r=15) ##

Added ISATAP support and fixed [Issue 3](https://code.google.com/p/tb-tun/issues/detail?id=3), fully supports 6to4.

**It contains a critical error**, Error happen when the last number of remote\_ipv4 is larger than 127. This version is deleted from the Download page.

## [r18](https://code.google.com/p/tb-tun/source/detail?r=18) ##

Fixed the remote\_ipv4 bug in [r15](https://code.google.com/p/tb-tun/source/detail?r=15).