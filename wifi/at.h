
//These are the statuses fed back to each command if pStatus is not null. Negative on failure.
#define AT_NONE               0
#define AT_SUCCESS            1
#define AT_ERROR             -1
#define AT_RESPONCE_TIMEOUT  -2
#define AT_LINE_TIMEOUT      -3
#define AT_LINE_OVERFLOW     -4

extern int AtInit();
extern int AtMain();
extern int AtBusy();

extern void AtResetAndStop();
extern void AtAt(int *pStatus);
extern void AtReleaseResetAndStart(int *pStatus);
extern void AtWaitForWifiConnected(int *pStatus);
extern void AtWaitForWifiGotIp(int *pStatus);
extern void AtConnect(const char *ssid, const char *password, int *pStatus);
extern void AtSendData(int id, int length, const void * pData, int *pStatus);
extern void AtConnectId(int id, char * type, char * addr, int port, void * buffer, int bufferlength, int *pStatus);
extern void AtStartServer(int port, int *pStatus);
extern void AtClose(int id, int *pStatus);
extern void AtSetMode(int mode, int *pStatus);
extern void AtMux(int *pStatus);
extern void AtBaud(int baud, int *pStatus);
extern int  AtEspStatus;
extern void AtGetEspStatus(int *pStatus);
extern char AtEspVersion[];
extern void AtGetEspVersion(int *pStatus);
