#include <stdlib.h>
#include <unistd.h>

#include "FreeRTOS.h"

#include "board.h"
#include "chip.h"
#include "sfud.h"
#include "romfile.h"
#include "updatefile.h"
#include "animation.h"
#include "sysinfo.h"
#include "mmcsd_core.h"
#include "ff_stdio.h"
#include "source/crc32.h"
#include "ota_manage/hcn_ota.h"
#include "msg_manage/hcn_msg_manage.h"

#if DEVICE_TYPE_SELECT != EMMC_FLASH
#include "ff_sfdisk.h"
#endif

#ifdef OTA_UPDATE_SUPPORT
#include "ota_update.h"

const char *g_upfilename[UPFILE_TYPE_NUM] = {
	"update.bin",
	"rom.bin",
	"bootanim.bin",
	"amt630hv100.bin",
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	"emmcldr.bin",
#else
	"spildr.bin",
#endif
	"stepldr.bin",
	"lnchemmc.bin",
};

/* 获取已升级文件位置, toburn不为0时获取升级文件要烧录的位置 */
unsigned int get_upfile_offset(int filetype, int toburn)
{
	SysInfo *sysinfo = GetSysInfo();

	if (filetype == UPFILE_TYPE_WHOLE) {
		if (!toburn) return sysinfo->image_offset;
		if (sysinfo->image_offset == UPDATEFILE_MEDIA_OFFSET)
			return UPDATEFILE_MEDIA_B_OFFSET;
		else
			return UPDATEFILE_MEDIA_OFFSET;
	} else if (filetype == UPFILE_TYPE_FIRSTLDR) {
#if DEVICE_TYPE_SELECT == EMMC_FLASH
		if (!toburn) return sysinfo->loader_offset;
		if (sysinfo->loader_offset == LOADER_OFFSET)
			return LOADERB_OFFSET;
		else
			return LOADER_OFFSET;
#else
		return LOADER_OFFSET;
#endif
	} else if (filetype == UPFILE_TYPE_STEPLDR) {
		if (!toburn) return sysinfo->stepldr_offset;
		if (sysinfo->stepldr_offset == STEPLDRA_OFFSET)
			return STEPLDRB_OFFSET;
		else
			return STEPLDRA_OFFSET;
	} else if (filetype == UPFILE_TYPE_LNCHEMMC) {
		return 0;
	} else {
		uint8_t buf[512];
		UpFileHeader *header;
		UpFileInfo *appfile;
		unsigned int image_offset;
		int i;

		if (toburn) {
			if (sysinfo->image_offset == UPDATEFILE_MEDIA_OFFSET)
				image_offset = UPDATEFILE_MEDIA_B_OFFSET;
			else
				image_offset = UPDATEFILE_MEDIA_OFFSET;
		} else {
			image_offset = sysinfo->image_offset;
		}

#if DEVICE_TYPE_SELECT == EMMC_FLASH
		emmc_read(image_offset, 512, buf);
#else
		sfud_flash *sflash = sfud_get_device(0);
		sfud_read(sflash, image_offset, 512, buf);
#endif
		header = (UpFileHeader *)buf;
		if (header->magic != MKTAG('U', 'P', 'D', 'F')) {
			printf("update file isn't found, can't support module update.\n");
			return 0xffffffff;
		}
		for (i = 0; i < header->filenum; i++) {
			appfile = &header->files[i];
			if ((appfile->magic == UPFILE_APP_MAGIC && filetype == UPFILE_TYPE_APP) ||
				(appfile->magic == MKTAG('R', 'O', 'M', 'A') && filetype == UPFILE_TYPE_RESOURCE) ||
				(appfile->magic == MKTAG('B', 'A', 'N', 'I') && filetype == UPFILE_TYPE_ANIMATION)) {
				if (appfile->offset & (64 - 1)) {
					printf("offset isn't align to sector erase size, can't support module update.\n");
					return 0xffffffff;
				}
				return appfile->offset + image_offset;
			}
		}
	}

	return 0xffffffff;
}

