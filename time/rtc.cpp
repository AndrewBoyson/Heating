#include     "mbed.h"
#include      "cfg.h"
#include      "log.h"
#include       "io.h"
#include "settings.h"
#include     "time.hpp"
#include      "rtc.hpp"
#include  "rtc-cal.hpp"

static struct tm stm;
static void getstm()
{
    stm.tm_sec    = LPC_RTC->SEC; //Make sure have had at least 8 cycles
    stm.tm_sec    = LPC_RTC->SEC;
    stm.tm_sec    = LPC_RTC->SEC;
    stm.tm_sec    = LPC_RTC->SEC;
    stm.tm_min    = LPC_RTC->MIN;
    stm.tm_hour   = LPC_RTC->HOUR;
    stm.tm_mday   = LPC_RTC->DOM;
    stm.tm_mon    = LPC_RTC->MONTH - 1;
    stm.tm_year   = LPC_RTC->YEAR  - 1900;
    stm.tm_wday   = LPC_RTC->DOW;
    stm.tm_yday   = LPC_RTC->DOY   - 1;
    stm.tm_isdst  = -1; // -ve should signify that dst data is not available but it is used here to denote UTC
}
static void setstm()
{
    LPC_RTC->SEC     = stm.tm_sec;         // 0 --> 59
    LPC_RTC->MIN     = stm.tm_min;         // 0 --> 59
    LPC_RTC->HOUR    = stm.tm_hour;        // 0 --> 23
    LPC_RTC->DOM     = stm.tm_mday;        // 1 --> 31
    LPC_RTC->MONTH   = stm.tm_mon  + 1;    // rtc    1 -->   12; tm 0 -->  11
    LPC_RTC->YEAR    = stm.tm_year + 1900; // rtc 1900 --> 2100; tm 0 --> 200
    LPC_RTC->DOW     = stm.tm_wday;        // 0 --> 6 where 0 == Sunday
    LPC_RTC->DOY     = stm.tm_yday + 1;    // rtc 1 --> 366;     tm 0 --> 365 
}
static int  getRtcIsSet() { return (LPC_RTC->RTC_AUX & 0x10) == 0; } //27.6.2.5 RTC Auxiliary control register - RTC Oscillator Fail detect flag
static void setRtcIsSet() {         LPC_RTC->RTC_AUX = 0x10;       } //27.6.2.5 RTC Auxiliary control register - RTC Oscillator Fail detect flag - Writing a 1 to this bit clears the flag.

static int getFraction()  { return LPC_TIM1->TC; } //21.6.4 Timer Counter register

static void stopAndResetFraction() { LPC_TIM1->TCR =     2; } //Clear  the fractional part count   - 21.6.2 Timer Control Register - Reset  TC and PC.
static void releaseFraction()      { LPC_TIM1->TCR =     1; } //Enable the fractional part counter - 21.6.2 Timer Control Register - Enable TC and PC
static void stopAndResetClock()    { LPC_RTC->CCR  =  0x12; } //Stop  the clock (CLKEN bit 0 = 0); reset   the divider (CTCRST bit 1 = 1); stop and reset the calibration counter (CCALEN bit 4 = 1)
static void releaseClock()         { LPC_RTC->CCR  =     1; } //Start the clock (CLKEN bit 0 = 1); release the divider (CTCRST bit 1 = 0); release the calibration counter (CCALEN bit 4 = 0)

static volatile bool hadOneSecondIncrement = false;

