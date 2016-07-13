#include "mbed.h"
#include "uart.h"
#include  "esp.h"
#include   "at.h"
#include "wifi.h"
#include  "log.h"
#include   "io.h"
#include  "cfg.h"

#define IP_WAIT_TIME_MS 5000

int WifiStatus = WIFI_STOPPED;
enum {
    AM_STOP,
    AM_START,
    AM_WAIT_CONNECTED,
    AM_WAIT_IP,
    AM_AUTOBAUD,
    AM_CHANGE_BAUD,
    AM_SET_MODE,
    AM_CONNECT,
    AM_GET_STATUS,
    AM_WAIT_WIFI,
    AM_FINISH,
    AM_MUX,
    AM_VERSION,
    AM_STARTED
};
static int am = AM_STOP;
int WifiStarted() { return am == AM_STARTED; }

int WifiMain()
{
    if (AtBusy()) return 0;
    
    static int result = AT_NONE;
    static int autoBaudMultiplier = 1;
    static int autoBaudOdd = false;
    static int autoBaudRate = 0;
    
    static Timer wifiWaitTimer;
    
    switch (am)
    {
        case AM_STOP:
            AtResetAndStop();
            WifiStatus = WIFI_STOPPED;
            am = AM_START;
            result = AT_NONE;
            break;
        case AM_START:
            switch (result)
            {
                case AT_NONE:
                    AtReleaseResetAndStart(&result);
                    wifiWaitTimer.reset();
                    wifiWaitTimer.start();
                    break;
                case AT_SUCCESS:
                    WifiStatus = WIFI_READY;
                    am = AM_WAIT_CONNECTED;
                    result = AT_NONE;
                    break;
                default:
                    am = AM_AUTOBAUD;
                    result = AT_NONE;
                    autoBaudMultiplier = 1;
                    autoBaudOdd = false;
                    break;
            }
            break;
        case AM_WAIT_CONNECTED:
            switch (result)
            {
                case AT_NONE:
                    AtWaitForWifiConnected(&result);
                    break;
                case AT_SUCCESS:
                    WifiStatus = WIFI_CONNECTED;
                    am = AM_WAIT_IP;
                    result = AT_NONE;
                    break;
                default:
                    am = AM_CONNECT;
                    result = AT_NONE;
                    break;
            }
            break;
        case AM_WAIT_IP:
            switch (result)
            {
                case AT_NONE:
                    AtWaitForWifiGotIp(&result);
                    break;
                case AT_SUCCESS:
                    WifiStatus = WIFI_GOT_IP;
                    am = AM_FINISH;
                    result = AT_NONE;
                    break;
                default:
                    LogCrLf("Wifi connected but no IP obtained");
                    return -1;
            }
            break;
        case AM_AUTOBAUD:
            switch (result)
            {
                case AT_NONE:
                    autoBaudRate = 9600 * autoBaudMultiplier;
                    if (autoBaudOdd) autoBaudRate += autoBaudRate >> 1; //Multiply by 1.5
                    UartBaud(autoBaudRate);
                    AtAt(&result);
                    break;
                case AT_SUCCESS:
                    LogF("Connected successfully at a baud rate of %d\r\n", autoBaudRate);
                    am = AM_CHANGE_BAUD;
                    result = AT_NONE;
                    break;
                default:
                    if (autoBaudOdd) autoBaudMultiplier <<= 1;
                    autoBaudOdd = !autoBaudOdd;
                    if (autoBaudMultiplier > 64)
                    {
                        LogCrLf("Could not find a baud rate to connect to ESP");
                        return -1;
                    }
                    result = AT_NONE;
                    break;
            }
            break;
        case AM_CHANGE_BAUD:
            switch (result)
            {
                case AT_NONE:
                    LogF("Changing baud rate to %d\r\n", CfgBaud);
                    AtBaud(CfgBaud, &result);
                    break;
                case AT_SUCCESS:
                    UartBaud(CfgBaud);
                    am = AM_GET_STATUS;
                    result = AT_NONE;
                    break;
                default:
                    LogCrLf("Could not change baud");
                    return -1;
            }
            break;
        
        case AM_CONNECT:
            switch (result)
            {
                case AT_NONE:
                    AtConnect(CfgSsid, CfgPassword, &result);
                    break;
                case AT_SUCCESS:
                    wifiWaitTimer.reset();
                    wifiWaitTimer.start();
                    am = AM_GET_STATUS;
                    result = AT_NONE;
                    break;
                default:
                    LogCrLf("Could not connect to WiFi");
                    return -1;
            }
            break;
        case AM_GET_STATUS:
            if (wifiWaitTimer.read_ms() < IP_WAIT_TIME_MS) break; //Do nothing until enough time has passed
            wifiWaitTimer.stop();
            switch (result)
            {
                case AT_NONE:
                    AtGetEspStatus(&result);
                    break;
                case AT_SUCCESS:
                    switch(AtEspStatus)
                    {
                        case 2:
                            WifiStatus = WIFI_GOT_IP;
                            am = AM_FINISH;
                            break;
                        case 3:
                            WifiStatus = WIFI_CONNECTED;
                            LogCrLf("Could connect but not get an IP address");
                            return -1;
                        case 4:
                            WifiStatus = WIFI_READY;
                            am = AM_CONNECT;
                            break;
                        default:
                            LogF("Unknown CIPSTATUS --> STATUS:%d", AtEspStatus);
                            return -1;
                    }
                    result = AT_NONE;
                    break;
                default:
                    LogCrLf("Could not connect to WiFi");
                    return -1;
            }
            break;
        
        case AM_FINISH:
            am = AM_SET_MODE;
            result = AT_NONE;
            break;
        case AM_SET_MODE:
            switch (result)
            {
                case AT_NONE:
                    AtSetMode(1, &result);
                    break;
                case AT_SUCCESS:
                    am = AM_MUX;
                    result = AT_NONE;
                    break;
                default:
                    LogCrLf("Could not set mode");
                    return -1;
            }
            break;
        case AM_MUX:
            switch (result)
            {
                case AT_NONE:
                    AtMux(&result);
                    break;
                case AT_SUCCESS:
                    am = AM_VERSION;
                    result = AT_NONE;
                    break;
                default:
                    LogCrLf("Could not set up multiple ids");
                    return -1;
            }
            break;
        case AM_VERSION:
            switch (result)
            {
                case AT_NONE:
                    AtGetEspVersion(&result);
                    break;
                case AT_SUCCESS:
                    am = AM_STARTED;
                    result = AT_NONE;
                    break;
                default:
                    LogCrLf("Could not set up multiple ids");
                    return -1;
            }
            break;        
        case AM_STARTED:
            wifiWaitTimer.stop();
            return 0;
        default:
            LogF("Unknown \'am\' %d", am);
            return -1;
    }
   
    return 0;
}
