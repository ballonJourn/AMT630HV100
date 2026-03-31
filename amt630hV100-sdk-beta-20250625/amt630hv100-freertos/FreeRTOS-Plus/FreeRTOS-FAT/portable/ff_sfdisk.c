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
#include "ff_sfdisk.h"
#include "ff_sys.h"

#include "sfud.h"

/* Misc definitions. */
#define sfSIGNATURE 			0x41404342UL
#define sfHUNDRED_64_BIT		( 100ull )
#define sfBYTES_PER_MB			( 1024ull * 1024ull )
#define sfSECTOR_SIZE			( 4096ull )
#define sfSECTORS_PER_MB		( sfBYTES_PER_MB / sfSECTOR_SIZE )
#define sfIOMAN_MEM_SIZE		16384
#define sfDISK_MEDIA_OFFSET		OTA_MEDIA_OFFSET
#define sfAligned( pvAddress )	( ( ( ( size_t ) ( pvAddress ) ) & ( sizeof( size_t ) - 1 ) ) == 0 )

/*-----------------------------------------------------------*/

/*_RB_ Functions require comment blocks. */
static int32_t prvFFRead( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk );
static int32_t prvFFWrite( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk );


/*-----------------------------------------------------------*/

/*_RB_ Variables require a comment block where appropriate. */
static sfud_flash *sflash;
static SemaphoreHandle_t xPlusFATMutex;

/*-----------------------------------------------------------*/
static int32_t prvFFRead( uint8_t *pucBuffer, uint32_t ulSectorNumber, uint32_t ulSectorCount, FF_Disk_t *pxDisk )
{
	int32_t iReturn;

	/*_RB_ Many of the comments in this file apply to other functions in the file. */

	if( ( pxDisk != NULL ) &&
		( sflash != NULL ) &&
		( pxDisk->ulSignature == sfSIGNATURE ) &&
		( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
		( ulSectorNumber < pxDisk->ulNumberOfSectors ) &&
		( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) >= ulSectorCount ) )
	{
		iReturn = sfud_read(sflash, sfDISK_MEDIA_OFFSET + ulSectorNumber * sfSECTOR_SIZE,
			ulSectorCount * sfSECTOR_SIZE, pucBuffer);

		/*_RB_ I'm guessing 4096 is a sector size, but that needs to be clear.
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

	if( ( pxDisk != NULL ) &&
		( sflash != NULL ) &&
		( pxDisk->ulSignature == sfSIGNATURE ) &&
		( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
		( ulSectorNumber < pxDisk->ulNumberOfSectors ) &&
		( ( pxDisk->ulNumberOfSectors - ulSectorNumber ) >= ulSectorCount ) )
	{
		iReturn = sfud_erase_write(sflash, sfDISK_MEDIA_OFFSET + ulSectorNumber * sfSECTOR_SIZE,
			ulSectorCount * sfSECTOR_SIZE, pucBuffer);

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

void FF_SFDiskFlush( FF_Disk_t *pxDisk )
{
	if( ( pxDisk != NULL ) &&
		( pxDisk->xStatus.bIsInitialised != pdFALSE ) &&
		( pxDisk->pxIOManager != NULL ) )
	{
		FF_FlushCache( pxDisk->pxIOManager );
	}
}
/*-----------------------------------------------------------*/
	 
#define sfHIDDEN_SECTOR_COUNT    8
#define sfPRIMARY_PARTITIONS     1
#define sfPARTITION_NUMBER       0   /* Only a single partition is used. */
static FF_Error_t PartitionAndFormatSFDisk( FF_Disk_t * pxDisk )
{
    FF_PartitionParameters_t xPartition;
    FF_Error_t xError;

    /* Create a single partition that fills all available space on the disk. */
    memset( &xPartition, '\0', sizeof( xPartition ) );
    xPartition.ulSectorCount = OTA_MEDIA_SIZE / sfSECTOR_SIZE;
    xPartition.ulHiddenSectors = sfHIDDEN_SECTOR_COUNT;
    xPartition.xPrimaryCount = sfPRIMARY_PARTITIONS;
    xPartition.eSizeType = eSizeIsQuota;

    /* Partition the disk */
    xError = FF_Partition( pxDisk, &xPartition );
    FF_PRINTF( "FF_Partition: %s\n", ( const char * ) FF_GetErrMessage( xError ) );

    if( FF_isERR( xError ) == pdFALSE )
    {
        /* Format the partition. */
        xError = FF_Format( pxDisk, sfPARTITION_NUMBER, pdFALSE, pdFALSE );
        FF_PRINTF( "FF_Format: %s\n", ( const char * ) FF_GetErrMessage( xError ) );
		if ( FF_isERR( xError ) == pdFALSE )
		{
			xError = FF_Mount( pxDisk, 0 );
			FF_PRINTF( "FF_Mount: %s\n", ( const char * ) FF_GetErrMessage( xError ) );
			FF_SFDiskShowPartition( pxDisk );
		}
    }

    return xError;
}

/* mount an spi-flash partition */
FF_Disk_t *FF_SFDiskInit( const char *pcName )
{
FF_Error_t xFFError;
BaseType_t xPartitionNumber = 0;
FF_CreationParameters_t xParameters;
FF_Disk_t * pxDisk;

	sflash = sfud_get_device(0);
	if( sflash != NULL )
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
			//pxDisk->ulNumberOfSectors = sflash->chip.capacity / sfSECTOR_SIZE;
			pxDisk->ulNumberOfSectors = OTA_MEDIA_SIZE / sfSECTOR_SIZE;
			pxDisk->ulSignature = sfSIGNATURE;

			if( xPlusFATMutex != NULL)
			{
				memset( &xParameters, '\0', sizeof( xParameters ) );
				xParameters.ulMemorySize = sfIOMAN_MEM_SIZE;
				xParameters.ulSectorSize = sfSECTOR_SIZE;
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
					FF_PRINTF( "FF_SFDiskInit: FF_CreateIOManger: %s\n", ( const char * ) FF_GetErrMessage( xFFError ) );
					FF_SFDiskDelete( pxDisk );
					pxDisk = NULL;
				}
				else
				{
					pxDisk->xStatus.bIsInitialised = pdTRUE;
					pxDisk->xStatus.bPartitionNumber = xPartitionNumber;
					if( FF_SFDiskMount( pxDisk ) == 0 )
					{
						if ( PartitionAndFormatSFDisk(pxDisk) != FF_ERR_NONE )
						{
							FF_PRINTF( "FF_SFDiskInit: Mount spi-flash fail.\n");
							FF_SFDiskDelete( pxDisk );
							pxDisk = NULL;
						}
						else
						{
							if( pcName == NULL )
							{
								pcName = "/";
							}
							FF_FS_Add( pcName, pxDisk );
							FF_PRINTF( "FF_SFDiskInit: Mounted spi-flash as root \"%s\"\n", pcName );
						}
					}
					else
					{
						if( pcName == NULL )
						{
							pcName = "/";
						}
						FF_FS_Add( pcName, pxDisk );
						FF_PRINTF( "FF_SFDiskInit: Mounted spi-flash as root \"%s\"\n", pcName );
					}
				}	/* if( pxDisk->pxIOManager != NULL ) */
			}	/* if( xPlusFATMutex != NULL) */
		}	/* if( pxDisk != NULL ) */
		else
		{
			FF_PRINTF( "FF_SFDiskInit: Malloc failed\n" );
		}
	}	/* if( sflash != NULL ) */
	else
	{
		FF_PRINTF( "FF_SFDiskInit: sfud_get_device failed\n" );
		pxDisk = NULL;
	}

	return pxDisk;
}
/*-----------------------------------------------------------*/

