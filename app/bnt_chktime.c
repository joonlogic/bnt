/****************************************************************************
 * bnt_chktime.c : Application for checking elapsed time to read nonce result
 *
 * Copyright (c) 2018  TheFrons, Inc.
 * Copyright (c) 2018  Joon Kim <joon@thefrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <bnt_def.h>
#include <bnt_ext.h>

typedef struct {
	int            nboards;
	int            nchips;
} T_OptInfo;

static void print_usage(const char *prog)
{
	printf("Usage: %s [-b] [nboards] [-c] [nchips] \n", prog);
	puts("  -b --nboards  nboards(default 0). Range(0~3)\n"
	     "  -c --nchips   nchips(default 0). Range(0~256)\n");
	exit(1);
}

static int parse_opts(int argc, char *argv[], T_OptInfo* info)
{
	while (1) {
		static const struct option lopts[] = {
			{ "nboards", 1, 0, 'b' },
			{ "nchips",  1, 0, 'c' },
			{ "help",    0, 0, 'h' },
			{ NULL,      0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "b:c:h", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'b':
			info->nboards = atoi(optarg);
			break;
		case 'c':
			info->nchips = atoi(optarg);
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

	int spifd[4] = {0,};
	int bid = 0;
	int cid = 0;
	T_BntHashMRR mrr = {0,};

	for(bid=0; bid<info.nboards; bid++) {
		spifd[bid] = bnt_spi_open(0, bid);
		if(spifd[bid] < 0) {
			printf("NOT DETECTED BOARD %d\n", bid);
			continue;
		}
	}

	time_t ntime = time(NULL);
	printf("READY %s\n", ctime(&ntime));

	for(bid=0; bid<info.nboards; bid++) {
		for(cid=0; cid<info.nchips; cid++) 
			bnt_read_mrr(spifd[bid], cid, &mrr);

		close(spifd[bid]);
	}

	ntime = time(NULL);
	printf("TIMEOUT %s\n", ctime(&ntime));


	return 0;
} 

