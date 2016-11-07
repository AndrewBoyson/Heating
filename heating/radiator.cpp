#include       "io.h"
#include  "program.h"
#include  "ds18b20.h"
#include      "cfg.h"
#include     "fram.h"

static char    hallRom[8];       static int iHallRom;

static int32_t nightTemperature; static int iNightTemperature;
static int32_t frostTemperature; static int iFrostTemperature;

char* RadiatorGetHallRom         (){ return       hallRom;          } 
int   RadiatorGetNightTemperature(){ return (int) nightTemperature; } 
int   RadiatorGetFrostTemperature(){ return (int) frostTemperature; } 

void RadiatorSetHallRom          ( char* value) { memcpy(hallRom,  value, 8);        FramWrite(iHallRom,          8,  hallRom         ); }
void RadiatorSetNightTemperature ( int   value) { nightTemperature = (int32_t)value; FramWrite(iNightTemperature, 4, &nightTemperature); }
void RadiatorSetFrostTemperature ( int   value) { frostTemperature = (int32_t)value; FramWrite(iFrostTemperature, 4, &frostTemperature); }


int RadiatorInit()
{
    int address;
    int32_t def4;
               address = FramLoad( 8,  hallRom,           NULL); if (address < 0) return -1; iHallRom          = address;
    def4 = 15; address = FramLoad( 4, &nightTemperature, &def4); if (address < 0) return -1; iNightTemperature = address; 
    def4 =  8; address = FramLoad( 4, &frostTemperature, &def4); if (address < 0) return -1; iFrostTemperature = address; 
    
    return 0;
}

int RadiatorMain()
{
    //Control heating circulation pump
    int  hallTemp16ths = DS18B20ValueFromRom(hallRom);
    int nightTemp16ths = nightTemperature << 4;
    int frostTemp16ths = frostTemperature << 4;
    
    if (DS18B20IsValidValue(hallTemp16ths))
    {
        if (ProgramGetAuto())
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