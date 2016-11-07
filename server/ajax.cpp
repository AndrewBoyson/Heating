#include     "mbed.h"
#include   "boiler.h"
#include "radiator.h"
#include  "ds18b20.h"
#include "response.h"

int Ajax(int chunk)
{
    int posn = -1;
    if (++posn == chunk)
    {
        ResponseAddF("%x, %x, %x, %x", DS18B20ValueFromRom(BoilerGetTankRom()),
                                       DS18B20ValueFromRom(BoilerGetOutputRom()),
                                       DS18B20ValueFromRom(BoilerGetReturnRom()),
                                       DS18B20ValueFromRom(RadiatorGetHallRom()));
        return RESPONSE_SEND_CHUNK;
    }
    return RESPONSE_NO_MORE_CHUNKS;
}