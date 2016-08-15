#include       "io.h"
#include  "ds18b20.h"
#include      "cfg.h"
#include "settings.h"

int BoilerPump;
int BoilerCall;

int BoilerMain()
{
        
    //Control boiler call
    int tankTemp16ths = DS18B20ValueFromRom(CfgTankRom);
    if (DS18B20IsValidValue(tankTemp16ths)) //Ignore values which are likely to be wrong
    {
        int  tankUpper16ths = SettingsGetTankSetPoint()   << 4;
        int hysteresis16ths = SettingsGetTankHysteresis() << 4;
        int  tankLower16ths = tankUpper16ths - hysteresis16ths;
    
        if (tankTemp16ths >= tankUpper16ths) BoilerCall = false;
        if (tankTemp16ths <= tankLower16ths) BoilerCall = true;
    }
    
    //Control boiler circulation pump
    int boilerOutput16ths = DS18B20ValueFromRom(CfgBoilerOutputRom);
    int boilerReturn16ths = DS18B20ValueFromRom(CfgBoilerReturnRom);
    int boilerResidual16ths = boilerOutput16ths - boilerReturn16ths;
    bool boilerTempsAreValid =  DS18B20IsValidValue(boilerOutput16ths) && DS18B20IsValidValue(boilerReturn16ths);


    static Timer offTimer;
    if (BoilerCall)
    {
        BoilerPump = true;
        offTimer.stop();
        offTimer.reset();
    }
    else
    {
        offTimer.start();
        if (offTimer.read() > SettingsGetBoilerRunOnTime()) BoilerPump = false;
        if (boilerTempsAreValid)
        {
            int boilerStillSupplyingHeat = boilerResidual16ths > SettingsGetBoilerRunOnResidual();
            if (!boilerStillSupplyingHeat) BoilerPump = false;
        }
    }
    return 0;   
}