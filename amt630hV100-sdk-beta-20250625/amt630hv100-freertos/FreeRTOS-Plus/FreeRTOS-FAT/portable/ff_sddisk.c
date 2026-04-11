/*
 * FreeRTOS+FAT build 191128 - Note:  FreeRTOS+FAT is still in the lab!
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Authors include James Walmsley, Hein Tibosch and Richard Barry
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* LPC18xx includes. */
#include "chip.h"
#include "board.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "portmacro.h"

/* FreeRTOS+FAT includes. */
#include "ff_sddisk.h"
#include "ff_sys.h"

#include "mmcsd_core.h"

/* Misc definitions. */
#define sdSIGNATURE 			0x41404342UL
#define sdHUNDRED_64_BIT		( 100ull )
#define sdBYTES_PER_MB			( 1024ull * 1024ull )
#define sdSECTORS_PER_MB		( sdBYTES_PER_MB / 512ull )
#define sdIOMAN_MEM_SIZE		4096
#define sdAligned( pvAddress )	( ( ( ( size_t ) ( pvAddress ) ) & ( sizeof( size_t ) - 1 ) ) == 0 )

/*-----------------------------------------------------------*/

/*_RB_ Functions require comment blocks. */
static int32_t prvSDMMC_Init( void );
static int32_t prvFFRead( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk );
static int32_t prvFFWrite( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk );


/*-----------------------------------------------------------*/

/*_RB_ Variables require a comment block where appropriate. */
static struct mmcsd_card *xCardInfo;
static BaseType_t xSDCardStatus;
static SemaphoreHandle_t xPlusFATMutex;

/*-----------------------------------------------------------*/
static int32_t prvFFRead( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk )
{
int32_t iReturn;

	/*_RB_ Many of the comments in this file apply to other functions in the file. */
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	ulSectorNumber += OTA_MEDIA_OFFSET / 512;
#endif

	if( ( pxDisk != NULL ) &&
		( xSDCardStatus == pdPASS ) &&
		( pxDisk->ulSignature == sdSIGNATURE ) &&
		( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
		( ulSectorNumber < pxDisk->ulNumberOfSectors ) &&
		( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) >= ulSectorCount ) )
	{
		iReturn = mmcsd_req_blk( xCardInfo, ulSectorNumber, pucBuffer, ulSectorCount, 0 );

		/*_RB_ I'm guessing 512 is a sector size, but that needs to be clear.
		Is it defined in a header somewhere?  If so we can do a search and
		replace in files on it as it seems to be used everywhere. */
		if( iReturn == 0 ) /*_RB_ Signed/unsigned mismatch (twice!) */
		{
			iReturn = FF_ERR_NONE;
		}
		else
		{
			/*_RB_ Signed number used to return bitmap (again below). */
			iReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_READ | FF_ERRFLAG );
		}
	}
	else
	{
		memset( ( void * ) pucBuffer, '\0', ulSectorCount * 512 );

		if( pxDisk->xStatus.bIsInitialised != 0 )
		{
			FF_PRINTF( "prvFFRead: warning: %lu + %lu > %lu\n", ulSectorNumber, ulSectorCount, pxDisk->ulNumberOfSectors );
		}

		iReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_READ | FF_ERRFLAG );
	}

	return iReturn;
}
/*-----------------------------------------------------------*/

static int32_t prvFFWrite( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk )
{
	int32_t iReturn;

#if DEVICE_TYPE_SELECT == EMMC_FLASH
	ulSectorNumber += OTA_MEDIA_OFFSET / 512;
#endif
	if( ( pxDisk != NULL ) &&
		( xSDCardStatus == pdPASS ) &&
		( pxDisk->ulSignature == sdSIGNATURE ) &&
		( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
		( ulSectorNumber < pxDisk->ulNumberOfSectors ) &&
		( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) >= ulSectorCount ) )
	{
		iReturn = mmcsd_req_blk( xCardInfo, ulSectorNumber, pucBuffer, ulSectorCount, 1 );

		if( iReturn == 0 ) /*_RB_ Signed/unsigned mismatch (twice!) */
		{
			iReturn = FF_ERR_NONE;
		}
		else
		{
			iReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG );
		}
	}
	else
	{
		memset( ( void * ) pucBuffer, '\0', ulSectorCount * 512 );
		if( pxDisk->xStatus.bIsInitialised )
		{
			FF_PRINTF( "prvFFWrite: warning: %lu + %lu > %lu\n", ulSectorNumber, ulSectorCount, pxDisk->ulNumberOfSectors );
		}

		iReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG );
	}

	return iReturn;
}
/*-----------------------------------------------------------*/

