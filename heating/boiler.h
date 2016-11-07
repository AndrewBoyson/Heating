extern int   BoilerGetTankSetPoint();   extern void BoilerSetTankSetPoint  (int   value);
extern int   BoilerGetTankHysteresis(); extern void BoilerSetTankHysteresis(int   value);
extern int   BoilerGetRunOnResidual();  extern void BoilerSetRunOnResidual (int   value);
extern int   BoilerGetRunOnTime();      extern void BoilerSetRunOnTime     (int   value);
extern char* BoilerGetTankRom();        extern void BoilerSetTankRom       (char* value);
extern char* BoilerGetOutputRom();      extern void BoilerSetOutputRom     (char* value);
extern char* BoilerGetReturnRom();      extern void BoilerSetReturnRom     (char* value);

extern int   BoilerInit();
extern int   BoilerMain();




