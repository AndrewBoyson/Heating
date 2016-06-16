#include "mbed.h"

#define VREF 3.32

DigitalOut Led1(LED1);
DigitalOut Led2(LED2);
DigitalOut Led3(LED3);
DigitalOut Led4(LED4);

AnalogIn Bat(p15);

float IoVbat()
{
    return Bat * VREF;
}

int IoInit()
{
    Led1 = 0; Led2 = 0; Led3 = 0; Led4 = 0;
    return 0;
}