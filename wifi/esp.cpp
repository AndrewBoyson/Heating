#include "mbed.h"
#include  "log.h"
#include  "esp.h"
#include   "io.h"
#include  "cfg.h"
#include "uart.h"

#define RECV_TIMEOUT_MS 5000
#define DEBUG true

//State
#define LINE_LENGTH 256
#define IDLE           0
#define IN_LINE        1
#define IPD_WAIT_START 2
#define IPD_READ       3
#define SEND_DATA      4
static int state;   //Initialised in init

//Solicited responses received
//============================
#define LINE_LENGTH 256
int   EspLineAvailable; //Initialised in init; can be one of the values defined in esp.h
char  EspLine[LINE_LENGTH];
static char* pLineNext; //Initialised in init to EspLine

static int addRawCharToBuffer(char* pBuff, char** ppNext, int len, char c)
{
    
    // if *ppNext is at the last position in pBuff (pBuff+len-1) then we are full and should stop
    if (*ppNext >= pBuff + len - 1) return -1;
    
    // Put the char into *ppNext and NUL into *ppNext + 1.
    **ppNext = c; 
    ++*ppNext;
    **ppNext = '\0';
    
    return 0;
}
static int addCharToBuffer(char* pBuff, char** ppNext, int len, char c, int includeCrLf)
{
    if (!pBuff) return -1;
    switch (c)
    {
        case '\0':
            if (addRawCharToBuffer(pBuff, ppNext, len, '\\')) return -1;
            if (addRawCharToBuffer(pBuff, ppNext, len, '0' )) return -1; //This is the character zero '0' not NUL '\0'
            break;
        case '\r':
        case '\n':
            if (includeCrLf && addRawCharToBuffer(pBuff, ppNext, len, c)) return -1;
            break;
        default:
            if (addRawCharToBuffer(pBuff, ppNext, len, c)) return -1;
            break;
    }
    return 0;
}
static int addChar(char c)
{
    int r = addCharToBuffer(EspLine,   &pLineNext, LINE_LENGTH, c, false);
    return r;
}
//Unsolicited ntp or http requests
//================================
void *EspIpdBuffer[ESP_ID_COUNT];
int   EspIpdBufferLen[ESP_ID_COUNT];
int   EspIpdReserved[ESP_ID_COUNT];
int   EspIpdLength;
int   EspIpdId;
int   EspDataAvailable;


//Stuff to send
//=============
int          EspLengthToSend;
const void * EspDataToSend;

void EspSendCommandF(char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    int size = vsnprintf(NULL, 0, fmt, argptr);
    char snd[size + 1];
    vsprintf(snd, fmt, argptr);
    va_end(argptr);
    EspSendCommand(snd);
}
void EspSendCommand(char* p)
{    
    while(*p) UartSendPush(*p++);
}
void EspSendData(int length, const void * snd)
{
    const char* p = (char*)snd;
    const char* e = (char*)snd + length;
    while (p < e) UartSendPush(*p++);
}


//Reset ESP8266 zone
#define   RESET_TIME_MS  250
static DigitalOut espRunOnHighResetOnLow(p26);
int reset; //Set to 1 by EspReset (and hence EspInit); set to 0 by EspReleasefromReset
void EspResetAndStop()
{
    reset = true;
    UartReset();
    pLineNext = EspLine;
    *pLineNext = '\0';
    EspLengthToSend = 0;
    EspDataToSend = NULL;
    state = IDLE;
}
void EspReleaseFromReset(void)
{
    reset = false;
}
void pulseResetPin()
{
    static Timer resetTimer;

    if (reset)
    {
        resetTimer.stop();
        resetTimer.reset();
        espRunOnHighResetOnLow = 0;
    }
    else
    {
        resetTimer.start();
        if (resetTimer.read_ms() > RESET_TIME_MS)
        {
            resetTimer.stop();
            espRunOnHighResetOnLow = 1;
        }
    }
}
//General commands
int EspInit()
{
    EspResetAndStop();
    UartBaud(CfgBaud);
    for (int i = 0; i < ESP_ID_COUNT; i++)
    {
        EspIpdBuffer[i] = NULL;
        EspIpdBufferLen[i] = 0;
        EspIpdReserved[i] = 0;
    }
    return 0;
}

