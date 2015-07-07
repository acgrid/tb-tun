## [Get TB-TUN](http://code.google.com/p/tb-tun/downloads/list) ##
### Compile ###
Download the sourcefile **tb\_userspace.c** and compile it by gcc

**# gcc tb\_userspace.c -l pthread -o tb\_userspace**
### Binary(Recommended) ###
I compiled the source file on Debian 5.0 kernel **2.6.32-trunk-686** with gcc version **4.4.3 20100108**, and the binary file can work well on kernel **2.6.18-128.2.1.el5.028stab064.7**
You can download the binary file **tb\_userspace** directly.
## Config and Example ##

---


You should first know what is 6to4 tunnel(or tunnelbroker or ISATAP).

6to4 encapsulates IPv6 package in IPv4 package. To use 6to4, A relay server and a public IPv4 address for the client is necessary. By default IPv4 address 192.88.99.1 is assigned for relay server and anycasted by ISP. Thus the default IPv6 addressfor the client must be 2002:IPv4\_addr::IPv4\_addr. e.g. 2002:0B0B:0B0B::0B0B:0B0B for client IPv4 address 11.11.11.11.

Some ISPs(like [Hurricane Electric](http://www.tunnelbroker.net/)) provides tunnelbroker service. You can get your IPv6 address and IPv4 address of the relay server from the tunnelbroker provider or use the default one.

Other ISPs provide public ISATAP tunnel. ISATAP is another tunnel protocol a little bit different from 6to4 or tunnelbroker. The relay server announced its prefix and the client automatically uses IPv6 address as Prefix:0:5efe:xx.xx.xx.xx where xx.xx.xx.xx is the IPv4 address of the client. Currently TB-TUN cannot recognize the router announcement so you should configure your IPv6 address manually.


---


**The host system must support TUN/TAP device.**

If you have a firewall, you should allow IP proto **41**(IPPROTO\_IPV6) passing through.

Useage: tb\_userspace tun\_name remote\_ipv4 local\_ipv4 mode

The example below shows how to config a general 6to4 tunnel with client IPv4 address 11.11.11.11

**# setsid ./tb\_userspace tun6to4 any any sit > /dev/null**

**# ifconfig tun6to4 up**

**# ifconfig tun6to4 inet6 add 2002:0B0B:0B0B::0B0B:0B0B/64**

**# ifconfig tun6to4 mtu 1480**

**# route -A inet6 add ::/0 dev tun6to4**

(Or **ip -6 route add default dev tun6to4**, Thx the comments below)

tb\_userspace takes exactly 4 arguments:

The 1st is the name of device TUN(here is 'tun6to4').

The 2nd is the IPv4 addr of relay server. If **any** is here, a 6to4 tunnel will be build.

The 3rd is the local IPv4 addr. If **any** is here, TB-TUN will not bind the local IPv4 address.

The 4th is the tunnel mode. Currently **sit** for 6to4 and tunnelbroker and **isatap** for ISATAP are available.

Note that if Remote\_IPv4 address is specified, TB-TUN will only accept packages from this specified IPv4 address. So **./tb\_userspace tb 192.88.99.1 any sit** is **not** a proper way to set up 6to4 tunnel for it drops package from other IPv4 address, and only send packages to default gateway 192.88.99.1. It would cause problems when transmitting packages between 6to4 networks. (See [Issue3](http://code.google.com/p/tb-tun/issues/detail?id=3))

Usually the MTU of most networks is 1500. So the MTU of IPv6 tunnel should be 1480 by removing the 20 bytes IPv4 header. To set a proper MTU that neither cause IPv6 packet be **truncated** nor be too small for IPv6, You'd better find the IPv4 MTU from you to your tunnel relay server by your self and set the IPv6 MTU by minus 20 bytes.


The example below shows how to config a tunnelbroker client with client IPv4 address 1.2.3.4, relay server 5.6.7.8 and client IPv6 address 2001:a:b:c::2/64

**# setsid ./tb\_userspace tb 5.6.7.8 1.2.3.4 sit > /dev/null**

**# ifconfig tb up**

**# ifconfig tb inet6 add 2001:a:b:c::2/64**

**# ifconfig tb mtu 1480**

**# route -A inet6 add ::/0 dev tb**

The example below shows how to config a ISATAP client whith client IPv4 address 1.2.3.4, relay server 5.6.7.8, and IPv6 prefix 2001:a:b:c::/64.

**# setsid ./tb\_userspace isatap 5.6.7.8 1.2.3.4 isatap > /dev/null**

**# ifconfig isatap up**

**# ifconfig isatap inet6 add 2001:a:b:c:0:5efe:1.2.3.4/64**

**# ifconfig isatap mtu 1480**

**# route -A inet6 add ::/0 dev isatap**

And now you can try to ping some IPv6-only hosts:

**# ping6 ipv6.google.com**

**PING ipv6.google.com(vx-in-x67.1e100.net) 56 data bytes**

**64 bytes from vx-in-x67.1e100.net: icmp\_seq=1 ttl=55 time=43.0 ms**

**....**