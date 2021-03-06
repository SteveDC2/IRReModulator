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
#include "inc/hw_nvic.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/usb.h"
#include "driverlib/pwm.h"
#include "driverlib/rom.h"
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
#include "Tiva.h"
#include "Helpers.h"
#include "Tiva.h"
#include "USBSerial.h"
#include "CommandProcessor.h"
#include "EEPROM.h"
#include "Init.h"
#include "inc/hw_i2c.h"
#include "driverlib/i2c.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "utils/uartstdio.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////
const unsigned char FWVersion[] = {"Project FW version 1.00"};
const unsigned char BuildDate[] = "Build date = " __DATE__;
const unsigned char BuildTime[] = "Build Time = " __TIME__;

uint32_t SerialNumber = 0;
DeviceInfoType DeviceInfo;

const unsigned char NoMessage[] = "No";
const unsigned char YesMessage[] = "Yes";
const unsigned char *DisplayYesNoMessagePointer[] = {NoMessage, YesMessage};

uint8_t IRColor[8] = {RED_LED, GREEN_LED, BLUE_LED, YELLOW_LED, BLUE_LED, GREEN_LED, YELLOW_LED, MAGENTA_LED};

uint8_t IRState[4] = {0, 0, 0, 0};
uint8_t IRDataPhase[4] = {0, 0, 0, 0};
uint8_t IRAddress[4] = {0, 0, 0, 0};
uint8_t IRNotAddress[4] = {0, 0, 0, 0};
uint8_t IRData[4] = {0, 0, 0, 0};
uint8_t IRNotData[4] = {0, 0, 0, 0};
uint8_t IRNewData = 0;
uint16_t IRReceived[4] = {0, 0, 0, 0};
uint64_t IRLastCounter[4] = {0, 0, 0, 0};
uint64_t IRBitCounter[4] = {0, 0, 0, 0};
uint64_t CurrentTimerValue;
uint16_t LEDDelay = 0;
//const uint32_t TicksPeruS = 80000000 / 1000000;
#define TicksPeruS 80

bool LearnMode = false;

bool Stop = false;
uint8_t Temp;


unsigned char MiscBuffer[640];
//*****************************************************************************
//
// Interrupt handler for the system tick counter.
//
//*****************************************************************************
void SysTickIntHandler(void)
{
    if (DelayCounter > 0)
    {
        DelayCounter--;
    }
}

inline bool InRange(uint64_t Value, uint64_t Min, uint64_t Max)
{
    if (Stop)
        Temp = 1;

    if ((Value > Min) && (Value < Max))
        return true;
    else
        return false;
}

inline void UpdateAddressData(uint8_t LSB, uint8_t Channel, uint8_t PosNegAddData)
{

    if (IRBitCounter[Channel] == 2)
        Temp++;

    if (PosNegAddData == 0)
        IRAddress[Channel] = (IRAddress[Channel] << 1) | LSB;
    else if (PosNegAddData == 1)
        IRNotAddress[Channel] = (IRNotAddress[Channel] << 1) | LSB;
    else if (PosNegAddData == 2)
        IRData[Channel] = (IRData[Channel] << 1) | LSB;
    else if (PosNegAddData == 3)
        IRNotData[Channel] = (IRNotData[Channel] << 1) | LSB;

    if (IRBitCounter[Channel] == 7)
    {
        IRState[Channel]++;
        IRBitCounter[Channel] = 0;
        IRDataPhase[Channel]++;
    }
    else
    {
        IRBitCounter[Channel]++;
    }
}

//Spec = 9ms
//Measured = 10.6
#define PREAMBLE_1_PERIOD_US_LOW  (TicksPeruS * 7200)
#define PREAMBLE_1_PERIOD_US_HIGH (TicksPeruS * 12200)
//Spec = 4.5ms
//Measured = 4.5ms
#define PREAMBLE_2_PERIOD_US_LOW  (TicksPeruS * 3500)
#define PREAMBLE_2_PERIOD_US_HIGH (TicksPeruS * 5500)
//Spec = 562us
//Measured = 652us (716 from matrix switcher!)
#define BIT_LEADIN_PERIOD_US_LOW  (TicksPeruS * 330)
#define BIT_LEADIN_PERIOD_US_HIGH (TicksPeruS * 890)
//Spec = 562us
//Measured =652us (520 from matrix switcher)
#define BIT_ZERO_PERIOD_US_LOW    (TicksPeruS * 330)
#define BIT_ZERO_PERIOD_US_HIGH   (TicksPeruS * 890)
//Spec = 1675us
//Measured =1700us
#define BIT_ONE_PERIOD_US_LOW     (TicksPeruS * 1175)
#define BIT_ONE_PERIOD_US_HIGH    (TicksPeruS * 2075)


