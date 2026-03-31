#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"

#include "board.h"
#include "sfud.h"
#include "sysinfo.h"
#include "crc32.h"

#if DEVICE_TYPE_SELECT == EMMC_FLASH
#include "mmcsd_core.h"
#endif

#pragma data_alignment=4
static SysInfo amt630h_sysinfo;

int ReadSysInfo(void)
{
	unsigned int checksum;

#if DEVICE_TYPE_SELECT != EMMC_FLASH
	sfud_flash *sflash = sfud_get_device(0);
	
	sfud_read(sflash, SYSINFOA_MEDIA_OFFSET, sizeof(SysInfo), (void*)&amt630h_sysinfo);
#else
	emmc_read(SYSINFOA_MEDIA_OFFSET, sizeof(SysInfo), (void*)&amt630h_sysinfo);
#endif
	checksum = xcrc32((unsigned char*)&amt630h_sysinfo, sizeof(SysInfo) - 4, 0xffffffff);
	if (checksum == amt630h_sysinfo.checksum)
		return 0;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	sfud_read(sflash, SYSINFOB_MEDIA_OFFSET, sizeof(SysInfo), (void*)&amt630h_sysinfo);
#else
	emmc_read(SYSINFOB_MEDIA_OFFSET, sizeof(SysInfo), (void*)&amt630h_sysinfo);
#endif
	checksum = xcrc32((unsigned char*)&amt630h_sysinfo, sizeof(SysInfo) - 4, 0xffffffff);
	if (checksum == amt630h_sysinfo.checksum)
		return 0;	

	return -1;
}

SysInfo *GetSysInfo(void)
{
	return &amt630h_sysinfo;
}

void SaveSysInfo(void)
{
	amt630h_sysinfo.checksum = xcrc32((unsigned char*)&amt630h_sysinfo, sizeof(SysInfo) - 4, 0xffffffff);
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	sfud_flash *sflash = sfud_get_device(0);
	if(sflash) {
		sfud_erase_write(sflash, SYSINFOB_MEDIA_OFFSET, sizeof(SysInfo), (void*)&amt630h_sysinfo);
		sfud_erase_write(sflash, SYSINFOA_MEDIA_OFFSET, sizeof(SysInfo), (void*)&amt630h_sysinfo);
#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
		sfud_flush(sflash);
#endif
	}
#else
	emmc_write(SYSINFOB_MEDIA_OFFSET, sizeof(SysInfo), (void*)&amt630h_sysinfo);
	emmc_write(SYSINFOA_MEDIA_OFFSET, sizeof(SysInfo), (void*)&amt630h_sysinfo);
#endif
}
