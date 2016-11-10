extern bool     NetEndianessSame;
extern int      NetInit();

extern uint64_t NetToHost64   (uint64_t n);
extern void     NetIp4BinToStr(char* bytes, int len, char* text);
extern void     NetIp4StrToBin(char* text, char* bytes);