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
#include "ff_usbdisk.h"
#include "usb_massstorage.h"
#include "ff_sys.h"


/* Misc definitions. */
#define usbSIGNATURE 			0x41404342UL
#define usbHUNDRED_64_BIT		( 100ull )
#define usbBYTES_PER_MB			( 1024ull * 1024ull )
#define usbSECTORS_PER_MB		( usbBYTES_PER_MB / 512ull )
#define usbIOMAN_MEM_SIZE		4096
#define usbAligned( pvAddress )	( ( ( ( size_t ) ( pvAddress ) ) & ( sizeof( size_t ) - 1 ) ) == 0 )

/*-----------------------------------------------------------*/

/*_RB_ Functions require comment blocks. */
static int32_t prvUSBDisk_Init( void );
static int32_t prvFFUSBDiskRead( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk );
static int32_t prvFFUSBDiskWrite( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk );


/*-----------------------------------------------------------*/

/*_RB_ Variables require a comment block where appropriate. */
static struct blk_desc *xUSBInfo;
static BaseType_t xUSBDiskStatus;
static SemaphoreHandle_t xPlusFATMutex;

/*-----------------------------------------------------------*/
static int32_t prvFFUSBDiskRead( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk )
{
	int32_t iReturn;

	/*_RB_ Many of the comments in this file apply to other functions in the file. */
	if( ( pxDisk != NULL ) && NULL != xUSBInfo->block_read &&
		( xUSBDiskStatus == pdPASS ) &&
		( pxDisk->ulSignature == usbSIGNATURE ) &&
		( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
		( ulSectorNumber < pxDisk->ulNumberOfSectors ) &&
		( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) >= ulSectorCount ) )
	{
		iReturn = xUSBInfo->block_read(xUSBInfo, ulSectorNumber, ulSectorCount, (void *)pucBuffer);

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

static int32_t prvFFUSBDiskWrite( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk )
{
	int32_t iReturn;

	if( ( pxDisk != NULL ) && NULL != xUSBInfo->block_write &&
		( xUSBDiskStatus == pdPASS ) &&
		( pxDisk->ulSignature == usbSIGNATURE ) &&
		( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
		( ulSectorNumber < pxDisk->ulNumberOfSectors ) &&
		( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) >= ulSectorCount ) )
	{
		iReturn = xUSBInfo->block_write(xUSBInfo, ulSectorNumber, ulSectorCount, (const void *)pucBuffer);

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

void FF_USBDiskFlush( FF_Disk_t *pxDisk )
{
	if( ( pxDisk != NULL ) &&
		( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
		( pxDisk->pxIOManager != NULL ) )
	{
		FF_FlushCache( pxDisk->pxIOManager );
	}
}
/*-----------------------------------------------------------*/

/* Initialise the USB driver and mount an Udisk */
FF_Disk_t *FF_USBDiskInit( const char *pcName )
{
	FF_Error_t xFFError;
	BaseType_t xPartitionNumber = 0;
	FF_CreationParameters_t xParameters;
	FF_Disk_t * pxDisk;

	xUSBDiskStatus = prvUSBDisk_Init();
	if( xUSBDiskStatus == pdPASS )
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
			pxDisk->ulNumberOfSectors = xUSBInfo->lba;
			pxDisk->ulSignature = usbSIGNATURE;

			if( xPlusFATMutex != NULL)
			{
				memset( &xParameters, '\0', sizeof( xParameters ) );
				xParameters.ulMemorySize = usbIOMAN_MEM_SIZE;
				xParameters.ulSectorSize = 512;
				xParameters.fnWriteBlocks = prvFFUSBDiskWrite;
				xParameters.fnReadBlocks = prvFFUSBDiskRead;
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
					FF_PRINTF( "FF_USBDiskInit: FF_CreateIOManger: %s\n", ( const char * ) FF_GetErrMessage( xFFError ) );
					FF_USBDiskDelete( pxDisk );
					pxDisk = NULL;
				}
				else
				{
					pxDisk->xStatus.bIsInitialised = pdTRUE;
					pxDisk->xStatus.bPartitionNumber = xPartitionNumber;
					if( FF_USBDiskMount( pxDisk ) == 0 )
					{
						FF_USBDiskDelete( pxDisk );
						pxDisk = NULL;
					}
					else
					{
						if( pcName == NULL )
						{
							pcName = "/";
						}
						FF_FS_Add( pcName, pxDisk );
						FF_PRINTF( "FF_USBDiskInit: Mounted Udisk as root \"%s\"\n", pcName );
						FF_USBDiskShowPartition( pxDisk );
					}
				}	/* if( pxDisk->pxIOManager != NULL ) */
			}	/* if( xPlusFATMutex != NULL) */
		}	/* if( pxDisk != NULL ) */
		else
		{
			FF_PRINTF( "FF_USBDiskInit: Malloc failed\n" );
		}
	}	/* if( xUSBDiskStatus == pdPASS ) */
	else
	{
		FF_PRINTF( "FF_USBDiskInit: prvUSBDisk_Init failed\n" );
		pxDisk = NULL;
	}

	return pxDisk;
}
/*-----------------------------------------------------------*/

BaseType_t FF_USBDiskFormat( FF_Disk_t *pxDisk, BaseType_t xPartitionNumber )
{
	FF_Error_t xError;
	BaseType_t xReturn = pdFAIL;

	xError = FF_Unmount( pxDisk );

	if( FF_isERR( xError ) != pdFALSE )
	{
		FF_PRINTF( "FF_USBDiskFormat: unmount fails: %08x\n", ( unsigned ) xError );
	}
	else
	{
		/* Format the drive - try FAT32 with large clusters. */
		xError = FF_Format( pxDisk, xPartitionNumber, pdFALSE, pdFALSE);

		if( FF_isERR( xError ) )
		{
			FF_PRINTF( "FF_USBDiskFormat: %s\n", (const char*)FF_GetErrMessage( xError ) );
		}
		else
		{
			FF_PRINTF( "FF_USBDiskFormat: OK, now remounting\n" );
			pxDisk->xStatus.bPartitionNumber = xPartitionNumber;
			xError = FF_USBDiskMount( pxDisk );
			FF_PRINTF( "FF_USBDiskFormat: rc %08x\n", ( unsigned )xError );
			if( FF_isERR( xError ) == pdFALSE )
			{
				xReturn = pdPASS;
			}
		}
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

/* Get a pointer to IOMAN, which can be used for all FreeRTOS+FAT functions */
BaseType_t FF_USBDiskMount( FF_Disk_t *pxDisk )
{
	FF_Error_t xFFError;
	BaseType_t xReturn;

	/* Mount the partition */
	xFFError = FF_Mount( pxDisk, pxDisk->xStatus.bPartitionNumber );

	if( FF_isERR( xFFError ) )
	{
		FF_PRINTF( "FF_USBDiskMount: %08lX\n", xFFError );
		xReturn = pdFAIL;
	}
	else
	{
		pxDisk->xStatus.bIsMounted = pdTRUE;
		FF_PRINTF( "****** FreeRTOS+FAT initialized %lu sectors\n", pxDisk->pxIOManager->xPartition.ulTotalSectors );
		FF_USBDiskShowPartition( pxDisk );
		xReturn = pdPASS;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

FF_IOManager_t *usbdisk_ioman( FF_Disk_t *pxDisk )
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
BaseType_t FF_USBDiskDelete( FF_Disk_t *pxDisk )
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

BaseType_t FF_USBDiskShowPartition( FF_Disk_t *pxDisk )
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
		iPercentageFree = ( int ) ( ( usbHUNDRED_64_BIT * ullFreeSectors + pxIOManager->xPartition.ulDataSectors / 2 ) /
			( ( uint64_t )pxIOManager->xPartition.ulDataSectors ) );

		ulTotalSizeMB = pxIOManager->xPartition.ulDataSectors / usbSECTORS_PER_MB;
		ulFreeSizeMB = ( uint32_t ) ( ullFreeSectors / usbSECTORS_PER_MB );

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

static int32_t prvUSBDisk_Init( void )
{
	xUSBInfo = &usb_dev_desc[0];//use the first device
	return pdPASS;
}
/*-----------------------------------------------------------*/