void set_upfile_offset(SysInfo *sysinfo, int filetype, uint32_t offset)
{
	if (filetype == UPFILE_TYPE_WHOLE) {
		sysinfo->image_offset = offset;
	} else if (filetype == UPFILE_TYPE_FIRSTLDR) {
#if DEVICE_TYPE_SELECT == EMMC_FLASH
		sysinfo->loader_offset = offset;
#endif
	} else if (filetype == UPFILE_TYPE_STEPLDR) {
		sysinfo->stepldr_offset = offset;
	}
}

unsigned int get_upfile_size(int filetype)
{
	SysInfo *sysinfo = GetSysInfo();
	uint32_t offset;
	uint8_t buf[512];

	if (filetype <= UPFILE_TYPE_ANIMATION) {
		offset = get_upfile_offset(filetype, 0);
#if DEVICE_TYPE_SELECT == EMMC_FLASH
		emmc_read(offset, 512, buf);
#else
		sfud_flash *sflash = sfud_get_device(0);
		sfud_read(sflash, offset, 512, buf);
#endif

		if (filetype == UPFILE_TYPE_WHOLE) {
			UpFileHeader *header = (UpFileHeader *)buf;
			return header->size;
		} else if (filetype == UPFILE_TYPE_RESOURCE) {
			RomHeader *header = (RomHeader *)buf;
			return header->romsize;
		} else if (filetype == UPFILE_TYPE_ANIMATION) {
			BANIHEADER *header = (BANIHEADER *)buf;
			return header->aniSize;
		}
	} else if (filetype == UPFILE_TYPE_APP) {
		return sysinfo->app_size;
	} else if (filetype == UPFILE_TYPE_FIRSTLDR) {
		return sysinfo->loader_size;
	} else if (filetype == UPFILE_TYPE_STEPLDR) {
		return sysinfo->stepldr_size;
	}

	return 0;
}

/* 获取已升级文件校验和
   filesize   0:从已升级的文件中读取该文件大小
   toburn     0:获取当运行文件的校验和 1:获取要烧录位置的旧升级文件的校验和
   checkmode  0:不校验 1:进行校验，校验错误返回0 2:进行校验,校验出错也返回校验值 */
