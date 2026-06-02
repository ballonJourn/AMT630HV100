/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/

#include "diskio.h"

//#include <windows.h>

//#include "sdfmd.h"
//#include <ethdbg.h>
//#include <halether.h>

//extern DWORD SDInitialize(void);
//extern DWORD WriteSector(PBYTE pData, DWORD dwStartSector, DWORD dwNumber);
//extern DWORD ReadSector(PBYTE pData, DWORD dwStartSector, DWORD dwNumber);
//extern void Delay(UINT32 count);



extern void lcd_printstr(char* str);
#define printf lcd_printstr
/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */
/* Note that Tiny-FatFs supports only single drive and always            */
/* accesses drive number 0.                                              */

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{
	DSTATUS stat;
	int result;

	switch (drv) {
	/*case ATA :
		result = ATA_disk_initialize();
		// translate the reslut code here

		return stat;*/

	case SDMMC :
		result = MMC_disk_initialize();
		// translate the reslut code here
		if(result < 0)
			stat = STA_NOINIT;
		else
			stat = 0;
		return stat;
#if 0
	case USB :
		result = USB_disk_initialize();
		// translate the reslut code here
		if(result < 0)
			stat = STA_NOINIT;
		else
			stat = 0;
		
		return stat;
#endif
	}
	return STA_NOINIT;

}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{
	//drv = drv;
	return 0;
#if 0

	DSTATUS stat;
	int result;


	switch (drv) {
	case ATA :
		result = ATA_disk_status();
		// translate the reslut code here

		return stat;

	case SDMMC :
		result = MMC_disk_status();
		// translate the reslut code here

		return stat;
#if 0
	case USB :
		result = USB_disk_status();
		// translate the reslut code here

		return stat;
#endif
	}
	return STA_NOINIT;
#endif	
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
	DRESULT res;
	int result;

	switch (drv) {
	/*case ATA :
		result = ATA_disk_read(buff, sector, count);
		// translate the reslut code here

		return res;*/

	case SDMMC :
		result = MMC_disk_read(buff, sector, count);
		// translate the reslut code here
		if(result < 0)
			res = RES_ERROR;
		else
			res = RES_OK;
		return res;
#if 0
	case USB :
		result = USB_disk_read(buff, sector, count);
		// translate the reslut code here
		if(result < 0)
			res = RES_ERROR;
		else
			res = RES_OK;
		return res;
#endif
	}
	
	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
	DRESULT res;
	int result;

	switch (drv) {
	case ATA :
		result = ATA_disk_write(buff, sector, count);
		// translate the reslut code here

		return res;

	case SDMMC :
		result = MMC_disk_write(buff, sector, count);
		// translate the reslut code here

		return res;
#if 0
	case USB :
		result = USB_disk_write(buff, sector, count);
		// translate the reslut code here

		return res;
#endif
	}
	return RES_PARERR;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

	switch (drv) {
	/*case ATA :
		// pre-process here

		result = ATA_disk_ioctl(ctrl, buff);
		// post-process here

		return res;*/

	case SDMMC :
		// pre-process here

		result = MMC_disk_ioctl(ctrl, buff);
		// post-process here
		if(result < 0)
			res = RES_PARERR;
		else
			res = RES_OK;
		return res;

	/*case USB :
		// pre-process here

		result = USB_disk_ioctl(ctrl, buff);
		if(result < 0)
			res = RES_PARERR;
		else
			res = RES_OK;

		return res;*/
	}
	return RES_PARERR;
}

