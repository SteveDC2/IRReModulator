#include "TIVAMonitor.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/rom_map.h"
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_uart.h"
#include "inc/hw_sysctl.h"
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
#include "Helpers.h"
#include "USBSerial.h"
#include "CommandProcessor.h"
#include "EEPROM.h"
#include "Init.h"

//Is USB plugged in to PC?
volatile bool USBConfigured = false;
//Is traffic active to an application (i.e. terminal window or other application)?
volatile bool USBConnected = false;
uint32_t USB_TX_Timeout = TXDEFAULTTIMEOUT;

uint8_t CommandCircularBuffer[COMMAND_BUFFER_SIZE];
char CommandBuffer[COMMAND_BUFFER_SIZE];
uint8_t CommandWritePointer = 0;
uint8_t CommandReadPointer = 0;
uint8_t CommandCount = 0;

//--------------------------------------------------------------------------------------------------
// IsValidChar
//--------------------------------------------------------------------------------------------------
bool IsValidChar(uint8_t Char)
{
    if (isalpha(Char) || isdigit(Char) || (Char == 32) || (Char == '+') || (Char == '-') || (Char == '.') || (Char == 0))
    {
        return true;
    }
    else
    {
        return false;
    }
}


//*****************************************************************************
//
// When a character is received from USB add it to the command buffer
// and flag a new command if newline received
//
//*****************************************************************************
static void USBUARTPrimeTransmit()
{
    uint32_t ui32Read;
    uint8_t ui8Char;
    uint8_t temp;
    static uint8_t ui8LastChar = 0;
    //
    // Get a character from the buffer.
    //
    ui32Read = USBBufferRead((tUSBBuffer *)&g_sRxBuffer, &ui8Char, 1);
    while (ui32Read)
    {
        USBConnected = true;    //Received something across the vcom so something must be on the other end
        //If echo is enabled then send it back
        if (DeviceInfo.EchoEnable == 1)
        {
            USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)&ui8Char, 1);
        }

        //If character is a new line then flag a new command is received and convert to null terminator
        if ((ui8Char == 10) || (ui8Char == 13))
        {
            //Check for LF-CR and CR-LF combinations and filter by making character invalid and resetting CF/LF checking
            if (((ui8Char == 10) && (ui8LastChar == 13)) || ((ui8Char == 13) && (ui8LastChar == 10)))
            {
                ui8LastChar = 0;
                ui8Char = 1;
            }
            else     //if (CommandWritePointer != CommandReadPointer)
            {
                CommandCount++;
                ui8LastChar = ui8Char;
                ui8Char = 0;
            }
        }
        else
        {
            ui8LastChar = ui8Char;
        }

        if ((CommandWritePointer == CommandReadPointer) && (ui8Char == 32))    //If space is first character then remove it by setting to invalid character
        {
            ui8Char = 1;
        }
//				if ((ui8Char == 0x08) || (ui8Char == 0x1b))//backspace or delete?
        if ((ui8Char == 0x08) || (ui8Char == 0x7f))   //backspace ? Putty defaults to 0x7f for backspace
        {
            if (CommandWritePointer != CommandReadPointer)    //Something is in the buffer so allowed to remove the character
            {
                if (CommandWritePointer == 0)
                {
                    CommandWritePointer = COMMAND_BUFFER_SIZE - 1;
                }
                else
                {
                    CommandWritePointer = CommandWritePointer - 1;
                }
                //Now delete the character from the terminal display by sending a space then a backspace character
                temp = 32;
                USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)&temp, 1);
                temp = 8;
                USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)&ui8Char, 1);
            }
        }
        else if (IsValidChar(ui8Char))    // If alpha or digit or space then add to buffer
        {
            //Put the command in the circular buffer
            CommandCircularBuffer[CommandWritePointer] = toupper(ui8Char);
            //Cycle through the circular buffer
            //NB no overflow checking since not currently needed
            if (CommandWritePointer == COMMAND_BUFFER_SIZE - 1)
            {
                CommandWritePointer = 0;
            }
            else
            {
                CommandWritePointer = CommandWritePointer + 1;
            }
        }
        ui32Read = USBBufferRead((tUSBBuffer *)&g_sRxBuffer, &ui8Char, 1);
    }
}


