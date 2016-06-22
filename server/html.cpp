#include    "mbed.h"
#include     "log.h"
#include "ds18b20.h"
#include     "rtc.h"
#include    "main.h"
#include     "cfg.h"
#include      "io.h"
#include "heating.h"
#include    "html.h"
#include      "at.h"

#define SEND_BUFFER_SIZE 256
static char sendbuffer[SEND_BUFFER_SIZE];

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
            "<title>Heating</title>\r\n"
            "</head>\r\n"
            "<style>\r\n"
            "* { font-family: Tahoma, Geneva, sans-serif; }\r\n"
            "</style>\r\n"
            "<body>\r\n";

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

#define THIS_CHUNK_IS_NOT_FINISHED 0
#define THIS_CHUNK_IS_FINISHED     1
#define NO_MORE_CHUNKS             2

static int fillNotFound(int chunk, int* pLength)
{
    if (chunk == 1)
    {
        *pLength = fillChunk("HTTP/1.0 404 Not Found\r\n\r\n");
        return THIS_CHUNK_IS_FINISHED;
    }
    else
    {
        *pLength = 0;
        return NO_MORE_CHUNKS;
    }
}
static int fillBadRequest(int chunk, int* pLength)
{
    if (chunk == 1)
    {
        *pLength = fillChunk("HTTP/1.0 400 Bad Request\r\n\r\n");
        return THIS_CHUNK_IS_FINISHED;
    }
    else
    {
        *pLength = 0;
        return NO_MORE_CHUNKS;
    }
}
static int fillBadMethod(int chunk, int* pLength)
{
    if (chunk == 1)
    {
        *pLength = fillChunk("HTTP/1.0 405 Method Not Allowed\r\nAllow: GET\r\n\r\n");
        return THIS_CHUNK_IS_FINISHED;
    }
    else
    {
        *pLength = 0;
        return NO_MORE_CHUNKS;
    }
}
static int fillLog(int chunk, int* pLength)
{
    int posn = 0;
    int result = 0;
    if (++posn == chunk)
    {
        *pLength = fillChunk(header);
        result = THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        *pLength = fillChunk("<code><pre>");
        result = THIS_CHUNK_IS_FINISHED;
    }
        
    if (++posn == chunk)
    {
        *pLength = fillLogChunk();
        result = *pLength < SEND_BUFFER_SIZE ? THIS_CHUNK_IS_FINISHED : THIS_CHUNK_IS_NOT_FINISHED; //only increments after the last chunk
    }
    if (++posn == chunk)
    {
         *pLength = fillChunk(
            "</pre></code>\r\n"
            "</body>\r\n"
            "</html>\r\n");
        result = THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        *pLength = 0;
        result = NO_MORE_CHUNKS;
    }
    return result;
}
static int fillLed(int chunk, int* pLength)
{
    int posn = 0;
    int len = 0;
    if (++posn == chunk)
    {
        len = fillChunk(header);
    }
    if (++posn == chunk)
    {
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
    }
    if (++posn == chunk)
    {
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
    }
    if (++posn == chunk)
    {
        int16_t temp;
        
        temp = DS18B20ValueFromRom(CfgTankRom);
        len = addChunk (len, "Tank  temperature: ");
        len = addTemperature(len, temp);
        len = addChunk (len, "<br/>\r\n");
        
        temp = DS18B20ValueFromRom(CfgInletRom);
        len = addChunk (len, "Inlet temperature: ");
        len = addTemperature(len, temp);
        len = addChunk (len, "<br/><br/>\r\n");
    }
    if (++posn == chunk)
    {
        len = addChunkF(len, "Battery voltage: %1.2f<br/><br/>\r\n", IoVbat());
    }
    if (++posn == chunk) //LED on off checkbox form
    {
        len = fillChunk(
        "<br/>"
        "<form action='/' method='get'>\r\n"
        "<input type='hidden' name='ledonoff'>\r\n");
    }
    if (++posn == chunk)
    {
        len = fillChunk("Led <input type='checkbox' name='led' value='on'");
        if (Led1) len = addChunk(len, " checked='checked'");
        len = addChunk(len, ">\r\n");
        len = addChunk(len,
        "<input type='submit' value='Set'><br/>\r\n"
        "</form>\r\n");
    }
    if (++posn == chunk) //Scheme text input form
    {
        len = addChunk(len,
        "<br/>"
        "<form action='/' method='get'>\r\n"
        "Scheme 1 <input type='text' name='scheme1' value='");
        len += HeatingGetSchemeA(1, SEND_BUFFER_SIZE - len, sendbuffer + len);
        len = addChunk(len, "'>\r\n");
    }
    if (++posn == chunk)
    {
        len = fillChunk(
        "<input type='submit' value='Set'><br/>\r\n"
        "</form>\r\n");
    }
    if (++posn == chunk)
    {
        len = fillChunk(
        "</body>\r\n"
        "</html>\r\n");
    }
    if (++posn == chunk)
    {
        len = -1;
    }
    
    if (len >= 0)
    {
        *pLength = len;
        return THIS_CHUNK_IS_FINISHED;
    }
    else
    {
        *pLength = 0;
        return NO_MORE_CHUNKS;
    }
}
static int fillSendBuffer(int whatToSend, int chunk, int* pLength)
{
    switch(whatToSend)
    {
        case HTML_NOT_FOUND:   return fillNotFound  (chunk, pLength);
        case HTML_BAD_REQUEST: return fillBadRequest(chunk, pLength);
        case HTML_BAD_METHOD:  return fillBadMethod (chunk, pLength);
        case HTML_LOG:         return fillLog       (chunk, pLength);
        case HTML_LED:         return fillLed       (chunk, pLength);
        default:
            LogF("No such 'whatToSendToId' %s", whatToSend);
            return -1;
    }
}

static int  whatToSendToId[4];
static int chunkToSendToId[4]; //0 == do nothing

int HtmlInit()
{
    for (int id = 0; id < 4; id++)
    {
         whatToSendToId[id] = 0;
        chunkToSendToId[id] = 0;
    }
    return 0;
}
void HtmlStart(int id, int whatToSend)
{
     whatToSendToId[id] = whatToSend;
    chunkToSendToId[id] = 1;           //Set up the next line to send to be the first
}
int HtmlGetNextChunkToSend(int id, int* pLength, char** ppBuffer)
{
    *ppBuffer = sendbuffer;

    if (chunkToSendToId[id] == 0) return HTML_NOTHING_TO_SEND;
    
    int chunkResult = fillSendBuffer(whatToSendToId[id], chunkToSendToId[id], pLength);
    
    switch (chunkResult)
    {
        case THIS_CHUNK_IS_NOT_FINISHED:
            ; //Don't increment the chunk
            return HTML_MORE_TO_SEND;
        case THIS_CHUNK_IS_FINISHED:
            chunkToSendToId[id] += 1;
            return HTML_MORE_TO_SEND;
        case NO_MORE_CHUNKS:
            chunkToSendToId[id] = 0;
            return HTML_NO_MORE_TO_SEND;
        default:
            return HTML_ERROR;
    }
}
