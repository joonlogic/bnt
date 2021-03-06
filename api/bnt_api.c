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
#ifdef DEMO
#include "../app/demo/ConsFunc.h"
#endif
#include "bnt_def.h"
#include "bnt_ext.h"

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
		int addr,
		void* buf,
		int nchips
		)
{
	BNT_CHECK_NULL(buf, -1);

	unsigned char txbuf[MAX_LENGTH_BNT_SPI] = {0,};
	unsigned char rxbuf[MAX_LENGTH_BNT_SPI] = {0,};
	T_BntAccess* access = (T_BntAccess*)txbuf;
	int txlen = LENGTH_SPI_MSG(0);
	int rxlen = SIZE_REG_DATA_BYTE + LENGTH_SPI_PADDING_BYTE;
	int idstep = 0;

	idstep = 1 << bnt_get_id_shift(nchips);

	for(int chipid=0; chipid<MAX_NCHIPS_PER_BOARD; chipid += idstep) {
		access->cmdid = HEADER_FIRST(CMD_READ, CMD_UNICAST, chipid);
		access->addr = HEADER_SECOND(addr);
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
	for(int i=0; i<MAX_NBOARDS; i++) {
		if(handle->spifd[i] <= 0) continue;
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

bool
hello_there(
		int fd,
		int chipid,
		bool verbose
		)
{
	unsigned short idr = 0;
	regread(fd, chipid, IDR, &idr, SIZE_REG_DATA_BYTE, false);

	idr = ntohs(idr);
	if(verbose) {
		if(idr) BNT_INFO(("\t[ %02d (0x%02X) ] IDR %04X\n", chipid, chipid, idr)); 
	}
	
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
	//sleep(1);

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
//deprecated
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

int
bnt_set_interrupt(
		int fd,
		int chipid,
		EnumInterruptKind ikind,
		bool enable,
		bool broadcast,
		T_BntHandle* handle
		)
{
	//fd < 0, broadcast true --> all system
	//fd > 0, broadcast true --> all chips on the fd
	//if broadcast is false then fd should be > 0

	unsigned short ier;

	if(fd > 0) {
		regread(fd, 0, IER, &ier, sizeof(ier), false);
		ier = ntohs(ier);
		ier = htons(enable ? ier | ikind : ier & (~ikind));

		regwrite(fd, chipid, IER, &ier, sizeof(ier), broadcast, false);
	}
	else {
		for(int i=0; i<MAX_NBOARDS; i++) {
			if(handle->spifd[i] <= 0) continue;

			fd = handle->spifd[i];

			regread(fd, 0, IER, &ier, sizeof(ier), false);
			ier = ntohs(ier);
			ier = htons(enable ? ier | ikind : ier & (~ikind));

			regwrite(fd, chipid, IER, &ier, sizeof(ier), broadcast, false);
		}
	}

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

unsigned short 
bnt_get_nchips(
		unsigned char mask
		)
{
	static const unsigned short BNT_TOTAL_CHIPS_FROM_MASK[256] = {
		[0] = 1,
		[0x20] = 2,
		[0x30] = 4,
		[0x38] = 8,
		[0x3C] = 16,
		[0x3E] = 32,
		[0x3F] = 64,
		[0x40] = 2,
		[0x60] = 4,
		[0x70] = 8,
		[0x78] = 16,
		[0x7C] = 32,
		[0x7E] = 64,
		[0x7F] = 128,
		[0xC0] = 4,
		[0xE0] = 8,
		[0xF0] = 16,
		[0xF8] = 32,
		[0xFC] = 64,
		[0xFE] = 128,
		[0xFF] = 256,
	};

	return BNT_TOTAL_CHIPS_FROM_MASK[mask];
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
bnt_read_workid(
		int fd,
		int chipid,
		unsigned char* workid
		)
{
	unsigned short mrr0;
	regread(fd, chipid, MRR0, &mrr0, sizeof(mrr0), false);
	*workid = (unsigned char)ntohs(mrr0);
	return 0;
}

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

//compare nonce out with the nonce of the block chain header already mined.
bool
bnt_test_validnonce(
		T_BntHash* bhash,
		T_BntHashMRR* mrr,
		T_BntHandle* handle,
		int board,
		int chip
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
#ifndef DEMO
		BNT_INFO(("\n[%d][%02d] Met! bhash->bh.nonce %08X vs mrr %08X => %08X )\n",
				board, chip, ntohl(bhash->bh.nonce), mrr->nonceout, realnonce));
#endif
		return false;
	}

	return true; 
}

//calculate hash and compare target bits
bool
bnt_test_validnonce_out(
		T_BntHash* bhash,
		T_BntHashMRR* mrr,
		T_BntHandle* handle,
		int board,
		int chip
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

	BNT_INFO(("\n[%d][%02d] FOUND : NONCE %08X\n", board, chip, realnonce));
	bhash->bh.nonce = ntohl(realnonce);

	unsigned char hashout[32] = {0,};
	bnt_gethash((unsigned char*)&bhash->bh, sizeof(bhash->bh), hashout);
	bnt_gethash(hashout, sizeof(hashout), hashout);

	unsigned char hashoutswap[32] = {0,};
	bnt_swap_byte(hashout, hashoutswap, 32);
	printout_hash(hashoutswap, "HASH OUT    "); 

	//TODO: compare target
	unsigned int bits = 0;
	bits = bnt_get_bits(hashoutswap);

	printf("%s: target bits %08X vs found bits %08X\n",
			__func__, bhash->bh.bits, bits);

	return bits < bhash->bh.bits ? true : false;
}

void
bnt_printout_validnonce(
		int board,
		int chip,
		T_BntHash* bhash
		)
{
	BNT_PRINT(("[%d][%02d] (( MINED !! )) NONCE %08X\n",
			board, chip, htonl(bhash->bh.nonce)));
}

void printout_bh(
        T_BlockHeader* bh
        )
{
    char outstr[65] = {0,};
    time_t ntime = bh->ntime;

#ifdef DEMO //need byte swap
    BNT_PRINT(("Version     : %08X\n", bh->version));

	char swapstr[65] = {0,};
    bnt_hash2str(bh->prevhash, outstr);
	bnt_swap_byte((unsigned char*)outstr, (unsigned char*)swapstr, 64);
    BNT_PRINT(("Prev Hash   : %s\n", swapstr));

    bnt_hash2str(bh->merkle, outstr);
	bnt_swap_byte((unsigned char*)outstr, (unsigned char*)swapstr, 64);
    BNT_PRINT(("Merkle Root : %s\n", swapstr));

    BNT_PRINT(("Time Stamp  : %s", ctime(&ntime)));
//    BNT_PRINT(("Bits        : %08X\n", bh->bits));

	memset(outstr, 0x00, sizeof(outstr));
	bnt_get_targetstr(bh->bits, outstr);
    BNT_PRINT(("Hash Target : %s\n", outstr));

    BNT_PRINT(("Nonce       : 00000000\n"));
	BNT_PRINT(("\n\n"));

#else
    BNT_PRINT(("Version     : %08X\n", ntohl(bh->version)));

    bnt_hash2str(bh->prevhash, outstr);
    BNT_PRINT(("Prev Hash   : %s\n", outstr));

    bnt_hash2str(bh->merkle, outstr);
    BNT_PRINT(("Merkle Root : %s\n", outstr));

    BNT_PRINT(("Time Stamp  : (%08X) %s", ntohl(bh->ntime), ctime(&ntime)));
    BNT_PRINT(("Bits        : %08X\n", bh->bits));

	memset(outstr, 0x00, sizeof(outstr));
	bnt_get_targetstr(bh->bits, outstr);
    BNT_PRINT(("Target      : %s\n", outstr));

    BNT_PRINT(("Nonce       : %08X\n", ntohl(bh->nonce)));
#endif
}

void printout_hash(
        unsigned char* hash,
		char* title
        )
{
    char outstr[65] = {0,};

    bnt_hash2str(hash, outstr);
    BNT_PRINT(("%s: %s\n", title, outstr));
}

void printout_hash_swap(
        unsigned char* hash,
		char* title
        )
{
    char outstr[65] = {0,};
    char swapstr[65] = {0,};

    bnt_hash2str(hash, outstr);
	bnt_swap_byte((unsigned char*)outstr, (unsigned char*)swapstr, 64);
    BNT_PRINT(("%s: %s\n", title, swapstr));
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
	usleep(100);

	//2. Wait & Get results
	T_BntHashMRR mrr={0,};
	bool isvalid;
	int count = 0;
	int idstep = 1 << bnt_get_id_shift(handle->nchips);
	do {
		//check works from Engines
		for(int board=0; board<MAX_NBOARDS; board++) {
			if(handle->spifd[board] <= 0) continue;
			for(int chip=0; chip<MAX_NCHIPS_PER_BOARD; chip+=idstep) {
				bnt_read_workid(
						handle->spifd[board],
						chip,
						&mrr.workid
						);

				if(mrr.workid != bhash->workid) {
					continue; //means NO results
				}

				bnt_read_mrr(
						handle->spifd[board],
						chip,
						&mrr
						);

#ifdef DEMO
				isvalid = bnt_test_validnonce(bhash, &mrr, handle, board, chip);
#else
				isvalid = bnt_test_validnonce_out(bhash, &mrr, handle, board, chip);
#endif
				if(isvalid) {
					//found it
					bnt_printout_validnonce(board, chip, bhash);
					return 0;
				}
				memset(&mrr, 0x00, sizeof(mrr));
			}
		}
#ifdef DEMO
		if(Cons_kbhit()) {
			int ch = Cons_getch();
			if((ch == 27) || (ch == 'p')) {
				return ch;
			}
		}
		int localcounter = 0;
		do {
			usleep(100000); //100ms
			BNT_PRINT(("MINING IN PROGRESS : %06d\r", count*10 + localcounter)); 
			fflush(stdout); 
		} while(localcounter++<10);
	} while(count++ < (10*THRESHOLD_GET_NONCE_COUNT/(handle->mask ? bnt_get_nchips(handle->mask) : 1)));
#else
		sleep(1);
		if(count%20 == 0) BNT_PRINT(("waiting count %d\n", count));
	} while(count++ < (THRESHOLD_GET_NONCE_COUNT/(handle->mask ? bnt_get_nchips(handle->mask) : 1)));
#endif

	BNT_PRINT(("Timedout. Waiting Count %d\n", count));
	return -1;
}

int 
bnt_devscan(
		int* nboards,
		int* nchips
		)
{
	int spifd[MAX_NBOARDS]={0,};
	int board, chip;
	int _nboard = 0;
	int chipcount[MAX_NBOARDS] = {0,};

	puts("BNT DEVICE SCAN : ");
	for(board=0; board<MAX_NBOARDS; board++) {
		spifd[board] = bnt_spi_open(0, board);
		if(spifd[board] <= 0) break;

		BNT_PRINT(("Board [%d] : ", board));
		if(!hello_there(spifd[board], 0, false)) {
			BNT_PRINT(("No Chips installed.\n"));
			close(spifd[board]);
			spifd[board] = -1;
			continue;
		}
		else puts("");

		for(chip=0; chip<MAX_NCHIPS_PER_BOARD; chip++) {
			if(hello_there(spifd[board], chip, true)) {
//				putchar('O');
				chipcount[board]++;
			}
			else {
//				putchar('.');
			}

//			if(chip % 8 == 7) putchar(' ');
		}

		_nboard++;

		close(spifd[board]);
	}

	*nboards = _nboard;
	*nchips = chipcount[0];

	BNT_PRINT(("\n===== SUMMARY =========\n"));
	for(int i=0; i<MAX_NBOARDS; i++) {
		if(spifd[i] <= 0) continue;
		BNT_PRINT(("\tBoard[%d] : %d Chips installed\n", i, chipcount[i]));
	}

	puts("");

	/*
	for(board=*nboards; board>1; board--) {
		printf("%s: board %d chip count %d\n", __func__, board-1, chipcount[board-1]);
		if(chipcount[board-1] != chipcount[board-2]) {
			printf("Error! Wrong Configuration. Board %d/%d Chip count %d vs %d\n",
					board-1, board-2, chipcount[board-1], chipcount[board-2]);
			return -1;
		}
	}
	*/

	return 0;
}

unsigned int
bnt_get_realnonce(
		unsigned int mrr,
		unsigned char mask
		)
{
	//TODO:
	//TODO: Needs to be updated when ASIC spec is fixed...........
	//TODO: Now written as 64 Blocks / chip
	//TODO:
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
		[0] = 0xFFFFF000,
		[0x30] = 0x3FFFF000,
		[0x70] = 0x1FFFF000,
		[0xF0] = 0x0FFFF000,
#endif
		[0x20] = 0x7FFFF000,
		[0x38] = 0x1FFFF000,
		[0x3C] = 0x0FFFF000,
		[0x3E] = 0x07FFF000,
		[0x3F] = 0x03FFF000,
		[0x40] = 0x7FFFF000,
		[0x60] = 0x3FFFF000,
		[0x78] = 0x0FFFF000,
		[0x7C] = 0x07FFF000,
		[0x7E] = 0x03FFF000,
		[0x7F] = 0x01FFF000,
		[0xC0] = 0x3FFFF000,
		[0xE0] = 0x1FFFF000,
		[0xF8] = 0x07FFF000,
		[0xFC] = 0x03FFF000,
		[0xFE] = 0x01FFF000,
		[0xFF] = 0x00FFF000,
	};

	offset = ((unsigned char)(mrr & (~window[mask])) >> 1) ? 2 : 3;
	nonce = (mrr & window[mask]) >> SHIFT_INTERNAL_HASH_ENGINES;
	nonce -= offset; 
	nonce <<= SHIFT_INTERNAL_HASH_ENGINES;
	nonce = (mrr & (~(window[mask]))) | (nonce & window[mask]);

	return nonce; 
}
