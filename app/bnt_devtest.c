/********************************************************************
 * bnt_devtest.c : Application for verifying BNT registers access
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
	unsigned int   speed;
	bool           isall;
	bool           isnchips;
	bool           verbose;
	unsigned char* buf;
} T_OptInfo;

static void print_usage(const char *prog)
{
	printf("Usage: %s [-bcnsav]\n", prog);
	puts("  -b --boardid  board id(default 0). Range(0~3)\n"
	     "  -c --chipid   spi chip id(default 0). Range(0~63)\n"
	     "  -n --nchips   number of chips per board. (default 1)\n"
	     "  -s --speed    max speed (Hz)\n"
	     "  -a --all      all boards & chips\n"
	     "  -v --verbose  Verbose (show log)\n");
	exit(1);
}

static int parse_opts(int argc, char *argv[], T_OptInfo* info)
{
	while (1) {
		static const struct option lopts[] = {
			{ "boardid", 1, 0, 'b' },
			{ "chipid",  1, 0, 'c' },
			{ "nchips",  1, 0, 'n' },
			{ "speed",   1, 0, 's' },
			{ "all",     0, 0, 'a' },
			{ "verbose", 0, 0, 'v' },
			{ NULL,      0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "b:c:n:s:av", lopts, NULL);

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
			info->nchips = atoi(optarg);
			info->isnchips = true;
			break;
		case 's':
			info->speed = atoi(optarg);
			break;
		case 'v':
			info->verbose = true;
			break;
		case 'a':
			info->isall = true;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}

	return 0; 
}

void regreadHVR(
		int fd,
		int chipid,
		unsigned char* rbuf
		)
{
	BNT_CHECK_NULL(rbuf,);

	/*
	for(int i=0; i<SIZE_TOTAL_HVR_BYTE/SIZE_REG_DATA_BYTE; i+=3, rbuf+=(SIZE_REG_DATA_BYTE*3)) {
		regread(fd, chipid, HVR0+i, rbuf, SIZE_REG_DATA_BYTE*3, false);
	}
	*/
	for(int i=0; i<SIZE_TOTAL_HVR_BYTE/SIZE_REG_DATA_BYTE; i++, rbuf+=(SIZE_REG_DATA_BYTE*1)) {
		regread(fd, chipid, HVR0+i, rbuf, SIZE_REG_DATA_BYTE*1, false);
	}
} 

bool do_memtest(
		int fd,
		int csidx,
		int chipid,
		unsigned char pattern,
		bool bcast
		)
{
	unsigned char buf[SIZE_TOTAL_HVR_BYTE+1] = {0,};
	unsigned char rbuf[SIZE_TOTAL_HVR_BYTE+16] = {0,};
	int cmp = 0;
	int count = 0;

	printf("\t[%d][%02d] TEST PATTERN %02X%02X : ", csidx, chipid, pattern, pattern);
	memset(buf, pattern, SIZE_TOTAL_HVR_BYTE);
	regwrite(fd, chipid, HVR0, buf, SIZE_TOTAL_HVR_BYTE, (int)bcast, false);

	buf[0] = 0; //RO at HVR0

	do {
		regreadHVR(fd, chipid, rbuf);

		cmp = memcmp(buf, rbuf, SIZE_TOTAL_HVR_BYTE);
		if(cmp) {
			printf("FAILED!\nREAD REGISTER ---- RETRY %d\n", count);
			printreg(rbuf, SIZE_TOTAL_HVR_BYTE/SIZE_REG_DATA_BYTE, HVR0);
		}
		else {
			printf("VERIFIED\n");
			break;
		}
	} while(count++<5);

	return cmp ? false : true;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	bool result;
	int fd;
	unsigned char buf[512]={0,};
	T_OptInfo info = {
		.buf = buf,
	};

	ret = parse_opts(argc, argv, &info);
	BNT_CHECK_RESULT(ret, ret);

	//set board/chip range
	int cs_start, cs_end, csidx;
	int chipid_start, chipid_end, chipid_step;

	cs_start = info.isall ? 0 : info.boardid;
	cs_end = info.isall ? MAX_SPI_CS : info.boardid;
	csidx = cs_start;

	chipid_start = 
		info.isall ? 0 : 
		info.isnchips ? 0 : info.chipid;
	chipid_end = 
		info.isall ? MAX_CHIPID :
		info.isnchips ? LAST_CHIPID(info.nchips) : info.chipid;

	chipid_step = info.isnchips ? 1 << bnt_get_id_shift(info.nchips) : 1;

    //loop
	unsigned char pattern[] = {0xFF, 0x00, 0xA5, 0x5A};

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
		}

		for(int chipid=chipid_start; chipid<=chipid_end; chipid += chipid_step) {
			if(hello_there(fd, chipid, false) == false) continue;

			bnt_softreset(fd, chipid, false);

			//1. For each mode
			printf("[[%d][%02d]] (1) Write & Read (for each) ---------------------\n", csidx, chipid);
			for(int i=0; i<sizeof(pattern); i++) {
				result = do_memtest(fd, csidx, chipid, pattern[i], false); 
				if(!result) break;
			}
			if(!result) continue;

			//2. Broadcast write mode
			printf("[[%d][%02d]] (2) Write & Read (Broadcast) -------------------\n", csidx, chipid);
			for(int i=0; i<sizeof(pattern); i++) {
				result = do_memtest(fd, csidx, chipid, pattern[i], true); 
				if(!result) break;
			}
			if(!result) continue;

			printf("\n");
		}

		close(fd);
	} while(csidx++ < cs_end);

	return ret;
}
