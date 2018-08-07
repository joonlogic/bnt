/********************************************************************
 * bnt_api.c : Application Interface functions
 *
 * Copyright (c) 2018  TheFrons, Inc.
 * Copyright (c) 2018  Joon Kim <joonlogic@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 ********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <bnt_def.h>
#include <bnt_ext.h>

int
regwrite(
		int fd,
		int chipid,
		int regaddr,
		void* buf,
		int wrbytes,
		int isbcast
		)
{
	BNT_CHECK_NULL(buf, -1);

	unsigned char txbuf[MAX_LENGTH_BNT_SPI] = {0,};
	T_BntAccess* access = (T_BntAccess*)txbuf;
	int ret = 0;

	access->cmdid = HEADER_FIRST(CMD_WRITE, isbcast, chipid);
	access->addr = HEADER_SECOND(regaddr);
	access->length = HEADER_THIRD(wrbytes);
	memcpy(access->data, buf, wrbytes);

	ret = do_write(fd, txbuf, LENGTH_SPI_MSG(wrbytes));
	return ret - LENGTH_MSG_HEADER;
}

int 
regread(
		int fd,
		int chipid,
		int regaddr,
		void* buf,
		int rdbytes
		)
{
	BNT_CHECK_NULL(buf, -1);

	unsigned char txbuf[MAX_LENGTH_BNT_SPI] = {0,};
	unsigned char rxbuf[MAX_LENGTH_BNT_SPI] = {0,};
	T_BntAccess* access = (T_BntAccess*)txbuf;
	int txlen = 0;
	int rxlen = 0;
	int ret = 0;

	access->cmdid = HEADER_FIRST(CMD_READ, CMD_UNICAST, chipid);
	access->addr = HEADER_SECOND(regaddr);
	access->length = HEADER_THIRD(rdbytes);

	txlen = LENGTH_SPI_MSG(0);
	rxlen = rdbytes;

	ret = do_spi_tx_rx(fd, txbuf, rxbuf, txlen, rxlen);
	BNT_CHECK_TRUE(ret >= 0, ret);

	memcpy(buf, rxbuf, rxlen);
	return ret;
}

int
request_hash( 
		int fd,
		T_BntHash* bnthash
		)
{
	BNT_CHECK_NULL(bnthash, -1);

	unsigned char txbuf[MAX_LENGTH_BNT_SPI] = {0,};
	T_BntAccess* access = (T_BntAccess*)txbuf;
	int ret = 0;

	access->cmdid = HEADER_FIRST(CMD_WRITE, bnthash->isbcast, bnthash->chipid);
	access->addr = HEADER_SECOND(HVR0);
	access->length = HEADER_THIRD(COUNT_BNT_HASH_TUPLE);
	memcpy(access->data, &bnthash->workid, SIZE_BNT_HASH_TUPLE_CORE);

	ret = do_write(fd, txbuf, LENGTH_SPI_MSG(SIZE_BNT_HASH_TUPLE));
	return ret - LENGTH_MSG_HEADER;
}




		

