/********************************************************************
 * bnt_def.h : Definitions for BNT 
 *
 * Copyright (c) 2018  TheFrons, Inc.
 * Copyright (c) 2018  Joon Kim <joon@thefrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 ********************************************************************/
#ifndef BNT_DEF_H
#define BNT_DEF_H

#define BNT_TRACE(args)     printf args
#define BNT_ERROR(args)     printf args
#define BNT_MSG(args)       printf args
#define BNT_DEV(args)       printf args

#ifdef DEMO
#define BNT_INFO(args)      Cons_printf args
#define BNT_PRINT(args)     Cons_printf args
#else
#define BNT_INFO(args)      printf args
#define BNT_PRINT(args)     printf args
#endif

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
    if((exp) != true) { \
        fprintf(stderr, "%s:%s:%d BNT_CHECK_TRUE\n", __FILE__, __func__, __LINE__ ); \
        return ret; \
    }

/////
///// SPI MESSAGE HEADER DEFINITIONS 
/////
#define MAX_LENGTH_BNT_SPI                 64  //TODO: should be optimized.
#define LENGTH_MSG_HEADER                  3
#define LENGTH_SPI_PADDING_BYTE            1   //ASIC needs more clock 

#define CMD_READ                           0
#define CMD_WRITE                          1
#define CMD_UNICAST                        0
#define CMD_BCAST                          1

#define HEADER_CMD(CMD)                    ((CMD)?0x80:0)
#define HEADER_BCAST(ISBCAST)              ((ISBCAST)?0x40:0)
#define HEADER_CHIPID(CHIPID)              ((CHIPID)&0xFF)
#define HEADER_CHIPID_1(CHIPID)            ((CHIPID)&0xFC)
#define HEADER_CHIPID_2(CHIPID)            ((CHIPID)&0x03)
#define HEADER_REGADDR(ADDR)               ((ADDR)&0x3F)
#define HEADER_DLEN(BYTE)                  (((BYTE)>>1)&0xFF)

#define HEADER_FIRST(CMD, BCAST, CHIPID)   \
	(HEADER_CMD(CMD) | HEADER_BCAST(BCAST) | (HEADER_CHIPID_1(CHIPID)>>2))
#define HEADER_SECOND(CHIPID, REGADDR)     ((HEADER_CHIPID_2(CHIPID)<<6) | HEADER_REGADDR(REGADDR))
#define HEADER_THIRD(BYTE)                 HEADER_DLEN(BYTE)

#define LENGTH_SPI_MSG(DATA_BYTES)         LENGTH_MSG_HEADER + (DATA_BYTES)

#define BUS_SPI(BUS)                       BUS
#define MAX_COUNT_REG_ACCESS               23 //means 46 bytes

#define MAX_SPI_CS                         3

/////
///// BNT system
/////
#define MAX_CHIPID                         0xFF
#define MAX_NBOARDS                        4
#define MAX_NCHIPS_PER_BOARD               256    // should be <= nboards x nchips = 1024

#define BITS_BOARDID                       2
#define BITS_CHIPID                        8

#define CHIPID_PHYSICAL(ID_LOGICAL, HANDLE) \
	((ID_LOGICAL) << (HANDLE)->idshift)       

#ifdef FPGA
#define N_INTERNAL_HASH_ENGINES            8
#define SHIFT_INTERNAL_HASH_ENGINES        3
#else
#define N_INTERNAL_HASH_ENGINES            1024
#define SHIFT_INTERNAL_HASH_ENGINES        10     //16 + 64
#endif

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
    TSR = 0x05,   // (RW) Turbo Mode Set Register
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
    HVR20 = 0x1A,   // (RW) Hash Value Set Register 20
    HVR21 = 0x1B,   // (RW) Hash Value Set Register 21
    HVR22 = 0x1C,   // (RW) Hash Value Set Register 22

    MRR0 = 0x1D,   // (RO) Mining Result Register 0
    MRR1 = 0x1E,   // (RO) Mining Result Register 1
    MRR2 = 0x1F,   // (RO) Mining Result Register 2
	ENDOF_BNT_REGISTERS = 0x20,
} bnt_register_t;

//I_ means offset bit
//V_ means valid size for the field
#define MASKFOR_LENGTH(LEN)                 ((1<<(LEN))-1)

#define IDR_SIGNATURE                       0xAB //427
#define I_IDR_CHIPID                        0
#define V_IDR_CHIPID                        8
#define I_IDR_SIGNATURE                     8
#define V_IDR_SIGNATURE                     8

