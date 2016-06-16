#include "mbed.h"
#include <stdarg.h>
#include "time.h"
#define LOG_FILE "/local/log.txt"
#define DATE_FORMAT "%05d "

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
void LogF(char *fmt, ...)
{
    //Set up variable arguments
    va_list argptr;
    va_start(argptr, fmt);
    
    //Find the size required
    int size  = vsnprintf(NULL, 0, fmt, argptr);
    
    //Allocate enough memory for the size required with an extra byte for the terminating null
    char snd[size + 1];
    
    //Fill the new buffer
    vsprintf(snd, fmt, argptr);
    
    //Finish with variable arguments
    va_end(argptr);
    
    //Send the string to the log buffer
    for (char* ptr = snd; *ptr; ptr++) LogPush(*ptr);
    
}
void LogCrLf (char * text)
{
    LogF("%s\r\n", text);
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
