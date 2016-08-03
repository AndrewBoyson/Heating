#include       "io.h"
#include "schedule.h"

int HeatingMain()
{
    ScheduleMain();
        
    Led1 = ScheduleIsCalling;
    
    return 0;
}
int HeatingInit()
{
    ScheduleInit();
    return 0;
}