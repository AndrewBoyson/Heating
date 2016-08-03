extern void ScheduleSave(int index, char *text);
extern int  ScheduleRead(int index, int bufferSize, char *text);

extern bool ScheduleGetAuto();
extern void ScheduleSetAuto(bool value);

extern int  ScheduleGetMon();
extern void ScheduleSetMon(int value);
extern int  ScheduleGetTue();
extern void ScheduleSetTue(int value);
extern int  ScheduleGetWed();
extern void ScheduleSetWed(int value);
extern int  ScheduleGetThu();
extern void ScheduleSetThu(int value);
extern int  ScheduleGetFri();
extern void ScheduleSetFri(int value);
extern int  ScheduleGetSat();
extern void ScheduleSetSat(int value);
extern int  ScheduleGetSun();
extern void ScheduleSetSun(int value);

extern bool ScheduleIsCalling;
extern bool ScheduleOverride;

extern void ScheduleInit();
extern  int ScheduleMain();