uint32_t get_upfile_checksum(int filetype, size_t filesize, int checkmode, int toburn)
{
	uint32_t checksum, calc_checksum = 0xffffffff;
	uint8_t *buf;
	sfud_flash *sflash = sfud_get_device(0);
	uint32_t fileoffset;
	int off, size, leftsize;

	buf = pvPortMalloc(IMAGE_RW_SIZE);
	if (!buf) {
		printf("%s %d malloc %d bytes fail.\n", __FUNCTION__, __LINE__, IMAGE_RW_SIZE);
		return 0;
	}

	if (!filesize) {
		filesize = get_upfile_size(filetype);
		if (!filesize) {
			if (!checkmode) {
				filesize = IMAGE_RW_SIZE;
			} else {
				printf("Error, filesize is zero when needing chekced.\n");
				return 0;
			}
		}
	}

	fileoffset = get_upfile_offset(filetype, toburn);
	size = filesize > IMAGE_RW_SIZE ? IMAGE_RW_SIZE : filesize;
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	if (filetype == UPFILE_TYPE_LNCHEMMC) {
		if (!sflash->init_ok)
			sfud_init();
		sfud_read(sflash, fileoffset, size, buf);
	} else emmc_read(fileoffset, size, buf);
#else
	sfud_read(sflash, fileoffset, size, buf);
#endif

	if (filetype == UPFILE_TYPE_WHOLE) {
		UpFileHeader *header = (UpFileHeader *)buf;
		checksum = header->checksum;
		header->checksum = 0;
		if (checkmode)
			calc_checksum = xcrc32(buf, size, calc_checksum);
	} else if (filetype == UPFILE_TYPE_RESOURCE) {
		RomHeader *header = (RomHeader *)buf;
		checksum = header->checksum;
		header->checksum = 0;
		if (checkmode)
			calc_checksum = xcrc32(buf, size, calc_checksum);
	} else if (filetype == UPFILE_TYPE_ANIMATION) {
		BANIHEADER *header = (BANIHEADER *)buf;
		checksum = header->checksum;
		header->checksum = 0;
		if (checkmode)
			calc_checksum = xcrc32(buf, size, calc_checksum);
	} else if (filetype >= UPFILE_TYPE_APP) {
		checksum = *(uint32_t*)(buf + APPLDR_CHECKSUM_OFFSET);
		*(uint32_t*)(buf + APPLDR_CHECKSUM_OFFSET) = 0;
		if (checkmode)
			calc_checksum = xcrc32(buf, size, calc_checksum);
	}

	if (!checkmode) {
		vPortFree(buf);
		return checksum;
	}

	off = fileoffset + IMAGE_RW_SIZE;
	leftsize = filesize - size;
	while (leftsize > 0) {
		size = leftsize > IMAGE_RW_SIZE ? IMAGE_RW_SIZE : leftsize;
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	if (filetype == UPFILE_TYPE_LNCHEMMC)
		sfud_read(sflash, off, size, buf);
	else
		emmc_read(off, size, buf);
#else
		sfud_read(sflash, off, size, buf);
#endif
		calc_checksum = xcrc32(buf, size, calc_checksum);
		off += size;
		leftsize -= size;
	}
	vPortFree(buf);
	if (calc_checksum == checksum || checkmode > 1)
		return calc_checksum;
	else
		return 0;
}

/* 获取升级文件的校验和，bchecked不为0时表示需要校验，校验错误返回0 */
static uint32_t get_mediafile_checksum(const char *ufile, int filetype, int bchecked)
{
	uint32_t checksum, calc_checksum = 0xffffffff;
	FF_FILE *fp;
	uint8_t *buf;
	int rlen;

	fp = ff_fopen(ufile, "rb");
	if (!fp) {
		printf("open %s fail.\n", ufile);
		return 0;
	}

	buf = pvPortMalloc(IMAGE_RW_SIZE);
	if (!buf) {
		printf("%s %d malloc %d bytes fail.\n", __FUNCTION__, __LINE__, IMAGE_RW_SIZE);
		ff_fclose(fp);
		return 0;
	}

	rlen = ff_fread(buf, 1, IMAGE_RW_SIZE, fp);
	if (rlen <= 0) {
		printf("read %s data fail.\n", ufile);
		ff_fclose(fp);
		vPortFree(buf);
		return 0;
	}

	if (filetype == UPFILE_TYPE_WHOLE) {
		UpFileHeader *header = (UpFileHeader *)buf;
		checksum = header->checksum;
		header->checksum = 0;
		if (bchecked)
			calc_checksum = xcrc32(buf, rlen, calc_checksum);
	} else if (filetype == UPFILE_TYPE_RESOURCE) {
		RomHeader *header = (RomHeader *)buf;
		checksum = header->checksum;
		header->checksum = 0;
		if (bchecked)
			calc_checksum = xcrc32(buf, rlen, calc_checksum);
	} else if (filetype == UPFILE_TYPE_ANIMATION) {
		BANIHEADER *header = (BANIHEADER *)buf;
		checksum = header->checksum;
		header->checksum = 0;
		if (bchecked)
			calc_checksum = xcrc32(buf, rlen, calc_checksum);
	} else if (filetype >= UPFILE_TYPE_APP) {
		checksum = *(uint32_t*)(buf + APPLDR_CHECKSUM_OFFSET);
		*(uint32_t*)(buf + APPLDR_CHECKSUM_OFFSET) = 0;
		if (bchecked)
			calc_checksum = xcrc32(buf, rlen, calc_checksum);
	}

	if (!bchecked) {
		ff_fclose(fp);
		vPortFree(buf);
		return checksum;
	}

	while ((rlen = ff_fread(buf, 1, IMAGE_RW_SIZE, fp)) > 0)
		calc_checksum = xcrc32(buf, rlen, calc_checksum);

	ff_fclose(fp);
	vPortFree(buf);

	if (calc_checksum == checksum)
		return checksum;
	else
		return 0;
}

