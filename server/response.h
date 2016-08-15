#define RESPONSE_SEND_PART_CHUNK 0
#define RESPONSE_SEND_CHUNK      1
#define RESPONSE_NO_MORE_CHUNKS  2

extern  int ResponseAddChar(char c);
extern void ResponseAddF(char *fmt, ...);
extern void ResponseAdd(char *text);

extern int  ResponseInit();
extern void ResponseStart(int id, int whatToSend, char* lastModified);
extern int  ResponseGetNextChunkToSend(int id, int** ppLength, const char** ppBuffer);

