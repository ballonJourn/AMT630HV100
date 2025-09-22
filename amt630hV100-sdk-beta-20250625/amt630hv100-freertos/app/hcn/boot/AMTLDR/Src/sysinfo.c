#include "amt630h.h"
#include "sysinfo.h"
#include "crc32.h"

extern int SpiReadSysInfo(SysInfo *info);
extern void SpiWriteSysInfo(SysInfo *info);
extern int EmmcReadSysInfo(SysInfo *info);
extern void EmmcWriteSysInfo(SysInfo *info);

static SysInfo sysinfo = {0};

SysInfo *GetSysInfo(void)
{
	return &sysinfo;
}

void SetDefaultSysInfo(void)
{
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	sysinfo.update_media_type = UPDATE_MEDIA_SD;
#else
	sysinfo.update_media_type = UPDATE_MEDIA_USB;
	sysinfo.loader_offset = LOADER_OFFSET;
	sysinfo.loader_size = LOADER_MAX_SIZE;
#endif
	sysinfo.image_offset = IMAGE_OFFSET;
	sysinfo.update_status = UPDATE_STATUS_START;
	sysinfo.stepldr_offset = STEPLDRB_OFFSET;
	sysinfo.stepldr_size = STEPLDR_MAX_SIZE;
}

int ReadSysInfo(void)
{
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	return SpiReadSysInfo(&sysinfo);
#else
	return EmmcReadSysInfo(&sysinfo);
#endif	
}

void SaveSysInfo(SysInfo *info)
{
	if (!info)
		info = &sysinfo;
	info->checksum = xcrc32((unsigned char*)info, sizeof(SysInfo) - 4, 0xffffffff);
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	SpiWriteSysInfo(info);
#else	
	EmmcWriteSysInfo(info);
#endif		
}