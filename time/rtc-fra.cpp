#include "mbed.h"
#include  "rtc.h"

int  RtcFractionGet()          { return LPC_TIM1->TC;      } //21.6.4 Timer Counter register
void RtcFractionStopAndResetCounter() { LPC_TIM1->TCR = 2; } //Clear  the fractional part count   - 21.6.2 Timer Control Register - Reset  TC and PC.
void RtcFractionRelease()             { LPC_TIM1->TCR = 1; } //Enable the fractional part counter - 21.6.2 Timer Control Register - Enable TC and PC

void RtcFractionInit()
{
    int pre = 96000000 / (1 << RTC_RESOLUTION_BITS) - 1;
    int max = (1 << RTC_RESOLUTION_BITS) - 1;
    
    LPC_SC->PCLKSEL0 &= ~0x20;    //  4.7.3 Peripheral Clock Selection - PCLK_peripheral PCLK_TIMER1 00xxxx = CCLK/4; 01xxxx = CCLK
    LPC_SC->PCLKSEL0 |=  0x10;
    LPC_SC->PCONP    |=     4;    //  4.8.9 Power Control for Peripherals register - Timer1 Power On
    LPC_TIM1->TCR     =     2;    // 21.6.2 Timer Control Register - Reset  TC and PC.
    LPC_TIM1->CTCR    =     0;    // 21.6.3 Count Control Register - Timer mode
    LPC_TIM1->PR      =   pre;    // 21.6.5 Prescale register      - Prescale 96MHz clock to 1s == 2048 (divide by PR+1).
    LPC_TIM1->MR0     =   max;    // 21.6.7 Match Register 0       - Match count
    LPC_TIM1->MCR     =     0;    // 21.6.8 Match Control Register - Do nothing when matched
    LPC_TIM1->TCR     =     1;    // 21.6.2 Timer Control Register - Enable TC and PC
}