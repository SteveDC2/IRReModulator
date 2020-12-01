#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <ctype.h>
//#include <stddef.h>
#include <stdbool.h>
//#include <stdint.h>
//#include "inc/tm4c123gh6pm.h"
#include "TIVAMonitor.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
//#include "inc/hw_uart.h"
#include "driverlib/uart.h"
#include "utils/uartstdio.h"
#include "inc/hw_sysctl.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/usb.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/eeprom.h"
#include "usblib/usblib.h"
#include "usblib/usbcdc.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdcdc.h"
#include "utils/ustdlib.h"
#include "usb_serial_structs.h"
#include "utils/uartstdio.h"
#include "driverlib/ssi.h"
#include "USBSerial.h"
#include "Init.h"
#include "USBSerial.h"
#include "EEPROM.h"
#include "Helpers.h"
#include "Tiva.h"

//--------------------------------------------------------------------------------------------------
// ConfigurePins
//--------------------------------------------------------------------------------------------------
void ConfigurePins(void)
{
    //
    // Enable the GPIO ports
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    //Unlock the GPIO inputs which are locked
    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) |= GPIO_PIN_7;

    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= GPIO_PIN_0;

//    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);
//    MAP_GPIOPinWrite(PWR_EN_VAR1, 0);
    MAP_GPIOPinTypeGPIOOutput(LED_BASE, RED_LED|GREEN_LED|BLUE_LED);

    //Enable the Launchpad push button
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);
//    //Enable the interrupts for the GPIO
//    GPIOIntTypeSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_FALLING_EDGE);
//    GPIOIntRegister(GPIO_PORTF_BASE,PortFIntHandler);
//    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
//    GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);

    //Configure the 4 inputs from the IR receivers
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTA_BASE,GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    //Configure the 4 de-modulated outupts
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    //    //Enable the interrupts for the pulse feedback GPIOs
    GPIOIntTypeSet(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,GPIO_BOTH_EDGES);
    GPIOIntRegister(GPIO_PORTD_BASE,PortDIntHandler);
    GPIOIntClear(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);


    //Configure ISR usage monitor pin
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_0);
}

//--------------------------------------------------------------------------------------------------
// Init_SystemInit
//--------------------------------------------------------------------------------------------------
void Init_SystemInit()
{
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    FPULazyStackingEnable();

    // Set the clocking to run from the PLL at 80MHz
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Enable the system tick.
    SysTickPeriodSet(MAP_SysCtlClockGet() / SYSTICKS_PER_SECOND);
    SysTickIntEnable();
    SysTickEnable();
}

//--------------------------------------------------------------------------------------------------
// Init_ResetDefaultEEPROMSettings
//--------------------------------------------------------------------------------------------------
void Init_ResetDefaultEEPROMSettings(void)
{
    uint8_t MapDefault[8] = {1, 2, 3, 7, 0, 0, 0, 0};
    uint16_t CodeDefaults[9] = {0x52ff, 0x6801, 0x9802, 0xb003, 0x3004, 0x1805, 0x7a06, 0x1007, 0x3808};
    //Default configuration is invalid so re-configure and store
    //Scale factor to convert from desired voltage to digital DAC code
    DeviceInfo.NLFormat = 0;
    DeviceInfo.EchoEnable = 1;
    DeviceInfo.IRAddress = 0;
    memcpy(&DeviceInfo.IRInOutMap, &MapDefault, sizeof(DeviceInfo.IRInOutMap));
    memcpy(&DeviceInfo.IRKeyCodes, &CodeDefaults, sizeof(DeviceInfo.IRKeyCodes));
    DeviceInfo.SerialNumber = 12345678;
    EEPROM_StoreSettings();

}


//--------------------------------------------------------------------------------------------------
// Init_ReadEEPROMDefaults
//--------------------------------------------------------------------------------------------------
void Init_ReadEEPROMDefaults()
{
    int State;
    int i;

    //Read settings from EEPROM and check if valid settings exist at indicated configuration
    State = EEPROM_GetSettings();
    //If not valid then set the defaults
    if (State != 0)
    {
        Init_ResetDefaultEEPROMSettings();
    }

    //Copy serial number to USB descriptor
    sprintf((char *)MiscBuffer, "%08d", DeviceInfo.SerialNumber);
    for (i = 0; i < 8; i++)
    {
        g_pui8SerialNumberString[2 + (i * 2)] = MiscBuffer[i];
    }
}

void TimerSamplerISR()
{
    static uint8_t Sample1 = 0xf;
    static uint8_t Sample2 = 0xf;
    static uint8_t Sample3 = 0xf;
    static uint8_t Sample4 = 0xf;
    static uint8_t Sample5 = 0xf;
    static uint8_t Filtered = 0xf;

    MAP_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0xff);
    MAP_TimerIntClear(INTERVAL_SAMPLING_TIMER_BASE, TIMER_TIMA_TIMEOUT);  // Clear the timer interrupt

    Sample5 = Sample4;
    Sample4 = Sample3;
    Sample3 = Sample2;
    Sample2 = Sample1;
    Sample1 = GPIOPinRead(GPIO_PORTA_BASE,GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4) >> 4;
    Filtered = Sample1 & Sample2 & Sample3 & Sample4 & Sample5;

    MAP_GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, Filtered);

    MAP_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_0, 0);
}

