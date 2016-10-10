extern void   TimeTmUtcToLocal(struct tm* ptm);

extern void   TimeAsciiDateTimeToTm(const char* pDate, const char* pTime, struct tm* ptm);
extern void   TimeToTmLocal(uint32_t time, struct tm* ptm);
extern void   TimeToTmUtc(uint32_t time, struct tm* ptm);

extern time_t TimeFromTmUtc(struct tm* ptm);
extern int    TimePeriodBetween(struct tm* ptmLater, struct tm* ptmEarlier);