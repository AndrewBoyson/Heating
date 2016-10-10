#include          "mbed.h"
#include           "cfg.h"
#include           "rtc.h"
#include           "log.h"
#include            "at.h"
#include           "ntp.h"
#include           "esp.h"
#include            "io.h"
#include          "time.h"
#include        "server.h"
#include          "wifi.h"
#include          "uart.h"
#include        "1-wire.h"
#include "1-wire-device.h"
#include       "heating.h"
#include      "watchdog.h"
#include      "settings.h"

int MainScanUs = 0;
int MainLastProgramPosition;
static volatile int position = 0;

void MainSaveProgramPositionAndReset()
{
    SettingsSetProgramPosition(position);
    NVIC_SystemReset();
}

int main()
{
    //static Timer stopTimer;
    //stopTimer.reset();
    //stopTimer.start();
    MainLastProgramPosition = SettingsGetProgramPosition();
    
    int r = 0;
    
    r = WatchdogInit();
    r =       IoInit();
    r =      RtcInit();
    r =      CfgInit();
    r = SettingsInit();
    r =      LogInit();
    r =     UartInit();
    r =      EspInit();
    r =       AtInit();
    r =      NtpInit();
    r =   ServerInit(); //Call this after any connections (ntp) are reserved
    r =  OneWireInit();
    r =   DeviceInit();
    r =  HeatingInit();
           
    while (1)
    {        
        //Scan each module
        position =  1; r = WatchdogMain(); if (r) break;
        position =  2; r =     WifiMain(); if (r) break;
        position =  3; r =       AtMain(); if (r) break;
        position =  4; r =     UartMain(); if (r) break;
        position =  5; r =      EspMain(); if (r) break;
        position =  6; r =      RtcMain(); if (r) break;
        position =  7; r =      NtpMain(); if (r) break;
        position =  8; r =   ServerMain(); if (r) break;
        position =  9; r =  OneWireMain(); if (r) break;
        position = 10; r =   DeviceMain(); if (r) break;
        position = 11; r =  HeatingMain(); if (r) break;
        position = 12; 
        switch (WifiStatus)
        {
            case WIFI_STOPPED:   Led2 = 0; Led3 = 0; Led4 = 1; break;
            case WIFI_READY:     Led2 = 0; Led3 = 1; Led4 = 0; break;
            case WIFI_CONNECTED: Led2 = 1; Led3 = 0; Led4 = 0; break;
            case WIFI_GOT_IP:    Led2 = 0; Led3 = 0; Led4 = 0; break;
            
        }
        
        //Establish the scan time
        static Timer scanTimer;
        int scanUs = scanTimer.read_us();
        scanTimer.reset();
        scanTimer.start();
        if (scanUs > MainScanUs) MainScanUs++;
        if (scanUs < MainScanUs) MainScanUs--;

        
        //Led1 = AtBusy();
        //if (stopTimer.read() > 20) break;
    }
    
    Led1 = 1; Led2 = 1; Led3 = 1; Led4 = 1;
    
    LogCrLf("Finished");
    LogSave();
    wait(1);
    return EXIT_SUCCESS;
}
