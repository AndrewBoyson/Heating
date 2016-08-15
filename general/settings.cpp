#include "mbed.h"
#include  "log.h"

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
ALSEC   6 (00 -  64) Alarm value for Seconds       TankHysteresis
ALMIN   6 (00 -  64) Alarm value for Minutes       BoilerRunOnResidual
ALHOUR  5 (00 -  32) Alarm value for Hours         NightTemperature
ALDOM   5 (00 -  32) Alarm value for Day of Month  FrostTemperature
ALDOW   3 (00 -  07) Alarm value for Day of Week
ALDOY   9 (00 - 511) Alarm value for Day of Year   TankSetPoint
ALMON   4 (00 -  15) Alarm value for Months
ALYEAR 12 Alarm value for Years         BoilerRunOnTime
Total  50
*/

int  SettingsGetTankSetPoint()
{
    return LPC_RTC->ALDOY;
}
void SettingsSetTankSetPoint(int value)
{
    LPC_RTC->ALDOY = value;
}
int  SettingsGetTankHysteresis()
{
    return LPC_RTC->ALSEC;
}
void SettingsSetTankHysteresis(int value)
{
    LPC_RTC->ALSEC = value;
}
int  SettingsGetBoilerRunOnResidual()
{
    return LPC_RTC->ALMIN;
}
void SettingsSetBoilerRunOnResidual(int value)
{
    LPC_RTC->ALMIN = value;
}
int  SettingsGetBoilerRunOnTime()
{
    return LPC_RTC->ALYEAR;
}
void SettingsSetBoilerRunOnTime(int value)
{
    LPC_RTC->ALYEAR = value;
}
int  SettingsGetNightTemperature()
{
    return LPC_RTC->ALHOUR;
}
void SettingsSetNightTemperature(int value)
{
    LPC_RTC->ALHOUR = value;
}
int  SettingsGetFrostTemperature()
{
    return LPC_RTC->ALDOM;
}
void SettingsSetFrostTemperature(int value)
{
    LPC_RTC->ALDOM = value;
}