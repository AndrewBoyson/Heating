#include "favicon.h"
#include  "server.h"
#include "request.h"

#define CHUNK_LENGTH 512
static char* pChunk;
static int   length;

#define NO_MORE_CHUNKS 0
#define MORE_CHUNKS    1
#define CHUNK_ERROR    2

static int  whatToSendToId[4];
static int chunkToSendToId[4]; //0 == do nothing

static int getChunk(int whatToSend, int chunk)
{
    switch (whatToSend)
    {
        case REQUEST_ICO:
            pChunk = favicon + (chunk - 1) * CHUNK_LENGTH;
            length = favicon + sizeof(favicon) - pChunk;
            break;
        default:
            pChunk = 0;
            length = 0;
            return CHUNK_ERROR;
    }
            
    if (length > CHUNK_LENGTH) length = CHUNK_LENGTH;
    if (length >= 0) return MORE_CHUNKS;
    else             return NO_MORE_CHUNKS;
}


int ResourceInit()
{
    for (int id = 0; id < 4; id++)
    {
         whatToSendToId[id] = 0;
        chunkToSendToId[id] = 0;
    }
    return 0;
}
void ResourceStart(int id, int whatToSend)
{
     whatToSendToId[id] = whatToSend;
    chunkToSendToId[id] = 1;           //Set up the next line to send to be the first
}
int ResourceGetNextChunkToSend(int id, int* pLength, char** ppBuffer)
{    
    if (chunkToSendToId[id] == 0) return SERVER_NOTHING_TO_SEND;
    
    int chunkResult = getChunk(whatToSendToId[id], chunkToSendToId[id]);
    
    *pLength = length;
    *ppBuffer = pChunk;    
    
    switch (chunkResult)
    {
        case MORE_CHUNKS:
            chunkToSendToId[id] += 1;
            return SERVER_MORE_TO_SEND;
        case NO_MORE_CHUNKS:
            chunkToSendToId[id] = 0;
            return SERVER_NO_MORE_TO_SEND;
        default:
            return SERVER_ERROR;
    }
}
