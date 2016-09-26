#define RTC_RESOLUTION_BITS 11

extern int      RtcInit();
extern int      RtcMain();
extern void     RtcSet(uint64_t t);
extern uint64_t RtcGet();
extern void     RtcGetTmUtc(struct tm* ptm);
extern void     RtcGetTmLocal(struct tm* ptm);

