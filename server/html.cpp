#include     "mbed.h"
#include      "log.h"
#include  "ds18b20.h"
#include      "rtc.h"
#include     "main.h"
#include      "cfg.h"
#include  "heating.h"
#include "schedule.h"
#include       "at.h"
#include  "request.h"
#include   "server.h"
#include "response.h"
#include "radiator.h"
#include   "boiler.h"
#include "settings.h"
#include "watchdog.h"

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
    "<html lang='en-GB'>\r\n"
    "<head>\r\n"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>\r\n"
    "<link rel='icon'       href='/favicon.ico' type='image/x-icon'/>\r\n"
    "<link rel='stylesheet' href='/styles.css'  type='text/css'    />\r\n"
    "<title>Heating</title>\r\n"
    "</head>\r\n"
    "<body>\r\n";
static char* footer = 
    "</body>\r\n"
    "</html>\r\n";
static void addNavItem(int highlight, char* href, char* title)
{
    ResponseAdd("<li ");
    if (highlight) ResponseAdd("class='this'");
    else           ResponseAdd("            ");
    ResponseAdd("><a href='");
    ResponseAdd(href);
    ResponseAdd("'>");
    ResponseAdd(title);
    ResponseAdd("</a></li>\r\n");
}
enum pages
{
        HOME_PAGE,
    SCHEDULE_PAGE,
     HEATING_PAGE,
      BOILER_PAGE,
      SYSTEM_PAGE,
         LOG_PAGE
};
static void addNav(int page)
{
    ResponseAdd("<nav><ul>\r\n");
    addNavItem(page ==     HOME_PAGE, "/",        "Home");
    addNavItem(page == SCHEDULE_PAGE, "/timer",   "Timer");
    addNavItem(page ==  HEATING_PAGE, "/heating", "Heating");
    addNavItem(page ==   BOILER_PAGE, "/boiler",  "Boiler");
    addNavItem(page ==   SYSTEM_PAGE, "/system",  "System");
    addNavItem(page ==      LOG_PAGE, "/log",     "Log");
    ResponseAdd("</ul></nav>\r\n");
}

