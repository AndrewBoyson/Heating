#define REQUEST_NOT_FOUND   0
#define REQUEST_BAD_REQUEST 1
#define REQUEST_BAD_METHOD  2
#define REQUEST_LED         3
#define REQUEST_LOG         4
#define REQUEST_ICO         5
#define REQUEST_CSS         6
#define REQUEST_SAME_ICO    7
#define REQUEST_SAME_CSS    8

extern int RequestHandle(int id);
extern int RequestInit();