#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "Tiva.h"
#include "TivaMonitor.h"
#include "Helpers.h"
#include "driverlib/rom.h"
#include "driverlib/gpio.h"
#include "inc/hw_nvic.h"
#include "driverlib/rom_map.h"
#include "driverlib/rom.h"
#include "driverlib/usb.h"
#include "driverlib/sysctl.h"
#include "inc/hw_types.h"
#include "USBSerial.h"

void TIVA_DFU(void)
{
    MAP_USBDevDisconnect(0);    
    USBSerial_Disconnect();

    //Now set as USB
    MAP_GPIOPinTypeUSBAnalog(MONITOR_USB_P);
    MAP_GPIOPinTypeUSBAnalog(MONITOR_USB_N);


    MAP_IntMasterDisable();
    MAP_SysTickIntDisable();
    MAP_SysTickDisable();

    HWREG(NVIC_DIS0) = 0xffffffff;
    HWREG(NVIC_DIS1) = 0xffffffff;

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
    MAP_SysCtlPeripheralReset(SYSCTL_PERIPH_USB0);
    MAP_SysCtlUSBPLLEnable();

    WaitFormS(1000);

    MAP_IntMasterEnable();
    ROM_UpdateUSB(0);
    
 
    // should never get here
    while (true) {
    }
}
