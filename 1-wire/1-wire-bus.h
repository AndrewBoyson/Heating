extern volatile int OneWireBusValue;
extern int OneWireBusBusy();
extern int OneWireBusInit();
extern void OneWireBusReset();
extern void OneWireBusWriteBit(int bit);
extern void OneWireBusWriteBitWithPullUp(int bit, int pullupms);
extern void OneWireBusReadBit();