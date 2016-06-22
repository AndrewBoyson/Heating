#include    "mbed.h"
#include    "html.h"
#include      "io.h"
#include "heating.h"
#include     "log.h"
#include     "esp.h"

#define RECV_BUFFER_SIZE 128
static char recvbuffer[RECV_BUFFER_SIZE];

static int splitRequest(char** ppMethod, char** ppPath, char** ppQuery, int *pLenMethod, int *pLenPath, int *pLenQuery) //returns 1 if malformed request; 0 if ok
{    
    char* p  = recvbuffer;
    char* pE = recvbuffer + RECV_BUFFER_SIZE;
    
    *ppMethod   = NULL;
    *ppPath     = NULL;
    *ppQuery    = NULL;
    *pLenMethod = 0;
    *pLenPath   = 0;
    *pLenQuery  = 0;

    while (*p == ' ')                   //Loop to non space
    {
        if (p == pE)     return -1;
        if (*p < ' ')    return -1;
        if (*p >= 0x7f)  return -1;
        p++;
    }
    *ppMethod = p;                      //Record the start of the method GET or POST
 
    while (*p != ' ')                   //Loop to a space
    {
        if (p == pE)     return -1;
        if (*p < ' ')    return -1;
        if (*p >= 0x7f)  return -1;
        p++;
    }
    *pLenMethod = p - *ppMethod;        //Record the length of the method GET or POST
    

    while (*p == ' ')         //Loop to non space
    {
        if (p == pE)     return -1;
        if (*p < ' ')    return -1;
        if (*p >= 0x7f)  return -1;
        p++;
    }
    *ppPath = p;                        //Record the start of the file part of the url
    
    while (*p != ' ')                   //Loop to space
    {
        if (p == pE)     return -1;
        if (*p < ' ')    return -1;
        if (*p >= 0x7f)  return -1;
        if (*p == '?')  *ppQuery = p + 1;
        p++;
    }

    if (*ppQuery)                       //Calulate length of the file and query parts
    {
        *pLenPath  = *ppQuery - *ppPath - 1;
        *pLenQuery = p - *ppQuery;
    }
    else
    {
        *pLenPath = p - *ppPath;
        *pLenQuery = 0;
    }
    
    return 0;
}
static int splitQuery(char* pQuery, int lenQuery, char** ppName, char** ppValue, int* pLenName, int* pLenValue) //returns 1 if malformed request; 0 if ok
{
    char* p  = pQuery;
    char* pE = pQuery + lenQuery;
    
    *ppName    = NULL;
    *ppValue   = NULL;
    *pLenName  = 0;
    *pLenValue = 0;

    *ppName = p;                        //Record the start of the name

    while (*p != '=')                   //Loop to an '='
    {
        if (p == pE)     return -1;
        if (*p < ' ')    return -1;
        if (*p >= 0x7f)  return -1;
        p++;
    }
    *pLenName = p - *ppName;        //Record the length of the name
    p++;
    *ppValue = p;
    *pLenValue = pE - *ppValue;     //Record the length of the value
    return 0;
}
int compare(char* mem, int len, char* str) //Returns true if same and false if not
{
    if (strlen(str) != len) return false;
    if (memcmp(mem, str, len)) return false;
    return true;
}
int RequestHandle(int id)
{
    char* pMethod;
    char* pPath;
    char* pQuery;
    int lenMethod;
    int lenQuery;
    int lenPath;
    int r = splitRequest(&pMethod, &pPath, &pQuery, &lenMethod, &lenPath, &lenQuery);
    if (r)
    {
        HtmlStart(id, HTML_BAD_REQUEST);
        return 0;
    }

    if (!compare(pMethod, lenMethod, "GET"))
    {
        HtmlStart(id, HTML_BAD_METHOD);
        return 0;
    }
    
    if (compare(pPath, lenPath, "/"))
    {
        if (pQuery)
        {
            char* pName;
            char* pValue;
            int lenName;
            int lenValue;
            splitQuery(pQuery, lenQuery, &pName, &pValue, &lenName, &lenValue);
            if (compare(pName, lenName, "ledonoff"))
            {
                if (compare(pValue, lenValue, "&led=on")) Led1 = 1;
                else                                      Led1 = 0;
            }
            if (compare(pName, lenName, "scheme1"))
            {
                HeatingSetSchemeA(1, lenValue, pValue);
            }
        }
        HtmlStart(id, HTML_LED);
        return 0;
    }
    
    if (compare(pPath, lenPath, "/log"))
    {
        HtmlStart(id, HTML_LOG);
        return 0;
    }
    
    HtmlStart(id, HTML_NOT_FOUND);
    return 0;
}

int RequestInit()
{
    for (int id = 0; id < 4; id++)
    {
        if (!EspIpdReserved[id])
        {
            EspIpdBuffer[id] = recvbuffer;
            EspIpdBufferLen[id] = RECV_BUFFER_SIZE;
        }
    }
    return 0;
}