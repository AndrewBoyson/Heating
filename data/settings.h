extern int  SettingsGetMainPosition();
extern int  SettingsGetRtcFraction();

extern void SettingsSetMainPosition(int value);
extern void SettingsSetRtcFraction(int value);


extern int  SettingsGetTankSetPoint();
extern int  SettingsGetTankHysteresis();
extern int  SettingsGetBoilerRunOnResidual();
extern int  SettingsGetBoilerRunOnTime();
extern int  SettingsGetNightTemperature();
extern int  SettingsGetFrostTemperature();

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
extern int   SettingsGetClockNtpMaxDelayMs();
extern int   SettingsGetClockCalDivisor();

extern void  SettingsSetClockNtpIp           (char *value);
extern void  SettingsSetClockInitialInterval (int   value);
extern void  SettingsSetClockNormalInterval  (int   value);
extern void  SettingsSetClockRetryInterval   (int   value);
extern void  SettingsSetClockOffsetMs        (int   value);
extern void  SettingsSetClockNtpMaxDelayMs   (int   value);
extern void  SettingsSetClockCalDivisor      (int   value);

extern const int SettingsProgramCount;
extern const int SettingsProgramTransitionCount;
extern bool  SettingsGetProgramOverride     ();             extern void SettingsSetProgramOverride     (              bool  value);
extern bool  SettingsGetProgramAuto         ();             extern void SettingsSetProgramAuto         (              bool  value);
extern int   SettingsGetProgramDay          (int i);        extern void SettingsSetProgramDay          (int i,        int   value);
extern short SettingsGetProgramTransition   (int i, int j); extern void SettingsSetProgramTransition   (int i, int j, short value);


extern int   SettingsInit();

