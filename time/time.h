extern int TimeScanUs;

extern int TimeMain();

extern int TimeAsciiDateTimeToTm(const char* pDate, const char* pTime, struct tm* ptm);
extern void TimeToTmLocal(uint32_t time, struct tm* ptm);
extern void TimeToTmUtc(uint32_t time, struct tm* ptm);
