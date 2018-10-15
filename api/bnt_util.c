/********************************************************************
 * bnt_util.c : Utility functions
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
#include <math.h>

void 
hexdump(
		const void *src, 
		size_t length, 
		size_t line_size, 
		char *prefix
		)
{
    int i = 0;
    const unsigned char *address = src;
    const unsigned char *line = address;
    unsigned char c;

    printf("%s | ", prefix);
    while (length-- > 0) {
        printf("%02X ", *address++);
        if (!(++i % line_size) || (length == 0 && i % line_size)) {
            if (length == 0) {
                while (i++ % line_size)
                    printf("__ ");
            }
			printf(" | ");  /* right close */
			while (line < address) {
				c = *line++;
				printf("%c", (c < 33 || c == 255) ? 0x2E : c);
			}
            printf("\n");
            if (length > 0)
                printf("%s | ", prefix);
        }
    }
}

void 
printreg(
		const void *src, 
		size_t count, 
		int addr
		)
{
    int i = 0;
    const unsigned short* address = src;
	const int line_size = 8;

    printf("[%04X] | ", addr);

    while (count-- > 0) {
        printf("%04X ", htons(*address++));
        if (!(++i % line_size) || (count == 0 && i % line_size)) {
            if (count == 0) {
                while (i++ % line_size);
//                  printf("____ ");
            }
            printf("\n");
            if (count > 0)
				printf("[%04X] | ", addr+i);
        }
    }
	printf("\n");
}

bool bnt_getmidhash(
		unsigned char* input, 
		unsigned char* out
		)
{
	SHA256_CTX context;
	if(!SHA256_Init(&context)) return false;
	if(!SHA256_Update(&context, input, 64)) return false;

	unsigned int* hashp = (unsigned int*)context.h;
	unsigned int* outp = (unsigned int*)out;

	for(int i=0; i<8; i++) 
		outp[7-i] = htonl(*(unsigned int*)hashp++);

	return true;
}

bool bnt_gethash(
		unsigned char* input, 
		unsigned int   length, 
		unsigned char* out
		)
{
	SHA256_CTX context;
	if(!SHA256_Init(&context)) return false;
	if(!SHA256_Update(&context, input, length)) return false;
	if(!SHA256_Final(out, &context)) return false;

	return true;
}

void bnt_hash2str(
		unsigned char* hash,
		char* out
		)
{
	static const char* tbl = "0123456789abcdef";
	for(int i=0; i<SHA256_DIGEST_LENGTH; i++) {
		out[2*i+0] = tbl[hash[i] >> 4];
		out[2*i+1] = tbl[hash[i] & 0x0F];
	}
}

void bnt_str2hex(
		char* str, 
		int len, 
		unsigned char* hex
		)
{
	do {
		sscanf(str, "%2hhx", hex++);
		str += 2;
	} while(*str);
}

void bnt_hex2str(
		unsigned char* hex, 
		int hexlen, 
		char* str
		)
{
    for(int i=0; i<hexlen; i++, str+=2) {
        sprintf(str, "%02x", *hex++);
    }
}

static void
bnt_fill_zeros(unsigned int nbytes, char* outstr)
{
	//outstr should be clean
	for(int i=0; i<nbytes; i++) {
		*outstr++ = '0';
		*outstr++ = '0';
	}
}
	
static void
bnt_pad_zeros(char* outstr)
{
	for(int i=strlen(outstr); i<64; i++) {
	   outstr[i] = '0';
	} 
}	

void bnt_get_targetstr(
		unsigned int bits,
		char* str
		)
{
	unsigned int nzerobytes = ((bits >> 24) - 3);
	unsigned int target_int = bits & 0x00FFFFFF;

	bnt_fill_zeros(32 - (3 + nzerobytes), str); //3 means byte length of target_int
	sprintf(str + strlen(str), "%06X", target_int);
	bnt_pad_zeros(str);
}

void bnt_swap_byte(
		unsigned char* in,
		unsigned char* out,
		int   sizebyte
		)
{
	if(!in || !out) return;

	in += sizebyte - 1; 
	do {
		*out++ = *in--;
	} while(--sizebyte > 0);
}

//TODO: compare with open source
unsigned int bnt_get_bits(
		unsigned char* hash
		)
{
//input hash is hex format
	int zerobyte = 0;
	unsigned int bits_0 = 0;
	unsigned int bits = 0;

	do {
		if(hash[zerobyte]) break;
	} while(zerobyte++ < 32);
	
	if(zerobyte == 32) {
		printf("%s: Something Wrong. zerobyte %d\n", __func__, zerobyte);
		return 0x7FFFFFFF;
	}

	bits_0 = 32 - zerobyte;
	bits = (bits_0 << 24) | ((ntohl(*(unsigned int*)&hash[zerobyte]))>>8);

	printf("%s: MY BITS ARE %08X\n", __func__, bits);

	return bits; 
}
