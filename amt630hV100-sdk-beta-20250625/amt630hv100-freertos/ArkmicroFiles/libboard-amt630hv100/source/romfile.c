#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "chip.h"
#include "board.h"
#include "sfud.h"
#include "updatefile.h"
#include "romfile.h"

#if DEVICE_TYPE_SELECT == EMMC_FLASH
#include "mmcsd_core.h"
#endif

static RomHeader *romheader = NULL;
static SemaphoreHandle_t romfileMutex;

#ifndef ROMFILE_USE_SMALL_MEM
#ifndef READ_ROMFILE_ONCE
static unsigned char *filestatus;
static uint32_t romoffset;
#endif
#else
static uint32_t romoffset;
#if ROMFILE_CACHE_DEF_SIZE
#define ROMFILE_CACHE_LIFE_LIMIT	1000000
static RomFileCache *romfilecache;
#endif
#endif

int ReadRomFile(void)
{
	uint32_t offset = GetUpFileOffset(MKTAG('R', 'O', 'M', 'A'));
	uint32_t size = GetUpFileSize(MKTAG('R', 'O', 'M', 'A'));

	romfileMutex = xSemaphoreCreateMutex();

	xSemaphoreTake(romfileMutex, portMAX_DELAY);

	if (size > 0) {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
		sfud_flash *sflash = sfud_get_device(0);
#endif
#ifndef ROMFILE_USE_SMALL_MEM
		void *rombuf = pvPortMalloc(size);
		if (rombuf) {
#if DEVICE_TYPE_SELECT != EMMC_FLASH			
	#ifdef READ_ROMFILE_ONCE
			sfud_read(sflash, offset, size, rombuf);
	#else
			sfud_read(sflash, offset, sizeof(RomHeader), rombuf);
			romoffset = offset;
	#endif
#else
	#ifdef READ_ROMFILE_ONCE
			emmc_read(offset, size, rombuf);
	#else
			emmc_read(offset, sizeof(RomHeader), rombuf);
			romoffset = offset;
	#endif

#endif
			romheader = rombuf;
			if (romheader->magic != MKTAG('R', 'O', 'M', 'A')) {
				printf("Read rom file error!\n");
				vPortFree(rombuf);
				xSemaphoreGive(romfileMutex);
				return -1;
			}
#ifdef READ_ROMFILE_ONCE
			CP15_clean_dcache_for_dma((uint32_t)rombuf, (uint32_t)rombuf + size);
#else
			int files = romheader->filenum;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
			sfud_read(sflash, offset + sizeof(RomHeader), sizeof(RomFileInfo) * files, 
				(unsigned char*)rombuf + sizeof(RomHeader));
#else
			emmc_read(offset + sizeof(RomHeader), sizeof(RomFileInfo) * files, 
				(unsigned char*)rombuf + sizeof(RomHeader));
#endif
			filestatus = pvPortMalloc(files * sizeof(*filestatus));
			if (!filestatus) {
				printf("filestatus malloc fail.\n");
				vPortFree(rombuf);
				xSemaphoreGive(romfileMutex);
				return -1;
			}
			memset(filestatus, 0, files * sizeof(*filestatus));
			CP15_clean_dcache_for_dma((uint32_t)rombuf, (uint32_t)rombuf + sizeof(RomHeader) + 
				sizeof(RomFileInfo) * files);
#endif
		} else {
			printf("Error! No enough memory for romfile.\n");
			xSemaphoreGive(romfileMutex);
			return -1;
		}
#else
		RomHeader header;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
		sfud_read(sflash, offset, sizeof(RomHeader), (void*)&header);
#else		
		emmc_read(offset, sizeof(RomHeader), (void*)&header);
#endif
		romoffset = offset;
		romheader = pvPortMalloc(sizeof(RomHeader) + sizeof(RomFileInfo) * header.filenum);
		if (romheader == NULL) {
			printf("Error! No enough memory for romheader.\n");
			xSemaphoreGive(romfileMutex);
			return -1;
		}
		memcpy(romheader, &header, sizeof(RomHeader));
#if DEVICE_TYPE_SELECT != EMMC_FLASH
		sfud_read(sflash, offset + sizeof(RomHeader), sizeof(RomFileInfo) * header.filenum,
			(unsigned char*)romheader + sizeof(RomHeader));
#else
		emmc_read(offset + sizeof(RomHeader), sizeof(RomFileInfo) * header.filenum,
			(unsigned char*)romheader + sizeof(RomHeader));
#endif
		CP15_clean_dcache_for_dma((uint32_t)romheader, (uint32_t)romheader + sizeof(RomHeader) +
			sizeof(RomFileInfo) * header.filenum);
#if ROMFILE_CACHE_DEF_SIZE
		romfilecache = pvPortMalloc(sizeof(RomFileCache) * ROMFILE_CACHE_DEF_SIZE);
		if (romfilecache == NULL) {
			printf("Error! No enough memory for romfile cache.\n");
			xSemaphoreGive(romfileMutex);
			return -1;
		}
		memset(romfilecache, 0, sizeof(RomFileCache) * ROMFILE_CACHE_DEF_SIZE);
#endif
#endif
	} else {
		printf("Warning! Not found romfile in update file.\n");
		xSemaphoreGive(romfileMutex);
		return -1;
	}

	xSemaphoreGive(romfileMutex);

	return 0;
}