void ProcessIRTrigger(uint8_t Channel, uint8_t State)
{
    uint64_t Delta;
    uint8_t ErrorFlag;

    ErrorFlag = 0;

    if (IRState[Channel] == 0)
    {
        if (State == 0)//Idle, moving in to 9ms leader. Just make a note of "Now" and move on
        {
            IRLastCounter[Channel] = CurrentTimerValue;
            IRState[Channel] ++;
        }
        //Idle state with state != 0 is not valid to just stay in this state
    }
    else if (IRState[Channel] == 1)
    {
        //We were in the 9ms pre-amble, now moving to 4.5ms pre-amble. Check the period makes sense
        Delta = CurrentTimerValue - IRLastCounter[Channel];
        if (InRange(Delta, PREAMBLE_1_PERIOD_US_LOW, PREAMBLE_1_PERIOD_US_HIGH))
        {
            IRLastCounter[Channel] = CurrentTimerValue;
            IRState[Channel] ++;
        }
        else
        {
            //Not a valid period so move back to idle state
            IRLastCounter[Channel] = CurrentTimerValue;
            IRState[Channel] = 8;
            ErrorFlag = 1;
        }
    }
    else if (IRState[Channel] == 2)
    {
        //We were in the 4.5ms pre-amble, now moving to positive address phase. Check the period makes sense
        Delta = CurrentTimerValue - IRLastCounter[Channel];
        if (InRange(Delta, PREAMBLE_2_PERIOD_US_LOW, PREAMBLE_2_PERIOD_US_HIGH)) //~ 3.5 to 5.5ms
        {
            IRLastCounter[Channel] = CurrentTimerValue;
            IRBitCounter[Channel] = 0;
            IRDataPhase[Channel] = 0;
//            IRNewData = IRNewData & (~(1 << Channel));
            IRState[Channel] ++;
        }
        else
        {
            //Not a valid period so move back to idle state
            IRLastCounter[Channel] = CurrentTimerValue;
            IRState[Channel] = 8;
            ErrorFlag = 2;
        }
    }
    else if ((IRState[Channel] == 3) || (IRState[Channel] == 4) || (IRState[Channel] == 5) || (IRState[Channel] == 6))
    {
        Delta = CurrentTimerValue - IRLastCounter[Channel];
        if (State != 0)//Bit marker region
        {
            if (!InRange(Delta, BIT_LEADIN_PERIOD_US_LOW, BIT_LEADIN_PERIOD_US_HIGH))
            {
                //Bad so go back to idle state
                IRState[Channel] = 8;
                ErrorFlag = 3;
            }
        }
        else//Bit data region
        {
            if (InRange(Delta, BIT_ZERO_PERIOD_US_LOW, BIT_ZERO_PERIOD_US_HIGH))
            {
                //Period denotes a 0
                UpdateAddressData(0, Channel, IRDataPhase[Channel]);
            }
            else if (InRange(Delta, BIT_ONE_PERIOD_US_LOW, BIT_ONE_PERIOD_US_HIGH))
            {
                //Period denotes a 1
                UpdateAddressData(1, Channel, IRDataPhase[Channel]);
            }
            else
            {
                //Bad so go back to idle state
                IRState[Channel] = 8;
                ErrorFlag = 4 + IRDataPhase[Channel];
            }
        }
        IRLastCounter[Channel] = CurrentTimerValue;
    }
    else if (IRState[Channel] == 7)
    {
        if ((IRAddress[Channel] == (IRNotAddress[Channel] ^ 255))
         && (IRData[Channel] == (IRNotData[Channel] ^ 255))
         && ((IRAddress[Channel] == DeviceInfo.IRAddress) | LearnMode))
        {
            IRReceived[Channel] = (IRAddress[Channel] << 8) | IRData[Channel];
            IRNewData = IRNewData | (1 << Channel);
        }
        IRState[Channel] = 8;
    }
    if (IRState[Channel] == 8)//Used for debugging
    {
        IRState[Channel] = 0;
    }
}

