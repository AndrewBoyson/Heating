extern char* NtpGetIp();              extern void  NtpSetIp              (char *value);
extern int   NtpGetInitialInterval(); extern void  NtpSetInitialInterval (int   value);
extern int   NtpGetNormalInterval();  extern void  NtpSetNormalInterval  (int   value);
extern int   NtpGetRetryInterval();   extern void  NtpSetRetryInterval   (int   value);
extern int   NtpGetOffsetMs();        extern void  NtpSetOffsetMs        (int   value);
extern int   NtpGetMaxDelayMs();      extern void  NtpSetMaxDelayMs      (int   value);

extern int  NtpInit();
extern int  NtpSendRequest();
extern int  NtpHandleResponse();
extern int  NtpMain();
extern void NtpRequestReconnect();