#include     "mbed.h"
#include      "rtc.h"
#include  "rtc-clk.h"
#include "settings.h"
#include       "io.h"
#include      "log.h"


//[*|-][00-23][00-59];
void ProgramToString(int program, int buflen, char* buffer)
{
    if (buflen < 25) return;
    char* p = buffer;
    for (int cycle = 0; cycle < 4; cycle++)
    {
        bool     on      = SettingsGetProgramCycleOn(program, cycle);
        unsigned minutes = SettingsGetProgramCycleMinutes(program, cycle);
        unsigned hours = minutes / 60;
        if (hours > 23) continue;
        unsigned tensOfHours = hours / 10;
        hours %= 10;
        minutes %= 60;
        unsigned tensOfMinutes = minutes / 10;
        minutes %= 10;
        if (p > buffer) *p++ = ' ';
        *p++ = on ? '+' : '-';
        *p++ = tensOfHours + '0';
        *p++ = hours + '0';
        *p++ = tensOfMinutes + '0';
        *p++ = minutes + '0';
    }
    *p = 0;
}
static void handleCycleDelim(int program, int cycle, bool on, unsigned hours, unsigned mins)
{
    unsigned total = hours * 60 + mins;
    if (total < 1440)
    {
        SettingsSetProgramCycleOn(program, cycle, on);
        SettingsSetProgramCycleMinutes(program, cycle, total);
    }
    else
    {
        SettingsSetProgramCycleOn(program, cycle, false);
        SettingsSetProgramCycleMinutes(program, cycle, 1440);
    }
}
void ProgramParse(int program, char* text)
{
    int cycle = 0;
    bool on = false;
    unsigned hours = 24;
    unsigned mins = 0;
    int posn = 0;
    char *p = text;
    while (*p) 
    {
        if (*p == '+')
        {
            on = true;
            posn = 0;
        }
        else if (*p == '-')
        {
            on = false;
            posn = 0;
        }
        else if (*p == ' ')
        {
            handleCycleDelim(program, cycle, on, hours, mins);
            cycle++;
            on = false;
            hours = 24;
            mins = 0;
            posn = 0;
        }
        else if (*p >= '0' && *p <= '9')
        {
            switch (posn)
            {
                case 0:
                    hours = 10 * (*p - '0');
                    posn++;
                    break;
                case 1:
                    hours += *p - '0';
                    posn++;
                    break;
                case 2:
                    mins = 10 * (*p - '0');
                    posn++;
                    break;
                case 3:
                    mins += *p -'0';
                    posn++;
                    break;
            }
        }
        p++;
    }
    while (cycle < 4)
    {
        handleCycleDelim(program, cycle, on, hours, mins);
        cycle++;
        on = false;
        hours = 24;
        mins = 0;
    }
}



static bool checkProgramBeforeOverride()
{   
    if (RtcClockIsNotSet()) return false;

    struct tm tm;
    RtcGetTmLocal(&tm);
    
    int dayOfWeek = tm.tm_wday;
    
    int program = SettingsGetProgramDay(dayOfWeek);
    
    bool calling = false;
    for (int cycle = 0; cycle < 4; cycle++)
    {
        bool     on      = SettingsGetProgramCycleOn(program, cycle);
        unsigned minutes = SettingsGetProgramCycleMinutes(program, cycle);
        if (minutes >= 1440) continue;
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