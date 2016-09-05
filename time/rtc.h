#define RTC_RESOLUTION_BITS 11

extern int      RtcInit();
extern void     RtcSet(uint64_t t);
extern uint64_t RtcGet();

extern void     RtcGetTmUtc(struct tm* ptm);
extern void     RtcGetTmLocal(struct tm* ptm);

extern int      RtcGetCalibration();
extern void     RtcSetCalibration(int value);
