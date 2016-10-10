#include          "mbed.h"
#include           "log.h"
#include        "1-wire.h"
#include "1-wire-device.h"
#include       "ds18b20.h"

int16_t DS18B20Value[DEVICE_MAX];

#define MAX_TEMP_16THS 1600
#define MIN_TEMP_16THS -160
bool DS18B20IsValidValue(int16_t value)
{
    return value < MAX_TEMP_16THS && value > MIN_TEMP_16THS;
}
void DS18B20ValueToString(int16_t value, char* buffer)
{
    switch (value)
    {
        case DS18B20_ERROR_CRC:                     strcpy (buffer, "CRC error"                     ); break;
        case DS18B20_ERROR_NOT_FOUND:               strcpy (buffer, "ROM not found"                 ); break;
        case DS18B20_ERROR_TIMED_OUT:               strcpy (buffer, "Timed out"                     ); break;
        case DS18B20_ERROR_NO_DEVICE_PRESENT:       strcpy (buffer, "No device detected after reset"); break;
        case DS18B20_ERROR_NO_DEVICE_PARTICIPATING: strcpy (buffer, "Device removed during search"  ); break;
        default:                                    sprintf(buffer, "%1.1f", value / 16.0           ); break;
    }
}
int16_t DS18B20ValueFromRom(char* rom)
{
    for (int device = 0; device < DeviceCount; device++) if (memcmp(DeviceList + 8 * device, rom, 8) == 0) return DS18B20Value[device];
    return DS18B20_ERROR_NOT_FOUND;
}

void DS18B20ReadValue(int oneWireResult, int device, char byte0, char byte1)
{
    switch (oneWireResult)
    {
        case ONE_WIRE_RESULT_OK:
            DS18B20Value[device] = byte1;
            DS18B20Value[device] <<= 8;
            DS18B20Value[device] |= byte0;
            break;
        case ONE_WIRE_RESULT_CRC_ERROR:               DS18B20Value[device] = DS18B20_ERROR_CRC;                     break;
        case ONE_WIRE_RESULT_NO_DEVICE_PRESENT:       DS18B20Value[device] = DS18B20_ERROR_NO_DEVICE_PRESENT;       break;
        case ONE_WIRE_RESULT_TIMED_OUT:               DS18B20Value[device] = DS18B20_ERROR_TIMED_OUT;               break;            
        case ONE_WIRE_RESULT_NO_DEVICE_PARTICIPATING: DS18B20Value[device] = DS18B20_ERROR_NO_DEVICE_PARTICIPATING; break;
        default:
            LogF("Unknown OneWireResult %d\r\n", OneWireResult());
            break;
    }
}