#define HTML_NOT_FOUND   0
#define HTML_BAD_REQUEST 1
#define HTML_BAD_METHOD  2
#define HTML_LED         3
#define HTML_LOG         4

#define HTML_NOTHING_TO_SEND 0
#define HTML_MORE_TO_SEND    1
#define HTML_NO_MORE_TO_SEND 2
#define HTML_ERROR           3

extern int  HtmlInit();
extern void HtmlStart(int id, int whatToSend);
extern int  HtmlGetNextChunkToSend(int id, int* pLength, char** ppBuffer);