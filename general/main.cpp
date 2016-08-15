#include     "mbed.h"
#include      "cfg.h"
#include      "rtc.h"
#include      "log.h"
#include       "at.h"
#include      "ntp.h"
#include      "esp.h"
#include       "io.h"
#include     "time.h"
#include   "server.h"
#include     "wifi.h"
#include     "uart.h"
#include   "1-wire.h"
#include  "ds18b20.h"
#include  "heating.h"
#include "watchdog.h"

int MainScanUs = 0;

int main()
{
    //static Timer stopTimer;
    //stopTimer.reset();
    //stopTimer.start();
    
    int r = 0;
    
    r =       IoInit();
    r =      RtcInit();
    r =      CfgInit();
    r =      LogInit();
    r =     UartInit();
    r =      EspInit();
    r =       AtInit();
    r =      NtpInit();
    r =   ServerInit(); //Call this after any connections (ntp) are reserved
    r =  OneWireInit();
    r =  DS18B20Init();
    r =  HeatingInit();
    r = WatchdogInit();
           
    while (1)
    {        
        //Scan each module
        r =     WifiMain(); if (r) break;
        r =       AtMain(); if (r) break;
        r =     UartMain(); if (r) break;
        r =      EspMain(); if (r) break;
        r =      NtpMain(); if (r) break;
        r =   ServerMain(); if (r) break;
        r =  OneWireMain(); if (r) break;
        r =  DS18B20Main(); if (r) break;
        r =  HeatingMain(); if (r) break;
        r = WatchdogMain(); if (r) break;
        
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
