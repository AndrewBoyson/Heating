#define WIFI_STOPPED    0
#define WIFI_READY      2
#define WIFI_CONNECTED  3
#define WIFI_GOT_IP     4
extern int WifiStatus;
extern int WifiStarted(void);
extern int WifiMain(void);
