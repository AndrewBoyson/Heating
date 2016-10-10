#include     "mbed.h"
#include "settings.h"
#include  "ds18b20.h"
#include "response.h"

int Ajax(int chunk)
{
    int posn = -1;
    if (++posn == chunk)
    {
        ResponseAddF("%x, %x, %x, %x", DS18B20ValueFromRom(SettingsGetTankRom()),
                                       DS18B20ValueFromRom(SettingsGetBoilerOutputRom()),
                                       DS18B20ValueFromRom(SettingsGetBoilerReturnRom()),
                                       DS18B20ValueFromRom(SettingsGetHallRom()));
        return RESPONSE_SEND_CHUNK;
    }
    return RESPONSE_NO_MORE_CHUNKS;
}