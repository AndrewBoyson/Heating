#define SETTINGS_SCHEDULE_REG_COUNT 5

extern int  SettingsGetScheduleReg(int index);
extern void SettingsSetScheduleReg(int index, int value);


extern int  SettingsGetRtcFraction();
extern void SettingsSetRtcFraction(int value);


extern int  SettingsGetProgramPosition();
extern int  SettingsGetTankSetPoint();
extern int  SettingsGetTankHysteresis();
extern int  SettingsGetBoilerRunOnResidual();
extern int  SettingsGetBoilerRunOnTime();
extern int  SettingsGetNightTemperature();
extern int  SettingsGetFrostTemperature();

extern void SettingsSetProgramPosition(int value);
extern void SettingsSetTankSetPoint(int value);
extern void SettingsSetTankHysteresis(int value);
extern void SettingsSetBoilerRunOnResidual(int value);
extern void SettingsSetBoilerRunOnTime(int value);
extern void SettingsSetNightTemperature(int value);
extern void SettingsSetFrostTemperature(int value);


extern char* SettingsGetTankRom();
extern char* SettingsGetBoilerOutputRom();
extern char* SettingsGetBoilerReturnRom();
extern char* SettingsGetHallRom();

extern void  SettingsSetTankRom              (char* value);
extern void  SettingsSetBoilerOutputRom      (char* value);
extern void  SettingsSetBoilerReturnRom      (char* value);
extern void  SettingsSetHallRom              (char* value);


extern char* SettingsGetClockNtpIp();
extern int   SettingsGetClockInitialInterval();
extern int   SettingsGetClockNormalInterval();
extern int   SettingsGetClockRetryInterval();
extern int   SettingsGetClockOffsetMs();
extern int   SettingsGetClockCalDivisor();

extern void  SettingsSetClockNtpIp           (char *value);
extern void  SettingsSetClockInitialInterval (int   value);
extern void  SettingsSetClockNormalInterval  (int   value);
extern void  SettingsSetClockRetryInterval   (int   value);
extern void  SettingsSetClockOffsetMs        (int   value);
extern void  SettingsSetClockCalDivisor      (int   value);


extern int   SettingsInit();