int backup_whole_image(void)
{
	uint8_t *buf;
	SysInfo *sysinfo = GetSysInfo();
	uint32_t imagechecksum, imagesize, imageoff, roff, woff;
	UpFileHeader *header = NULL;
	int leftsize, rwsize;
	int retimes = 3;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	sfud_flash *sflash = sfud_get_device(0);
#endif

	buf = pvPortMalloc(IMAGE_RW_SIZE);
	if (!buf) {
		printf("%s %d malloc %d bytes fail.\n", __FUNCTION__, __LINE__, IMAGE_RW_SIZE);
		return -1;
	}

	if (sysinfo->image_offset == UPDATEFILE_MEDIA_OFFSET)
		imageoff = UPDATEFILE_MEDIA_B_OFFSET;
	else
		imageoff = UPDATEFILE_MEDIA_OFFSET;
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	emmc_read(imageoff, IMAGE_RW_SIZE, buf);
#else
	sfud_read(sflash, imageoff, IMAGE_RW_SIZE, buf);
#endif
	header = (UpFileHeader*)buf;
	if (header->checksum == sysinfo->app_checksum) {
		if (get_upfile_checksum(UPFILE_TYPE_WHOLE, header->size, 1, 1) == sysinfo->app_checksum) {
			printf("the whole images are same, no need to backup.\n");
			vPortFree(buf);
			return 0;
		}
	} else if (sysinfo->app_checksum == 0) {
		uint32_t checksum = get_upfile_checksum(UPFILE_TYPE_WHOLE, header->size, 2, 1);
		if (checksum && checksum == get_upfile_checksum(UPFILE_TYPE_WHOLE, header->size, 2, 0)) {
			printf("the whole images are same, no need to backup.\n");
			vPortFree(buf);
			return 0;
		}
	}

	printf("start backup the whole image...\n");
rewrite:
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	emmc_read(sysinfo->image_offset, IMAGE_RW_SIZE, buf);
	emmc_write(imageoff, IMAGE_RW_SIZE, buf);
#else
	sfud_read(sflash, sysinfo->image_offset, IMAGE_RW_SIZE, buf);
	sfud_erase_write(sflash, imageoff, IMAGE_RW_SIZE, buf);
#endif
	imagesize = header->size;
	leftsize = imagesize - IMAGE_RW_SIZE;
	woff = imageoff + IMAGE_RW_SIZE;
	roff = sysinfo->image_offset + IMAGE_RW_SIZE;
	/* 如果串口单独升级过update.bin内某一种文件的话可能导致header->checksum并不是真实的校验和 */
	header->checksum = 0;
	imagechecksum = xcrc32(buf, IMAGE_RW_SIZE, 0xffffffff);
	while (leftsize > 0) {
		rwsize = leftsize > IMAGE_RW_SIZE ? IMAGE_RW_SIZE : leftsize;
#if DEVICE_TYPE_SELECT == EMMC_FLASH
		emmc_read(roff, rwsize, buf);
		emmc_write(woff, rwsize, buf);
#else
		sfud_read(sflash, roff, rwsize, buf);
		sfud_erase_write(sflash, woff, rwsize, buf);
#endif
		leftsize -= rwsize;
		roff += rwsize;
		woff += rwsize;
		imagechecksum = xcrc32(buf, rwsize, imagechecksum);
	}

#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
	sfud_flush(sflash);
#endif

#if DEVICE_TYPE_SELECT == EMMC_FLASH
	emmc_read(sysinfo->image_offset, IMAGE_RW_SIZE, buf);
	header->checksum = imagechecksum;
	emmc_write(imageoff, IMAGE_RW_SIZE, buf);
#else
	sfud_read(sflash, sysinfo->image_offset, IMAGE_RW_SIZE, buf);
	header->checksum = imagechecksum;
	sfud_erase_write(sflash, imageoff, IMAGE_RW_SIZE, buf);
#endif

	printf("checksum after backup...\n");
	if (get_upfile_checksum(UPFILE_TYPE_WHOLE, imagesize, 1, 1) == imagechecksum) {
		printf("backup the whole image ok.\n");
		vPortFree(buf);
#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
		sfud_flush(sflash);
#endif
		return 0;
	} else if (retimes-- > 0) {
		printf("checksum fail, retry...\n");
		goto rewrite;
	} else {
		printf("backup the whole image fail.\n");
	}

	vPortFree(buf);
	return -1;
}

