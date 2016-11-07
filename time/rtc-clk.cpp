#include "mbed.h"
#include  "log.h"

int  RtcClockIsNotSet() { return LPC_RTC->RTC_AUX & 0x10; } //27.6.2.5 RTC Auxiliary control register - RTC Oscillator Fail detect flag

void RtcClockStopAndResetDivider() { LPC_RTC->CCR |=  2; } //Stop and reset the divider (CTCRST bit 1 = 1)
void RtcClockRelease()             { LPC_RTC->CCR &= ~2; } //Release        the divider (CTCRST bit 1 = 0)

volatile bool RtcClockHasIncremented = false;

static void rtcInterrupt()
{
    if (LPC_RTC->ILR & 1) //A counter has incremented
    {
        RtcClockHasIncremented = true;
        LPC_RTC->ILR = 1;  // 27.6.2.1 Interrupt Location Register - Clear interrupt by writing a 1 to bit 0
    }
    
    if (LPC_RTC->ILR & 2) //An alarm has matched
    {
        LPC_RTC->ILR = 2;  // 27.6.2.1 Interrupt Location Register - Clear interrupt by writing a 1 to bit 1
    }
}

void RtcClockGetTm(struct tm* ptm)
{
    ptm->tm_sec    = LPC_RTC->SEC;
    ptm->tm_min    = LPC_RTC->MIN;
    ptm->tm_hour   = LPC_RTC->HOUR;
    ptm->tm_mday   = LPC_RTC->DOM;
    ptm->tm_mon    = LPC_RTC->MONTH - 1;
    ptm->tm_year   = LPC_RTC->YEAR  - 1900;
    ptm->tm_wday   = LPC_RTC->DOW;
    ptm->tm_yday   = LPC_RTC->DOY   - 1;
    ptm->tm_isdst  = -1; // -ve should signify that dst data is not available but it is used here to denote UTC
}
void RtcClockSetTm(struct tm* ptm)
{
    LogTimeF("Setting clock %02d:%02d:%02d\r\n", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    LPC_RTC->SEC     = ptm->tm_sec;         // 0 --> 59
    LPC_RTC->MIN     = ptm->tm_min;         // 0 --> 59
    LPC_RTC->HOUR    = ptm->tm_hour;        // 0 --> 23
    LPC_RTC->DOM     = ptm->tm_mday;        // 1 --> 31
    LPC_RTC->MONTH   = ptm->tm_mon  + 1;    // rtc    1 -->   12; tm 0 -->  11
    LPC_RTC->YEAR    = ptm->tm_year + 1900; // rtc 1900 --> 2100; tm 0 --> 200
    LPC_RTC->DOW     = ptm->tm_wday;        // 0 --> 6 where 0 == Sunday
    LPC_RTC->DOY     = ptm->tm_yday + 1;    // rtc 1 --> 366;     tm 0 --> 365 
    LPC_RTC->RTC_AUX = 0x10;                //27.6.2.5 RTC Auxiliary control register - RTC Oscillator Fail detect flag - Writing a 1 to this bit clears the flag.
}
void RtcClkAddTime(int t)
{
    //Extract the seconds, minutes, hours and days from the time_t t
    div_t divres;
    divres = div(          t, 60);    int seconds  = divres.rem;
    divres = div(divres.quot, 60);    int minutes  = divres.rem;
    divres = div(divres.quot, 24);    int hours    = divres.rem;
                                      int days     = divres.quot;
    LPC_RTC->SEC     += seconds;
    LPC_RTC->MIN     += minutes;
    LPC_RTC->HOUR    += hours;
    LogTimeF("Added %02d:%02d:%02d to RTC\r\n", hours, minutes, seconds);
    if (days) LogTimeF("Could not add %d days to RTC\r\n", days);
}

void RtcClockInit()
{
    LPC_RTC->CCR       =    1;    //27.6.2.2 Clock Control Register - enable the time counters (CLKEN bit 0 = 1); no reset (CTCRST bit 1 = 0); enable calibration counter (CCALEN bit 4 = 0)
    LPC_RTC->CIIR      =    1;    //27.6.2.3 Counter Increment Interrupt Register - set to interrupt on the second only
    LPC_RTC->AMR       = 0xFF;    //27.6.2.4 Alarm Mask Register - mask all alarms
    LPC_RTC->RTC_AUXEN =    0;    //27.6.2.6 RTC Auxiliary Enable register - mask the oscillator stopped interrupt
    LPC_RTC->ILR       =    3;    //27.6.2.1 Interrupt Location Register - Clear all interrupts
    NVIC_SetVector(RTC_IRQn, (uint32_t)&rtcInterrupt);
    NVIC_EnableIRQ(RTC_IRQn);     //Allow the fractional part to be reset at the end of a second including any pending
    
    RtcClockHasIncremented = false;

}
