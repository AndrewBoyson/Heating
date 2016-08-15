#include "schedule.h"
#include "radiator.h"
#include   "boiler.h"

int HeatingMain()
{
    ScheduleMain();
    RadiatorMain();
    BoilerMain();
    
    return 0;
}
int HeatingInit()
{
    ScheduleInit();
    return 0;
}