/* 获取app, resource, rom升级文件单独升级时能支持的最大文件大小 */
unsigned int get_subfile_maxsize(int filetype)
{
	uint8_t buf[512];
	UpFileHeader *header;
	UpFileInfo *appfile;
	SysInfo *sysinfo = GetSysInfo();
	int i;

	if (filetype < UPFILE_TYPE_RESOURCE || filetype > UPFILE_TYPE_APP)
		return 0;

#if DEVICE_TYPE_SELECT == EMMC_FLASH
	emmc_read(sysinfo->image_offset, 512, buf);
#else
	sfud_flash *sflash = sfud_get_device(0);
	sfud_read(sflash, sysinfo->image_offset, 512, buf);
#endif
	header = (UpFileHeader *)buf;
	if (header->magic != MKTAG('U', 'P', 'D', 'F'))
		return 0;
	for (i = 0; i < header->filenum; i++) {
		appfile = &header->files[i];
		if ((appfile->magic == UPFILE_APP_MAGIC && filetype == UPFILE_TYPE_APP) ||
			(appfile->magic == MKTAG('R', 'O', 'M', 'A') && filetype == UPFILE_TYPE_RESOURCE) ||
			(appfile->magic == MKTAG('B', 'A', 'N', 'I') && filetype == UPFILE_TYPE_ANIMATION)) {
			if (i < header->filenum - 1) {
				UpFileInfo *nextfile = &header->files[i + 1];
				return nextfile->offset - appfile->offset;
			} else {
				return UPDATEFILE_MAX_SIZE - appfile->offset;
			}
		}
	}

	return 0;
}

void update_part_upfile_info(SysInfo *sysinfo, int filetype, int filesize)
{
	uint8_t *buf;
	UpFileHeader *header;
	UpFileInfo *appfile;
	unsigned int image_offset;
	int i;

	configASSERT(filetype >= UPFILE_TYPE_RESOURCE && filetype <= UPFILE_TYPE_APP);

	if (sysinfo->image_offset == UPDATEFILE_MEDIA_OFFSET)
		image_offset = UPDATEFILE_MEDIA_B_OFFSET;
	else
		image_offset = UPDATEFILE_MEDIA_OFFSET;

#if DEVICE_TYPE_SELECT == EMMC_FLASH
	buf = pvPortMalloc(512);
	emmc_read(image_offset, 512, buf);
#else
	sfud_flash *sflash = sfud_get_device(0);
	buf = pvPortMalloc(sflash->chip.erase_gran);
	sfud_read(sflash, image_offset, sflash->chip.erase_gran, buf);
#endif
	header = (UpFileHeader *)buf;
	if (header->magic != MKTAG('U', 'P', 'D', 'F')) {
		printf("update file isn't found, can't support module update.\n");
		vPortFree(buf);
		return;
	}
	for (i = 0; i < header->filenum; i++) {
		appfile = &header->files[i];
		if ((appfile->magic == UPFILE_APP_MAGIC && filetype == UPFILE_TYPE_APP) ||
			(appfile->magic == MKTAG('R', 'O', 'M', 'A') && filetype == UPFILE_TYPE_RESOURCE) ||
			(appfile->magic == MKTAG('B', 'A', 'N', 'I') && filetype == UPFILE_TYPE_ANIMATION)) {
			if (i == header->filenum - 1) {
				if (filesize > header->files[i].size)
					header->size += filesize - header->files[i].size;
				else
					header->size -= header->files[i].size - filesize;
			}
			header->files[i].size = filesize;
			break;
		}
	}
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	emmc_write(image_offset, 512, buf);
#else
	sfud_erase_write(sflash, image_offset, sflash->chip.erase_gran, buf);
#endif
	header->checksum = get_upfile_checksum(UPFILE_TYPE_WHOLE, header->size, 2, 1);
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	emmc_write(image_offset, 512, buf);
#else
	sfud_erase_write(sflash, image_offset, sflash->chip.erase_gran, buf);
#endif
}


