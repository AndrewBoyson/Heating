#include    "main.h"
#include     "esp.h"
#include      "at.h"
#include      "io.h"
#include     "log.h"
#include     "rtc.h"
#include    "wifi.h"
#include "ds18b20.h"
#include     "cfg.h"
#define RECV_BUFFER_SIZE 128
#define SEND_BUFFER_SIZE 256
#define SERVER_PORT 80

static char recvbuffer[RECV_BUFFER_SIZE];
static char sendbuffer[SEND_BUFFER_SIZE];

#define WHAT_NOT_FOUND   0
#define WHAT_BAD_REQUEST 1
#define WHAT_BAD_METHOD  2
#define WHAT_LED         3
#define WHAT_LOG         4
static int whatToSendToId[4];
static int lineToSendToId[4]; //0 == do nothing; -1 == close

int ServerInit(void) //Make sure this is only called after any other ids are reserved.
{
    for (int id = 0; id < 4; id++)
    {
        if (!EspIpdReserved[id])
        {
            EspIpdBuffer[id] = recvbuffer;
            EspIpdBufferLen[id] = RECV_BUFFER_SIZE;
        }
        whatToSendToId[id] = 0;
        lineToSendToId[id] = 0;
    }
    return 0;
}
static int addChunkF(int prevlen, char *fmt, ...)
{
    //Set up variable arguments
    va_list argptr;
    va_start(argptr, fmt);
        
    //Fill the buffer
    int room = SEND_BUFFER_SIZE - prevlen;
    int sent = vsnprintf(sendbuffer + prevlen, room, fmt, argptr);
    if (sent > room) sent = room;
    if (sent < 0) sent = 0;
    
    //Finish with variable arguments
    va_end(argptr);
    
    //Return length
    int len = prevlen + sent;
    return len < SEND_BUFFER_SIZE ? len : SEND_BUFFER_SIZE;
    
}

static int addChunk(int prevlen, char *text)
{
    strncpy(sendbuffer + prevlen, text, SEND_BUFFER_SIZE - prevlen);
    int len = prevlen + strlen(text);
    return len < SEND_BUFFER_SIZE ? len : SEND_BUFFER_SIZE;
}
static int fillChunk(char * text)
{
    strncpy(sendbuffer, text, SEND_BUFFER_SIZE);
    int len = strlen(text);
    return len < SEND_BUFFER_SIZE ? len : SEND_BUFFER_SIZE;
}
static int fillNotFound(int id)
{
    int len = fillChunk("HTTP/1.0 404 Not Found\r\n\r\n");
    lineToSendToId[id] = -1;
    return len;
}
static int fillBadRequest(int id)
{
    int len = fillChunk("HTTP/1.0 400 Bad Request\r\n\r\n");
    lineToSendToId[id] = -1;
    return len;
}
static int fillBadMethod(int id)
{
    int len = fillChunk("HTTP/1.0 405 Method Not Allowed\r\nAllow: GET\r\n\r\n");
    lineToSendToId[id] = -1;
    return len;
}
static int fillLogChunk() //Returns length. If length is less than the buffer size then that was the last chunk to send: that could mean a length of zero.
{
    static int enumerationStarted = false;
    if (!enumerationStarted)
    {
        LogEnumerateStart();
        enumerationStarted = true;
    }
    char* p = sendbuffer;
    while (p < sendbuffer + SEND_BUFFER_SIZE)
    {
        int c = LogEnumerate();
        if (c == EOF)
        {
            enumerationStarted = false;
            break;
        }
        *p++ = c;
    }
    return p - sendbuffer;
}
static char* header = 
            "HTTP/1.0 200 OK\r\n"
            "Content-Type: text/html; charset=ISO-8859-1\r\n"
            "\r\n"
            "<!DOCTYPE html>\r\n"
            "<html>\r\n"
            "<head>\r\n"
            "<title>IoT mbed</title>\r\n"
            "</head>\r\n"
            "<style>\r\n"
            "* { font-family: Tahoma, Geneva, sans-serif; }\r\n"
            "</style>\r\n"
            "<body>\r\n";

