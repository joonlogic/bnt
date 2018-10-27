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

	access->cmdid = HEADER_FIRST(CMD_WRITE, isbcast, isbcast ? 0xFF : chipid);
	access->addr = HEADER_SECOND(isbcast ? 0x03 : chipid, regaddr);
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
		bool isbcast,
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

	access->cmdid = HEADER_FIRST(CMD_READ, isbcast, isbcast ? 0xFF : chipid);
	access->addr = HEADER_SECOND(isbcast ? 0x03 : chipid, regaddr);
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
		void* buf,
		bool verbose
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
		access->addr = HEADER_SECOND(chipid, i);
		bnt_spi_tx_rx(fd, txbuf, rxbuf, txlen, rxlen, verbose); 
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

	for(int chipid=0; chipid<nchips; chipid++) {
		access->cmdid = HEADER_FIRST(CMD_READ, CMD_UNICAST, chipid);
		access->addr = HEADER_SECOND(chipid, addr);
		access->length = HEADER_THIRD(1);

		bnt_spi_tx_rx(fd, txbuf, rxbuf, txlen, rxlen, false); 
		*(unsigned short*)(buf+(chipid<<1)) = *(unsigned short*)rxbuf;
	}

	return 0;
}

int 
regmcast(
		int fd,
		bool* mined,
		bool* unique,
		unsigned char* chipid,
		bool verbose
		)
{
	unsigned short mine;
	unsigned char* regp;
	regread(fd, 0xFF, MRR1, &mine, sizeof(mine), true, verbose);

	regp = (unsigned char*)&mine;
	*chipid = *regp;
	*mined = (regp[0] | regp[1]) ? true : false; 
	*unique = ((regp[0] ^ regp[1]) == 0xFF) ? true : false; 

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
	
void
bnt_write_board(
		int regaddr,
		void* buf,
		int wrbytes,
		int boardid,
		T_BntHandle* handle
		)
{
	if(handle->spifd[boardid] <= 0) return;
	regwrite(
			handle->spifd[boardid],
			0,
			regaddr,
			buf,
			wrbytes,
			(int)true,
			false
			);
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
	regread(fd, chipid, IDR, &idr, SIZE_REG_DATA_BYTE, false, false);

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

	regread(handle->spifd[0], 0, IER, &ier, sizeof(ier), false, false);
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
bnt_set_boardid(
		int fd,
		int boardid
		)
{
	unsigned short ssr;

	BNT_CHECK_TRUE(fd>0, -1);

	regread(fd, 0, SSR, &ssr, sizeof(ssr), false, false);
	ssr = ntohs(ssr);
	ssr &= (((1 << V_SSR_BOARDID) - 1) << I_SSR_BOARDID);
	ssr |= (boardid & ((1 << V_SSR_BOARDID) - 1)) << I_SSR_BOARDID;
	ssr = htons(ssr);

	regwrite(fd, 0, SSR, &ssr, sizeof(ssr), true, false);

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
		regread(fd, 0, IER, &ier, sizeof(ier), false, false);
		ier = ntohs(ier);
		ier = htons(enable ? ier | ikind : ier & (~ikind));

		regwrite(fd, chipid, IER, &ier, sizeof(ier), broadcast, false);
	}
	else {
		for(int i=0; i<MAX_NBOARDS; i++) {
			if(handle->spifd[i] <= 0) continue;

			fd = handle->spifd[i];

			regread(fd, 0, IER, &ier, sizeof(ier), false, false);
			ier = ntohs(ier);
			ier = htons(enable ? ier | ikind : ier & (~ikind));

			regwrite(fd, chipid, IER, &ier, sizeof(ier), broadcast, false);
		}
	}

	return 0;
}

unsigned short 
bnt_get_nonce_mask(
		int nboards,
		int nchips
		)
{
	static const unsigned char CHIP_MASK[MAX_NCHIPS_PER_BOARD] = {
		[0] = 0,
		[1] = 0x80,
		[3] = 0xC0,
		[7] = 0xE0,
		[15] = 0xF0,
		[31] = 0xF8,
		[63] = 0xFC,
		[127] = 0xFE,
		[255] = 0xFF,
	};

	unsigned short bmask = 0;

	BNT_CHECK_TRUE(nboards <= MAX_NBOARDS, 0);
	BNT_CHECK_TRUE(nchips <= MAX_NCHIPS_PER_BOARD, 0);

	bmask = (nboards-1) << BITS_CHIPID;

	return bmask | CHIP_MASK[nchips-1];
}

unsigned short 
bnt_get_nchips(
		unsigned short mask
		)
{
	static const unsigned short NCHIPS_FROM_MASK[256] = {
		[0] = 1,
		[0x80] = 2,
		[0xC0] = 4,
		[0xE0] = 8,
		[0xF0] = 16,
		[0xF8] = 32,
		[0xFC] = 64,
		[0xFE] = 128,
		[0xFF] = 256,
	};

	unsigned short chipmask = mask & (MAX_NCHIPS_PER_BOARD - 1);
	unsigned short boardmask = (mask & ((MAX_NBOARDS - 1) << BITS_CHIPID))>>(BITS_CHIPID);

	return NCHIPS_FROM_MASK[chipmask] * (boardmask + 1);
}

//get chipid shift according to nChips
//deprecated according to id sequence change
int 
bnt_get_id_shift(
		int nchips
		)
{
	static const unsigned short BITS[256] = {
		[0] = 0,
		[1] = BITS_CHIPID-1,
		[3] = BITS_CHIPID-2,
		[7] = BITS_CHIPID-3,
		[15] = BITS_CHIPID-4,
		[31] = BITS_CHIPID-5,
		[63] = BITS_CHIPID-6,
		[127] = BITS_CHIPID-7,
		[255] = BITS_CHIPID-8,
	};

	return BITS[nchips-1];
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
		unsigned char* extraid,
		unsigned char* workid
		)
{
	unsigned short mrr0;
	regread(fd, chipid, MRR0, &mrr0, sizeof(mrr0), false, false);
	*extraid = (unsigned char)(ntohs(mrr0) >> I_MRR0_EXTRA_HASHID);
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
	regread(fd, chipid, MRR0, mrr, sizeof(*mrr), false, false);
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
	T_BlockHeader* bhp;

	//check work id
	if(bhash->workid != mrr->workid) {
		BNT_INFO(("%s: mismatch workid. bhash->workid %02X vs mrr->workid %02X\n",
				__func__, bhash->workid, mrr->workid));
		return false;
	}

	//check nonce
	unsigned int realnonce = 0;
	realnonce = bnt_get_realnonce(mrr->nonceout, handle->mask);

	BNT_INFO(("\n[%d][%02d] FOUND : NONCE %08X (MRR was %08X)\n", board, chip, realnonce, mrr->nonceout));
	bhash->bh.nonce = ntohl(realnonce);

	//consider ntime roll
	T_BlockHeader bh;
	int offset = handle->ntrollplus ? (board << handle->ntroll) : 0;
	memcpy(&bh, &bhash->bh, sizeof(bh));
	bh.ntime += mrr->extraid + offset;
	bhp = &bh;

	BNT_INFO(("Hashed Block Header -----\n"));
	printout_bh(bhp);

	unsigned char hashout[32] = {0,};
	bnt_gethash((unsigned char*)bhp, sizeof(bhash->bh), hashout);
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
	int timeout = (THRESHOLD_GET_NONCE_COUNT/(handle->mask ? bnt_get_nchips(handle->mask) : 1));
	timeout <<= handle->ntroll;

	printf("timeout %d from %d\n", timeout, bnt_get_nchips(handle->mask));
	do {
		//check works from Engines
		for(int board=0; board<MAX_NBOARDS; board++) {
			if(handle->spifd[board] <= 0) continue;
			for(int chip=0; chip<handle->nchips; chip++) {

				bnt_read_workid(
						handle->spifd[board],
						chip,
						&mrr.extraid,
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

				printf("mrr.workid %d mrr.extraid %d\n", mrr.workid, mrr.extraid);

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
	} while(count++ < timeout);
#endif

	BNT_PRINT(("Timedout. Waiting Count %d\n", count));

	//debug
	char debugbuf[516] = {0,};
	for(int board=0; board<MAX_NBOARDS; board++) {
		if(handle->spifd[board] <= 0) continue;
		for(int chip=0; chip<handle->nchips; chip++) {
			regdump(handle->spifd[board], chip, debugbuf, false);
			printf("[BNT REG %s] Board %d Chip %d %s\n",
								 "READ", board, chip, "");  
			printreg(debugbuf, ENDOF_BNT_REGISTERS, 0x00);
		}
	}
	
	return -1;
}


/*
 * Get nonce from Hardware Engines
 * This is useful only for Chip test..
 */
int
bnt_getnonce2(
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
	int timeout = (THRESHOLD_GET_NONCE_COUNT/(handle->mask ? bnt_get_nchips(handle->mask) : 1));
	timeout <<= handle->ntroll;

	bool ismined;
	bool isunique;
	unsigned char minedchip;
	int chip_s, chip_e;

	printf("timeout %d from %d\n", timeout, bnt_get_nchips(handle->mask));
	do {
		//check works from Engines
		for(int board=0; board<MAX_NBOARDS; board++) {
			if(handle->spifd[board] <= 0) continue;
			regmcast(handle->spifd[board], &ismined, &isunique, &minedchip, false);
			if(ismined) {
				if(isunique) {
				   chip_s = minedchip;
				   chip_e = minedchip;
				}
				else {
				   chip_s = 0;
				   chip_e = handle->nchips;
				}
					   
				for(int chip=chip_s; chip<chip_e; chip++) {
					bnt_read_workid(
						handle->spifd[board],
						chip,
						&mrr.extraid,
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

					printf("mrr.workid %d mrr.extraid %d\n", mrr.workid, mrr.extraid);

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
	} while(count++ < timeout);
#endif

	BNT_PRINT(("Timedout. Waiting Count %d\n", count));

	//debug
	char debugbuf[516] = {0,};
	for(int board=0; board<MAX_NBOARDS; board++) {
		if(handle->spifd[board] <= 0) continue;
		for(int chip=0; chip<handle->nchips; chip++) {
			regdump(handle->spifd[board], chip, debugbuf, false);
			printf("[BNT REG %s] Board %d Chip %d %s\n",
								 "READ", board, chip, "");  
			printreg(debugbuf, ENDOF_BNT_REGISTERS, 0x00);
		}
	}
	
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
				chipcount[board]++;
			}
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

	return 0;
}

unsigned int
bnt_get_realnonce(
		unsigned int mrr,
		unsigned short mask
		)
{
	//TODO:
	//TODO: Needs to be updated when ASIC spec is fixed...........
	//TODO: Now written as 16 Blocks / chip
	//TODO:
	unsigned char offset = 2;
	unsigned int nonce;

	static const unsigned int window_unit[256] = {
#ifdef FPGA
		//8 Engines
		[0x00] = 0xFFFFFFFF,
		[0xC0] = 0x3FFFFFFF,
#else
		[0x00] = 0xFFFFFFFF,
		[0x80] = 0x7FFFFFFF,
		[0xC0] = 0x3FFFFFFF,
		[0xE0] = 0x1FFFFFFF,
		[0xF0] = 0x0FFFFFFF,
		[0xF8] = 0x07FFFFFF,
		[0xFC] = 0x03FFFFFF,
		[0xFE] = 0x01FFFFFF,
		[0xFF] = 0x00FFFFFF,
#endif
	};

	unsigned int window = 0;
	unsigned char boardmask = mask >> BITS_CHIPID;

	offset = ((mrr & (N_INTERNAL_HASH_ENGINES-1)) >> 1) ? 2 : 3;
	window = window_unit[(unsigned char)mask];
	window >>= 
		(boardmask == 3) ? 2 :
		(boardmask == 1) ? 1 : 0;

	window &= (~(N_INTERNAL_HASH_ENGINES-1));
	
	nonce = (mrr & window) >> SHIFT_INTERNAL_HASH_ENGINES;
	nonce -= offset; 
	nonce <<= SHIFT_INTERNAL_HASH_ENGINES;
	nonce = (mrr & (~window)) | (nonce & window);

	return nonce; 
}