RomFile *RomFileOpen(const char *name)
{
	int i;

	xSemaphoreTake(romfileMutex, portMAX_DELAY);

	for (i = 0; i < romheader->filenum; i++) {
		if (!strcmp(romheader->files[i].name, name)) {
			RomFile *file = pvPortMalloc(sizeof(RomFile));
			if (!file) {
				xSemaphoreGive(romfileMutex);
				return NULL;
			}
			memset(file, 0, sizeof(RomFile));
			file->index = i;
#ifndef ROMFILE_USE_SMALL_MEM
			file->buf = (void*)((uint32_t)romheader + romheader->files[i].offset);
#endif
			file->size = romheader->files[i].size;
			xSemaphoreGive(romfileMutex);
			return file;
		}
	}

	xSemaphoreGive(romfileMutex);

	return NULL;
}

size_t RomFileRead(RomFile *file, void *buf, size_t size)
{
#if ROMFILE_CACHE_DEF_SIZE
	int i;
#endif

	if (!file) return 0;

	if (size > file->size - file->pos)
		size = file->size - file->pos;

	if(size == 0)
		return 0;

	xSemaphoreTake(romfileMutex, portMAX_DELAY);

#ifndef ROMFILE_USE_SMALL_MEM
#ifndef READ_ROMFILE_ONCE
	if (!filestatus[file->index]) {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
		sfud_flash *sflash = sfud_get_device(0);
		sfud_read(sflash, romoffset + romheader->files[file->index].offset, file->size, file->buf);
#else
		emmc_read(romoffset + romheader->files[file->index].offset, file->size, file->buf);
#endif
		CP15_clean_dcache_for_dma((uint32_t)file->buf, (uint32_t)file->buf + file->size);
		filestatus[file->index] = 1;
	}
#endif
#else

#if ROMFILE_CACHE_DEF_SIZE
	if (!file->cache) {
		for (i = 0; i < ROMFILE_CACHE_DEF_SIZE; i++) {
			if(!strcmp(romfilecache[i].name, romheader->files[file->index].name)) {
				configASSERT(romfilecache[i].buf);
				file->buf = romfilecache[i].buf;
				file->cache = &romfilecache[i];
				romfilecache[i].cached_filenum++;
				romfilecache[i].life++;
				if (romfilecache[i].life > ROMFILE_CACHE_LIFE_LIMIT)
					romfilecache[i].life = ROMFILE_CACHE_LIFE_LIMIT;
			}
		}
	}

	for (i = 0; i < ROMFILE_CACHE_DEF_SIZE; i++) {
		if(romfilecache[i].life > INT32_MIN + 1)
			romfilecache[i].life -= 1;
	}
#endif

	if (!file->cache) {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
		sfud_flash *sflash = sfud_get_device(0);
#endif

#if ROMFILE_CACHE_DEF_SIZE
		RomFileCache *cachefile = NULL;
		uint32_t stime;
		int i;

		for (i = 0; i < ROMFILE_CACHE_DEF_SIZE; i++) {
			if (romfilecache[i].name[0] == 0) {
				cachefile = &romfilecache[i];
				break;
			}
		}
		if (!cachefile) {
			for (i = 0; i < ROMFILE_CACHE_DEF_SIZE; i++) {
				if (romfilecache[i].cached_filenum == 0) {
					cachefile = &romfilecache[i];
					break;
				}
			}
			if (cachefile) {
				for(i = i + 1; i < ROMFILE_CACHE_DEF_SIZE; i++) {
					if (romfilecache[i].cached_filenum == 0 &&
						romfilecache[i].life < cachefile->life)
						cachefile = &romfilecache[i];
				}
			}
		}
		if (cachefile) {
			int filelife;

			file->buf = pvPortMalloc(romheader->files[file->index].size);
			if (!file->buf) {
				printf("ERROR! No enough memory for opening romfile.\n");
				vPortFree(file);
				xSemaphoreGive(romfileMutex);
				return 0;
			}
			stime = get_timer(0);
#if DEVICE_TYPE_SELECT != EMMC_FLASH
			sfud_read(sflash, romoffset + romheader->files[file->index].offset, file->size, file->buf);
#else
			emmc_read(romoffset + romheader->files[file->index].offset, file->size, file->buf);
#endif
			CP15_clean_dcache_for_dma((uint32_t)file->buf, (uint32_t)file->buf + file->size);
			filelife = get_timer(stime);
			if (cachefile->buf)
				vPortFree(cachefile->buf);
			strncpy(cachefile->name, romheader->files[file->index].name, ROMFILE_NAME_MAX_LEN);
			cachefile->buf = file->buf;
			cachefile->cached_filenum = 1;
			cachefile->life = filelife;
			if (cachefile->life > ROMFILE_CACHE_LIFE_LIMIT)
				cachefile->life = ROMFILE_CACHE_LIFE_LIMIT;
			file->cache = cachefile;
		} else
#endif
		{
			file->buf = pvPortMalloc(size);
			if (!file->buf) {
				printf("ERROR! No enough memory for opening romfile.\n");
				vPortFree(file);
				xSemaphoreGive(romfileMutex);
				return 0;
			}
#if DEVICE_TYPE_SELECT != EMMC_FLASH
			sfud_read(sflash, romoffset + romheader->files[file->index].offset + file->pos,
				size, file->buf);
#else
			emmc_read(romoffset + romheader->files[file->index].offset + file->pos,
				size, file->buf);
#endif
			CP15_clean_dcache_for_dma((uint32_t)file->buf, (uint32_t)file->buf + size);
		}
	}
#endif
	if (buf) {
#ifdef ROMFILE_USE_SMALL_MEM
		if (!file->cache) {
			memcpy(buf, (char *)file->buf, size);
			vPortFree(file->buf);
			file->buf = NULL;
		} else {
#endif
			memcpy(buf, (char *)file->buf + file->pos, size);
#ifdef ROMFILE_USE_SMALL_MEM
		}
#endif
	}

	file->pos += size;

	xSemaphoreGive(romfileMutex);

	return size;
}

