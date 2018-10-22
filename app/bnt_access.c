/********************************************************************
 * bnt_access.c : Application for accessing BNT registers
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
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

typedef struct {
	int            boardid;
	int            chipid;
	int            nchips;
	int            count;
	int            addr;
	unsigned int   speed;
	int            verbose;
	bool           setboard; //set boardid by user
	bool           isbcast;
	bool           isdump;
	bool           isregscan;
	int            iswrite;
	unsigned char* buf;
} T_AccessInfo;

static void print_usage(const char *prog)
{
	printf("Usage: %s [-bcnav] [-rw] <REG_ADDR> [REG_VAL(s)]\n", prog);
	puts("  -b --boardid  board id(default 0). Range(0~3)\n"
	     "  -c --chipid   spi chip id(default 0). Range(0~63)\n"
	     "  -n --count    count (default 1). Range(1-23)\n"
	     "  -s --speed    max speed (Hz)\n"
	     "  -a --all      all boards & chips (broadcast write)\n"
	     "  -d --dump     Dump all registers\n"
	     "  -r --read     read\n"
	     "  -w --write    write\n"
	     "  -i --regscan  get registers for each chips\n"
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
			{ "speed",   1, 0, 's' },
			{ "read",    1, 0, 'r' },
			{ "write",   1, 0, 'w' },
			{ "dump",    0, 0, 'd' },
			{ "all",     0, 0, 'a' },
			{ "regscan", 1, 0, 'i' },
			{ "verbose", 0, 0, 'v' },
			{ NULL,      0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "b:c:n:s:r:w:i:avd", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'b':
			info->boardid = atoi(optarg);
			info->setboard = true;
			break;
		case 'c':
			info->chipid = atoi(optarg);
			break;
		case 'n':
			info->count = atoi(optarg);
			break;
		case 's':
			info->speed = atoi(optarg);
			break;
		case 'v':
			info->verbose = 1;
			break;
		case 'a':
			info->isbcast = true;
			break;
		case 'i':
			info->isregscan = true;
			info->nchips = atoi(optarg);
			break;
		case 'd':
			info->isdump = true;
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
	unsigned char buf[512]={0,};
	T_AccessInfo info = {
		.count = 1,
		.buf = buf,
	};

	if(argc == 1) print_usage(argv[0]);

	ret = parse_opts(argc, argv, &info);
	BNT_CHECK_RESULT(ret, ret);

	//set board range
	if(info.isbcast && info.iswrite && !info.setboard) {
		cs_start = 0;
		cs_end = MAX_SPI_CS;
	} 
	else {
		cs_start = info.boardid;
		cs_end = info.boardid;
	} 

	int csidx = cs_start;
	do {
		fd = bnt_spi_open(BUS_SPI(0), csidx); 
		if(fd < 0) {
			printf("%s: SPI open error. board %d cs %d fd %d\n", 
					argv[0], 0, csidx, fd);
			continue;
		}

		//max speed
		if(info.speed != 0) {
			ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &info.speed);
			if(ret < 0) {
				printf("%s: Can't set max speed %d HZ\n", 
						argv[0], info.speed);
			}

			ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &info.speed);
			if(ret < 0) {
				printf("%s: Can't get max speed.\n", argv[0]);
			}
			else {
				printf("%s: max speed %d Hz (%d kHz)\n", 
						argv[0], info.speed, info.speed/1000);
			}
		}

		nbytes = info.count << 1;

		ret = info.isdump ? regdump(fd, info.chipid, info.buf, info.verbose) :
			  info.isregscan ? regscan(fd, info.addr, info.buf, info.nchips) :
			  info.iswrite ? \
				  regwrite(fd, info.chipid, info.addr, info.buf, 
						  nbytes, info.isbcast, info.verbose) :
				  regread(fd, info.chipid, info.addr, info.buf, nbytes, info.verbose);

		printf("[BNT REG %s] Board %d Chip %d %s\n", 
				info.isregscan ? "REGISTER SCAN" :
					info.iswrite ? "WRITE" : "READ", csidx, info.chipid, 
				info.iswrite ? info.isbcast ? "Broadcast" : "" : 
					info.isregscan ? "~ 64" : "");

		info.isdump ? \
			printreg(info.buf, ENDOF_BNT_REGISTERS, 0x00) :
			info.isregscan ? printreg(info.buf, MAX_NCHIPS_PER_BOARD, 0x00) :
				printreg(info.buf, info.count, info.addr);
		close(fd);
	} while(csidx++ < cs_end);

	return ret;
}
