#include "mbed.h"
#include  "rtc.h"
#define LOG_FILE "/local/log.txt"

LocalFileSystem local("local");

#define BUFFER_LENGTH 4096
static char buffer[BUFFER_LENGTH];
static char* pPush; //Initialised in init
static char* pPull; //Initialised in init
static int enable = true;
static char* incrementPushPullPointer(char* p, char* buffer, int bufferLength)
{
    p++; //increment the pointer by one
    if (p == buffer + bufferLength) p = buffer; //if the pointer is now beyond the end then point it back to the start
    return p;
}
void LogPush(char c)
{
    //Only add if allowed
    if (!enable) return;
    
    //Move the pull position if about to run into it
    char* pNext = incrementPushPullPointer(pPush, buffer, BUFFER_LENGTH);
    if (pNext == pPull) pPull = incrementPushPullPointer(pPull, buffer, BUFFER_LENGTH);
    
    //Add the character at the push position
    *pPush = c;
    pPush = incrementPushPullPointer(pPush, buffer, BUFFER_LENGTH);
}
static char *pEnumerate;
static int hadNull;
void LogEnumerateStart()
{
    pEnumerate = pPull;
    hadNull = false;
}
int LogEnumerate()
{
    if (hadNull)
    {
        hadNull = false;    
        return '0';
    }
    if (pEnumerate == pPush) return EOF;
    char c = *pEnumerate;
    pEnumerate = incrementPushPullPointer(pEnumerate, buffer, BUFFER_LENGTH); 
    if (c)
    {
        return c;
    }
    else
    {
        hadNull = true;
        return '\\';
    }
}
void LogEnable(int on)
{
    enable = on;
}
int LogInit()
{
    pPush = buffer;
    pPull = buffer;
    return 0;
}
void LogV(char *fmt, va_list argptr)
{
    int size  = vsnprintf(NULL, 0, fmt, argptr);      //Find the size required
    char snd[size + 1];                               //Allocate enough memory for the size required with an extra byte for the terminating null
    vsprintf(snd, fmt, argptr);                       //Fill the new buffer
    for (char* ptr = snd; *ptr; ptr++) LogPush(*ptr); //Send the string to the log buffer
}
void LogF(char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    LogV(fmt, argptr);
    va_end(argptr);
}
void LogCrLf (char * text)
{
    LogF("%s\r\n", text);
}
static void pushuint4(int value)
{    
    if      (value > 9999) { LogPush('+'); LogPush('+'); LogPush('+'); LogPush('+'); }
    else if (value <    0) { LogPush('-'); LogPush('-'); LogPush('-'); LogPush('-'); }
    else
    {
        div_t divres;
        int k, c, t, u;
        divres = div(value      , 10); u = divres.rem;
        divres = div(divres.quot, 10); t = divres.rem;
        divres = div(divres.quot, 10); c = divres.rem;
                                       k = divres.quot;                           
        LogPush(k + '0'); LogPush(c + '0'); LogPush(t + '0'); LogPush(u + '0');
    }
}
static void pushuint3(int value)
{
    if      (value > 999) { LogPush('+'); LogPush('+'); LogPush('+'); }
    else if (value <   0) { LogPush('-'); LogPush('-'); LogPush('-'); }
    else
    {
        div_t divres;
        int c, t, u;
        divres = div(value      , 10); u = divres.rem;
        divres = div(divres.quot, 10); t = divres.rem;
                                       c = divres.quot;
        LogPush(c + '0'); LogPush(t + '0'); LogPush(u + '0');
    }
}
static void pushuint2(int value)
{
    if      (value > 99) { LogPush('+'); LogPush('+'); }
    else if (value <  0) { LogPush('-'); LogPush('-'); }
    else
    {
        div_t divres;
        int t, u;
        divres = div(value      , 10); u = divres.rem;
                                       t = divres.quot;
        LogPush(t + '0'); LogPush(u + '0');
    }
}
void LogTime()
{
    struct tm tm;
    RtcGetTmUtc(&tm);
    
    pushuint4(tm.tm_year + 1900);
    LogPush('-');
    pushuint3(tm.tm_yday + 1);
    LogPush(' ');
    pushuint2(tm.tm_hour);
    LogPush(':');
    pushuint2(tm.tm_min);
    LogPush(':');
    pushuint2(tm.tm_sec);
}
void LogTimeCrLf(char * text)
{
    LogTime();
    LogF(" %s\r\n", text);
}
void LogTimeF(char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    LogTime();
    LogPush(' ');
    LogV(fmt, argptr);
    va_end(argptr);
}
void LogSave()
{
    FILE *fp = fopen(LOG_FILE, "w");
    LogEnumerateStart();
    while(1)
    {
        int c = LogEnumerate();
        if (c == EOF) break;
        putc(c, fp);
    }
    fclose(fp);
}
