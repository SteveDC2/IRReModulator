#ifndef __TIVA_H
#define __TIVA_H

#include <stdint.h>

#define LED_BASE GPIO_PORTF_BASE
#define RED_LED     GPIO_PIN_1
#define BLUE_LED    GPIO_PIN_2
#define GREEN_LED   GPIO_PIN_3
#define YELLOW_LED  (GREEN_LED | RED_LED)
#define MAGENTA_LED (RED_LED | BLUE_LED)
#define CYAN_LED    (BLUE_LED | GREEN_LED)
#define WHITE_LED   (BLUE_LED | GREEN_LED | RED_LED)
void TIVA_DFU(void);


#endif // ifndef __TIVA_H
