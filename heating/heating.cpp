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
    ProgramInit();
    return 0;
}