#define SETTINGS_SCHEDULE_REG_COUNT 5

extern int  SettingsGetScheduleReg(int index);
extern void SettingsSetScheduleReg(int index, int value);

extern int  SettingsGetTankSetPoint();
extern void SettingsSetTankSetPoint(int value);
extern int  SettingsGetTankHysteresis();
extern void SettingsSetTankHysteresis(int value);
extern int  SettingsGetTankMinHoldOnLoss();
extern void SettingsSetTankMinHoldOnLoss(int value);