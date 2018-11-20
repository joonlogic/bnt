/********************************************************************
 * bnt_getnonce.c : Application for verifying BNT hash engines
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
#include <time.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <bnt_def.h>
#include <bnt_ext.h>
#ifdef DEMO
#include "bnt_demoext.h"
#include "ConsFunc.h"
#endif

#define USE_INTERRUPT
#define MAX_FILENAME_STR            128
typedef struct {
	int            nboards;
	int            nchips;
	int            ntroll;
	int            ntimeoffset;
	int            ntplus;
	unsigned short nmask;
	char*          infile;
	char*          outfile;
	bool           fast;
	bool           queue;
	bool           autodetect;
} T_OptInfo;

static void print_usage(const char *prog)
{
	printf("\nUsage: %s [-b] <nBoards> [-c] <nChips per board> [-rw] <file>\n", 
			prog);
	puts("  -b --nboards      number of boards in system. Range(1~4)\n"
	     "  -c --nchips       number of chips per board. Range(1~128)\n"
	     "  -a --auto         auto configuration.\n"
	     "  -f --fast         fast mode (high speed clock)\n"
	     "  -t --ntroll       ntime rolling offset(1,2,3)\n"
	     "  -o --ntimeoffset  ntime data offset from input blockheader\n"
	     "  -m --mask         override nonce mask\n"
	     "  -p --ntplus       ntime rolling plus\n"
	     "  -q --queue        queueing hash data using internal fifo\n"
	     "  -r --read         input block header sample file\n"
	     "  -w --write        write log to file (NYI)\n");
	exit(1);
}

static int parse_opts(int argc, char *argv[], T_OptInfo* info)
{
	while (1) {
		static const struct option lopts[] = {
			{ "nboards",       1, 0, 'b' },
			{ "nchips",        1, 0, 'c' },
			{ "ntroll",        1, 0, 'n' },
			{ "ntimeoffset",   1, 0, 'o' },
			{ "mask",          1, 0, 'm' },
			{ "ntplus",        0, 0, 'p' },
			{ "auto",          0, 0, 'a' },
			{ "queue",         0, 0, 'q' },
			{ "fast",          0, 0, 'f' },
			{ "help",          0, 0, 'h' },
			{ "read",          1, 0, 'r' },
			{ "write",         1, 0, 'w' },
			{ NULL,            0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "b:c:r:w:t:o:m:faqsh", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'b':
				info->nboards = atoi(optarg);
				info->autodetect = false;
				break;
			case 'c':
				info->nchips = atoi(optarg);
				info->autodetect = false;
				break;
			case 't':
				info->ntroll = atoi(optarg);
				break;
			case 'o':
				info->ntimeoffset = atoi(optarg);
				break;
			case 'm':
				info->nmask = strtol(optarg, NULL, 16);
				break;
			case 'a':
				info->autodetect = true;
				break;
			case 'r':
				strcpy(info->infile, optarg);
				break;
			case 'w':
				strcpy(info->outfile, optarg);
				break;
			case 'f':
				info->fast = true;
				break;
			case 's':
				info->ntplus = true;
				break;
			case 'q':
				info->queue = true;
				break;
			case 'h':
			default:
				print_usage(argv[0]);
				break;
		}
	}

	switch(info->nboards) {
		case 0:
		case 1:
		case 2:
		case 4:
			break;
		default:
			printf("Out of range nboards %d\n", info->nboards);
			print_usage(argv[0]);
			break;
	}

	switch(info->nchips) {
		case 0:
		case 1:
		case 2:
		case 4:
		case 8:
		case 16:
		case 32:
		case 64:
		case 128:
			break;
		default:
			printf("Out of range nchips %d\n", info->nchips);
			print_usage(argv[0]);
			break;
	}

	switch(info->ntroll) {
		case 0:
		case 1:
		case 2:
		case 3:
			break;
		default:
			printf("Out of range ntime roll %d\n", info->ntroll);
			print_usage(argv[0]);
			break;
	}


	if(!strlen(info->infile)) {
		printf("Missing input filename\n");
		print_usage(argv[0]);
	}

	if(access(info->infile, R_OK)) {
		printf("Error in accessing file %s\n", info->infile);
		print_usage(argv[0]);
	}

	return 0; 
}

int 
bnt_init(
		T_BntHandle* handle,
		T_OptInfo* info 
		)
{
	//reset
	for(int i=0; i<MAX_NBOARDS; i++) {
		if(handle->spifd[i] <= 0) continue;
		bnt_softreset(handle->spifd[i], 0, true);
	}

	//set GPIO irq
	//TODO:

	/*
	system("raspi-gpio set 5 dl");
	sleep(1);
	system("raspi-gpio set 5 dh");
	*/

	BNT_INFO(("MASK %03X %s\n", handle->mask, info->nmask ? "(overide)" : ""));
	handle->ssr = handle->mask << I_SSR_MASK;
	
