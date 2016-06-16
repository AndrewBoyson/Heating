#define ESP_IDLE       0
#define ESP_AVAILABLE  1
#define ESP_TIMEOUT    2
#define ESP_OVERFLOW   3

extern char  EspLine[];
extern char  EspResp[];
extern void *EspIpdBuffer[];
extern int   EspIpdBufferLen[];
extern int   EspIpdReserved[];
extern int   EspIpdId;
extern int   EspIpdLength;
extern int   EspLineAvailable;
extern int   EspDataAvailable;
extern int   EspLineIs(char * text);

extern int  EspMain();
extern void EspResetAndStop();
extern void EspReleaseFromReset(void);
extern int  EspInit(void);

extern int          EspLengthToSend;
extern const void * EspDataToSend;
extern void         EspSendData(int length, const void * buf);

extern void EspSendCommandF(char *fmt, ...);
extern void EspSendCommand (char * buf);
