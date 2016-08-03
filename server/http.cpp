#include "mbed.h"
#include "http.h"
#include "time.h"
#include "response.h"

static void dateHeaderFromTm(struct tm* ptm, char* ptext)
{
    size_t size = strftime(ptext, HTTP_DATE_SIZE, "%a, %d %b %Y %H:%M:%S GMT", ptm);//Sun, 06 Nov 1994 08:49:37 GMT  ; RFC 822, updated by RFC 1123
}
void dateHeaderNow(char* ptext)
{
    time_t now = time(NULL);
    struct tm* ptm = localtime(&now);
    dateHeaderFromTm(ptm, ptext);
}
void HttpMakeDate(const char* date, const char *ptime, char* ptext)
{
    struct tm tm;
    TimeAsciiDateTimeToTm(date, ptime, &tm);
    dateHeaderFromTm(&tm, ptext);
}

static void addHeaders(char *pResponse, char *pAllow, char *pContentType, char *pLastModified, char *pCacheControl)
{
    ResponseAddChunk("HTTP/1.1 ");
    ResponseAddChunk(pResponse);
    ResponseAddChunk("\r\n");
    
    ResponseAddChunk("Connection: close\r\n");
    
    ResponseAddChunk("Date: ");
    char date[HTTP_DATE_SIZE];
    dateHeaderNow(date);
    ResponseAddChunk(date);
    ResponseAddChunk("\r\n");
    
    if (pAllow)
    {
        ResponseAddChunk("Allow: ");
        ResponseAddChunk(pAllow);
        ResponseAddChunk("\r\n");
    }    
    if (pContentType)
    {
        ResponseAddChunk("Content-Type: ");
        ResponseAddChunk(pContentType);
        ResponseAddChunk("\r\n");
    }
    if (pLastModified)
    {
        ResponseAddChunk("Last-Modified: ");
        ResponseAddChunk(pLastModified);
        ResponseAddChunk("\r\n");
    }
    if (pCacheControl)
    {
        ResponseAddChunk("Cache-Control: ");
        ResponseAddChunk(pCacheControl);
        ResponseAddChunk("\r\n");
    }
    ResponseAddChunk("\r\n");    
}
void HttpOk            (char *pContentType, char *pLastModified, char *pCacheControl) { addHeaders("200 OK",                 NULL,  pContentType, pLastModified, pCacheControl); }
void HttpNotModified   (char *pContentType, char *pLastModified, char *pCacheControl) { addHeaders("304 Not Modified",       NULL,  pContentType, pLastModified, pCacheControl); }
void HttpNotFound      ()                                                             { addHeaders("404 Not Found",          NULL,  NULL,         NULL,  NULL); }
void HttpBadRequest    ()                                                             { addHeaders("400 Bad Request",        NULL,  NULL,         NULL,  NULL); }
void HttpBadMethod     ()                                                             { addHeaders("405 Method Not Allowed", "GET", NULL,         NULL,  NULL); }
void HttpNotImplemented()                                                             { addHeaders("501 Not Implemented",    NULL,  NULL,         NULL,  NULL); }
