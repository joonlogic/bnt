/********************************************************************
 * bnt_gpioint.c : Application for verifying BNT interrupt 
 *
 * Copyright (c) 2018  TheFrons, Inc.
 * Copyright (c) 2018  Joon Kim <joon@thefrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 ********************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <poll.h>
#include <bnt_def.h>
#include <bnt_ext.h>

typedef struct {
	int           boardid;
	bool          unexport;
	bool          isall;
	bool          poll;
} T_OptInfo;

static void print_usage(const char *prog)
{
	printf("Usage: %s [-abdp]\n", prog);
	puts("  -b --boardid     board id(default 0). Range(0~3)\n"
	     "  -d --disable     disable(unexport) gpio\n"
	     "  -a --all         all boards\n"
	     "  -p --poll        poll from interrupt\n"
		 "--------------------------------------------------\n"
		 "  [Board 0] : GPIO (23)\n"
		 "  [Board 1] : GPIO (24)\n"
		 "  [Board 2] : GPIO (17)\n"
		 "  [Board 3] : GPIO (18)\n"
		 );
	exit(1);
}

static int parse_opts(int argc, char *argv[], T_OptInfo* info)
{
	while (1) {
		static const struct option lopts[] = {
			{ "boardid", 1, 0, 'b' },
			{ "all",     0, 0, 'a' },
			{ "disable", 0, 0, 'd' },
			{ "poll",    0, 0, 'p' },
			{ NULL,      0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "b:adp", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'b':
			info->boardid = atoi(optarg);
			if((info->boardid < 0) || (info->boardid > MAX_SPI_CS)) 
				print_usage(argv[0]);
			break;
		case 'a':
			info->isall = 1;
			break;
		case 'd':
			info->unexport = true;
			break;
		case 'p':
			info->poll = true;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}

	return 0; 
}

const unsigned int BNT_GPIO_IRQ[] = {23, 24, 17, 18};
const int BNT_GPIOINT_TIMEOUT_MS = 60000;

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd[4]={0,};
	T_OptInfo info = {0,};
	struct pollfd pfd[4] = {0,};
	struct pollfd* ppfd;
	int nfds = 1;

	ret = parse_opts(argc, argv, &info);
	BNT_CHECK_RESULT(ret, ret);

	if( info.isall ) {
		for(int i=0; i<sizeof(BNT_GPIO_IRQ)/sizeof(int); i++) {
			fd[i] = do_config_gpio_irq(BNT_GPIO_IRQ[i], info.unexport);
			printf("GPIO(%d) fd(%d)\n", BNT_GPIO_IRQ[i], fd[i]);
			pfd[i].fd = fd[i];
			pfd[i].events = POLLPRI | POLLERR;
		}
		nfds = 4;
		ppfd = pfd;
	}
	else {
		fd[info.boardid] = do_config_gpio_irq(BNT_GPIO_IRQ[info.boardid], info.unexport);
		printf("GPIO(%d) fd(%d)\n", BNT_GPIO_IRQ[info.boardid], fd[info.boardid]);
		pfd[info.boardid].fd = fd[info.boardid];
		pfd[info.boardid].events = POLLPRI | POLLERR;
		nfds = 1;
		ppfd = &pfd[info.boardid];
	}

	if(info.poll != true) return 0;

	//consume any prior interrupts
	int c;
	for(int i=0; i<nfds; i++) {
		lseek(ppfd[i].fd, 0, SEEK_SET);
		read(ppfd[i].fd, &c, sizeof c);
	}

	unsigned int count = 0;
	while(1) {
		ret = poll(ppfd, nfds, BNT_GPIOINT_TIMEOUT_MS);
		if(ret == 0 ) {
			printf("%s: TIMEOUT(60s) ...%d\n", argv[0], count++);
		}
		else if(ret > 0) {
			printf("%s: Event FD(%d)\n", argv[0], ret);
			for(int i=0; i<nfds; i++) 
				printf("%s: fd %d revents %08X\n", argv[0], ppfd[i].fd, ppfd[i].revents);
			break;
		}
		else {
			printf("%s: ret %d errno(%d)\n", argv[0], ret, errno);
			return ret;
		}

		continue;
	}

	return ret;
}