static void addLabelledValue(char* label, float labelwidth, char* value)
{
    ResponseAdd("<div>");
    ResponseAddF("<div style='width:%.1fem; display:inline-block;'>", labelwidth);
    ResponseAdd(label);
    ResponseAdd("</div>");
    ResponseAdd(value);
    ResponseAdd("</div>\r\n");
}
static void addLabelledOnOff(char* label, float labelwidth, bool value)
{
    if (value) addLabelledValue(label, labelwidth, "On");
    else       addLabelledValue(label, labelwidth, "Off");
}
static void addLabelledInt(char* label, float labelwidth, int value)
{
    char buffer[30];
    snprintf(buffer, sizeof(buffer), "%d", value);
    addLabelledValue(label, labelwidth, buffer);
}
static void addLabelledTemperature(char* label, float labelwidth, int16_t value)
{
    char buffer[DS18B20_VALUE_STRING_LENGTH];
    DS18B20ValueToString(value, buffer);
    addLabelledValue(label, labelwidth, buffer);
}
static void addFormStart(char* action)
{
    ResponseAddF("<form action='%s' method='get'>\r\n", action);
}
static void addFormTextInput(float width, char* label, float labelwidth, char* name, float inputwidth, char* value)
{
    if (width < 0.01) ResponseAdd ("<div>");
    else              ResponseAddF("<div style='width:%.1fem; display:inline-block;'>", width);
    ResponseAddF("<div style='width:%.1fem; display:inline-block;'>%s</div>", labelwidth, label);
    ResponseAddF("<input type='text' name='%s' style='width:%.1fem;' value='%s'>", name, inputwidth, value);
    ResponseAdd ("</div>\r\n");              
}
static void addFormIntInput(float width, char* label, float labelwidth, char* name, float inputwidth, int value)
{
    char buffer[30];
    snprintf(buffer, sizeof(buffer), "%d", value);
    addFormTextInput(width, label, labelwidth, name, inputwidth, buffer);
}
static void addFormEnd()
{
    ResponseAdd("<input type='submit' value='Set' style='display:none;'>\r\n</form>\r\n");
}
static void addFormCheckInput(char* action, char* label, char* name, char* button)
{
    ResponseAddF("<form action='%s' method='get'>\r\n",  action);
    ResponseAddF("<input type='hidden' name='%s'>\r\n",  name);
    ResponseAddF("%s\r\n",                               label);
    ResponseAddF("<input type='submit' value='%s'>\r\n", button);
    ResponseAdd ("</form>\r\n");
}
static void addTm(struct tm* ptm)
{
    ResponseAddF("%d-%02d-%02d ", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
    switch(ptm->tm_wday)
    {
        case  0: ResponseAdd("Sun"); break;
        case  1: ResponseAdd("Mon"); break;
        case  2: ResponseAdd("Tue"); break;
        case  3: ResponseAdd("Wed"); break;
        case  4: ResponseAdd("Thu"); break;
        case  5: ResponseAdd("Fri"); break;
        case  6: ResponseAdd("Sat"); break;
        default: ResponseAdd("???"); break;
    }
    ResponseAddF(" %02d:%02d:%02d", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    if      (ptm->tm_isdst  > 0) ResponseAdd(" BST");
    else if (ptm->tm_isdst == 0) ResponseAdd(" GMT");
    else                         ResponseAdd(" UTC");
}
int HtmlHome(int chunk)
{
    int posn = -1;
    if (++posn == chunk)
    {
        ResponseAdd(header);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        addNav(HOME_PAGE);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd("<h1>Time</h1>\r\n");
        struct tm tm;
        RtcGetTmUtc(&tm);
        ResponseAdd("<div>\r\n");
        addTm(&tm);
        ResponseAdd("</div>\r\n");
        RtcGetTmLocal(&tm);
        ResponseAdd("<div>\r\n");
        addTm(&tm);
        ResponseAdd("</div>\r\n");
        
        ResponseAdd("<h1>Temperature</h1>\r\n");
        int16_t temp;
        temp = DS18B20ValueFromRom(CfgHallRom);
        addLabelledTemperature("Hall", 3, temp);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd("<h1>Heating mode</h1>\r\n");
        if (ScheduleAuto) addFormCheckInput("/", "Mode is winter", "autooff", "change to summer");
        else              addFormCheckInput("/", "Mode is summer", "autoon" , "change to winter");
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd("<h1>Heating timer</h1>\r\n");
        if (ScheduleAuto)
        {
            if (ScheduleIsCalling)
            {
                if (ScheduleOverride) addFormCheckInput("/", "Heating is on (manual)", "overrideoff", "turn it off");
                else                  addFormCheckInput("/", "Heating is on (auto)",   "overrideon",  "turn it off");
            }
            else
            {
                if (ScheduleOverride) addFormCheckInput("/", "Heating is off (manual)", "overrideoff", "turn it on");
                else                  addFormCheckInput("/", "Heating is off (auto)",   "overrideon",  "turn it on");
            }
        }
        else
        {
            ResponseAdd("Heating is off (summer)");
        }
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd(footer);
        return RESPONSE_SEND_CHUNK;
    }
    return RESPONSE_NO_MORE_CHUNKS;
}
int HtmlTimer(int chunk)
{
    int posn = -1;
    if (++posn == chunk)
    {
        ResponseAdd(header);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        addNav(SCHEDULE_PAGE);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd("<h1>Schedules</h1>\r\n");
        const int SCHEDULE_CHARACTER_LENGTH = 30;
        char value[SCHEDULE_CHARACTER_LENGTH];
        addFormStart("/timer");
        ScheduleToString(0, SCHEDULE_CHARACTER_LENGTH, value); addFormTextInput(0, "1", 1, "schedule1", 15, value);
        ScheduleToString(1, SCHEDULE_CHARACTER_LENGTH, value); addFormTextInput(0, "2", 1, "schedule2", 15, value);
        ScheduleToString(2, SCHEDULE_CHARACTER_LENGTH, value); addFormTextInput(0, "3", 1, "schedule3", 15, value);
        addFormEnd();
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd("<h1>Days</h1>\r\n");
        addFormStart("/timer");
        ResponseAdd("<div>\r\n");
        addFormIntInput(3.3, "Mon", 2.3, "mon", 1, ScheduleDay[1] + 1);
        addFormIntInput(3.3, "Tue", 2.3, "tue", 1, ScheduleDay[2] + 1);
        addFormIntInput(3.3, "Wed", 2.3, "wed", 1, ScheduleDay[3] + 1);
        addFormIntInput(3.3, "Thu", 2.3, "thu", 1, ScheduleDay[4] + 1);
        addFormIntInput(3.3, "Fri", 2.3, "fri", 1, ScheduleDay[5] + 1);
        ResponseAdd("</div>\r\n");
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {       
        ResponseAdd("<div>\r\n");
        addFormIntInput(3.3, "Sat", 2.3, "sat", 1, ScheduleDay[6] + 1);
        addFormIntInput(3.3, "Sun", 2.3, "sun", 1, ScheduleDay[0] + 1);
        ResponseAdd("</div>\r\n");
        addFormEnd();
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd(footer);
        return RESPONSE_SEND_CHUNK;
    }
    return RESPONSE_NO_MORE_CHUNKS;
}
int HtmlHeating(int chunk)
{
    int posn = -1;
    if (++posn == chunk)
    {
        ResponseAdd(header);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        addNav(HEATING_PAGE);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {        
        ResponseAdd("<h1>Inputs</h1>\r\n");
        int16_t temp = DS18B20ValueFromRom(CfgHallRom);
        addLabelledTemperature("Hall"   , 10, temp);
        addLabelledOnOff("Winter mode"  , 10, ScheduleAuto);
        addLabelledOnOff("Heating timer", 10, ScheduleIsCalling);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd("<h1>Parameters</h1>\r\n");
        addFormStart("/heating");
        addFormIntInput(0, "Winter (night)", 10, "nighttemp", 2, SettingsGetNightTemperature());
        addFormIntInput(0, "Summer (frost)", 10, "frosttemp", 2, SettingsGetFrostTemperature());
        addFormEnd();
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd("<h1>Output</h1>\r\n");
        addLabelledOnOff("Radiator pump" , 10, RadiatorPump);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd(footer);
        return RESPONSE_SEND_CHUNK;
    }
    return RESPONSE_NO_MORE_CHUNKS;
}
int HtmlBoiler(int chunk)
{
    int posn = -1;
    if (++posn == chunk)
    {
        ResponseAdd(header);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        addNav(BOILER_PAGE);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {        
        ResponseAdd("<h1>Inputs</h1>\r\n");
        int16_t temp;
        
        temp = DS18B20ValueFromRom(CfgTankRom);          addLabelledTemperature("Tank",          10, temp);
        temp = DS18B20ValueFromRom(CfgBoilerOutputRom);  addLabelledTemperature("Boiler output", 10, temp);
        temp = DS18B20ValueFromRom(CfgBoilerReturnRom);  addLabelledTemperature("Boiler return", 10, temp);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd("<h1>Parameters</h1>\r\n");
        addFormStart("/boiler");
        addFormIntInput(0, "Tank set point",         10, "tanksetpoint",   2, SettingsGetTankSetPoint());
        addFormIntInput(0, "Tank hysteresis",        10, "tankhysteresis", 2, SettingsGetTankHysteresis());
        addFormIntInput(0, "Boiler run on residual", 10, "boilerresidual", 2, SettingsGetBoilerRunOnResidual());
        addFormIntInput(0, "Boiler run on time",     10, "boilerrunon",    2, SettingsGetBoilerRunOnTime());
        addFormEnd();
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd("<h1>Outputs</h1>\r\n");
        addLabelledOnOff("Boiler flame", 10, BoilerCall);
        addLabelledOnOff("Boiler pump" , 10, BoilerPump);
        
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd(footer);
        return RESPONSE_SEND_CHUNK;
    }
    return RESPONSE_NO_MORE_CHUNKS;
}
int HtmlSystem(int chunk)
{
    int posn = -1;
    if (++posn == chunk)
    {
        ResponseAdd(header);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        addNav(SYSTEM_PAGE);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd("<h1>Scan times</h1>\r\n");
        addLabelledInt("Program &micro;s", 8, MainScanUs);
        addLabelledInt("Devices ms",       8, DS18B20ScanMs);

        ResponseAdd("<h1>Watchdog</h1>\r\n");
        if (WatchdogFlag) addFormCheckInput("/system", "Watchdog flag is on", "watchdogflagoff", "turn it off");
        else              ResponseAdd("<div>Watchdog flag is off</div>\r\n");

        ResponseAdd("<h1>DS18B20 1-wire devices</h1>\r\n");
        for (int device = 0; device < DS18B20DeviceCount; device++)
        {
            char addressbuffer[DS18B20_ADDRESS_STRING_LENGTH];   DS18B20AddressToString(device,             addressbuffer);
            char   valuebuffer[DS18B20_VALUE_STRING_LENGTH];     DS18B20ValueToString  (DS18B20Value[device], valuebuffer);
            ResponseAddF("<div>%d - %s - %s</div>\r\n", device, addressbuffer, valuebuffer);
        }
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd(footer);
        return RESPONSE_SEND_CHUNK;
    }
    return RESPONSE_NO_MORE_CHUNKS;
}
int HtmlLog(int chunk)
{
    int posn = -1;
    if (++posn == chunk)
    {
        ResponseAdd(header);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        addNav(LOG_PAGE);
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd("<h1>Log</h1>\r\n");
        ResponseAdd("<pre><code>");
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
        ResponseAdd("</code></pre>\r\n");
        return RESPONSE_SEND_CHUNK;
    }
    if (++posn == chunk)
    {
        ResponseAdd(footer);
        return RESPONSE_SEND_CHUNK;
    }
    return RESPONSE_NO_MORE_CHUNKS;
}
