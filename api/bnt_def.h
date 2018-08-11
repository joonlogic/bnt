/********************************************************************
 * bnt_def.h : Definitions for BNT 
 *
 * Copyright (c) 2018  TheFrons, Inc.
 * Copyright (c) 2018  Joon Kim <joonlogic@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 ********************************************************************/
#ifndef BNT_DEF_H
#define BNT_DEF_H

#define TRUE                1
#define FALSE               0

#define BNT_TRACE(args)     printf args
#define BNT_INFO(args)      printf args
#define BNT_ERROR(args)     printf args
#define BNT_PRINT(args)     printf args
#define BNT_MSG(args)       printf args
#define BNT_DEV(args)       printf args

#define BNT_ASSERT(exp) \
    if(!(exp)) { \
        fprintf(stderr, "%s:%s:%d BNT_ASSERT\n", __FILE__, __func__, __LINE__ ); \
        exit(0); \
    }

#define BNT_CHECK_NULL(exp, ret) \
    if(!(exp)) { \
        fprintf(stderr, "%s:%s:%d BNT_CHECK_NULL\n", __FILE__, __func__, __LINE__ ); \
        return ret; \
    }

#define BNT_CHECK_EXP(exp, ret) \
    if(exp) { \
        fprintf(stderr, "%s:%s:%d BNT_CHECK_EXP\n", __FILE__, __func__, __LINE__ ); \
        return ret; \
    }

#define BNT_CHECK_RESULT(exp, ret) \
    if((exp) != 0) { \
        fprintf(stderr, "%s:%s:%d BNT_CHECK_RESULT\n", __FILE__, __func__, __LINE__ ); \
        return ret; \
    } // 0 means SUCCESS

#define BNT_CHECK_TRUE(exp, ret) \
    if((exp) != TRUE) { \
        fprintf(stderr, "%s:%s:%d BNT_CHECK_TRUE\n", __FILE__, __func__, __LINE__ ); \
        return ret; \
    }

/////
///// SPI MESSAGE HEADER DEFINITIONS 
/////
#define MAX_LENGTH_BNT_SPI                 64  //TODO: should be optimized.
#define LENGTH_MSG_HEADER                  3

#define CMD_READ                           0
#define CMD_WRITE                          1
#define CMD_UNICAST                        0
#define CMD_BCAST                          1

#define HEADER_CMD(CMD)                    ((CMD)?0x80:0)
#define HEADER_BCAST(ISBCAST)              ((ISBCAST)?0x40:0)
#define HEADER_CHIPID(CHIPID)              ((CHIPID)&0x3F)
#define HEADER_REGADDR(ADDR)               ((ADDR)&0xFF)
#define HEADER_DLEN(BYTE)                  (((BYTE)>>1)&0xFF)

#define HEADER_FIRST(CMD, BCAST, CHIPID)   \
	(HEADER_CMD(CMD) | HEADER_BCAST(BCAST) | HEADER_CHIPID(CHIPID))
#define HEADER_SECOND(REGADDR)             HEADER_REGADDR(REGADDR)
#define HEADER_THIRD(BYTE)                 HEADER_DLEN(BYTE)

#define LENGTH_SPI_MSG(DATA_BYTES)         LENGTH_MSG_HEADER + (DATA_BYTES)

#define BUS_SPI(BUS)                       BUS
#define MAX_COUNT_REG_ACCESS               23 //means 46 bytes

#define MAX_SPI_CS                         3

/////
///// BNT system
/////
#define MAX_CHIPID                         0x3F


/////
///// BNT registers
/////
#define SIZE_REG_DATA_BYTE                 2 

