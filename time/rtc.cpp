#include "mbed.h"
#include  "cfg.h"
#include  "log.h"
#include   "io.h"
#include "time.h"
#include  "rtc.h"

static void rtcInterrupt()
{
    if (LPC_RTC->ILR & 1)
    {
        LPC_TIM1->TCR = 2; // 21.6.2 Timer Control Register - Reset TC and PC
        LPC_TIM1->TCR = 1; // 21.6.2 Timer Control Register - Enable TC and PC
    }
    LPC_RTC->ILR = 3; //27.6.2.1 Interrupt Location Register - Clear all interrupts
}

int RtcGetCalibration()
{
    int calval = LPC_RTC->CALIBRATION & 0x1FFFF; //27.6.4.2 Calibration register
    int caldir = LPC_RTC->CALIBRATION & 0x20000; //Backward calibration. When CALVAL is equal to the calibration counter, the RTC timers will stop incrementing for 1 second.
    
    return caldir ? -calval : calval;
}
void RtcSetCalibration(int match) //+ve means skip a second each match seconds; -ve means add two seconds each match seconds
{
    LPC_RTC->CALIBRATION = (match >= 0) ? match & 0x1FFFF : -match & 0x1FFFF | 0x2FFFF;
}
int RtcNotSet()
{
    return LPC_RTC->RTC_AUX & 0x10; //27.6.2.5 RTC Auxiliary control register - RTC Oscillator Fail detect flag
}
static int fractionalPartOfSetTime; //Initialised to -1 then will be non-negative once the fractional part of the clock is set.
int RtcFractionalPartNotSet()
{
    return fractionalPartOfSetTime < 0;
}
int RtcInit()
{
    //Link the RTC interrupts and leave disabled until the time is set
    NVIC_DisableIRQ(RTC_IRQn);      //Stop the fractional part being reset at the end of a second
    NVIC_SetVector(RTC_IRQn, (uint32_t)&rtcInterrupt);
    
    //Set up the real time clock which handles whole seconds    
    LPC_RTC->CCR       =    1; //27.6.2.2 Clock Control Register - Enable time counter and calibration counter
    LPC_RTC->AMR       = 0xFF; //27.6.2.4 Alarm Mask Register - mask all alarms
    LPC_RTC->RTC_AUXEN =    0; //27.6.2.6 RTC Auxiliary Enable register - mask the oscillator stopped interrupt
    LPC_RTC->ILR       =    3; //27.6.2.1 Interrupt Location Register - Clear all interrupts
    LPC_RTC->CIIR      =    1; //27.6.2.3 Counter Increment Interrupt Register - set to interrupt on the second only
    RtcSetCalibration(CfgClockCalibration);

    //Set up timer 1 to handle the fractional part and leave stopped until the time is set
    int pre = 96000000 / (1 << RTC_RESOLUTION_BITS) - 1;
    int max = (1 << RTC_RESOLUTION_BITS) - 1;
    
    LPC_SC->PCLKSEL0 &= ~0x20; //  4.7.3 Peripheral Clock Selection - PCLK_peripheral PCLK_TIMER1 00xxxx = CCLK/4; 01xxxx = CCLK
    LPC_SC->PCLKSEL0 |=  0x10;
    LPC_SC->PCONP    |=     4; //  4.8.9 Power Control for Peripherals register - Timer1 Power On
    LPC_TIM1->TCR     =     2; // 21.6.2 Timer Control Register - Reset TC and PC
    LPC_TIM1->CTCR    =     0; // 21.6.3 Count Control Register - Timer mode
    LPC_TIM1->PR      =   pre; // 21.6.5 Prescale register      - Prescale 96MHz clock to 1s == 2048 (divide by PR+1).
    LPC_TIM0->MR0     =   max; // 21.6.7 Match Register 0       - Match count
    LPC_TIM1->MCR     =     4; // 21.6.8 Match Control Register - Stop on match 0
    LPC_TIM1->TCR     =     1; // 21.6.2 Timer Control Register - Enable TC and PC
    
    fractionalPartOfSetTime = -1; 
    NVIC_ClearPendingIRQ(RTC_IRQn); //Clear any pending fractional part resets generated before the reset 
    NVIC_EnableIRQ(RTC_IRQn);  //Allow the fractional part to be reset at the end of a second including any pending
   
    return 0;
}
uint64_t RtcGet()
{
    if (RtcNotSet()) return 0;                                                  //Bomb out with a value of zero if the RTC is not set
    uint64_t t = time(NULL);                                                    //Read the time
    NVIC_DisableIRQ(RTC_IRQn);                                                  //Stop the fractional part being reset at the end of a second
    if (time(NULL) > t && LPC_TIM1->TC > (1 << (RTC_RESOLUTION_BITS - 1))) t--; //If the RTC has incremented but the fractional part reset is pending then subtract one  
    t <<= RTC_RESOLUTION_BITS;                                                  //Move the seconds to the left of the decimal point
    if (!RtcFractionalPartNotSet()) t += fractionalPartOfSetTime;               //Add remaining fraction of a second if it has been set
    t += LPC_TIM1->TC;                                                          //Add the fractional part - 21.6.4 Timer Counter register
    NVIC_EnableIRQ(RTC_IRQn);                                                   //Allow the fractional part to be reset at the end of a second including any pending
    return t;
}
void     RtcSet(uint64_t t)
{
    NVIC_DisableIRQ(RTC_IRQn);                                      //Stop the fractional part being reset at the end of a second
    fractionalPartOfSetTime = t & ((1 << RTC_RESOLUTION_BITS) - 1); //Record the remaining fraction of a second not set in the RTC
    set_time(t >> RTC_RESOLUTION_BITS);                             //Set the RTC to the whole number of seconds in the time
    LPC_TIM1->TCR     =    2;                                       //Clear the fractional part count    - 21.6.2 Timer Control Register - Reset  TC and PC
    LPC_TIM1->TCR     =    1;                                       //Enable the fractional part counter -  21.6.2 Timer Control Register - Enable TC and PC
    LPC_RTC->RTC_AUX  = 0x10;                                       //Record the RTC is set - 27.6.2.5 RTC Auxiliary control register - RTC Oscillator Fail detect flag - Writing a 1 to this bit clears the flag.
    NVIC_ClearPendingIRQ(RTC_IRQn);                                 //Clear any pending fractional part resets generated before the reset 
    NVIC_EnableIRQ(RTC_IRQn);                                       //Allow the fractional part to be reset at the end of a second
}
int RtcGetGenReg(int index)
{
    switch (index)
    {
        case 0: return LPC_RTC->GPREG0;
        case 1: return LPC_RTC->GPREG1;
        case 2: return LPC_RTC->GPREG2;
        case 3: return LPC_RTC->GPREG3;
        case 4: return LPC_RTC->GPREG4;
        default:
            LogF("Unknown RtcGetGenReg index %d\r\n", index);
            return 0;
    }
}
void RtcSetGenReg(int index, int value)
{
    switch (index)
    {
        case 0: LPC_RTC->GPREG0 = value; break;
        case 1: LPC_RTC->GPREG1 = value; break;
        case 2: LPC_RTC->GPREG2 = value; break;
        case 3: LPC_RTC->GPREG3 = value; break;
        case 4: LPC_RTC->GPREG4 = value; break;
        default:
            LogF("Unknown RtcSetGenReg index %d\r\n", index);
            break;
    }
}
