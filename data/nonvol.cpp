#include "mbed.h"

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

int  NonVolGetMainPosition()
{
    return LPC_RTC->ALMON;
}
void NonVolSetMainPosition(int value)
{
    if (value > 15) value = 15;
    if (value <  0) value =  0;
    LPC_RTC->ALMON  = value;
}


bool haveBaseFraction = false;
int baseFraction;

int  NonVolGetBaseFraction()
{
    if (!haveBaseFraction)
    {
        baseFraction = LPC_RTC->ALYEAR;
        haveBaseFraction = true;
    }
    return baseFraction;
}

void NonVolSetBaseFraction(int value)
{
    if (value > 4095) value = 4095;
    if (value <    0) value =    0;
    LPC_RTC->ALYEAR = value;
    baseFraction    = value;
    haveBaseFraction = true;
}


