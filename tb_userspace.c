#include <stdio.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_tun.h>
#include <assert.h>
#include <memory.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <netinet/in.h>
#include <syslog.h>
#include <stddef.h>
#define ipaddr_string(x) inet_ntoa(*(struct in_addr *)(&(x) - \
		offsetof(struct in_addr, s_addr)))

/* compile with 
 * cc -l pthread -Wall -O2 -s -o tb_userspace tb_userspace.c
 */

struct Threadargs {
                int sockv6;
                int tun;
		int remote_ip;
        	int tun_mode;	/*0 for 6to4, 1 for tunnelbroker, 2 for isatap*/
	};

int tun_create(char *dev, int flags)
{
    struct ifreq ifr;
    int fd, err;
    assert(dev != NULL);
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
        return fd;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags |= flags;
    if (*dev != '\0')
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
        close(fd);
        return err;
    }
    strcpy(dev, ifr.ifr_name);
    return fd;
}

void s2t_thread(void *s2targs) {
	int sockv6, tun, ret, res, remote_ip, leniphead, tun_mode;
	unsigned char bufsock[4096];
	struct tun_pi pi = {0, htons(ETH_P_IPV6)};
	/*pi is sth. like link-layer header for device tun and should be set type IPV6*/
	sockv6 = (*(struct Threadargs *)s2targs).sockv6;
	tun = (*(struct Threadargs *)s2targs).tun;
	remote_ip = (*(struct Threadargs *)s2targs).remote_ip;
	tun_mode = (*(struct Threadargs *)s2targs).tun_mode;
	while (1) {
                res = recv(sockv6, bufsock, sizeof(bufsock), 0);
                if (res < 0) {
                        syslog(LOG_ERR, "SOCKS ERROR");
                        continue;
                }
		leniphead = 4*(bufsock[0]&0x0f);
		if (leniphead>60 || leniphead<20) {
			syslog(LOG_ERR,
				"IPv4 header invalid size %d", leniphead);
			continue;
		}
		if (tun_mode==1) {/*Only accept package with source IPv4 addr the same as tunnel server's*/
			if ( *(int *)(&bufsock[12])!=remote_ip ) {
				syslog(LOG_DEBUG, "s2t:Drop package from source IPv4 %s not the relay server %s",
					ipaddr_string(bufsock[12]),
					ipaddr_string(remote_ip));
				continue;
			}
		}
		else if (tun_mode==2) {/*In ISATAP mode, we should accept package (IPv6)prefix:0:5efe:xx.xx.xx.xx in (IPv4)xx.xx.xx.xx*/
			if ( ( *(int *)(&bufsock[12])!=remote_ip ) && (( *(int *)(&bufsock[(leniphead+16)])!=ntohl(0x00005efe)) || ( *(int *)(&bufsock[12])!=*(int *)(&bufsock[leniphead+20])) ) ) {
				syslog(LOG_DEBUG, "s2t:Drop package from source IPv4 %s which does not correspond to its IPv6 address.",
					ipaddr_string(bufsock[12]));
				continue;
			}
		}
		else if ((tun_mode==0) && ( *(int *)(&bufsock[12])!=remote_ip) ) {/*In 6to4 mode, (IPv6)2002:xxxx:xxxx::/48 in (IPv4)xx.xx.xx.xx should be accepted*/
			if (*(short *)(&bufsock[(leniphead+8)])!=ntohs(0x2002)) {
				syslog(LOG_DEBUG, "s2t:Drop package from source IPv4 %s which does not correspond to its IPv6 address.",
					ipaddr_string(bufsock[12]));
				continue;
			}
			else if( (*(int *)(&bufsock[12]))!=(*(int *)(&bufsock[(leniphead+10)]))) {
				syslog(LOG_DEBUG, "s2t:Drop package from source IPv4 %s which does not correspond to its IPv6 address.",
					ipaddr_string(bufsock[12]));
				continue;
			}
		}
		else syslog(LOG_DEBUG, "s2t:Accepting packet from source IPv4 %s",
			ipaddr_string(bufsock[12]));
		ret = res - leniphead;
		memcpy(&bufsock[leniphead-sizeof(struct tun_pi)], &pi, sizeof(struct tun_pi));
		ret = write(tun, &bufsock[leniphead-sizeof(struct tun_pi)], ret + sizeof(struct tun_pi));
                syslog(LOG_DEBUG, "s2t: %d/%d bytes", res, ret);
	}
}

