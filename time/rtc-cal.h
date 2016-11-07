
extern int  RtcCalGetDivisor(); extern void  RtcCalSetDivisor(int value);
extern int  RtcCalInit();

extern void RtcCalStopAndResetCounter();
extern void RtcCalReleaseCounter();
extern void RtcCalSecondsHandler(int added);
extern void RtcCalTimeSetHandler(uint64_t thisRtc, uint64_t thisAct, struct tm* ptm);
extern  int RtcCalGetFraction();
extern  int RtcCalGet();
extern void RtcCalSet(int value);
