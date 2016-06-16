#include <mbed.h>
#include  "log.h"
#include  "esp.h"
#include "wifi.h"
#include  "rtc.h"
#include   "io.h"
#include   "at.h"
#include  "cfg.h"

#define ERA_BASE     0
#define ERA_PIVOT 2016

#define ID 0

static Timer timer;

struct Packet {
    union
    {
        uint32_t FirstLine;
        struct
        {
            unsigned Mode : 3;
            unsigned VN   : 3;
            unsigned LI   : 2;
            uint8_t  Stratum; 
             int8_t  Poll;
             int8_t  Precision;
        };
    };
    uint32_t RootDelay;
    uint32_t Dispersion;
    uint32_t RefIdentifier;
    
    uint64_t RefTimeStamp;
    uint64_t OriTimeStamp;
    uint64_t RecTimeStamp;
    uint64_t TraTimeStamp;
};
static struct Packet packet;

static uint64_t ntohll(uint64_t n) {
    int testInt = 0x0001; //Big end contains 0x00; little end contains 0x01
    int *pTestInt = &testInt;
    char *pTestByte = (char*)pTestInt;
    char testByte = *pTestByte; //fetch the first byte
    if (testByte == 0x00) return n; //If the first byte is the big end then host and network have same endianess
    
    union ull
    {
        uint64_t Whole;
        char Bytes[8];
    };
    union ull h;
    h.Whole = n;
    
    char t;
    t = h.Bytes[7]; h.Bytes[7] = h.Bytes[0]; h.Bytes[0] = t;
    t = h.Bytes[6]; h.Bytes[6] = h.Bytes[1]; h.Bytes[1] = t;
    t = h.Bytes[5]; h.Bytes[5] = h.Bytes[2]; h.Bytes[2] = t;
    t = h.Bytes[4]; h.Bytes[4] = h.Bytes[3]; h.Bytes[3] = t;
    
    return h.Whole;
}
uint64_t addMs(uint64_t ntpTime, int ms)
{
    int64_t adjustment = ms;  //Put the ms into a 64 bit int
    adjustment <<= 32;        //Move the into the seconds part of the ntp timestamp (the upper 32 bits)
    adjustment /= 1000;       //Divide by 1000 to convert to ms
    ntpTime += adjustment;
    return ntpTime;
}
uint64_t getTimeAsNtp()
{
    uint64_t ntp28 = 2208988800ULL << 28;            //Fill with 1970 - 1900
    ntp28 += RtcGet() << (28 - RTC_RESOLUTION_BITS); //Add the rtc time left shifted so that the fraction parts align
    return ntp28 << 4;                               //This wipes off any era in the timestamp
}
void setTimeAsNtp(uint64_t ntpTime)
{
    uint64_t ntp28 = ntpTime >> 4; //Drop the decimal point at the expense of precision to make room for more seconds   

    uint64_t eraPivot = (ERA_PIVOT - 1900) * 365ULL * 24 * 3600;
    uint64_t era = (ntp28 >> 28) > eraPivot ? ERA_BASE : ERA_BASE + 1; //Calculate the current era
    ntp28 |= (era << 60);                                              //Add it to the top 4 bits
        
    ntp28 -= 2208988800ULL << 28; //Subtract 1970 - 1900 to align the epochs
    uint64_t rtcTime = (ntp28 >> (28 - RTC_RESOLUTION_BITS));
    RtcSet(rtcTime);
}
void NtpInit()
{
    EspIpdReserved[ID] = true;
}
int preparePacket()
{        
    memset(&packet, 0, sizeof(packet));
    packet.LI   = 0;
    packet.VN   = 1;
    packet.Mode = 3; //Client
    packet.TraTimeStamp = ntohll(getTimeAsNtp());
    return 0;
}
int handlePacket()
{        
    
    //Handle the reply  
    char leap    = packet.LI;
    char version = packet.VN;
    char mode    = packet.Mode;
    char stratum = packet.Stratum;
    if (leap    == 3) { LogF("Remote clock has a fault\r\n");                     return -1; }
    if (version  < 1) { LogF("Version is %d\r\n", version);                       return -1; }
    if (mode    != 4) { LogF("Mode is %d\r\n", mode);                             return -1; }
    if (stratum == 0) { LogF("Received Kiss of Death packet (stratum is 0)\r\n"); return -1; }

    //Set the RTC
    uint64_t ntpTime = ntohll(packet.RecTimeStamp);
    ntpTime = addMs(ntpTime, CfgClockOffsetMs);
    setTimeAsNtp(ntpTime);
    
    return 0;
}
int NtpIdConnectStatus = AT_NONE;
static void outgoingMain()
{
    if (AtBusy()) return;
    if (!WifiStarted()) return;
    
    static int firstAttempt = true;
    static int result = AT_NONE;
         
    if (NtpIdConnectStatus == AT_SUCCESS)
    {
        int retryAfterFailure =  timer.read() > CfgClockSetRetryInterval && result != AT_SUCCESS;
        int repeat            =  timer.read() > CfgClockSetInterval;
        
        if (firstAttempt || retryAfterFailure || repeat)
        {
            preparePacket();
            AtSendData(ID, sizeof(packet), &packet, &result);
            firstAttempt = false;
            timer.reset();
            timer.start();
        }
    }
    else
    {
        AtConnectId(ID, "UDP", CfgNtpIp, 123, &packet, sizeof(packet), &NtpIdConnectStatus);
    }
}
static void incomingMain()
{
    if (EspDataAvailable == ESP_AVAILABLE && EspIpdId == ID)
    {
        if (EspIpdLength == sizeof(packet)) handlePacket();
        else LogF("Incorrect NTP packet length of %d bytes", EspIpdLength);
    }
}
int NtpMain()
{
    outgoingMain();
    incomingMain();
    return 0;
}