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
#include <arpa/inet.h>
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
		bool isbcast,
		bool verbose
		)
{
	BNT_CHECK_NULL(buf, -1);

	unsigned char txbuf[MAX_LENGTH_BNT_SPI] = {0,};
	T_BntAccess* access = (T_BntAccess*)txbuf;
	int ret = 0;

	access->cmdid = HEADER_FIRST(CMD_WRITE, isbcast, isbcast ? 0x3F : chipid);
	access->addr = HEADER_SECOND(regaddr);
	access->length = HEADER_THIRD(wrbytes);
	memcpy(access->data, buf, wrbytes);

	ret = bnt_spi_write(fd, txbuf, LENGTH_SPI_MSG(wrbytes) + LENGTH_SPI_PADDING_BYTE, verbose);
	return ret - LENGTH_MSG_HEADER;
}

int 
regread(
		int fd,
		int chipid,
		int regaddr,
		void* buf,
		int rdbytes,
		bool verbose
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
	rxlen = rdbytes + LENGTH_SPI_PADDING_BYTE;

	ret = bnt_spi_tx_rx(fd, txbuf, rxbuf, txlen, rxlen, verbose); //TODO: compare with bnt_spi_read
	BNT_CHECK_TRUE(ret >= 0, ret);

	memcpy(buf, rxbuf, rdbytes);

	return ret;
}

int 
regdump(
		int fd,
		int chipid,
		void* buf
		)
{
	BNT_CHECK_NULL(buf, -1);

	unsigned char txbuf[MAX_LENGTH_BNT_SPI] = {0,};
	unsigned char rxbuf[MAX_LENGTH_BNT_SPI] = {0,};
	T_BntAccess* access = (T_BntAccess*)txbuf;
	int txlen = 0;
	int rxlen = 0;

	access->cmdid = HEADER_FIRST(CMD_READ, CMD_UNICAST, chipid);
	access->length = HEADER_THIRD(1);
	txlen = LENGTH_SPI_MSG(0);
	rxlen = SIZE_REG_DATA_BYTE + LENGTH_SPI_PADDING_BYTE;

	for(int i=0; i<ENDOF_BNT_REGISTERS; i++) {
		access->addr = HEADER_SECOND(i);
		bnt_spi_tx_rx(fd, txbuf, rxbuf, txlen, rxlen, false); 
		*(unsigned short*)(buf+(i<<1)) = *(unsigned short*)rxbuf;
	}

	return 0;
}

int 
regscan(
		int fd,
		void* buf
		)
{
	BNT_CHECK_NULL(buf, -1);

	unsigned char txbuf[MAX_LENGTH_BNT_SPI] = {0,};
	unsigned char rxbuf[MAX_LENGTH_BNT_SPI] = {0,};
	T_BntAccess* access = (T_BntAccess*)txbuf;
	int txlen = LENGTH_SPI_MSG(0);
	int rxlen = SIZE_REG_DATA_BYTE + LENGTH_SPI_PADDING_BYTE;

	for(int chipid=0; chipid<MAX_NCHIPS_PER_BOARD; chipid++) {
		access->cmdid = HEADER_FIRST(CMD_READ, CMD_UNICAST, chipid);
		access->addr = HEADER_SECOND(IDR);
		access->length = HEADER_THIRD(1);

		bnt_spi_tx_rx(fd, txbuf, rxbuf, txlen, rxlen, false); 
		*(unsigned short*)(buf+(chipid<<1)) = *(unsigned short*)rxbuf;
	}

	return 0;
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
				(int)true,
				false
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

#ifdef DEBUG
//	bnt_hex2str((unsigned char*)&hvrs, SIZE_TOTAL_HVR_BYTE, hvrs.strout);
//	BNT_INFO(("%s: HVR : %s\n", __func__, hvrs.strout));
#endif

	//TODO: temporary
//	bnt_pop_fifo(0, 0, true, handle);

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
	regread(fd, chipid, IDR, &idr, SIZE_REG_DATA_BYTE, false);

	idr = ntohs(idr);
	BNT_INFO(("\t[ %02d (0x%02X) ] IDR %04X\n", chipid, chipid, idr)); 
	
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
	unsigned short set = htons(1);
	unsigned short unset = 0;
	int wrbytes = 2;

	regwrite(fd, chipid, regaddr, &set, wrbytes, broadcast, false);

	usleep(100000); //100ms TODO: check the exact value

	//release
	regwrite(fd, chipid, regaddr, &unset, wrbytes, broadcast, false);

	return 0;
}

int 
bnt_pop_fifo(
		int fd,
		int chipid,
		bool broadcast,
		T_BntHandle* handle
		)
{
	unsigned short ier = 0;
	unsigned short set = 0;
	unsigned short unset = 0;

	regread(handle->spifd[0], 0, IER, &ier, sizeof(ier), false);
	ier = ntohs(ier);

	unset = htons(ier & (~(1 << I_IER_MINED)));
	set = htons(ier | (1 << I_IER_MINED)); 


	if(broadcast)
		for(int i=0; i<handle->nboards; i++) 
			regwrite(handle->spifd[i], chipid, IER, &unset, sizeof(ier), broadcast, false);
	else
			regwrite(fd, chipid, IER, &unset, sizeof(ier), broadcast, false);

	usleep(1000); //1ms TODO: check the exact value

	if(broadcast)
		for(int i=0; i<handle->nboards; i++) 
			regwrite(handle->spifd[i], chipid, IER, &set, sizeof(ier), broadcast, false);
	else
			regwrite(fd, chipid, IER, &set, sizeof(ier), broadcast, false);

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
	unsigned int le_data[16] = {0,};

	memcpy(le_data, &bhash->bh, 64); 

	ret = bnt_getmidhash(
			(unsigned char*)le_data,
			bhash->midstate
			);
	BNT_CHECK_TRUE(ret, -1);

	//debug
	/*
	char str[129] = {0,};
	bnt_hex2str((unsigned char*)le_data, 64, str);
	printf("%s: Input 64byte : %s\n", __func__, str);
	memset(str, 0x00, sizeof(str));
	bnt_hex2str((unsigned char*)bhash->midstate, 32, str);
	printf("%s: Output 64byte: %s\n", __func__, str);
	*/

	return 0;
};