#if 0
	handle->ssr |= (1<<2); //64bit
	BNT_INFO(("64 Bits mode\n"));
#endif

	unsigned short ssr = 0;
	unsigned short tsr = 0;

	for(int i=0; i<MAX_NBOARDS; i++) {
		if(handle->spifd[i] <= 0) continue;

		//SSR
		ssr = htons(handle->ssr | (i << I_SSR_BOARDID) | ((info->ntplus ? 1 : 0) << I_SSR_PLUSMODE));
		bnt_write_board(
				SSR, 
				&ssr,
				sizeof(ssr),
				i,
				handle
				);

		//TSR
		tsr = htons(info->ntroll);
		bnt_write_board(
				TSR,
				&tsr,
				sizeof(tsr),
				i,
				handle
				);

	}

#ifdef FPGA
	if(info->fast) {
		unsigned short psr = htons(0x0001);
		bnt_write_all(
				PSR,
				&psr,
				sizeof(psr),
				handle
				);
	}
#else
	//set PLL
#endif

	//Verify and logging
	for(int board=0; board<MAX_NBOARDS; board++) {
		if(handle->spifd[board] <= 0) continue;
		unsigned short ssr_wr = handle->ssr | (board << I_SSR_BOARDID);
		for(int chip=0; chip<handle->nchips; chip++) {
			regread(
					handle->spifd[board], 
					chip,
					SSR,
					&ssr,
					sizeof(ssr),
					false,
					false
				   );
			ssr = ntohs(ssr) & 0xFFF4;

			if(ssr != ssr_wr) {
				int retry = 0;
				do {
					regwrite(handle->spifd[board], 0, SSR, &ssr, sizeof(ssr), (int)true, false);
					regread(handle->spifd[board], chip, SSR, &ssr, sizeof(ssr), false, false);
					ssr = ntohs(ssr) & 0xFFF4;
				} while((ssr != ssr_wr) && (retry++ < 5));

				BNT_PRINT(("Retried:[%d][%03d] SSR %04X SSR_WR %04X\n", board, chip, ssr, ssr_wr)); 
			}
			ssr = 0; 
		}
	}

	return 0;
}


