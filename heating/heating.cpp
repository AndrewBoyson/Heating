#include "mbed.h"
#include  "rtc.h"

void HeatingSetSchemeA(int index, int size, char* text)
{
    char buffer[size+1];
    for (int i = 0; i < size; i++) buffer[i] = text[i];
    buffer[size] = 0;
    int value;
    sscanf(buffer, "%d", &value);
    RtcSetGenReg(index, value);
}
int HeatingGetSchemeA(int index, int bufSize, char* text)
{
    int size = snprintf(text, bufSize, "%d", RtcGetGenReg(index));
    if (size > bufSize - 1) return bufSize - 1;
    return size;
}