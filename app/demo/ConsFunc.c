/******************************************************************************
 *
 * File Name:
 *
 *      ConsFunc.c
 *
 * Description:
 *
 *      Provides a common layer to work with the console
 *
 * Revision History:
 *
 *      10-01-08 : PLX SDK v6.10
 *
 ******************************************************************************/


#include <stdarg.h>
#include "ConsFunc.h"




/*************************************
 *            Globals
 ************************************/
static unsigned char  _Gbl_bThrottleOutput = FALSE;
static unsigned char  _Gbl_bOutputDisable  = FALSE;
static unsigned short _Gbl_LineCount       = 0;
static unsigned short _Gbl_LineCountMax    = DEFAULT_SCREEN_SIZE - SCREEN_THROTTLE_OFFSET;




/******************************************************************
 *
 * Function   :  ConsoleInitialize
 *
 * Description:  Initialize the console
 *
 *****************************************************************/
void
ConsoleInitialize(
    void
    )
{
    int            ret;
    struct winsize ConsoleWindow;


    // Get current console size
    ret = ioctl( STDIN_FILENO, TIOCGWINSZ, &ConsoleWindow );

    if (ret == 0)
    {
        // Set the max line count based on current screen size
        _Gbl_LineCountMax = ConsoleWindow.ws_row - SCREEN_THROTTLE_OFFSET;
    }
}




/******************************************************************
 *
 * Function   :  ConsoleEnd
 *
 * Description:  Restore the console
 *
 *****************************************************************/
void
ConsoleEnd(
    void
    )
{
}




/******************************************************************
 *
 * Function   :  ConsoleScreenHeightSet
 *
 * Description:  Sets the size of the console in number of lines
 *
 *****************************************************************/
unsigned short
ConsoleScreenHeightSet(
    unsigned short NumLines
    )
{

    // Not fully supported yet in Linux
    return -1;

    /*************************************************
     * The following code is capable of adjusting the
     * console height, but it does not re-size the
     * terminal window.  It is left here for future
     * reference, but is disabled at this time.
     ************************************************/
  #if 0
    int            ret;
    struct winsize ConsoleWindow;


    // Get current console size
    ret = ioctl( STDIN_FILENO, TIOCGWINSZ, &ConsoleWindow );

    if (ret != 0)
        return -1;

    // Set new window height
    ConsoleWindow.ws_row = NumLines;

    // Adjust window to new height
    ret = ioctl( STDIN_FILENO, TIOCSWINSZ, &ConsoleWindow );

    if (ret != 0)
        return -1;

    // Update internal limit
    _Gbl_LineCountMax = NumLines - SCREEN_THROTTLE_OFFSET;

    return ret;
  #endif

}




/******************************************************************
 *
 * Function   :  ConsoleScreenHeightGet
 *
 * Description:  Returns the height of the current screen size
 *
 *****************************************************************/
unsigned short
ConsoleScreenHeightGet(
    void
    )
{
    return _Gbl_LineCountMax + SCREEN_THROTTLE_OFFSET;
}




/******************************************************************
 *
 * Function   :  ConsoleIoThrottle
 *
 * Description:  Toggle throttling of the console output
 *
 *****************************************************************/
void
ConsoleIoThrottle(
    unsigned char bEnable
    )
{
    _Gbl_bThrottleOutput = bEnable;

    // Reset if disabled
    if (!bEnable)
    {
        _Gbl_LineCount      = 0;
        _Gbl_bOutputDisable = FALSE;
    }
}




/******************************************************************
 *
 * Function   :  ConsoleIoThrottleReset
 *
 * Description:  Resets console line count
 *
 *****************************************************************/
void
ConsoleIoThrottleReset(
    void
    )
{
    _Gbl_LineCount = 0;
}




/******************************************************************
 *
 * Function   :  ConsoleIoOutputDisable
 *
 * Description:  Toggle console output
 *
 *****************************************************************/
void
ConsoleIoOutputDisable(
    unsigned char bEnable
    )
{
    _Gbl_bOutputDisable = bEnable;
}




/*********************************************************************
 *
 * Function   :  PlxPrintf
 *
 * Description:  Outputs a formatted string
 *
 ********************************************************************/
