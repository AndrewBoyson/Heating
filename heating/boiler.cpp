#include       "io.h"
#include  "ds18b20.hpp"
#include      "cfg.h"
#include "settings.h"

int BoilerMain()
{
    //Control boiler call
    int tankTemp16ths = DS18B20ValueFromRom(SettingsGetTankRom());
    if (DS18B20IsValidValue(tankTemp16ths)) //Ignore values which are likely to be wrong
    {
        int  tankUpper16ths = SettingsGetTankSetPoint()   << 4;
        int hysteresis16ths = SettingsGetTankHysteresis() << 4;
        int  tankLower16ths = tankUpper16ths - hysteresis16ths;
    
        if (tankTemp16ths >= tankUpper16ths) BoilerCall = false;
        if (tankTemp16ths <= tankLower16ths) BoilerCall = true;
    }
    
    //Control boiler circulation pump
    int  boilerOutput16ths   = DS18B20ValueFromRom(SettingsGetBoilerOutputRom());
    int  boilerReturn16ths   = DS18B20ValueFromRom(SettingsGetBoilerReturnRom());
    int  boilerResidual16ths = boilerOutput16ths - boilerReturn16ths;
    bool boilerTempsAreValid = DS18B20IsValidValue(boilerOutput16ths) && DS18B20IsValidValue(boilerReturn16ths);

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