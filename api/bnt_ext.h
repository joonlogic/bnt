/********************************************************************
 * bnt_ext.h : Extern functions 
 *
 * Copyright (c) 2018  TheFrons, Inc.
 * Copyright (c) 2018  Joon Kim <joonlogic@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 ********************************************************************/

#ifndef BNT_EXT_H
#define BNT_EXT_H

extern int
regwrite(int fd, int chipid, int regaddr, void* buf, int wrbytes, int isbcast),
regread(int fd, int chipid, int regaddr, void* buf, int rdbytes),
do_open(int bus, int cs),
do_read(int fd, void* buf, int len),
do_write(int fd, void* buf, int len),
do_spi_tx_rx(int fd, unsigned char* txbuf, unsigned char* rxbuf, int txlen, int rxlen),
do_config_gpio_irq(int gpiopin, int);

extern void
hexdump(const void *src, size_t length, size_t line_size, char *prefix),
regdump( const void *src, size_t count, int addr); 

#endif // BNT_EXT_H