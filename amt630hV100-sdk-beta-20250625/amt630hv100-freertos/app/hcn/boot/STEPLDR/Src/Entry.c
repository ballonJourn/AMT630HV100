/*
**********************************************************************
Copyright (c)2021 Arkmicro Technologies Inc.  All Rights Reserved
Filename: Entry.c
Version : 1.00
Date    : 2021.08.20
Author  : Sim
***********************************************************************
*/

#include "hcn_config.h"
#include "typedef.h"
#include "amt630h.h"
#include "uart.h"
#include "timer.h"
#include "aic.h"
#include "mmu.h"
#include "lcd.h"
#include "spi.h"
#include "sysinfo.h"
#include "sysctl.h"
#include "board.h"
#include "fs/ff.h"
#include "fs/diskio.h"
#include "crc32.h"
#include "sdmmc.h"
#include "gpio.h"
#include "pwm.h"

#include <intrinsics.h>

extern void bootFromSPI(void);
extern void updateFromSD(int chipid);
extern void updateFromUSB(void);
int wdt_init(void);
void wdt_stop(void);
void wdt_start(void);

static SysInfo *pSysInfo = NULL;

void UpdateFromMedia(int drv)
{
	FRESULT  fret;
	FIL fp = {0};
	UINT32 size;
	unsigned int checksum, calc_checksum;
	int timeout = 0;
	int update_ok = 0;
	SysInfo *sysinfo = GetSysInfo();
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	char launchemmcfile[32];
#endif
	char loaderfile[32];
	char stepldrfile[32];
	char appfile[32];
	FILINFO fileinfo = {0};
	int leftsize;

	if (drv == SDMMC) {
		strcpy(loaderfile, LOADER_FILE_NAME);
		strcpy(stepldrfile, STEPLDR_FILE_NAME);
		strcpy(appfile, APP_FILE_NAME);
	} else if (drv == USB) {
#if DEVICE_TYPE_SELECT == EMMC_FLASH
		strcpy(launchemmcfile, "1:/");
		strcat(launchemmcfile, LAUNCHEMMC_FILE_NAME);
#endif
		strcpy(loaderfile, "1:/");
		strcat(loaderfile, LOADER_FILE_NAME);
		strcpy(stepldrfile, "1:/");
		strcat(stepldrfile, STEPLDR_FILE_NAME);
		strcpy(appfile, "1:/");
		strcat(appfile, APP_FILE_NAME);
	} else {
		SendUartString("Unknown disk drv\r\n");
		return;
	}

	fret = f_mount(drv, &g_fs);
	if(fret == FR_OK)
		SendUartString("Mount file ok\r\n");
	else
	{
		SendUartString("Mount file fail\r\n");
		return;
	}

#if DEVICE_TYPE_SELECT == EMMC_FLASH
	SendUartString("burn emmc launcher start... \r\n");
readlaunchemmc:
		fret = f_open(&fp, launchemmcfile, FA_OPEN_EXISTING | FA_READ);
		if(fret != FR_OK) {
			SendUartString("Open file fail, don't update.\r\n");
		} else {
			fret = f_read(&fp, (void *)IMAGE_ENTRY, LOADER_MAX_SIZE, &size);
			f_close(&fp);
			if(fret != FR_OK) {
				if (timeout++ < 3) {
					SendUartString("Read file fail, read again.\r\n");
					goto readlaunchemmc;
				}
			} else {
				checksum = *(unsigned int*)(IMAGE_ENTRY + APPLDR_CHECKSUM_OFFSET);
				*(unsigned int*)(IMAGE_ENTRY + APPLDR_CHECKSUM_OFFSET) = 0;
				calc_checksum = xcrc32((unsigned char*)IMAGE_ENTRY, size, 0xffffffff);
				*(unsigned int*)(IMAGE_ENTRY + APPLDR_CHECKSUM_OFFSET) = checksum;
				if (calc_checksum != checksum) {
					if (timeout++ < 3) {
						SendUartString("read step emmc checksum fail, read again.\n");
						goto readlaunchemmc;
					}
					else {
						SendUartString("read step emmc fail, don't update.\n");
					}
				} else {
					if (FlashBurn((void*)IMAGE_ENTRY, LOADER_OFFSET, size, 0)) {
						SendUartString("burn step emmc fail.\n");
						goto end;
					}
				}
			}
		}
	SendUartString("burn emmc launcher end. \r\n");
#endif

	SendUartString("burn loader start... \r\n");
readloader:
	fret = f_open(&fp, loaderfile, FA_OPEN_EXISTING | FA_READ);
	if(fret != FR_OK) {
		SendUartString("Open file fail, don't update.\r\n");
	} else {
		fret = f_read(&fp, (void *)IMAGE_ENTRY, LOADER_MAX_SIZE, &size);
		f_close(&fp);
		if(fret != FR_OK) {
			if (timeout++ < 3) {
				SendUartString("Read file fail, read again.\r\n");
				goto readloader;
			}
		} else {
			checksum = *(unsigned int*)(IMAGE_ENTRY + APPLDR_CHECKSUM_OFFSET);
			*(unsigned int*)(IMAGE_ENTRY + APPLDR_CHECKSUM_OFFSET) = 0;
			calc_checksum = xcrc32((unsigned char*)IMAGE_ENTRY, size, 0xffffffff);
			*(unsigned int*)(IMAGE_ENTRY + APPLDR_CHECKSUM_OFFSET) = checksum;
			if (calc_checksum != checksum) {
				if (timeout++ < 3) {
					SendUartString("read loader checksum fail, read again.\n");
					goto readloader;
				}
				else {
					SendUartString("read loader fail, don't update.\n");
				}
			} else {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
				if (FlashBurn((void*)IMAGE_ENTRY, LOADER_OFFSET, size, 0)) {
					SendUartString("burn loader fail.\n");
					goto end;
				}
#else
				if (sysinfo->loader_offset == LOADER_OFFSET)
					sysinfo->loader_offset = LOADERB_OFFSET;
				else
					sysinfo->loader_offset = LOADER_OFFSET;
				sysinfo->loader_size = size;
				if (EmmcBurn((void*)IMAGE_ENTRY, sysinfo->loader_offset, size, 0)) {
					SendUartString("burn emmc loader fail.\n");
					goto end;
				}

#endif
			}
		}
	}
	SendUartString("burn loader end. \r\n");

	timeout = 0;
	SendUartString("burn stepldr start... \r\n");
readstepldr:
	fret = f_open(&fp, stepldrfile, FA_OPEN_EXISTING | FA_READ);
	if(fret != FR_OK) {
		SendUartString("Open file fail, don't update.\r\n");
	} else {
		fret = f_read(&fp, (void *)IMAGE_ENTRY, STEPLDR_MAX_SIZE, &size);
		f_close(&fp);
		if(fret != FR_OK) {
			if (timeout++ < 3) {
				SendUartString("Read file fail, read again.\r\n");
				goto readstepldr;
			} else {
				SendUartString("read stepldr fail, don't update.\n");
			}
		} else {
			checksum = *(unsigned int*)(IMAGE_ENTRY + APPLDR_CHECKSUM_OFFSET);
			*(unsigned int*)(IMAGE_ENTRY + APPLDR_CHECKSUM_OFFSET) = 0;
			calc_checksum = xcrc32((unsigned char*)IMAGE_ENTRY, size, 0xffffffff);
			*(unsigned int*)(IMAGE_ENTRY + APPLDR_CHECKSUM_OFFSET) = checksum;
			if (calc_checksum != checksum) {
				if (timeout++ < 3) {
					SendUartString("read stepldr checksum fail, read again.\n");
					goto readstepldr;
				}
				else {
					SendUartString("read stepldr fail, don't update.\n");
				}
			} else {
				if (sysinfo->stepldr_offset == STEPLDRA_OFFSET)
					sysinfo->stepldr_offset = STEPLDRB_OFFSET;
				else
					sysinfo->stepldr_offset = STEPLDRA_OFFSET;
				sysinfo->stepldr_size = size;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
				if (FlashBurn((void*)IMAGE_ENTRY, sysinfo->stepldr_offset, size, 0)) {
					SendUartString("burn stepldr fail.\n");
					goto end;
				}
#else
				if (EmmcBurn((void*)IMAGE_ENTRY, sysinfo->stepldr_offset, size, 0)) {
					SendUartString("burn emmc stepldr fail.\n");
					goto end;
				}

#endif
			}
		}
	}
	SendUartString("burn stepldr end. \r\n");

	timeout = 0;
	SendUartString("burn app start... \r\n");
	update_logo_init();
readapp:
	fret = f_stat(appfile, &fileinfo);
	if (fret != FR_OK) {
		SendUartString("Get file info fail, don't update.\r\n");
		goto end;
	}
	leftsize = fileinfo.fsize;
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	if (sysinfo->image_offset == IMAGE_OFFSET)
		sysinfo->image_offset = IMAGEB_OFFSET;
	else
		sysinfo->image_offset = IMAGE_OFFSET;
	if(leftsize)
	{
		unsigned int start = sysinfo->image_offset;
		unsigned int blkcnt;
		unsigned int blkstart;
		if(start % 512)
			blkstart = start / 512 + 1;
		else
			blkstart = start / 512;

		if(fileinfo.fsize % 512)
			blkcnt = fileinfo.fsize / 512 + 1;
		else
			blkcnt = fileinfo.fsize / 512;

		mmc_berase(blkstart, blkcnt);
	}
#endif

	fret = f_open(&fp, appfile, FA_OPEN_EXISTING | FA_READ);
	if(fret != FR_OK) {
		SendUartString("Open file fail, don't update.\r\n");
	} else {
		UpFileHeader *header = (UpFileHeader *)IMAGE_ENTRY;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
		UINT32 woff = IMAGE_OFFSET;
#else
		UINT32 woff = sysinfo->image_offset;
#endif
		UINT32 app_size;

		while (leftsize > 0) {
			fret = f_read(&fp, (void *)IMAGE_ENTRY, IMAGE_READ_SIZE, &size);
			if(fret != FR_OK) {
				SendUartString("Read file fail\r\n");
				if (timeout++ < 3) {
					SendUartString("Read file fail, read again.\r\n");
					f_close(&fp);
					goto readapp;
				} else {
					SendUartString("read update file fail, don't update.\n");
				}
			} else {
				if (leftsize == fileinfo.fsize) {
					calc_checksum = 0xffffffff;
					checksum = header->checksum;
					header->checksum = 0;
					app_size = header->files[0].size;
				}
				calc_checksum = xcrc32((unsigned char*)IMAGE_ENTRY, size, calc_checksum);
                if (leftsize == fileinfo.fsize)
                    header->checksum = checksum;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
				if(FlashBurn((void*)IMAGE_ENTRY, woff, size, 0)) {
					SendUartString("burn app fail.\n");
					f_close(&fp);
					goto end;
				}
#else
				if (mmc_write((void*)IMAGE_ENTRY, woff, size, 0)) {
					SendUartString("burn app fail.\n");
					f_close(&fp);
					goto end;
				}
#endif
				update_progress_set(100 - leftsize * 100 / fileinfo.fsize);
			}
			woff += size;
			leftsize -= size;
		}
		f_close(&fp);
		update_progress_set(100);
		if (calc_checksum != checksum) {
			SendUartString("app checksum fail, update again.\n");
		} else {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
			//sysinfo->image_offset = IMAGE_OFFSET;
#endif
			sysinfo->app_checksum = header->checksum = checksum;
			sysinfo->app_size = app_size;
			update_ok = 1;
		}
	}
	SendUartString("burn app end.\r\n");

	if (update_ok) {
		SendUartString("update ok, save sysinfo.\r\n");
		sysinfo->update_status = UPDATE_STATUS_END;
		SaveSysInfo(sysinfo);
	}
end:
	SendUartString("Update is finished.\r\n");
}


