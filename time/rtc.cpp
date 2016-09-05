#include "mbed.h"
#include  "cfg.h"
#include  "log.h"
#include   "io.h"
#include "time.h"
#include  "rtc.h"

static int fractionalPartOfSetTime; //Initialised to -1 then will be non-negative once the fractional part of the clock is set.
static uint64_t lastSet = 0;        //Set when time is set, unset if calibration match value changed manually. Used to determine calibration value.


static void rtcInterrupt()
{
    if (LPC_RTC->ILR & 1)
    {
        LPC_TIM1->TCR = 2; // 21.6.2 Timer Control Register - Reset TC and PC
        LPC_TIM1->TCR = 1; // 21.6.2 Timer Control Register - Enable TC and PC
        LPC_RTC->ILR = 1;  // 27.6.2.1 Interrupt Location Register - Clear interrupt by writing a 1 to bit 0
    }
    if (LPC_RTC->ILR & 2)
    {
        LPC_RTC->ILR = 2;  // 27.6.2.1 Interrupt Location Register - Clear interrupt by writing a 1 to bit 1
    }
}

static void setCalibration(int value) //-ve means skip a second each match seconds; +ve means add two seconds each match seconds
{
    LPC_RTC->CALIBRATION = (value >= 0) ? value & 0x1FFFF : -value & 0x1FFFF | 0x20000;
}
int RtcGetCalibration()
{
    int calval = LPC_RTC->CALIBRATION & 0x1FFFF; //27.6.4.2 Calibration register
    int caldir = LPC_RTC->CALIBRATION & 0x20000; //Backward calibration. When CALVAL is equal to the calibration counter, the RTC timers will stop incrementing for 1 second.
    
    return caldir ? -calval : calval;
}
void RtcSetCalibration(int value)
{
    lastSet = 0;
    setCalibration(value);
}
int RtcWholePartIsSet()
{
    return (LPC_RTC->RTC_AUX & 0x10) == 0; //27.6.2.5 RTC Auxiliary control register - RTC Oscillator Fail detect flag
}
int RtcFractionalPartIsSet()
{
    return fractionalPartOfSetTime >= 0;
}

