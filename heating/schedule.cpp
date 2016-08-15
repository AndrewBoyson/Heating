#include     "mbed.h"
#include      "rtc.h"
#include "settings.h"
#include       "io.h"
#include      "log.h"

static union
{
    int GenRegs[SETTINGS_SCHEDULE_REG_COUNT];
    __packed struct
    {
        unsigned Auto    :  1;
        
        unsigned Mon     :  2;
        unsigned Tue     :  2;
        unsigned Wed     :  2;
        unsigned Thu     :  2;
        unsigned Fri     :  2;
        unsigned Sat     :  2;
        unsigned Sun     :  2;
        
        unsigned S0C0On  :  1;
        unsigned S0C0Min : 11;
        
        unsigned S0C1On  :  1;
        unsigned S0C1Min : 11;
        
        unsigned S0C2On  :  1;
        unsigned S0C2Min : 11;
        
        unsigned S0C3On  :  1;
        unsigned S0C3Min : 11;
        
        unsigned S1C0On  :  1;
        unsigned S1C0Min : 11;
        
        unsigned S1C1On  :  1;
        unsigned S1C1Min : 11;
        
        unsigned S1C2On  :  1;
        unsigned S1C2Min : 11;
        
        unsigned S1C3On  :  1;
        unsigned S1C3Min : 11;
        
        unsigned S2C0On  :  1;
        unsigned S2C0Min : 11;
        
        unsigned S2C1On  :  1;
        unsigned S2C1Min : 11;
        
        unsigned S2C2On  :  1;
        unsigned S2C2Min : 11;
        
        unsigned S2C3On  :  1;
        unsigned S2C3Min : 11;
        
        unsigned         :  1;
    } ;
} schedulesUnion;

bool ScheduleAuto;

int ScheduleDay[7];

bool ScheduleCycleOn[3][4];
int  ScheduleCycleMinutes[3][4];

void ScheduleSaveAll()
{
    schedulesUnion.Auto = ScheduleAuto;
        
    schedulesUnion.Mon = ScheduleDay[1];
    schedulesUnion.Tue = ScheduleDay[2];
    schedulesUnion.Wed = ScheduleDay[3];
    schedulesUnion.Thu = ScheduleDay[4];
    schedulesUnion.Fri = ScheduleDay[5];
    schedulesUnion.Sat = ScheduleDay[6];
    schedulesUnion.Sun = ScheduleDay[0];
    
    schedulesUnion.S0C0On = ScheduleCycleOn[0][0]; schedulesUnion.S0C0Min = ScheduleCycleMinutes[0][0];
    schedulesUnion.S0C1On = ScheduleCycleOn[0][1]; schedulesUnion.S0C1Min = ScheduleCycleMinutes[0][1];
    schedulesUnion.S0C2On = ScheduleCycleOn[0][2]; schedulesUnion.S0C2Min = ScheduleCycleMinutes[0][2];
    schedulesUnion.S0C3On = ScheduleCycleOn[0][3]; schedulesUnion.S0C3Min = ScheduleCycleMinutes[0][3];

    schedulesUnion.S1C0On = ScheduleCycleOn[1][0]; schedulesUnion.S1C0Min = ScheduleCycleMinutes[1][0];
    schedulesUnion.S1C1On = ScheduleCycleOn[1][1]; schedulesUnion.S1C1Min = ScheduleCycleMinutes[1][1];
    schedulesUnion.S1C2On = ScheduleCycleOn[1][2]; schedulesUnion.S1C2Min = ScheduleCycleMinutes[1][2];
    schedulesUnion.S1C3On = ScheduleCycleOn[1][3]; schedulesUnion.S1C3Min = ScheduleCycleMinutes[1][3];

    schedulesUnion.S2C0On = ScheduleCycleOn[2][0]; schedulesUnion.S2C0Min = ScheduleCycleMinutes[2][0];
    schedulesUnion.S2C1On = ScheduleCycleOn[2][1]; schedulesUnion.S2C1Min = ScheduleCycleMinutes[2][1];
    schedulesUnion.S2C2On = ScheduleCycleOn[2][2]; schedulesUnion.S2C2Min = ScheduleCycleMinutes[2][2];
    schedulesUnion.S2C3On = ScheduleCycleOn[2][3]; schedulesUnion.S2C3Min = ScheduleCycleMinutes[2][3];
    
    for (int i = 0; i < SETTINGS_SCHEDULE_REG_COUNT; i++) SettingsSetScheduleReg(i, schedulesUnion.GenRegs[i]);
}
void ScheduleLoadAll()
{
    for (int i = 0; i < SETTINGS_SCHEDULE_REG_COUNT; i++) schedulesUnion.GenRegs[i] = SettingsGetScheduleReg(i);
    
    ScheduleAuto = schedulesUnion.Auto;
        
    ScheduleDay[1] = schedulesUnion.Mon;
    ScheduleDay[2] = schedulesUnion.Tue;
    ScheduleDay[3] = schedulesUnion.Wed;
    ScheduleDay[4] = schedulesUnion.Thu;
    ScheduleDay[5] = schedulesUnion.Fri;
    ScheduleDay[6] = schedulesUnion.Sat;
    ScheduleDay[0] = schedulesUnion.Sun;
    
    ScheduleCycleOn[0][0] = schedulesUnion.S0C0On; ScheduleCycleMinutes[0][0] = schedulesUnion.S0C0Min;
    ScheduleCycleOn[0][1] = schedulesUnion.S0C1On; ScheduleCycleMinutes[0][1] = schedulesUnion.S0C1Min;
    ScheduleCycleOn[0][2] = schedulesUnion.S0C2On; ScheduleCycleMinutes[0][2] = schedulesUnion.S0C2Min;
    ScheduleCycleOn[0][3] = schedulesUnion.S0C3On; ScheduleCycleMinutes[0][3] = schedulesUnion.S0C3Min;

    ScheduleCycleOn[1][0] = schedulesUnion.S1C0On; ScheduleCycleMinutes[1][0] = schedulesUnion.S1C0Min;
    ScheduleCycleOn[1][1] = schedulesUnion.S1C1On; ScheduleCycleMinutes[1][1] = schedulesUnion.S1C1Min;
    ScheduleCycleOn[1][2] = schedulesUnion.S1C2On; ScheduleCycleMinutes[1][2] = schedulesUnion.S1C2Min;
    ScheduleCycleOn[1][3] = schedulesUnion.S1C3On; ScheduleCycleMinutes[1][3] = schedulesUnion.S1C3Min;

    ScheduleCycleOn[2][0] = schedulesUnion.S2C0On; ScheduleCycleMinutes[2][0] = schedulesUnion.S2C0Min;
    ScheduleCycleOn[2][1] = schedulesUnion.S2C1On; ScheduleCycleMinutes[2][1] = schedulesUnion.S2C1Min;
    ScheduleCycleOn[2][2] = schedulesUnion.S2C2On; ScheduleCycleMinutes[2][2] = schedulesUnion.S2C2Min;
    ScheduleCycleOn[2][3] = schedulesUnion.S2C3On; ScheduleCycleMinutes[2][3] = schedulesUnion.S2C3Min;
    
}

