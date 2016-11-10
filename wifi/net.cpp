#include <mbed.h>

bool NetEndianessSame;

int NetInit()
{
    int    testInt   = 0x0001;           //Big end contains 0x00; little end contains 0x01
    int*  pTestInt   = &testInt;
    char* pTestByte  = (char*)pTestInt;
    char   testByte  = *pTestByte;       //Fetch the first byte
    NetEndianessSame = testByte == 0x00; //If the first byte is the big end then host and network have same endianess
    return 0;
}

uint64_t NetToHost64(uint64_t n) {
    
    if (NetEndianessSame) return n;
    
    union { uint64_t Whole; char Bytes[8]; } h;
    h.Whole = n;
    
    char t;
    t = h.Bytes[7]; h.Bytes[7] = h.Bytes[0]; h.Bytes[0] = t;
    t = h.Bytes[6]; h.Bytes[6] = h.Bytes[1]; h.Bytes[1] = t;
    t = h.Bytes[5]; h.Bytes[5] = h.Bytes[2]; h.Bytes[2] = t;
    t = h.Bytes[4]; h.Bytes[4] = h.Bytes[3]; h.Bytes[3] = t;
    
    return h.Whole;
}

void NetIp4BinToStr(char* bytes, int len, char* text)
{
    snprintf(text, len, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
}

void NetIp4StrToBin(char* text, char* bytes)
{
    int ints[4];
    sscanf(text, "%d.%d.%d.%d", &ints[3], &ints[2], &ints[1], &ints[0]);
    bytes[3] = ints[3] & 0xFF;
    bytes[2] = ints[2] & 0xFF;
    bytes[1] = ints[1] & 0xFF;
    bytes[0] = ints[0] & 0xFF;
}