BaseType_t FF_SFDiskFormat( FF_Disk_t *pxDisk, BaseType_t xPartitionNumber )
{
FF_Error_t xError;
BaseType_t xReturn = pdFAIL;

	xError = FF_Unmount( pxDisk );

	if( FF_isERR( xError ) != pdFALSE )
	{
		FF_PRINTF( "FF_SFDiskFormat: unmount fails: %08x\n", ( unsigned ) xError );
	}
	else
	{
		/* Format the drive - try FAT32 with large clusters. */
		xError = FF_Format( pxDisk, xPartitionNumber, pdFALSE, pdFALSE);

		if( FF_isERR( xError ) )
		{
			FF_PRINTF( "FF_SFDiskFormat: %s\n", (const char*)FF_GetErrMessage( xError ) );
		}
		else
		{
			FF_PRINTF( "FF_SFDiskFormat: OK, now remounting\n" );
			pxDisk->xStatus.bPartitionNumber = xPartitionNumber;
			xError = FF_SFDiskMount( pxDisk );
			FF_PRINTF( "FF_SFDiskFormat: rc %08x\n", ( unsigned )xError );
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
BaseType_t FF_SFDiskMount( FF_Disk_t *pxDisk )
{
FF_Error_t xFFError;
BaseType_t xReturn;

	/* Mount the partition */
	xFFError = FF_Mount( pxDisk, pxDisk->xStatus.bPartitionNumber );

	if( FF_isERR( xFFError ) )
	{
		FF_PRINTF( "FF_SFDiskMount: %08lX\n", xFFError );
		xReturn = pdFAIL;
	}
	else
	{
		pxDisk->xStatus.bIsMounted = pdTRUE;
		FF_PRINTF( "****** FreeRTOS+FAT initialized %lu sectors\n", pxDisk->pxIOManager->xPartition.ulTotalSectors );
		FF_SFDiskShowPartition( pxDisk );
		xReturn = pdPASS;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

/* Release all resources */
BaseType_t FF_SFDiskDelete( FF_Disk_t *pxDisk )
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

BaseType_t FF_SFDiskShowPartition( FF_Disk_t *pxDisk )
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
		iPercentageFree = ( int ) ( ( sfHUNDRED_64_BIT * ullFreeSectors + pxIOManager->xPartition.ulDataSectors / 2 ) /
			( ( uint64_t )pxIOManager->xPartition.ulDataSectors ) );

		ulTotalSizeMB = pxIOManager->xPartition.ulDataSectors / sfSECTORS_PER_MB;
		ulFreeSizeMB = ( uint32_t ) ( ullFreeSectors / sfSECTORS_PER_MB );

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
