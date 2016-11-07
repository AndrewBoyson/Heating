#include     "mbed.h"
#include      "cfg.h"
#include      "log.h"
#include       "io.h"
#include     "time.h"
#include      "rtc.h"
#include  "rtc-cal.h"
#include  "rtc-fra.h"
#include  "rtc-clk.h"
#include   "nonvol.h"

#define DEBUG true

//Define midpoints between one and two seconds - note that '-' is higher precedence than '<<'
static const int         A_HALF_SECOND  = 1 << RTC_RESOLUTION_BITS - 1;
static const int ONE_AND_A_HALF_SECONDS = 3 << RTC_RESOLUTION_BITS - 1;
static const int TWO_AND_A_HALF_SECONDS = 5 << RTC_RESOLUTION_BITS - 1;

static bool havePrevTm = false; //Set when prevTm is set from this Tm
static struct tm thisTm;        //Updated in RtcMain whenever a second has elapsed
static struct tm prevTm;        //Set when time is set or a second is handled. Used to determine if a second has been skipped or added.

static void compareSeconds(int elapsedTim)
{
    //Only compare if have a previous value
    if (!havePrevTm) return;
       
    //Work out the RTC elapsed time
    int elapsedRtc = TimeFromTmUtc(&thisTm) - TimeFromTmUtc(&prevTm);
    
    //Compare the RTC elapsed time with the TIM1 elapsed time.
    switch (elapsedRtc)
    {
        case 1:
            if      (elapsedTim <         A_HALF_SECOND ) {            LogTimeF("TIM 0; RTC 1\r\n");                                             } //<1 seconds elapsed.
            else if (elapsedTim < ONE_AND_A_HALF_SECONDS) {                                                       RtcCalSecondsHandler( 0);      } // 1 second  elapsed. This is normal.
            else if (elapsedTim < TWO_AND_A_HALF_SECONDS) { if (DEBUG) LogTimeF("TIM 2; RTC 1\r\n");              RtcCalSecondsHandler(-1);      } // 2 seconds elapsed. A second has been skipped from the RTC
            else                                          {            LogTimeF("TIM 3; RTC 1\r\n");                                             } //>2 seconds elapsed.
            break;
            
        case 2:
            if      (elapsedTim <         A_HALF_SECOND ) {            LogTimeF("TIM 0; RTC 2\r\n");                                             } //<1 seconds elapsed.
            else if (elapsedTim < ONE_AND_A_HALF_SECONDS) { if (DEBUG) LogTimeF("TIM 1; RTC 2\r\n");              RtcCalSecondsHandler(+1);      } // 1 second  elapsed. A second has been added to the RTC
            else if (elapsedTim < TWO_AND_A_HALF_SECONDS) {            LogTimeF("TIM 2; RTC 2\r\n");                                             } // 2 seconds elapsed. An interrupt has been missed
            else                                          {            LogTimeF("TIM 3; RTC 2\r\n");                                             } //>2 seconds elapsed.
            break;
            
        default:
            if      (elapsedTim <         A_HALF_SECOND ) {            LogTimeF("TIM 0; RTC %d\r\n", elapsedRtc);                                } //<1 seconds elapsed.
            else if (elapsedTim < ONE_AND_A_HALF_SECONDS) {            LogTimeF("TIM 1; RTC %d\r\n", elapsedRtc); RtcClkAddTime(1 - elapsedRtc); } // 1 second  elapsed. The RTC has lost time
            else if (elapsedTim < TWO_AND_A_HALF_SECONDS) {            LogTimeF("TIM 2; RTC %d\r\n", elapsedRtc);                                } // 2 seconds elapsed.
            else                                          {            LogTimeF("TIM 3; RTC %d\r\n", elapsedRtc);                                } //>2 seconds elapsed.
            break;
    }
}


int RtcMain()
{
    if (RtcClockHasIncremented)
    {    
        RtcClockGetTm(&thisTm);             //Fetch the seconds
        int fraction = RtcFractionGet();    //Fetch the fraction
        RtcFractionStopAndResetCounter();   //Reset the timer
        compareSeconds(fraction);           //Compare the timer and rtc elapsed seconds and handle the differences
        prevTm = thisTm; havePrevTm = true; //Remember the seconds for next time
        RtcFractionRelease();               //Reenable the timer
        RtcClockHasIncremented = false;     //Record that we have processed this second
    }
    return 0;
}
int RtcInit()
{
    //Do very first time set up
    if (RtcClockIsNotSet())
    {
        RtcClockStopAndResetDivider();
        NonVolSetBaseFraction(0);
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
    LogTimeF("Before set: seconds %02d:%02d:%02d; fraction base=%04d, cal=%04d, tim=%04d\r\n", thisTm.tm_hour, thisTm.tm_min, thisTm.tm_sec, NonVolGetBaseFraction(), RtcCalGetFraction(), RtcFractionGet());
    
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
    NonVolSetBaseFraction(fraction);
    
    //Set the RTC to the whole number of seconds in the time
    TimeToTmUtc(wholeseconds, &thisTm);
    RtcClockSetTm(&thisTm);  
    
    //Adjust the calibration and send the seconds after the change
    RtcCalTimeSetHandler(rtc, act, &thisTm);
    prevTm = thisTm; havePrevTm = true;
    
    //Release the counters
    RtcFractionRelease();
    RtcCalReleaseCounter();
    RtcClockRelease();
    
    LogTimeF("After  set: seconds %02d:%02d:%02d; fraction base=%04d, cal=%04d, tim=%04d\r\n\r\n", thisTm.tm_hour, thisTm.tm_min, thisTm.tm_sec, NonVolGetBaseFraction(), RtcCalGetFraction(), RtcFractionGet());
}
uint64_t RtcGet()
{                                        //Dont be tempted to return zero if the clock is not set - elapsed time is still needed.    
    uint64_t t = TimeFromTmUtc(&thisTm); //Convert struct tm to a time_t and put into 64 bits
    t <<= RTC_RESOLUTION_BITS;           //Move the seconds to the left of the decimal point
    t += NonVolGetBaseFraction();        //Add remaining fraction of a second if it has been set
    t += RtcCalGetFraction();            //Add fraction from calibration
    t += RtcFractionGet();               //Add the fractional part - 21.6.4 Timer Counter register
    return t;
}
static void getCopyTm(struct tm* ptm)
{
    ptm->tm_sec   = thisTm.tm_sec;
    ptm->tm_min   = thisTm.tm_min;
    ptm->tm_hour  = thisTm.tm_hour;
    ptm->tm_mday  = thisTm.tm_mday;
    ptm->tm_mon   = thisTm.tm_mon;
    ptm->tm_year  = thisTm.tm_year;
    ptm->tm_wday  = thisTm.tm_wday;
    ptm->tm_yday  = thisTm.tm_yday;
    ptm->tm_isdst = thisTm.tm_isdst;

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
