#ifndef __INIT_H
#define __INIT_H

#include <stdint.h>

//64 bit interval timer
#define COUNTER_64_TIMER         TIMER_BOTH
#define COUNTER_64_TIMER_PERIPH  SYSCTL_PERIPH_WTIMER0
#define COUNTER_64_TIMER_BASE    WTIMER0_BASE
#define COUNTER_64_TIMER_CFG     TIMER_CFG_A_PERIODIC_UP

//32 bit interval timer
#define COUNTER_32_TIMER         TIMER_A
#define COUNTER_32_TIMER_PERIPH  SYSCTL_PERIPH_TIMER1
#define COUNTER_32_TIMER_BASE    TIMER1_BASE
#define COUNTER_32_TIMER_CFG     TIMER_CFG_A_PERIODIC

#define INTERVAL_SAMPLING_TIMER          TIMER_A
#define INTERVAL_SAMPLING_TIMER_PERIPH   SYSCTL_PERIPH_TIMER0
#define INTERVAL_SAMPLING_TIMER_BASE     TIMER0_BASE
#define INTERVAL_SAMPLING_TIMER_CFG      TIMER_CFG_PERIODIC
#define INTERVAL_SAMPLING_TIMER_INT      INT_TIMER0A

/*
#define IR_TIMER         TIMER_A
#define IR_TIMER_PERIPH  SYSCTL_PERIPH_TIMER1
#define IR_TIMER_BASE    TIMER1_BASE
#define IR_TIMER_CFG     TIMER_CFG_A_PERIODIC
*/

extern void Init_SystemInit(void);
extern void Init_PeripheralInit(void);
extern void Init_SetDefaults(void);
extern void Init_ResetDefaultEEPROMSettings(void);
extern void Init_RerunUserSettings(void);
extern void Init_SetFPGADefaults(void);
#endif // ifndef __INIT_H
