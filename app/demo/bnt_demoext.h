/********************************************************************
 * bnt_demo_ext.h : Extern functions 
 *
 * Copyright (c) 2018  TheFrons, Inc.
 * Copyright (c) 2018  Joon Kim <joon@thefrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 ********************************************************************/

#ifndef BNT_DEMOEXT_H
#define BNT_DEMOEXT_H

typedef struct webnoti {
	FILE* fp;
	char* name;
	char* status;
	unsigned int workid;
	char* time;
	char* target;
	unsigned int nonce;
} T_BNT_WEBHANDLE;


extern void
bnt_open_noti_web(T_BNT_WEBHANDLE* handle),
bnt_set_status_noti_web(T_BNT_WEBHANDLE* handle, char* status, unsigned int workid, char* time, unsigned int nonce);

#endif // BNT_DEMOEXT_H