int update_from_media(char *mpath, int filetype)
{
	char update_file[32];
	FF_FILE *fp;
	size_t filesize;
	uint8_t *buf;
	size_t file_offset;
	SysInfo *sysinfo = GetSysInfo();
	sfud_flash *sflash = sfud_get_device(0);
	int leftsize, rwoffset, rwsize;
	int rlen;
	uint32_t checksum;
	SysInfo bak_sysinfo;

	memcpy(&bak_sysinfo, sysinfo, sizeof(SysInfo));

	strcpy(update_file, mpath);
	strcat(update_file, "/");
	strcat(update_file, g_upfilename[filetype]);

	printf("%s checksum...\n", update_file);
	if (!(checksum = get_mediafile_checksum(update_file, filetype, 1))) {
		printf("checksum fail, don't update.\n");
		return 0;
	}

	if (checksum == get_upfile_checksum(filetype, 0, 0, 0)) {
		if (!(filetype == UPFILE_TYPE_WHOLE && sysinfo->app_checksum == 0)) {
			printf("checksum is the same as now, don't update.\n");
			return 1;
		}
	}

	fp = ff_fopen(update_file, "rb");
	if (!fp) {
		printf("open %s fail.\n", update_file);
		return -1;
	}
	filesize = ff_filelength(fp);

	buf = pvPortMalloc(IMAGE_RW_SIZE);
	if (!buf) {
		printf("%s %d malloc %d bytes fail.\n", __FUNCTION__, __LINE__, IMAGE_RW_SIZE);
		ff_fclose(fp);
		return -1;
	}

	rlen = ff_fread(buf, 1, IMAGE_RW_SIZE, fp);
	if (rlen <= 0) {
		printf("read %s data fail.\n", update_file);
		goto end;
	}

	if (filetype == UPFILE_TYPE_WHOLE) {
		UpFileHeader *header = (UpFileHeader *)buf;
		bak_sysinfo.app_checksum = header->checksum;
		bak_sysinfo.app_size = header->files[0].size;
	} else if (filetype == UPFILE_TYPE_APP) {
		bak_sysinfo.app_size = filesize;
	} else if (filetype == UPFILE_TYPE_FIRSTLDR) {
		bak_sysinfo.loader_size = filesize;
	} else if (filetype == UPFILE_TYPE_STEPLDR) {
		bak_sysinfo.stepldr_size = filesize;
	} else if (filetype == UPFILE_TYPE_LNCHEMMC) {
		if (!sflash->init_ok)
			sfud_init();
	}

	/* 这三种文件没有备份，需要先备份整个update.bin镜像再升级备份区域 */
	if (filetype >= UPFILE_TYPE_RESOURCE && filetype <= UPFILE_TYPE_APP) {
		if (filesize > get_subfile_maxsize(filetype)) {
			printf("Not have enough space to update subfile %s.\n", update_file);
			goto end;
		}
		if (backup_whole_image()) {
			printf("backup_whole_image fail.\n");
			goto end;
		}
		if (sysinfo->image_offset == UPDATEFILE_MEDIA_OFFSET)
			bak_sysinfo.image_offset = UPDATEFILE_MEDIA_B_OFFSET;
		else
			bak_sysinfo.image_offset = UPDATEFILE_MEDIA_OFFSET;
	}

	file_offset = get_upfile_offset(filetype, 1);
	if (file_offset == 0xffffffff) {
		printf("get_upfile_offset fail.\n");
		goto end;
	}

	leftsize = filesize;
	rwoffset = file_offset;
	while (leftsize > 0) {
		if (hcn_get_usb_status() == USB_STATUS_REMOVED) {
			printf("USB removed, stop update.\n");
			goto end;
		}
		rwsize = leftsize > rlen ? rlen : leftsize;
#if DEVICE_TYPE_SELECT == EMMC_FLASH
		if (filetype == UPFILE_TYPE_LNCHEMMC) {
			if (sfud_erase_write(sflash, rwoffset, rwsize, buf) != SFUD_SUCCESS)
			{
				printf("burn %s data fail.\n", update_file);
				goto end;
			}
		} else {
			if (emmc_write(rwoffset, rwsize, buf))
#else
			if (sfud_erase_write(sflash, rwoffset, rwsize, buf) != SFUD_SUCCESS)
#endif
			{
				if (filetype == UPFILE_TYPE_WHOLE) {
					send_update_status(HCN_MSG_USB_UPDATE_STATUS, filesize, \
									rwoffset - file_offset, UPDATE_ERROR_FLASH);
				}
			
				printf("burn %s data fail.\n", update_file);
				goto end;
			} 
			else 
			{
				if (filetype == UPFILE_TYPE_WHOLE) {
					send_update_status(HCN_MSG_USB_UPDATE_STATUS, filesize, \
									rwoffset - file_offset, UPDATE_ERROR_NONE);
				}
			}
#if DEVICE_TYPE_SELECT == EMMC_FLASH
		}
#endif
		rwoffset += rwsize;
		leftsize -= rwsize;
		printf("burn %d/%d.\n", filesize - leftsize, filesize);

		rlen = ff_fread(buf, 1, IMAGE_RW_SIZE, fp);
		if (rlen < 0) {
			printf("read %s data fail.\n", update_file);
			goto end;
		}
	}

#if DEVICE_TYPE_SELECT == SPI_NAND_FLASH
	sfud_flush(sflash);
#endif

	printf("checksum after burn...\n");
	if (checksum == get_upfile_checksum(filetype, filesize, 1, 1)) {
		/* 升级这三个文件后需要更新UpFileHeader头信息以及app_checksum */
		if (filetype >= UPFILE_TYPE_RESOURCE && filetype <= UPFILE_TYPE_APP) {
			update_part_upfile_info(sysinfo, filetype, filesize);
			bak_sysinfo.app_checksum = get_upfile_checksum(UPFILE_TYPE_WHOLE, 0, 0, 1);
		} else {
			set_upfile_offset(&bak_sysinfo, filetype, file_offset);
		}
		memcpy(sysinfo, &bak_sysinfo, sizeof(SysInfo));
		SaveSysInfo();
		ff_fclose(fp);
		vPortFree(buf);
		printf("burn %s ok.\n", update_file);

		if (filetype == UPFILE_TYPE_WHOLE) {
			send_update_status(HCN_MSG_USB_UPDATE_STATUS, filesize, \
							rwoffset - file_offset, UPDATE_ERROR_NONE);
		}
		return 0; 
	} else {
		printf("checksum after burn fail.\n");
		if (filetype == UPFILE_TYPE_WHOLE) {
			send_update_status(HCN_MSG_USB_UPDATE_STATUS, filesize, \
							rwoffset - file_offset, UPDATE_ERROR_CRC);
		}
	}

end:
	ff_fclose(fp);
	vPortFree(buf);
	return -1;
}

