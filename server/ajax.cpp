#include "mbed.h"
#include "cfg.h"
#include "ds18b20.h"
#include "response.h"

int Ajax(int chunk)
{
    int posn = -1;
    if (++posn == chunk)
    {
        ResponseAddF("%x, %x, %x, %x", DS18B20ValueFromRom(CfgTankRom), DS18B20ValueFromRom(CfgBoilerOutputRom), DS18B20ValueFromRom(CfgBoilerReturnRom), DS18B20ValueFromRom(CfgHallRom));
        return RESPONSE_SEND_CHUNK;
    }
    return RESPONSE_NO_MORE_CHUNKS;
}