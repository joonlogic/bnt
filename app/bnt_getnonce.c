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
#include <getopt.h>
#include <arpa/inet.h>
#include <bnt_def.h>
#include <bnt_ext.h>

#define MAX_FILENAME_STR            128
typedef struct {
	int            nboards;
	int            nchips;
	char*          infile;
	char*          outfile;
	bool           autodetect;
} T_OptInfo;

static void print_usage(const char *prog)
{
	printf("\nUsage: %s [-b] <nBoards> [-c] <nChips per board> [-rw] <file>\n", 
			prog);
	puts("  -b --nboards  number of boards in system. Range(1~4)\n"
	     "  -c --nchips   number of chips per board. Range(1~64)\n"
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
			{ "help",    0, 0, 'h' },
			{ "read",    1, 0, 'r' },
			{ "write",   1, 0, 'w' },
			{ NULL,      0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "b:c:r:w:ah", lopts, NULL);

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
		T_BntHandle* handle
		)
{
	//reset
	for(int i=0; i<handle->nboards; i++) {
		bnt_softreset(handle->spifd[i], 0, true);
	}

	//set GPIO irq
	//TODO:

	//set Mask
	handle->mask = bnt_get_nonce_mask(handle->nboards, handle->nchips);
	BNT_INFO(("%s: mask %02X\n", __func__, handle->mask));
	handle->idshift = bnt_get_id_shift(handle->nchips);
	handle->ssr = ((unsigned short)handle->mask) << I_SSR_MASK;

	bnt_write_all(
			SSR, 
			&handle->ssr, 
			sizeof(handle->ssr),
			handle
			);

	//Verify and logging
	unsigned short ssr = 0;
	for(int board=0; board<handle->nboards; board++) {
		for(int chip=0; chip<handle->nchips; chip++) {
			regread(
					handle->spifd[board], 
					CHIPID_PHYSICAL(chip, handle), 
					SSR,
					&ssr,
					sizeof(ssr)
				   );

			printf("%s:[%d][%d] SSR %04X (physical chip id %02X)\n", 
					__func__, board, chip, ssr, chip<<handle->idshift);
			BNT_CHECK_TRUE(ssr==handle->ssr, -1);
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
	for(int i=0; i<handle->nboards; i++) {
		close(handle->spifd[i]);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int ret = 0;
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

	if(argc == 1) print_usage(argv[0]);

	ret = parse_opts(argc, argv, &info);
	BNT_CHECK_RESULT(ret, ret);

	if(info.autodetect) {
		ret = bnt_detect(&info.nboards, &info.nchips);
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
	for(int i=0; i<handle.nboards; i++) {
		handle.spifd[i] = do_open(0, i);
		BNT_CHECK_TRUE(handle.spifd[i] > 0, -1);
	}

	//process one by one
	do {
		//initialize
		ret = bnt_init(&handle);
		BNT_CHECK_RESULT(ret, ret);

		readlen = fread((void*)&bhash.bh, sizeof(bhash.bh), 1, handle.bhfp);
		if(readlen != sizeof(bhash.bh)) break;

		ret = bnt_get_midstate(&bhash);
		BNT_CHECK_TRUE(ret, -1);
		
		printout_bh(&bhash.bh);
		printout_hash(bhash.midstate);

		bhash.workid == 0 ? bhash.workid++ : bhash.workid;
		ret = bnt_getnonce(&bhash, &handle);

		printf("[%d] Workid %d Passed with %s.\n", 
				count++, bhash.workid, ret == 0 ? "SUCCESS" : "FAIL");

	} while(1);

	bnt_close(&handle);
	return ret;
}
