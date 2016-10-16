#include          "mbed.h"
#include          "http.h"
#include       "heating.h"
#include       "program.h"
#include           "log.h"
#include           "esp.h"
#include       "request.h"
#include      "response.h"
#include        "server.h"
#include      "settings.h"
#include           "cfg.h"
#include      "watchdog.h"
#include "1-wire-device.h"
#include           "rtc.h"
#include       "rtc-cal.h"
#include            "io.h"
#include           "ntp.h"

#define RECV_BUFFER_SIZE 512
static char recvbuffer[RECV_BUFFER_SIZE];
static char* pE = recvbuffer + RECV_BUFFER_SIZE - 1; //Guarantee have room to put a NUL at the end of a string

static char* getNextLine(char* p) //Terminates this line and returns the start of the next line or NULL if none
{
    while (true)
    {
        if (p == pE)             //There are no more lines
        {
            *p = 0;              //terminate the line
            return NULL;   
        }
        if (*p == 0) return NULL;//There are no more lines
        if (*p == '\n')
        {
            *p = 0;              //make the line a c string
            return p + 1;        //return the start of the next line
        }
        if (*p < ' ')    *p = 0; //terminate the line at any invalid characters
        if (*p >= 0x7f)  *p = 0; //terminate the line at any invalid characters
        p++;
    }
}

