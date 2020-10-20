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

unsigned char MiscBuffer[640];
///////////////////////////////////////////////////////////////////////////////////////////////////////


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

/////////////////////////////////////////////////////////////////////////////////////
// Code from IR decode
// Buffer sizes must be 2^n
// e.g 8,16,32,64,128...
#define IRBUFSIZE   64
#define IRMASK      (IRBUFSIZE-1)

// define PORT and PIN that the detector is connected to
#define IR_PORT          GPIO_PORTA_BASE
#define IR_PIN           GPIO_PIN_2

// constants used to configure the TIMER selected
// see ir_init() in remote.c
//#define IR_TIMER_BASE    TIMER1_BASE
//#define IR_TIMER         TIMER_A
//#define IR_TIMER_PERIPH  SYSCTL_PERIPH_TIMER1
//#define IR_TIMER_CFG     TIMER_CFG_A_PERIODIC
#define IR_TIMER_INT1    INT_TIMER1A
#define IR_TIMER_INT2    TIMER_TIMA_TIMEOUT

// Timeout in microseconds  (10000 = 10ms)
#define IR_TIMEOUT_VAL 10000

// number of bits (pulse counts) in 1 message/code
// Sony protocol has 1 start + 12 data bits
#define IR_MAX_BITS_VAL 13

// max number of pulses we will collect
// ** this is way more than we actually need
#define MAX_PULSE_COUNT 100

volatile unsigned int  irbuf[IRBUFSIZE];    // IR byte receive buffer
volatile unsigned char ir_in;               // IR Rx buffer in index
volatile unsigned char ir_out;              // IR Rx buffer out index

unsigned int pulse_buf[MAX_PULSE_COUNT+1];  // pulse width count buffer

volatile unsigned long ir_pulse_count, ir_timeout_flag, ir_ppct;

volatile unsigned long g_ulCountsPerMicrosecond;  // Holds the calculated value
volatile unsigned long g_ulPeriod;                // Holds the IR_TIMER timeout value (in microseconds)

// *************************************************************************
// Lookup table for Sony remote keys   0x80-0x88='1'-'9' keys 0x89='0'
// U=up  D=down R=right L=left P=power M=menu C=ch+ c=ch- V=vol+ v=vol-
// A=audio E=video m=mute I=TV/VCR i=TV/CATV S=MTS p=POWER(VCR) X=1-/11
// Y=2-/12 l=REW s=PLAY r=FF e=REC x=STOP z=PAUSE
const unsigned int dev_000[]={
   0x89,'0',0x90,'C',0x91,'c',0x92,'V',0x93,'v',0x94,'m',0x95,'P',0x9D,'Y',
   0xA5,'I',0xB8,'i',0xDB,'A',0xDC,'D',0xDE,'U',0xDF,'S',0xE0,'M',0xE5,'X',
   0xF4,'R',0xF5,'L',0xFC,'E',0x115,'p',0x11B,'l',0x11A,'s',0x11C,'r',0x11D,'e',
   0x118,'x',0x119,'z',0x00,'?'};



void PA2IntHandler(void);
void flush_irbuf(void);

