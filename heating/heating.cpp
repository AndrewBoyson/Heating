#include  "program.h"
#include "radiator.h"
#include   "boiler.h"

int HeatingMain()
{
    ProgramMain();
    RadiatorMain();
    BoilerMain();
    
    return 0;
}
int HeatingInit()
{
    BoilerInit();
    RadiatorInit();
    ProgramInit();
    
    return 0;
}
