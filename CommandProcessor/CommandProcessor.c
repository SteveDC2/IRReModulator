#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include "driverlib/eeprom.h"
#include "TIVAMonitor.h"
#include "Helpers.h"
#include "TIVA.h"
#include "CommandProcessor.h"
#include "USBSerial.h"
#include <string.h>
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "Init.h"
#include "driverlib/timer.h"

int8_t SendUSB = 0;

//--------------------------------------------------------------------------------------------------
// GetNextCommand
//--------------------------------------------------------------------------------------------------
void GetNextCommand(void)
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
// ProcessHelp
//--------------------------------------------------------------------------------------------------
int8_t ProcessHelp(void)
{
    USBSerial_SendMessage((unsigned char *)"Interface Help");
    USBSerial_SendMessage((unsigned char *)"\n");
    USBSerial_SendMessage((unsigned char *)FWVersion);
    USBSerial_SendMessage((unsigned char *)"\n");
    USBSerial_SendMessage((unsigned char *)BuildDate);
    USBSerial_SendMessage((unsigned char *)"\n");
    USBSerial_SendMessage((unsigned char *)BuildTime);

    return 0;
}


//--------------------------------------------------------------------------------------------------
// ProcessSetSerial
//--------------------------------------------------------------------------------------------------
int8_t ProcessSetSerial(char *Buffer)
{
    SerialNumber = strtol((const char *)Buffer, NULL, 0);
    EEPROMProgram((uint32_t *)&SerialNumber, EEPROMSize - sizeof(SerialNumber), sizeof(SerialNumber));
    return 0;
}


//--------------------------------------------------------------------------------------------------
// ProcessReset
//--------------------------------------------------------------------------------------------------
int8_t ProcessReset(void)
{
    EEPROMMassErase();
    USBSerial_SendMessage((unsigned char *)"EEPROM reset. Power cycle to continue");

    return 0;
}


//--------------------------------------------------------------------------------------------------
// ProcessSetDisplayFormat
//--------------------------------------------------------------------------------------------------
int8_t ProcessSetDisplayFormat(uint32_t Format)
{
    DeviceInfo.NLFormat = Format;
    EEPROMProgram((uint32_t *)&DeviceInfo, 0, sizeof(DeviceInfo));

    return 0;
}

//--------------------------------------------------------------------------------------------------
// ComProc_ProcessCommand
//--------------------------------------------------------------------------------------------------
void ComProc_ProcessCommand(void)
{
    int8_t Result;

    if (CommandCount != 0)
    {
        USBSerial_GetNextCommand();
        //Signal that message should go to USB and not LCD
        SendUSB = 1;
        if (CommandMatch(CommandBuffer, "HELP"))
        {
            Result = ProcessHelp();
        }
        else if (CommandMatch(CommandBuffer, "ENTERDFU"))
        {
            USBSerial_SendMessage((unsigned char *)"\nTIVA in DFU mode\n");
            USBSerial_SendMessage((unsigned char *)"\nUse LM Flash to download new code\n");
            TIVA_DFU();
        }
        else if (CommandMatch(CommandBuffer, "RED"))
        {
            MAP_GPIOPinWrite(LED_BASE, RED_LED|BLUE_LED|GREEN_LED, RED_LED);
        }
        else if (CommandMatch(CommandBuffer, "GREEN"))
        {
            MAP_GPIOPinWrite(LED_BASE, RED_LED|BLUE_LED|GREEN_LED, GREEN_LED);
        }
        else if (CommandMatch(CommandBuffer, "BLUE"))
        {
            MAP_GPIOPinWrite(LED_BASE, RED_LED|BLUE_LED|GREEN_LED, BLUE_LED);
        }
        else if (SubCommandMatch(CommandBuffer, "SERIALNUMBER "))
        {
            Result = ProcessSetSerial((char *)&CommandBuffer[sizeof("SERIALNUMBER")]);
        }
        else if (CommandMatch(CommandBuffer, "SETUNIX"))
        {
            Result = ProcessSetDisplayFormat(UNIX_FORMAT);
        }
        else if (CommandMatch(CommandBuffer, "SETDOS"))
        {
            Result = ProcessSetDisplayFormat(DOS_FORMAT);
        }
        else if (CommandMatch(CommandBuffer, "SETOLD"))
        {
            Result = ProcessSetDisplayFormat(PRE_OSX_FORMAT);
        }
        else if (CommandMatch(CommandBuffer, "ECHOON"))
        {
            DeviceInfo.EchoEnable = 1;
            Result = EEPROM_StoreSettings();
        }
        else if (CommandMatch(CommandBuffer, "ECHOOFF"))
        {
            DeviceInfo.EchoEnable = 0;
            Result = EEPROMProgram((uint32_t *)&DeviceInfo, 0, sizeof(DeviceInfo));
        }
        else if (CommandBuffer[0] != 0)
        {
            USBSerial_SendMessage((unsigned char *)"\nUnknown command\n");
            Result = -1;
        }

        if (Result == 0)
        {
            USBSerial_SendMessage((unsigned char *)"\nOK\n");
        }
        else
        {
            USBSerial_SendMessage((unsigned char *)"\nNOK\n");
        }

        SendUSB = 0;
    }
}
