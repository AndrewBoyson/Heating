#include     "mbed.h"
#include      "cfg.h"
#include      "rtc.hpp"
#include      "log.h"
#include       "at.h"
#include      "rtc.hpp"
#include      "ntp.hpp"
#include      "esp.h"
#include       "io.h"
#include     "time.h"
#include   "server.h"
#include     "wifi.h"
#include     "uart.h"
#include   "1-wire.hpp"
#include   "device.hpp"
#include  "heating.h"
#include "watchdog.h"
#include "settings.h"

int MainScanUs = 0;
int MainLastProgramPosition;

int main()
{
    //static Timer stopTimer;
    //stopTimer.reset();
    //stopTimer.start();
    
    MainLastProgramPosition = SettingsGetProgramPosition();
    
    int r = 0;
    
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
    r = WatchdogInit();
           
    while (1)
    {        
        //Scan each module
        SettingsSetProgramPosition( 0); r =     WifiMain(); if (r) break;
        SettingsSetProgramPosition( 1); r =       AtMain(); if (r) break;
        SettingsSetProgramPosition( 2); r =     UartMain(); if (r) break;
        SettingsSetProgramPosition( 3); r =      EspMain(); if (r) break;
        SettingsSetProgramPosition( 4); r =      RtcMain(); if (r) break;
        SettingsSetProgramPosition( 5); r =      NtpMain(); if (r) break;
        SettingsSetProgramPosition( 6); r =   ServerMain(); if (r) break;
        SettingsSetProgramPosition( 7); r =  OneWireMain(); if (r) break;
        SettingsSetProgramPosition( 8); r =   DeviceMain(); if (r) break;
        SettingsSetProgramPosition( 9); r =  HeatingMain(); if (r) break;
        SettingsSetProgramPosition(10); r = WatchdogMain(); if (r) break;
        
        
        SettingsSetProgramPosition(11); 
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
