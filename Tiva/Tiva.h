#ifndef __TIVA_H
#define __TIVA_H

#include <stdint.h>

#define LED_BASE GPIO_PORTF_BASE
#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3


void TIVA_DFU(void);


#endif // ifndef __TIVA_H
