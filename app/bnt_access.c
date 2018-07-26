/********************************************************************
 * bnt_access.c : Application for accessing BNT registers
 *
 * Copyright (c) 2018  TheFrons, Inc.
 * Copyright (c) 2018  Joon Kim <joonlogic@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 ********************************************************************/
#include <stdio.h>
#include <stdlib.h>
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
	int            count;
	int            addr;
	int            verbose;
	int            isbcast;
	int            iswrite;
	unsigned char* buf;
} T_AccessInfo;

static void print_usage(const char *prog)
{
	printf("Usage: %s [-bcnav] [-rw] <REG_ADDR> [REG_VAL(s)]\n", prog);
	puts("  -b --boardid  board id(default 0). Range(0~3)\n"
	     "  -c --chipid   chip id(default 0). Range(0~63)\n"
	     "  -n --count    count (default 1). Range(1-23)\n"
	     "  -a --all      all boards & chips (broadcast write)\n"
	     "  -r --read     read\n"
	     "  -w --write    write\n"
	     "  -v --verbose  Verbose (show tx/rx buffer)\n"
	     "  REG_ADDR      register address (hex)\n"
	     "  REG_VAL(s)    register value(s) (ex> A5A5 3C3C ...)\n");
	exit(1);
}

static int parse_opts(int argc, char *argv[], T_AccessInfo* info)
{
	while (1) {
		static const struct option lopts[] = {
			{ "boardid", 1, 0, 'b' },
			{ "chipid",  1, 0, 'c' },
			{ "count",   1, 0, 'n' },
			{ "read",    1, 0, 'r' },
			{ "write",   1, 0, 'w' },
			{ "all",     0, 0, 'a' },
			{ "verbose", 0, 0, 'v' },
			{ NULL,      0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "b:c:n:r:w:av", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'b':
			info->boardid = atoi(optarg);
			break;
		case 'c':
			info->chipid = atoi(optarg);
			break;
		case 'n':
			info->count = atoi(optarg);
			break;
		case 'v':
			info->verbose = 1;
			break;
		case 'a':
			info->isbcast = 1;
			break;
		case 'r':
			info->iswrite = 0;
			info->addr = strtol(optarg, NULL, 16);
			break;
		case 'w':
			info->iswrite = 1;
			info->addr = strtol(optarg, NULL, 16);
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}

	if(info->addr == ERANGE) {
		printf("Invalid register address!\n");
		return -1;
	}

	if(info->count > MAX_COUNT_REG_ACCESS) {
		printf("count %d is out of range!(1-23)\n", info->count);
		return -1;
	}

	if(info->iswrite) { 
		if(argc - optind != info->count) {
			printf("Input register value(s) to write as many as count(s)\n");
			return -1;
		}
	}

	if(optind < argc) {
		unsigned short* bufp = (unsigned short*)info->buf;
		while (optind < argc) {
			*bufp++ = htons(strtol(argv[optind++], NULL, 16));
//			sscanf(argv[optind++], "%4x", *bufp++);
		}
	}

	return 0; 
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;
	int nbytes = 0;
	int cs_start, cs_end;
	unsigned char buf[MAX_LENGTH_BNT_SPI]={0,};
	T_AccessInfo info = {
		.count = 1,
		.buf = buf,
	};

	if(argc == 1) print_usage(argv[0]);

	ret = parse_opts(argc, argv, &info);
	BNT_CHECK_RESULT(ret, ret);

	//set board range
	if(info.isbcast && info.iswrite) {
		cs_start = 0;
		cs_end = MAX_SPI_CS;
	} 
	else {
		cs_start = info.boardid;
		cs_end = info.boardid;
	} 

	int csidx = cs_start;
	do {
		fd = do_open(BUS_SPI(0), csidx); 
		if(fd < 0) {
			printf("%s: SPI open error. board %d cs %d fd %d\n", 
					__func__, 0, csidx, fd);
			continue;
		}

		nbytes = info.count << 1;

		ret = info.iswrite ? \
			  regwrite(fd, info.chipid, info.addr, info.buf, 
					  nbytes, info.isbcast) :
			  regread(fd, info.chipid, info.addr, info.buf, nbytes);

		printf("[BNT REG %s] Board %d Chip %d %s\n", 
				info.iswrite ? "WRITE" : "READ", csidx, info.chipid, 
				info.iswrite ? info.isbcast ? "Broadcast" : "" : "");
		regdump(info.buf, info.count, info.addr);
		close(fd);
	} while(csidx++ < cs_end);

	return ret;
}
