#include     "mbed.h"
#include  "favicon.h"
#include      "css.h"
#include   "server.h"
#include  "request.h"
#include "response.h"

int ResourceIco(int chunk)
{
    static const char *p;
    static const char *pEnd;
    if(chunk == 0)
    {
        p = Favicon;
        pEnd = Favicon + FaviconSize;
    }
    int sent = false;
    while (p < pEnd)
    {
        sent = true;
        if (ResponseAddChar(*p++)) break;
    }
    if (sent) return RESPONSE_SEND_CHUNK;
    else      return RESPONSE_NO_MORE_CHUNKS;
}
int ResourceCss(int chunk)
{
    static const char *p;
    if(chunk == 0)
    {
        p = Css;
    }
    int sent = false;
    while (*p)
    {
        sent = true;
        if (ResponseAddChar(*p++)) break;
    }
    if (sent) return RESPONSE_SEND_CHUNK;
    else      return RESPONSE_NO_MORE_CHUNKS;
}

