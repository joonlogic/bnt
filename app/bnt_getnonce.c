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

#define MAX_FILENAME_STR            128
typedef struct {
	int            nboards;
	int            nchips;
	char*          infile;
	char*          outfile;
	bool           fast;
	bool           autodetect;
} T_OptInfo;

static void print_usage(const char *prog)
{
	printf("\nUsage: %s [-b] <nBoards> [-c] <nChips per board> [-rw] <file>\n", 
			prog);
	puts("  -b --nboards  number of boards in system. Range(1~4)\n"
	     "  -c --nchips   number of chips per board. Range(1~64)\n"
	     "  -f --fast     fast mode (high speed clock)\n"
	     "  -r --read     input block header sample file\n"
	     "  -w --write    write log to file (NYI)\n");
	exit(1);
}

static int parse_opts(int argc, char *argv[], T_OptInfo* info)
{
	while (1) {
		static const struct option lopts[] = {
			{ "nboards", 1, 0, 'b' },
			{ "nchips",  1, 0, 'c' },
			{ "auto",    0, 0, 'a' },
			{ "fast",    0, 0, 'f' },
			{ "help",    0, 0, 'h' },
			{ "read",    1, 0, 'r' },
			{ "write",   1, 0, 'w' },
			{ NULL,      0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "b:c:r:w:fah", lopts, NULL);

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

	//set Mask
	handle->mask = bnt_get_nonce_mask(handle->nboards, handle->nchips);
#ifndef DEMO
	BNT_INFO(("MASK %02X\n", handle->mask));
#endif
	handle->idshift = bnt_get_id_shift(handle->nchips);
	handle->ssr = ((unsigned short)handle->mask) << I_SSR_MASK;
	
	handle->ssr |= (1<<2); //64bit
	BNT_INFO(("64 Bits mode\n"));

	unsigned short ssr = htons(handle->ssr);
	bnt_write_all(
			SSR, 
			&ssr,
			sizeof(ssr),
			handle
			);

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
#endif

	//Verify and logging
	for(int board=0; board<MAX_NBOARDS; board++) {
		if(handle->spifd[board] <= 0) continue;
		int chipid_step = 1 << bnt_get_id_shift(handle->nchips);
		for(int chip=0; chip<MAX_NCHIPS_PER_BOARD; chip+=chipid_step) {
			regread(
					handle->spifd[board], 
					chip,
					SSR,
					&ssr,
					sizeof(ssr),
					false
				   );
			ssr = ntohs(ssr) & 0xFF04;

			if(ssr != handle->ssr) {
				int retry = 0;
				do {
					ssr = htons(handle->ssr);
					regwrite(handle->spifd[board], 0, SSR, &ssr, sizeof(ssr), (int)true, false);
					regread(handle->spifd[board], chip, SSR, &ssr, sizeof(ssr), false);
					ssr = ntohs(ssr) & 0xFF00;
				} while((ssr != handle->ssr) && (retry++ < 5));

				BNT_PRINT(("Retried:[%d][%02d] SSR %04X\n", board, chip, ssr)); 
			}
//			BNT_CHECK_TRUE(ssr==handle->ssr, -1);
			ssr = 0; 
		}
	}

	//Interrupt enable
	bnt_set_interrupt(-1, 0, IntAll, true, true, handle);

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
		.autodetect = true
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

	handle.mask = bnt_get_nonce_mask(handle.nboards, handle.nchips);
	BNT_INFO(("NONCE MASK %02X\n", handle.mask));
	printf("\t--> Press any key to continue...");
#ifdef DEMO
	ConsoleInitialize();
	Plx_getch();
#else
	getchar();
#endif

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
		//initialize
		ntime = time(NULL);
		start_time = ntime;
//		BNT_PRINT(("[System Information] --------------------------------------\n"));
		BNT_PRINT(("BNT SYSTEM  : Aracore-Miner version 1.0.0\n"));
		BNT_PRINT(("              %-3d Engines Working\n", handle.nboards*handle.nchips*8));
		BNT_PRINT(("              %-3d FPGA Installed\n", handle.nboards*handle.nchips));
		BNT_PRINT(("\n"));
		BNT_PRINT(("[[ %d ]] START %s-------------------------------------------\n", count+1, ctime(&ntime)));

		ret = bnt_init(&handle, &info);
		BNT_CHECK_RESULT(ret, ret);

		ret = bnt_get_midstate(&bhash);
		BNT_CHECK_RESULT(ret, -1);
		
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

		ret = bnt_getnonce(&bhash, &handle);

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

/*			if(Cons_kbhit()) {
				int ch = Cons_getch();
				if(ch == 27) {
					BNT_PRINT(("BYE--------------------------------\n\n"));
					break;
				}
				else if(ch == 'p'){
					bnt_set_status_noti_web(&notihandle, "ready", 0, 0, 0);
					printf("\t--> Press any key to continue...");
					Plx_getch();
				}
				else sleep(10); 
			}
			else 
*/
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
