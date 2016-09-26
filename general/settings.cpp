#include "mbed.h"
#include  "log.h"
#include   "io.h"

int SettingsGetScheduleReg(int index)
{
    switch (index)
    {
        case 0: return LPC_RTC->GPREG0;
        case 1: return LPC_RTC->GPREG1;
        case 2: return LPC_RTC->GPREG2;
        case 3: return LPC_RTC->GPREG3;
        case 4: return LPC_RTC->GPREG4;
        default:
            LogF("Unknown SettingsGetGenReg index %d\r\n", index);
            return 0;
    }
}
void SettingsSetScheduleReg(int index, int value)
{
    switch (index)
    {
        case 0: LPC_RTC->GPREG0 = value; break;
        case 1: LPC_RTC->GPREG1 = value; break;
        case 2: LPC_RTC->GPREG2 = value; break;
        case 3: LPC_RTC->GPREG3 = value; break;
        case 4: LPC_RTC->GPREG4 = value; break;
        default:
            LogF("Unknown SettingsSetGenReg index %d\r\n", index);
            break;
    }
}


/*
ALSEC   6 (  64) Alarm value for Seconds       
ALMIN   6 (  64) Alarm value for Minutes       ProgramPosition
ALHOUR  5 (  32) Alarm value for Hours         
ALDOM   5 (  32) Alarm value for Day of Month  
ALDOW   3 (   8) Alarm value for Day of Week
ALDOY   9 ( 512) Alarm value for Day of Year   
ALMON   4 (  16) Alarm value for Months        
ALYEAR 12 (4096) Alarm value for Years         RtcBaseFraction
Total  50
*/

int  SettingsGetProgramPosition() { return LPC_RTC->ALMIN;     }
int  SettingsGetRtcFraction    () { return LPC_RTC->ALYEAR;    }

void SettingsSetProgramPosition(int value) { if (value >   63) value =   63; if (value <   0) value =   0; LPC_RTC->ALMIN  = value;     }
void SettingsSetRtcFraction    (int value) { if (value > 4095) value = 4095; if (value <   0) value =   0; LPC_RTC->ALYEAR = value;     }

static void saveIp(char* filename, char* pSetting, char* value)
{
    strncpy(pSetting, value, 16);
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    fputs(value, fp);
    fclose(fp);
}
static void saveRom(char* filename, char* pSetting, char* value)
{
    memcpy(pSetting, value, 8);
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    fwrite(pSetting, 1, 8, fp);
    fclose(fp);
}
static void saveInt(char* filename, int* pSetting, int value)
{
    *pSetting = value;
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    fwrite(pSetting, sizeof(int), 1, fp);
    fclose(fp);
}
static void loadIp(char* filename, char* pSetting)
{
    *pSetting = 0;
    FILE *fp = fopen(filename, "r");
    if (!fp) return;
    fgets(pSetting, 16, fp); // At most n−1 characters are read (leaving room for the null).
    fclose(fp);
}
static void loadRom(char* filename, char* pSetting)
{
    memset(pSetting, 0, 8);
    FILE *fp = fopen(filename, "r");
    if (!fp) return;
    fread(pSetting, 1, 8, fp);
    fclose(fp);   
}
static void loadInt(char* filename, int* pSetting, int defaultValue)
{
    *pSetting = defaultValue;
    FILE *fp = fopen(filename, "r");
    if (!fp) return;
    fread(pSetting, sizeof(int), 1, fp);
    fclose(fp);   
}

static char clockNtpIp[16];
static int  clockInitialInterval;
static int  clockNormalInterval;
static int  clockRetryInterval;
static int  clockOffsetMs;
static int  clockCalDivisor;

static char tankRom[8];
static char boilerOutputRom[8];
static char boilerReturnRom[8];
static char hallRom[8];

static int tankSetPoint;
static int tankHysteresis;
static int boilerRunOnResidual;
static int boilerRunOnTime;
static int nightTemperature;
static int frostTemperature;

char* SettingsGetClockNtpIp          () { return clockNtpIp;           }
int   SettingsGetClockInitialInterval() { return clockInitialInterval; }
int   SettingsGetClockNormalInterval () { return clockNormalInterval;  }
int   SettingsGetClockRetryInterval  () { return clockRetryInterval;   }
int   SettingsGetClockOffsetMs       () { return clockOffsetMs;        }
int   SettingsGetClockCalDivisor     () { return clockCalDivisor;      }