int RomFileSeek(RomFile *file, int offset, int whence)
{
	if (!file) return -1;

	xSemaphoreTake(romfileMutex, portMAX_DELAY);
	
	if (whence == SEEK_SET)
		file->pos = offset;
	else if (whence == SEEK_CUR)
		file->pos += offset;
	else if (whence == SEEK_END)
		file->pos = file->size - 1 + offset;

	if (file->pos >= file->size)
		file->pos = file->size - 1;
	else if (file->pos < 0)
		file->pos = 0;

	xSemaphoreGive(romfileMutex);

	return file->pos;
}

void RomFileClose(RomFile *file)
{
	xSemaphoreTake(romfileMutex, portMAX_DELAY);

	if (file) {
#ifdef ROMFILE_USE_SMALL_MEM
#if ROMFILE_CACHE_DEF_SIZE
		if (file->cache) {
			file->cache->cached_filenum--;
		} else {
#endif
			if (file->buf)
				vPortFree(file->buf);
#if ROMFILE_CACHE_DEF_SIZE
		}
#endif
#endif
		vPortFree(file);
	}

	xSemaphoreGive(romfileMutex);
}

/**
 * Return with the extension of the filename
 * @param fn string with a filename
 * @return pointer to the beginning extension or empty string if no extension
 */
const char * RomFileGetExt(const char * path)
{
    size_t i;
    for(i = strlen(path); i > 0; i--) {
        if(path[i] == '.') {
            return &path[i + 1];
        }
        else if(path[i] == '/' || path[i] == '\\') {
            return ""; /*No extension if a '\' or '/' found*/
        }
    }

    return ""; /*Empty string if no '.' in the file name. */
}

int RomFileTell(RomFile *file)
{
	return file->pos;
}


int RomFileGetSize(RomFile *file)
{
	return file->size;
}

int RomFileExist(const char *name)
{
	int i;

	for (i = 0; i < romheader->filenum; i++) {
		if (!strcmp(romheader->files[i].name, name)) {
			return 1;
		}
	}

	return 0;
}

int RomFileDirExist(const char *name)
{
	int i;
	int len = strlen(name);

	for (i = 0; i < romheader->filenum; i++) {
		if (!strncmp(romheader->files[i].name, name, len) &&
			romheader->files[i].name[len] == '/') {
			return 1;
		}
	}

	return 0;
}
