#include     <mbed.h>
#include      "log.h"
#include      "esp.h"
#include     "wifi.h"
#include      "rtc.h"
#include       "io.h"
#include       "at.h"
#include "settings.h"

#define ERA_BASE     0
#define ERA_PIVOT 2016

#define ID 3

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
    if (leap    == 3) { LogTimeF("Remote clock has a fault\r\n");                     return -1; }
    if (version  < 1) { LogTimeF("Version is %d\r\n", version);                       return -1; }
    if (mode    != 4) { LogTimeF("Mode is %d but expected 4\r\n", mode);              return -1; }
    if (stratum == 0) { LogTimeF("Received Kiss of Death packet (stratum is 0)\r\n"); return -1; }

    //Check the received timestamp delay
    uint64_t delay = getTimeAsNtp() - ntohll(packet.OriTimeStamp);
    uint64_t delayMs = delay >> 22; //This is approximate as the seconds are divided by 1024 rather than 1000 but close enough
    uint64_t limit = SettingsGetClockNtpMaxDelayMs();
    if (delayMs > limit) { LogTimeF("Delay %llu ms is greater than limit %llu ms\r\n", delayMs, limit); return -1; }
    
    //Set the RTC
    uint64_t ntpTime = ntohll(packet.RecTimeStamp);
    int64_t offset = SettingsGetClockOffsetMs() << 22; //This is approximate as the milliseconds are multiplied by 1024 rather than 1000 but close enough
    setTimeAsNtp(ntpTime + offset);
    
    return 0;
}

static bool requestReconnect = false;
void NtpRequestReconnect()
{
    requestReconnect = true;
}

enum {
    AM_DISCONNECTED,
    AM_CONNECT,
    AM_CONNECTED,
    AM_SEND,
    AM_CLOSE
};
enum {
    INTERVAL_INITIAL,
    INTERVAL_NORMAL,
    INTERVAL_RETRY
};

static uint64_t timeStart;
static int intervalType;
static bool intervalComplete()
{
    uint64_t interval;
    switch(intervalType)
    {
        case INTERVAL_INITIAL: interval = SettingsGetClockInitialInterval(); break;
        case INTERVAL_NORMAL:  interval = SettingsGetClockNormalInterval();  break;
        case INTERVAL_RETRY:   interval = SettingsGetClockRetryInterval();   break;
    }
    interval <<= RTC_RESOLUTION_BITS;
    return RtcGet() - timeStart >= interval;
}
static void startInterval(int type)
{
    timeStart = RtcGet();
    intervalType = type;
}

static int outgoingMain()
{
    static int am     = AM_DISCONNECTED;
    static int result = AT_NONE;
    
    if (AtBusy()) return 0;
    if (!WifiStarted())
    {
        am           = AM_DISCONNECTED;
        result       = AT_NONE;
        startInterval(INTERVAL_INITIAL);
        return 0;
    }
    
    char* ntpIp = SettingsGetClockNtpIp();

    switch (am)
    {
        case AM_DISCONNECTED:
            requestReconnect = false;
            if (intervalComplete() && *ntpIp) am = AM_CONNECT; //Check the ip is not an empty string
            break;
        case AM_CONNECT:
            switch (result)
            {
                case AT_NONE:
                    AtConnectId(ID, "UDP", ntpIp, 123, &packet, sizeof(packet), &result);
                    break;
                case AT_SUCCESS:
                    am = AM_CONNECTED;
                    result = AT_NONE;
                    break;
                default:
                    startInterval(INTERVAL_RETRY);
                    am = AM_CLOSE;
                    result = AT_NONE;
                    break;
            }
            break;
        case AM_CONNECTED:
            if (intervalComplete()) am = AM_SEND;
            if (requestReconnect)   am = AM_CLOSE;
            break;
        case AM_SEND:
            switch (result)
            {
                case AT_NONE:
                    preparePacket();
                    AtSendData(ID, sizeof(packet), &packet, &result);
                    break;
                case AT_SUCCESS:
                    startInterval(INTERVAL_RETRY); //Start interval is set to normal when packet is successfully received
                    am = AM_CONNECTED;
                    result = AT_NONE;
                    break;
                default:
                    startInterval(INTERVAL_RETRY);
                    am = AM_CONNECTED;
                    result = AT_NONE;
                    break;
            }
            break;
        case AM_CLOSE:
            switch (result)
            {
                case AT_NONE:
                    AtClose(ID, &result);
                    break;
                default:
                    am = AM_DISCONNECTED;
                    result = AT_NONE;
                    break;
            }
            break;
        default:
            LogF("Unknown \'am\' %d", am);
            return -1;
    }
   
    return 0;
}

static int incomingMain()
{
    if (EspDataAvailable == ESP_AVAILABLE && EspIpdId == ID)
    {
        if (EspIpdLength == sizeof(packet))
        {
            if (!handlePacket()) startInterval(INTERVAL_NORMAL); //Set the next retry as normal after successful reply
        }
        else
        {
            LogF("Incorrect NTP packet length of %d bytes", EspIpdLength);
        }
    }
    return 0;
}
int NtpMain()
{
    int r;
    r = outgoingMain(); if (r) return -1;
    r = incomingMain(); if (r) return -1;
    return 0;
}