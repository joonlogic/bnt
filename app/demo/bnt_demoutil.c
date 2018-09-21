/********************************************************************
 * bnt_demoutil.c : Utility functions for DEMO
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
#include <string.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <time.h>
#include "bnt_def.h"
#include "bnt_demoext.h"

const char* FILE_WEBNOTI = "/var/www/html/database/json_coin.json";

static void
bnt_set_noti_web(
		T_BNT_WEBHANDLE* handle
		)
{
	char buf[256] = {0,};
	int ret = 0;

	BNT_CHECK_NULL(handle,);
	BNT_CHECK_NULL(handle->fp,);

	sprintf(buf, 
			"{\n"
			"\t\"name\"  : \"mining\",\n"
			"\t\"status\": \"%s\",\n"
			"\t\"workid\": \"%d\",\n"
			"\t\"time\"  : \"%s\",\n"
			"\t\"target\": \"%s\",\n"
			"\t\"nonce\" : \"%08X\"\n"
			"}\n",
			handle->status,
			handle->workid,
			handle->time,
			handle->target,
			handle->nonce
		   );

	ret = fwrite(buf, strlen(buf), 1, handle->fp);
	BNT_CHECK_TRUE(ret>0, );
}

void
bnt_open_noti_web(
		T_BNT_WEBHANDLE* handle
		)
{
	BNT_CHECK_NULL(handle,);

	handle->fp = fopen(FILE_WEBNOTI, "w");
}

void
bnt_set_status_noti_web(
		T_BNT_WEBHANDLE* handle,
		char* status,
		unsigned int workid,
		char* time,
//		char* target,
		unsigned int nonce
		)
{
#if 1 //def WEB_NOTI
	BNT_CHECK_NULL(handle,);

	bnt_open_noti_web(handle);
	BNT_CHECK_NULL(handle->fp,);

	strcpy(handle->status, status?status:"\n");
	handle->workid = workid;

	memset(handle->time, 0x00, 32);
	strncpy(handle->time, time?time:"", time?strlen(time)-1:1);
//	strcpy(handle->target, target?status:"");
	handle->nonce = nonce;

	bnt_set_noti_web(handle);

	fclose(handle->fp);
	handle->fp = NULL;
#endif
}
