#include     "mbed.h"
#include      "rtc.h"
#include  "rtc-clk.h"
#include "settings.h"
#include       "io.h"
#include      "log.h"


/*
There are three programs available [0|1|2]; any of which can be allocated to a given day [0-6].
Each program contains four transitions with an index [0|1|2|3].
A transition is defined to be a short (16 bit) and consists of:
+---------+--------+--------+---------+
| 15 - 13 |   12   |   11   | 10 - 00 |
+---------+--------+--------+---------+
|         | in use | switch | minute  |
|         | yes/no | on/off | in day  |
|         |   1/0  |  1/0   | 0-1439  |
+---------+--------+--------+---------+
*/

static short encodeTransition(bool inuse, bool onoff, int minutes)
{
    short      transition  = minutes;
               transition &= 0x07FF;
    if (onoff) transition |= 0x0800;
    if (inuse) transition |= 0x1000;
    return transition;
}
static void decodeTransition(short transition, bool* pinuse, bool* ponoff, int* pminutes)
{
    *pinuse   = transition & 0x1000;
    *ponoff   = transition & 0x0800;
    *pminutes = transition & 0x07FF;
}

//[+|-][00-23][00-59];
void ProgramToString(int program, int buflen, char* buffer)
{
    if (buflen < 25) return;
    char* p = buffer;
    for (int i = 0; i < SettingsProgramTransitionCount; i++)
    {
        short transition = SettingsGetProgramTransition(program, i);
        bool  inuse;
        bool  on;
        int   minuteUnits;
        decodeTransition(transition, &inuse, &on, &minuteUnits);
        if (!inuse) continue;
        
        int minuteTens  = minuteUnits / 10; minuteUnits %= 10;
        int   hourUnits = minuteTens  /  6; minuteTens  %=  6;
        int   hourTens  =   hourUnits / 10;   hourUnits %= 10;
        
        if (p > buffer) *p++ = ' ';
        *p++ = on ? '+' : '-';
        *p++ = hourTens    + '0';
        *p++ = hourUnits   + '0';
        *p++ = minuteTens  + '0';
        *p++ = minuteUnits + '0';
    }
    *p = 0;
}
static void handleParseDelim(int program, int* pIndex, bool* pInUse, bool* pOn, int* pHourTens,  int* pHourUnits, int* pMinuteTens, int* pMinuteUnits)
{
    int hour = *pHourTens * 10 + *pHourUnits;
    if (hour   <  0) *pInUse = false;
    if (hour   > 23) *pInUse = false;
    
    int minute = *pMinuteTens * 10 + *pMinuteUnits;
    if (minute <  0) *pInUse = false;
    if (minute > 59) *pInUse = false;
    
    int minutes = hour * 60 + minute;
    
    short transition = encodeTransition(*pInUse, *pOn, minutes);
    SettingsSetProgramTransition(program, *pIndex, transition);
    
    *pIndex      += 1;
    *pInUse       = false;
    *pOn          = false;
    *pHourTens    = 0;
    *pHourUnits   = 0;
    *pMinuteTens  = 0;
    *pMinuteUnits = 0;

}
void ProgramParse(int program, char* p)
{
    int            i = 0;
    bool       inUse = false;  bool          on = false;
    int    hourUnits = 0;      int    hourTens  = 0;
    int  minuteUnits = 0;      int  minuteTens  = 0;
    while (*p && i < SettingsProgramTransitionCount) 
    {
        if      (*p == '+')              { on = true ; }
        else if (*p == '-')              { on = false; }
        else if (*p >= '0' && *p <= '9') { inUse = true; hourTens = hourUnits; hourUnits = minuteTens; minuteTens = minuteUnits; minuteUnits = *p - '0'; }
        else if (*p == ' ')              { handleParseDelim(program, &i, &inUse, &on, &hourTens, &hourUnits, &minuteTens, &minuteUnits); }
        p++;
    }
    while (i < SettingsProgramTransitionCount) handleParseDelim(program, &i, &inUse, &on, &hourTens, &hourUnits, &minuteTens, &minuteUnits);
}



static bool checkProgramBeforeOverride()
{   
    if (RtcClockIsNotSet()) return false;

    struct tm tm;
    RtcGetTmLocal(&tm);
    
    int dayOfWeek = tm.tm_wday;
    
    int program = SettingsGetProgramDay(dayOfWeek);
    
    bool calling = false;
    for (int i = 0; i < SettingsProgramTransitionCount; i++)
    {
        short transition = SettingsGetProgramTransition(program, i);
        bool  inuse;
        bool  on;
        int   minutes;
        decodeTransition(transition, &inuse, &on, &minutes);
        if (!inuse) continue;
        int minutesNow = tm.tm_hour * 60 + tm.tm_min;
        if (minutes <= minutesNow) calling = on;
    }
    
    return calling;
}

static bool programBeforeOverrideHasChanged(bool isOn)
{
    static bool wasOn = false;
    bool changed = wasOn != isOn;
    wasOn = isOn;
    return changed;
}

bool ProgramIsCalling;

int ProgramMain()
{    
    if (SettingsGetProgramAuto())
    {
        int programBeforeOverride = checkProgramBeforeOverride();
        
        //Reset override whenever the program calling changes
        if (programBeforeOverrideHasChanged(programBeforeOverride)) SettingsSetProgramOverride(false);
        
        //Call for heating
        if (SettingsGetProgramOverride()) ProgramIsCalling = !programBeforeOverride;
        else                              ProgramIsCalling =  programBeforeOverride;
    }
    else
    {
        SettingsSetProgramOverride(false);
        ProgramIsCalling = false;
    }
    
    return 0;
}