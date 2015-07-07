1.Why not use linux tunnel?

Linux has already provided device 'sit' for 6to4. But if the kernel is compiled without device 'sit', TB-TUN becomes useful for it only needs the host supporting TUN/TAP. It is especially useful for Openvz VPSes. As far as I know, most Openvz VPS providers provide neither a native IPv6 address nor a kernel supporting device 'sit', but TUN/TAP is often supported.

2.I have tried TB-TUN but errors occur.

Till now I only test the program on my computer and my VPS. If the host system is a different one, I cannot guarantee a good performance. But If you look into the source code you will find the 6to4 protocol is so easy - just put the IPv6 package into IPv4 package with IPv4 protocol 41. So, please help me to improve this program.

3.6to4 is IPv6 over IP, Could I use TB-TUN for IPv6 over PPPOE & etc.?

Currently no. And I did not plan to study linux socket further. If you have any ideas, please share with me.

4.I also need a userspace firewall.

You may find [ustun](http://www.lucabert.de/myProgram/viewProgramDetail.php?lang=en&programName=ustun) helpful.