//*****************************************************************************
//
// Get the communication parameters in use on the UART.
// Just set some defaults. These don't matter at all since 'virtual'
//
//*****************************************************************************
static void GetLineCoding(tLineCoding *psLineCoding)
{
    psLineCoding->ui32Rate = 115200;
    psLineCoding->ui8Databits = 8;
    psLineCoding->ui8Parity = USB_CDC_PARITY_NONE;
    psLineCoding->ui8Stop = USB_CDC_STOP_BITS_1;
}


//*****************************************************************************
//
// Handles CDC driver notifications related to control and setup of the device.
//
// \param pvCBData is the client-supplied callback pointer for this channel.
// \param ui32Event identifies the event we are being notified about.
// \param ui32MsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to perform control-related
// operations on behalf of the USB host.  These functions include setting
// and querying the serial communication parameters, setting handshake line
// states and sending break conditions.
//
// \return The return value is event-specific.
//
//*****************************************************************************
uint32_t ControlHandler(
    void *pvCBData,
    uint32_t ui32Event,
    uint32_t ui32MsgValue,
    void *pvMsgData)
{
    uint32_t ui32IntsOff;

    //
    // Which event are we being asked to process?
    //
    switch (ui32Event)
    {
        //
        // We are connected to a host and communication is now possible.
        //
        case USB_EVENT_CONNECTED:
            USBConfigured = true;
            USBConnected = false;            //Can't assume application is accepting data yet. In fact most likely it isn't
            //
            // Flush our buffers.
            //
            USBSerial_FlushBuffers();
            ui32IntsOff = ROM_IntMasterDisable();
            if (!ui32IntsOff)
            {
                ROM_IntMasterEnable();
            }
            break;

        //
        // The host has disconnected.
        //
        case USB_EVENT_DISCONNECTED:
            USBConfigured = false;
            USBConnected = false;
            ui32IntsOff = ROM_IntMasterDisable();
            if (!ui32IntsOff)
            {
                ROM_IntMasterEnable();
            }
            break;

        //
        // Return the current serial communication parameters.
        //
        case USBD_CDC_EVENT_GET_LINE_CODING:
            GetLineCoding(pvMsgData);
            break;

        //
        // Ignore unnecessary calls
        //
        case USBD_CDC_EVENT_SEND_BREAK:
        case USBD_CDC_EVENT_CLEAR_BREAK:
        case USBD_CDC_EVENT_SET_CONTROL_LINE_STATE:
        case USBD_CDC_EVENT_SET_LINE_CODING:
            break;

        //
        // Ignore SUSPEND and RESUME for now.
        //
        case USB_EVENT_SUSPEND:
        case USB_EVENT_RESUME:
            break;

        //Ignore things we don't expect
        default:
            break;
    }

    return 0;
}


//*****************************************************************************
//
// Handles CDC driver notifications related to the transmit channel (data to
// the USB host).
//
// \param ui32CBData is the client-supplied callback pointer for this channel.
// \param ui32Event identifies the event we are being notified about.
// \param ui32MsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the transmit data channel (the IN channel carrying
// data to the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
uint32_t TxHandler(
    void *pvCBData,
    uint32_t ui32Event,
    uint32_t ui32MsgValue,
    void *pvMsgData)
{
    //
    // Which event have we been sent?
    //
    switch (ui32Event)
    {
        case USB_EVENT_TX_COMPLETE:
            //
            // Since we are using the USBBuffer, we don't need to do anything
            // here.
            //
            break;

        //
        // We don't expect to receive any other events.  Ignore any that show
        // up in a release build or hang in a debug build.
        //
        default:
            #ifdef DEBUG
            while (1)
            {
            }
            #else
            break;
            #endif
    }
    return 0;
}


