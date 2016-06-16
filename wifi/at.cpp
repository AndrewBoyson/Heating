#include "mbed.h"
#include  "log.h"
#include  "esp.h"
#include   "io.h"
#include   "at.h"

#define TIMEOUT    10
#define TIMEOUT_OK  2

enum { WAIT_FOR_COMMAND,
       WAIT_FOR_ESP_READY,
       WAIT_FOR_WIFI_CONNECTED,
       WAIT_FOR_WIFI_GOT_IP,
       WAIT_FOR_STATUS,
       WAIT_FOR_VERSION,
       WAIT_FOR_OK,
       WAIT_FOR_SEND_OK };
static int wait_for;

static int * pFeedback;
static void startCommand(int* pStatus, int waitfor)
{
    wait_for = waitfor;
    pFeedback = pStatus;
    if (pFeedback) *pFeedback = AT_NONE;
}
static void finishAndReset(int feedback)
{
    if (pFeedback)
    {
        *pFeedback = feedback;
        pFeedback = NULL;
    }
    wait_for = WAIT_FOR_COMMAND;
}
void AtReleaseResetAndStart(int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_ESP_READY);
    EspReleaseFromReset();
}
void AtWaitForWifiConnected(int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_WIFI_CONNECTED);
}
void AtWaitForWifiGotIp(int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_WIFI_GOT_IP);
}
void AtConnect(const char *ssid, const char *password, int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_OK);
    EspSendCommandF("AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
}
void AtAt(int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_OK);
    EspSendCommand("AT\r\n");
}

void AtConnectId(int id, char * type, char * addr, int port, void * buffer, int bufferlength, int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_OK);
    
    EspIpdBuffer[id] = buffer;
    EspIpdBufferLen[id] = bufferlength;

    EspSendCommandF("AT+CIPSTART=%d,\"%s\",\"%s\",%d\r\n", id, type, addr, port);
}
void AtStartServer(int port, int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_OK);
    EspSendCommandF("AT+CIPSERVER=1,%d\r\n", port);
}
void AtClose(int id, int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_OK);
    EspSendCommandF("AT+CIPCLOSE=%d\r\n", id);
}
void AtSetMode(int mode, int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_OK);
    EspSendCommandF("AT+CWMODE=%d\r\n", mode);
}
void AtMux(int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_OK);
    EspSendCommand("AT+CIPMUX=1\r\n");
}
void AtBaud(int baud, int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_OK);
    EspSendCommandF("AT+CIOBAUD=%d\r\n", baud);
}
int  AtEspStatus;
void AtGetEspStatus(int *pStatus)
{
    AtEspStatus = 0;
    startCommand(pStatus, WAIT_FOR_STATUS);
    EspSendCommand("AT+CIPSTATUS\r\n");
}
char AtEspVersion[20];
static int versionlinecount;
void AtGetEspVersion(int *pStatus)
{
    AtEspVersion[0] = '\0';
    startCommand(pStatus, WAIT_FOR_VERSION);
    EspSendCommand("AT+GMR\r\n");
    versionlinecount = 0;
}
void AtSendData(int id, int length, const void * pdata, int *pStatus)
{
    startCommand(pStatus, WAIT_FOR_SEND_OK);
    EspLengthToSend = length;
    EspDataToSend = pdata;
    EspSendCommandF("AT+CIPSEND=%d,%d\r\n", id, length);
}

int AtBusy()
{
    return wait_for;
}
void AtResetAndStop()
{
    startCommand(NULL, WAIT_FOR_COMMAND);
    EspResetAndStop();
}
int AtInit()
{   
    startCommand(NULL, WAIT_FOR_COMMAND);
    return 0;
}
int handleLine()
{
    switch (wait_for)
    {
        //No command
        case WAIT_FOR_COMMAND:
            return 0;
            
        //Connect    
        case WAIT_FOR_ESP_READY:
            if (strcmp(EspLine, "ready") == 0) finishAndReset(AT_SUCCESS);
            return 0;
        case WAIT_FOR_WIFI_CONNECTED:
            if (strcmp(EspLine, "WIFI CONNECTED") == 0) finishAndReset(AT_SUCCESS);
            return 0;
        case WAIT_FOR_WIFI_GOT_IP:
            if (strcmp(EspLine, "WIFI GOT IP") == 0) finishAndReset(AT_SUCCESS);
            return 0;
        
        //Send command
        case WAIT_FOR_SEND_OK:
            if (strcmp(EspLine, "SEND OK") == 0) finishAndReset(AT_SUCCESS);
            return 0;
            
        //Status command
        case WAIT_FOR_STATUS:
            if (sscanf(EspLine, "STATUS:%d", &AtEspStatus) == 1) wait_for = WAIT_FOR_OK;
            return 0;
            
        //Version command
        case WAIT_FOR_VERSION:
            if (++versionlinecount == 3)
            {
                strcpy(AtEspVersion, EspLine);
                wait_for = WAIT_FOR_OK;
            }
            return 0;
        
        //Most commands
        case WAIT_FOR_OK:
            if (strcmp(EspLine, "OK"     ) == 0) finishAndReset(AT_SUCCESS);
            if (strcmp(EspLine, "ERROR"  ) == 0) finishAndReset(AT_ERROR);
            return 0;
            
        default:
            LogF("Unknown wait_for %d\r\n", wait_for);
            return -1;
    }
}
int AtMain()
{
    
    //Monitor time taken
    static Timer receiveTimer;
    if (wait_for)
    {
        receiveTimer.start();
    }
    else
    {
        receiveTimer.stop();
        receiveTimer.reset();
    }
    
    //Check for problems; feedback status and reset
    if (receiveTimer.read() > TIMEOUT || (wait_for == WAIT_FOR_OK && receiveTimer.read() > TIMEOUT_OK))
    {
        finishAndReset(AT_RESPONCE_TIMEOUT);
        LogCrLf("Command timed out");
        return 0;
    }
    

    //Depending on what the Esp has returned: do nothing; treat the line or finish and reset
    switch (EspLineAvailable)
    {
        case ESP_IDLE:
            break;
        case ESP_AVAILABLE:
            if (handleLine()) return -1;
            break;
        case ESP_TIMEOUT:
            finishAndReset(AT_LINE_TIMEOUT);
            LogCrLf("Esp timed out");
            break;
        case ESP_OVERFLOW:
            if (wait_for == WAIT_FOR_ESP_READY) break; //Ignore overflows from the esp's 450 byte boot message
            finishAndReset(AT_LINE_OVERFLOW);
            LogCrLf("Esp buffer overflow");
            break;
        default:
            LogF("Unknown EsplineAvailable %d\r\n");
            return -1;
    }
    
    return 0;
}
