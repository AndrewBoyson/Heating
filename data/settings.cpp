#include "mbed.h"
#include  "log.h"
#include   "io.h"
#include "fram.h"

#define PROGRAMS                3
#define TRANSITIONS_PER_PROGRAM 4
#define TRANSITIONS             PROGRAMS * TRANSITIONS_PER_PROGRAM

/*
ALSEC   6 (  64) Alarm value for Seconds       
ALMIN   6 (  64) Alarm value for Minutes
ALHOUR  5 (  32) Alarm value for Hours         
ALDOM   5 (  32) Alarm value for Day of Month  
ALDOW   3 (   8) Alarm value for Day of Week
ALDOY   9 ( 512) Alarm value for Day of Year   
ALMON   4 (  16) Alarm value for Months        WatchdogPosition
ALYEAR 12 (4096) Alarm value for Years         RtcBaseFraction
Total  50
*/

int  SettingsGetMainPosition() { return LPC_RTC->ALMON;     }
int  SettingsGetRtcFraction () { return LPC_RTC->ALYEAR;    }

void SettingsSetMainPosition(int value) { if (value >   15) value =   15; if (value <   0) value =   0; LPC_RTC->ALMON  = value;     }
void SettingsSetRtcFraction (int value) { if (value > 4095) value = 4095; if (value <   0) value =   0; LPC_RTC->ALYEAR = value;     }


