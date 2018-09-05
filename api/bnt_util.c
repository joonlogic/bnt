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

