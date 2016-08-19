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
        
        unsigned P0C0On  :  1;
        unsigned P0C0Min : 11;
        
        unsigned P0C1On  :  1;
        unsigned P0C1Min : 11;
        
        unsigned P0C2On  :  1;
        unsigned P0C2Min : 11;
        
        unsigned P0C3On  :  1;
        unsigned P0C3Min : 11;
        
        unsigned P1C0On  :  1;
        unsigned P1C0Min : 11;
        
        unsigned P1C1On  :  1;
        unsigned P1C1Min : 11;
        
        unsigned P1C2On  :  1;
        unsigned P1C2Min : 11;
        
        unsigned P1C3On  :  1;
        unsigned P1C3Min : 11;
        
        unsigned P2C0On  :  1;
        unsigned P2C0Min : 11;
        
        unsigned P2C1On  :  1;
        unsigned P2C1Min : 11;
        
        unsigned P2C2On  :  1;
        unsigned P2C2Min : 11;
        
        unsigned P2C3On  :  1;
        unsigned P2C3Min : 11;
        
        unsigned         :  1;
    } ;
} programsUnion;

bool ProgramAuto;

int ProgramDay[7];

bool ProgramCycleOn[3][4];
int  ProgramCycleMinutes[3][4];

void ProgramSaveAll()
{
    programsUnion.Auto = ProgramAuto;
        
    programsUnion.Mon = ProgramDay[1];
    programsUnion.Tue = ProgramDay[2];
    programsUnion.Wed = ProgramDay[3];
    programsUnion.Thu = ProgramDay[4];
    programsUnion.Fri = ProgramDay[5];
    programsUnion.Sat = ProgramDay[6];
    programsUnion.Sun = ProgramDay[0];
    
    programsUnion.P0C0On = ProgramCycleOn[0][0]; programsUnion.P0C0Min = ProgramCycleMinutes[0][0];
    programsUnion.P0C1On = ProgramCycleOn[0][1]; programsUnion.P0C1Min = ProgramCycleMinutes[0][1];
    programsUnion.P0C2On = ProgramCycleOn[0][2]; programsUnion.P0C2Min = ProgramCycleMinutes[0][2];
    programsUnion.P0C3On = ProgramCycleOn[0][3]; programsUnion.P0C3Min = ProgramCycleMinutes[0][3];

    programsUnion.P1C0On = ProgramCycleOn[1][0]; programsUnion.P1C0Min = ProgramCycleMinutes[1][0];
    programsUnion.P1C1On = ProgramCycleOn[1][1]; programsUnion.P1C1Min = ProgramCycleMinutes[1][1];
    programsUnion.P1C2On = ProgramCycleOn[1][2]; programsUnion.P1C2Min = ProgramCycleMinutes[1][2];
    programsUnion.P1C3On = ProgramCycleOn[1][3]; programsUnion.P1C3Min = ProgramCycleMinutes[1][3];

    programsUnion.P2C0On = ProgramCycleOn[2][0]; programsUnion.P2C0Min = ProgramCycleMinutes[2][0];
    programsUnion.P2C1On = ProgramCycleOn[2][1]; programsUnion.P2C1Min = ProgramCycleMinutes[2][1];
    programsUnion.P2C2On = ProgramCycleOn[2][2]; programsUnion.P2C2Min = ProgramCycleMinutes[2][2];
    programsUnion.P2C3On = ProgramCycleOn[2][3]; programsUnion.P2C3Min = ProgramCycleMinutes[2][3];
    
    for (int i = 0; i < SETTINGS_SCHEDULE_REG_COUNT; i++) SettingsSetScheduleReg(i, programsUnion.GenRegs[i]);
}
void ProgramLoadAll()
{
    for (int i = 0; i < SETTINGS_SCHEDULE_REG_COUNT; i++) programsUnion.GenRegs[i] = SettingsGetScheduleReg(i);
    
    ProgramAuto = programsUnion.Auto;
        
    ProgramDay[1] = programsUnion.Mon;
    ProgramDay[2] = programsUnion.Tue;
    ProgramDay[3] = programsUnion.Wed;
    ProgramDay[4] = programsUnion.Thu;
    ProgramDay[5] = programsUnion.Fri;
    ProgramDay[6] = programsUnion.Sat;
    ProgramDay[0] = programsUnion.Sun;
    
    ProgramCycleOn[0][0] = programsUnion.P0C0On; ProgramCycleMinutes[0][0] = programsUnion.P0C0Min;
    ProgramCycleOn[0][1] = programsUnion.P0C1On; ProgramCycleMinutes[0][1] = programsUnion.P0C1Min;
    ProgramCycleOn[0][2] = programsUnion.P0C2On; ProgramCycleMinutes[0][2] = programsUnion.P0C2Min;
    ProgramCycleOn[0][3] = programsUnion.P0C3On; ProgramCycleMinutes[0][3] = programsUnion.P0C3Min;

    ProgramCycleOn[1][0] = programsUnion.P1C0On; ProgramCycleMinutes[1][0] = programsUnion.P1C0Min;
    ProgramCycleOn[1][1] = programsUnion.P1C1On; ProgramCycleMinutes[1][1] = programsUnion.P1C1Min;
    ProgramCycleOn[1][2] = programsUnion.P1C2On; ProgramCycleMinutes[1][2] = programsUnion.P1C2Min;
    ProgramCycleOn[1][3] = programsUnion.P1C3On; ProgramCycleMinutes[1][3] = programsUnion.P1C3Min;

    ProgramCycleOn[2][0] = programsUnion.P2C0On; ProgramCycleMinutes[2][0] = programsUnion.P2C0Min;
    ProgramCycleOn[2][1] = programsUnion.P2C1On; ProgramCycleMinutes[2][1] = programsUnion.P2C1Min;
    ProgramCycleOn[2][2] = programsUnion.P2C2On; ProgramCycleMinutes[2][2] = programsUnion.P2C2Min;
    ProgramCycleOn[2][3] = programsUnion.P2C3On; ProgramCycleMinutes[2][3] = programsUnion.P2C3Min;
    
}

