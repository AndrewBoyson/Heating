#include     "mbed.h"
#include      "rtc.h"
#include  "rtc-clk.h"
#include       "io.h"
#include      "log.h"
#include     "fram.h"

#define PROGRAMS    3
#define TRANSITIONS 4

static char    programOverride;                 static int iOverride;
static char    programAuto;                     static int iAuto;
static char    programDay[7];                   static int iDay;
static int16_t programs[PROGRAMS][TRANSITIONS]; static int iPrograms;
static char    programNewDayHour;               static int iNewDayHour;


bool  ProgramGetOverride     ()             { return (bool)programOverride;        } 
bool  ProgramGetAuto         ()             { return (bool)programAuto;            } 
int   ProgramGetDay          (int i)        { return (int) programDay[i];          } 
int   ProgramGetNewDayHour   ()             { return (int) programNewDayHour;      }

void ProgramSetOverride      (              bool  value) { programOverride         = (char)   value;                                 FramWrite(iOverride,       1, &programOverride        ); }
void ProgramSetAuto          (              bool  value) { programAuto             = (char)   value;                                 FramWrite(iAuto,           1, &programAuto            ); }
void ProgramSetDay           (int i,        int   value) { programDay       [i]    = (char)   value;                                 FramWrite(iDay        + i, 1, &programDay       [i]   ); }
void ProgramSetNewDayHour    (              int   value) { programNewDayHour       = (char)   value;                                 FramWrite(iNewDayHour,     1, &programNewDayHour      ); }

int ProgramInit()
{
    char def1;
    int address;
    int programSize = PROGRAMS * TRANSITIONS * sizeof(int16_t);
    def1 = 0; address = FramLoad( 1,          &programOverride,   &def1); if (address < 0) return -1; iOverride   = address; 
    def1 = 0; address = FramLoad( 1,          &programAuto,       &def1); if (address < 0) return -1; iAuto       = address; 
              address = FramLoad( 7,           programDay,         NULL); if (address < 0) return -1; iDay        = address;
              address = FramLoad(programSize,  programs,           NULL); if (address < 0) return -1; iPrograms   = address; //3 x 4 x 2
    def1 = 2; address = FramLoad(1,           &programNewDayHour, &def1); if (address < 0) return -1; iNewDayHour = address;
    return 0;
}

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
static int16_t encodeTransition(bool inuse, bool onoff, int minutes)
{
    int16_t    transition  = minutes;
               transition &= 0x07FF;
    if (onoff) transition |= 0x0800;
    if (inuse) transition |= 0x1000;
    return transition;
}
static void decodeTransition(int16_t transition, bool* pinuse, bool* ponoff, int* pminutes)
{
    *pinuse   = transition & 0x1000;
    *ponoff   = transition & 0x0800;
    *pminutes = transition & 0x07FF;
}

static int compareTransition (const void * a, const void * b) //-ve a goes before b; 0 same; +ve a goes after b
{
    bool  inUseA,   inUseB;
    bool     onA,      onB;
    int minutesA, minutesB;
    decodeTransition(*(int16_t*)a, &inUseA, &onA, &minutesA);
    decodeTransition(*(int16_t*)b, &inUseB, &onB, &minutesB);
    
    if (!inUseA && !inUseB) return  0;
    if (!inUseA)            return +1;
    if (!inUseB)            return -1;
    
    if (minutesA < programNewDayHour * 60) minutesA += 1440;
    if (minutesB < programNewDayHour * 60) minutesB += 1440;
    
    if (minutesA < minutesB) return -1;
    if (minutesA > minutesB) return +1;
    return 0;
}
static void sort(int16_t* pProgram)
{
    qsort (pProgram, 4, sizeof(int16_t), compareTransition);
}

//[+|-][00-23][00-59];
void ProgramToString(int program, int buflen, char* buffer)
{
    if (buflen < 25) return;
    char* p = buffer;
    for (int i = 0; i < TRANSITIONS; i++)
    {
        int16_t transition = programs[program][i];
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
    
    int16_t transition = encodeTransition(*pInUse, *pOn, minutes);
    programs[program][*pIndex] = transition;
    
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
    while (*p && i < TRANSITIONS) 
    {
        if      (*p == '+')              { on = true ; }
        else if (*p == '-')              { on = false; }
        else if (*p >= '0' && *p <= '9') { inUse = true; hourTens = hourUnits; hourUnits = minuteTens; minuteTens = minuteUnits; minuteUnits = *p - '0'; }
        else if (*p == ' ')              { handleParseDelim(program, &i, &inUse, &on, &hourTens, &hourUnits, &minuteTens, &minuteUnits); }
        p++;
    }
    while (i < TRANSITIONS) handleParseDelim(program, &i, &inUse, &on, &hourTens, &hourUnits, &minuteTens, &minuteUnits);
    sort(&programs[program][0]);
    FramWrite(iPrograms + program * 8, 8, &programs[program]);
}

static bool checkProgramBeforeOverride()
{   

    if (RtcClockIsNotSet()) return false;

    struct tm tm;
    RtcGetTmLocal(&tm);
    
    int dayOfWeek = tm.tm_wday;
    int minutesNow = tm.tm_hour * 60 + tm.tm_min;
    if (tm.tm_hour < programNewDayHour) //Before 2am should be matched against yesterday's program.
    {
        dayOfWeek--;
        if (dayOfWeek < 0) dayOfWeek = 6;
    }
    
    int program = programDay[dayOfWeek];
    
    bool calling = false;
    for (int i = 0; i < TRANSITIONS; i++)
    {
        int16_t transition = programs[program][i];
        bool  inuse;
        bool  on;
        int   minutes;
        decodeTransition(transition, &inuse, &on, &minutes);
        if (!inuse) continue;
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
    if (programAuto)
    {
        int programBeforeOverride = checkProgramBeforeOverride();
        
        //Reset override whenever the program calling changes
        if (programBeforeOverrideHasChanged(programBeforeOverride)) ProgramSetOverride(false);
        
        //Call for heating
        if (programOverride) ProgramIsCalling = !programBeforeOverride;
        else                 ProgramIsCalling =  programBeforeOverride;
    }
    else
    {
        ProgramSetOverride(false);
        ProgramIsCalling = false;
    }
    
    return 0;
}
