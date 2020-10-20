#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/eeprom.h"
#include "TIVAMonitor.h"
#include "Helpers.h"
#include "EEPROM.h"

uint32_t EEPROMSize;
uint32_t EEPROMGood = 0;

//--------------------------------------------------------------------------------------------------
// EEPROM_Initialize
//--------------------------------------------------------------------------------------------------
void EEPROM_Initialize(void)
{
    uint32_t ErrorMessage;
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
    // Check if EEPROM Does not have a error code.
    EEPROMGood = 1;
    ErrorMessage = EEPROMInit();
    if (ErrorMessage != EEPROM_INIT_OK)
    {
        EEPROMGood = 0;
        while(1);
    }
    EEPROMSize = EEPROMSizeGet();
}


//--------------------------------------------------------------------------------------------------
// EEPROM_GetSettings
//--------------------------------------------------------------------------------------------------
int8_t EEPROM_GetSettings(void)
{
    uint32_t Sum = 0;
    uint8_t i;
    uint8_t *DeviceInfoPtr;

    DeviceInfoPtr = (uint8_t *)&DeviceInfo;

    EEPROMRead((uint32_t *)&DeviceInfo, 0, sizeof(DeviceInfo));

    for (i = 0; i < (sizeof(DeviceInfo) - 4); i++)
    {
        Sum += *DeviceInfoPtr++;
    }

    if (Sum == DeviceInfo.GoodSig)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


//--------------------------------------------------------------------------------------------------
// EEPROM_StoreSettings
//--------------------------------------------------------------------------------------------------
int8_t EEPROM_StoreSettings(void)
{
    uint32_t Sum = 0;
    uint8_t i;
    uint8_t *DeviceInfoPtr;

    DeviceInfoPtr = (uint8_t *)&DeviceInfo;

    for (i = 0; i < (sizeof(DeviceInfo) - 4); i++)
    {
        Sum += *DeviceInfoPtr++;
    }
    DeviceInfo.GoodSig = Sum;
    return EEPROMProgram((uint32_t *)&DeviceInfo, 0, sizeof(DeviceInfo));
}