typedef enum {
    IDR = 0x00,   // (RO) Chip ID Register
    SSR = 0x01,   // (RW) Status Set Register
    PSR = 0x02,   // (RW) PLL Set Register
    IER = 0x03,   // (RW) Interrupt Enable Register
    ISR = 0x04,   // (RW) Interrupt Status Register
    SNR = 0x05,   // (RO) Status Notification Register
    HVR0 = 0x06,   // (RW) Hash Value Set Register 0
    HVR1 = 0x07,   // (RW) Hash Value Set Register 1
    HVR2 = 0x08,   // (RW) Hash Value Set Register 2
    HVR3 = 0x09,   // (RW) Hash Value Set Register 3
    HVR4 = 0x0a,   // (RW) Hash Value Set Register 4
    HVR5 = 0x0b,   // (RW) Hash Value Set Register 5
    HVR6 = 0x0c,   // (RW) Hash Value Set Register 6
    HVR7 = 0x0d,   // (RW) Hash Value Set Register 7
    HVR8 = 0x0e,   // (RW) Hash Value Set Register 8
    HVR9 = 0x0f,   // (RW) Hash Value Set Register 9
    HVR10 = 0x10,   // (RW) Hash Value Set Register 10
    HVR11 = 0x11,   // (RW) Hash Value Set Register 11
    HVR12 = 0x12,   // (RW) Hash Value Set Register 12
    HVR13 = 0x13,   // (RW) Hash Value Set Register 13
    HVR14 = 0x14,   // (RW) Hash Value Set Register 14
    HVR15 = 0x15,   // (RW) Hash Value Set Register 15
    HVR16 = 0x16,   // (RW) Hash Value Set Register 16
    HVR17 = 0x17,   // (RW) Hash Value Set Register 17
    HVR18 = 0x18,   // (RW) Hash Value Set Register 18
    HVR19 = 0x19,   // (RW) Hash Value Set Register 19
    HVR20 = 0x20,   // (RW) Hash Value Set Register 20
    HVR21 = 0x21,   // (RW) Hash Value Set Register 21
    HVR22 = 0x22,   // (RW) Hash Value Set Register 22

    MRR0 = 0x1D,   // (RO) Mining Result Register 0
    MRR1 = 0x1E,   // (RO) Mining Result Register 1
    MRR2 = 0x1F,   // (RO) Mining Result Register 2
    MRR3 = 0x20,   // (RO) Mining Result Register 3
    MRR4 = 0x21,   // (RO) Mining Result Register 4
    MRR5 = 0x22,   // (RO) Mining Result Register 5
    MRR6 = 0x23,   // (RO) Mining Result Register 6
    MRR7 = 0x24,   // (RO) Mining Result Register 7
    MRR8 = 0x25,   // (RO) Mining Result Register 8
    MRR9 = 0x26,   // (RO) Mining Result Register 9
    MRR10 = 0x27,   // (RO) Mining Result Register 10
    MRR11 = 0x28,   // (RO) Mining Result Register 11
    MRR12 = 0x29,   // (RO) Mining Result Register 12
    MRR13 = 0x2a,   // (RO) Mining Result Register 13
    MRR14 = 0x2b,   // (RO) Mining Result Register 14
    MRR15 = 0x2c,   // (RO) Mining Result Register 15
    MRR16 = 0x2d,   // (RO) Mining Result Register 16
    TMR0 = 0x2e,   // (RW) Test Nonce Value Register 0
    TMR1 = 0x2f,   // (RW) Test Nonce Value Register 1
} bnt_register_t;

//I_ means offset bit
//V_ means valid size for the field
#define MASKFOR_LENGTH(LEN)                 ((1<<(LEN))-1)

#define IDR_SIGNATURE                       0x1AB //427
#define I_IDR_SIGNATURE                     7
#define V_IDR_SIGNATURE                     9
#define I_IDR_CHIPID                        0
#define V_IDR_CHIPID                        6

#define I_SSR_SOFTRESET                     0
#define V_SSR_SOFTRESET                     1
#define I_SSR_ADMINMODE                     1
#define V_SSR_ADMINMODE                     1
#define I_SSR_DATABITS                      2
#define V_SSR_DATABITS                      1
#define I_SSR_MASK                          8
#define V_SSR_MASK                          8

#define I_PSR_PLL                           0
#define V_PSR_PLL                           8

#define I_IER_MINED                         0
#define V_IER_MINED                         1
#define I_IER_OVERHEAT                      1
#define V_IER_OVERHEAT                      1
#define I_IER_RSLT_FIFO_ALMOST_FULL         2
#define V_IER_RSLT_FIFO_ALMOST_FULL         1
#define I_IER_BHV_FIFO_FULL                 3
#define V_IER_BHV_FIFO_FULL                 1

#define I_ISR_MINED                         0
#define V_ISR_MINED                         1
#define I_ISR_OVERHEAT                      1
#define V_ISR_OVERHEAT                      1
#define I_ISR_RSLT_FIFO_ALMOST_FULL         2
#define V_ISR_RSLT_FIFO_ALMOST_FULL         1
#define I_ISR_BHV_FIFO_FULL                 3
#define V_ISR_BHV_FIFO_FULL                 1

#define I_SNR_TEMPERATURE                   0
#define V_SNR_TEMPERATURE                   9

#define I_HVR0_HASHID                       0
#define V_HVR0_HASHID                       8

#define I_MRR0_HASHID                       0
#define V_MRR0_HASHID                       8


/////
///// BNT structs
/////

#pragma pack(1)

#define SIZE_BNT_HASH_TUPLE                 46 // Byte = WorkId + midstate + ...
#define SIZE_BNT_HASH_TUPLE_CORE            44 // Byte = midstate + ...
#define COUNT_BNT_HASH_TUPLE                23 // Short

typedef struct bnt_spi_header {
	unsigned char cmdid;
	unsigned char addr;
	unsigned char length;
	unsigned char data[0];
} T_BntAccess;

typedef struct bnt_block_header {
	unsigned short workid;  //BNT internal
	unsigned int version;
	unsigned char prev_hash[32];
	unsigned char merkle[32];
	unsigned char ntime[4];
	unsigned char target[4];
	unsigned char nonce[4];
	unsigned char chipid;
	int fd;
} T_BlockHeader;

typedef struct bnt_hash_tuple {
	unsigned short workid;  //BNT internal
	unsigned char midstate[32];
	unsigned char merkle[4];
	unsigned char ntime[4];
	unsigned char target[4];
	unsigned char nonce[4];
	unsigned char post_hash[32];
	unsigned char chipid;
	int fd;
	unsigned char isbcast;
} T_BntHash;

#endif //BNT_DEF_H
