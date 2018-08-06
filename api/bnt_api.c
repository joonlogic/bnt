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
	unsigned char* ptr = txbuf;
	int ret = 0;

	*ptr++ = HEADER_FIRST(CMD_WRITE, isbcast, chipid);
	*ptr++ = HEADER_SECOND(regaddr);
	*ptr++ = HEADER_THIRD(wrbytes);
	memcpy(ptr, buf, wrbytes);

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
	int txlen = 0;
	int rxlen = 0;
	unsigned char* ptr = txbuf;
	int ret = 0;

	*ptr++ = HEADER_FIRST(CMD_READ, CMD_UNICAST, chipid);
	*ptr++ = HEADER_SECOND(regaddr);
	*ptr++ = HEADER_THIRD(rdbytes);

	txlen = LENGTH_SPI_MSG(0);
	rxlen = rdbytes;

	ret = do_spi_tx_rx(fd, txbuf, rxbuf, txlen, rxlen);
	BNT_CHECK_TRUE(ret >= 0, ret);

	memcpy(buf, rxbuf, rxlen);
	return ret;
}

