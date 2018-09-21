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
#include <bnt_demoext.h>

typedef struct {
	int            status;
} T_OptInfo;

static void print_usage(const char *prog)
{
	printf("Usage: %s [-s] status(0:ready, 1:mining, 2:mined)\n", prog);
	exit(1);
}

static int parse_opts(int argc, char *argv[], T_OptInfo* info)
{
	while (1) {
		static const struct option lopts[] = {
			{ "status",  1, 0, 's' },
			{ NULL,      0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "s:h", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 's':
			info->status = atoi(optarg);
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
		.status = 0,
	};

	ret = parse_opts(argc, argv, &info);
	BNT_CHECK_RESULT(ret, ret);

    char strstatus[32] = {0,};
	char strtime[32] = {0,};
	char strtarget[65] = {0,};
	T_BNT_WEBHANDLE notihandle = {
			.status = strstatus,
			.time = strtime,
			.target = strtarget,
	};

	bnt_open_noti_web(&notihandle);
	bnt_set_status_noti_web(&notihandle, 
			info.status == 0 ? "ready" : info.status == 1 ? "mining" : "mined", 
			0, 0, 0);

	return 0;
} 

