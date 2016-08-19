#include       "io.h"
#include  "program.h"
#include  "ds18b20.h"
#include      "cfg.h"
#include "settings.h"

int RadiatorMain()
{
    //Control heating circulation pump
    int  hallTemp16ths = DS18B20ValueFromRom(SettingsGetTankRom());
    int nightTemp16ths = SettingsGetNightTemperature() << 4;
    int frostTemp16ths = SettingsGetFrostTemperature() << 4;
    
    if (DS18B20IsValidValue(hallTemp16ths))
    {
        if (ProgramAuto)
        {
            RadiatorPump = ProgramIsCalling || hallTemp16ths < nightTemp16ths ||  hallTemp16ths < frostTemp16ths;
        }
        else
        {
            RadiatorPump = hallTemp16ths < frostTemp16ths;
        }
    }
    return 0;
}