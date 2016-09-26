extern void RtcCalSecondsHandler(int thisRtcSecond, int elapsedTim);
extern void RtcCalTimeSetHandler(uint64_t thisRtc, uint64_t thisAct, int thisRtcSecond);
extern void RtcCalInit();
extern  int RtcCalGet();
extern void RtcCalSet(int value);
