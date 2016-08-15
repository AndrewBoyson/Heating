#define SETTINGS_SCHEDULE_REG_COUNT 5

extern int  SettingsGetScheduleReg(int index);
extern void SettingsSetScheduleReg(int index, int value);

extern int  SettingsGetTankSetPoint();
extern void SettingsSetTankSetPoint(int value);
extern int  SettingsGetTankHysteresis();
extern void SettingsSetTankHysteresis(int value);
extern int  SettingsGetBoilerRunOnResidual();
extern void SettingsSetBoilerRunOnResidual(int value);
extern int  SettingsGetBoilerRunOnTime();
extern void SettingsSetBoilerRunOnTime(int value);
extern int  SettingsGetNightTemperature();
extern void SettingsSetNightTemperature(int value);
extern int  SettingsGetFrostTemperature();
extern void SettingsSetFrostTemperature(int value);
