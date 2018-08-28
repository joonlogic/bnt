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
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <bnt_def.h>
#include <bnt_ext.h>

int
regwrite(
		int fd,
		int chipid,
		int regaddr,
		void* buf,
		int wrbytes,
		bool isbcast
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
	BNT_CHECK_NULL(rxbuf, -1);

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

void
bnt_write_all(
		int regaddr,
		void* buf,
		int wrbytes,
		T_BntHandle* handle
		)
{
	for(int i=0; i<handle->nboards; i++) {
		regwrite(
				handle->spifd[i],
				0,
				regaddr,
				buf,
				wrbytes,
				(int)true
				);
	}
}

int
bnt_request_hash( 
		T_BntHash* hash,
		T_BntHandle* handle
		)
{
	BNT_CHECK_NULL(hash, -1);

	T_BntHashHVR hvrs = {0,};

	hvrs.workid = hash->workid;
	memcpy(hvrs.midstate, hash->midstate, 32);
	memcpy(&hvrs.merkle, &hash->bh.merkle[28], 12);

	bnt_hex2str((unsigned char*)&hvrs, SIZE_TOTAL_HVR_BYTE, hvrs.strout);
	BNT_INFO(("%s: HVR : %s\n", __func__, hvrs.strout));

	bnt_write_all(
			HVR0, 
			(void*)&hvrs,
			SIZE_TOTAL_HVR_BYTE,
			handle
			);

	return 0;
}

static bool
hello_there(
		int fd,
		int chipid
		)
{
	unsigned short idr = 0;
	regread(fd, chipid, IDR, &idr, SIZE_REG_DATA_BYTE);

	BNT_INFO(("%s: chipid %d -- IDR %04X\n", __func__, chipid, idr)); 
	
	return (idr == ((IDR_SIGNATURE << I_IDR_SIGNATURE) | chipid)) ? 
		true : false;
}

int 
bnt_softreset(
		int fd,
		int chipid,
		bool broadcast
		)
{
	int regaddr = SSR;
	unsigned short set = 1;
	unsigned short unset = 0;
	int wrbytes = 2;

	regwrite(fd, chipid, regaddr, &set, wrbytes, broadcast);

	usleep(100000); //100ms TODO: check the exact value

	//release
	regwrite(fd, chipid, regaddr, &unset, wrbytes, broadcast);

	return 0;
}

unsigned char 
bnt_get_nonce_mask(
		int nboards,
		int nchips
		)
{
	static const unsigned char BNT_CONF_MASK[MAX_NBOARDS][MAX_NCHIPS_PER_BOARD] = {
		[0][0] = 0,
		[0][1] = 0x20,
		[0][3] = 0x30,
		[0][7] = 0x38,
		[0][15] = 0x3C,
		[0][31] = 0x3E,
		[0][63] = 0x3F,
		[1][0] = 0x40,
		[1][1] = 0x60,
		[1][3] = 0x70,
		[1][7] = 0x78,
		[1][15] = 0x7C,
		[1][31] = 0x7E,
		[1][63] = 0x7F,
		[3][0] = 0xC0,
		[3][1] = 0xE0,
		[3][3] = 0xF0,
		[3][7] = 0xF8,
		[3][15] = 0xFC,
		[3][31] = 0xFE,
		[3][63] = 0xFF,
	};

	BNT_CHECK_TRUE(nboards <= MAX_NBOARDS, 0);
	BNT_CHECK_TRUE(nchips <= MAX_NCHIPS_PER_BOARD, 0);

	return BNT_CONF_MASK[nboards-1][nchips-1];
}

//get chipid shift according to nChips
int 
bnt_get_id_shift(
		int nchips
		)
{
	return 
		nchips == 1 ? 0 : // theoretically 6 but no meaning
		nchips == 2 ? 5 :
		nchips == 4 ? 4 :
		nchips == 8 ? 3 :
		nchips == 16 ? 2 :
		nchips == 32 ? 1 : 0;
}

int
bnt_get_midstate(
		T_BntHash* bhash
		)
{
	bool ret;
	ret = bnt_gethash(
			(unsigned char*)&bhash->bh,
			64,
			bhash->midstate
			);
	BNT_CHECK_TRUE(ret, -1);

	return 0;
};

static int
bnt_read_mrr(
		int fd,
		int chipid,
		T_BntHashMRR* mrr
		)
{
	return regread(
			fd,
			chipid,
			MRR0,
			mrr,
			sizeof(*mrr)
		   );
}

bool
bnt_test_validnonce(
		T_BntHash* bhash,
		T_BntHashMRR* mrr,
		T_BntHandle* handle
		)
{
	//check work id
	if(bhash->workid != mrr->workid) {
		BNT_INFO(("%s: mismatch workid. bhash->workid %02X vs mrr->workid %02X\n",
				__func__, bhash->workid, mrr->workid));
		return false;
	}

	//check nonce
	if(bhash->bh.nonce != mrr->nonceout) {
		BNT_INFO(("%s: mismatch nonce. bhash->bh.nonce %08X vs mrr->nonce %08X\n",
				__func__, bhash->bh.nonce, mrr->nonceout));
		return false;
	}

	return true; 
}

void
bnt_printout_validnonce(
		int board,
		int chip,
		T_BntHash* bhash
		)
{
	printf("((FOUND)) [%d][%02d] nonce %08x\n",
			board, chip, bhash->bh.nonce);
}

void printout_bh(
        T_BlockHeader* bh
        )
{
    char outstr[65] = {0,};
    time_t ntime = bh->ntime;

    printf("Version     : %08x\n", bh->version);

    bnt_hash2str(bh->prevhash, outstr);
    printf("Prev Hash   : %s\n", outstr);

    bnt_hash2str(bh->merkle, outstr);
    printf("Merkle Root : %s\n", outstr);

    printf("Time Stamp  : (%08x) %s", bh->ntime, ctime(&ntime));
    printf("Target      : %08x\n", bh->bits);
    printf("Nonce       : %08x\n", bh->nonce);
}

void printout_hash(
        unsigned char* hash
        )
{
    char outstr[65] = {0,};

    bnt_hash2str(hash, outstr);
    printf("Hash String : %s\n", outstr);
}



/*
 * Get nonce from Hardware Engines
 * This is useful only for Chip test..
 */
int
bnt_getnonce(
		T_BntHash* bhash,
		T_BntHandle* handle
		)
{
	//1. Write midstate & block header info to registers 
	bnt_request_hash(bhash, handle); 
	sleep(1);

	//2. Wait & Get results
	T_BntHashMRR mrr={0,};
	bool isvalid;
	int count = 0;
	do {
		//check works from Engines
		for(int board=0; board<handle->nboards; board++) {
			for(int chip=0; chip<handle->nchips; chip++) {
				bnt_read_mrr(
						handle->spifd[board],
						CHIPID_PHYSICAL(chip, handle),
						&mrr
						);
				if(mrr.workid == 0) continue; //means NO results
				isvalid = bnt_test_validnonce(bhash, &mrr, handle);
				if(isvalid) {
					//found it
					bnt_printout_validnonce(board, chip, bhash);
					return 0;
				}
				memset(&mrr, 0x00, sizeof(mrr));
			}
		}
		sleep(1);
		printf("%s: %d turn arround\n", __func__, count);
	} while(count++ < THRESHOLD_GET_NONCE_COUNT);

	printf("%s: Timedout. count %d\n", __func__, count);
	return -1;
}

int
bnt_detect(
		int* nboards,
		int* nchips
		)
{
	int spifd[MAX_NBOARDS]={0,};
	int board, chip, shift;
	int chipcount[MAX_NBOARDS] = {0,};

	puts("BNT_DETECT : ");
	for(board=0; board<MAX_NBOARDS; board++) {
		spifd[board] = do_open(0, board);
		if(spifd[board] < 0) break;

		chip = 0;
		if(!hello_there(spifd[board], chip)) 
			break; //no chips on this board

		chipcount[board] = 1;
		for(int i=0; i<BITS_CHIPID; i++) {
			chip = (MAX_NCHIPS_PER_BOARD - (1 << i)) & MAX_CHIPID;
			if(hello_there(spifd[board], chip)) {
				chipcount[board] = MAX_NCHIPS_PER_BOARD >> i;
				break;
			}
		}
	}

	*nboards = board;
	*nchips = chipcount[0];

	printf("\n\t nBoards %d\n", *nboards);
	printf("\n\t nChips %d\n", *nchips);

	//verify
	shift = bnt_get_id_shift(*nchips);
	for(board=0; board<*nboards; board++)  {
		for(chip=0; chip<*nchips; chip++) {
			if(hello_there(spifd[board], chip << shift)) {
				printf("(Verification Okay) Hello from Board %d Chip %d(0x%06X)\n",
						board, chip << shift, chip << shift);
			}
			else {
				printf("(Verification Error) ** NO Response from Board %d Chip %d(0x%06X)\n",
						board, chip << shift, chip << shift);
			}
		}
		close(spifd[board]);
	}

	printf("Verification Done\n");

	return 0;
}

int 
bnt_devscan(
		int* nboards,
		int* nchips
		)
{
	int spifd[MAX_NBOARDS]={0,};
	int board, chip;
	int chipcount[MAX_NBOARDS] = {0,};

	puts("BNT DEVICE SCAN : ");
	for(board=0; board<MAX_NBOARDS; board++) {
		spifd[board] = do_open(0, board);
		if(spifd[board] < 0) break;

		printf("\n\t[Board %d] : ", board);
		for(chip=0; chip<MAX_NCHIPS_PER_BOARD; chip++) {
			if(hello_there(spifd[board], chip)) {
				putchar('O');
				chipcount[board]++;
			}
			else {
				putchar('.');
			}

			if(chip % 8 == 7) putchar(' ');
		}

		printf("\n\t\t nChips %d\n", chipcount[board]);
		close(spifd[board]);
	}

	*nboards = board;
	*nchips = chipcount[0];

	for(board=*nboards; board>1; board++) {
		if(chipcount[board-1] != chipcount[board-2]) {
			printf("Error! Wrong Configuration. Board %d/%d Chip count %d vs %d\n",
					board-1, board-2, chipcount[board-1], chipcount[board-2]);
			return -1;
		}
	}

	return 0;
}