extern void lvds_avdd_init(void);
void main(void)
{
	wdt_init();
	timer_init();
	AIC_Initialize();
	InitUart(115200);
	SendUartString("ARK AMT630HV100 STEPLDR V 1.0\r\n");
#ifdef MMU_ENABLE
	MMU_Init();
#endif

	if(SpiInit()) {
		SendUartString("SpiInit failed.\n");
		while(1);
	}
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	Emmcinit(0);
#endif

	if (ReadSysInfo()) {
		SendUartString("read sysinfo fail, use default.\r\n");
		SetDefaultSysInfo();
	}

	pSysInfo = GetSysInfo();
	//pSysInfo->update_status = UPDATE_STATUS_START;
	//pSysInfo->update_media_type = UPDATE_MEDIA_USB;
	if (pSysInfo->update_status == UPDATE_STATUS_START) {
			///< 屏幕供电
		gpio_direction_output(HCN_LCD_DISPLAY_EN_GPIO, 1);
        lvds_avdd_init();
		lcd_init();
		wdt_stop();

		switch (pSysInfo->update_media_type) {
		case UPDATE_MEDIA_SD:
			updateFromSD(0);
			break;
#ifdef USB_SUPPORT
		case UPDATE_MEDIA_USB:
			__enable_irq();
			extern int ark_usb_init(void);
			if (!ark_usb_init())
				UpdateFromMedia(USB);
			break;
#endif
		case UPDATE_MEDIA_UART:
			__enable_irq();
			updateFromUart(UART_MCU_PORT);
			break;
		}
		wdt_start();
	}

	__disable_irq();
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	 bootFromSPI();
#else
     bootFromEmmc(0);
#endif
}