int
bnt_read_mrr(
		int fd,
		int chipid,
		T_BntHashMRR* mrr
		)
{
	regread(fd, chipid, MRR0, mrr, sizeof(*mrr), false);
	mrr->nonceout = htonl(mrr->nonceout);
	return 0;
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
	unsigned int realnonce = 0;
	realnonce = bnt_get_realnonce(mrr->nonceout, handle->mask);

	if(ntohl(bhash->bh.nonce) != realnonce) {
		BNT_INFO(("%s: mismatch nonce. bhash->bh.nonce %08X vs mrr->nonce %08X, realnonce %08X\n",
				__func__, bhash->bh.nonce, mrr->nonceout, realnonce));
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

    printf("Version     : %08x\n", ntohl(bh->version));

    bnt_hash2str(bh->prevhash, outstr);
    printf("Prev Hash   : %s\n", outstr);

    bnt_hash2str(bh->merkle, outstr);
    printf("Merkle Root : %s\n", outstr);

    printf("Time Stamp  : (%08x) %s", ntohl(bh->ntime), ctime(&ntime));
    printf("Target      : %08x\n", ntohl(bh->bits));
    printf("Nonce       : %08x\n", ntohl(bh->nonce));
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
				if(mrr.workid != bhash->workid) continue; //means NO results

				//TODO: temporary
//				bnt_pop_fifo(handle->spifd[board], CHIPID_PHYSICAL(chip, handle), false, handle);

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
		if(count%20 == 0) printf("%s: count %d\n", __func__, count);
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
		spifd[board] = bnt_spi_open(0, board);
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
		spifd[board] = bnt_spi_open(0, board);
		if(spifd[board] < 0) break;

		printf("\n[Board %d] : ", board);
		for(chip=0; chip<MAX_NCHIPS_PER_BOARD; chip++) {
			if(hello_there(spifd[board], chip)) {
//				putchar('O');
				chipcount[board]++;
			}
			else {
//				putchar('.');
			}

//			if(chip % 8 == 7) putchar(' ');
		}

		printf("\nnChips %d\n", chipcount[board]);
		close(spifd[board]);
	}

	*nboards = board;
	*nchips = chipcount[0];

	if(*nchips == 0) {
		printf("No Devices scanned!\n");
		return 0;
	}

	for(board=*nboards; board>1; board--) {
		printf("%s: board %d chip count %d\n", __func__, board-1, chipcount[board-1]);
		if(chipcount[board-1] != chipcount[board-2]) {
			printf("Error! Wrong Configuration. Board %d/%d Chip count %d vs %d\n",
					board-1, board-2, chipcount[board-1], chipcount[board-2]);
			return -1;
		}
	}

	return 0;
}

unsigned int
bnt_get_realnonce(
		unsigned int mrr,
		unsigned char mask
		)
{
	unsigned char offset;
	unsigned int nonce;
	static const unsigned int window[256] = {
		//refer to "nonce compensation by S/W" slide
#ifdef FPGA
		//8 Engines
		[0] = 0xFFFFFFF8,
		[0x30] = 0x3FFFFFF8,
		[0x70] = 0x1FFFFFF8,
		[0xF0] = 0x0FFFFFF8,
#else
		[0] = 0xFFFFFFC0,
		[0x30] = 0x3FFFFFC0,
		[0x70] = 0x1FFFFFC0,
		[0xF0] = 0x0FFFFFC0,
#endif
		[0x20] = 0x7FFFFFC0,
		[0x38] = 0x1FFFFFC0,
		[0x3C] = 0x0FFFFFC0,
		[0x3E] = 0x07FFFFC0,
		[0x3F] = 0x03FFFFC0,
		[0x40] = 0x7FFFFFC0,
		[0x60] = 0x3FFFFFC0,
		[0x78] = 0x0FFFFFC0,
		[0x7C] = 0x07FFFFC0,
		[0x7E] = 0x03FFFFC0,
		[0x7F] = 0x01FFFFC0,
		[0xC0] = 0x3FFFFFC0,
		[0xE0] = 0x1FFFFFC0,
		[0xF8] = 0x07FFFFC0,
		[0xFC] = 0x03FFFFC0,
		[0xFE] = 0x01FFFFC0,
		[0xFF] = 0x00FFFFC0,
	};

	offset = ((unsigned char)( mrr & (~window[mask]) ) >> 1) ? 2 : 3;
	nonce = (mrr & window[mask]) >> SHIFT_INTERNAL_HASH_ENGINES;
	nonce -= offset; 
	nonce <<= SHIFT_INTERNAL_HASH_ENGINES;
	nonce = (mrr & (~(window[mask]))) | (nonce & window[mask]);

	return nonce; 
}
