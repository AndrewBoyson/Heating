extern char* RadiatorGetHallRom();          extern void RadiatorSetHallRom         (char* value);
extern int   RadiatorGetNightTemperature(); extern void RadiatorSetNightTemperature(int   value);
extern int   RadiatorGetFrostTemperature(); extern void RadiatorSetFrostTemperature(int   value);

extern int RadiatorInit();
extern int RadiatorMain();