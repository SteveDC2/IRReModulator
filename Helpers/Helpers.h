#ifndef __HELPERS_H
#define __HELPERS_H

#include <stdint.h>

extern volatile uint32_t DelayCounter;

void WaitFormS(unsigned int Delayms);
void WaitForuS(unsigned int Delayus);
uint8_t CommandMatch(
    char *SearchIn,
    char *SearchFor);
uint8_t SubCommandMatch(
    char *SearchIn,
    char *SearchFor);
char *SeekFirstCharacter(char *Buffer);
#endif // ifndef __HELPERS_H
