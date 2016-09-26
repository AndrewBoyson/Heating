#include     "mbed.h"
#include      "log.h"
#include "settings.h"
#include      "rtc.hpp"

static uint64_t prev = 0;   //Set when time is set. Used to determine calibration value.
static  int     prevSecond; //Set when time is set or a second is handled. Used to determine if a second has been skipped or added.
    
static int  addedSeconds = 0; //Incremented and decremented in calculateSeconds and zeroed after use in adjustCalibration
static int missedSeconds = 0;
static int  errorSeconds = 0;

static void updateSeconds(int thisSecond, int elapsedTim)
{
    //Define midpoints between one and two seconds
    const int         A_HALF_SECOND  = 0x1 << RTC_RESOLUTION_BITS - 1; // '-' is higher precedence than '<<'
    const int ONE_AND_A_HALF_SECONDS = 0x3 << RTC_RESOLUTION_BITS - 1; // '-' is higher precedence than '<<'
    const int TWO_AND_A_HALF_SECONDS = 0x5 << RTC_RESOLUTION_BITS - 1; // '-' is higher precedence than '<<'
                
    //Work out the RTC elapsed time
    int elapsedRtc = thisSecond - prevSecond; if (elapsedRtc < 0) elapsedRtc += 60; //If the seconds have wrapped around then do modulo 60; 0 - 59 + 60 = +1
    
    //Compare the reported elapsed time (RTC) with the actual elapsed time (TIM1). At this point the RTC seconds count has incremented but TIM1 has not been reset so both should be one.
    switch (elapsedRtc)
    {
        case 1:
            if      (elapsedTim <         A_HALF_SECOND )  errorSeconds++; //<1 seconds elapsed.
            else if (elapsedTim < ONE_AND_A_HALF_SECONDS)                ; // 1 second  elapsed. This is normal, do nothing
            else if (elapsedTim < TWO_AND_A_HALF_SECONDS)  addedSeconds--; // 2 seconds elapsed. A second has been skipped from the RTC
            else                                           errorSeconds++; //>2 seconds elapsed.
            break;
            
        case 2:
            if      (elapsedTim <         A_HALF_SECOND )  errorSeconds++; //<1 seconds elapsed.
            else if (elapsedTim < ONE_AND_A_HALF_SECONDS)  addedSeconds++; // 1 second  elapsed. A second has been added to the RTC
            else if (elapsedTim < TWO_AND_A_HALF_SECONDS) missedSeconds--; // 2 seconds elapsed. An interrupt has been missed
            else                                           errorSeconds++; //>2 seconds elapsed.
            break;
            
        default:
            errorSeconds++;
            break;
    }
}
static void resetSeconds()
{
     addedSeconds = 0;
    missedSeconds = 0;
     errorSeconds = 0;
}
static void setCalibration(int value) //-ve means skip a second each match seconds; +ve means add two seconds each match seconds
{
    LPC_RTC->CALIBRATION = (value >= 0) ? value & 0x1FFFF : -value & 0x1FFFF | 0x20000;
}
static int getCalibration()
{
    int calval = LPC_RTC->CALIBRATION & 0x1FFFF; //27.6.4.2 Calibration register
    int caldir = LPC_RTC->CALIBRATION & 0x20000; //Backward calibration. When CALVAL is equal to the calibration counter, the RTC timers will stop incrementing for 1 second.
    
    return caldir ? -calval : calval;
}
static void adjustCalibration(uint64_t rtc, uint64_t act)
{
    LogF("added=%d; missed=%d; errors=%d\r\n", addedSeconds, missedSeconds, errorSeconds);

                  rtc -= addedSeconds << RTC_RESOLUTION_BITS;                   //Remove any added seconds to give the rtc as it would have been
    uint64_t interval  = rtc - prev;                                            //Get the actual elapsed time (from NTP)
     int64_t     loss  = act - rtc;                                             //Get the difference between the theo (actual) and the rtc
     
    LogF("act=%llu; rtc=%llu; prev=%llu;\r\n", act, rtc, prev);
    LogF("interval=%llu; loss=%lld\r\n",       interval, loss);
    
    resetSeconds();
    
    int32_t lastCalibration = getCalibration();                                 //Work out how many calibration seconds were added; -ve means skip a second each match seconds; +ve means add two seconds each match seconds
    int32_t theoCalibration;
    if      (loss > 0) theoCalibration =   interval /  loss;                    //Calculate the theoretical calibration (or +infinity if no difference) from these times
    else if (loss < 0) theoCalibration = -(interval / -loss);
    else               theoCalibration = 0x1FFFF;
    
    int K = SettingsGetClockCalDivisor();
    int32_t addCalibration = (theoCalibration - lastCalibration) / K;           //take a part of the difference between theo and actual
    int32_t newCalibration = lastCalibration + addCalibration;                  //Add that part to smooth out the new calibration
     
    LogF("last=%d; theo=%d; add=%d; new=%d\r\n\r\n", lastCalibration, theoCalibration, addCalibration, newCalibration);
                
    if (newCalibration <  100 && newCalibration >= 0) newCalibration =  100;    //Do not allow monster adjustments over +/- 1%
    if (newCalibration > -100 && newCalibration <  0) newCalibration = -100;
    if (lastCalibration) setCalibration(newCalibration);                        //Only update the calibration if it is not disabled (set to zero)
}
void RtcCalSecondsHandler(int second, int fraction)
{
    if (prev) updateSeconds(second, fraction);
    else       resetSeconds();
    
    prevSecond = second;
}
void RtcCalTimeSetHandler(uint64_t rtc, uint64_t act, int second) //'rtc' is from before the RTC is set to actual; 'act' and 'second' are what the RTC has been set to.
{     
    if (prev) adjustCalibration(rtc, act);
    prev       = act;
    prevSecond = second;
}
int RtcCalGet()
{
    return getCalibration();
}
void RtcCalSet(int value)
{
    setCalibration(value);
}
void RtcCalInit()
{
    prev = 0;
}
