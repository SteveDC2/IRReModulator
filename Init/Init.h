#ifndef __INIT_H
#define __INIT_H

#include <stdint.h>

#define IR_TIMER         TIMER_BOTH
#define IR_TIMER_PERIPH  SYSCTL_PERIPH_WTIMER0
#define IR_TIMER_BASE    WTIMER0_BASE
#define IR_TIMER_CFG     TIMER_CFG_A_PERIODIC_UP

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
