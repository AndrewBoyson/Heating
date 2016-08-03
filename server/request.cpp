#include     "mbed.h"
#include     "http.h"
#include  "heating.h"
#include "schedule.h"
#include      "log.h"
#include      "esp.h"
#include  "request.h"
#include "response.h"
#include   "server.h"

#define RECV_BUFFER_SIZE 512
static char recvbuffer[RECV_BUFFER_SIZE];
static char* pE = recvbuffer + RECV_BUFFER_SIZE - 1; //Guarantee have room for null at end of string

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
        int overrideonoff = false;
        int     autoonoff = false;
        int       checked = false;
        while (pQuery)
        {
            char* pName;
            char* pValue;
            pQuery = splitQuery(pQuery, &pName, &pValue);
            if (strcmp(pName, "override" ) == 0) overrideonoff = true;
            if (strcmp(pName, "auto"     ) == 0)     autoonoff = true;
            if (strcmp(pName, "on"       ) == 0)       checked = true;
            if (strcmp(pName, "schedule1") == 0) ScheduleSave(0, pValue);
            if (strcmp(pName, "schedule2") == 0) ScheduleSave(1, pValue);
            if (strcmp(pName, "schedule3") == 0) ScheduleSave(2, pValue);
            int schedule = *pValue - '0';
            if (schedule < 1) schedule = 1;
            if (schedule > 3) schedule = 3;
            schedule--;
            if (strcmp(pName, "mon") == 0) ScheduleSetMon(schedule);
            if (strcmp(pName, "tue") == 0) ScheduleSetTue(schedule);
            if (strcmp(pName, "wed") == 0) ScheduleSetWed(schedule);
            if (strcmp(pName, "thu") == 0) ScheduleSetThu(schedule);
            if (strcmp(pName, "fri") == 0) ScheduleSetFri(schedule);
            if (strcmp(pName, "sat") == 0) ScheduleSetSat(schedule);
            if (strcmp(pName, "sun") == 0) ScheduleSetSun(schedule);
        }
        if (overrideonoff) ScheduleOverride = checked;
        if (    autoonoff) ScheduleSetAuto(checked);
        ResponseStart(id, REQUEST_LED, NULL);
        return 0;
    }
    
    if (strcmp(pPath, "/log") == 0)
    {
        ResponseStart(id, REQUEST_LOG, NULL);
        return 0;
    }
    if (strcmp(pPath, "/favicon.ico") == 0)
    {
        ResponseStart(id, REQUEST_ICO, pLastModified);
        return 0;
    }
    if (strcmp(pPath, "/styles.css") == 0)
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