#include     "mbed.h"
#include      "log.h"
#include  "ds18b20.h"
#include      "rtc.h"
#include     "time.h"
#include      "cfg.h"
#include  "heating.h"
#include "schedule.h"
#include       "at.h"
#include  "request.h"
#include   "server.h"
#include "response.h"

#define SCHEDULE_CHARACTER_LENGTH 30
static int fillLogChunk() //Returns true if send buffer is full
{
    static int enumerationStarted = false;
    if (!enumerationStarted)
    {
        LogEnumerateStart();
        enumerationStarted = true;
    }
    while (true)
    {
        int c = LogEnumerate();
        if (c == EOF)
        {
            enumerationStarted = false;
            break;
        }
        if (ResponseAddChar(c)) return true;
    }
    return false;
}
static char* header = 
            "<!DOCTYPE html>\r\n"
            "<html lang = 'EN_GB'>\r\n"
            "<head>\r\n"
            "<link rel='icon'       href='/favicon.ico' type='image/x-icon'/>\r\n"
            "<link rel='stylesheet' href='/styles.css'  type='text/css'    />\r\n"
            "<title>Heating</title>\r\n"
            "</head>\r\n"
            "<body>\r\n"
            ;
static void addNavItem(int highlight, char* href, char* title)
{
    ResponseAddChunk("<li ");
    if (highlight) ResponseAddChunk("class='this'");
    else           ResponseAddChunk("            ");
    ResponseAddChunk("><a href='");
    ResponseAddChunk(href);
    ResponseAddChunk("'>");
    ResponseAddChunk(title);
    ResponseAddChunk("</a></li>\r\n");
}
static void addNav(int page)
{
    ResponseAddChunk("<nav><ul>\r\n");
    int posn = 0;
    addNavItem(posn++ == page, "/", "Home");
    addNavItem(posn++ == page, "/log", "Log");
    ResponseAddChunk("</ul></nav>\r\n");
}
static void addTemperature(int16_t value)
{
    switch (value)
    {
        case DS18B20_ERROR_CRC:                     ResponseAddChunk ("CRC error"                     ); break;
        case DS18B20_ERROR_NOT_FOUND:               ResponseAddChunk ("ROM not found"                 ); break;
        case DS18B20_ERROR_TIMED_OUT:               ResponseAddChunk ("Timed out"                     ); break;
        case DS18B20_ERROR_NO_DEVICE_PRESENT:       ResponseAddChunk ("No device detected after reset"); break;
        case DS18B20_ERROR_NO_DEVICE_PARTICIPATING: ResponseAddChunk ("Device removed during search"  ); break;
        default:                                    ResponseAddChunkF("%1.1f", value / 16.0           ); break;
    }
}