#define I_SSR_SOFTRESET                     0
#define V_SSR_SOFTRESET                     1
#define I_SSR_TESTMODE                      1
#define V_SSR_TESTMODE                      1
#define I_SSR_DATABITS                      2
#define V_SSR_DATABITS                      1
#define I_SSR_BOARDID                       4
#define V_SSR_BOARDID                       2
#define I_SSR_MASK                          6
#define V_SSR_MASK                          10

#define I_PSR_M                             0
#define V_PSR_M                             7
#define I_PSR_P                             7
#define V_PSR_P                             6
#define I_PSR_S                             13
#define V_PSR_S                             3

#define I_IER_MINED                         0
#define V_IER_MINED                         1
#define I_IER_RSLT_FIFO_ALMOST_FULL         1
#define V_IER_RSLT_FIFO_ALMOST_FULL         1
#define I_IER_BHV_FIFO_FULL                 2
#define V_IER_BHV_FIFO_FULL                 1

#define I_ISR_MINED                         1
#define V_ISR_MINED                         1
#define I_ISR_RSLT_FIFO_ALMOST_FULL         2
#define V_ISR_RSLT_FIFO_ALMOST_FULL         1
#define I_ISR_BHV_FIFO_FULL                 3
#define V_ISR_BHV_FIFO_FULL                 1

#define I_TSR_TURBOMODE                     0
#define V_TSR_TURBOMODE                     2

#define I_HVR0_HASHID                       0
#define V_HVR0_HASHID                       8
#define SIZE_TOTAL_HVR_BYTE                 46

#define I_MRR0_HASHID                       0
#define V_MRR0_HASHID                       8
#define I_MRR0_EXTRA_HASHID                 8
#define V_MRR0_EXTRA_HASHID                 3


/////
///// BNT structs
/////

#pragma pack(1)

#define SIZE_BNT_HASH_TUPLE                 46 // Byte = WorkId + midstate + ...
#define SIZE_BNT_HASH_TUPLE_CORE            44 // Byte = midstate + ...
#define COUNT_BNT_HASH_TUPLE                23 // Short

#define THRESHOLD_GET_NONCE_COUNT           700 // 

typedef struct bnt_spi_header {
	unsigned char  cmdid;
	unsigned char  addr;
	unsigned char  length;
	unsigned char  data[0];
} T_BntAccess;

typedef struct {
    int            nboards;
    int            nchips;
    int            idshift;
    int            spifd[MAX_NBOARDS];
    int            gpiofd[MAX_NBOARDS];
    int            nonce_mode;
    unsigned short ssr;
    unsigned short mask;
    FILE*          bhfp;
    //TODO: 
    //thread
    //mutex
    //stats
    //works
    //tasks
} T_BntHandle;

typedef struct block_header_p1 {
	unsigned int    version;
	unsigned char   prevhash[32];
	unsigned char   merkle[28];
} T_BlockHeaderP1;

typedef struct block_header_p2 {
	unsigned char   merkle[4];
	unsigned int    ntime;
	unsigned int    bits;
	unsigned int    nonce;
} T_BlockHeaderP2;

typedef struct block_header {
	unsigned int    version;
	unsigned char   prevhash[32];
	unsigned char   merkle[32];
	unsigned int    ntime;
	unsigned int    bits;
	unsigned int    nonce;
} T_BlockHeader;

typedef struct bnt_hash_tuple {
	T_BlockHeader   bh;
	unsigned char   workid;   //BNT internal
	unsigned char   midstate[32];
	unsigned char   hashout[32];
	unsigned int    nonceout;
} T_BntHash;

typedef struct { //HVR
	unsigned char   reserved;
	unsigned char   workid;
	unsigned char   midstate[32];
	unsigned char   merkle[4];
	unsigned int    ntime;
	unsigned int    bits;
	char            strout[SIZE_TOTAL_HVR_BYTE*2+1]; 
} T_BntHashHVR;

typedef struct {
	unsigned char   extraid;
	unsigned char   workid;  //BNT internal
	unsigned int    nonceout;
} T_BntHashMRR;

typedef enum {
	IntMined = 1,
	IntResultFifoAlmostFull = 2,
	IntBHVFifoFull = 4,
	IntAll = 1|2|4
} EnumInterruptKind;


#endif //BNT_DEF_H