static void stopAndReset()
{
    LPC_RTC->CCR      =  0x12; //Stop the clock (CLKEN bit 0 = 0); reset the divider (CTCRST bit 1 = 1); stop and reset the calibration counter (CCALEN bit 4 = 1)
    LPC_TIM1->TCR     =     2; //Clear the fractional part count    - 21.6.2 Timer Control Register - Reset  TC and PC.
}
static void release()
{
    LPC_RTC->CCR      =     1; //Start the clock (CLKEN bit 0 = 1); release the divider (CTCRST bit 1 = 0); release the calibration counter (CCALEN bit 4 = 0)
    LPC_TIM1->TCR     =     1; //Enable the fractional part counter -  21.6.2 Timer Control Register - Enable TC and PC
}
int RtcInit()
{
    stopAndReset();
    
    //Set up the real time clock which handles whole seconds    
    LPC_RTC->AMR       = 0xFF;    //27.6.2.4 Alarm Mask Register - mask all alarms
    LPC_RTC->RTC_AUXEN =    0;    //27.6.2.6 RTC Auxiliary Enable register - mask the oscillator stopped interrupt
    LPC_RTC->ILR       =    3;    //27.6.2.1 Interrupt Location Register - Clear all interrupts
    LPC_RTC->CIIR      =    1;    //27.6.2.3 Counter Increment Interrupt Register - set to interrupt on the second only
    NVIC_SetVector(RTC_IRQn, (uint32_t)&rtcInterrupt);
    NVIC_EnableIRQ(RTC_IRQn);     //Allow the fractional part to be reset at the end of a second including any pending

    //Set up timer 1 to handle the fractional part and leave stopped until the time is set
    int pre = 96000000 / (1 << RTC_RESOLUTION_BITS) - 1;
    int max = (1 << RTC_RESOLUTION_BITS) - 1;
    
    LPC_SC->PCLKSEL0 &= ~0x20;    //  4.7.3 Peripheral Clock Selection - PCLK_peripheral PCLK_TIMER1 00xxxx = CCLK/4; 01xxxx = CCLK
    LPC_SC->PCLKSEL0 |=  0x10;
    LPC_SC->PCONP    |=     4;    //  4.8.9 Power Control for Peripherals register - Timer1 Power On
    LPC_TIM1->CTCR    =     0;    // 21.6.3 Count Control Register - Timer mode
    LPC_TIM1->PR      =   pre;    // 21.6.5 Prescale register      - Prescale 96MHz clock to 1s == 2048 (divide by PR+1).
    LPC_TIM0->MR0     =   max;    // 21.6.7 Match Register 0       - Match count
    LPC_TIM1->MCR     =     0;    // 21.6.8 Match Control Register - Do nothing when matched
    
    fractionalPartOfSetTime = -1; //Label the fractional part as not being set
    lastSet = 0;
    
    release();
   
    return 0;
}
static void getTmUtc(struct tm* ptm)
{
    ptm->tm_sec   =  LPC_RTC->SEC   & 0x03F;         //  6 bits 00 --> 59
    ptm->tm_min   =  LPC_RTC->MIN   & 0x03F;         //  6 bits 00 --> 59
    ptm->tm_hour  =  LPC_RTC->HOUR  & 0x01F;         //  5 bits 00 --> 23
    ptm->tm_mday  =  LPC_RTC->DOM   & 0x01F;         //  5 bits 01 --> 31
    ptm->tm_mon   = (LPC_RTC->MONTH & 0x00F) - 1;    //  4 bits 00 --> 11
    ptm->tm_year  = (LPC_RTC->YEAR  & 0xFFF) - 1900; // 12 bits Years since 1900
    ptm->tm_wday  =  LPC_RTC->DOW   & 0x007;         //  3 bits 0 --> 6 where 0 == Sunday
    ptm->tm_yday  = (LPC_RTC->DOY   & 0x1FF) - 1;    //  9 bits 0 --> 365
    ptm->tm_isdst = -1;                              //  +ve if DST, 0 if not DST, -ve if the information is not available. Note that 'true' evaluates to +1.
}
static void setTmUtc(struct tm* ptm)
{
    LPC_RTC->SEC      = ptm->tm_sec;         // 00 --> 59
    LPC_RTC->MIN      = ptm->tm_min;         // 00 --> 59
    LPC_RTC->HOUR     = ptm->tm_hour;        // 00 --> 23
    LPC_RTC->DOM      = ptm->tm_mday;        // 01 --> 31
    LPC_RTC->MONTH    = ptm->tm_mon  + 1;    // 00 --> 11
    LPC_RTC->YEAR     = ptm->tm_year + 1900; // Years since 1900
    LPC_RTC->DOW      = ptm->tm_wday;        // 0 --> 6 where 0 == Sunday
    LPC_RTC->DOY      = ptm->tm_yday + 1;    // 0 --> 365
    LPC_RTC->RTC_AUX  = 0x10;                // Record the RTC is set - 27.6.2.5 RTC Auxiliary control register - RTC Oscillator Fail detect flag - Writing a 1 to this bit clears the flag.
}
uint64_t RtcGet()
{
    if (!RtcWholePartIsSet()) return 0;     //Bomb out with a value of zero if the RTC is not set

    struct tm tm;
    int fraction;
    
    getTmUtc(&tm);                          //Read the time and fraction
    fraction = LPC_TIM1->TC;
    
    if ((LPC_RTC->SEC & 0x3F) != tm.tm_sec) //Check if the clock has incremented
    {
        getTmUtc(&tm);                      //If so then read the time again
        fraction = LPC_TIM1->TC;
    }
    
    uint64_t t = TimeFromTmUtc(&tm);
    t <<= RTC_RESOLUTION_BITS;             //Move the seconds to the left of the decimal point
    if (RtcFractionalPartIsSet())          //Add remaining fraction of a second if it has been set
    {
        t += fractionalPartOfSetTime;
    }
    t += fraction;                        //Add the fractional part - 21.6.4 Timer Counter register
    
    return t;
}

