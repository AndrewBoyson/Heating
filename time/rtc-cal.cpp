#include     "mbed.h"
#include      "log.h"
#include "settings.h"
#include      "rtc.h"
#include     "time.h"

#define ALLOW true

static uint64_t prev = 0; //Set when time is set. Used to determine calibration value.
static struct tm prevTm;  //Set when time is set or a second is handled. Used to determine if a second has been skipped or added.
    

static int  addedSeconds = 0; //Incremented and decremented in calculateSeconds and zeroed after use in adjustCalibration

static int calval = 0; //-ve means skip a second each match seconds; +ve means add two seconds each match seconds
static int calcnt = 0; //Incremented each second. Used to calculate the effective calibration value.
static int calfra = 0; //Calculated each second or whenever calcnt is updated
static void setCalibration(int value)
{
    int neg = value < 0;
    int val = neg ? -value : value;
 
    if (val > 0x1FFFF) { val = 0; neg = false; }  //Big values are set to infinity which is represented by 0;
    
    LPC_RTC->CALIBRATION = neg ?  val | 0x20000 : val;
    calval               = neg ? -val           : val;
    
    LogF("Calibration set to %06d (%05X)\r\n", LPC_RTC->CALIBRATION, LPC_RTC->CALIBRATION);
}
static int getCalibration()
{
    int val = LPC_RTC->CALIBRATION & 0x1FFFF; //27.6.4.2 Calibration register
    int neg = LPC_RTC->CALIBRATION & 0x20000; //Backward calibration. When CALVAL is equal to the calibration counter, the RTC timers will stop incrementing for 1 second.
    
    return neg ? -val : val;
}
static void updateSeconds(struct tm* ptm, int elapsedTim)
{
    //Define midpoints between one and two seconds - note that '-' is higher precedence than '<<'
    const int         A_HALF_SECOND  = 1 << RTC_RESOLUTION_BITS - 1;
    const int ONE_AND_A_HALF_SECONDS = 3 << RTC_RESOLUTION_BITS - 1;
    const int TWO_AND_A_HALF_SECONDS = 5 << RTC_RESOLUTION_BITS - 1;
                
    //Work out the RTC elapsed time
    int elapsedRtc = TimeFromTmUtc(ptm) - TimeFromTmUtc(&prevTm);
    
    //Compare the RTC elapsed time with the TIM1 elapsed time.
    switch (elapsedRtc)
    {
        case 1:
            if      (elapsedTim <         A_HALF_SECOND ) { LogTimeCrLf("RTC 1; TIM 0" );                             } //<1 seconds elapsed.
            else if (elapsedTim < ONE_AND_A_HALF_SECONDS) {                                               calcnt++;   } // 1 second  elapsed. This is normal.
            else if (elapsedTim < TWO_AND_A_HALF_SECONDS) { LogTimeCrLf("RTC 1; TIM 2" ); addedSeconds--; calcnt = 0; } // 2 seconds elapsed. A second has been skipped from the RTC
            else                                          { LogTimeCrLf("RTC 1; TIM 3+");                             } //>2 seconds elapsed.
            break;
            
        case 2:
            if      (elapsedTim <         A_HALF_SECOND ) { LogTimeCrLf("RTC 2; TIM 0" );                             } //<1 seconds elapsed.
            else if (elapsedTim < ONE_AND_A_HALF_SECONDS) { LogTimeCrLf("RTC 2; TIM 1" ); addedSeconds++; calcnt = 0; } // 1 second  elapsed. A second has been added to the RTC
            else if (elapsedTim < TWO_AND_A_HALF_SECONDS) { LogTimeCrLf("RTC 2; TIM 2" );                             } // 2 seconds elapsed. An interrupt has been missed
            else                                          { LogTimeCrLf("RTC 2; TIM 3+");                             } //>2 seconds elapsed.
            break;
            
        default:
            LogTimeF("RTC %d\r\n", elapsedRtc);
            break;
    }
    
    //Work out the fraction to add
    calfra = (calcnt << RTC_RESOLUTION_BITS) / calval; //count is 17 bit and resolution is 11 bits giving 28 bits for both so an int is fine.
}
static float getCalRate()
{
    return calval ? 1.0f / calval : 0.0f; //RTC uses a zero calval to disable cal adjustments (== infinity) so the rate to return is 0.0f
}
static void setCalRate(float value)
{
    if (value >  0.01) value =  0.01f;             //Do not allow monster adjustments over 1%
    if (value < -0.01) value = -0.01f;
    bool nearZeroRate = abs(value) < 1.0e-6f;
    int32_t newcal = nearZeroRate ? 0 : 1 / value; //For very small rates set cal (which is 1 / rate) to zero which the RTC uses to represent infinity
    setCalibration(newcal);
}
static void adjustCalibration(uint64_t rtc, uint64_t act)
{
    if (!ALLOW) return;
    
    LogF("calval=%d; calcnt=%d; added=%d\r\n", calval, calcnt, addedSeconds);

    //rtc -= addedSeconds << RTC_RESOLUTION_BITS; //Remove any added seconds to give the rtc as it would have been
    addedSeconds = 0;

    uint64_t interval = rtc - prev;
     int64_t     loss = act - rtc;
     
    float  lossRate = (float)loss / interval;    // 246 / (3600 * 2048) = 3.3 E-5. Need to move about e9 or 3 x 2^10 = 30 bits to get the resolution required.
    float   newRate = getCalRate() + lossRate / SettingsGetClockCalDivisor();
    
    setCalRate(newRate);
     
    LogF("act=%llu; rtc=%llu; prev=%llu;\r\n", act, rtc, prev);
    LogF("interval=%llu; loss=%lld; calval=%d\r\n", interval, loss, calval);
    
    calcnt = 0;
    calfra = 0;
}
void RtcCalStopAndResetCounter() { LPC_RTC->CCR |=  0x10; } //stop and reset the calibration counter (CCALEN bit 4 = 1)
void RtcCalReleaseCounter()      { LPC_RTC->CCR &= ~0x10; } //release        the calibration counter (CCALEN bit 4 = 0)

void RtcCalSecondsHandler(struct tm* ptm, int fraction)
{
    if (prev) updateSeconds(ptm, fraction);
    else       addedSeconds = 0;
    
    prevTm = *ptm;
}
void RtcCalTimeSetHandler(uint64_t rtc, uint64_t act, struct tm* ptm) //'rtc' is from before the RTC is set to actual; 'act' and 'ptm' are what the RTC has been set to.
{     
    if (prev) adjustCalibration(rtc, act);
    prev   = act;
    prevTm = *ptm;
}
int RtcCalGetFraction()
{
    return calfra;
}
int RtcCalGet()
{
    return calval;
}
void RtcCalSet(int value)
{
    setCalibration(value);
}
void RtcCalInit()
{
    RtcCalStopAndResetCounter();
    calval = getCalibration();
    prev = 0;
    RtcCalReleaseCounter();
}
