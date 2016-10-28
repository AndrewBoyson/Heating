#include "mbed.h"
#include  "log.h"

static SPI spi(p5, p6, p7);
static DigitalOut cs(p8);

#define WREN  0x06 // Set Write Enable Latch   0000 0110B
#define WRDI  0x04 // Reset Write Enable Latch 0000 0100B
#define RDSR  0x05 // Read Status Register     0000 0101B
#define WRSR  0x01 // Write Status Register    0000 0001B
#define READ  0x03 // Read Memory Code         0000 0011B
#define WRITE 0x02 // Write Memory Code        0000 0010B
#define RDID  0x9F // Read Device ID           1001 1111B

#define FRAM_SIZE   8192
#define FRAM_ID     0x047f0302
#define FRAM_MAGIC  42          //Magic number to show that the FRAM has been intialised
#define FRAM_FREQ   10000000    //10MHz

bool FramEmpty; //Set in FramInit
int  FramUsed;  //Initialised in FramInit and used by FramAllocate to remember the next available location

int FramAllocate(int size) //Allocates a number of bytes in FRAM 
{
    int start = FramUsed;
    int used = FramUsed + size;
    if (used > FRAM_SIZE - 1)
    {
        LogCrLf("FramAllocate - No more room in FRAM");
        return -1;
    }
    FramUsed = used;
    return start;
}
int FramInit()
{
    //Configure SPI
    spi.frequency(FRAM_FREQ);
    spi.format(8, 0);         //bits, mode
    
    //Check if the FRAM is connected and working properly
    cs = 0;
    spi.write(RDID);
    int id = 0;
    id = (id << 8) + spi.write(0);
    id = (id << 8) + spi.write(0);
    id = (id << 8) + spi.write(0);
    id = (id << 8) + spi.write(0);
    cs = 1;
    if (id != FRAM_ID)
    {
        LogF("FramInit - Expected FRAM id %08x but got %08x\r\n", FRAM_ID, id);
        return -1;
    }
    
    //Check the first byte to see if the FRAM is initialised and zero if not
    cs = 0;
    spi.write(READ);
    spi.write(0);
    spi.write(0);
    char magic = spi.write(0);
    cs = 1;
    FramEmpty = magic != FRAM_MAGIC;
    if (FramEmpty)
    {
        LogF("FramInit - Byte 0 value %d is not the magic value %d so initialising FRAM to zero\r\n", magic, FRAM_MAGIC);
        cs = 0;
        spi.write(WREN);
        cs = 1; cs = 1; cs = 1; cs = 1; cs = 1; cs = 1;
        cs = 0;
        spi.write(WRITE);
        spi.write(0);
        spi.write(0);
        spi.write(FRAM_MAGIC);                           //Set the magic number in byte 0
        for(int i = 1; i < FRAM_SIZE; i++) spi.write(0); //Zero all other locations
        cs = 1;
    }
    FramUsed = 1; //Set the next available location to one past the byte used for the magic number
    return 0;
}
void FramWrite(int address, int len, void* pVoid)
{
    if (address + len > FRAM_SIZE - 1 || address < 0)
    {
        LogCrLf("FramWrite - Invalid FRAM address");
        return;
    }
    cs = 0;
    spi.write(WREN);
    cs = 1; cs = 1; cs = 1; cs = 1; cs = 1; cs = 1; //Deselect must be at least 60ns. One operation at 96MHz is about 10ns
    cs = 0;
    spi.write(WRITE);
    spi.write(address >> 8);
    spi.write(address & 0xFF);
    char* p = (char*)pVoid;
    for (int i = 0; i < len; i++) spi.write(*p++);
    cs = 1;
}
void FramRead(int address, int len, void* pVoid)
{
    if (address + len > FRAM_SIZE - 1 || address < 0)
    {
        LogCrLf("FramRead - Invalid FRAM address");
        return;
    }
    cs = 0;
    spi.write(READ);
    spi.write(address >> 8);
    spi.write(address & 0xFF);
    char* p = (char*)pVoid;
    for (int i = 0; i < len; i++) *p++ = spi.write(0);
    cs = 1;
}

int FramLoad(int len, void* pValue, void* pDefault)
{
    int address = FramAllocate(len);
    if (address >= 0) 
    {
        if (FramEmpty)
        {
            if (pDefault)
            {
                bool defaultHasContent = false;
                char* pFrom = (char*)pDefault;
                char* pTo   = (char*)pValue;
                for (int i = 0; i < len; i++);
                {
                    if (*pFrom) defaultHasContent = true;
                    *pTo++ = *pFrom++;
                }
                if (defaultHasContent) FramWrite(address, len, pValue);
            }
            else //pDefault is NULL so zero the setting
            {
                memset(pValue, 0, len);
            }
        }
        else
        {
            FramRead(address, len, pValue);
        }
    }
    return address;
}
