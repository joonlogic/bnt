/********************************************************************
 * bnt_ext.h : Extern functions 
 *
 * Copyright (c) 2018  TheFrons, Inc.
 * Copyright (c) 2018  Joon Kim <joon@thefrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 ********************************************************************/

#ifndef BNT_EXT_H
#define BNT_EXT_H

extern int
regwrite(int fd, int chipid, int regaddr, void* buf, int wrbytes, bool isbcast, bool verbose),
regread(int fd, int chipid, int regaddr, void* buf, int rdbytes, bool verbose),
regdump(int fd, int chipid, void* buf),
regscan(int fd, int addr, void* buf, int nchips),
bnt_spi_open(int bus, int cs),
bnt_spi_read(int fd, void* buf, int len),
bnt_spi_write(int fd, void* buf, int len, bool verbose),
bnt_spi_tx_rx(int fd, unsigned char* txbuf, unsigned char* rxbuf, int txlen, int rxlen, bool verbose),
bnt_config_gpio_irq(int gpiopin, int);

extern void
hexdump(const void *src, size_t length, size_t line_size, char *prefix),
printreg(const void *src, size_t count, int addr); 

extern bool 
bnt_gethash(unsigned char* input, unsigned int length, unsigned char* out),
bnt_getmidhash(unsigned char* input, unsigned char* out),
hello_there(int fd, int chipid, bool verbose);

extern void 
bnt_hash2str(unsigned char* hash, char* out),
bnt_str2hex(char* str, int len, unsigned char* hex),
bnt_hex2str(unsigned char* hex, int hexlen, char* str),
bnt_printout_validnonce(int board, int chip, T_BntHash* bhash),
printout_bh(T_BlockHeader* bh),
printout_hash(unsigned char* hash);


extern void
bnt_write_all(int regaddr, void* buf, int wrbytes, T_BntHandle* handle);


extern unsigned char bnt_get_nonce_mask(int nboards, int nchips);
extern bool
bnt_test_validnonce(T_BntHash* bhash, T_BntHashMRR* mrr, T_BntHandle* handle, int board, int chip);

extern int 
bnt_softreset(int fd, int chipid, bool broadcast),
bnt_get_id_shift(int nchips),
bnt_get_midstate(T_BntHash* bhash),
bnt_getnonce(T_BntHash* bhash, T_BntHandle* handle),
bnt_detect(int* nboards, int* nchips),
bnt_read_mrr(int fd, int chipid, T_BntHashMRR* mrr),
bnt_devscan(int* nboards, int* nchips),
bnt_pop_fifo(int fd, int chipid, bool broadcast, T_BntHandle* handle),
bnt_set_interrupt(int fd, int chipid, EnumInterruptKind ikind, bool enable, bool broadcast, T_BntHandle* handle);

extern unsigned int
bnt_get_realnonce(unsigned int mrr, unsigned char mask);


#endif // BNT_EXT_H
