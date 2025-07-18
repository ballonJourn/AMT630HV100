#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"

#include "board.h"
#include "sfud.h"
#include "updatefile.h"
#include "mmcsd_core.h"
#include "sysinfo.h"

static UpFileHeader *upfile = NULL;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
int GetUpFileInfo(void)
{
	UpFileHeader header;
	uint32_t headersize;
	sfud_flash *sflash;
	SysInfo *sysinfo = GetSysInfo();

	sflash = sfud_get_device(0);
	
	sfud_read(sflash, sysinfo->image_offset, sizeof(header), (void*)&header);
	if (header.magic != MKTAG('U', 'P', 'D', 'F')) {
		printf("Error! Wrong update file.\n");
		return -1;
	}
	headersize = sizeof(UpFileHeader) + sizeof(UpFileInfo) * header.filenum;
	upfile = pvPortMalloc(headersize);
	if (!upfile) return -1;
	sfud_read(sflash, sysinfo->image_offset, headersize, (void*)upfile);

	return 0;
}
#else
int GetUpFileInfo(void)
{
	UpFileHeader header;
	uint32_t headersize;
	SysInfo *sysinfo = GetSysInfo();

	emmc_read(sysinfo->image_offset, sizeof(header), (void*)&header);
	if (header.magic != MKTAG('U', 'P', 'D', 'F')) {
		printf("Error! Wrong update file.\n");
		return -1;
	}
	headersize = sizeof(UpFileHeader) + sizeof(UpFileInfo) * header.filenum;
	upfile = pvPortMalloc(headersize);
	if (!upfile) return -1;
	emmc_read(sysinfo->image_offset, headersize, (void*)upfile);

	return 0;
}
#endif

uint32_t GetUpFileOffset(uint32_t magic)
{
	int i;
	SysInfo *sysinfo = GetSysInfo();

	if (!upfile) return 0;

	for (i = 0; i < upfile->filenum; i++) {
		if (upfile->files[i].magic == magic) {
			return upfile->files[i].offset + sysinfo->image_offset;
		}
	}

	return 0;
}

uint32_t GetUpFileSize(uint32_t magic)
{
	int i;

	if (!upfile) return 0;

	for (i = 0; i < upfile->filenum; i++) {
		if (upfile->files[i].magic == magic) {
			return upfile->files[i].size;
		}
	}

	return 0;
}
