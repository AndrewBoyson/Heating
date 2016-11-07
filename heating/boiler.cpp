#include       "io.h"
#include  "ds18b20.h"
#include      "cfg.h"
#include     "fram.h"

static char    tankRom[8];   static int iTankRom;
static char    outputRom[8]; static int iOutputRom;
static char    returnRom[8]; static int iReturnRom;


static int32_t tankSetPoint;   static int iTankSetPoint;
static int32_t tankHysteresis; static int iTankHysteresis;
static int32_t runOnResidual;  static int iRunOnResidual;
static int32_t runOnTime;      static int iRunOnTime;

char* BoilerGetTankRom       () { return       tankRom;        } 
char* BoilerGetOutputRom     () { return       outputRom;      } 
char* BoilerGetReturnRom     () { return       returnRom;      } 
int   BoilerGetTankSetPoint  () { return (int) tankSetPoint;   } 
int   BoilerGetTankHysteresis() { return (int) tankHysteresis; } 
int   BoilerGetRunOnResidual () { return (int) runOnResidual;  } 
int   BoilerGetRunOnTime     () { return (int) runOnTime;      } 

void BoilerSetTankRom        (char* value) { memcpy(tankRom,   value, 8);      FramWrite(iTankRom,         8,  tankRom        ); }
void BoilerSetOutputRom      (char* value) { memcpy(outputRom, value, 8);      FramWrite(iOutputRom,       8,  outputRom      ); }
void BoilerSetReturnRom      (char* value) { memcpy(returnRom, value, 8);      FramWrite(iReturnRom,       8,  returnRom      ); }
void BoilerSetTankSetPoint   (int   value) { tankSetPoint    = (int32_t)value; FramWrite(iTankSetPoint,    4, &tankSetPoint   ); }
void BoilerSetTankHysteresis (int   value) { tankHysteresis  = (int32_t)value; FramWrite(iTankHysteresis,  4, &tankHysteresis ); }
void BoilerSetRunOnResidual  (int   value) { runOnResidual   = (int32_t)value; FramWrite(iRunOnResidual,   4, &runOnResidual  ); }
void BoilerSetRunOnTime      (int   value) { runOnTime       = (int32_t)value; FramWrite(iRunOnTime,       4, &runOnTime      ); }


int BoilerInit()
{
    int address;
    int32_t def4;
                address = FramLoad( 8,  tankRom,         NULL); if (address < 0) return -1; iTankRom        = address;
                address = FramLoad( 8,  outputRom,       NULL); if (address < 0) return -1; iOutputRom      = address;
                address = FramLoad( 8,  returnRom,       NULL); if (address < 0) return -1; iReturnRom      = address;
    def4 =  80; address = FramLoad( 4, &tankSetPoint,   &def4); if (address < 0) return -1; iTankSetPoint   = address; 
    def4 =   5; address = FramLoad( 4, &tankHysteresis, &def4); if (address < 0) return -1; iTankHysteresis = address; 
    def4 =   2; address = FramLoad( 4, &runOnResidual,  &def4); if (address < 0) return -1; iRunOnResidual  = address; 
    def4 = 360; address = FramLoad( 4, &runOnTime,      &def4); if (address < 0) return -1; iRunOnTime      = address; 
    
    return 0;
}

int BoilerMain()
{
    //Control boiler call
    int tankTemp16ths = DS18B20ValueFromRom(tankRom);
    if (DS18B20IsValidValue(tankTemp16ths)) //Ignore values which are likely to be wrong
    {
        int  tankUpper16ths = tankSetPoint   << 4;
        int hysteresis16ths = tankHysteresis << 4;
        int  tankLower16ths = tankUpper16ths - hysteresis16ths;
    
        if (tankTemp16ths >= tankUpper16ths) BoilerCall = false;
        if (tankTemp16ths <= tankLower16ths) BoilerCall = true;
    }
    
    //Control boiler circulation pump
    int  boilerOutput16ths   = DS18B20ValueFromRom(outputRom);
    int  boilerReturn16ths   = DS18B20ValueFromRom(returnRom);
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
        if (offTimer.read() > runOnTime) BoilerPump = false;
        if (boilerTempsAreValid)
        {
            int boilerStillSupplyingHeat = boilerResidual16ths > runOnResidual;
            if (!boilerStillSupplyingHeat) BoilerPump = false;
        }
    }
    return 0;   
}