static int fillLog(int id)
{
    int len = 0;

    switch (lineToSendToId[id])
    {
        case 1: len = fillChunk(header);                             lineToSendToId[id] += 1; break;
        case 2: len = fillChunk("<code><pre>");                      lineToSendToId[id] += 1; break;
        case 3: len = fillLogChunk();    if (len < SEND_BUFFER_SIZE) lineToSendToId[id] += 1; break; //only increments after the last chunk
        case 4: len = fillChunk(
            "</pre></code>\r\n"
            "</body>\r\n"
            "</html>\r\n");
            lineToSendToId[id] = -1;
            break;
    }
    return len;
}
static int addTemperature(int len, int16_t value)
{
    switch (value)
    {
        case DS18B20_ERROR_CRC:                     return addChunk (len, "CRC error"                     );
        case DS18B20_ERROR_NOT_FOUND:               return addChunk (len, "ROM not found"                 );
        case DS18B20_ERROR_TIMED_OUT:               return addChunk (len, "Timed out"                     );
        case DS18B20_ERROR_NO_DEVICE_PRESENT:       return addChunk (len, "No device detected after reset");
        case DS18B20_ERROR_NO_DEVICE_PARTICIPATING: return addChunk (len, "Device removed during search"  );
        default:                                    return addChunkF(len, "%1.1f", value / 16.0           );
    }
}
static int fillLed(int id)
{
    int len;
    switch (lineToSendToId[id])
    {
        case 1:
            len = fillChunk(header);
            lineToSendToId[id] += 1;
            break;
        case 2:
            len = 0;
            struct tm tm;
            RtcGetTm(&tm);
            len = addChunkF(len, "Time: %d-%02d-%02d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
            switch(tm.tm_wday)
            {
                case  0: len = addChunk(len, "Sun"); break;
                case  1: len = addChunk(len, "Mon"); break;
                case  2: len = addChunk(len, "Tue"); break;
                case  3: len = addChunk(len, "Wed"); break;
                case  4: len = addChunk(len, "Thu"); break;
                case  5: len = addChunk(len, "Fri"); break;
                case  6: len = addChunk(len, "Sat"); break;
                default: len = addChunk(len, "???"); break;
            }
            len = addChunkF(len, " %02d:%02d:%02d UTC", tm.tm_hour, tm.tm_min, tm.tm_sec);
            len = addChunk (len, "<br/><br/>\r\n");
            len = addChunkF(len, "Scan &micro;s: %d<br/><br/>\r\n", MainScanUs);
            lineToSendToId[id] += 1;
            break;
        case 3:
            len = 0;
            len = addChunk (len, "Devices:<br/>\r\n");
            for (int j = 0; j < DS18B20DeviceCount; j++)
            {
                len = addChunkF(len, "%d - ", j);
                for (int i = 0; i < 8; i++) len = addChunkF(len, " %02X", DS18B20DeviceList[j*8 + i]);
                len = addChunk(len, " - ");
                len = addTemperature(len, DS18B20Value[j]);
                len = addChunk (len, "<br/>\r\n");
            }
            len = addChunk (len, "<br/>\r\n");
            lineToSendToId[id] += 1;
            break;
        case 4:
            len = 0;
            int16_t temp;
            
            temp = DS18B20ValueFromRom(CfgTankRom);
            len = addChunk (len, "Tank  temperature: ");
            len = addTemperature(len, temp);
            len = addChunk (len, "<br/>\r\n");
            
            temp = DS18B20ValueFromRom(CfgInletRom);
            len = addChunk (len, "Inlet temperature: ");
            len = addTemperature(len, temp);
            len = addChunk (len, "<br/><br/>\r\n");
            
            lineToSendToId[id] += 1;
            break;
        case 5:
            len = 0;
            len = addChunkF(len, "Battery voltage: %1.2f<br/><br/>\r\n", IoVbat());
            lineToSendToId[id] += 1;
            break;
        case 6:
            len = fillChunk(
            "<br/>"
            "<form action='/' method='get'>\r\n"
            "<input type='hidden' name='ledonoff'>\r\n");
            lineToSendToId[id] += 1;
            break;
        case 7:
            len = fillChunk("Led <input type='checkbox' name='led' value='on'");
            if (Led1) len = addChunk(len, " checked='checked'");
            len = addChunk(len, ">\r\n");
            lineToSendToId[id] +=  1;
            break;
        case 8:
            len = fillChunk(
            "<input type='submit' value='Set'><br/>\r\n"
            "</form>\r\n"
            "</body>\r\n"
            "</html>\r\n");
            lineToSendToId[id] = -1;
            break;
    }
    return len;
}
static int fillSendBuffer(int id)
{
    switch(whatToSendToId[id])
    {
        case WHAT_NOT_FOUND:   return fillNotFound(id);
        case WHAT_BAD_REQUEST: return fillBadRequest(id);
        case WHAT_BAD_METHOD:  return fillBadMethod(id);
        case WHAT_LOG:         return fillLog(id);
        case WHAT_LED:         return fillLed(id);
        default:
            LogF("No such 'whatToSendToId' %s", whatToSendToId[id]);
            return -1;
    }
}
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
int compare(char* mem, int len, char* str) //Returns true if same and false if not
{
    if (strlen(str) != len) return false;
    if (memcmp(mem, str, len)) return false;
    return true;
}
static int recvMain()
{
    //Wait for data to be available oneshot. Buffer will normally hit limit.
    if (!EspDataAvailable) return 0;
    
    //Ignore ids that have been reserved elsewhere (ie NTP)
    if (EspIpdReserved[EspIpdId]) return 0;
    
    //Set up the next line to send to be the first
    lineToSendToId[EspIpdId] = 1;
    
    char* pMethod;
    char* pPath;
    char* pQuery;
    int lenMethod;
    int lenQuery;
    int lenPath;
    int r = splitRequest(&pMethod, &pPath, &pQuery, &lenMethod, &lenPath, &lenQuery);
    if (r)
    {
        whatToSendToId[EspIpdId] = WHAT_BAD_REQUEST;
        return 0;
    }

    if (!compare(pMethod, lenMethod, "GET"))
    {
        whatToSendToId[EspIpdId] = WHAT_BAD_METHOD;
        return 0;
    }
    
    if (compare(pPath, lenPath, "/"))
    {
        if (pQuery)
        {
             if (compare(pQuery, lenQuery, "ledonoff=&led=on")) Led1 = 1;
             if (compare(pQuery, lenQuery, "ledonoff="       )) Led1 = 0;
        }
        whatToSendToId[EspIpdId] = WHAT_LED;
        return 0;
    }
    
    if (compare(pPath, lenPath, "/log"))
    {
        whatToSendToId[EspIpdId] = WHAT_LOG;
        return 0;
    }
    
    whatToSendToId[EspIpdId] = WHAT_NOT_FOUND;
    return 0;
}
static int sendMain()
{
    //Do nothing if a command is already in progress
    if (AtBusy()) return 0;
    
    //Send data. Do each id in turn to avoid interleaved calls to fillers like LogEnumerate which are not reentrant.
    for(int id = 0; id < 4; id++)
    {
        if (lineToSendToId[id] > 0)
        {
            int length = fillSendBuffer(id);
            if (length < 0) return -1;
            if (length > 0) AtSendData(id, length, sendbuffer, NULL);
        }
        else if (lineToSendToId[id] < 0)
        {
            AtClose(id, NULL);
            lineToSendToId[id] = 0;
        }
    }

    return 0;
}
int startMain()
{
    //Do nothing if a command is already in progress
    if (AtBusy()) return 0;
    
    //Do nothing until WiFi is running
    if (!WifiStarted()) return 0;
    
    //Start the server if not started
    static int startServerReply = AT_NONE;
    if (startServerReply != AT_SUCCESS) AtStartServer(SERVER_PORT, &startServerReply);
    return 0;
}
int ServerMain()
{
    if (startMain()) return -1;
    if ( recvMain()) return -1;
    if ( sendMain()) return -1;
    
    return 0;
}