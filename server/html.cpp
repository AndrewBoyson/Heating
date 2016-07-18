#include    "mbed.h"
#include     "log.h"
#include "ds18b20.h"
#include     "rtc.h"
#include    "main.h"
#include     "cfg.h"
#include      "io.h"
#include "heating.h"
#include      "at.h"
#include "request.h"
#include  "server.h"

#define SCHEDULE_CHARACTER_LENGTH 30

#define SEND_BUFFER_SIZE 256
static char sendbuffer[SEND_BUFFER_SIZE];
static int length;

static void addChunkF(char *fmt, ...)
{
    //Set up variable arguments
    va_list argptr;
    va_start(argptr, fmt);
        
    //Fill the buffer
    int room = SEND_BUFFER_SIZE - length;
    int sent = vsnprintf(sendbuffer + length, room, fmt, argptr);
    if (sent > room) sent = room;
    if (sent < 0) sent = 0;
    
    //Finish with variable arguments
    va_end(argptr);
    
    //Return length
    length += sent;
    if (length > SEND_BUFFER_SIZE) length = SEND_BUFFER_SIZE;
}

static void addChunk(char *text)
{
    strncpy(sendbuffer + length, text, SEND_BUFFER_SIZE - length);
    length += strlen(text);
    if (length > SEND_BUFFER_SIZE) length = SEND_BUFFER_SIZE;
}
static void fillChunk(char * text)
{
    strncpy(sendbuffer, text, SEND_BUFFER_SIZE);
    length = strlen(text);
    if (length > SEND_BUFFER_SIZE) length = SEND_BUFFER_SIZE;
}
static void fillLogChunk() //Sets length. If length is less than the buffer size then that was the last chunk to send: that could mean a length of zero.
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
    length = p - sendbuffer;
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

static void addTemperature(int16_t value)
{
    switch (value)
    {
        case DS18B20_ERROR_CRC:                     addChunk ("CRC error"                     ); break;
        case DS18B20_ERROR_NOT_FOUND:               addChunk ("ROM not found"                 ); break;
        case DS18B20_ERROR_TIMED_OUT:               addChunk ("Timed out"                     ); break;
        case DS18B20_ERROR_NO_DEVICE_PRESENT:       addChunk ("No device detected after reset"); break;
        case DS18B20_ERROR_NO_DEVICE_PARTICIPATING: addChunk ("Device removed during search"  ); break;
        default:                                    addChunkF("%1.1f", value / 16.0           ); break;
    }
}

#define THIS_CHUNK_IS_NOT_FINISHED 0
#define THIS_CHUNK_IS_FINISHED     1
#define NO_MORE_CHUNKS             2

