#include   "mbed.h"
#include    "log.h"
#include     "io.h"
#include "1-wire-bus.h"
#include "1-wire.h"

#define DEBUG false //Set to true to add debug messages to the log

#define BUS_TIMEOUT_MS 5000
static Timer busytimer;

//Exchange zone
//=============
#define WAIT_FOR_SOMETHING_TO_DO     0
#define HANDLE_RESET_END             1
#define SEND_NEXT_XCHG_WRITE         2
#define HANDLE_XCHG_READ             3
#define SEND_NEXT_SEARCH_WRITE       4
#define SEND_NEXT_SEARCH_READ_BIT    5
#define HANDLE_SEARCH_READ_BIT_TRUE  6
#define HANDLE_SEARCH_READ_BIT_COMP  7
static int state = WAIT_FOR_SOMETHING_TO_DO;

#define JOB_NONE   0
#define JOB_XCHG   1
#define JOB_SEARCH 2
static int job = JOB_NONE;

static int  lensend = 0;
static int  lenrecv = 0;
static char*  psend = NULL;
static char*  precv = NULL;
static int pullupms = 0;

static char crc;
static void resetCrc()
{
    crc = 0;
}
static void addBitToCrc(int bit)
{
    int feedback = !(crc & 0x80) != !bit; //Logical Exclusive Or of the msb of the shift register with the input
    crc <<= 1;                            //Move the shift register one place to the left leaving a zero in the lsb and losing the msb
    if (feedback) crc ^= 0x31;            //Exclusive Or the shift register with polynomial X5 + X4 + 1
}

static void resetBitPosition(int* pByteIndex, char* pBitMask)
{
    *pBitMask = 1;
    *pByteIndex = 0;
}
static void incrementBitPosition(int* pByteIndex, char* pBitMask)
{
    *pBitMask <<= 1;
    if (!*pBitMask)
    {
        *pBitMask = 1;
        *pByteIndex += 1;
    }
}

int getBitAtPosition(int byteIndex, char bitMask)
{
    return psend[byteIndex] & bitMask;
}
void setBitAtPosition(int byteIndex, char bitMask, int bit)
{
    if ( bit) precv[byteIndex] |=  bitMask;
    else      precv[byteIndex] &= ~bitMask;
}
int moreBitsToRead(int byteIndex)
{
    return byteIndex < lenrecv;
}
int moreBitsToWrite(int byteIndex)
{
    return byteIndex < lensend;
}

