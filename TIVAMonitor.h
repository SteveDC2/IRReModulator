#ifndef __TIVAMONITOR_H
#define __TIVAMONITOR_H

#include <stdint.h>
#include "EEPROM.h"

//*****************************************************************************
//
// The system tick rate expressed both as ticks per second and a millisecond
// period.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 1000
#define SYSTICK_PERIOD_MS (1000 / SYSTICKS_PER_SECOND)

#define SETHIGH        0xff
#define SETLOW         0x00

#define VALID_KEY 0x2869ea83
#define INVALID_KEY 0xffffffff

#define LED_CYCLE_DELAY 30

extern const unsigned char FWVersion[];
extern const unsigned char BuildDate[];
extern const unsigned char BuildTime[];
extern uint32_t SerialNumber;
extern DeviceInfoType DeviceInfo;

extern unsigned char MiscBuffer[];
extern unsigned char SmallBuffer[];

extern void PortDIntHandler();

#endif // ifndef __TIVAMONITOR_H
