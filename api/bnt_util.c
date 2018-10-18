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

unsigned char bitswap(
		unsigned char bits
		)
{
	static const unsigned char swaptab[256] = {
		0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
		0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
		0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
		0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
		0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
		0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
		0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
		0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
		0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
		0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
		0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
		0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
		0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
		0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
		0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
		0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
		0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
		0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
		0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
		0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
		0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
		0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
		0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
		0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
		0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
		0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
		0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
		0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
		0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
		0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
		0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
		0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
	};

	return swaptab[bits];
}
