extern void ProgramSaveAll();
extern int  ProgramLoadAll();

extern bool ProgramAuto;

extern int ProgramDay[];

extern bool ProgramCycleOn[3][4];
extern int  ProgramCycleMinutes[3][4];

extern void ProgramToString(int schedule, int buflen, char* buffer);
extern void ProgramParse(int schedule, char* text);

extern bool ProgramIsCalling;
extern bool ProgramOverride;

extern void ProgramInit();
extern  int ProgramMain();