void PortDIntHandler()
{
    uint32_t status=0;
    uint8_t PinState;

    status = GPIOIntStatus(GPIO_PORTD_BASE,true);
    GPIOIntClear(GPIO_PORTD_BASE,status);

    //Shouldn't need to debounce when generated internally for IR code
//    //Disabling and re-enabling interrupts seems to be enough time to debounce
//    GPIOIntDisable(GPIO_PORTD_BASE, GPIO_INT_PIN_7 | GPIO_INT_PIN_6);

    PinState = GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    //CurrentTimer is global so no need to push/pop/move
//    CurrentTimerValue = TimerValueGet(COUNTER_32_TIMER_BASE, COUNTER_32_TIMER);
    CurrentTimerValue = TimerValueGet64(COUNTER_64_TIMER_BASE);


    if( (status & GPIO_INT_PIN_0) != 0)
        ProcessIRTrigger(0, PinState & (1 << 0));
    if( (status & GPIO_INT_PIN_1) != 0)
        ProcessIRTrigger(1,PinState & (1 << 1));
    if( (status & GPIO_INT_PIN_2) != 0)
        ProcessIRTrigger(2,PinState & (1 << 2));
    if( (status & GPIO_INT_PIN_3) != 0)
        ProcessIRTrigger(3,PinState & (1 << 3));

//    GPIOIntEnable(GPIO_PORTD_BASE, GPIO_INT_PIN_7 | GPIO_INT_PIN_6);

}

//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************

void ReportIRKey()
{
    uint8_t i;
    uint8_t Key;
    uint8_t NewData;

    GPIOIntDisable(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    NewData = IRNewData;
    IRNewData = 0;
    GPIOIntEnable(GPIO_PORTD_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

    for (i = 0; i < 4; i ++)
    {
        if (NewData & (1 << i) != 0)
        {
            Key = IRReceived[i] & 0xff;
            sprintf((char*)MiscBuffer, "%02X\n", Key);
            USBSerial_SendMessage((unsigned char *)MiscBuffer);
        }
    }
}

void UARTSendBlocking(unsigned char* Buffer)
{
    uint8_t Index = 0;

    while(Buffer[Index] != 0)
    {
        UARTCharPut(UART1_BASE, Buffer[Index]);
        Index++;
    }
}

uint8_t KeyToSource(uint8_t Key)
{
    uint8_t Index = 0;
    uint16_t ScanCode;

    for (Index = 0; Index < 9; Index++)
    {
        ScanCode = DeviceInfo.IRKeyCodes[Index];
        if ((ScanCode >> 8) == Key)
            return DeviceInfo.IRKeyCodes[Index] & 0xff;
    }
    return 0;
}

void SendSetup()
{
    uint8_t i;

    MAP_GPIOPinWrite(LED_BASE, RED_LED|BLUE_LED|GREEN_LED, WHITE_LED);
    LEDDelay = 1000;

    UARTSendBlocking((unsigned char *)"%0901.\r");//Set forced carrier mode
    WaitFormS(50);
    for (i = 1; i <= 8; i++)
    {
        sprintf((char*)MiscBuffer, "%dR%d.\r", i, i);//Reset each IR channel to the corresponding target port and not the
        UARTSendBlocking((unsigned char *)MiscBuffer);
        WaitFormS(50);
    }
}

void SetMatrixSource()
{
    uint8_t i;
    uint8_t NewData;
    uint8_t TargetChannel;
    uint8_t TargetSource;

    MAP_IntDisable(INTERVAL_SAMPLING_TIMER_INT);
    NewData = IRNewData;
    IRNewData = 0;
    MAP_IntEnable(INTERVAL_SAMPLING_TIMER_INT);

    for (i = 0; i < 4; i ++)
    {
        if ((NewData & (1 << i)) != 0)
        {
            TargetSource = KeyToSource(IRReceived[i] & 0xff);
            if (TargetSource == 0xff)
            {
                SendSetup();
            }
            else if (TargetSource >= 1)
            {
                MAP_GPIOPinWrite(LED_BASE, RED_LED|BLUE_LED|GREEN_LED, IRColor[i]);
                LEDDelay = 65000;
                TargetChannel = DeviceInfo.IRInOutMap[i];
                sprintf((char*)MiscBuffer, "%dV%d.\r", TargetSource, TargetChannel);
                UARTSendBlocking((unsigned char *)MiscBuffer);
            }
        }
    }
}

int main(void)
{
    //Initialize stack, clocks and system ticker
    Init_SystemInit();
  
    //Initialize TIVA sub-systems
    Init_PeripheralInit();

    WaitFormS(3000);//Let the RS232 transceiver charge up
    //Make sure default IR modes are set
    SendSetup();

    while (1)
    {
        ComProc_ProcessCommand();
        if (IRNewData != 0)
        {
            //ReportIRKey();
            SetMatrixSource();
        }
        if (LEDDelay != 0)
        {
            LEDDelay--;
            if (LEDDelay == 0)
                MAP_GPIOPinWrite(LED_BASE, RED_LED|BLUE_LED|GREEN_LED, 0);
        }
        if (MAP_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0)
        {
            SendSetup();//This should also debounce the button
            while((MAP_GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4) == 0));//Wait for button release
            WaitFormS(100);//Debounce button release
        }
    }
}

