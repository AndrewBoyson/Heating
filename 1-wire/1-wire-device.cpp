#include          "mbed.h"
#include        "1-wire.h"
#include           "log.h"
#include            "io.h"
#include          "wifi.h"
#include "1-wire-device.h"
#include       "ds18b20.h"

#define DEBUG false //Set this to true to add debug messages to the log

#define SEND_BUFFER_LENGTH 10
#define RECV_BUFFER_LENGTH 10
static char send[SEND_BUFFER_LENGTH];
static char recv[RECV_BUFFER_LENGTH];
static int sendlen = 0;
static int recvlen = 0;

int   DeviceScanMs = 0;
int   DeviceCount  = 0;
char  DeviceList[DEVICE_MAX * 8];
char* DeviceAddress(int device)
{
    return DeviceList + device * 8;
}
void DeviceAddressToString(char* pAddress, char* pText)
{
    char *pAddressAfter = pAddress + 8;
    for (char* p = pAddress; p < pAddressAfter; p++)
    {
        char highnibble = *p >> 4; //Unsigned so fills with zeros
        char  lownibble = *p & 0xF;
        *pText++ = highnibble < 0xA ? highnibble + '0' : highnibble - 0xA + 'A'; //Replace high nibble with its ascii equivalent
        *pText++ =  lownibble < 0xA ?  lownibble + '0' :  lownibble - 0xA + 'A'; //Replace low  nibble with its ascii equivalent
        *pText++ = p < pAddressAfter - 1 ? ' ' : 0;                              //Put in a space between the bytes or a NUL at the end of the last one
    }
}
void DeviceParseAddress(char* pText, char *pAddress)
{
    int highNibble = true;
    char *pAddressAfter = pAddress + 8;
    while(*pText && pAddress < pAddressAfter)
    {
        int nibble = -1;
        if (*pText >= '0' && *pText <= '9') nibble = *pText - '0';
        if (*pText >= 'A' && *pText <= 'F') nibble = *pText - 'A' + 0xA;
        if (*pText >= 'a' && *pText <= 'f') nibble = *pText - 'a' + 0xA;
        if (nibble >= 0)                                  //Ignore characters which do not represent a hex number
        {
            if (highNibble)
            {
                *pAddress = nibble << 4;                  //Set the high nibble of the current address byte and zero the low nibble
                highNibble = false;                       //Move onto the low nibble of this address byte
            }
            else //low nibble
            {
                *pAddress += nibble;                      //Add the low nibble to the current address byte
                highNibble = true;                        //Move onto the high nibble of the next address byte
                pAddress++;
            }
        }
        pText++;
    }
    while (pAddress < pAddressAfter) *pAddress++ = 0; //Set any remaining bytes to zero
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
    for (int i = 0; i < 8; i++) send[i+1] = DeviceList[device * 8 + i];
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
enum {
    IDLE,
    LIST_FIRST_DEVICE,
    LIST_NEXT_DEVICE,
    LIST_DEVICE_CHECK,
    CONVERT_T,
    CONVERT_T_CHECK,
    ENUMERATE_START,
    ENUMERATE,
    READ_SCRATCHPAD,
    EXTRACT_TEMPERATURE
    };
static volatile int state = IDLE;
int DeviceBusy() { return state; }
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
            diffMs = scanMs - DeviceScanMs;
            if (diffMs > 0xF || diffMs < 0xF)
            {
                DeviceScanMs += (scanMs - DeviceScanMs) >> 4;
            }
            else
            {
                if (scanMs > DeviceScanMs) DeviceScanMs++;
                if (scanMs < DeviceScanMs) DeviceScanMs--;
            }
            state = LIST_FIRST_DEVICE;
            break;
            
        case LIST_FIRST_DEVICE:
            device = 0;
            searchRom(true);
            state = LIST_DEVICE_CHECK;
            break;
        case LIST_DEVICE_CHECK:
            if (OneWireResult()) state = IDLE;
            else                 state = LIST_NEXT_DEVICE;
            break;
        case LIST_NEXT_DEVICE:
            for (int i = 0; i < 8; i++) DeviceList[8 * device + i] = rom[i];
            device++;
            if (allRomsFound || device >= DEVICE_MAX)
            {
                DeviceCount = device;
                state = CONVERT_T;
            }
            else
            {
                searchRom(false);
                state = LIST_DEVICE_CHECK;
            }
            break;
            
        case CONVERT_T:
            convertT();
            state = CONVERT_T_CHECK;
            break;
        case CONVERT_T_CHECK:
            if (OneWireResult()) state = IDLE;
            else                 state = ENUMERATE_START;
            break;
            
        case ENUMERATE_START:
            device = -1;
            state = ENUMERATE;
            break;
        case ENUMERATE:
            device++;
            if (device >= DeviceCount)                              state = IDLE;
            else if (DeviceList[device * 8] == DS18B20_FAMILY_CODE) state = READ_SCRATCHPAD;
            else                                                    state = ENUMERATE;
            break;
            
        case READ_SCRATCHPAD:
            readScratchpad(device);
            state = EXTRACT_TEMPERATURE;
            break;
        case EXTRACT_TEMPERATURE:
            DS18B20ReadValue(OneWireResult(), device, recv[0], recv[1]);
            state = ENUMERATE;
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
int DeviceInit()
{
    return 0;
}
int DeviceMain()
{
    
    if (DEBUG) logcomms();
    
    int r = handlestate(); if (r) return -1;
    
    return 0;
}