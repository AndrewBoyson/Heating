#include "mbed.h"
#include "time.h"
#include  "rtc.h"
#include   "io.h"
#include  "log.h"

#define REG_COUNT 5

static union
{
    int GenRegs[REG_COUNT];
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
} schedules;

static void saveSchedules()
{
    for (int i = 0; i < REG_COUNT; i++) RtcSetGenReg(i, schedules.GenRegs[i]);
}
static void loadSchedules()
{
    for (int i = 0; i < REG_COUNT; i++) schedules.GenRegs[i] = RtcGetGenReg(i);
}
static void setCycle(int schedule, int cycle, bool on, unsigned minutes)
{    
    if (schedule == 0 && cycle == 0) { schedules.S0C0On = on; schedules.S0C0Min = minutes; }
    if (schedule == 0 && cycle == 1) { schedules.S0C1On = on; schedules.S0C1Min = minutes; }
    if (schedule == 0 && cycle == 2) { schedules.S0C2On = on; schedules.S0C2Min = minutes; }
    if (schedule == 0 && cycle == 3) { schedules.S0C3On = on; schedules.S0C3Min = minutes; }

    if (schedule == 1 && cycle == 0) { schedules.S1C0On = on; schedules.S1C0Min = minutes; }
    if (schedule == 1 && cycle == 1) { schedules.S1C1On = on; schedules.S1C1Min = minutes; }
    if (schedule == 1 && cycle == 2) { schedules.S1C2On = on; schedules.S1C2Min = minutes; }
    if (schedule == 1 && cycle == 3) { schedules.S1C3On = on; schedules.S1C3Min = minutes; }

    if (schedule == 2 && cycle == 0) { schedules.S2C0On = on; schedules.S2C0Min = minutes; }
    if (schedule == 2 && cycle == 1) { schedules.S2C1On = on; schedules.S2C1Min = minutes; }
    if (schedule == 2 && cycle == 2) { schedules.S2C2On = on; schedules.S2C2Min = minutes; }
    if (schedule == 2 && cycle == 3) { schedules.S2C3On = on; schedules.S2C3Min = minutes; }
}
static void getCycle(int schedule, int cycle, bool* pon, unsigned* pminutes)
{
    if (schedule == 0 && cycle == 0) { *pon = schedules.S0C0On; *pminutes = schedules.S0C0Min; }
    if (schedule == 0 && cycle == 1) { *pon = schedules.S0C1On; *pminutes = schedules.S0C1Min; }
    if (schedule == 0 && cycle == 2) { *pon = schedules.S0C2On; *pminutes = schedules.S0C2Min; }
    if (schedule == 0 && cycle == 3) { *pon = schedules.S0C3On; *pminutes = schedules.S0C3Min; }

    if (schedule == 1 && cycle == 0) { *pon = schedules.S1C0On; *pminutes = schedules.S1C0Min; }
    if (schedule == 1 && cycle == 1) { *pon = schedules.S1C1On; *pminutes = schedules.S1C1Min; }
    if (schedule == 1 && cycle == 2) { *pon = schedules.S1C2On; *pminutes = schedules.S1C2Min; }
    if (schedule == 1 && cycle == 3) { *pon = schedules.S1C3On; *pminutes = schedules.S1C3Min; }

    if (schedule == 2 && cycle == 0) { *pon = schedules.S2C0On; *pminutes = schedules.S2C0Min; }
    if (schedule == 2 && cycle == 1) { *pon = schedules.S2C1On; *pminutes = schedules.S2C1Min; }
    if (schedule == 2 && cycle == 2) { *pon = schedules.S2C2On; *pminutes = schedules.S2C2Min; }
    if (schedule == 2 && cycle == 3) { *pon = schedules.S2C3On; *pminutes = schedules.S2C3Min; }
}

//[*|-][00-23][00-59];
static int toStringSchedule(int schedule, int buflen, char* buffer)
{
    if (buflen < 25) return -1;
    char* p = buffer;
    for (int cycle = 0; cycle < 4; cycle++)
    {
        bool on;
        unsigned minutes;
        getCycle(schedule, cycle, &on, &minutes);
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
    return 0;
}
static void handleCycleDelim(int schedule, int cycle, bool on, unsigned hours, unsigned mins)
{
    unsigned total = hours * 60 + mins;
    if (total < 1440) setCycle(schedule, cycle, on, total);
    else              setCycle(schedule, cycle, 0, 1440);
}
static void parseSchedule(int schedule, char* text)
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

void ScheduleSave(int schedule, char* text)
{
    parseSchedule(schedule, text);
    saveSchedules();
}
int ScheduleRead(int schedule, int bufSize, char* text)
{
    return toStringSchedule(schedule, bufSize, text);
}
bool  ScheduleGetAuto()
{
    return schedules.Auto;
}
void ScheduleSetAuto(bool value)
{
    schedules.Auto = value;
    saveSchedules();
}

int  ScheduleGetMon()
{
    return schedules.Mon;
}
void ScheduleSetMon(int value)
{
    schedules.Mon = value;
    saveSchedules();
}
int  ScheduleGetTue()
{
    return schedules.Tue;
}
void ScheduleSetTue(int value)
{
    schedules.Tue = value;
    saveSchedules();
}
int  ScheduleGetWed()
{
    return schedules.Wed;
}
void ScheduleSetWed(int value)
{
    schedules.Wed = value;
    saveSchedules();
}
int  ScheduleGetThu()
{
    return schedules.Thu;
}
void ScheduleSetThu(int value)
{
    schedules.Thu = value;
    saveSchedules();
}
int  ScheduleGetFri()
{
    return schedules.Fri;
}
void ScheduleSetFri(int value)
{
    schedules.Fri = value;
    saveSchedules();
}
int  ScheduleGetSat()
{
    return schedules.Sat;
}
void ScheduleSetSat(int value)
{
    schedules.Sat = value;
    saveSchedules();
}
int  ScheduleGetSun()
{
    return schedules.Sun;
}
void ScheduleSetSun(int value)
{
    schedules.Sun = value;
    saveSchedules();
}


static bool checkScheduleBeforeOverride()
{   
    time_t now = time(NULL);
    struct tm tm;
    TimeToTmLocal(now, &tm);
    
    int dayOfWeek = tm.tm_wday;
    
    int schedule;
    
    switch(dayOfWeek)
    {
        case 0: schedule = schedules.Sun; break;
        case 1: schedule = schedules.Mon; break;
        case 2: schedule = schedules.Tue; break;
        case 3: schedule = schedules.Wed; break;
        case 4: schedule = schedules.Thu; break;
        case 5: schedule = schedules.Fri; break;
        case 6: schedule = schedules.Sat; break;
    }
    
    bool calling = false;
    for (int cycle = 0; cycle < 4; cycle++)
    {
        bool on;
        unsigned minutes;
        getCycle(schedule, cycle, &on, &minutes);
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
    if (schedules.Auto)
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
    loadSchedules();
    
    return 0;
}