int save_file_to_ota(int filetype)
{
	char update_file[32];
	char ota_file[32];
	FF_FILE *fp, *fota;
	size_t filesize;
	int leftsize;
	uint8_t *buf = NULL;
	size_t readlen;

	strcpy(update_file,"/usb/");
	strcat(update_file, g_upfilename[filetype]);
	strcpy(ota_file, OTA_MOUNT_PATH "/");
	strcat(ota_file, g_upfilename[filetype]);

	if (get_mediafile_checksum(update_file, filetype, 0) ==
		get_mediafile_checksum(ota_file, filetype, 0)) {
		if (get_mediafile_checksum(ota_file, filetype, 1)) {
			printf("%s is same with ota, not save.\n", g_upfilename[filetype]);
			return 0;
		}
	}

	fp = ff_fopen(update_file, "rb");
	if (!fp) {
		printf("open %s fail.\n", update_file);
		return -1;
	}
	filesize = ff_filelength(fp);
	fota = ff_fopen(ota_file, "wb");
	if (!fota) {
		printf("create %s in ota partition fail.\n", ota_file);
		ff_fclose(fp);
		return -1;
	}
	buf = pvPortMalloc(IMAGE_RW_SIZE);
	if (!buf) {
		printf("malloc IMAGE_RW_SIZE fail.\n");
		ff_fclose(fota);
		ff_fclose(fp);
		return -1;
	}
	leftsize = filesize;
	while (leftsize > 0) {
		if (readlen = ff_fread(buf, 1, IMAGE_RW_SIZE, fp)) {
			if (ff_fwrite(buf, 1, readlen, fota) != readlen) {
				printf("write ota data fail.\n");
				break;
			} else {
				printf("write ota data %d/%d.\n", filesize - leftsize + readlen, filesize);
			}
		} else {
			break;
		}
		leftsize -= readlen;
	}
	ff_fclose(fota);
	ff_fclose(fp);
	vPortFree(buf);
	if (leftsize == 0) {
		printf("save file ok.\n");
		return 0;
	}
	return -1;
}

