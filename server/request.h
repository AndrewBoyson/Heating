#define REQUEST_NOT_FOUND   0
#define REQUEST_BAD_REQUEST 1
#define REQUEST_BAD_METHOD  2
#define REQUEST_HOME        3
#define REQUEST_TIMER       4
#define REQUEST_HEATING     5
#define REQUEST_BOILER      6
#define REQUEST_SYSTEM      7
#define REQUEST_LOG         8
#define REQUEST_AJAX        9
#define REQUEST_ICO        10
#define REQUEST_CSS        11

extern int RequestHandle(int id);
extern int RequestInit();