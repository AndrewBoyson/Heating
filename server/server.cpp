#include    "mbed.h"
#include     "esp.h"
#include      "at.h"
#include     "log.h"
#include    "wifi.h"
#include    "html.h"
#include      "io.h"
#include "request.h"


#define SERVER_PORT 80
static int startMain()
{
    //Do nothing if a command is already in progress
    if (AtBusy()) return 0;
        
    //Start the server if not started
    static int startServerReply = AT_NONE;
    if (startServerReply != AT_SUCCESS) AtStartServer(SERVER_PORT, &startServerReply);
    return 0;
}
static int recvMain()
{
    //Wait for data to be available oneshot. Buffer will normally hit limit.
    if (!EspDataAvailable) return 0;
    
    //Ignore ids that have been reserved elsewhere (ie NTP)
    if (EspIpdReserved[EspIpdId]) return 0;
    
    //Treat the request
    RequestHandle(EspIpdId);

    return 0;
}
static int sendMain()
{
    //Do nothing if a command is already in progress
    if (AtBusy()) return 0;
    
    //Send data one id at a time
    static int id = 0;
    int length;
    char* pBuffer;
    int chunk = HtmlGetNextChunkToSend(id, &length, &pBuffer);
    switch (chunk)
    {
        case HTML_NOTHING_TO_SEND:
            id++;
            if (id >= ESP_ID_COUNT) id = 0;
            break;
        case HTML_MORE_TO_SEND:
            AtSendData(id, length, pBuffer, NULL);
            break;
        case HTML_NO_MORE_TO_SEND:
            AtClose(id, NULL);
            break;
        case HTML_ERROR:
            return -1;
    }
    return 0;
}
int ServerMain()
{
    if (!WifiStarted()) return 0; //Do nothing until WiFi is running

    if (startMain()) return -1;
    if ( recvMain()) return -1;
    if ( sendMain()) return -1;
    
    return 0;
}
int ServerInit(void) //Make sure this is only called after any other ids are reserved.
{
    RequestInit();
    HtmlInit();
    return 0;
}
