#include       "io.h"
#include "schedule.h"
#include  "ds18b20.h"
#include      "cfg.h"
#include "settings.h"

#define MAX_TEMP_16THS 1600
#define MIN_TEMP_16THS -160

int HeatingRadiatorPump;
int HeatingBoilerPump;
int HeatingBoilerCall;

int HeatingMain()
{
    ScheduleMain();
    
    //Control heating circulation pump
    HeatingRadiatorPump = ScheduleIsCalling;
    
    //Control boiler call
    int tankTemp16ths = DS18B20ValueFromRom(CfgTankRom);
    if (tankTemp16ths < MAX_TEMP_16THS && tankTemp16ths > MIN_TEMP_16THS) //Ignore values which are likely to be wrong
    {
        int  tankUpper16ths = SettingsGetTankSetPoint()   << 4;
        int hysteresis16ths = SettingsGetTankHysteresis() << 4;
        int  tankLower16ths = tankUpper16ths - hysteresis16ths;
    
        if (tankTemp16ths >= tankUpper16ths) HeatingBoilerCall = false;
        if (tankTemp16ths <= tankLower16ths) HeatingBoilerCall = true;
    }
    
    //Control boiler circulation pump
    if ( HeatingBoilerCall) HeatingBoilerPump = true;

    int tankCoilIn16ths  = DS18B20ValueFromRom(CfgTankCoilInRom);
    int tankCoilOut16ths = DS18B20ValueFromRom(CfgTankCoilOutRom);
    if (tankCoilIn16ths < MAX_TEMP_16THS && tankCoilIn16ths > MIN_TEMP_16THS && tankCoilOut16ths < MAX_TEMP_16THS && tankCoilOut16ths > MIN_TEMP_16THS)
    {
        int tankCoilLoss16ths = tankCoilIn16ths - tankCoilOut16ths;
        int boilerStillSupplyingHeat = tankCoilLoss16ths > SettingsGetTankMinHoldOnLoss();
        
        if (!HeatingBoilerCall && !boilerStillSupplyingHeat) HeatingBoilerPump = false;
    }
    
    return 0;
}
int HeatingInit()
{
    ScheduleInit();
    return 0;
}