void Configure64bitTimer()
{
  MAP_SysCtlPeripheralEnable(COUNTER_64_TIMER_PERIPH);
  MAP_SysCtlDelay(3);

  MAP_TimerConfigure(COUNTER_64_TIMER_BASE, COUNTER_64_TIMER_CFG);
  MAP_SysCtlDelay(3);

//  MAP_TimerLoadSet64(COUNTER_64_TIMER_BASE, 0xffffffffffffffff); //Doesn't seem to do anything
//  MAP_SysCtlDelay(3);

  // Enable the timer
  MAP_TimerEnable(COUNTER_64_TIMER_BASE, COUNTER_64_TIMER);

}

void Configure32bitTimer()
{
    /*
  MAP_SysCtlPeripheralEnable(COUNTER_32_TIMER_PERIPH);
  MAP_SysCtlDelay(3);

  MAP_TimerConfigure(COUNTER_32_TIMER_BASE, COUNTER_32_TIMER_CFG);
  MAP_SysCtlDelay(3);

  MAP_TimerPrescaleSet(COUNTER_32_TIMER_BASE, COUNTER_32_TIMER, 63);

//  MAP_TimerLoadSet(COUNTER_32_TIMER_BASE, COUNTER_32_TIMER, 0); //Doesn't seem to do anything
//  MAP_SysCtlDelay(3);

  // Enable the timer
  MAP_TimerEnable(COUNTER_32_TIMER_BASE, COUNTER_32_TIMER);
*/


    SysCtlPeripheralEnable(COUNTER_32_TIMER_PERIPH);
    TimerDisable(COUNTER_32_TIMER_BASE,COUNTER_32_TIMER);
    TimerConfigure(COUNTER_32_TIMER_BASE, TIMER_CFG_PERIODIC);
    TimerPrescaleSet(COUNTER_32_TIMER_BASE, TIMER_BOTH, 5);
    TimerLoadSet(COUNTER_32_TIMER_BASE, COUNTER_32_TIMER, 0x00ffffff);
    TimerEnable(COUNTER_32_TIMER_BASE, COUNTER_32_TIMER);

}

void ConfigureSamplerTimer()
{

    MAP_SysCtlPeripheralEnable(INTERVAL_SAMPLING_TIMER_PERIPH);
    MAP_SysCtlDelay(3);
    MAP_TimerConfigure(INTERVAL_SAMPLING_TIMER_BASE, INTERVAL_SAMPLING_TIMER_CFG);
    MAP_SysCtlDelay(3);
    TimerIntRegister(INTERVAL_SAMPLING_TIMER_BASE, INTERVAL_SAMPLING_TIMER, TimerSamplerISR);
    MAP_TimerEnable(INTERVAL_SAMPLING_TIMER_BASE, INTERVAL_SAMPLING_TIMER);
    MAP_TimerIntEnable(INTERVAL_SAMPLING_TIMER_BASE, TIMER_TIMA_TIMEOUT);
//    MAP_TimerLoadSet(INTERVAL_SAMPLING_TIMER_BASE, INTERVAL_SAMPLING_TIMER, SysCtlClockGet() / 100000);
//    MAP_TimerLoadSet(INTERVAL_SAMPLING_TIMER_BASE, INTERVAL_SAMPLING_TIMER, SysCtlClockGet() / 50000);
    MAP_TimerLoadSet(INTERVAL_SAMPLING_TIMER_BASE, INTERVAL_SAMPLING_TIMER, SysCtlClockGet() / 60000);
}

void ConfigureTimers()
{
    Configure64bitTimer();
//    Configure32bitTimer();
    ConfigureSamplerTimer();
}

void ConfigureUART()
{
//    UART    Rx Pin  Tx Pin
//    UART0   PA0     PA1
//    UART1   PC4     PC5 *
//    UART2   PD6     PD7
//    UART3   PC6     PC7
//    UART4   PC4     PC5
//    UART5   PE4     PE5
//    UART6   PD4     PD5
//    UART7   PE0     PE1

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlDelay(3);

    MAP_GPIOPinConfigure(GPIO_PC4_U1RX);
    MAP_GPIOPinConfigure(GPIO_PC5_U1TX);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    MAP_SysCtlDelay(3);
    MAP_GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    MAP_UARTClockSourceSet(UART1_BASE, UART_CLOCK_PIOSC);

    MAP_UARTFIFOEnable(UART1_BASE);

    //UARTStdioConfig(0, 115200, 16000000);
    MAP_UARTConfigSetExpClk(UART1_BASE, 16000000, 9600,(UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE |UART_CONFIG_WLEN_8));
}

//--------------------------------------------------------------------------------------------------
// Init_PeripheralInit
//--------------------------------------------------------------------------------------------------
void Init_PeripheralInit(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //Make sure a USB disconnect is triggered
    USBSerial_Disconnect();

    ConfigurePins();

    EEPROM_Initialize();

    Init_ReadEEPROMDefaults();

    ConfigureTimers();

    ConfigureUART();

    //Make sure EEPROM read first (Init_ReadEEPROMDefaults) since sets the USB descriptor serial number, otherwise 0 will be used
    USBSerial_ConfigureUSB();

    MAP_IntEnable(INTERVAL_SAMPLING_TIMER_INT);
    MAP_IntMasterEnable();
}

