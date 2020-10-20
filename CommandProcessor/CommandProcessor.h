#ifndef __COMPROC_H
#define __COMPROC_H

#include <stdint.h>
#include "USBSerial.h"

extern int8_t SendUSB;

void ComProc_ProcessCommand(void);

#endif // ifndef __COMPROC_H