void ir_init(char bDoIME)
{
  for(ir_in=0; ir_in<IRBUFSIZE; ir_in++)
      irbuf[ir_in]=0;

  flush_irbuf();    // reset irbuf[] input indexes

  //**********************************************************************
  // ***** Configure and Enable TIMERA  **********************************
  //**********************************************************************

  // Calculate the number of timer counts/microsecond
  g_ulCountsPerMicrosecond = ROM_SysCtlClockGet()/1000000;

  // 10ms = timeout delay
  g_ulPeriod= g_ulCountsPerMicrosecond * IR_TIMEOUT_VAL;

  // Enable TIMER1 Peripheral
  MAP_SysCtlPeripheralEnable(IR_TIMER_PERIPH);

  // Configure the TIMER1A
  MAP_TimerConfigure(IR_TIMER_BASE, IR_TIMER_CFG);

  // Initially load the timer with 20ms interval time
  MAP_TimerLoadSet(IR_TIMER_BASE, IR_TIMER, g_ulPeriod);

  // Setup the interrupt for the TIMER1A timeout.
  MAP_IntEnable(IR_TIMER_INT1);
  MAP_TimerIntEnable(IR_TIMER_BASE, IR_TIMER_INT2);

  // Enable the IR_TIMER.
  MAP_TimerEnable(IR_TIMER_BASE, IR_TIMER);

  //**********************************************************************
  // ***** Configure and Enable GPIO PIN interrupt on IR_PIN   ***********
  //**********************************************************************

  // Register IR GPIO pin interrupt handler
//  MAP_GPIOPortIntRegister(IR_PORT, PA2IntHandler);

  // Set IR PIN as INPUT
  MAP_GPIOPinTypeGPIOInput(IR_PORT, IR_PIN);

  // Configure IR Interrupt on FALLING edge
  MAP_GPIOIntTypeSet(IR_PORT, IR_PIN,  GPIO_FALLING_EDGE);

  // Enable GPIO IR PIN interrupt
  MAP_GPIOIntEnable(IR_PORT, IR_PIN);
  //**********************************************************************

  ir_pulse_count=0;             // Reset pulse count
  ir_timeout_flag=0;            // Clear timeout flag

  if(bDoIME)                    //if called with this set
     ROM_IntMasterEnable();     // Enable processor interrupts.

  return;
}

// ***********************************************************************
// Reset the buffer indexes
//************************************************************************
void flush_irbuf(void)
{
 ir_in =0;
 ir_out = 0;
}

// ***********************************************************************
// Report pending chars in RX buffer
//************************************************************************
char irbuflen(void)
{
 return(ir_in - ir_out);
}

//************************************************************************
// Retrieves  16 bit code from IR Buffer.
//************************************************************************
unsigned int ir_getCode(void)
{
    unsigned int c;

    while(irbuflen() == 0);     // Wait for data to be placed in buffer...

    c = irbuf[ir_out & IRMASK]; // Get 1 byte
    ir_out++;                   // update the ir_out index
    return(c);                  // return the byte
}

//************************************************************
// decode_dev000(code)
// Convert the code from the remote to a more friendly single
// byte character from the lookup table for Sony remote control
//************************************************************
char decode_dev000(unsigned int code)
{
 int p, sz;

 if((code>0x7F)&&(code<0x89))    // 0x80-0x89 = '1'-'9' buttons
    return code-0x4F;
 else
 {
  sz=sizeof(dev_000)/sizeof(int);
  p=0;
  while(p < sz) // other buttons use lookup.
    {
     if(code == dev_000[p])
       return (char)dev_000[p+1];

     p+=2;
    }
 }
 return '?';   // Not found
}

//************************************************************************
// decodePulseBuffer(pulse_buf, code_buf) (Sony protocol)
// pulse_buf contains 13 bytes representing 1 start pulse(4t) and 12 bytes
// representing 12 data bits. counts of 2t="0" and 3t="1" (1t=approx 600us)
// Bit pattern for the Sony Power button code 0x095=(lsb-msb)0101 1001 000
// (shown inverted as it would appear from the detector chip output)
// ____      _   _    _   _    _    _   _   _    _   _   _   _   ______
//     |____| |_| |__| |_| |__| |__| |_| |_| |__| |_| |_| |_| |_|
//
//    Start | 0 | 1  | 0 | 1  | 1  | 0 | 0 | 1  | 0 | 0 | 0 | 0 |
//************************************************************************
int decodePulseBuffer(unsigned int *pulse_buf)
{
 unsigned int bp, ctZeroBit;
 unsigned int code;

 code=0;
 bp = 0;

 // First pulse is a start bit representing 4 time periods(approx. 2.4ms)
 // calculate 2.5 time periods as the threshold between 2t and 3t
 // any pulse width greater that 2.5t will be considered a logical '1'
 ctZeroBit = (pulse_buf[0]/8)*5;

 for(bp=IR_MAX_BITS_VAL-1; bp>0; bp--)   //Start with pulse/bit 12 (MSB)
    {
       code=(code << 1);               //Shift the bits left.

     if(pulse_buf[bp] > ctZeroBit)   // If a '1' is detected..
       code |= 1;                    // Set bit 0
     else
       code &= 0xFFFE;               // Clear bit 0
    }

 return (int)code;
}

