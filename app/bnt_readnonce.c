/********************************************************************
 * bnt_reset.c : Application for reseting chips warmly
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
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <bnt_def.h>
#include <bnt_ext.h>

typedef struct {
	int            boardid;
	int            chipid;
} T_OptInfo;

static void print_usage(const char *prog)
{
	printf("Usage: %s [-b] [boardid] [-c] [chipid] \n", prog);
	puts("  -b --boardid  board id(default 0). Range(0~3)\n"
	     "  -c --chipid   SPI chip id(default 0). Range(0~63)\n"
	exit(1);
}

static int parse_opts(int argc, char *argv[], T_OptInfo* info)
{
	while (1) {
		static const struct option lopts[] = {
			{ "boardid", 1, 0, 'b' },
			{ "chipid",  1, 0, 'c' },
			{ "help",    0, 0, 'v' },
			{ NULL,      0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "b:c:h", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'b':
			info->boardid = atoi(optarg);
			info->allboards = false;
			break;
		case 'c':
			info->chipid = atoi(optarg);
			info->allboards = false;
			info->allchips = false;
			break;
		case 'h':
		default:
			print_usage(argv[0]);
			break;
		}
	}

	return 0; 
}

int main(int argc, char *argv[])
{
	int ret = 0;
	T_OptInfo info = {
	};

	ret = parse_opts(argc, argv, &info);
	BNT_CHECK_RESULT(ret, ret);

	int sure = 0;
	int spifd = 0;

	spifd = do_open(0, info.boardid);
	if(spifd < 0) {
		printf("NOT DETECTED BOARD %d\n", info.boardid);
		return 0;
	}

	T_BntHashMRR mrr = {0,};
	bnt_read_mrr(spifd, info.chipid, &mrr);
	
	unsigned short mask = 0;
	unsigned short realnonce = 0;
	regread(spifd, info.chipid, SSR, &mask, sizeof(mask));
	mask >>= I_SSR_MASK; 

	printf("MASK       : %02X\n", mask);
	realnonce = bnt_get_realnonce(mrr->nonceout, (unsigned char)mask);

	printf("OUT NONCE  : %08X\n", mrr->nonceout);
	printf("REAL NONCE : %08X\n", realnonce);

	close(spifd);

	return 0;
} 

