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
	bool           allboards;
	bool           allchips;
	bool           verbose;
} T_OptInfo;

static void print_usage(const char *prog)
{
	printf("Usage: %s [-b] [boardid] [-c] [chipid] \n", prog);
	puts("  -b --boardid  board id(default 0). Range(0~3)\n"
	     "  -c --chipid   SPI chip id(default 0). Range(0~63)\n"
	     "  -v --verbose  No Verbose (No Asking are you sure.)\n"
		 "  'no args' means all boards & all chips\n");
	exit(1);
}

static int parse_opts(int argc, char *argv[], T_OptInfo* info)
{
	while (1) {
		static const struct option lopts[] = {
			{ "boardid", 1, 0, 'b' },
			{ "chipid",  1, 0, 'c' },
			{ "verbose", 0, 0, 'v' },
			{ "help",    0, 0, 'h' },
			{ NULL,      0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "b:c:hv", lopts, NULL);

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
		case 'v':
			info->verbose = false;
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
		.allboards = true,
		.allchips = true,
		.verbose = true,
	};

	ret = parse_opts(argc, argv, &info);
	BNT_CHECK_RESULT(ret, ret);

	int sure = 0;
	int spifd = 0;

	if(info.allboards) printf("All boards & chips to be reset. ");
	else if(info.allchips) 
		printf("All chips on Board %d to be reset. ", info.boardid);
	else 
		printf("Chip id %d(0x%X) on Board %d to be reset. ", 
				info.chipid, info.chipid, info.boardid);

	if(info.verbose == true) {
		printf("Are You Sure? (Y/N) : ");

		sure = fgetc(stdin);
		if(sure != 'y' && sure != 'Y') return 0;
	}

	if(info.allboards) {
		for(int board=0; board<MAX_NBOARDS; board++) {
			spifd = bnt_spi_open(0, board);
			if(spifd < 0) break;

			bnt_softreset(spifd, 0, true);
			printf("RESET DONE All Chips on BOARD %d\n", board);
			close(spifd);
		}
	}
	else if(info.allchips) {
		spifd = bnt_spi_open(0, info.boardid);
		if(spifd < 0) {
			printf("NOT DETECTED BOARD %d\n", info.boardid);
			return 0;
		}
		bnt_softreset(spifd, 0, true);
		printf("RESET DONE All Chips on BOARD %d\n", info.boardid);
		close(spifd);
	}
	else {
		spifd = bnt_spi_open(0, info.boardid);
		if(spifd < 0) {
			printf("NOT DETECTED BOARD %d\n", info.boardid);
			return 0;
		}
		bnt_softreset(spifd, info.chipid, false);
		printf("RESET DONE CHIP %d on BOARD %d\n", info.chipid, info.boardid);
		close(spifd);
	}

	return 0;
} 

