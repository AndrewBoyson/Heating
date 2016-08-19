#include "mbed.h"
#include "settings.h"

bool WatchdogFlag;  //Set in WatchdogInit - true if last reset was caused by the watchdog; false if was hardware reset

int WatchdogInit()
{
    WatchdogFlag      = LPC_WDT->WDMOD & 0x4; //WDTOF Watchdog time-out flag. Set when the watchdog timer times out, cleared by software or an external reset.
    LPC_WDT->WDCLKSEL = 0x2;     //Selects the RTC oscillator (rtc_clk) as the Watchdog clock source.
    LPC_WDT->WDMOD    = 0x3;     //Watchdog enable WDEN; reset enable WDRESET; clear WDTOF
    LPC_WDT->WDTC     = 0x2000;  //Counts at 4/32khz therefore 1sec = 0x2000, 16sec = 0x20000.
    LPC_WDT->WDFEED   = 0xAA;    //Start the watchdog
    LPC_WDT->WDFEED   = 0x55;
    return 0;
}
int WatchdogMain()
{
    LPC_WDT->WDFEED = 0xAA;      //Feed the watchdog
    LPC_WDT->WDFEED = 0x55;
    return 0;
}