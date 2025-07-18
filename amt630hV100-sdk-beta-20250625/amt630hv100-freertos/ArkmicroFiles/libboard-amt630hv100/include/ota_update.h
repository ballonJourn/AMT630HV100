#ifndef _OTA_UPDATE_H
#define _OTA_UPDATE_H

#include "board.h"
#include "sysinfo.h"

typedef enum {
	UPFILE_TYPE_WHOLE,
	UPFILE_TYPE_RESOURCE,
	UPFILE_TYPE_ANIMATION,
	UPFILE_TYPE_APP,
	UPFILE_TYPE_FIRSTLDR,
	UPFILE_TYPE_STEPLDR,
	UPFILE_TYPE_LNCHEMMC,
	UPFILE_TYPE_NUM,
} eUpfileType;

extern const char *g_upfilename[UPFILE_TYPE_NUM];

#define SF_MOUNT_PATH			"/sf"
#define	SDMMC_MOUNT_PATH		"/sd"
#if DEVICE_TYPE_SELECT == EMMC_FLASH
#define OTA_MOUNT_PATH			SDMMC_MOUNT_PATH
#else
#define OTA_MOUNT_PATH			SF_MOUNT_PATH
#endif
#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
#define IMAGE_RW_SIZE			0x20000
#else
#define IMAGE_RW_SIZE			0x10000
#endif
#define UPFILE_APP_MAGIC		0xe59ff030
#define APPLDR_CHECKSUM_OFFSET	20


int update_from_media(char *mpath, int filetype);
/* ?????????, toburn??0????????????? */
unsigned int get_upfile_offset(int filetype, int toburn);
void set_upfile_offset(SysInfo *sysinfo, int filetype, uint32_t offset);
unsigned int get_upfile_size(int filetype);
uint32_t get_upfile_checksum(int filetype, size_t filesize, int checkmode, int toburn);
int backup_whole_image(void);
unsigned int get_subfile_maxsize(int filetype);
void update_part_upfile_info(SysInfo *sysinfo, int filetype, int filesize);

#endif
