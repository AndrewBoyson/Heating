#include "mbed.h"
#include  "rtc.h"
#include   "io.h"
#include  "log.h"

static union
{
    int GenRegs[5];
    __packed struct
    {
        unsigned On      :  1;
        
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

void saveSchedules()
{
    for (int i = 0; i < 5; i++) RtcSetGenReg(i, schedules.GenRegs[i]);
}
void readSchedules()
{
    for (int i = 0; i < 5; i++) schedules.GenRegs[i] = RtcGetGenReg(i);
}
void setCycle(int schedule, int cycle, int on, unsigned minutes)
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
void getCycle(int schedule, int cycle, int* pon, unsigned* pminutes)
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
    char* p = buffer;
    for (int cycle = 0; cycle < 4; cycle++)
    {
        int on;
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
    return p - buffer;
}
static void handleCycleDelim(int schedule, int cycle, int on, unsigned hours, unsigned mins)
{
    unsigned total = hours * 60 + mins;
    if (total < 1440) setCycle(schedule, cycle, on, total);
    else              setCycle(schedule, cycle, 0, 1440);
}
static void parseSchedule(int schedule, char* text)
{
    int cycle = 0;
    int on = false;
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

void HeatingScheduleSave(int schedule, char* text)
{
    readSchedules();
    parseSchedule(schedule, text);
    saveSchedules();
}
int HeatingScheduleRead(int schedule, int bufSize, char* text)
{
    readSchedules();
    return toStringSchedule(schedule, bufSize, text);
}