//*****************************************************************************
// The timeout interrupt handler for timer1
// On each TIMER1A timeout, we will assume code transmission (if any) is
// complete since we haven't gotten any pulses for 10ms.
// If pulse counts were accumulated, decode the pulse buffer and save the
// resulting code. update the irbuf input index and save the pulse count.
// Finally; reset the pulse count, TIMER1A, set the PIN interrupt to look for
// the next falling edge and set the timeout flag.
//*****************************************************************************
void Timer1IntHandler(void)
{
 unsigned int code;

 // Clear the timer interrupt.
 ROM_TimerIntClear(IR_TIMER_BASE, IR_TIMER_INT2);

 ir_timeout_flag=1;

 //Reload the 10ms timeout val
 TimerLoadSet(IR_TIMER_BASE, IR_TIMER, g_ulPeriod);

 // Reset to Falling Edge
 ROM_GPIOIntTypeSet(IR_PORT, IR_PIN,  GPIO_FALLING_EDGE);

 if(ir_pulse_count>8)   // If we got data - add it to irbuf[]
   {
    // decode and store translated code to irbuf[]
    code = decodePulseBuffer(pulse_buf);

    irbuf[ir_in & IRMASK]= code;  // add to the buffer
    ir_in++;                      // Increment receive index.
    ir_ppct=ir_pulse_count-1;     // save the # of pulses read
   }

 ir_pulse_count=0;               // reset pulse counter

// **DEMO CODE** //turn OFF the on-board LEDs
  ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1, 0);
// **END DEMO CODE**
}

//*****************************************************************************
// The GPIO interrupt handler for PA2
// On the first falling edge and each following rising edge of the input pin
// this routine will be invoked.
// It will save the width of the pulse, count it and reset the timer.
//*****************************************************************************
void PA2IntHandler(void)
{
  unsigned long ulTimerVal, ulLeds;

  // Clear the PIN interrupt.
  MAP_GPIOIntClear(IR_PORT, IR_PIN);

  ulTimerVal = TimerValueGet(IR_TIMER_BASE, IR_TIMER);  //Read timer value

  // Reset the timer
  TimerIntClear(IR_TIMER_BASE, IR_TIMER_INT2);
    TimerLoadSet(IR_TIMER_BASE, IR_TIMER, g_ulPeriod);
    ir_timeout_flag=0;

    if(ir_pulse_count==0)
      {
       ROM_GPIOIntTypeSet(IR_PORT, IR_PIN,  GPIO_RISING_EDGE);
      }
    else
      {
       if(ir_pulse_count < MAX_PULSE_COUNT)
          pulse_buf[ir_pulse_count-1] = (int)(g_ulPeriod - ulTimerVal)/g_ulCountsPerMicrosecond;
      }

    ir_pulse_count++;

    // **DEMO CODE** *** flash the on-board LEDs to indicate pulses are being received ***
  ulLeds = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1);
    ulLeds = (ulLeds+2) & 0x0E;
    ROM_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1, ulLeds);
    // **END DEMO CODE**
}

//*****************************************************************************
//
// This is the main application entry function.
//
//*****************************************************************************

int main(void)
{
    volatile uint16_t GPIOState;

    //Initialize stack, clocks and system ticker
    Init_SystemInit();
  
    //Initialize TIVA sub-systems
    Init_PeripheralInit();

//    ir_init(1);
		
    while (1)
    {
        ComProc_ProcessCommand();
    }
}

/*
  if(irbuflen() > 0 )       // Any IR codes available?
   {
    irCode = ir_getCode();  // Read a code
    irChar = decode_dev000(irCode);   // Translate to a single character

    // Throw away anything that we get in the next 200ms
    // in case we held the button down too long
    SysCtlDelay(g_ulCountsPerMicrosecond * (200000/3));  // wait 200ms
    flush_irbuf();                            // discard any pending data
   }


*/