int 
bnt_close(
		T_BntHandle* handle
		)
{
	fclose(handle->bhfp);
	for(int i=0; i<MAX_NBOARDS; i++) {
		if(handle->spifd[i] <= 0) continue;
		close(handle->spifd[i]);
	}

	//GPIO interrupt
	bnt_config_gpio_irq(17, false);
	bnt_config_gpio_irq(18, false);

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
	time_t ntime, start_time;
	unsigned int count = 0;
	unsigned int readlen = 0;
	char infile[MAX_FILENAME_STR]={0,};
	char outfile[MAX_FILENAME_STR]={0,};
	T_OptInfo info = {
		.nboards = 0,
		.nchips = 0,
		.infile = infile,
		.outfile = outfile,
		.autodetect = true,
	};
	char regbuf[256] = {0,};
	bool mined[300] = {0,};

	if(argc == 1) print_usage(argv[0]);

	ret = parse_opts(argc, argv, &info);
	BNT_CHECK_RESULT(ret, ret);

	if(info.autodetect) {
		ret = bnt_devscan(&info.nboards, &info.nchips);
		BNT_CHECK_RESULT(ret, ret);
	}

	//prepare data structure
	T_BntHandle handle = {
		.nboards = info.nboards,
		.nchips = info.nchips,
		.ntroll = info.ntroll,
		.ntrollplus = (int)info.ntplus,
	};


	T_BntHash bhash[256] = {0,}; //MAX 256 test sets
	T_BntHash* pbh;

	//open input blockheader file
	handle.bhfp = fopen(info.infile, "r");
	BNT_CHECK_NULL(handle.bhfp, -1);

	//open SPI
	for(int i=0; i<MAX_NBOARDS; i++) {
		handle.spifd[i] = bnt_spi_open(0, i);
		if(!hello_there(handle.spifd[i], 0, false)) {
			close(handle.spifd[i]);
			handle.spifd[i] = -1;
			continue;
		}
		printf("Open Board %d\n", i);
	}

	//set Mask
	handle.mask = info.nmask ? info.nmask : bnt_get_nonce_mask(handle.nboards, handle.nchips);
	BNT_INFO(("NONCE MASK %03X %s\n", handle.mask, info.nmask ? "(overide)" : ""));
	printf("\t--> Press any key to continue...");
	getchar();

	//initialize
	BNT_PRINT(("BNT SYSTEM  : Aracore-Miner version 1.0.0\n"));
	BNT_PRINT(("              %-3d Engines Working\n", handle.nboards*handle.nchips*8));
	BNT_PRINT(("              %-3d FPGA Installed\n", handle.nboards*handle.nchips));
	BNT_PRINT(("\n"));

	ret = bnt_init(&handle, &info);
	BNT_CHECK_RESULT(ret, ret);

#ifdef USE_INTERRUPT
	//GPIO interrupt
	bnt_config_gpio_irq(17, true);
	bnt_config_gpio_irq(18, true);
#endif

	unsigned char workid = 0;

	do {
		pbh = &bhash[++workid];
		readlen = fread((void*)&pbh->bh, sizeof(pbh->bh), 1, handle.bhfp);
		if(readlen <= 0) break;

		pbh->workid = workid;
		ret = bnt_get_midstate(pbh);
		BNT_CHECK_RESULT(ret, -1);

		if(info.ntimeoffset) {
			pbh->bh.ntime -= info.ntimeoffset;
			printf("%s: pbh->bh.ntime %08X, info.ntroll %d, info.ntimeoffset %d\n", __func__, pbh->bh.ntime, info.ntroll, info.ntimeoffset);
		}

		ntime = time(NULL);
		BNT_PRINT(("[[ %d ]] WorkId (%d) REQUEST %s-------------------------------------------\n", \
					count, pbh->workid, ctime(&ntime)));
		printout_bh(&pbh->bh);
		printout_hash(pbh->midstate, "Mid State   ");

		bnt_request_hash(pbh, &handle);

		//register dump for just board 0 chip 0
		regdump(handle.spifd[0], 0, regbuf, false);
		printf("[BNT REG %s] Board %d Chip %d %s\n", "READ", 0, 0, "");
		printreg(regbuf, ENDOF_BNT_REGISTERS, 0x00);

	} while(count++<255);

	BNT_PRINT(("/////////////////////////////////////////////\n"));
	BNT_PRINT(("All Data Requested %d\n", pbh->workid));
	BNT_PRINT(("/////////////////////////////////////////////\n"));

	bool isvalid;
	T_BntHashMRR mrr={0,};

	ntime = time(NULL);
	start_time = ntime;
	BNT_PRINT(("Mining Start %s-------------------------------------------\n", ctime(&ntime)));

	do {
		for(int board=0; board<handle.nboards; board++) {
			for(int chip=0; chip<handle.nchips; chip++) {
				bnt_read_workid(
						handle.spifd[board],
						chip,
						&mrr.extraid,
						&mrr.workid
						);
				if(mrr.workid == 0) continue;

				bnt_read_mrr(
						handle.spifd[board],
						chip,
						&mrr
						);
				printf("MRR : workid(%d) extraid(%d) - %08X\n",
						mrr.workid, mrr.extraid, mrr.nonceout);
				if(mined[mrr.workid] == true) continue;

				pbh = &bhash[mrr.workid];
				isvalid = bnt_test_validnonce_out(pbh, &mrr, &handle, board, chip);
				if(isvalid) {
					//found it
					bnt_printout_validnonce(board, chip, pbh);
					mined[mrr.workid] = true;
				}
			}
		}
		sleep(1);
	} while(!mined[255]);

	ntime = time(NULL);
	BNT_PRINT(("[Mining Done] ( %ld sec waited ) : DATE %s \n\n", 
			ntime - start_time, ctime(&ntime)));

	for(int i=1; i<256; i++) {
		BNT_PRINT(("MINED[%d] = %s\n", i, mined[i] ? "OK" : "MISSED"));
	}

	bnt_close(&handle);

	return ret;
}
