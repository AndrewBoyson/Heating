#include     "mbed.h"
#include      "cfg.h"
#include      "log.h"
#include       "io.h"
#include "settings.h"
#include     "time.h"
#include      "rtc.h"
#include  "rtc-cal.h"
#include  "rtc-fra.h"
#include  "rtc-clk.h"

static struct tm stm;

int RtcMain()
{
    if (RtcClockHasIncremented)
    {    
        RtcClockGetTm(&stm);                  //Fetch the seconds
        int fraction = RtcFractionGet();      //Fetch the fraction
        RtcFractionStopAndResetCounter();     //Reset the timer
        RtcCalSecondsHandler(&stm, fraction); //Allow calibration to determine if a second has been lost or not.
        RtcFractionRelease();                 //Reenable the timer
        RtcClockHasIncremented = false;       //Record that we have processed this second
    }
    return 0;
}
int RtcInit()
{
    //Do very first time set up
    if (RtcClockIsNotSet())
    {
        RtcClockStopAndResetDivider();
        SettingsSetRtcFraction(0);
    }
    
    //Set up the real time clock to handle whole seconds
    RtcClockInit();

    //Set up timer 1 to handle the fractional part
    RtcFractionInit();
    
    //Set up the calibration
    RtcCalInit();
   
    return 0;
}
void getSecondsAndFractions(uint64_t act, int* pseconds, int* pfraction)
{
    *pfraction = act & ((1 << RTC_RESOLUTION_BITS) - 1); //Record the remaining fraction of a second not set in the RTC
    *pseconds  = act >> RTC_RESOLUTION_BITS;             //Set the RTC to the whole number of seconds in the time
}
void RtcSet(uint64_t act)
{
    LogTimeF("Before set: seconds %02d:%02d:%02d; fraction base=%04d, cal=%04d, tim=%04d\r\n", stm.tm_hour, stm.tm_min, stm.tm_sec, SettingsGetRtcFraction(), RtcCalGetFraction(), RtcFractionGet());
    
    //Split the time into seconds and the fraction
    int wholeseconds, fraction;
    getSecondsAndFractions(act, &wholeseconds, &fraction);
    
    //Record the RTC time before it is changed ready for use by the calibration after the change
    uint64_t rtc = RtcGet();
    
    //Stop the counters
    RtcClockStopAndResetDivider();
    RtcCalStopAndResetCounter();
    RtcFractionStopAndResetCounter();
        
    //Record the remaining fraction of a second not set in the RTC
    SettingsSetRtcFraction(fraction);
    
    //Set the RTC to the whole number of seconds in the time
    TimeToTmUtc(wholeseconds, &stm);
    RtcClockSetTm(&stm);  
    
    //Adjust the calibration and send the seconds after the change
    RtcCalTimeSetHandler(rtc, act, &stm);
    
    //Release the counters
    RtcFractionRelease();
    RtcCalReleaseCounter();
    RtcClockRelease();
    
    LogTimeF("After  set: seconds %02d:%02d:%02d; fraction base=%04d, cal=%04d, tim=%04d\r\n\r\n", stm.tm_hour, stm.tm_min, stm.tm_sec, SettingsGetRtcFraction(), RtcCalGetFraction(), RtcFractionGet());
}
uint64_t RtcGet()
{                                     //Dont be tempted to return zero if the clock is not set - elapsed time is still needed.    
    uint64_t t = TimeFromTmUtc(&stm); //Convert struct tm to a time_t and put into 64 bits
    t <<= RTC_RESOLUTION_BITS;        //Move the seconds to the left of the decimal point
    t += SettingsGetRtcFraction();    //Add remaining fraction of a second if it has been set
    t += RtcCalGetFraction();         //Add fraction from calibration
    t += RtcFractionGet();            //Add the fractional part - 21.6.4 Timer Counter register
    return t;
}
static void getCopyTm(struct tm* ptm)
{
    ptm->tm_sec   = stm.tm_sec;
    ptm->tm_min   = stm.tm_min;
    ptm->tm_hour  = stm.tm_hour;
    ptm->tm_mday  = stm.tm_mday;
    ptm->tm_mon   = stm.tm_mon;
    ptm->tm_year  = stm.tm_year;
    ptm->tm_wday  = stm.tm_wday;
    ptm->tm_yday  = stm.tm_yday;
    ptm->tm_isdst = stm.tm_isdst;

}
void RtcGetTmUtc(struct tm* ptm)
{
    getCopyTm(ptm);
}
void RtcGetTmLocal(struct tm* ptm)
{
    getCopyTm(ptm);
    TimeTmUtcToLocal(ptm);
}
