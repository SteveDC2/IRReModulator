#ifndef __USBSERIAL_H
#define __USBSERIAL_H

#include <stdint.h>

extern void USBSerial_SendMessage(unsigned char *Message);
extern uint8_t USBSerial_GetBytes(
    uint8_t ByteCount,
    uint8_t *Buffer,
    uint32_t Timeout);
void USBSerial_ConfigureUSB(void);
void USBSerial_GetNextCommand(void);
extern uint8_t USBSerialMode;
extern uint32_t USB_TX_Timeout;
void USBSerial_SendCharacter(unsigned char Character);
void USBSerial_FlushBuffers(void);
void USBSerial_Disconnect(void);

#define DOS_FORMAT     0
#define UNIX_FORMAT    1
#define PRE_OSX_FORMAT 2
#define LAST_FORMAT    3

#define USBBINARYMODE  0
#define USBTEXTMODE    1

#define COMMAND_BUFFER_SIZE 32

extern uint8_t CommandCircularBuffer[COMMAND_BUFFER_SIZE];
extern char CommandBuffer[COMMAND_BUFFER_SIZE];
extern uint8_t CommandCount;
extern uint8_t CommandWritePointer;
extern uint8_t CommandReadPointer;


//USB transmit timeout before assuming receiver has closed/died
#define TXDEFAULTTIMEOUT      (5 * SYSTICKS_PER_SECOND)

//*****************************************************************************
//
// Defines required to redirect UART0 via USB.
//
//*****************************************************************************
//#define USB_UART_BASE           UART0_BASE
//#define USB_UART_PERIPH         SYSCTL_PERIPH_UART0
//#define USB_UART_INT            INT_UART0

//*****************************************************************************
//
// Default line coding settings for the redirected UART.
//
//*****************************************************************************
//#define DEFAULT_BIT_RATE        115200
//#define DEFAULT_UART_CONFIG     (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | \
                                 UART_CONFIG_STOP_ONE)

//*****************************************************************************
//
// GPIO peripherals and pins muxed with the redirected UART.  These will depend
// upon the IC in use and the UART selected in USB_UART_BASE.  Be careful that
// these settings all agree with the hardware you are using.
//
//*****************************************************************************

//*****************************************************************************
//
// Defines required to redirect UART0 via USB.
//
//*****************************************************************************
//#define TX_GPIO_BASE            GPIO_PORTA_BASE
//#define TX_GPIO_PERIPH          SYSCTL_PERIPH_GPIOA
//#define TX_GPIO_PIN             GPIO_PIN_1

//#define RX_GPIO_BASE            GPIO_PORTA_BASE
//#define RX_GPIO_PERIPH          SYSCTL_PERIPH_GPIOA
//#define RX_GPIO_PIN             GPIO_PIN_0

#define MONITOR_USB_P GPIO_PORTD_BASE, GPIO_PIN_4
#define MONITOR_USB_N GPIO_PORTD_BASE, GPIO_PIN_5

#endif // ifndef __USBSERIAL_H
