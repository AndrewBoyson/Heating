#include     "mbed.h"
#include     "http.h"
#include     "time.hpp"
#include      "rtc.hpp"
#include "response.h"

static void dateHeaderFromTm(struct tm* ptm, char* ptext)
{
    size_t size = strftime(ptext, HTTP_DATE_SIZE, "%a, %d %b %Y %H:%M:%S GMT", ptm);//Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
}
void dateHeaderNow(char* ptext)
{
    struct tm tm;
    RtcGetTmUtc(&tm);
    dateHeaderFromTm(&tm, ptext);
}
void HttpMakeDate(const char* date, const char *ptime, char* ptext)
{
    struct tm tm;
    TimeAsciiDateTimeToTm(date, ptime, &tm);
    dateHeaderFromTm(&tm, ptext);
}

static void addHeaders(char *pResponse, char *pAllow, char *pContentType, char *pLastModified, char *pCacheControl)
{
    ResponseAdd("HTTP/1.1 ");
    ResponseAdd(pResponse);
    ResponseAdd("\r\n");
    
    ResponseAdd("Connection: close\r\n");
    
    ResponseAdd("Date: ");
    char date[HTTP_DATE_SIZE];
    dateHeaderNow(date);
    ResponseAdd(date);
    ResponseAdd("\r\n");
    
    if (pAllow)
    {
        ResponseAdd("Allow: ");
        ResponseAdd(pAllow);
        ResponseAdd("\r\n");
    }    
    if (pContentType)
    {
        ResponseAdd("Content-Type: ");
        ResponseAdd(pContentType);
        ResponseAdd("\r\n");
    }
    if (pLastModified)
    {
        ResponseAdd("Last-Modified: ");
        ResponseAdd(pLastModified);
        ResponseAdd("\r\n");
    }
    if (pCacheControl)
    {
        ResponseAdd("Cache-Control: ");
        ResponseAdd(pCacheControl);
        ResponseAdd("\r\n");
    }
    ResponseAdd("\r\n");    
}
void HttpOk            (char *pContentType, char *pLastModified, char *pCacheControl) { addHeaders("200 OK",                 NULL,  pContentType, pLastModified, pCacheControl); }
void HttpNotModified   (char *pContentType, char *pLastModified, char *pCacheControl) { addHeaders("304 Not Modified",       NULL,  pContentType, pLastModified, pCacheControl); }
void HttpNotFound      ()                                                             { addHeaders("404 Not Found",          NULL,  NULL,         NULL,  NULL); }
void HttpBadRequest    ()                                                             { addHeaders("400 Bad Request",        NULL,  NULL,         NULL,  NULL); }
void HttpBadMethod     ()                                                             { addHeaders("405 Method Not Allowed", "GET", NULL,         NULL,  NULL); }
void HttpNotImplemented()                                                             { addHeaders("501 Not Implemented",    NULL,  NULL,         NULL,  NULL); }
