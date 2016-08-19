#include    "mbed.h"
#include  "1-wire.h"
#include     "log.h"
#include      "io.h"
#include    "wifi.h"
#include "ds18b20.h"

#define DEBUG false //Set this to true to add debug messages to the log

#define DEVICE_MAX 8

#define SEND_BUFFER_LENGTH 10
#define RECV_BUFFER_LENGTH 10
static char send[SEND_BUFFER_LENGTH];
static char recv[RECV_BUFFER_LENGTH];
static int sendlen = 0;
static int recvlen = 0;

int DS18B20ScanMs = 0;


int     DS18B20DeviceCount = 0;
char    DS18B20DeviceList[DEVICE_MAX * 8];
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
char* DS18B20DeviceAddress(int device)
{
    return DS18B20DeviceList + device * 8;
}
void DS18B20AddressToString(char* pAddress, char* pBuffer)
{
    char *pAddressEnd = pAddress + 8;
    for (char* p = pAddress; p < pAddressEnd; p++)
    {
        char highnibble = *p >> 4; //Unsigned so fills with zeros
        char  lownibble = *p & 0xF;
        *pBuffer++ = highnibble < 0xA ? highnibble + '0' : highnibble - 0xA + 'A'; //Replace high nibble with its ascii equivalent
        *pBuffer++ =  lownibble < 0xA ?  lownibble + '0' :  lownibble - 0xA + 'A'; //Replace low nibble with its ascii equivalent
        *pBuffer++ = p < pAddressEnd - 1 ? ' ' : 0;                                              //Put in a space between the bytes or a NUL at the end of the last one
    }
}
int DS18B20ParseAddress(char* pText, char *pAddress)
{
    if (strlen(pText) != DS18B20_ADDRESS_STRING_LENGTH - 1)
    {
        LogF("Error parsing device address - wrong length");
        return -1; 
    }
    for (int i = 0; i < 8; i++) pAddress[i] = (char)strtoul(pText, &pText, 16);
    return 0;
}

int16_t DS18B20ValueFromRom(char* rom)
{
    for (int device = 0; device < DS18B20DeviceCount; device++) if (memcmp(DS18B20DeviceList + 8 * device, rom, 8) == 0) return DS18B20Value[device];
    return DS18B20_ERROR_NOT_FOUND;
}
char rom[8];
int allRomsFound = false;
static void searchRom(int first)
{
    sendlen = 1;
    send[0] = 0xF0; //Search Rom
    recvlen = 0;
    for (int i = 0; i < recvlen; i++) recv[i] = 0;
    if (first)  OneWireSearch(send[0], rom, &allRomsFound);
    else        OneWireSearch(send[0], NULL, NULL);
}
static void readScratchpad(int device)
{   
    sendlen = 10;
    send[0] = 0x55; //Match Rom
    for (int i = 0; i < 8; i++) send[i+1] = DS18B20DeviceList[device * 8 + i];
    send[9] = 0xBE; //Read Scratchpad
    recvlen = 9;
    for (int i = 0; i < recvlen; i++) recv[i] = 0;
    OneWireExchange(sendlen, recvlen, send, recv, 0);
}
static void convertT()
{
    sendlen = 2;
    send[0] = 0xCC; //Skip Rom
    send[1] = 0x44; //Convert T
    recvlen = 0;
    for (int i = 0; i < recvlen; i++) recv[i] = 0;
    OneWireExchange(sendlen, recvlen, send, recv, 750);
}
#define IDLE                0
#define LIST_FIRST_DEVICE   1
#define LIST_NEXT_DEVICE    2
#define CONVERT_T           3
#define READ_SCRATCH_PAD    4
#define EXTRACT_TEMPERATURE 5
static volatile int state = IDLE;
int DS18B20Busy() { return state; }
static int handlestate()
{
    if (OneWireBusy()) return 0;
    static Timer scanTimer;
    static int device;
    switch (state)
    {
        case IDLE:
            //Establish the scan time
            int scanMs;
            scanMs = scanTimer.read_ms();
            scanTimer.reset();
            scanTimer.start();
            int diffMs;
            diffMs = scanMs - DS18B20ScanMs;
            if (diffMs > 0xF || diffMs < 0xF)
            {
                DS18B20ScanMs += (scanMs - DS18B20ScanMs) >> 4;
            }
            else
            {
                if (scanMs > DS18B20ScanMs) DS18B20ScanMs++;
                if (scanMs < DS18B20ScanMs) DS18B20ScanMs--;
            }

            state = LIST_FIRST_DEVICE;
            break;
        case LIST_FIRST_DEVICE:
            device = 0;
            searchRom(true);
            state = LIST_NEXT_DEVICE;
            break;
        case LIST_NEXT_DEVICE:
            if (OneWireResult())
            {
                state = IDLE;
            }
            else
            {
                for (int i = 0; i < 8; i++) DS18B20DeviceList[8 * device + i] = rom[i];
                device++;
                if (allRomsFound || device >= DEVICE_MAX)
                {
                    DS18B20DeviceCount = device;
                    state = CONVERT_T;
                }
                else
                {
                    searchRom(false);
                }
            }
            break;            
        case CONVERT_T:
            if (OneWireResult())
            {
                state = IDLE;
            }
            else
            {
                convertT();
                device = 0;
                state = READ_SCRATCH_PAD;
            }
            break;
        case READ_SCRATCH_PAD:
            if (OneWireResult())
            {
                state = IDLE;
            }
            else
            {
                readScratchpad(device);
                state = EXTRACT_TEMPERATURE;
            }
            break;
        case EXTRACT_TEMPERATURE:
            switch (OneWireResult())
            {
                case ONE_WIRE_RESULT_OK:
                    DS18B20Value[device] = recv[1];
                    DS18B20Value[device] <<= 8;
                    DS18B20Value[device] |= recv[0];
                    break;
                case ONE_WIRE_RESULT_CRC_ERROR:               DS18B20Value[device] = DS18B20_ERROR_CRC;                     break;
                case ONE_WIRE_RESULT_NO_DEVICE_PRESENT:       DS18B20Value[device] = DS18B20_ERROR_NO_DEVICE_PRESENT;       break;
                case ONE_WIRE_RESULT_TIMED_OUT:               DS18B20Value[device] = DS18B20_ERROR_TIMED_OUT;               break;            
                case ONE_WIRE_RESULT_NO_DEVICE_PARTICIPATING: DS18B20Value[device] = DS18B20_ERROR_NO_DEVICE_PARTICIPATING; break;
                default:
                    LogF("Unknown OneWireResult %d\r\n", OneWireResult());
                    break;
            }
            device++;
            if (device < DS18B20DeviceCount) state = READ_SCRATCH_PAD;
            else                             state = IDLE;
            break;
        default:
            LogF("Unknown DS18B20 state %d\r\n", state);
            return -1;
    }
    return 0;
}
static void logcomms()
{
    static int wasbusy = false;
    if (!OneWireBusy() && wasbusy)
    {
        LogF("1-wire | send:");
        for (int i = 0; i < sendlen; i++) LogF(" %02x", send[i]);
        LogF(" | recv:");
        for (int i = 0; i < recvlen; i++) LogF(" %02x", recv[i]);
        LogF("\r\n");
    }
    wasbusy = OneWireBusy();
}
int DS18B20Init()
{
    return 0;
}
int DS18B20Main()
{
    
    if (DEBUG) logcomms();
    
    int r = handlestate(); if (r) return -1;
    
    return 0;
}