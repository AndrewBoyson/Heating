#include <mbed.h>
#include  "log.h"
#include  "esp.h"
#include "wifi.h"
#include  "rtc.h"
#include   "io.h"
#include   "at.h"
#include "fram.h"
#include  "net.h"

#define ERA_BASE     0
#define ERA_PIVOT 2016

#define ID 3

static char    ip[16];           static int iIp;
static int32_t initialInterval;  static int iInitialInterval;
static int32_t normalInterval;   static int iNormalInterval;
static int32_t retryInterval;    static int iRetryInterval;
static int32_t offsetMs;         static int iOffsetMs;
static int32_t maxDelayMs;       static int iMaxDelayMs;

char* NtpGetIp             (){ return       ip;              } 
int   NtpGetInitialInterval(){ return (int) initialInterval; } 
int   NtpGetNormalInterval (){ return (int) normalInterval;  } 
int   NtpGetRetryInterval  (){ return (int) retryInterval;   } 
int   NtpGetOffsetMs       (){ return (int) offsetMs;        } 
int   NtpGetMaxDelayMs     (){ return (int) maxDelayMs;      } 


void NtpSetIp              ( char *value) { strncpy(ip, value, 16); char bin[4]; NetIp4StrToBin(ip, bin); FramWrite(iIp,              4, bin              ); }
void NtpSetInitialInterval ( int   value) { initialInterval    = (int32_t)value;                          FramWrite(iInitialInterval, 4, &initialInterval ); }
void NtpSetNormalInterval  ( int   value) { normalInterval     = (int32_t)value;                          FramWrite(iNormalInterval,  4, &normalInterval  ); }
void NtpSetRetryInterval   ( int   value) { retryInterval      = (int32_t)value;                          FramWrite(iRetryInterval,   4, &retryInterval   ); }
void NtpSetOffsetMs        ( int   value) { offsetMs           = (int32_t)value;                          FramWrite(iOffsetMs,        4, &offsetMs        ); }
void NtpSetMaxDelayMs      ( int   value) { maxDelayMs         = (int32_t)value;                          FramWrite(iMaxDelayMs,      4, &maxDelayMs      ); }

int  NtpInit()
{
    int address;
    int32_t def4;
    
    char bin[4]; address = FramLoad( 4,  bin,              NULL); if (address < 0) return -1; iIp              = address; NetIp4BinToStr(bin, sizeof(ip), ip);
    def4 =    1; address = FramLoad( 4, &initialInterval, &def4); if (address < 0) return -1; iInitialInterval = address;
    def4 =  600; address = FramLoad( 4, &normalInterval,  &def4); if (address < 0) return -1; iNormalInterval  = address;
    def4 =   60; address = FramLoad( 4, &retryInterval,   &def4); if (address < 0) return -1; iRetryInterval   = address; 
    def4 =    0; address = FramLoad( 4, &offsetMs,        &def4); if (address < 0) return -1; iOffsetMs        = address; 
    def4 =   50; address = FramLoad( 4, &maxDelayMs,      &def4); if (address < 0) return -1; iMaxDelayMs      = address; 

    EspIpdReserved[ID] = true;
    
    return 0;
}
__packed struct {
    unsigned Mode : 3;
    unsigned VN   : 3;
    unsigned LI   : 2;
    uint8_t  Stratum; 
     int8_t  Poll;
     int8_t  Precision;
    uint32_t RootDelay;
    uint32_t Dispersion;
    uint32_t RefIdentifier;
    
    uint64_t RefTimeStamp;
    uint64_t OriTimeStamp;
    uint64_t RecTimeStamp;
    uint64_t TraTimeStamp;
} packet;

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
int preparePacket()
{        
    memset(&packet, 0, sizeof(packet));
    packet.LI   = 0;
    packet.VN   = 1;
    packet.Mode = 3; //Client
    packet.TraTimeStamp = NetToHost64(getTimeAsNtp());
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
    uint64_t delay   = getTimeAsNtp() - NetToHost64(packet.OriTimeStamp);
    uint64_t delayMs = delay >> 22; //This is approximate as the seconds are divided by 1024 rather than 1000 but close enough
    uint64_t limit   = NtpGetMaxDelayMs();
    if (delayMs > limit) { LogTimeF("Delay %llu ms is greater than limit %llu ms\r\n", delayMs, limit); return -1; }
    
    //Set the RTC
    uint64_t ntpTime = NetToHost64(packet.RecTimeStamp);
    int64_t offset = NtpGetOffsetMs() << 22; //This is approximate as the milliseconds are multiplied by 1024 rather than 1000 but close enough
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
        case INTERVAL_INITIAL: interval = NtpGetInitialInterval(); break;
        case INTERVAL_NORMAL:  interval = NtpGetNormalInterval();  break;
        case INTERVAL_RETRY:   interval = NtpGetRetryInterval();   break;
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
    
    char* ntpIp = NtpGetIp();

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
            LogTimeF("Unknown \'am\' %d", am);
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
            LogTimeF("Incorrect NTP packet length of %d bytes", EspIpdLength);
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