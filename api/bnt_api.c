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
		void* rxbuf,
		int rdbytes
		)
{
	BNT_CHECK_NULL(buf, -1);

	unsigned char txbuf[MAX_LENGTH_BNT_SPI] = {0,};
	T_BntAccess* access = (T_BntAccess*)txbuf;
	int txlen = 0;
	int rxlen = 0;
	int ret = 0;

	access->cmdid = HEADER_FIRST(CMD_READ, CMD_UNICAST, chipid);
	access->addr = HEADER_SECOND(regaddr);
	access->length = HEADER_THIRD(rdbytes);

	txlen = LENGTH_SPI_MSG(0);
	rxlen = rdbytes;

	ret = do_spi_tx_rx(fd, txbuf, rxbuf, txlen, rxlen); //TODO: compare with do_read
	BNT_CHECK_TRUE(ret >= 0, ret);

	return ret;
}

int
request_hash( 
		int fd,
		T_BntHash* bnthash
		)
{
	BNT_CHECK_NULL(bnthash, -1);

	return regwrite( fd, bnthash->chipid, HVR0, &bnthash->workid, 
			SIZE_BNT_HASH_TUPLE, bnthash->isbcast );
}

int
hello_there(
		int fd,
		int chipid
		)
{
	unsigned short idr = 0;
	regread(fd, chipid, IDR, &idr, SIZE_REG_DATA_BYTE);

	BNT_INFO("%s: chipid %d -- IDR %04X\n", __func__, chipid, idr); 
	
	return (idr == ( IDR_SIGNATURE << I_IDR_SIGNATURE ) | chipid) ? 
		TRUE : FALSE;
}

int
get_lastone(
		int fd
		)
{
	int chipid = 0;
	int ret = FALSE;

	do {
		ret = hello_there(fd, chipid);
		if(chipid++ == MAX_CHIPID) break;
	} while(ret == TRUE);

	return chipid-1;
}
