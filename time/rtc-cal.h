extern void RtcCalSecondsHandler(struct tm* ptm, int fraction);
extern void RtcCalTimeSetHandler(uint64_t thisRtc, uint64_t thisAct, struct tm* ptm);
extern void RtcCalInit();
extern  int RtcCalGetFraction();
extern  int RtcCalGet();
extern void RtcCalSet(int value);