//*****************************************************************************
//
// Handles CDC driver notifications related to the receive channel (data from
// the USB host).
//
// \param ui32CBData is the client-supplied callback data value for this channel.
// \param ui32Event identifies the event we are being notified about.
// \param ui32MsgValue is an event-specific value.
// \param pvMsgData is an event-specific pointer.
//
// This function is called by the CDC driver to notify us of any events
// related to operation of the receive data channel (the OUT channel carrying
// data from the USB host).
//
// \return The return value is event-specific.
//
//*****************************************************************************
uint32_t RxHandler(
    void *pvCBData,
    uint32_t ui32Event,
    uint32_t ui32MsgValue,
    void *pvMsgData)
{
    //
    // Which event are we being sent?
    //
    switch (ui32Event)
    {
        //
        // A new packet has been received.
        //
        case USB_EVENT_RX_AVAILABLE:
        {
            //
            // Characters received from USB vcom so processes them
            //
            USBUARTPrimeTransmit();
            break;
        }

        //
        // We are being asked how much unprocessed data we have still to
        // process. We return 0 if the UART is currently idle or 1 if it is
        // in the process of transmitting something. The actual number of
        // bytes in the UART FIFO is not important here, merely whether or
        // not everything previously sent to us has been transmitted.
        //
        case USB_EVENT_DATA_REMAINING:
        {
            //
            // Get the number of bytes in the buffer and add 1 if some data
            // still has to clear the transmitter.
            //
            //ui32Count = ROM_UARTBusy(USB_UART_BASE) ? 1 : 0;
            //return(ui32Count);
            //UART not actually used, so never busy
            return 0;
        }

        //
        // We are being asked to provide a buffer into which the next packet
        // can be read. We do not support this mode of receiving data so let
        // the driver know by returning 0. The CDC driver should not be sending
        // this message but this is included just for illustration and
        // completeness.
        //
        case USB_EVENT_REQUEST_BUFFER:
        {
            return 0;
        }

        //
        // We don't expect to receive any other events.
        default:
            break;
    }

    return 0;
}


//*****************************************************************************
//
// Initialization
//
//*****************************************************************************

void USBSerial_ConfigureUSB(void)
{
    //Set GPIO as USB
    MAP_GPIOPinTypeUSBAnalog(MONITOR_USB_P);
    MAP_GPIOPinTypeUSBAnalog(MONITOR_USB_N);
    //
    // Initialize the transmit and receive buffers.
    //
    USBBufferInit(&g_sTxBuffer);
    USBBufferInit(&g_sRxBuffer);

    //
    // Set the USB stack mode to Device mode with VBUS monitoring.
    //
    USBStackModeSet(0, eUSBModeForceDevice, 0);

    //
    // Pass our device information to the USB library and place the device
    // on the bus.
    //
    USBDCDCInit(0, &g_sCDCDevice);

//	//Enable interrupt handler in SRAM
//	IntRegister(INT_USB0, USB0DeviceIntHandler);
    USB_TX_Timeout = TXDEFAULTTIMEOUT;
}


//--------------------------------------------------------------------------------------------------
// USBSerial_SendCharacter
//--------------------------------------------------------------------------------------------------
void USBSerial_SendCharacter(unsigned char Character)
{
    uint32_t ui32Space;

    if (USBConfigured == true)//Is USB actually connected? Note, this does NOT mean anything has actually opened the com port
    {
        if (USBConnected == true)//Does it look like something is accepting data via the vcom port? This stops data clogging up if USB plugged in but no application running to accept the data
        {
            //Wait until enough space in USB send buffer to send character
            DelayCounter = USB_TX_Timeout;    //Set a timeout. If can't send after timeout then likely terminal closed or receiver died
            do
            {
                ui32Space = USBBufferSpaceAvailable((tUSBBuffer *)&g_sTxBuffer);
            } while ((ui32Space < 11) & (DelayCounter != 0));

            //Check if USB timed out
            if (DelayCounter == 0)
            {
                //Timed out so possibly the receiving application died or closed
                USBConnected = false;
                USBSerial_FlushBuffers();
            }
            else
            {
                //Not timed out so OK to send
                USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)&Character, 1);
            }
        }
    }
}


