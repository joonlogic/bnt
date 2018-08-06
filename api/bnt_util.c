/********************************************************************
 * bnt_util.c : Utility functions
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
regdump(
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

