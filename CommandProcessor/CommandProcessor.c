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
    USBSerial_SendMessage((unsigned char *)"IR Matric Control Help");
    USBSerial_SendMessage((unsigned char *)"\n");
    USBSerial_SendMessage((unsigned char *)FWVersion);
    USBSerial_SendMessage((unsigned char *)"\n");
    USBSerial_SendMessage((unsigned char *)BuildDate);
    USBSerial_SendMessage((unsigned char *)"\n");
    USBSerial_SendMessage((unsigned char *)BuildTime);
    USBSerial_SendMessage((unsigned char *)"\n\nIR Input 1 2 3 4 [5 6 7 8]\n");
    sprintf((char *)MiscBuffer, "Output   %d %d %d %d  %d %d %d %d", DeviceInfo.IRInOutMap[0], DeviceInfo.IRInOutMap[1], DeviceInfo.IRInOutMap[2], DeviceInfo.IRInOutMap[3],
                                                                   DeviceInfo.IRInOutMap[4], DeviceInfo.IRInOutMap[5], DeviceInfo.IRInOutMap[6], DeviceInfo.IRInOutMap[7]);
    USBSerial_SendMessage((unsigned char *)MiscBuffer);
    USBSerial_SendMessage((unsigned char *)"\n\n");
    USBSerial_SendMessage((unsigned char *)"map abcdefgh where a thru h = 1-8\n");



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

uint8_t ProcessSetMap(char *Buffer)
{
    uint8_t Count = 0;
    uint8_t Char;
    uint8_t Map[8] = {1, 2, 3, 7, 0, 0, 0, 0};
    uint8_t Error = 0;

    do
    {
        Char = Buffer[Count];
        if ((Char >= '1') && (Char <= '8'))
        {
            Map[Count] = Char - '1' + 1;
        }
        else if (Char != 0)
        {
            Error = 1;
        }
        Count++;

    }while((Count < 8) && (Char != 0));

    if (Error == 0)
    {
        memcpy(&DeviceInfo.IRInOutMap, &Map, sizeof(DeviceInfo.IRInOutMap));
        EEPROM_StoreSettings();
    }

    return Error;
}

uint16_t WaitForAnyNewKeyPressed()
{
    uint8_t i;

    while(IRNewData == 0);

    for (i = 0; i < 4; i ++)
    {
        if ((IRNewData & (1 << i)) != 0)
        {
            IRNewData = 0;
            return IRReceived[i];
        }
    }
    //Should never get here but return just in case
    return 0;
}

uint8_t ProcessLearnCommand()
{
    uint8_t Index;
    uint16_t NewAddress;
    uint16_t NewCodes[9];

    LearnMode = true;

    USBSerial_SendMessage((unsigned char *)"\nRemote learn mode\n");
    USBSerial_SendMessage((unsigned char *)"\nPress any key to learn the remote address\n");
    NewAddress = WaitForAnyNewKeyPressed() >> 8;
    for (Index = 1; Index <= 8; Index++)
    {
        sprintf((char*)MiscBuffer, "Press the key for %d\n",Index);
        USBSerial_SendMessage((unsigned char *)MiscBuffer);
        NewCodes[Index] = ((WaitForAnyNewKeyPressed() & 0xff) << 8 ) | Index;
    }
    USBSerial_SendMessage((unsigned char *)"Press the key for IR reset\n");
    NewCodes[0] = ((WaitForAnyNewKeyPressed() & 0xff) << 8 ) | 0xff;

    memcpy(&DeviceInfo.IRKeyCodes, &NewCodes, sizeof(DeviceInfo.IRKeyCodes));
    DeviceInfo.IRAddress = NewAddress;
    EEPROM_StoreSettings();

    LearnMode = false;

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
        else if (SubCommandMatch(CommandBuffer, "MAP "))
        {
            Result = ProcessSetMap((char *)&CommandBuffer[sizeof("MAP")]);
        }
        else if (CommandMatch(CommandBuffer, "LEARN"))
        {
            Result = ProcessLearnCommand();
        }
        else if (CommandMatch(CommandBuffer, "RED"))
        {
            MAP_GPIOPinWrite(LED_BASE, RED_LED|BLUE_LED|GREEN_LED, RED_LED);
            Result = 0;
        }
        else if (CommandMatch(CommandBuffer, "GREEN"))
        {
            MAP_GPIOPinWrite(LED_BASE, RED_LED|BLUE_LED|GREEN_LED, GREEN_LED);
            Result = 0;
        }
        else if (CommandMatch(CommandBuffer, "BLUE"))
        {
            MAP_GPIOPinWrite(LED_BASE, RED_LED|BLUE_LED|GREEN_LED, BLUE_LED);
            Result = 0;
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
