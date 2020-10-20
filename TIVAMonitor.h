#ifndef __TIVAMONITOR_H
#define __TIVAMONITOR_H

#include <stdint.h>
#include "EEPROM.h"

//*****************************************************************************
//
// The system tick rate expressed both as ticks per second and a millisecond
// period.
//
//*****************************************************************************
#define SYSTICKS_PER_SECOND 1000
#define SYSTICK_PERIOD_MS (1000 / SYSTICKS_PER_SECOND)

#define LED_N GPIO_PIN_7
#define LED_S GPIO_PIN_6
#define LED_E GPIO_PIN_3
#define LED_W GPIO_PIN_1
#define LED_C GPIO_PIN_6

#define SW_N GPIO_PIN_4
#define SW_S GPIO_PIN_7
#define SW_E GPIO_PIN_6
#define SW_W GPIO_PIN_4
#define SW_C GPIO_PIN_5

#define SETHIGH        0xff
#define SETLOW         0x00

#define VALID_KEY 0x2869ea83
#define INVALID_KEY 0xffffffff

#define LED_CYCLE_DELAY 30

#define FPGA_I2C_ADDRESS 0x39
#define FPGA_I2C_CHANNEL 2
/*
 #define LITE_ADCCHANNELTHERMAL   0
 #define LITE_ADCCHANNELVBUS1     1
 #define LITE_ADCCHANNELVBUS2     2
 #define LITE_ADCCHANNELPP_HV1    3
 #define LITE_ADCCHANNELPP_HV2    4
 #define LITE_ADCCHANNELVI_VBUS1  5
 #define LITE_ADCCHANNELVI_VBUS2  6
 #define LITE_ADCCHANNELLDO_3V3   7
 #define LITE_ADCCHANNELBC1P2     8
 #define LITE_ADCCHANNELADCIN1    9
 #define LITE_ADCCHANNELADCIN2    10
 #define LITE_ADCCHANNELGPIO2     11
 #define LITE_ADCCHANNELGPIO7     12
 #define LITE_ADCCHANNELGPIO16    13
 #define LITE_ADCCHANNELGPIO17    14
 #define LITE_ADCCHANNELC1_CC1    15
 #define LITE_ADCCHANNELC1_CC2    16
 #define LITE_ADCCHANNELC2_CC1    17
 #define LITE_ADCCHANNELC2_CC2    18
 #define LITE_ADCCHANNELPP1       19
 #define LITE_ADCCHANNELPP2       20
 */
/*
#define ADC_CHANNEL_ADCIN1     0
#define ADC_CHANNEL_ADCIN2     1
#define ADC_CHANNEL_LDO_3V3    2
#define ADC_CHANNEL_PA_VBUS    3
#define ADC_CHANNEL_PA_CC1     4
#define ADC_CHANNEL_I_PA_VBUS  5
#define ADC_CHANNEL_PA_CC2     6
#define ADC_CHANNEL_GPIO4      7
#define ADC_CHANNEL_GPIO5      8
#define ADC_CHANNEL_GPIO0      9
#define ADC_CHANNEL_GPIO2      10
#define ADC_CHANNEL_PP_5V      11
#define ADC_CHANNEL_PA_PP_EXT  12
#define ADCCHANNELBC1P2DP      13
#define ADCCHANNELBC1P2DM      14
*/

//Blackjack chanels
#define ADC_CHANNEL_I_TVSP     0
#define ADC_CHANNEL_VIN        1
#define ADC_CHANNEL_LDO_3V3    2
#define ADC_CHANNEL_PA_VBUS    3
#define ADC_CHANNEL_PB_VBUS    4
#define ADC_CHANNEL_I_PA_VBUS  5
#define ADC_CHANNEL_PB_BC1P2DP 6
#define ADC_CHANNEL_PB_BC1P2DM 7
#define ADC_CHANNEL_NTC        8
#define ADC_CHANNEL_PA_BC1P2DP 9
#define ADC_CHANNEL_PA_BC1P2DM 10
#define ADC_CHANNEL_LDO_5V     11
#define ADC_CHANNEL_PA_CC1_1x  12
#define ADC_CHANNEL_PA_CC2_1x  13
#define ADC_CHANNEL_PB_CC1_1x  14
#define ADC_CHANNEL_PB_CC2_1x  15
#define ADC_CHANNEL_VVAR1      16
#define ADC_CHANNEL_VVAR2      17
#define ADC_CHANNEL_VVAR3      18
#define ADC_CHANNEL_PA_CC1_3x  19
#define ADC_CHANNEL_PA_CC2_3x  20
#define ADC_CHANNEL_PB_CC1_3x  21
#define ADC_CHANNEL_PB_CC2_3x  22


#define GANDALF_EMULATOR    1

extern uint8_t PlatformHardware;
extern uint8_t DefaultsSet;
extern const unsigned char FWVersion[];
extern const unsigned char BuildDate[];
extern const unsigned char BuildTime[];
extern uint8_t DeviceMajor;
extern uint8_t DeviceMinor;
extern uint8_t WrapperMajor;
extern uint8_t WrapperMinor;
extern uint16_t WrapperMajorMinor;
extern uint8_t SidebandMajor;
extern uint8_t SidebandMinor;

extern uint32_t SerialNumber;
extern DeviceInfoType DeviceInfo;

extern unsigned char MiscBuffer[];
extern unsigned char SmallBuffer[];
extern const char *ADCNameList[];
//extern const float RawToVoltageFactor[];
extern const float RawToVoltageFactorSource[];
extern const uint8_t ADCCaptureMap[];

extern uint8_t PP_EXT_Enables[];

#endif // ifndef __TIVAMONITOR_H
