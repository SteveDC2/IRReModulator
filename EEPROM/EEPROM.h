#ifndef __EEPROM_H
#define __EEPROM_H

#include <stdint.h>
typedef struct//This structure must be multiples of 4 bytes for correct EEPROM read/write
{
    unsigned char ConfigurationName[16];
    int32_t ValidKey;
} ConfigurationType;

typedef struct
{
    //Align on 4 byte boundary
    int8_t NLFormat;
    int8_t EchoEnable;
    uint8_t IRAddress;
    uint8_t Pad1;

    uint8_t IRInOutMap[8];
    uint16_t IRKeyCodes[9];
    uint8_t Pad2;
    uint8_t Pad3;

    //Electronic serial number
    uint32_t SerialNumber;

    //Flag to indicate the information is good and valid. SimpleCRC summation of all data
    uint32_t GoodSig;
} DeviceInfoType;

extern uint32_t EEPROMSize;
extern uint32_t EEPROMGood;

void EEPROM_Initialize(void);
int8_t EEPROM_StoreSettings(void);
int8_t EEPROM_GetSettings(void);

#endif // ifndef __EEPROM_H
