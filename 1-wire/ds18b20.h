#define DS18B20_ERROR_CRC                     0x7FFF
#define DS18B20_ERROR_NOT_FOUND               0x7FFE
#define DS18B20_ERROR_TIMED_OUT               0x7FFD
#define DS18B20_ERROR_NO_DEVICE_PRESENT       0x7FFC
#define DS18B20_ERROR_NO_DEVICE_PARTICIPATING 0x7FFB

extern int     DS18B20DeviceCount;
extern char    DS18B20DeviceList[];
extern int16_t DS18B20Value[];
extern int16_t DS18B20ValueFromRom(char* rom);

extern int DS18B20Busy();
extern int DS18B20Init();
extern int DS18B20Main();