static void rtcInterrupt()
{
    if (LPC_RTC->ILR & 1) //A counter has incremented
    {
        hadOneSecondIncrement = true;
        LPC_RTC->ILR = 1;  // 27.6.2.1 Interrupt Location Register - Clear interrupt by writing a 1 to bit 0
    }
    
    if (LPC_RTC->ILR & 2) //An alarm has matched
    {
        LPC_RTC->ILR = 2;  // 27.6.2.1 Interrupt Location Register - Clear interrupt by writing a 1 to bit 1
    }
}
int RtcMain()
{
    if (!hadOneSecondIncrement) return 0;
    hadOneSecondIncrement = false;
    
    //Fetch the RTC
    getstm();
    int fraction = getFraction();

    //Reset the timer
    stopAndResetFraction();

     //Allow calibration to determine if a second has been lost or not.
    RtcCalSecondsHandler(stm.tm_sec, fraction);
    
    //Reenable the timer
    releaseFraction();
    
    return 0;
}
int RtcInit()
{
    //Do very first time set up
    if (!getRtcIsSet())
    {
        stopAndResetClock();
        SettingsSetRtcFraction(0);
    }
    
    //Set up the real time clock to handle whole seconds    
    LPC_RTC->AMR       = 0xFF;    //27.6.2.4 Alarm Mask Register - mask all alarms
    LPC_RTC->RTC_AUXEN =    0;    //27.6.2.6 RTC Auxiliary Enable register - mask the oscillator stopped interrupt
    LPC_RTC->ILR       =    3;    //27.6.2.1 Interrupt Location Register - Clear all interrupts
    LPC_RTC->CIIR      =    1;    //27.6.2.3 Counter Increment Interrupt Register - set to interrupt on the second only
    NVIC_SetVector(RTC_IRQn, (uint32_t)&rtcInterrupt);
    NVIC_EnableIRQ(RTC_IRQn);     //Allow the fractional part to be reset at the end of a second including any pending


    //Set up timer 1 to handle the fractional part
    stopAndResetFraction();

    int pre = 96000000 / (1 << RTC_RESOLUTION_BITS) - 1;
    int max = (1 << RTC_RESOLUTION_BITS) - 1;
    
    LPC_SC->PCLKSEL0 &= ~0x20;    //  4.7.3 Peripheral Clock Selection - PCLK_peripheral PCLK_TIMER1 00xxxx = CCLK/4; 01xxxx = CCLK
    LPC_SC->PCLKSEL0 |=  0x10;
    LPC_SC->PCONP    |=     4;    //  4.8.9 Power Control for Peripherals register - Timer1 Power On
    LPC_TIM1->CTCR    =     0;    // 21.6.3 Count Control Register - Timer mode
    LPC_TIM1->PR      =   pre;    // 21.6.5 Prescale register      - Prescale 96MHz clock to 1s == 2048 (divide by PR+1).
    LPC_TIM1->MR0     =   max;    // 21.6.7 Match Register 0       - Match count
    LPC_TIM1->MCR     =     0;    // 21.6.8 Match Control Register - Do nothing when matched
    
    //Initialise one second handler
    hadOneSecondIncrement = false;
    
    //Set up the calibration
    RtcCalInit();                 //Set up the calibration side
    
    //Release clock and fraction timer
    releaseClock();
    releaseFraction();
   
    return 0;
}
void RtcSet(uint64_t act)
{
    
    //Record the RTC time before it is changed ready for use by the calibration after the change
    uint64_t rtc = RtcGet();
    
    //Set the RTC time
    stopAndResetClock();
    stopAndResetFraction();
        
    int fraction = act & ((1 << RTC_RESOLUTION_BITS) - 1); //Record the remaining fraction of a second not set in the RTC
    SettingsSetRtcFraction(fraction);
    int wholeseconds = act >> RTC_RESOLUTION_BITS;         //Set the RTC to the whole number of seconds in the time
    
    TimeToTmUtc(wholeseconds, &stm);
    setstm();  
    setRtcIsSet();
    if (LPC_RTC->SEC != stm.tm_sec || LPC_RTC->MIN != stm.tm_min || LPC_RTC->HOUR != stm.tm_hour) LogF("RTC value did not set %d:%d:%d\r\n", stm.tm_hour, stm.tm_min, stm.tm_sec); 

    
    LogF("%04d-%03d %02d:%02d:%02d\r\n", stm.tm_year + 1900, stm.tm_yday, stm.tm_hour, stm.tm_min, stm.tm_sec);
    
    releaseClock();
    releaseFraction();
    
    //Adjust the calibration and send the seconds after the change
    RtcCalTimeSetHandler(rtc, act, stm.tm_sec);

}
uint64_t RtcGet()
{
    if (!getRtcIsSet()) return 0;      //Bomb out with a value of zero if the RTC is not set
    
    uint64_t t = TimeFromTmUtc(&stm);   //Convert struct tm to a time_t and put into 64 bits
    t <<= RTC_RESOLUTION_BITS;         //Move the seconds to the left of the decimal point
    t += SettingsGetRtcFraction();     //Add remaining fraction of a second if it has been set
    t += getFraction();                //Add the fractional part - 21.6.4 Timer Counter register
    
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