void FF_SDDiskFlush( FF_Disk_t *pxDisk )
{
	if( ( pxDisk != NULL ) &&
		( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
		( pxDisk->pxIOManager != NULL ) )
	{
		FF_FlushCache( pxDisk->pxIOManager );
	}
}
/*-----------------------------------------------------------*/

#if DEVICE_TYPE_SELECT == EMMC_FLASH
#define emmcSECTOR_SIZE            512
#define emmcHIDDEN_SECTOR_COUNT    8
#define emmcPRIMARY_PARTITIONS     1
#define emmcPARTITION_NUMBER       0   /* Only a single partition is used. */
static FF_Error_t PartitionAndFormatEmmcDisk( FF_Disk_t * pxDisk )
{
    FF_PartitionParameters_t xPartition;
    FF_Error_t xError;

    /* Create a single partition that fills all available space on the disk. */
    memset( &xPartition, '\0', sizeof( xPartition ) );
    xPartition.ulSectorCount = OTA_MEDIA_SIZE / emmcSECTOR_SIZE;
    xPartition.ulHiddenSectors = emmcHIDDEN_SECTOR_COUNT;
    xPartition.xPrimaryCount = emmcPRIMARY_PARTITIONS;
    xPartition.eSizeType = eSizeIsQuota;

    /* Partition the disk */
    xError = FF_Partition( pxDisk, &xPartition );
    FF_PRINTF( "FF_Partition: %s\n", ( const char * ) FF_GetErrMessage( xError ) );

    if( FF_isERR( xError ) == pdFALSE )
    {
        /* Format the partition. */
        xError = FF_Format( pxDisk, emmcPARTITION_NUMBER, pdFALSE, pdFALSE );
        FF_PRINTF( "FF_Format: %s\n", ( const char * ) FF_GetErrMessage( xError ) );
		if ( FF_isERR( xError ) == pdFALSE )
		{
			xError = FF_Mount( pxDisk, 0 );
			FF_PRINTF( "FF_Mount: %s\n", ( const char * ) FF_GetErrMessage( xError ) );
			FF_SDDiskShowPartition( pxDisk );
		}
    }

    return xError;
}
#endif

/* Initialise the SDIO driver and mount an SD card */
FF_Disk_t *FF_SDDiskInit( const char *pcName )
{
FF_Error_t xFFError;
BaseType_t xPartitionNumber = 0;
FF_CreationParameters_t xParameters;
FF_Disk_t * pxDisk;

	xSDCardStatus = prvSDMMC_Init();
	if( xSDCardStatus == pdPASS )
	{
		pxDisk = ( FF_Disk_t * ) pvPortMalloc( sizeof( *pxDisk ) );

		if( pxDisk != NULL )
		{

			/* Initialise the created disk structure. */
			memset( pxDisk, '\0', sizeof( *pxDisk ) );


			if( xPlusFATMutex == NULL)
			{
				xPlusFATMutex = xSemaphoreCreateRecursiveMutex();
			}
			pxDisk->ulNumberOfSectors = (xCardInfo->card_capacity / 512) << 10;
			pxDisk->ulSignature = sdSIGNATURE;

			if( xPlusFATMutex != NULL)
			{
				memset( &xParameters, '\0', sizeof( xParameters ) );
				xParameters.ulMemorySize = sdIOMAN_MEM_SIZE;
				xParameters.ulSectorSize = 512;
				xParameters.fnWriteBlocks = prvFFWrite;
				xParameters.fnReadBlocks = prvFFRead;
				xParameters.pxDisk = pxDisk;

				/* prvFFRead()/prvFFWrite() are not re-entrant and must be
				protected with the use of a semaphore. */
				xParameters.xBlockDeviceIsReentrant = pdFALSE;

				/* The semaphore will be used to protect critical sections in
				the +FAT driver, and also to avoid concurrent calls to
				prvFFRead()/prvFFWrite() from different tasks. */
				xParameters.pvSemaphore = ( void * ) xPlusFATMutex;

				pxDisk->pxIOManager = FF_CreateIOManger( &xParameters, &xFFError );

				if( pxDisk->pxIOManager == NULL )
				{
					FF_PRINTF( "FF_SDDiskInit: FF_CreateIOManger: %s\n", ( const char * ) FF_GetErrMessage( xFFError ) );
					FF_SDDiskDelete( pxDisk );
					pxDisk = NULL;
				}
				else
				{
					pxDisk->xStatus.bIsInitialised = pdTRUE;
					pxDisk->xStatus.bPartitionNumber = xPartitionNumber;
					if( FF_SDDiskMount( pxDisk ) == 0 )
					{
#if DEVICE_TYPE_SELECT == EMMC_FLASH
						if ( PartitionAndFormatEmmcDisk(pxDisk) != FF_ERR_NONE )
						{
							FF_PRINTF( "FF_SDDiskInit: Mounted SD-card/emmc fail.\n");
							FF_SDDiskDelete( pxDisk );
							pxDisk = NULL;
						}
						else
						{
							if( pcName == NULL )
							{
								pcName = "/";
							}
							FF_FS_Add( pcName, pxDisk );
							FF_PRINTF( "FF_SDDiskInit: Mount SD-card/emmc as root \"%s\"\n", pcName );
						}
#else
						FF_PRINTF( "FF_SDDiskInit: Mounted SD-card/emmc fail.\n");
						FF_SDDiskDelete( pxDisk );
						pxDisk = NULL;
#endif
					}
					else
					{
						if( pcName == NULL )
						{
							pcName = "/";
						}
						FF_FS_Add( pcName, pxDisk );
						FF_PRINTF( "FF_SDDiskInit: Mounted SD-card as root \"%s\"\n", pcName );
					}
				}	/* if( pxDisk->pxIOManager != NULL ) */
			}	/* if( xPlusFATMutex != NULL) */
		}	/* if( pxDisk != NULL ) */
		else
		{
			FF_PRINTF( "FF_SDDiskInit: Malloc failed\n" );
		}
	}	/* if( xSDCardStatus == pdPASS ) */
	else
	{
		FF_PRINTF( "FF_SDDiskInit: prvSDMMC_Init failed\n" );
		pxDisk = NULL;
	}

	return pxDisk;
}
/*-----------------------------------------------------------*/

BaseType_t FF_SDDiskFormat( FF_Disk_t *pxDisk, BaseType_t xPartitionNumber )
{
FF_Error_t xError;
BaseType_t xReturn = pdFAIL;

	xError = FF_Unmount( pxDisk );

	if( FF_isERR( xError ) != pdFALSE )
	{
		FF_PRINTF( "FF_SDDiskFormat: unmount fails: %08x\n", ( unsigned ) xError );
	}
	else
	{
		/* Format the drive - try FAT32 with large clusters. */
		xError = FF_Format( pxDisk, xPartitionNumber, pdFALSE, pdFALSE);

		if( FF_isERR( xError ) )
		{
			FF_PRINTF( "FF_SDDiskFormat: %s\n", (const char*)FF_GetErrMessage( xError ) );
		}
		else
		{
			FF_PRINTF( "FF_SDDiskFormat: OK, now remounting\n" );
			pxDisk->xStatus.bPartitionNumber = xPartitionNumber;
			xError = FF_SDDiskMount( pxDisk );
			FF_PRINTF( "FF_SDDiskFormat: rc %08x\n", ( unsigned )xError );
			if( FF_isERR( xError ) == pdFALSE )
			{
				xReturn = pdPASS;
			}
		}
	}

	return xReturn;
}
BaseType_t FF_SDDiskFormatRemount( FF_Disk_t *pxDisk, const char *pcName )
{
FF_Error_t xError;
BaseType_t xReturn = pdFAIL;

	xError = FF_Unmount( pxDisk );

	if( FF_isERR( xError ) != pdFALSE )
	{
		FF_PRINTF( "FF_SDDiskFormat: unmount fails: %08x\n", ( unsigned ) xError );
	}
	else
	{
#if DEVICE_TYPE_SELECT == EMMC_FLASH
		if ( PartitionAndFormatEmmcDisk(pxDisk) != FF_ERR_NONE )
		{
			FF_PRINTF( "FF_SDDiskFormatRemount: PartitionAndFormatEmmcDisk fail.\n");
			FF_SDDiskDelete( pxDisk );
			pxDisk = NULL;
		}
		else
		{
			if( pcName == NULL )
			{
				pcName = "/";
			}
			FF_FS_Add( pcName, pxDisk );
			FF_PRINTF( "FF_SDDiskFormatRemount: Mount SD-card/emmc as root \"%s\"\n", pcName );
		}
#else
						FF_PRINTF( "FF_SDDiskInit: Mounted SD-card/emmc fail.\n");
						FF_SDDiskDelete( pxDisk );
						pxDisk = NULL;
#endif
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

/* Get a pointer to IOMAN, which can be used for all FreeRTOS+FAT functions */
BaseType_t FF_SDDiskMount( FF_Disk_t *pxDisk )
{
FF_Error_t xFFError;
BaseType_t xReturn;

	/* Mount the partition */
	xFFError = FF_Mount( pxDisk, pxDisk->xStatus.bPartitionNumber );

	if( FF_isERR( xFFError ) )
	{
		FF_PRINTF( "FF_SDDiskMount: %08lX\n", xFFError );
		xReturn = pdFAIL;
	}
	else
	{
		pxDisk->xStatus.bIsMounted = pdTRUE;
		FF_PRINTF( "****** FreeRTOS+FAT initialized %lu sectors\n", pxDisk->pxIOManager->xPartition.ulTotalSectors );
		FF_SDDiskShowPartition( pxDisk );
		xReturn = pdPASS;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

FF_IOManager_t *sddisk_ioman( FF_Disk_t *pxDisk )
{
FF_IOManager_t *pxReturn;

	if( ( pxDisk != NULL ) && ( pxDisk->xStatus.bIsInitialised != pdFALSE ) )
	{
		pxReturn = pxDisk->pxIOManager;
	}
	else
	{
		pxReturn = NULL;
	}
	return pxReturn;
}
/*-----------------------------------------------------------*/

/* Release all resources */
BaseType_t FF_SDDiskDelete( FF_Disk_t *pxDisk )
{
	if( pxDisk != NULL )
	{
		pxDisk->ulSignature = 0;
		pxDisk->xStatus.bIsInitialised = 0;
		if( pxDisk->pxIOManager != NULL )
		{
			if( FF_Mounted( pxDisk->pxIOManager ) != pdFALSE )
			{
				FF_Unmount( pxDisk );
			}
			FF_DeleteIOManager( pxDisk->pxIOManager );
		}

		vPortFree( pxDisk );
	}
	return 1;
}
/*-----------------------------------------------------------*/

BaseType_t FF_SDDiskShowPartition( FF_Disk_t *pxDisk )
{
FF_Error_t xError;
uint64_t ullFreeSectors;
uint32_t ulTotalSizeMB, ulFreeSizeMB;
int iPercentageFree;
FF_IOManager_t *pxIOManager;
const char *pcTypeName = "unknown type";
BaseType_t xReturn = pdPASS;

	if( pxDisk == NULL )
	{
		xReturn = pdFAIL;
	}
	else
	{
		pxIOManager = pxDisk->pxIOManager;

		FF_PRINTF( "Reading FAT and calculating Free Space\n" );

		switch( pxIOManager->xPartition.ucType )
		{
			case FF_T_FAT12:
				pcTypeName = "FAT12";
				break;

			case FF_T_FAT16:
				pcTypeName = "FAT16";
				break;

			case FF_T_FAT32:
				pcTypeName = "FAT32";
				break;

			default:
				pcTypeName = "UNKOWN";
				break;
		}

		FF_GetFreeSize( pxIOManager, &xError );

		ullFreeSectors = pxIOManager->xPartition.ulFreeClusterCount * pxIOManager->xPartition.ulSectorsPerCluster;
		iPercentageFree = ( int ) ( ( sdHUNDRED_64_BIT * ullFreeSectors + pxIOManager->xPartition.ulDataSectors / 2 ) /
			( ( uint64_t )pxIOManager->xPartition.ulDataSectors ) );

		ulTotalSizeMB = pxIOManager->xPartition.ulDataSectors / sdSECTORS_PER_MB;
		ulFreeSizeMB = ( uint32_t ) ( ullFreeSectors / sdSECTORS_PER_MB );

		/* It is better not to use the 64-bit format such as %Lu because it
		might not be implemented. */
		FF_PRINTF( "Partition Nr   %8u\n", pxDisk->xStatus.bPartitionNumber );
		FF_PRINTF( "Type           %8u (%s)\n", pxIOManager->xPartition.ucType, pcTypeName );
		FF_PRINTF( "VolLabel       '%8s' \n", pxIOManager->xPartition.pcVolumeLabel );
		FF_PRINTF( "TotalSectors   %8lu\n", pxIOManager->xPartition.ulTotalSectors );
		FF_PRINTF( "SecsPerCluster %8lu\n", pxIOManager->xPartition.ulSectorsPerCluster );
		FF_PRINTF( "Size           %8lu MB\n", ulTotalSizeMB );
		FF_PRINTF( "FreeSize       %8lu MB ( %d perc free )\n", ulFreeSizeMB, iPercentageFree );
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

static int32_t prvSDMMC_Init( void )
{
	if (!xCardInfo) {
		xCardInfo = mmcsd_get_sdmmc_card_info();
		if (!xCardInfo)
			return pdFAIL;
	}

	mmcsd_set_blksize(xCardInfo);

	return pdPASS;
}
/*-----------------------------------------------------------*/

#if DEVICE_TYPE_SELECT == EMMC_FLASH
static uint32_t cached_sector = 0xffffffff;
static uint8_t cached_data[emmcSECTOR_SIZE];

int32_t phyRead( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount)
{
	int32_t iReturn;
	/*_RB_ Many of the comments in this file apply to other functions in the file. */
	iReturn = mmcsd_req_blk( xCardInfo, ulSectorNumber, pucBuffer, ulSectorCount, 0 );

	/*_RB_ I'm guessing 512 is a sector size, but that needs to be clear.
	Is it defined in a header somewhere?  If so we can do a search and
	replace in files on it as it seems to be used everywhere. */
	if( iReturn == 0 ) /*_RB_ Signed/unsigned mismatch (twice!) */
	{
		iReturn = FF_ERR_NONE;
	}
	else
	{
		/*_RB_ Signed number used to return bitmap (again below). */
		iReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_READ | FF_ERRFLAG );
	}
	return iReturn;
}
/*-----------------------------------------------------------*/

int32_t phyWrite( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount)
{
	int32_t iReturn;
	xSDCardStatus = prvSDMMC_Init();
	iReturn = mmcsd_req_blk( xCardInfo, ulSectorNumber, pucBuffer, ulSectorCount, 1 );
	if( iReturn == 0 ) /*_RB_ Signed/unsigned mismatch (twice!) */
	{
		iReturn = FF_ERR_NONE;
	}
	else
	{
		iReturn = ( FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG );
	}
	return iReturn;
}

#define EMMC_RW_MAX_SIZE	0x100000
static int raw_emmc_read(uint32_t offset, size_t size, uint8_t *data)
{
	unsigned int blkcnt = 0;
	unsigned int blkstart = 0;
	unsigned char *ptembuf = NULL;
	unsigned int inoffset = 0;
	unsigned int readsize = 0;
	int ret = 0;

	xSDCardStatus = prvSDMMC_Init();

    blkstart = offset / emmcSECTOR_SIZE;
	inoffset = offset - blkstart * emmcSECTOR_SIZE;
	readsize = size + inoffset;
	blkcnt = (readsize + emmcSECTOR_SIZE - 1) / emmcSECTOR_SIZE;

	ptembuf = (uint8_t *)pvPortMalloc(blkcnt * emmcSECTOR_SIZE);
	if(!ptembuf)
		printf("malloc Error!!1\n");

	ret = phyRead(ptembuf, blkstart, blkcnt);
	memcpy(data, ptembuf + inoffset, size);
	if(ptembuf) {
		free(ptembuf);
		ptembuf = NULL;
	}
	return ret;
}

static int raw_emmc_write(uint32_t offset, size_t size, uint8_t *data)
{
	unsigned int blkcnt = 0;
	unsigned int blkstart = 0, blkend = 0;
	unsigned char *ptembuf = NULL;
	unsigned int inoffset = 0;
	unsigned int endoffset = 0;
	unsigned int wsize = 0;
	int ret = 0;

	xSDCardStatus = prvSDMMC_Init();

    blkstart = offset / emmcSECTOR_SIZE;
	inoffset = offset % emmcSECTOR_SIZE;
	wsize = size + inoffset;
	blkcnt = (wsize + emmcSECTOR_SIZE - 1) / emmcSECTOR_SIZE;
	blkend = blkstart + blkcnt - 1;
	endoffset = wsize % emmcSECTOR_SIZE;

	ptembuf = (uint8_t *)pvPortMalloc(blkcnt * emmcSECTOR_SIZE);
	if(ptembuf) {
		if (inoffset) {
			if (blkstart != cached_sector) {
				phyRead(cached_data, blkstart, 1);
				cached_sector = blkstart;
			}
			memcpy(ptembuf, cached_data, inoffset);
		}

		if (data)
			memcpy(ptembuf + inoffset, data, size);
		else
			memset(ptembuf + inoffset, 0xff, size);

		if (cached_sector == blkstart) {
			if (size > emmcSECTOR_SIZE - inoffset)
				memcpy(cached_data + inoffset, ptembuf + inoffset, emmcSECTOR_SIZE - inoffset);
			else
				memcpy(cached_data + inoffset, ptembuf + inoffset, size);
		}

		if (endoffset) {
			if (blkend != cached_sector) {
				phyRead(cached_data, blkend, 1);
				cached_sector = blkend;
			}
			memcpy(ptembuf + wsize, cached_data + endoffset, emmcSECTOR_SIZE - endoffset);
			memcpy(cached_data, ptembuf + wsize - endoffset, endoffset);
		}

		ret = phyWrite(ptembuf,blkstart,blkcnt);
		if(ptembuf) {
			free(ptembuf);
			ptembuf = NULL;
		}
    } else
    	printf("emmc_write malloc Error!!1\n");
	return ret;
}

int emmc_read(uint32_t offset, size_t size, uint8_t *data)
{
	int32_t leftsize = size;
	int32_t off = offset;
	uint8_t *buf = data;
	uint32_t rsize;
	int ret;

	while (leftsize > 0) {
		rsize = leftsize > EMMC_RW_MAX_SIZE ? EMMC_RW_MAX_SIZE : leftsize;
		ret = raw_emmc_read(off, rsize, buf);
		if (ret)
			return ret;
		leftsize -= rsize;
		off += rsize;
		buf += rsize;
	}

	return 0;
}

int emmc_write(uint32_t offset, size_t size, uint8_t *data)
{
	int32_t leftsize = size;
	int32_t off = offset;
	uint8_t *buf = data;
	uint32_t wsize;
	int ret;

	while (leftsize > 0) {
		wsize = leftsize > EMMC_RW_MAX_SIZE ? EMMC_RW_MAX_SIZE : leftsize;
		ret = raw_emmc_write(off, wsize, buf);
		if (ret)
			return ret;
		leftsize -= wsize;
		off += wsize;
		if (buf)
			buf += wsize;
	}

	return 0;
}

#endif