int result = ONE_WIRE_RESULT_OK;
int OneWireResult()
{
    return result;
}
int OneWireInit()
{
    OneWireBusInit();
    busytimer.stop();
    busytimer.reset();
    state = WAIT_FOR_SOMETHING_TO_DO;
    result = ONE_WIRE_RESULT_OK;
    job   =  JOB_NONE;
    return 0;
}
int OneWireBusy()
{
    return state;
}
void OneWireExchange(int lenBytesToSend, int lenBytesToRecv, char *pBytesToSend, char *pBytesToRecv, int msToPullUp)
{
    lensend = lenBytesToSend;
    lenrecv = lenBytesToRecv;
    psend = pBytesToSend;
    precv = pBytesToRecv;
    pullupms = msToPullUp;
    state = HANDLE_RESET_END;
    job = JOB_XCHG;
    OneWireBusReset();
}
static int* pallfound;
static char* prom;
static void setRomBit(int position, int bit)
{
    int bitindex = position & 0x07;
    int byteindex = position >> 3;
    int bitmask = 1 << bitindex;
    if (bit) *(prom + byteindex) |=  bitmask;
    else     *(prom + byteindex) &= ~bitmask;
}
static int getRomBit(int position)
{    
    int bitindex = position & 0x07;
    int byteindex = position >> 3;
    int bitmask = 1 << bitindex;
    return *(prom + byteindex) & bitmask;
}
static int thisFurthestForkLeftPosn;
static int lastFurthestForkLeftPosn;
static char searchCommand;
static int searchBitPosn;
static int searchBitTrue;
static int searchBitComp;
static int chooseDirectionToTake()
{
    if ( searchBitTrue &&  searchBitComp) return -1; //No devices are participating in the search
    if ( searchBitTrue && !searchBitComp) return  1; //Only devices with a one  at this point are participating
    if (!searchBitTrue &&  searchBitComp) return  0; //Only devices with a zero at this point are participating
    //Both bits are zero so devices with both 0s and 1s at this point are still participating
    
    //If we have not yet reached the furthest away point we forked left (0) last time then just do whatever we did last time
    if (searchBitPosn <  lastFurthestForkLeftPosn) return getRomBit(searchBitPosn);
    
    //If we are at the furthest away point that we forked left (0) last time then this time fork right (1)
    if (searchBitPosn == lastFurthestForkLeftPosn) return 1;
    
    //We are at a new fork point further than we have been before so fork left (0) and record that we did so.
    thisFurthestForkLeftPosn = searchBitPosn;
    return 0; 
}
void OneWireSearch(char command, char* pDeviceRom, int* pAllDevicesFound) //Specify the buffer to receive the rom for the first search and NULL thereafter.
{
    if (pDeviceRom)
    {
        pallfound = pAllDevicesFound;
        *pallfound = false;
        lastFurthestForkLeftPosn = -1;
        prom = pDeviceRom;
        for (int i = 0; i < 8; i++) *(prom + i) = 0;
    }
    thisFurthestForkLeftPosn = -1;
    lensend = 1;
    lenrecv = 0;
    searchCommand = command;
    psend = &searchCommand;
    precv = NULL;
    pullupms = 0;
    job = JOB_SEARCH;
    state = HANDLE_RESET_END;
    OneWireBusReset();
}
char OneWireCrc()
{
    return crc;
}
int OneWireMain()
{
    static int byteindex;
    static char bitmask;
        
    if (state)
    {
        busytimer.start();
        if (busytimer.read_ms() > BUS_TIMEOUT_MS)
        {
            LogCrLf("1-wire bus timed out so protocol has been reset to idle.");
            OneWireInit();
            result = ONE_WIRE_RESULT_TIMED_OUT;
            return 0;
        }
    }
    else
    {
        busytimer.stop();
        busytimer.reset();
    }

    if (OneWireBusBusy()) return 0;
    
    switch(state)
    {
        case WAIT_FOR_SOMETHING_TO_DO:
            break;
        case HANDLE_RESET_END:
            if (OneWireBusValue)
            {
                LogCrLf("No 1-wire device presence detected on the bus");
                result = ONE_WIRE_RESULT_NO_DEVICE_PRESENT;
                state = WAIT_FOR_SOMETHING_TO_DO;
            }
            else
            {
                resetBitPosition(&byteindex, &bitmask);
                switch (job)
                {
                    case JOB_XCHG:   state =   SEND_NEXT_XCHG_WRITE; break;
                    case JOB_SEARCH: state = SEND_NEXT_SEARCH_WRITE; break;
                    default:
                        LogF("Unknown job in RESET_RELEASE %d\r\n", job);
                        return -1;
                }
            }
            break;
            
        case SEND_NEXT_XCHG_WRITE:
            if (moreBitsToWrite(byteindex))
            {
                int bit = getBitAtPosition(byteindex, bitmask);
                incrementBitPosition(&byteindex, &bitmask);
                if (moreBitsToWrite(byteindex)) OneWireBusWriteBit(bit);
                else                            OneWireBusWriteBitWithPullUp(bit, pullupms);
            }
            else
            {
                resetBitPosition(&byteindex, &bitmask);
                if (moreBitsToRead(byteindex))
                {
                    resetCrc();
                    OneWireBusReadBit();
                    state = HANDLE_XCHG_READ;
                }
                else
                {
                    result = ONE_WIRE_RESULT_OK;
                    state = WAIT_FOR_SOMETHING_TO_DO;
                }
            }
            break;
        case HANDLE_XCHG_READ:
            addBitToCrc(OneWireBusValue);
            setBitAtPosition(byteindex, bitmask, OneWireBusValue);
            incrementBitPosition(&byteindex, &bitmask);
            if (moreBitsToRead(byteindex))
            {
                OneWireBusReadBit();
            }
            else
            {
                state = WAIT_FOR_SOMETHING_TO_DO;
                result = crc ? ONE_WIRE_RESULT_CRC_ERROR : ONE_WIRE_RESULT_OK;
            }
            break;
            
        case SEND_NEXT_SEARCH_WRITE:
            if (moreBitsToWrite(byteindex))
            {
                int bit = getBitAtPosition(byteindex, bitmask);
                incrementBitPosition(&byteindex, &bitmask);
                OneWireBusWriteBit(bit);
            }
            else
            {
                searchBitPosn = 0;
                state = SEND_NEXT_SEARCH_READ_BIT;
            }
            break;
        case SEND_NEXT_SEARCH_READ_BIT: //Have to have this extra step to separate from action in HANDLE_SEARCH_READ_BIT_COMP
            OneWireBusReadBit();
            state = HANDLE_SEARCH_READ_BIT_TRUE;
            break;
        case HANDLE_SEARCH_READ_BIT_TRUE:
            searchBitTrue = OneWireBusValue;
            OneWireBusReadBit();
            state = HANDLE_SEARCH_READ_BIT_COMP;
            break;
        case HANDLE_SEARCH_READ_BIT_COMP:
            searchBitComp = OneWireBusValue;
            if (DEBUG) LogF("%d%d - ", searchBitTrue, searchBitComp);
            int direction;
            direction = chooseDirectionToTake();
            if (direction == -1)
            {
                LogCrLf("No devices have responded");
                result = ONE_WIRE_RESULT_NO_DEVICE_PARTICIPATING;
                state = WAIT_FOR_SOMETHING_TO_DO;
            }
            else
            {
                if (DEBUG) LogF(" %d -> %d\r\n", direction, searchBitPosn);
                setRomBit(searchBitPosn, direction);
                OneWireBusWriteBit(direction);
                searchBitPosn++;
                if (searchBitPosn < 64)
                {
                    state = SEND_NEXT_SEARCH_READ_BIT;
                }
                else
                {
                    if (thisFurthestForkLeftPosn == -1) *pallfound = true;
                    lastFurthestForkLeftPosn = thisFurthestForkLeftPosn;
                    result = ONE_WIRE_RESULT_OK;
                    state = WAIT_FOR_SOMETHING_TO_DO;
                }
            }
            break;
        default:
            LogF("Unknown state %d\r\n", state);
            return -1;
    }
    return 0;
}
