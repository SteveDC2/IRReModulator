#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "driverlib/rom.h"
#include "TIVAMonitor.h"
#include "Helpers.h"
#include "CommandProcessor.h"
#include "math.h"

volatile uint32_t DelayCounter = 0;

#define YESBUTTON 10, 195, 120, 35
#define NOBUTTON 190, 195, 120, 35

//--------------------------------------------------------------------------------------------------
// WaitFormS
//--------------------------------------------------------------------------------------------------
void WaitFormS(unsigned int Delayms)
{
//	DelayCounter = Delayms;
//	while(DelayCounter != 0);

    //delay (in cycles) = 3 * parameter
//	if (Delayms > 4290)//Fail if delay too large
//		while(1);
    if (Delayms != 0)//Needed due to bug in ROM_SysCtlDelay which incorrectly decrements before check so waits too long for 0 value
    {
        ROM_SysCtlDelay(((uint64_t)ROM_SysCtlClockGet() * Delayms) / 3000);
    }
}


//--------------------------------------------------------------------------------------------------
// WaitForuS
//--------------------------------------------------------------------------------------------------
void WaitForuS(unsigned int Delayus)
{
//	volatile uint32_t TPS;
//	volatile uint32_t TD;
    //delay (in cycles) = 3 * parameter
//	TPS = SysCtlClockGet();
//	TD = ((SysCtlClockGet() >> 10) * (uint32_t)Delayus) / 2929;
    //Shift and divide effectively ~3000000. Hopefully faster than 2 divides
    //Need 2 divides to stop overflow of times greater than 86uS
    if (Delayus != 0)//Needed due to bug in ROM_SysCtlDelay which incorrectly decrements before check so waits too long for 0 value
    {
        ROM_SysCtlDelay(((ROM_SysCtlClockGet() >> 10) * (uint32_t)Delayus) / 2929);
    }
}


//Check if the buffer matches the string with nothing following (i.e. 0x00 terminated)
uint8_t CommandMatch(
    char *SearchIn,
    char *SearchFor)
{
    uint8_t SearchLength;

    SearchLength = strlen(SearchFor);

    if (memcmp(SearchIn, SearchFor, SearchLength + 1) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


//Check if the buffer matches the sub string with a space at the end. The space needs to be passed in
uint8_t SubCommandMatch(
    char *SearchIn,
    char *SearchFor)
{
    uint8_t SearchLength;

    SearchLength = strlen(SearchFor);

    if (memcmp(SearchIn, SearchFor, SearchLength) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}


//Return a pointer to the first non-space character in the buffer passed
char *SeekFirstCharacter(char *Buffer)
{
    while (Buffer[0] == 32)
    {
        Buffer++;
    }

    return Buffer;
}

