extern int  RtcClockIsNotSet();

extern void RtcClockStopAndResetDivider();
extern void RtcClockRelease();

extern volatile bool RtcClockHasIncremented;

extern void RtcClockGetTm(struct tm* ptm);
extern void RtcClockSetTm(struct tm* ptm);

extern void RtcClockInit();