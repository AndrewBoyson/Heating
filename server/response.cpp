#include     "mbed.h"
#include "response.h"
#include   "server.h"
#include  "request.h"
#include     "http.h"
#include     "html.h"
#include "resource.h"
#include  "favicon.h"
#include      "css.h"

#define SEND_BUFFER_SIZE 1024
static char sendbuffer[SEND_BUFFER_SIZE];
static int length;

int ResponseAddChar(char c) //returns true when full
{
    sendbuffer[length++] = c;
    return length >= SEND_BUFFER_SIZE;
}
void ResponseAddChunkF(char *fmt, ...)
{
    //Set up variable arguments
    va_list argptr;
    va_start(argptr, fmt);
        
    //Fill the buffer
    int room = SEND_BUFFER_SIZE - length;
    int sent = vsnprintf(sendbuffer + length, room, fmt, argptr);
    if (sent > room) sent = room;
    if (sent < 0) sent = 0;
    
    //Finish with variable arguments
    va_end(argptr);
    
    //Return length
    length += sent;
    if (length > SEND_BUFFER_SIZE) length = SEND_BUFFER_SIZE;
}

void ResponseAddChunk(char *text)
{
    strncpy(sendbuffer + length, text, SEND_BUFFER_SIZE - length);
    length += strlen(text);
    if (length > SEND_BUFFER_SIZE) length = SEND_BUFFER_SIZE;
}

static int chunkToSendToId[4]; //0 == do nothing
static int  whatToSendToId[4];
static int      sameDateId[4];

int ResponseInit()
{    
    for (int id = 0; id < 4; id++)
    {
         whatToSendToId[id] = 0;
        chunkToSendToId[id] = 0;
             sameDateId[id] = false;
    }
    return 0;
}
void ResponseStart(int id, int whatToSend, char* lastModified)
{
    chunkToSendToId[id] = 1;
     whatToSendToId[id] = whatToSend;
     
     char fileDate[HTTP_DATE_SIZE];
     
    switch(whatToSendToId[id])
    {
        case REQUEST_ICO:
            HttpMakeDate(FaviconDate, FaviconTime, fileDate);
            sameDateId[id] = strcmp(fileDate, lastModified) == 0;
            break;
        case REQUEST_CSS:
            HttpMakeDate(CssDate, CssTime, fileDate);
            sameDateId[id] = strcmp(fileDate, lastModified) == 0;
            break;
        default:
            sameDateId[id] = false;
            break;
    }
}
int ResponseGetNextChunkToSend(int id, int** ppLength, const char** ppBuffer)
{
    
    *ppLength = &length;
    *ppBuffer = sendbuffer;
    
    length = 0;
         
    if (chunkToSendToId[id] == 0) return SERVER_NOTHING_TO_SEND;

    if (chunkToSendToId[id] == 1)
    {
        char fileDate[HTTP_DATE_SIZE];
        switch(whatToSendToId[id])
        {
            case REQUEST_LOG:         HttpOk("text/html; charset=utf-8", NULL, "no-cache"); break;
            case REQUEST_LED:         HttpOk("text/html; charset=utf-8", NULL, "no-cache"); break;
            case REQUEST_ICO:
                HttpMakeDate(FaviconDate, FaviconTime, fileDate);
                if (sameDateId[id]) HttpNotModified("image/x-icon", fileDate, "max-age=3600");
                else                HttpOk         ("image/x-icon", fileDate, "max-age=3600");
                break;
            case REQUEST_CSS:
                HttpMakeDate(CssDate, CssTime, fileDate);
                if (sameDateId[id]) HttpNotModified("text/css; charset=utf-8", fileDate, "max-age=3600");
                else                HttpOk         ("text/css; charset=utf-8", fileDate, "max-age=3600");
                break;
            case REQUEST_NOT_FOUND:   HttpNotFound();       break;
            case REQUEST_BAD_REQUEST: HttpBadRequest();     break;
            case REQUEST_BAD_METHOD:  HttpBadMethod();      break;
            default:                  HttpNotImplemented(); break;
        }
        chunkToSendToId[id] += 1;
        return SERVER_SEND_DATA;
    }
    
    int chunk = chunkToSendToId[id] - 2;
    int chunkResult;
    switch(whatToSendToId[id])
    {
        case REQUEST_LOG:         chunkResult = HtmlLog(chunk);          break;
        case REQUEST_LED:         chunkResult = HtmlLed(chunk);          break;
        case REQUEST_ICO:
            if (sameDateId[id])   chunkResult = RESPONSE_NO_MORE_CHUNKS;
            else                  chunkResult = ResourceIco(chunk);
            break;
        case REQUEST_CSS:
            if (sameDateId[id])   chunkResult = RESPONSE_NO_MORE_CHUNKS;
            else                  chunkResult = ResourceCss(chunk);
            break;
        case REQUEST_NOT_FOUND:   chunkResult = RESPONSE_NO_MORE_CHUNKS; break;
        case REQUEST_BAD_REQUEST: chunkResult = RESPONSE_NO_MORE_CHUNKS; break;
        case REQUEST_BAD_METHOD:  chunkResult = RESPONSE_NO_MORE_CHUNKS; break;
        default:                  chunkResult = RESPONSE_NO_MORE_CHUNKS; break;
    }
    switch (chunkResult)
    {
        case RESPONSE_SEND_PART_CHUNK: /*Don't increment the chunk*/ return SERVER_SEND_DATA;
        case RESPONSE_SEND_CHUNK:         chunkToSendToId[id] += 1;  return SERVER_SEND_DATA;
        case RESPONSE_NO_MORE_CHUNKS:     chunkToSendToId[id]  = 0;  return SERVER_CLOSE_CONNECTION;
        default:                                                     return SERVER_ERROR;
    }
}
