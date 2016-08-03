#define RTC_RESOLUTION_BITS 11

extern int      RtcInit();
extern uint64_t RtcGet();
extern void     RtcSet(uint64_t t);
extern int      RtcGetGenReg(int index);
extern void     RtcSetGenReg(int index, int value);