static void ipBinToStr(char* bytes, char* text)
{
    snprintf(text, 16, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
}

static void ipStrToBin(char* text, char* bytes)
{
    int ints[4];
    sscanf(text, "%d.%d.%d.%d", &ints[3], &ints[2], &ints[1], &ints[0]);
    bytes[3] = ints[3] & 0xFF;
    bytes[2] = ints[2] & 0xFF;
    bytes[1] = ints[1] & 0xFF;
    bytes[0] = ints[0] & 0xFF;
}

static char     clockNtpIp[16];         static int iClockNtpIp;

static int32_t clockInitialInterval;    static int iClockInitialInterval;
static int32_t clockNormalInterval;     static int iClockNormalInterval;
static int32_t clockRetryInterval;      static int iClockRetryInterval;
static int32_t clockOffsetMs;           static int iClockOffsetMs;
static int32_t clockNtpMaxDelayMs;      static int iClockNtpMaxDelayMs;
static int32_t clockCalDivisor;         static int iClockCalDivisor;

static char    tankRom[8];              static int iTankRom;
static char    boilerOutputRom[8];      static int iBoilerOutputRom;
static char    boilerReturnRom[8];      static int iBoilerReturnRom;
static char    hallRom[8];              static int iHallRom;

static int32_t tankSetPoint;            static int iTankSetPoint;
static int32_t tankHysteresis;          static int iTankHysteresis;
static int32_t boilerRunOnResidual;     static int iBoilerRunOnResidual;
static int32_t boilerRunOnTime;         static int iBoilerRunOnTime;
static int32_t nightTemperature;        static int iNightTemperature;
static int32_t frostTemperature;        static int iFrostTemperature;

static char    programOverride;         static int iProgramOverride;
static char    programAuto;             static int iProgramAuto;
static char    programDay[7];           static int iProgramDay;
static int16_t programTransition[3][4]; static int iProgramTransition;

extern const int SettingsProgramCount           = PROGRAMS;
extern const int SettingsProgramTransitionCount = TRANSITIONS_PER_PROGRAM;

char* SettingsGetClockNtpIp          ()             { return       clockNtpIp;             } 
int   SettingsGetClockInitialInterval()             { return (int) clockInitialInterval;   } 
int   SettingsGetClockNormalInterval ()             { return (int) clockNormalInterval;    } 
int   SettingsGetClockRetryInterval  ()             { return (int) clockRetryInterval;     } 
int   SettingsGetClockOffsetMs       ()             { return (int) clockOffsetMs;          } 
int   SettingsGetClockNtpMaxDelayMs  ()             { return (int) clockNtpMaxDelayMs;     } 
int   SettingsGetClockCalDivisor     ()             { return (int) clockCalDivisor;        } 

char* SettingsGetTankRom             ()             { return       tankRom;                } 
char* SettingsGetBoilerOutputRom     ()             { return       boilerOutputRom;        } 
char* SettingsGetBoilerReturnRom     ()             { return       boilerReturnRom;        } 
char* SettingsGetHallRom             ()             { return       hallRom;                } 

int   SettingsGetTankSetPoint        ()             { return (int) tankSetPoint;           } 
int   SettingsGetTankHysteresis      ()             { return (int) tankHysteresis;         } 
int   SettingsGetBoilerRunOnResidual ()             { return (int) boilerRunOnResidual;    } 
int   SettingsGetBoilerRunOnTime     ()             { return (int) boilerRunOnTime;        } 
int   SettingsGetNightTemperature    ()             { return (int) nightTemperature;       } 
int   SettingsGetFrostTemperature    ()             { return (int) frostTemperature;       } 

bool  SettingsGetProgramOverride     ()             { return (bool)programOverride;        } 
bool  SettingsGetProgramAuto         ()             { return (bool)programAuto;            } 
int   SettingsGetProgramDay          (int i)        { return (int) programDay[i];          } 
short SettingsGetProgramTransition   (int i, int j) { return (int) programTransition[i][j];} 

void SettingsSetClockNtpIp           (              char *value) { strncpy(clockNtpIp, value, 16); char bin[4]; ipStrToBin(clockNtpIp, bin); FramWrite(iClockNtpIp,            4, bin                     ); }
void SettingsSetClockInitialInterval (              int   value) { clockInitialInterval    = (int32_t)value;                                 FramWrite(iClockInitialInterval,  4, &clockInitialInterval   ); }
void SettingsSetClockNormalInterval  (              int   value) { clockNormalInterval     = (int32_t)value;                                 FramWrite(iClockNormalInterval,   4, &clockNormalInterval    ); }
void SettingsSetClockRetryInterval   (              int   value) { clockRetryInterval      = (int32_t)value;                                 FramWrite(iClockRetryInterval,    4, &clockRetryInterval     ); }
void SettingsSetClockOffsetMs        (              int   value) { clockOffsetMs           = (int32_t)value;                                 FramWrite(iClockOffsetMs,         4, &clockOffsetMs          ); }
void SettingsSetClockNtpMaxDelayMs   (              int   value) { clockNtpMaxDelayMs      = (int32_t)value;                                 FramWrite(iClockNtpMaxDelayMs,    4, &clockNtpMaxDelayMs     ); }
void SettingsSetClockCalDivisor      (              int   value) { clockCalDivisor         = (int32_t)value;                                 FramWrite(iClockCalDivisor,       4, &clockCalDivisor        ); }

void SettingsSetTankRom              (              char* value) { memcpy(tankRom,         value, 8);                                        FramWrite(iTankRom,               8,  tankRom                ); }
void SettingsSetBoilerOutputRom      (              char* value) { memcpy(boilerOutputRom, value, 8);                                        FramWrite(iBoilerOutputRom,       8,  boilerOutputRom        ); }
void SettingsSetBoilerReturnRom      (              char* value) { memcpy(boilerReturnRom, value, 8);                                        FramWrite(iBoilerReturnRom,       8,  boilerReturnRom        ); }
void SettingsSetHallRom              (              char* value) { memcpy(hallRom,         value, 8);                                        FramWrite(iHallRom,               8,  hallRom                ); }

void SettingsSetTankSetPoint         (              int   value) { tankSetPoint            = (int32_t)value;                                 FramWrite(iTankSetPoint,          4, &tankSetPoint           ); }
void SettingsSetTankHysteresis       (              int   value) { tankHysteresis          = (int32_t)value;                                 FramWrite(iTankHysteresis,        4, &tankHysteresis         ); }
void SettingsSetBoilerRunOnResidual  (              int   value) { boilerRunOnResidual     = (int32_t)value;                                 FramWrite(iBoilerRunOnResidual,   4, &boilerRunOnResidual    ); }
void SettingsSetBoilerRunOnTime      (              int   value) { boilerRunOnTime         = (int32_t)value;                                 FramWrite(iBoilerRunOnTime,       4, &boilerRunOnTime        ); }
void SettingsSetNightTemperature     (              int   value) { nightTemperature        = (int32_t)value;                                 FramWrite(iNightTemperature,      4, &nightTemperature       ); }
void SettingsSetFrostTemperature     (              int   value) { frostTemperature        = (int32_t)value;                                 FramWrite(iFrostTemperature,      4, &frostTemperature       ); }

void SettingsSetProgramOverride      (              bool  value) { programOverride         = (char)   value;                                 FramWrite(iProgramOverride,       1, &programOverride        ); }
void SettingsSetProgramAuto          (              bool  value) { programAuto             = (char)   value;                                 FramWrite(iProgramAuto,           1, &programAuto            ); }
void SettingsSetProgramDay           (int i,        int   value) { programDay       [i]    = (char)   value;                                 FramWrite(iProgramDay        + i, 1, &programDay       [i]   ); }
void SettingsSetProgramTransition    (int i, int j, short value) { programTransition[i][j] = (int16_t)value; int k = 2*(i*4 + j);            FramWrite(iProgramTransition + k, 2, &programTransition[i][j]); }

int  SettingsInit()
{
    int address;
    char    bin[4];
    int32_t def4;
    char    def1;
    
                address = FramLoad( 8,               tankRom,               NULL); if (address < 0) return -1; iTankRom              = address;
                address = FramLoad( 8,               boilerOutputRom,       NULL); if (address < 0) return -1; iBoilerOutputRom      = address;
                address = FramLoad( 8,               boilerReturnRom,       NULL); if (address < 0) return -1; iBoilerReturnRom      = address;
                address = FramLoad( 8,               hallRom,               NULL); if (address < 0) return -1; iHallRom              = address;
    
                address = FramLoad( 4,               bin,                   NULL); if (address < 0) return -1; iClockNtpIp           = address; ipBinToStr(bin, clockNtpIp);
    def4 =   1; address = FramLoad( 4,              &clockInitialInterval, &def4); if (address < 0) return -1; iClockInitialInterval = address;
    def4 = 600; address = FramLoad( 4,              &clockNormalInterval,  &def4); if (address < 0) return -1; iClockNormalInterval  = address;
    def4 =  60; address = FramLoad( 4,              &clockRetryInterval,   &def4); if (address < 0) return -1; iClockRetryInterval   = address; 
    def4 =   0; address = FramLoad( 4,              &clockOffsetMs,        &def4); if (address < 0) return -1; iClockOffsetMs        = address; 
    def4 =  50; address = FramLoad( 4,              &clockNtpMaxDelayMs,   &def4); if (address < 0) return -1; iClockNtpMaxDelayMs   = address; 
    def4 =  16; address = FramLoad( 4,              &clockCalDivisor,      &def4); if (address < 0) return -1; iClockCalDivisor      = address; 
    
    def4 =  80; address = FramLoad( 4,              &tankSetPoint,         &def4); if (address < 0) return -1; iTankSetPoint         = address; 
    def4 =   5; address = FramLoad( 4,              &tankHysteresis,       &def4); if (address < 0) return -1; iTankHysteresis       = address; 
    def4 =   2; address = FramLoad( 4,              &boilerRunOnResidual,  &def4); if (address < 0) return -1; iBoilerRunOnResidual  = address; 
    def4 = 360; address = FramLoad( 4,              &boilerRunOnTime,      &def4); if (address < 0) return -1; iBoilerRunOnTime      = address; 
    def4 =  15; address = FramLoad( 4,              &nightTemperature,     &def4); if (address < 0) return -1; iNightTemperature     = address; 
    def4 =   8; address = FramLoad( 4,              &frostTemperature,     &def4); if (address < 0) return -1; iFrostTemperature     = address; 
    
    def1 =   0; address = FramLoad( 1,              &programOverride,      &def1); if (address < 0) return -1; iProgramOverride      = address; 
    def1 =   0; address = FramLoad( 1,              &programAuto,          &def1); if (address < 0) return -1; iProgramAuto          = address; 
                address = FramLoad( 7,               programDay,            NULL); if (address < 0) return -1; iProgramDay           = address;
                address = FramLoad(TRANSITIONS * 2,  programTransition,     NULL); if (address < 0) return -1; iProgramTransition    = address; //3 x 4 x 2
    return 0;
}