int HtmlLog(int chunk)
{
    int posn = -1;
    if (++posn == chunk)
    {
        ResponseAddChunk(header);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        addNav(1);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAddChunk("<code><pre>");
        return RESPONSE_SEND_CHUNK;
    }
        
    if (++posn == chunk)
    {
        int sendBufferFilled = fillLogChunk();
        if (sendBufferFilled) return RESPONSE_SEND_PART_CHUNK; //Buffer is full so there is more of this chunk to come
        else                  return RESPONSE_SEND_CHUNK;      //Buffer is only part filled so move onto the next chunk
    }
    if (++posn == chunk)
    {
        ResponseAddChunk(
            "</pre></code>\r\n"
            "</body>\r\n"
            "</html>\r\n");
        return RESPONSE_SEND_CHUNK;
    }
    return RESPONSE_NO_MORE_CHUNKS;
}
static void addFormStart(char* action)
{
    ResponseAddChunkF("<br/><form action='%s' method='get'>\r\n", action);
}
static void addFormTextInput(char* label, char* name, int size, char* value)
{
    ResponseAddChunkF("%s <input type='text' name='%s' size='%d' value='%s'>\r\n", label, name, size, value);
}
static void addFormIntInput(char* label, char* name, int size, int value)
{
    ResponseAddChunkF("%s <input type='text' name='%s' size='%d' value='%d'>\r\n", label, name, size, value);
}
static void addFormEnd()
{
    ResponseAddChunk("<input type='submit' value='Set'><br/>\r\n</form>\r\n");
}
static void addFormCheckInput(char* action, char* label, char* name, int value)
{
    ResponseAddChunk("<br/><form action='");
    ResponseAddChunk(action);
    ResponseAddChunk("' method='get'>\r\n");
    ResponseAddChunk("<input type='hidden' name='");
    ResponseAddChunk(name);
    ResponseAddChunk("'>\r\n");
    ResponseAddChunk(label);
    ResponseAddChunk(" <input type='checkbox' name='on' onClick='submit();'");
    if (value) ResponseAddChunk(" checked='checked'");
    ResponseAddChunk(">\r\n");
    ResponseAddChunk("</form><br/>\r\n");
}
static void addTm(struct tm* ptm)
{
    ResponseAddChunkF("%d-%02d-%02d ", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
    switch(ptm->tm_wday)
    {
        case  0: ResponseAddChunk("Sun"); break;
        case  1: ResponseAddChunk("Mon"); break;
        case  2: ResponseAddChunk("Tue"); break;
        case  3: ResponseAddChunk("Wed"); break;
        case  4: ResponseAddChunk("Thu"); break;
        case  5: ResponseAddChunk("Fri"); break;
        case  6: ResponseAddChunk("Sat"); break;
        default: ResponseAddChunk("???"); break;
    }
    ResponseAddChunkF(" %02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    if (ptm->tm_isdst) ResponseAddChunk(" BST");
    else               ResponseAddChunk(" GMT");
}
int HtmlLed(int chunk)
{
    int posn = -1;
    if (++posn == chunk)
    {
        ResponseAddChunk(header);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        addNav(0);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        time_t now = time(NULL);
        struct tm tm;
        TimeToTmLocal(now, &tm);
        ResponseAddChunk ("Time: ");
        addTm(&tm);
        ResponseAddChunk ("<br/><br/>\r\n");
        
        ResponseAddChunkF("Scan &micro;s: %d<br/><br/>\r\n", TimeScanUs);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAddChunk ("Devices:<br/>\r\n");
        for (int j = 0; j < DS18B20DeviceCount; j++)
        {
            ResponseAddChunkF("%d - ", j);
            for (int i = 0; i < 8; i++) ResponseAddChunkF(" %02X", DS18B20DeviceList[j*8 + i]);
            ResponseAddChunk(" - ");
            addTemperature(DS18B20Value[j]);
            ResponseAddChunk ("<br/>\r\n");
        }
        ResponseAddChunk ("<br/>\r\n");
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        int16_t temp;
        
        temp = DS18B20ValueFromRom(CfgTankRom);
        ResponseAddChunk ("Tank  temperature: ");
        addTemperature(temp);
        ResponseAddChunk ("<br/>\r\n");
        
        temp = DS18B20ValueFromRom(CfgInletRom);
        ResponseAddChunk ("Inlet temperature: ");
        addTemperature(temp);
        ResponseAddChunk ("<br/><br/>\r\n");
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        addFormCheckInput("/", "Override", "override", ScheduleOverride);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        addFormCheckInput("/", "Auto", "auto", ScheduleGetAuto());
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        char value[SCHEDULE_CHARACTER_LENGTH];
        addFormStart("/");
        ScheduleRead(0, SCHEDULE_CHARACTER_LENGTH, value);
        addFormTextInput("Schedule 1", "schedule1", SCHEDULE_CHARACTER_LENGTH, value);
        ResponseAddChunk ("<br/>\r\n");
        ScheduleRead(1, SCHEDULE_CHARACTER_LENGTH, value);
        addFormTextInput("Schedule 2", "schedule2", SCHEDULE_CHARACTER_LENGTH, value);
        ResponseAddChunk ("<br/>\r\n");
        ScheduleRead(2, SCHEDULE_CHARACTER_LENGTH, value);
        addFormTextInput("Schedule 3", "schedule3", SCHEDULE_CHARACTER_LENGTH, value);
        ResponseAddChunk ("<br/>\r\n");
        addFormEnd();
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        addFormStart("/");
        addFormIntInput("Mon", "mon", 1, ScheduleGetMon()+1);
        addFormIntInput("Tue", "tue", 1, ScheduleGetTue()+1);
        addFormIntInput("Wed", "wed", 1, ScheduleGetWed()+1);
        addFormIntInput("Thu", "thu", 1, ScheduleGetThu()+1);
        addFormIntInput("Fri", "fri", 1, ScheduleGetFri()+1);
        addFormIntInput("Sat", "sat", 1, ScheduleGetSat()+1);
        addFormIntInput("Sun", "sun", 1, ScheduleGetSun()+1);
        addFormEnd();
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAddChunk(
        "</body>\r\n"
        "</html>\r\n");
        return RESPONSE_SEND_CHUNK;
    }
    return RESPONSE_NO_MORE_CHUNKS;
}
