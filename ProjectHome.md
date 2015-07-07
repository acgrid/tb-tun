**TB-TUN**

TB-TUN is an tiny **userspace** program to build **6to4/tunnelbroker/ISATAP** tunnel for Linux. The host kernel should have ipv6 stack and supports TUN/TAP. Generally the program should run with the root privilege.

Please read

### [HOWTO](http://code.google.com/p/tb-tun/wiki/HOWTO) ###


### [FAQ](http://code.google.com/p/tb-tun/wiki/FAQ) ###


---


**Update 2011.6.2**

Thanks to Luca Bertoncello, he wrote [ustun](http://www.lucabert.de/myProgram/viewProgramDetail.php?lang=en&programName=ustun), more powerful userspace tunnel program with firewall. Documentation can be found [here](http://www.lucabert.de/ipv6/index.php?lang=en).


---


**What is 6to4**

6to4 is an Internet transition mechanism for migrating from Internet Protocol version 4 (IPv4) to IPv6, a system that allows IPv6 packets to be transmitted over an IPv4 network (generally the IPv4 internet) without the need to configure explicit tunnels. Routing conventions are also in place that allow 6to4 hosts to communicate with hosts on the IPv6 internet. It is typically used when an end-site or end-user wants to connect to the IPv6 Internet using their existing IPv4 connection.

**What is ISATAP**

ISATAP (Intra-Site Automatic Tunnel Addressing Protocol) is an IPv6 transition mechanism meant to transmit IPv6 packets between dual-stack nodes on top of an IPv4 network.

> ----both from wikipedia

**What is IPv6**

IPv6 is the next-generation Internet Protocol version.

**Why TB-TUN**

Some old linux kernel or OpenVZ containers did not support linux-tunnel (device sit). TB-TUN can run in userspace and build 6to4/tunnelbroker/ISATAP tunnel.