/********************************************************************
 * bnt_devscan.c : Application for detecting BNT devices
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

int main(int argc, char *argv[])
{
	int nboards = 0;
	int nchips = 0;

	bnt_devscan(&nboards, &nchips); 

	return 0;
} 