void RtcGetTmUtc(struct tm* ptm)
{
    getTmUtc(ptm);
    if ((LPC_RTC->SEC & 0x3F) != ptm->tm_sec) getTmUtc(ptm);
}
static void adjustCalibration(uint64_t thisSet)
{
     int                   K = 4;
     int32_t     calibration = RtcGetCalibration();                             //Work out how many calibration seconds were added; -ve means skip a second each match seconds; +ve means add two seconds each match seconds
     
    uint64_t         thisRtc = RtcGet();                                        //Get the current RTC
    uint64_t      elapsedAct = thisSet - lastSet;                               //Get the actual elapsed time (from NTP)
    uint32_t  elapsedSeconds = elapsedAct >> RTC_RESOLUTION_BITS;

     int32_t    addedSeconds;
     if      (calibration > 0) addedSeconds =   elapsedSeconds /  calibration;  //Find the number of whole seconds that will have been added or removed. Use actual seconds as rtc depends on this outcome - catch 22
     else if (calibration < 0) addedSeconds = -(elapsedSeconds / -calibration);
     else                      addedSeconds = 0;
     
     int64_t        addedRtc = addedSeconds << RTC_RESOLUTION_BITS;
    uint64_t      elapsedRtc = thisRtc - lastSet - addedRtc;                    //Get the true elapsed RTC time by removing any added seconds
     int64_t       behindRtc = elapsedAct - elapsedRtc;                         //Get the difference between the theo (actual) and the rtc
    LogF("thisSet=%llu; thisRtc=%llu; lastSet=%llu\r\n",thisSet, thisRtc, lastSet);
    LogF("Fractional set part=%d; fractional part=%d\r\n", fractionalPartOfSetTime, LPC_TIM1->TC);
    LogF("Cal=%d; elapsedAct=%llu; added=%d; elapsedRtc=%llu; behindRtc=%lld\r\n",calibration, elapsedAct, addedSeconds, elapsedRtc, behindRtc);
    
     int32_t theoCalibration;
     if      (behindRtc > 0) theoCalibration =   elapsedAct /  behindRtc;      //Calculate the theoretical calibration (or +infinity if no difference) from these times
     else if (behindRtc < 0) theoCalibration = -(elapsedAct / -behindRtc);
     else                    theoCalibration = 0x1FFFF;
     
     int32_t  addCalibration = (theoCalibration - calibration) >> K;            //take a part of the difference between theo and actual
                calibration += addCalibration;                                  //Add that part to smooth out the new calibration
    LogF("theo=%d; add=%d; new=%d\r\n\r\n", theoCalibration, addCalibration, calibration);
                
    if (calibration <  100 && calibration >= 0) calibration =  100;             //Do not allow monster adjustments over +/- 1%
    if (calibration > -100 && calibration <  0) calibration = -100;
    setCalibration(calibration);
}

void     RtcSet(uint64_t t)
{
    stopAndReset();
    
    if (lastSet) adjustCalibration(t);
    lastSet = t;
    
    fractionalPartOfSetTime = t & ((1 << RTC_RESOLUTION_BITS) - 1); //Record the remaining fraction of a second not set in the RTC
    int wholeseconds = t >> RTC_RESOLUTION_BITS;                    //Set the RTC to the whole number of seconds in the time
    
    struct tm tm;
    TimeToTmUtc(wholeseconds, &tm);
    setTmUtc(&tm);
    setTmUtc(&tm); //Do it again as sometimes we don't seem to get the right answer
    
    release();
}
void RtcGetTmLocal(struct tm* ptm)
{
    RtcGetTmUtc(ptm);
    TimeTmUtcToLocal(ptm);
}