#ifndef SD_IF_H__
#define SD_IF_H__

int MMC_disk_initialize();

int MMC_disk_read(void *buff, DWORD sector, BYTE count);

int MMC_disk_ioctl(BYTE ctrl, void *buff);

#endif