int
PlxPrintf(
    const char *format,
    ...
    )
{
    int      NumChars;
    char    *pChar;
    char     toggle;
    char     pOut[4000];
    va_list  pArgs;


    // Exit if console output disabled
    if (_Gbl_bOutputDisable)
        return 0;

    // Initialize the optional arguments pointer
    va_start(pArgs, format);

    // Build string to write
    NumChars = vsprintf(pOut, format, pArgs);

    // Terminate arguments pointer
    va_end(pArgs);

    // Start at beginning of string
    pChar = pOut;

    while (*pChar != '\0')
    {
        // Display next character
        Cons_putchar( *pChar );

        // Wrap to next line on return
        if (*pChar == '\n')
        {
            // Check if need to pause output
            if (_Gbl_bThrottleOutput)
            {
                // Increment line count
                _Gbl_LineCount++;

                if (_Gbl_LineCount >= _Gbl_LineCountMax)
                {
                    Cons_outstring("-- More (Press any to continue, 'C' for continuous, or 'Q' to quit) --");

                    // Get user input
                    toggle = Cons_getch();

                    // Clear 'More' message
                    Cons_outstring("\r                                                                      \r");

                    if ((toggle == 'C') || (toggle == 'c'))
                    {
                        // Disable throttle output
                        ConsoleIoThrottle( FALSE );
                    }
                    else if ((toggle == 'Q') || (toggle == 'q'))
                    {
                        // Disable any further output
                        ConsoleIoOutputDisable( TRUE );

                        goto _Exit_PlxPrintf;
                    }
                    else
                    {
                        // Reset the line count
                        ConsoleIoThrottleReset();
                    }
                }
            }
        }

        // Go to next character
        pChar++;
    }

_Exit_PlxPrintf:
    // Return number of characters printed
    return NumChars;
}




/*************************************************
 *
 *         Linux-specific functions
 *
 ************************************************/

/******************************************************************
 *
 * Function   :  Plx_kbhit
 *
 * Description:  Determines if input is pending
 *
 *****************************************************************/
int
Plx_kbhit(
    void
    )
{
    int            count;
    struct timeval tv;
    struct termios Tty_Save;
    struct termios Tty_New;


    // Get current terminal attributes
    tcgetattr( STDIN_FILENO, &Tty_Save );

    // Copy attributes
    Tty_New = Tty_Save;

    // Disable canonical mode (handles special characters)
    Tty_New.c_lflag &= ~ICANON;

    // Disable character echo
    Tty_New.c_lflag &= ~ECHO;

    // Set timeouts
    Tty_New.c_cc[VMIN]  = 1;   // Minimum chars to wait for
    Tty_New.c_cc[VTIME] = 1;   // Minimum wait time

    // Set new terminal attributes
    if (tcsetattr( STDIN_FILENO, TCSANOW, &Tty_New ) != 0)
        return 0;

    // Set to no characters pending
    count = 0;

    // Check stdin for pending characters
    if (ioctl( STDIN_FILENO, FIONREAD, &count ) != 0)
        return 0;

    // Restore old settings
    tcsetattr( STDIN_FILENO, TCSANOW, &Tty_Save );

    // Small delay needed to give up CPU slice & allow use in a tight loop
    tv.tv_sec  = 0;
    tv.tv_usec = 1;
    select(1, NULL, NULL, NULL, &tv);

    return count;
}




/******************************************************************
 *
 * Function   :  Plx_getch
 *
 * Description:  Gets a character from the keyboard (with blocking)
 *
 *****************************************************************/
int
Plx_getch(
    void
    )
{
    int            retval;
    char           ch;
    struct termios Tty_Save;
    struct termios Tty_New;


    // Make sure all output data is flushed
    fflush( stdout );

    // Get current terminal attributes
    tcgetattr( STDIN_FILENO, &Tty_Save );

    // Copy attributes
    Tty_New = Tty_Save;

    // Disable canonical mode (handles special characters)
    Tty_New.c_lflag &= ~ICANON;

    // Disable character echo
    Tty_New.c_lflag &= ~ECHO;

    // Set timeouts
    Tty_New.c_cc[VMIN]  = 1;   // Minimum chars to wait for
    Tty_New.c_cc[VTIME] = 1;   // Minimum wait time

    // Set new terminal attributes
    if (tcsetattr( STDIN_FILENO, TCSANOW, &Tty_New ) != 0)
        return 0;

    // Get a single character from stdin
    retval = read( STDIN_FILENO, &ch, 1 ); 

    // Restore old settings
    tcsetattr( STDIN_FILENO, TCSANOW, &Tty_Save );

    if (retval > 0)
        return (int)ch;

    return 0;
}