char* SettingsGetTankRom             () { return tankRom;              }
char* SettingsGetBoilerOutputRom     () { return boilerOutputRom;      }
char* SettingsGetBoilerReturnRom     () { return boilerReturnRom;      }
char* SettingsGetHallRom             () { return hallRom;              }

int   SettingsGetTankSetPoint        () { return tankSetPoint;         }
int   SettingsGetTankHysteresis      () { return tankHysteresis;       }
int   SettingsGetBoilerRunOnResidual () { return boilerRunOnResidual;  }
int   SettingsGetBoilerRunOnTime     () { return boilerRunOnTime;      }
int   SettingsGetNightTemperature    () { return nightTemperature;     }
int   SettingsGetFrostTemperature    () { return frostTemperature;     }


void SettingsSetClockNtpIp           (char *value) { saveIp ("/local/clk_ntp.ip",    clockNtpIp,           value); }
void SettingsSetClockInitialInterval (int   value) { saveInt("/local/clk_init.tim", &clockInitialInterval, value); }
void SettingsSetClockNormalInterval  (int   value) { saveInt("/local/clk_norm.tim", &clockNormalInterval,  value); }
void SettingsSetClockRetryInterval   (int   value) { saveInt("/local/clk_retr.tim", &clockRetryInterval,   value); }
void SettingsSetClockOffsetMs        (int   value) { saveInt("/local/clk_offs.tim", &clockOffsetMs,        value); }
void SettingsSetClockCalDivisor      (int   value) { saveInt("/local/clk_cal.div",  &clockCalDivisor,      value); }

void SettingsSetTankRom              (char* value) { saveRom("/local/tank.rom",      tankRom,              value); }
void SettingsSetBoilerOutputRom      (char* value) { saveRom("/local/blr_out.rom",   boilerOutputRom,      value); }
void SettingsSetBoilerReturnRom      (char* value) { saveRom("/local/blr_rtn.rom",   boilerReturnRom,      value); }
void SettingsSetHallRom              (char* value) { saveRom("/local/hall.rom",      hallRom,              value); }

void SettingsSetTankSetPoint         (int   value) { saveInt("/local/tnk_setp.deg", &tankSetPoint,         value); }
void SettingsSetTankHysteresis       (int   value) { saveInt("/local/tnk_hyst.deg", &tankHysteresis,       value); }
void SettingsSetBoilerRunOnResidual  (int   value) { saveInt("/local/blr_resd.deg", &boilerRunOnResidual,  value); }
void SettingsSetBoilerRunOnTime      (int   value) { saveInt("/local/blr_runo.tim", &boilerRunOnTime,      value); }
void SettingsSetNightTemperature     (int   value) { saveInt("/local/ngt_setp.deg", &nightTemperature,     value); }
void SettingsSetFrostTemperature     (int   value) { saveInt("/local/frt_setp.deg", &frostTemperature,     value); }


int  SettingsInit()
{
    loadRom("/local/tank.rom",      tankRom                 );
    loadRom("/local/blr_out.rom",   boilerOutputRom         );
    loadRom("/local/blr_rtn.rom",   boilerReturnRom         );
    loadRom("/local/hall.rom",      hallRom                 );
    
    loadIp ("/local/clk_ntp.ip",    clockNtpIp              );
    loadInt("/local/clk_init.tim", &clockInitialInterval,  1);
    loadInt("/local/clk_norm.tim", &clockNormalInterval, 600);
    loadInt("/local/clk_retr.tim", &clockRetryInterval,   60);
    loadInt("/local/clk_offs.tim", &clockOffsetMs,         0);
    loadInt("/local/clk_cal.div",  &clockCalDivisor,      16);
    
    loadInt("/local/tnk_setp.deg", &tankSetPoint,         80);
    loadInt("/local/tnk_hyst.deg", &tankHysteresis,        5);
    loadInt("/local/blr_resd.deg", &boilerRunOnResidual,   2);
    loadInt("/local/blr_runo.tim", &boilerRunOnTime,     360);
    loadInt("/local/ngt_setp.deg", &nightTemperature,     15);
    loadInt("/local/frt_setp.deg", &frostTemperature,      8);
    
    return 0;
}