#ifdef WIFI_UPDATE_SUPPORT
#define USB_DEV_PLUGED       0
#define USB_DEV_UNPLUGED     1
extern int usb_wait_stor_dev_pluged(uint32_t timeout);
extern void hub_usb_dev_reset(void);

/* 用从usb读取数据来模拟wifi接收升级数据，真实的wifi接收升级文件
   需要和端app适配，需要客户自己实现 */
static void wifi_update_demo_thread(void *para)
{
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	FF_Disk_t *sfdisk = FF_SFDiskInit(SF_MOUNT_PATH);
	if (sfdisk)
#endif
	{
		unsigned int status;
		int filetype;

		for (;;) {
			status = usb_wait_stor_dev_pluged(portMAX_DELAY);
			if (status == USB_DEV_PLUGED) {
				printf("usb dev inserted.\n");
				for (filetype = UPFILE_TYPE_WHOLE; filetype <= UPFILE_TYPE_LNCHEMMC; filetype++) {
					if (save_file_to_ota(filetype)) {
						printf("save_file_to_ota fail.\n");
					} else {
						printf("start to update from ota...\n");
						update_from_media(OTA_MOUNT_PATH, filetype);
					}
				}
			} else {
				printf("usb removed.\n");
			}
		}
	}

	for (;;)
		vTaskDelay(portMAX_DELAY);
}

void wifi_update_demo(void)
{
	if (xTaskCreate(wifi_update_demo_thread, "wifiupdate", configMINIMAL_STACK_SIZE * 16, NULL,
			1, NULL) != pdPASS) {
		printf("create wifi update demo task fail.\n");
	}
}

#endif

#endif
