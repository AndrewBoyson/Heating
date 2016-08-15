#include       "io.h"
#include "schedule.h"
#include  "ds18b20.h"
#include      "cfg.h"
#include "settings.h"

int RadiatorPump;

int RadiatorMain()
{
    //Control heating circulation pump
    int  hallTemp16ths = DS18B20ValueFromRom(CfgTankRom);
    int nightTemp16ths = SettingsGetNightTemperature() << 4;
    int frostTemp16ths = SettingsGetFrostTemperature() << 4;
    
    if (DS18B20IsValidValue(hallTemp16ths))
    {
        if (ScheduleAuto)
        {
            RadiatorPump = ScheduleIsCalling || hallTemp16ths < nightTemp16ths;
        }
        else
        {
            RadiatorPump = hallTemp16ths < frostTemp16ths;
        }
    }
    return 0;
}