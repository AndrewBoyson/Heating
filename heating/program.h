extern bool  ProgramGetOverride     ();             extern void ProgramSetOverride     (              bool  value);
extern bool  ProgramGetAuto         ();             extern void ProgramSetAuto         (              bool  value);
extern int   ProgramGetDay          (int i);        extern void ProgramSetDay          (int i,        int   value);
extern int   ProgramGetNewDayHour   ();             extern void ProgramSetNewDayHour   (              int   value);


extern void ProgramToString(int i, int buflen, char* buffer);
extern void ProgramParse   (int i, char* text);

extern bool ProgramIsCalling;

extern  int ProgramMain();
extern  int ProgramInit();