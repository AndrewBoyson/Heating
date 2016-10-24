extern int  FramInit();
extern int  FramAllocate(int size);
extern void FramWriteBuffer(int address, int len, char* p);
extern void FramReadBuffer(int address, int len, char* p);