static void splitRequest(char* p, char** ppMethod, char** ppPath, char** ppQuery)
{        
    *ppMethod   = NULL;
    *ppPath     = NULL;
    *ppQuery    = NULL;

    while (*p == ' ')         //Move past any leading spaces
    {
        if (*p == 0) return;
        p++;
    }
    *ppMethod = p;            //Record the start of the method (GET or POST)
 
    while (*p != ' ')         //Move past the method
    {
        if (*p == 0) return;
        p++;
    } 
    *p = 0;                   //Terminate the method
    p++;                      //Start at next character

    while (*p == ' ')         //Move past any spaces
    {
        if (*p == 0) return;
        p++;
    } 
    *ppPath = p;              //Record the start of the path
    
    while (*p != ' ')         //Move past the path and query
    {
        if (*p == 0) return;
        if (*p == '?')
        {
            *p = 0;           //Terminate the path
            *ppQuery = p + 1; //Record the start of the query
        }
        p++;
    }
    *p = 0;                   //Terminate the path or query
}
static char* splitQuery(char* p, char** ppName, char** ppValue) //returns the start of the next name value pair
{    
    *ppName    = p;                     //Record the start of the name
    *ppValue   = NULL;

    while (*p != '=')                   //Loop to an '='
    {
        if (*p == 0)    return 0;
        p++;
    }
    *p = 0;                             //Terminate the name by replacing the '=' with a NUL char
    p++;                                //Move on to the start of the value
    *ppValue = p;                       //Record the start of the value
    while (*p != '&')                   //Loop to a '&'
    {
        if (*p == 0)    return 0;
        p++;
    }
    *p = 0;                            //Terminate the value by replacing the '&' with a NULL
    return p + 1;
}
static void splitHeader(char* p, char** ppName, char** ppValue)
{
    *ppName    = p;                     //Record the start of the name
    *ppValue   = NULL;

    while (*p != ':')                   //Loop to an ':'
    {
        if (!*p) return;
        p++;
    }
    *p = 0;                             //Terminate the name by replacing the ':' with a NUL char
    p++;
    while (*p == ' ')                   //Move past any spaces
    {
        if (*p == 0) return;
        p++;
    }
    *ppValue = p;                       //Record the start of the value
} 
int RequestHandle(int id)
{    
    char* pThis;
    char* pNext;
    char* pMethod;
    char* pPath;
    char* pQuery;
    char* pLastModified = NULL;

    pThis = recvbuffer;    
    pNext = getNextLine(pThis);
    splitRequest(pThis, &pMethod, &pPath, &pQuery);
    
    while(pNext)
    {
        pThis = pNext;
        pNext = getNextLine(pThis);
        if (*pThis == 0) break;     //This line is empty ie no more headers
        char* pName;
        char* pValue;
        splitHeader(pThis, &pName, &pValue);
        if (strcmp(pName, "If-Modified-Since") == 0) pLastModified = pValue;
    }

    if (strcmp(pMethod, "GET") != 0)
    {
        ResponseStart(id, REQUEST_BAD_METHOD, "");
        return 0;
    }
    
    if (strcmp(pPath, "/") == 0)
    {
        while (pQuery)
        {
            char* pName;
            char* pValue;
            pQuery = splitQuery(pQuery, &pName, &pValue);
                        
            if (strcmp(pName, "autoon" ) == 0) ProgramAuto = true;
            if (strcmp(pName, "autooff") == 0) ProgramAuto = false;
            
            if (strcmp(pName, "overrideon" ) == 0) ProgramOverride = true;
            if (strcmp(pName, "overrideoff") == 0) ProgramOverride = false;           
            
        }
        ResponseStart(id, REQUEST_HOME, NULL);
        return 0;
    }
    
    if (strcmp(pPath, "/program") == 0)
    {
        bool   needToSave = !!pQuery;
        while (pQuery)
        {
            char* pName;
            char* pValue;
            pQuery = splitQuery(pQuery, &pName, &pValue);
            
            int value = 0;           
            sscanf(pValue, "%d", &value);
                        
            if (strcmp(pName, "program1") == 0) ProgramParse(0, pValue);
            if (strcmp(pName, "program2") == 0) ProgramParse(1, pValue);
            if (strcmp(pName, "program3") == 0) ProgramParse(2, pValue);
            
            int program = value;
            if (program < 1) program = 1;
            if (program > 3) program = 3;
            program--;
            if (strcmp(pName, "mon") == 0) ProgramDay[1] = program;
            if (strcmp(pName, "tue") == 0) ProgramDay[2] = program;
            if (strcmp(pName, "wed") == 0) ProgramDay[3] = program;
            if (strcmp(pName, "thu") == 0) ProgramDay[4] = program;
            if (strcmp(pName, "fri") == 0) ProgramDay[5] = program;
            if (strcmp(pName, "sat") == 0) ProgramDay[6] = program;
            if (strcmp(pName, "sun") == 0) ProgramDay[0] = program;
            
        }
        if (needToSave) ProgramSaveAll();
        ResponseStart(id, REQUEST_PROGRAM, NULL);
        return 0;
    }
    if (strcmp(pPath, "/heating") == 0)
    {
        while (pQuery)
        {
            char* pName;
            char* pValue;
            pQuery = splitQuery(pQuery, &pName, &pValue);
            
            int value = 0;           
            sscanf(pValue, "%d", &value);      
            
            if (strcmp(pName, "nighttemp") == 0) SettingsSetNightTemperature(value);
            if (strcmp(pName, "frosttemp") == 0) SettingsSetFrostTemperature(value);
        }
        ResponseStart(id, REQUEST_HEATING, NULL);
        return 0;
    }
    if (strcmp(pPath, "/boiler") == 0)
    {
        while (pQuery)
        {
            char* pName;
            char* pValue;
            pQuery = splitQuery(pQuery, &pName, &pValue);

            int value = 0;
            sscanf(pValue, "%d", &value);
                        
            if (strcmp(pName, "tanksetpoint"  ) == 0) SettingsSetTankSetPoint       (value);
            if (strcmp(pName, "tankhysteresis") == 0) SettingsSetTankHysteresis     (value);
            if (strcmp(pName, "boilerresidual") == 0) SettingsSetBoilerRunOnResidual(value);
            if (strcmp(pName, "boilerrunon"   ) == 0) SettingsSetBoilerRunOnTime    (value);
        }
        ResponseStart(id, REQUEST_BOILER, NULL);
        return 0;
    }
    if (strcmp(pPath, "/system") == 0)
    {
        while (pQuery)
        {
            char* pName;
            char* pValue;
            pQuery = splitQuery(pQuery, &pName, &pValue);

            int value = 0;
            sscanf(pValue, "%d", &value);
                        
            if (strcmp(pName, "watchdogflagoff") == 0) WatchdogFlag = false;
            if (strcmp(pName, "tankrom")         == 0)
            {
                char rom[8];
                DeviceParseAddress(pValue, rom);
                SettingsSetTankRom(rom);
            }
            if (strcmp(pName, "boileroutputrom") == 0)
            {
                char rom[8];
                DeviceParseAddress(pValue, rom);
                SettingsSetBoilerOutputRom(rom);
            }
            if (strcmp(pName, "boilerreturnrom") == 0)
            {
                char rom[8];
                DeviceParseAddress(pValue, rom);
                SettingsSetBoilerReturnRom(rom);
            }
            if (strcmp(pName, "hallrom") == 0)
            {
                char rom[8];
                DeviceParseAddress(pValue, rom);
                SettingsSetHallRom(rom);
            }
            if (strcmp(pName, "ntpip") == 0)
            {
                SettingsSetClockNtpIp(pValue);
                NtpRequestReconnect();
            }
            if (strcmp(pName, "clockinitial" ) == 0) SettingsSetClockInitialInterval(value       );
            if (strcmp(pName, "clocknormal"  ) == 0) SettingsSetClockNormalInterval (value * 3600);
            if (strcmp(pName, "clockretry"   ) == 0) SettingsSetClockRetryInterval  (value       );
            if (strcmp(pName, "clockoffset"  ) == 0) SettingsSetClockOffsetMs       (value       );
            if (strcmp(pName, "clockmaxdelay") == 0) SettingsSetClockNtpMaxDelayMs  (value       );
            if (strcmp(pName, "clockcaldiv"  ) == 0) SettingsSetClockCalDivisor     (value       );
            if (strcmp(pName, "calibration"  ) == 0) RtcCalSet                      (value       );

        }
        ResponseStart(id, REQUEST_SYSTEM, NULL);
        return 0;
    }
    if (strcmp(pPath, "/log") == 0)
    {
        ResponseStart(id, REQUEST_LOG, NULL);
        return 0;
    }
    if (strcmp(pPath, "/ajax") == 0)
    {
        ResponseStart(id, REQUEST_AJAX, NULL);   
        return 0;    
    }
    if (strcmp(pPath, "/favicon.ico") == 0)
    {
        ResponseStart(id, REQUEST_ICO, pLastModified);
        return 0;
    }
    if (strcmp(pPath, "/heating.css") == 0)
    {
        ResponseStart(id, REQUEST_CSS, pLastModified);
        return 0;
    }
    
    ResponseStart(id, REQUEST_NOT_FOUND, NULL);
    return 0;
}

int RequestInit()
{
    for (int id = 0; id < ESP_ID_COUNT; id++)
    {
        if (!EspIpdReserved[id])
        {
            EspIpdBuffer[id] = recvbuffer;
            EspIpdBufferLen[id] = RECV_BUFFER_SIZE;
        }
    }
    return 0;
}