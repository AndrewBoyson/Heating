extern int  HtmlInit();
extern void HtmlStart(int id, int whatToSend);
extern int  HtmlGetNextChunkToSend(int id, int* pLength, char** ppBuffer);