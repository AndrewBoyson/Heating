extern int  ResourceInit();
extern void ResourceStart(int id, int watToSend);
extern int  ResourceGetNextChunkToSend(int id, int* pLength, char** ppBuffer);