//[*|-][00-23][00-59];
void ProgramToString(int program, int buflen, char* buffer)
{
    if (buflen < 25) return;
    char* p = buffer;
    for (int cycle = 0; cycle < 4; cycle++)
    {
        bool     on      = ProgramCycleOn[program][cycle];
        unsigned minutes = ProgramCycleMinutes[program][cycle];
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
static void handleCycleDelim(int program, int cycle, bool on, unsigned hours, unsigned mins)
{
    unsigned total = hours * 60 + mins;
    if (total < 1440)
    {
        ProgramCycleOn[program][cycle] = on;
        ProgramCycleMinutes[program][cycle] = total;
    }
    else
    {
        ProgramCycleOn[program][cycle] = false;
        ProgramCycleMinutes[program][cycle] = 1440;
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
            handleCycleDelim(program, cycle, on, hours, mins);
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
        handleCycleDelim(program, cycle, on, hours, mins);
        cycle++;
        on = false;
        hours = 24;
        mins = 0;
    }
}



static bool checkProgramBeforeOverride()
{   
    struct tm tm;
    RtcGetTmLocal(&tm);
    
    int dayOfWeek = tm.tm_wday;
    
    int program = ProgramDay[dayOfWeek];
    
    bool calling = false;
    for (int cycle = 0; cycle < 4; cycle++)
    {
        bool     on      = ProgramCycleOn[program][cycle];
        unsigned minutes = ProgramCycleMinutes[program][cycle];
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

bool ProgramOverride;
bool ProgramIsCalling;

int ProgramMain()
{    
    if (ProgramAuto)
    {
        int programBeforeOverride = checkProgramBeforeOverride();
        
        //Reset override whenever the program calling changes
        if (programBeforeOverrideHasChanged(programBeforeOverride)) ProgramOverride = false;
        
        //Call for heating
        if (ProgramOverride) ProgramIsCalling = !programBeforeOverride;
        else                 ProgramIsCalling =  programBeforeOverride;
    }
    else
    {
        ProgramOverride = false;
        ProgramIsCalling = false;
    }
    
    return 0;
}
int ProgramInit()
{
    ProgramOverride = false;
    ProgramLoadAll();
    
    return 0;
}