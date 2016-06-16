#include "mbed.h"
static RawSerial uart(p9, p10); // tx, rx

#define RECV_BUFFER_LENGTH 1024
#define SEND_BUFFER_LENGTH 1024
static char recvbuffer[RECV_BUFFER_LENGTH];
static char sendbuffer[SEND_BUFFER_LENGTH];
static char* pRecvPush; //Initialised in init
static char* pRecvPull; //Initialised in init
static char* pSendPush; //Initialised in init
static char* pSendPull; //Initialised in init

static void incrementPushPullPointer(char** pp, char* pbuffer, int bufferlength)
{
    ++*pp; //increment the pointer by one
    if (*pp == pbuffer + bufferlength) *pp = pbuffer; //if the pointer is now beyond the end then point it back to the start
}
static void recvpush(void) //Called by the uart data received interrupt.
{
    while (uart.readable())
    {
        int c = uart.getc();
        *pRecvPush = c;
        incrementPushPullPointer(&pRecvPush, recvbuffer, RECV_BUFFER_LENGTH);
    }
}
static int sendpull(void) //Called every scan by UartMain. Returns the next byte or EOF if no more are available
{
    if (pSendPull == pSendPush) return EOF;
    char c = *pSendPull;
    incrementPushPullPointer(&pSendPull, sendbuffer, SEND_BUFFER_LENGTH);
    return c;
}
int UartRecvPull(void) //Returns the next byte or EOF if no more are available
{
    if (pRecvPull == pRecvPush) return EOF;
    char c = *pRecvPull;
    incrementPushPullPointer(&pRecvPull, recvbuffer, RECV_BUFFER_LENGTH); 
    return c;
}
void UartSendPush(char c) //Called whenever something needs to be sent
{
    *pSendPush = c;
    incrementPushPullPointer(&pSendPush, sendbuffer, SEND_BUFFER_LENGTH);
}
void UartBaud(int baud)
{
    uart.baud(baud);
}
void UartReset()
{
    pRecvPush = recvbuffer;
    pRecvPull = recvbuffer;
    pSendPush = sendbuffer;
    pSendPull = sendbuffer;
}
int UartInit()
{
    UartReset();
    uart.attach(&recvpush, Serial::RxIrq);
    return 0;
}
int UartMain()
{
    while(uart.writeable())
    {
        int c = sendpull();
        if (c == EOF) break;
        uart.putc(c);
    }
    return 0;
}