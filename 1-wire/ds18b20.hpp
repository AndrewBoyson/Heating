#define DS18B20_ERROR_CRC                     0x7FFF
#define DS18B20_ERROR_NOT_FOUND               0x7FFE
#define DS18B20_ERROR_TIMED_OUT               0x7FFD
#define DS18B20_ERROR_NO_DEVICE_PRESENT       0x7FFC
#define DS18B20_ERROR_NO_DEVICE_PARTICIPATING 0x7FFB

#define DS18B20_VALUE_STRING_LENGTH 32
#define DS18B20_FAMILY_CODE 0x28

extern int16_t DS18B20Value[];
extern int16_t DS18B20ValueFromRom(char* rom);
extern bool    DS18B20IsValidValue(int16_t value);
extern void    DS18B20ValueToString(int16_t value, char* buffer);
extern void    DS18B20ReadValue(int oneWireResult, int device, char byte0, char byte1);