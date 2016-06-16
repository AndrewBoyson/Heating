#include "mbed.h"
#include   "io.h"
#include  "log.h"

static DigitalInOut pin(p25, PIN_INPUT, PullUp, 0);

volatile int OneWireBusValue;
int OneWireBusBusy()
{
    return LPC_TIM0->TCR;
}

static void timer0_IRQHandler(void)
{
    //Handle match channel 0
    if (LPC_TIM0->IR & 1)
    {
        LPC_TIM0->IR = 1; //Writing a logic one to the corresponding IR bit will reset the interrupt. Writing a zero has no effect. See 21.6.1.
        pin.input();
    }
    //Handle match channel 1
    if (LPC_TIM0->IR & 2)
    {
        LPC_TIM0->IR = 2;
        OneWireBusValue = pin.read();
    }
    //Handle match channel 2
    if (LPC_TIM0->IR & 4)
    {
        LPC_TIM0->IR = 4;
        pin.output();
        pin = 1;
    }
    //Handle match channel 3
    if (LPC_TIM0->IR & 8)
    {
        LPC_TIM0->IR = 8;
        pin.input();
    }
}


static void start(uint32_t usStartBusFloat, uint32_t usReadBusValue, uint32_t usStartBusDriveHigh, uint32_t usReleaseBus)
{    
    //Drop out if there is no end specified
    if (!usReleaseBus) return;
    
    //Prevent spurious interrupts and reset the clock to prevent a match 
    LPC_TIM0->MCR  = 0;   // 21.6.8 Match Control Register - Do nothing
    LPC_TIM0->TCR  = 2;   // 21.6.2 Timer Control Register - Reset TC and PC
    
    //Set up the match registers
    uint32_t us = 0;
    us += usStartBusFloat;     LPC_TIM0->MR0 = us; // 21.6.7 Match Register 0 - Match count
    us += usReadBusValue;      LPC_TIM0->MR1 = us; // 21.6.7 Match Register 1 - Match count
    us += usStartBusDriveHigh; LPC_TIM0->MR2 = us; // 21.6.7 Match Register 2 - Match count
    us += usReleaseBus;        LPC_TIM0->MR3 = us; // 21.6.7 Match Register 3 - Match count
    
    //Set up whether to interrupt or not.
                             LPC_TIM0->MCR  = 05000; // 21.6.8 Match Control Register - Stop (LPC_TIM0->TCR = 0) and Interrupt on match 3. The stop prevents a reset from occuring.
    if (usStartBusFloat    ) LPC_TIM0->MCR |= 00001; // 21.6.8 Match Control Register - Interrupt on match 0
    if (usReadBusValue     ) LPC_TIM0->MCR |= 00010; // 21.6.8 Match Control Register - Interrupt on match 1
    if (usStartBusDriveHigh) LPC_TIM0->MCR |= 00100; // 21.6.8 Match Control Register - Interrupt on match 2
       
    //Drive the bus low
    pin.output();
    pin = 0;
    
    //Start the timer
    LPC_TIM0->TCR  = 1;     // 21.6.2 Timer Control Register - Enable TC and PC    
}

int OneWireBusInit()
{
    NVIC_SetVector  (TIMER0_IRQn,(uint32_t)&timer0_IRQHandler);
    LPC_SC->PCLKSEL0 &= ~0x8; //  4.7.3 Peripheral Clock Selection - PCLK_peripheral PCLK_TIMER0 00xx = CCLK/4; 01xx = CCLK
    LPC_SC->PCLKSEL0 |=  0x4;
    LPC_SC->PCONP    |=    2; //  4.8.9 Power Control for Peripherals register - Timer0 Power On
    LPC_TIM0->TCR     =    2; // 21.6.2 Timer Control Register - Reset TC and PC
    LPC_TIM0->CTCR    =    0; // 21.6.3 Count Control Register - Timer mode
    LPC_TIM0->PR      =   95; // 21.6.5 Prescale register      - Prescale 96MHz clock to 1Mhz (divide by PR+1). When PC is equal to this value, the next clock increments the TC and clears the PC
    LPC_TIM0->MCR     =    0; // 21.6.8 Match Control Register - Do nothing
    LPC_TIM0->TCR     =    0; // 21.6.2 Timer Control Register - Disable TC and PC
    NVIC_EnableIRQ(TIMER0_IRQn); 
    return 0;
}

//Delays
#define    RESET_BUS_LOW_US  480
#define    READ_PRESENCE_US   70
#define    RESET_RELEASE_US  410

#define  WRITE_0_BUS_LOW_US   60
#define  WRITE_0_RELEASE_US   10

#define  WRITE_1_BUS_LOW_US    6
#define  WRITE_1_RELEASE_US   64

#define READ_BIT_BUS_LOW_US    6
#define         READ_BIT_US    9
#define READ_BIT_RELEASE_US   55

void OneWireBusReset()
{
    start(RESET_BUS_LOW_US, READ_PRESENCE_US, 0, RESET_RELEASE_US); 
}
void OneWireBusWriteBitWithPullUp(int bit, int pullupms)
{
    if (pullupms)
    {
        if (bit) start(0, 0, WRITE_1_BUS_LOW_US, pullupms * 1000);
        else     start(0, 0, WRITE_0_BUS_LOW_US, pullupms * 1000);
    }
    else
    {
        if (bit) start(WRITE_1_BUS_LOW_US, 0, 0, WRITE_1_RELEASE_US);
        else     start(WRITE_0_BUS_LOW_US, 0, 0, WRITE_0_RELEASE_US);
    }
}
void OneWireBusWriteBit(int bit)
{
    OneWireBusWriteBitWithPullUp(bit, 0);
}
void OneWireBusReadBit()
{
    start(READ_BIT_BUS_LOW_US, READ_BIT_US, 0, READ_BIT_RELEASE_US);
}