void t2s_thread(void *t2sargs) {
	int sockv6, tun, ret, res, remote_ip, tun_mode;
	unsigned char buftun[4096];
	sockv6 = (*(struct Threadargs *)t2sargs).sockv6;
        tun = (*(struct Threadargs *)t2sargs).tun;
	remote_ip = (*(struct Threadargs *)t2sargs).remote_ip;
	tun_mode = (*(struct Threadargs *)t2sargs).tun_mode;
	struct sockaddr_in remoteaddr;
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_port = htons(IPPROTO_IPV6);
	bzero(&(remoteaddr.sin_zero), 8);
	while (1) {
		ret = read(tun, buftun, sizeof(buftun) );
		if (ret < 0) {
			syslog(LOG_DEBUG, "t2sTUN ERROR");
			continue;
		}
		remoteaddr.sin_addr.s_addr = remote_ip;
		switch (tun_mode) {
			case 0:
				if (*(short *)(&buftun[(sizeof(struct tun_pi)+24)])==ntohs(0x2002)) {
					remoteaddr.sin_addr.s_addr = *(int *)(&buftun[(sizeof(struct tun_pi)+26)]);
				}/*send package directly to (IPv4)xx.xx.xx.xx other than default gw 192.88.99.1 when handing (IPv6)2002:xxxx:xxxx::/48 package*/
				break;
			case 1: break;
			case 2:
				if ( (*(int *)(&buftun[(sizeof(struct tun_pi)+24)])==*(int *)(&buftun[(sizeof(struct tun_pi)+8)])) &&
					(*(int *)(&buftun[(sizeof(struct tun_pi)+28)])==*(int *)(&buftun[(sizeof(struct tun_pi)+12)])) &&
					(*(int *)(&buftun[(sizeof(struct tun_pi)+32)])==*(int *)(&buftun[(sizeof(struct tun_pi)+16)])) &&
					(*(int *)(&buftun[(sizeof(struct tun_pi)+16)])==ntohl(0x00005efe))) {
					remoteaddr.sin_addr.s_addr = *(int *)(&buftun[(sizeof(struct tun_pi)+36)]);
				}/*In ISATAP mode, IPv6 packages to /64 neighbours are send directly to their IPv4 address without relay*/
				break;
		}
		res = sendto(sockv6, &buftun[sizeof(struct tun_pi)], ret, 0 ,(struct sockaddr *)&remoteaddr, sizeof(struct sockaddr));
		syslog(LOG_DEBUG, "t2s: %d/%d bytes", ret ,res);
	}
}

int main(int argc, char  *argv[])
{
        int tun;
        in_addr_t remote_ip;
        char tun_name[IFNAMSIZ];
	setlogmask(LOG_UPTO(LOG_NOTICE));
	openlog("tb_userspace", 0, LOG_DAEMON);
	if (argc != 5) {
		fprintf(stderr,
			"Usage: %s tun_name remote_ipv4 local_ipv4 mode\r\n",
		       	argv[0]);
		return 1;
	}
        strcpy(tun_name, argv[1]);
        tun = tun_create(tun_name, IFF_TUN );//| IFF_NO_PI);
        if (tun < 0) {
                perror("tun_create");
                return 1;
        }
        fprintf(stderr, "TUN name is %s\r\n", tun_name);
	int sockv6, if_bind, tun_mode;
	sockv6 = socket(AF_INET, SOCK_RAW, IPPROTO_IPV6);
	if (sockv6 < 0) {
		perror("v4_socket_create");
		return 1;
	}
	fprintf(stderr, "IPv4 SOCK_RAW created: %d\r\n", sockv6);
	struct sockaddr_in localaddr;
	localaddr.sin_family = AF_INET;
        localaddr.sin_port = htons(IPPROTO_IPV6);
        localaddr.sin_addr.s_addr = inet_addr(argv[3]);
	bzero(&(localaddr.sin_zero), 8);
	if (localaddr.sin_addr.s_addr != -1) {
		if_bind = bind(sockv6, (struct sockaddr *)&localaddr, sizeof(struct sockaddr));
		if (if_bind < 0) {
			perror("bind local address");
			return 1;
		}
		fprintf(stderr, "Bind local IPv4 address %s\r\n", ipaddr_string(localaddr.sin_addr.s_addr));
	}/*If local_ipv4(argv[3]) is "any", do not bind local IPv4 address*/
	else fprintf(stderr, "Do not bind local IPv4 address, using default.\r\n");
	pthread_t ids2t, idt2s;
	struct Threadargs s2targs, t2sargs;
	s2targs.sockv6 = sockv6;
	s2targs.tun = tun;
	if (!strcmp(argv[4],"sit")) {
		if (strcmp(argv[2],"any")) {
			remote_ip = inet_addr(argv[2]);
			tun_mode = 1;
		}
		else {
			remote_ip = inet_addr("192.88.99.1");
			tun_mode = 0;
		}
	}/*If remote_ipv4(argv[2]) is "any", use 6to4 mode and set default gateway 192.88.99.1*/
	else if (!strcmp(argv[4],"isatap")) {
		remote_ip = inet_addr(argv[2]);
		tun_mode = 2;
	}
	else {
		fprintf(stderr, "tunnel mode %s not found.\r\n", argv[4]);
		return 1;
	}
	s2targs.tun_mode = tun_mode;
        if (remote_ip == INADDR_NONE) {
                fprintf(stderr, "Bad remote ipv4 address.\r\n");
                return 1;
        }
	else {
		t2sargs.remote_ip = remote_ip;
		s2targs.remote_ip = remote_ip;
	}
        fprintf(stderr, "Using remote IPv4 %s\r\n", ipaddr_string(remote_ip));
	pthread_create(&ids2t, NULL, (void *)s2t_thread, (void *)&s2targs);
	t2sargs.sockv6 = sockv6;
	t2sargs.tun = tun;
	t2sargs.tun_mode = tun_mode;
        pthread_create(&idt2s, NULL, (void *)t2s_thread, (void *)&t2sargs);
	pthread_join(ids2t, NULL);
	pthread_join(idt2s, NULL);
        return 0;
}
