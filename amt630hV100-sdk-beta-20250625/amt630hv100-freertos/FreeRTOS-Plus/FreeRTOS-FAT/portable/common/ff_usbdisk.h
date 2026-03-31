/*
 * FreeRTOS+FAT V2.3.3
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 * https://github.com/FreeRTOS
 *
 */

#ifndef __USBDISK_H__

    #define __USBDISK_H__

    #ifdef __cplusplus
        extern "C" {
    #endif

    #include "ff_headers.h"


/* Return non-zero if the SD-card is present.
 * The parameter 'pxDisk' may be null, unless device locking is necessary. */
    BaseType_t FF_USBDiskDetect( FF_Disk_t * pxDisk );

/* Create a RAM disk, supplying enough memory to hold N sectors of 512 bytes each */
    FF_Disk_t * FF_USBDiskInit( const char * pcName );

    BaseType_t FF_USBDiskReinit( FF_Disk_t * pxDisk );

/* Unmount the volume */
    BaseType_t FF_USBDiskUnmount( FF_Disk_t * pDisk );

/* Mount the volume */
    BaseType_t FF_USBDiskMount( FF_Disk_t * pDisk );

/* Release all resources */
    BaseType_t FF_USBDiskDelete( FF_Disk_t * pDisk );

/* Show some partition information */
    BaseType_t FF_USBDiskShowPartition( FF_Disk_t * pDisk );

/* Flush changes from the driver's buf to disk */
    void FF_USBDiskFlush( FF_Disk_t * pDisk );

/* Format a given partition on an SD-card. */
    BaseType_t FF_USBDiskFormat( FF_Disk_t * pxDisk,
                                BaseType_t aPart );

/* Return non-zero if an SD-card is detected in a given slot. */
    BaseType_t FF_USBDiskInserted( BaseType_t xDriveNr );

/* _RB_ Temporary function - ideally the application would not need the IO
 * manageer structure, just a handle to a disk. */
    FF_IOManager_t * usbdisk_ioman( FF_Disk_t * pxDisk );

    #ifdef __cplusplus
        } /* extern "C" */
    #endif

#endif /* __USBDISK_H__ */
