tb_userspace: tb_userspace.c
	gcc -l pthread -Wall -O2 -s -o $@ $<
