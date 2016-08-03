#define SERVER_NOTHING_TO_SEND  0
#define SERVER_SEND_DATA        1
#define SERVER_CLOSE_CONNECTION 2
#define SERVER_ERROR            3

extern int ServerInit(void);
extern int ServerMain(void);