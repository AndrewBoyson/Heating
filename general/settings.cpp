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
int  SettingsGetTankSetPoint()
{
    return LPC_RTC->ALDOY; //Alarm day of year is 9 bits long
}
void SettingsSetTankSetPoint(int value)
{
    LPC_RTC->ALDOY = value; //9 bit long
}
int  SettingsGetTankHysteresis()
{
    return LPC_RTC->ALSEC; //Alarm Seconds is 6 bits
}
void SettingsSetTankHysteresis(int value)
{
    LPC_RTC->ALSEC = value; //6 bit long
}
int  SettingsGetTankMinHoldOnLoss()
{
    return LPC_RTC->ALMIN; //Alarm Seconds is 6 bits
}
void SettingsSetTankMinHoldOnLoss(int value)
{
    LPC_RTC->ALMIN = value; //6 bit long
}