//[*|-][00-23][00-59];
void ScheduleToString(int schedule, int buflen, char* buffer)
{
    if (buflen < 25) return;
    char* p = buffer;
    for (int cycle = 0; cycle < 4; cycle++)
    {
        bool     on      = ScheduleCycleOn[schedule][cycle];
        unsigned minutes = ScheduleCycleMinutes[schedule][cycle];
        unsigned hours = minutes / 60;
        if (hours > 23) continue;
        unsigned tensOfHours = hours / 10;
        hours %= 10;
        minutes %= 60;
        unsigned tensOfMinutes = minutes / 10;
        minutes %= 10;
        if (p > buffer) *p++ = ' ';
        *p++ = on ? '*' : '-';
        *p++ = tensOfHours + '0';
        *p++ = hours + '0';
        *p++ = tensOfMinutes + '0';
        *p++ = minutes + '0';
    }
    *p = 0;
}
static void handleCycleDelim(int schedule, int cycle, bool on, unsigned hours, unsigned mins)
{
    unsigned total = hours * 60 + mins;
    if (total < 1440)
    {
        ScheduleCycleOn[schedule][cycle] = on;
        ScheduleCycleMinutes[schedule][cycle] = total;
    }
    else
    {
        ScheduleCycleOn[schedule][cycle] = false;
        ScheduleCycleMinutes[schedule][cycle] = 1440;
    }
}
void ScheduleParse(int schedule, char* text)
{
    int cycle = 0;
    bool on = false;
    unsigned hours = 24;
    unsigned mins = 0;
    int posn = 0;
    char *p = text;
    while (*p) 
    {
        if (*p == '*')
        {
            on = true;
            posn = 0;
        }
        else if (*p == '-')
        {
            on = false;
            posn = 0;
        }
        else if (*p == '+')
        {
            handleCycleDelim(schedule, cycle, on, hours, mins);
            cycle++;
            on = false;
            hours = 24;
            mins = 0;
            posn = 0;
        }
        else if (*p == '%')
        {
            *p++;
            *p++;
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
        handleCycleDelim(schedule, cycle, on, hours, mins);
        cycle++;
        on = false;
        hours = 24;
        mins = 0;
    }
}



static bool checkScheduleBeforeOverride()
{   
    struct tm tm;
    RtcGetTmLocal(&tm);
    
    int dayOfWeek = tm.tm_wday;
    
    int schedule = ScheduleDay[dayOfWeek];
    
    bool calling = false;
    for (int cycle = 0; cycle < 4; cycle++)
    {
        bool     on      = ScheduleCycleOn[schedule][cycle];
        unsigned minutes = ScheduleCycleMinutes[schedule][cycle];
        if (minutes >= 1440) continue;
        int minutesNow = tm.tm_hour * 60 + tm.tm_min;
        if (minutes <= minutesNow) calling = on;
    }
    
    return calling;
}

static bool scheduleBeforeOverrideHasChanged(bool isOn)
{
    static bool wasOn = false;
    bool changed = wasOn != isOn;
    wasOn = isOn;
    return changed;
}

bool ScheduleOverride;
bool ScheduleIsCalling;

int ScheduleMain()
{    
    if (ScheduleAuto)
    {
        int scheduleBeforeOverride = checkScheduleBeforeOverride();
        
        //Reset override whenever the schedule calling changes
        if (scheduleBeforeOverrideHasChanged(scheduleBeforeOverride)) ScheduleOverride = false;
        
        //Call for heating
        if (ScheduleOverride) ScheduleIsCalling = !scheduleBeforeOverride;
        else                  ScheduleIsCalling =  scheduleBeforeOverride;
    }
    else
    {
        ScheduleOverride = false;
        ScheduleIsCalling = false;
    }
    
    return 0;
}
int ScheduleInit()
{
    ScheduleOverride = false;
    ScheduleLoadAll();
    
    return 0;
}