//Main loop zone
int handleCharacter(char c)
{
    //Static variables. Initialised on first load.
    static char   ipdHeader[20];
    static char * pipdHeader;    //Set to ipdHeader when a '+' is received
    static int    bytesRcvd;     //Set to 0 when the ':' following the +IPD is received
    static void * pData;         //Set to EspIpdBuffer[EspIpdId] when the ':' following the +IPD is received
    
    switch(state)
    {
        case IDLE:
            if (c == '+')
            {
                pipdHeader = ipdHeader;
               *pipdHeader = 0;
                state = IPD_WAIT_START;
            }
            else if (c == '>')
            {
                state = SEND_DATA;
            }
            else
            {
                pLineNext = EspLine;
                *pLineNext = 0;           
                int r = addChar(c);
                if (r)
                {
                    EspLineAvailable = ESP_OVERFLOW;
                    state = IDLE;
                }
                else
                {
                    state = IN_LINE;
                }
            }
            break;
        case IN_LINE:
            if (c == '\n')
            {
                EspLineAvailable = ESP_AVAILABLE;
                state = IDLE;
            }
            else
            {
                int r = addChar(c);
                if (r)
                {
                    EspLineAvailable = ESP_OVERFLOW;
                    state = IDLE;
                }
            }
            break;
        case IPD_WAIT_START:
            if (pipdHeader == ipdHeader && c != 'I') //If the first character after the '+' is not 'I' then start a line instead
            {
                pLineNext = EspLine;
                addChar('+'); 
                addChar(c);
                state = IN_LINE;               
            }
            else
            {
                *pipdHeader++ = c;
                *pipdHeader   = 0;
                if (c == ':')
                {
                    sscanf(ipdHeader, "IPD,%d,%d:", &EspIpdId, &EspIpdLength);
                    bytesRcvd = 0;
                    pData = EspIpdBuffer[EspIpdId];
                    state = IPD_READ;
                }
            }
            break;
        case IPD_READ:
            bytesRcvd++;
            if (bytesRcvd <= EspIpdBufferLen[EspIpdId])
            {
                *(char *)pData = c;
                pData = (char *)pData + 1;
            }
            if (bytesRcvd == EspIpdLength)
            {
                if (bytesRcvd <= EspIpdBufferLen[EspIpdId]) EspDataAvailable = ESP_AVAILABLE;
                else                                        EspDataAvailable = ESP_OVERFLOW;
                state = IDLE;
            }
            break;
        case SEND_DATA:
            EspSendData(EspLengthToSend, EspDataToSend);
            state = IDLE;
            break;
        default:
            LogF("Unknown state %d\n\r", state);
            return -1;
    }
    return 0;
}
int isTimeout()
{
    static Timer receiveTimer;

    if (state == IDLE)
    {
        receiveTimer.stop();
        receiveTimer.reset();
    }
    else
    {
        receiveTimer.start();
        if (receiveTimer.read_ms() > RECV_TIMEOUT_MS) return true;
    }
    return false;
}
int EspMain()
{
    pulseResetPin();
    
    //Reset line availability one shot
    EspLineAvailable = ESP_IDLE;
    EspDataAvailable = ESP_IDLE;
    
    if (isTimeout())
    {
        if (state == IN_LINE) EspLineAvailable = ESP_TIMEOUT;
        else                  EspDataAvailable = ESP_TIMEOUT;
        state = IDLE;
        return 0;
    }
    
    //Handle any incoming characters
    int c = UartRecvPull();
    if (c != EOF)
    {
        if (DEBUG) LogPush(c);
        int r = handleCharacter(c); //This will set the EspAvailable one-shots
        if (r) return -1;
    }
    
    return 0;
}



