#define SERVER_NOTHING_TO_SEND 0
#define SERVER_MORE_TO_SEND    1
#define SERVER_NO_MORE_TO_SEND 2
#define SERVER_ERROR           3

extern int ServerInit(void);
extern int ServerMain(void);