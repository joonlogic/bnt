#ifndef _CONSFUNC_H
#define _CONSFUNC_H

/******************************************************************************
 *
 * File Name:
 *
 *      ConsFunc.h
 *
 * Description:
 *
 *      Header file for the Console functions
 *
 * Revision History:
 *
 *      08-01-07 : PLX SDK v5.20
 *
 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h> 
#include <sys/ioctl.h> 
#include <sys/time.h> 
#include <sys/types.h>


#define FALSE 0
#define TRUE  1


/*************************************
 *          Definitions
 ************************************/
#define Plx_sleep(arg)              usleep((arg) * 1000)
#define Plx_strcasecmp              strcasecmp
#define Plx_strncasecmp             strncasecmp
#define Cons_clear()                system("clear")
#define Cons_FlushInputBuffer       flushinp
#define Cons_kbhit                  Plx_kbhit
#define Cons_getch                  Plx_getch
#define Cons_outstring(string)      fputs( (string), stdout )
#define Cons_putchar                putchar
#define Cons_scanf                  scanf
#define Cons_printf                 PlxPrintf



/******************************************************************
 * A 64-bit HEX value (0xFFFF FFFF FFFF FFFF) requires 20 decimal
 * digits or 22 octal digits. The following constant defines the
 * buffer size used to hold an ANSI string converted from a
 * 64-bit HEX value.
 *****************************************************************/
#define MAX_DECIMAL_BUFFER_SIZE         30

#define DEFAULT_SCREEN_SIZE             25  // Default lines to display before halting, if enabled
#define SCREEN_THROTTLE_OFFSET          2   // Num lines to offset for halting

#define _Pause                                                \
    do                                                        \
    {                                                         \
        Cons_printf("  -- Press any key to continue --");     \
        Cons_getch();                                         \
        Cons_printf("\r                                 \r"); \
    }                                                         \
    while(0)


#define _PauseWithExit                                                           \
    do                                                                           \
    {                                                                            \
        Cons_printf("  -- Press any key to continue or ESC to exit --");         \
        if (Cons_getch() == 27)                                                  \
        {                                                                        \
            Cons_printf("\r                                                \n"); \
            ConsoleEnd();                                                        \
            exit(0);                                                             \
        }                                                                        \
        Cons_printf("\r                                                \r");     \
    }                                                                            \
    while(0)




/*************************************
 *            Functions
 ************************************/
void
ConsoleInitialize(
    void
    );

void
ConsoleEnd(
    void
    );

unsigned short
ConsoleScreenHeightSet(
    unsigned short NumLines
    );

unsigned short
ConsoleScreenHeightGet(
    void
    );

void
ConsoleIoThrottle(
    unsigned char bEnable
    );

void
ConsoleIoThrottleReset(
    void
    );

void
ConsoleIoOutputDisable(
    unsigned char bEnable
    );

int
PlxPrintf(
    const char *format,
    ...
    );


// Linux-specific functions
int
Plx_kbhit(
    void
    );

int
Plx_getch(
    void
    );

#endif
