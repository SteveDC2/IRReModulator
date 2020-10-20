#include "inc/tm4c123gh6pm.h"
#include "TIVAMonitor.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_uart.h"
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "USBSerial.h"
#include "Init.h"
#include "USBSerial.h"
#include "EEPROM.h"
#include "Helpers.h"
#include "Tiva.h"
#include "Tiva.h"

void ProcessCounterTrigger()
{
    static uint64_t LastTimerValue = 0;
    uint64_t CurrentTimerValue;
    uint64_t Delta;
    char Temp[32];

    CurrentTimerValue = TimerValueGet64(IR_TIMER_BASE);
    Delta = (CurrentTimerValue - LastTimerValue);
    LastTimerValue = CurrentTimerValue;
    sprintf((char*)Temp, "%15llu\n", Delta);
    USBSerial_SendMessage((unsigned char *)Temp);
}

void PortFIntHandler()
{
    uint32_t status=0;

    status = GPIOIntStatus(GPIO_PORTF_BASE,true);
    GPIOIntClear(GPIO_PORTF_BASE,status);

    if( (status & GPIO_INT_PIN_4) == GPIO_INT_PIN_4)
    {
      //Then there was a pin4 interrupt i.e. the push button
        ProcessCounterTrigger();
    }

    if( (status & GPIO_INT_PIN_5) == GPIO_INT_PIN_5)
    {
      //Then there was a pin5 interrupt
      //etc...
    }
}


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
    //Enable the interrupts for the GPIO
    GPIOIntTypeSet(GPIO_PORTF_BASE,GPIO_PIN_4,GPIO_FALLING_EDGE);
    GPIOIntRegister(GPIO_PORTF_BASE,PortFIntHandler);
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
    GPIOIntClear(GPIO_PORTF_BASE, GPIO_INT_PIN_4);
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
    //Default configuration is invalid so re-configure and store
    //Scale factor to convert from desired voltage to digital DAC code
    DeviceInfo.NLFormat = 0;
    DeviceInfo.EchoEnable = 1;
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

void ConfigureTimer()
{
  /*
    Set the timer to be periodic.
  */

//  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
//  MAP_SysCtlDelay(3);
//  MAP_TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC);
//  MAP_TimerEnable(TIMER2_BASE,TIMER_A);

  // Enable TIMER1 Peripheral
  MAP_SysCtlPeripheralEnable(IR_TIMER_PERIPH);
  MAP_SysCtlDelay(3);

  // Configure the TIMER1A
  MAP_TimerConfigure(IR_TIMER_BASE, IR_TIMER_CFG);
  MAP_SysCtlDelay(3);

  MAP_TimerLoadSet64(IR_TIMER_BASE, 0xffffffffffffffff); //Doesn't seem to do anything
  MAP_SysCtlDelay(3);

//  // Setup the interrupt for the TIMER1A timeout.
//  MAP_IntEnable(IR_TIMER_INT1);
//  MAP_TimerIntEnable(IR_TIMER_BASE, IR_TIMER_INT2);

  // Enable the IR_TIMER.
  MAP_TimerEnable(IR_TIMER_BASE, IR_TIMER);

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

    ConfigureTimer();

    //Make sure EEPROM read first (Init_ReadEEPROMDefaults) since sets the USB descriptor serial number, otherwise 0 will be used
    USBSerial_ConfigureUSB();
}

