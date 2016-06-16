extern int  LogInit(void);
extern void LogTime(void);
extern void LogF(char *fmt, ...);
extern void LogCrLf(char *);
extern void LogSave(void);
extern void LogPush(char c);
extern void LogEnumerateStart(void);
extern int  LogEnumerate(void);
extern void LogEnable(int on);
