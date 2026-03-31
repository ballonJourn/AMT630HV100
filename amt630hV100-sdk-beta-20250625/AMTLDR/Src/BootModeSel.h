#ifndef BOOT_MODE_SEL_H__
#define BOOT_MODE_SEL_H__

void bootFromSPI(void);
void bootFromNand(void);
void bootFromUsbHost();
void bootFromUsbDevice(int highspeed);
void bootFromUart(void);
void bootFromSD(int chipid, int bcheckfile);
void bootFromEMMC(int chipid);
void bootFromSpinand(void);

#endif

