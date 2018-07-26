/********************************************************************
 *  FILE   : bnt_def.c
 *  Author : joon
 *  Content : definitions 
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

////////////// SPI MESSAGE HEADER DEFINITIONS /////////
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

#endif //BNT_DEF_H
