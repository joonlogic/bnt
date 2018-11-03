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
	bool           autodetect;
} T_OptInfo;

static void print_usage(const char *prog)
{
	printf("\nUsage: %s [-b] <nBoards> [-c] <nChips per board> [-rw] <file>\n", 
			prog);
	puts("  -b --nboards      number of boards in system. Range(1~4)\n"
	     "  -c --nchips       number of chips per board. Range(1~64)\n"
	     "  -f --fast         fast mode (high speed clock)\n"
	     "  -t --ntroll       ntime rolling offset(1,2,3)\n"
	     "  -o --ntimeoffset  ntime data offset from input blockheader\n"
	     "  -m --mask         override nonce mask\n"
	     "  -p --ntplus       ntime rolling plus\n"
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
			{ "fast",          0, 0, 'f' },
			{ "help",          0, 0, 'h' },
			{ "read",          1, 0, 'r' },
			{ "write",         1, 0, 'w' },
			{ NULL,            0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "b:c:r:w:t:o:m:fash", lopts, NULL);

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

#ifndef DEMO
	BNT_INFO(("MASK %03X %s\n", handle->mask, info->nmask ? "(overide)" : ""));
#endif
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
	bool nobreak = false;
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

#ifdef DEMO
	char strstatus[32] = {0,};
	char strtime[32] = {0,};
	char strtarget[65] = {0,};
	T_BNT_WEBHANDLE notihandle = {
		.status = strstatus,
		.time = strtime,
		.target = strtarget,
	};

	bnt_set_status_noti_web(&notihandle, "ready", 0, 0, 0);
#endif

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


	T_BntHash bhash = {0,};

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
//	printf("\t--> Press any key to continue...");
#ifdef DEMO
	ConsoleInitialize();
	Plx_getch();
#else
//	getchar();
#endif

#ifndef USE_BNT_RESET
	//initialize
	BNT_PRINT(("BNT SYSTEM  : Aracore-Miner version 1.0.0\n"));
	BNT_PRINT(("              %-3d Engines Working\n", handle.nboards*handle.nchips*8));
	BNT_PRINT(("              %-3d FPGA Installed\n", handle.nboards*handle.nchips));
	BNT_PRINT(("\n"));

	ret = bnt_init(&handle, &info);
	BNT_CHECK_RESULT(ret, ret);
	nobreak = true;
#endif

	//GPIO interrupt
	bnt_config_gpio_irq(17, true);
	bnt_config_gpio_irq(18, true);

	//process one by one
	do {
		readlen = fread((void*)&bhash.bh, sizeof(bhash.bh), 1, handle.bhfp);
		if(readlen <= 0) break;

#ifdef DEMO
		//Ready
		Cons_clear();
		Cons_printf(
			"\n\n"
			"\t\t        BNT Test Application for mining \n"
			"\t\t                Sep 2018\n\n"
			);

#endif

		ntime = time(NULL);
		start_time = ntime;
#ifdef USE_BNT_RESET
		//initialize
		BNT_PRINT(("BNT SYSTEM  : Aracore-Miner version 1.0.0\n"));
		BNT_PRINT(("              %-3d Engines Working\n", handle.nboards*handle.nchips*8));
		BNT_PRINT(("              %-3d FPGA Installed\n", handle.nboards*handle.nchips));
		BNT_PRINT(("\n"));
#endif
		BNT_PRINT(("[[ %d ]] START %s-------------------------------------------\n", count+1, ctime(&ntime)));
#ifdef USE_BNT_RESET
		ret = bnt_init(&handle, &info);
		BNT_CHECK_RESULT(ret, ret);
#endif

#ifdef USE_INTERRUPT
		//setting interrupts 
		bnt_set_interrupt(-1, 0, IntAll, false, true, &handle);
		bnt_set_interrupt(-1, 0, IntAll, true, true, &handle);
#endif
		ret = bnt_get_midstate(&bhash);
		BNT_CHECK_RESULT(ret, -1);
		
		if(info.ntimeoffset) {
			bhash.bh.ntime -= info.ntimeoffset;
			printf("%s: bhash.bh.ntime %08X, info.ntroll %d, info.ntimeoffset %d\n", __func__, bhash.bh.ntime, info.ntroll, info.ntimeoffset);
		}
		bhash.workid++ == 0xFF ? bhash.workid++ : bhash.workid;

#ifdef DEMO
		memset(notihandle.target, 0x00, 65);
		bnt_get_targetstr(bhash.bh.bits, notihandle.target);
		bnt_set_status_noti_web(&notihandle, "mining", bhash.workid, ctime(&ntime), 0);
		printout_bh(&bhash.bh);

#else
		printout_bh(&bhash.bh);
		printout_hash(bhash.midstate, "Mid State   ");
#endif

		ret = bnt_getnonce(&bhash, &handle, nobreak);

		ntime = time(NULL);
		BNT_PRINT(("[%d] Workid %d Passed ( %ld sec consumed ) : DATE %s \n\n", 
				count+1, bhash.workid, ntime - start_time, ctime(&ntime)));
		count++;

#ifdef DEMO
		if((ret == 27) || (ret == 'p')){ //ESC or 'p'
			if(ret == 27) {
				BNT_PRINT(("BYE--------------------------------\n\n"));
				break;
			}
			else if(ret == 'p') {
				bnt_set_status_noti_web(&notihandle, "ready", 0, 0, 0);
				printf("\t--> Press any key to continue...");
				Plx_getch();
			}
		}
		else {
			bnt_set_status_noti_web(&notihandle, "mined", bhash.workid, ctime(&ntime), bhash.bh.nonce);

			sleep(10);

			//ready for 10 secondes...
			bnt_set_status_noti_web(&notihandle, "ready", 0, 0, 0);
			sleep(10);
		}
#endif
	} while(1);

#ifdef DEMO
	bnt_set_status_noti_web(&notihandle, "ready", 0, 0, 0);
#endif
	bnt_close(&handle);

	return ret;
}