//--------------------------------------------------------------------------------------------------
// USBSerial_SendMessage
//--------------------------------------------------------------------------------------------------
void USBSerial_SendMessage(unsigned char *Message)
{
    uint32_t ui32Space;
    uint32_t MessageSize = 0;
    //uint8_t NLTerminated = 0;

    if (USBConfigured == true)//Is USB actually connected? Note, this does NOT mean anything has actually opened the com port
    {
        if (USBConnected == true)//Does it look like something is accepting data via the com port? This stops data clogging up if USB plugged in but no application running to accept the data
        {
            while ((Message[MessageSize] != 0) && (USBConnected))
            {
                //Wait until enough space in USB send buffer to send at least 2 characters (just in case a NL/CR required)
                DelayCounter = USB_TX_Timeout;//Set a timeout. If can't send after timeout then likely terminal closed or receiver died
                do
                {
                    ui32Space = USBBufferSpaceAvailable((tUSBBuffer *)&g_sTxBuffer);
                } while ((ui32Space < 2) & (DelayCounter != 0));

                //Check if USB timed out
                if (DelayCounter == 0)
                {
                    //Timed out so possibly the receiving application died or closed
                    USBConnected = false;
                }
                else
                {
                    //Not timed out so OK to send
                    //Check if the current character is a new line
                    if (Message[MessageSize] == 0x0A)
                    {
                        if (DeviceInfo.NLFormat == DOS_FORMAT)
                        {
                            USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)"\r\n", 2);
                        }
                        else if (DeviceInfo.NLFormat == UNIX_FORMAT)
                        {
                            USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)"\n", 1);
                        }
                        else
                        {
                            USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)"\r", 1);
                        }
                    }
                    else
                    {
                        USBBufferWrite((tUSBBuffer *)&g_sTxBuffer, (uint8_t *)&Message[MessageSize], 1);
                    }
                    MessageSize++;
                }
            }
        }
    }
//	if (SendLCD == 1)
//	{
//		LCD_Print((uint8_t*)Message, 0);
//	}
}


//--------------------------------------------------------------------------------------------------
// USBSerial_GetNextCommand
//--------------------------------------------------------------------------------------------------
void USBSerial_GetNextCommand(void)
{
    uint8_t CharacterCount = 0;
    uint8_t Character;

    //Make sure there is actually a command in the circular buffer
    if (CommandCount == 0)
    {
        //No, so return empty command
        CommandBuffer[0] = 0x00;
    }
    else
    {
        do
        {
            //Get the next character from the circular buffer
            Character = CommandCircularBuffer[CommandReadPointer];
            //Store to the linear buffer
            CommandBuffer[CharacterCount++] = Character;
            //Cycle the read pointer
            if (CommandReadPointer == (COMMAND_BUFFER_SIZE - 1))
            {
                CommandReadPointer = 0;
            }
            else
            {
                CommandReadPointer++;
            }
            //Do until line feed found or the entire buffer copied
        } while ((Character != 0x00) && (CommandReadPointer != CommandWritePointer));
        //Decrement command count
        CommandCount--;
    }
}


//--------------------------------------------------------------------------------------------------
// USBSerial_FlushBuffers
//--------------------------------------------------------------------------------------------------
void USBSerial_FlushBuffers()
{
    USBBufferFlush(&g_sTxBuffer);
    USBBufferFlush(&g_sRxBuffer);
}


//--------------------------------------------------------------------------------------------------
// USBSerial_FlushCommands
//--------------------------------------------------------------------------------------------------
void USBSerial_FlushCommands()
{
    while (CommandCount != 0)
    {
        USBSerial_GetNextCommand();
    }
}

void USBSerial_Disconnect()
{
    //Note, start out by setting as GPIO and driving low for a while. Hopefully this will trigger a USB bus reset
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_GPIODirModeSet(MONITOR_USB_P, GPIO_DIR_MODE_OUT);
    MAP_GPIODirModeSet(MONITOR_USB_N, GPIO_DIR_MODE_OUT);
    MAP_GPIOPadConfigSet(MONITOR_USB_P, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
    MAP_GPIOPadConfigSet(MONITOR_USB_N, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
    MAP_GPIOPinWrite(MONITOR_USB_P, 0);
    MAP_GPIOPinWrite(MONITOR_USB_N, 0);
    WaitFormS(100);
}
