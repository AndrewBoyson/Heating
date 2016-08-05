extern void ScheduleSaveAll();
extern int  ScheduleLoadAll();

extern bool ScheduleAuto;

extern int ScheduleDay[];

extern bool ScheduleCycleOn[3][4];
extern int  ScheduleCycleMinutes[3][4];

extern void ScheduleToString(int schedule, int buflen, char* buffer);
extern void ScheduleParse(int schedule, char* text);

extern bool ScheduleIsCalling;
extern bool ScheduleOverride;

extern void ScheduleInit();
extern  int ScheduleMain();