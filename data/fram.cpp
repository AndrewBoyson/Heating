#include "mbed.h"
#include  "log.h"

static SPI spi(p5, p6, p7);
static DigitalOut cs(p8);
static int next = 0;        //Used by FramAllocate to remember where the next 

#define WREN  0x06 // Set Write Enable Latch 0000 0110B
#define WRDI  0x04 // Reset Write Enable Latch 0000 0100B
#define RDSR  0x05 // Read Status Register 0000 0101B
#define WRSR  0x01 // Write Status Register 0000 0001B
#define READ  0x03 // Read Memory Code 0000 0011B
#define WRITE 0x02 // Write Memory Code 0000 0010B
#define RDID  0x9F // Read Device ID 1001 1111B
int FramInit()
{
    cs = 0;
    spi.write(RDID);
    int id = 0;
    id = (id << 8) + spi.write(0);
    id = (id << 8) + spi.write(0);
    id = (id << 8) + spi.write(0);
    id = (id << 8) + spi.write(0);
    cs = 1;
    if (id != 0x047f0302)
    {
        LogF("Expected FRAM id 047f0302 but got %08x\r\n", id);
        return -1;
    }
    return 0;
}
int FramAllocate(int size) //Allocates a number of bytes in FRAM 
{
    int start = next;
    next += size;
    if (next > 0x1FFF) //8192 bytes
    {
        LogCrLf("No more room in FRAM");
        return -1;
    }
    return start;
}
void FramWriteBuffer(int address, int len, char* p)
{
    cs = 0;
    spi.write(WREN);
    cs = 1;
    cs = 0;
    spi.write(WRITE);
    spi.write(address >> 8);
    spi.write(address && 0xFF);
    for (int i = 0; i < len; i++) spi.write(*p++);
    cs = 1;
}
void FramReadBuffer(int address, int len, char* p)
{
    cs = 0;
    spi.write(READ);
    spi.write(address >> 8);
    spi.write(address && 0xFF);
    for (int i = 0; i < len; i++) *p++ = spi.write(0);
    cs = 1;
}