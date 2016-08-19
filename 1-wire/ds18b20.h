#define DS18B20_ERROR_CRC                     0x7FFF
#define DS18B20_ERROR_NOT_FOUND               0x7FFE
#define DS18B20_ERROR_TIMED_OUT               0x7FFD
#define DS18B20_ERROR_NO_DEVICE_PRESENT       0x7FFC
#define DS18B20_ERROR_NO_DEVICE_PARTICIPATING 0x7FFB

#define DS18B20_VALUE_STRING_LENGTH 32
#define DS18B20_ADDRESS_STRING_LENGTH 24

extern int DS18B20ScanMs;

extern int     DS18B20DeviceCount;
extern char    DS18B20DeviceList[];
extern int16_t DS18B20Value[];
extern int16_t DS18B20ValueFromRom(char* rom);

extern bool DS18B20IsValidValue(int16_t value);
extern void DS18B20ValueToString(int16_t value, char* buffer);
extern void DS18B20AddressToString(char* pAddress, char* buffer);
extern int  DS18B20ParseAddress(char* pText, char *pAddress);

extern int DS18B20Busy();
extern int DS18B20Init();
extern int DS18B20Main();