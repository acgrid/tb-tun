#include <stdio.h>
#include <net/if.h>
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

struct Threadargs {
                int sockv6;
                int tun;
		int remote_ip;
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
	int sockv6, tun, ret, res, remote_ip, leniphead, size_of_pi;
	unsigned char bufsock[4096];
	unsigned char tmp[4096];
	struct tun_pi pi = {0, htons(ETH_P_IPV6)};
	sockv6 = (*(struct Threadargs *)s2targs).sockv6;
	tun = (*(struct Threadargs *)s2targs).tun;
	remote_ip = (*(struct Threadargs *)s2targs).remote_ip;
	size_of_pi = sizeof(struct tun_pi);
	while (1) {
                res = recv(sockv6, bufsock, sizeof(bufsock), 0);
                if (res < 0) {
                        printf("SOCKS ERROR\r\n");
                        continue;
                }
		if(*(int *)(&bufsock[12])!=remote_ip ) {
			printf("s2t:SOURCE IP %d(INT) NOT MATCH %d(INT)\r\n", *(int *)(&bufsock[12]), remote_ip);
			continue;
		}
		leniphead = 4*(bufsock[0]&0x000f);
		ret = res - leniphead;
		memcpy(tmp, &pi, size_of_pi);
		memcpy(&tmp[size_of_pi], &bufsock[leniphead], ret);
                ret = write(tun, tmp, ret + size_of_pi);
		if (ret < 0) printf("s2tTUN ERROR\r\n");
                printf("s2t:%d %d\r\n", res, ret);
        }
}

void t2s_thread(void *t2sargs) {
	int sockv6, tun, ret, res, remote_ip, leniphead, size_of_pi;
	unsigned char buftun[4096];
	unsigned char tmp[4096];
	sockv6 = (*(struct Threadargs *)t2sargs).sockv6;
        tun = (*(struct Threadargs *)t2sargs).tun;
	remote_ip = (*(struct Threadargs *)t2sargs).remote_ip;
	struct sockaddr_in remoteaddr;
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_port = htons(IPPROTO_IPV6);
	remoteaddr.sin_addr.s_addr = remote_ip;
	bzero(&(remoteaddr.sin_zero), 8);
	size_of_pi = sizeof(struct tun_pi);
	while (1) {
		ret = read(tun, buftun, sizeof(buftun) );
		if (ret < 0) {
			printf("t2sTUN ERROR\r\n");
			continue;
		}
		res = sendto(sockv6, &buftun[size_of_pi], ret, 0 ,&remoteaddr, sizeof(struct sockaddr));
		printf("t2s:%d %d\r\n", ret ,res);
	}
}

int main(int argc, char  *argv[])
{
        int tun, rets2t, rett2s;
        char tun_name[IFNAMSIZ];
        unsigned char buftun[4096];
	if (argc!=3) {
		printf("Useage:%s tun_name remote_ip\r\n",argv[0]);
		return 1;
	}
         strcpy(tun_name, argv[1]);
         tun = tun_create(tun_name, IFF_TUN );//| IFF_NO_PI);
        if (tun < 0) {
                perror("tun_create");
                return 1;
        }
        printf("TUN name is %s\n", tun_name);
	int sockv6;
	sockv6 = socket(AF_INET, SOCK_RAW, IPPROTO_IPV6);
	if (sockv6 < 0) {
		perror("v4_socket_create");
		return 1;
	}	
	pthread_t ids2t, idt2s;
	struct Threadargs s2targs, t2sargs;
	s2targs.sockv6 = sockv6;
	s2targs.tun = tun;
	s2targs.remote_ip = inet_addr(argv[2]);
        if (t2sargs.remote_ip < 0) {
                perror("remote_ip is not correct\r\n");
                return 1;
        }
	rets2t = pthread_create(&ids2t, NULL, (void *)s2t_thread, (void *)&s2targs);
	t2sargs.sockv6 = sockv6;
	t2sargs.tun = tun;
	t2sargs.remote_ip = inet_addr(argv[2]);
        rett2s = pthread_create(&idt2s, NULL, (void *)t2s_thread, (void *)&t2sargs);
	pthread_join(ids2t, NULL);
	pthread_join(idt2s, NULL);
        return 0;
}
