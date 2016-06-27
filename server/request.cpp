#include    "mbed.h"
#include    "html.h"
#include      "io.h"
#include "heating.h"
#include     "log.h"
#include     "esp.h"

#define RECV_BUFFER_SIZE 128
static char recvbuffer[RECV_BUFFER_SIZE];

static int splitRequest(char** ppMethod, char** ppPath, char** ppQuery) //returns 1 if malformed request; 0 if ok
{    
    char* p  = recvbuffer;
    char* pE = recvbuffer + RECV_BUFFER_SIZE - 1; //Guarantee have room for null at end of string
    
    *ppMethod   = NULL;
    *ppPath     = NULL;
    *ppQuery    = NULL;

    while (*p == ' ')                   //Move past any leading spaces
    {
        if (p == pE)     return -1;
        if (*p < ' ')    return -1;
        if (*p >= 0x7f)  return -1;
        p++;
    }
    *ppMethod = p;                      //Record the start of the method (GET or POST)
 
    while (*p != ' ')                   //Move past the method
    {
        if (p == pE)     return -1;
        if (*p < ' ')    return -1;
        if (*p >= 0x7f)  return -1;
        p++;
    }
    *p = 0;                             //Terminate the method
    p++;                                //Start at next character

    while (*p == ' ')                   //Move past any spaces
    {
        if (p == pE)     return -1;
        if (*p < ' ')    return -1;
        if (*p >= 0x7f)  return -1;
        p++;
    }
    *ppPath = p;                        //Record the start of the path
    
    while (*p != ' ')                   //Move past the path and query
    {
        if (p == pE)     return -1;
        if (*p < ' ')    return -1;
        if (*p >= 0x7f)  return -1;
        if (*p == '?')
        {
            *p = 0;                    //Terminate the path
            *ppQuery = p + 1;          //Record the start of the query
        }
        p++;
    }
    *p = 0;                            //Terminate the path or query

    
    return 0;
}
static int splitQuery(char* p, char** ppName, char** ppValue) //returns 1 if malformed request; 0 if ok
{    
    *ppName    = p;                     //Record the start of the name
    *ppValue   = NULL;

    while (*p != '=')                   //Loop to an '='
    {
        if (*p < ' ')    return -1;
        if (*p >= 0x7f)  return -1;
        p++;
    }
    *p = 0;                             //Terminate the name
    *ppValue = p + 1;                   //Record the start of the value
    return 0;
}
int RequestHandle(int id)
{
    char* pMethod;
    char* pPath;
    char* pQuery;
    int r = splitRequest(&pMethod, &pPath, &pQuery);
    if (r)
    {
        HtmlStart(id, HTML_BAD_REQUEST);
        return 0;
    }

    if (strcmp(pMethod, "GET") != 0)
    {
        HtmlStart(id, HTML_BAD_METHOD);
        return 0;
    }
    
    if (strcmp(pPath, "/") == 0)
    {
        if (pQuery)
        {
            char* pName;
            char* pValue;
            splitQuery(pQuery, &pName, &pValue);
            if (strcmp(pName, "ledonoff") == 0)
            {
                if (strcmp(pValue, "&led=on") == 0) Led1 = 1;
                else                                Led1 = 0;
            }
            if (strcmp(pName, "schedule1") == 0) HeatingScheduleSave(0, pValue);
            if (strcmp(pName, "schedule2") == 0) HeatingScheduleSave(1, pValue);
            if (strcmp(pName, "schedule3") == 0) HeatingScheduleSave(2, pValue);
        }
        HtmlStart(id, HTML_LED);
        return 0;
    }
    
    if (strcmp(pPath, "/log") == 0)
    {
        HtmlStart(id, HTML_LOG);
        return 0;
    }
    
    HtmlStart(id, HTML_NOT_FOUND);
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