static int fillNotFound(int chunk)
{
    if (chunk == 1)
    {
        fillChunk("HTTP/1.0 404 Not Found\r\n\r\n");
        return THIS_CHUNK_IS_FINISHED;
    }
    return NO_MORE_CHUNKS;
}
static int fillBadRequest(int chunk)
{
    if (chunk == 1)
    {
        fillChunk("HTTP/1.0 400 Bad Request\r\n\r\n");
        return THIS_CHUNK_IS_FINISHED;
    }
    return NO_MORE_CHUNKS;
}
static int fillBadMethod(int chunk)
{
    if (chunk == 1)
    {
        fillChunk("HTTP/1.0 405 Method Not Allowed\r\nAllow: GET\r\n\r\n");
        return THIS_CHUNK_IS_FINISHED;
    }
    return NO_MORE_CHUNKS;
}
static int fillNotImplemented(int chunk)
{
    if (chunk == 1)
    {
        fillChunk("HTTP/1.0 501 Not Implemented\r\n\r\n");
        return THIS_CHUNK_IS_FINISHED;
    }
    return NO_MORE_CHUNKS;
}
static int fillLog(int chunk)
{
    int posn = 0;
    if (++posn == chunk)
    {
        fillChunk(header);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        fillChunk("<code><pre>");
        return THIS_CHUNK_IS_FINISHED;
    }
        
    if (++posn == chunk)
    {
        fillLogChunk();
        if (length == SEND_BUFFER_SIZE) return THIS_CHUNK_IS_NOT_FINISHED; //Buffer is full so there is more of this chunk to come
        else                            return THIS_CHUNK_IS_FINISHED;     //Buffer is only part filled so move onto the next chunk
    }
    if (++posn == chunk)
    {
        fillChunk(
            "</pre></code>\r\n"
            "</body>\r\n"
            "</html>\r\n");
        return THIS_CHUNK_IS_FINISHED;
    }
    return NO_MORE_CHUNKS;
}
static void addFormTextInput(char* action, char* label, char* name, int size, char* value)
{
    addChunkF("<br/><form action='%s' method='get'>\r\n", action);
    addChunkF("%s <input type='text' name='%s' size='%d' value='%s'>\r\n", label, name, size, value);
    addChunk("<input type='submit' value='Set'><br/>\r\n</form>\r\n");
}
static void addFormIntInput(char* action, char* label, char* name, int size, int value)
{
    addChunkF("<br/><form action='%s' method='get'>\r\n", action);
    addChunkF("%s <input type='text' name='%s' size='%d' value='%d'>\r\n", label, name, size, value);
    addChunk("<input type='submit' value='Set'><br/>\r\n</form>\r\n");
}
static void addFormCheckInput(char* action, char* label, char* name, int value)
{
    addChunk("<br/><form action='");
    addChunk(action);
    addChunk("' method='get'>\r\n");
    addChunk("<input type='hidden' name='");
    addChunk(name);
    addChunk("'>\r\n");
    addChunk(label);
    addChunk(" <input type='checkbox' name='on' onCheck='submit();");
    if (value) addChunk(" checked='checked'");
    addChunk(">\r\n");
    //addChunk("<input type='submit' value='Set'>\r\n");
    addChunk("</form><br/>\r\n");
}
static int fillLed(int chunk)
{
    int posn = 0;
    length = 0;
    if (++posn == chunk)
    {
        fillChunk(header);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        struct tm tm;
        RtcGetTm(&tm);
        addChunkF("Time: %d-%02d-%02d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        switch(tm.tm_wday)
        {
            case  0: addChunk("Sun"); break;
            case  1: addChunk("Mon"); break;
            case  2: addChunk("Tue"); break;
            case  3: addChunk("Wed"); break;
            case  4: addChunk("Thu"); break;
            case  5: addChunk("Fri"); break;
            case  6: addChunk("Sat"); break;
            default: addChunk("???"); break;
        }
        addChunkF(" %02d:%02d:%02d UTC", tm.tm_hour, tm.tm_min, tm.tm_sec);
        addChunk ("<br/><br/>\r\n");
        addChunkF("Scan &micro;s: %d<br/><br/>\r\n", MainScanUs);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        addChunk ("Devices:<br/>\r\n");
        for (int j = 0; j < DS18B20DeviceCount; j++)
        {
            addChunkF("%d - ", j);
            for (int i = 0; i < 8; i++) addChunkF(" %02X", DS18B20DeviceList[j*8 + i]);
            addChunk(" - ");
            addTemperature(DS18B20Value[j]);
            addChunk ("<br/>\r\n");
        }
        addChunk ("<br/>\r\n");
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        int16_t temp;
        
        temp = DS18B20ValueFromRom(CfgTankRom);
        addChunk ("Tank  temperature: ");
        addTemperature(temp);
        addChunk ("<br/>\r\n");
        
        temp = DS18B20ValueFromRom(CfgInletRom);
        addChunk ("Inlet temperature: ");
        addTemperature(temp);
        addChunk ("<br/><br/>\r\n");
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        addChunkF("Battery voltage: %1.2f<br/><br/>\r\n", IoVbat());
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        addFormCheckInput("/", "Led", "led", Led1);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        addFormCheckInput("/", "Heating", "heating", HeatingGetOnOff());
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        char value[SCHEDULE_CHARACTER_LENGTH];
        HeatingScheduleRead(0, SCHEDULE_CHARACTER_LENGTH, value);
        addFormTextInput("/", "Schedule 1", "schedule1", SCHEDULE_CHARACTER_LENGTH, value);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        char value[SCHEDULE_CHARACTER_LENGTH];
        HeatingScheduleRead(1, SCHEDULE_CHARACTER_LENGTH, value);
        addFormTextInput("/", "Schedule 2", "schedule2", SCHEDULE_CHARACTER_LENGTH, value);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        char value[SCHEDULE_CHARACTER_LENGTH];
        HeatingScheduleRead(2, SCHEDULE_CHARACTER_LENGTH, value);
        addFormTextInput("/", "Schedule 3", "schedule3", SCHEDULE_CHARACTER_LENGTH, value);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        addFormIntInput("/", "Mon", "mon", 1, HeatingGetMon()+1);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        addFormIntInput("/", "Tue", "tue", 1, HeatingGetTue()+1);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        addFormIntInput("/", "Wed", "wed", 1, HeatingGetWed()+1);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        addFormIntInput("/", "Thu", "thu", 1, HeatingGetThu()+1);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        addFormIntInput("/", "Fri", "fri", 1, HeatingGetFri()+1);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        addFormIntInput("/", "Sat", "sat", 1, HeatingGetSat()+1);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        addFormIntInput("/", "Sun", "sun", 1, HeatingGetSun()+1);
        return THIS_CHUNK_IS_FINISHED;
    }
    if (++posn == chunk)
    {
        fillChunk(
        "</body>\r\n"
        "</html>\r\n");
        return THIS_CHUNK_IS_FINISHED;
    }
    return NO_MORE_CHUNKS;
}
static int fillSendBuffer(int whatToSend, int chunk)
{
    switch(whatToSend)
    {
        case REQUEST_NOT_FOUND:   return fillNotFound      (chunk);
        case REQUEST_BAD_REQUEST: return fillBadRequest    (chunk);
        case REQUEST_BAD_METHOD:  return fillBadMethod     (chunk);
        case REQUEST_LOG:         return fillLog           (chunk);
        case REQUEST_LED:         return fillLed           (chunk);
        default:                  return fillNotImplemented(chunk);
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
    if (chunkToSendToId[id] == 0) return SERVER_NOTHING_TO_SEND;
    
    int chunkResult = fillSendBuffer(whatToSendToId[id], chunkToSendToId[id]);
    
    *pLength = length;
    *ppBuffer = sendbuffer;    
    
    switch (chunkResult)
    {
        case THIS_CHUNK_IS_NOT_FINISHED:
            ; //Don't increment the chunk
            return SERVER_MORE_TO_SEND;
        case THIS_CHUNK_IS_FINISHED:
            chunkToSendToId[id] += 1;
            return SERVER_MORE_TO_SEND;
        case NO_MORE_CHUNKS:
            chunkToSendToId[id] = 0;
            return SERVER_NO_MORE_TO_SEND;
        default:
            return SERVER_ERROR;
    }
}
