//hpatchz.c
// patch tool
//
/*
 This is the HDiffPatch copyright.

 Copyright (c) 2012-2021 HouSisong All Rights Reserved.

 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:

 The above copyright notice and this permission notice shall be
 included in all copies of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>  //fprintf
#include "patch.h"
#include "file_for_patch.h"
#include "sysinfo.h"
#include "updatefile.h"
#include "mmcsd_core.h"
#include "ota_update.h"
#include "delta_update.h"

#ifdef DELTA_UPDATE_SUPPORT
#ifndef _IS_NEED_PRINT_LOG
#   define _IS_NEED_PRINT_LOG   1
#endif
#if (_IS_NEED_PRINT_LOG)
#   define  _log_info_utf8  hpatch_printPath_utf8
#else
#   define  printf(...)
#   define  _log_info_utf8(...) do{}while(0)
#endif

#ifndef _IS_NEED_DEFAULT_CompressPlugin
#   define _IS_NEED_DEFAULT_CompressPlugin 1
#endif
#if (_IS_NEED_DEFAULT_CompressPlugin)
//===== select needs decompress plugins or change to your plugin=====
#   define _CompressPlugin_zlib
#endif

#include "decompress_plugin_demo.h"

typedef enum THPatchResult {
    HPATCH_SUCCESS=0,
    HPATCH_OPTIONS_ERROR=1,
    HPATCH_OPENREAD_ERROR,
    HPATCH_OPENWRITE_ERROR,
    HPATCH_FILEREAD_ERROR,
    HPATCH_FILEWRITE_ERROR, // 5 //see 24
    HPATCH_FILEDATA_ERROR,
    HPATCH_FILECLOSE_ERROR,
    HPATCH_MEM_ERROR, //see 22
    HPATCH_HDIFFINFO_ERROR,
    HPATCH_COMPRESSTYPE_ERROR, // 10
    HPATCH_HPATCH_ERROR,
    HPATCH_PATHTYPE_ERROR, //adding begin v3.0
    HPATCH_TEMPPATH_ERROR,
    HPATCH_DELETEPATH_ERROR,
    HPATCH_RENAMEPATH_ERROR, // 15
    HPATCH_SPATCH_ERROR,
    HPATCH_BSPATCH_ERROR,
    HPATCH_VCPATCH_ERROR,

    HPATCH_DECOMPRESSER_OPEN_ERROR=20,
    HPATCH_DECOMPRESSER_CLOSE_ERROR,
    HPATCH_DECOMPRESSER_MEM_ERROR,
    HPATCH_DECOMPRESSER_DECOMPRESS_ERROR,
    HPATCH_FILEWRITE_NO_SPACE_ERROR,

} THPatchResult;

#define _return_check(value,exitCode,errorInfo){ \
    if (!(value)) { LOG_ERR(errorInfo " ERROR!\n"); return exitCode; } }

#define _options_check(value,errorInfo){ \
    if (!(value)) { LOG_ERR("options " errorInfo " ERROR!\n\n"); \
        printHelpInfo(); return HPATCH_OPTIONS_ERROR; } }

#define kPatchCacheSize_min      (hpatch_kStreamCacheSize*8)
#define kPatchCacheSize_bestmin  ((size_t)1<<21)
#define kPatchCacheSize_default  ((size_t)1<<18)

#define _kNULL_VALUE    (-1)
#define _kNULL_SIZE     (~(size_t)0)

#define  check_on_error(errorType) { \
    if (result==HPATCH_SUCCESS) result=errorType; if (!_isInClear){ goto clear; } }
#define  check(value,errorType,errorInfo) { \
    if (!(value)){ LOG_ERR(errorInfo " ERROR!\n"); check_on_error(errorType); } }

#if (_HPATCH_IS_USED_errno)
#   define check_ferr(fileError,errorType,errorInfo){ \
        if (fileError){ \
            result=HPATCH_SUCCESS; \
            if (ENOSPC==(fileError)){ \
                check(hpatch_FALSE,HPATCH_FILEWRITE_NO_SPACE_ERROR,errorInfo); \
            }else{ \
                check(hpatch_FALSE,errorType,errorInfo); } } }
#else
#   define check_ferr(fileError,errorType,errorInfo) { \
        if (fileError) { result=HPATCH_SUCCESS; check(hpatch_FALSE,errorType,errorInfo); } }
#endif

#define check_dec(decError) { \
        switch (decError){    \
            case hpatch_dec_ok:          break; \
            case hpatch_dec_mem_error:   check(0,HPATCH_DECOMPRESSER_MEM_ERROR,"decompressPlugin alloc memory"); \
            case hpatch_dec_open_error:  check(0,HPATCH_DECOMPRESSER_OPEN_ERROR,"decompressPlugin open"); \
            case hpatch_dec_close_error: check(0,HPATCH_DECOMPRESSER_CLOSE_ERROR,"decompressPlugin close"); \
            default: { assert(decError==hpatch_dec_error); \
                       check(0,HPATCH_DECOMPRESSER_DECOMPRESS_ERROR,"decompressPlugin decompress"); } break; \
        } }

#define _try_rt_dec(dec) { if (dec.is_can_open(compressType)) return &dec; }

static const hpatch_TDecompress* __find_decompressPlugin(const char* compressType){
#ifdef  _CompressPlugin_zlib
    _try_rt_dec(zlibDecompressPlugin);
#endif
    return 0;
}

static hpatch_BOOL getDecompressPlugin(const hpatch_compressedDiffInfo* diffInfo,
                                       hpatch_TDecompress* out_decompressPlugin){
    const hpatch_TDecompress* decompressPlugin=0;
    memset(out_decompressPlugin,0,sizeof(*out_decompressPlugin));
    if (diffInfo->compressedCount>0){
        decompressPlugin=__find_decompressPlugin(diffInfo->compressType);
        if ((0==decompressPlugin)||(decompressPlugin->open==0)) return hpatch_FALSE; //error
    }
    if (decompressPlugin){
        *out_decompressPlugin=*decompressPlugin;
        out_decompressPlugin->decError=hpatch_dec_ok;
    }
    return hpatch_TRUE;
}



typedef struct _THDiffInfos{
    hpatch_BOOL                 isSingleCompressedDiff;
    hpatch_BOOL                 isBsDiff;
    hpatch_BOOL                 isVcDiff;
    hpatch_compressedDiffInfo   diffInfo;
    hpatch_TDecompress          _decompressPlugin;
} _THDiffInfos;

#define _kUnavailableSize   hpatch_kNullStreamPos

static int _getHDiffInfos(_THDiffInfos* out_diffInfos,const hpatch_TFileStreamInput* diffData){
    int     result=HPATCH_SUCCESS;
    int     _isInClear=hpatch_FALSE;
    hpatch_TDecompress* decompressPlugin=&out_diffInfos->_decompressPlugin;
    hpatch_compressedDiffInfo* diffInfo=&out_diffInfos->diffInfo;
    if (getCompressedDiffInfo(diffInfo,&diffData->base)){
        check(diffInfo->oldDataSize!=_kUnavailableSize,HPATCH_HDIFFINFO_ERROR,"saved oldDataSize");
    }else{
        check(hpatch_FALSE,HPATCH_HDIFFINFO_ERROR,"is hdiff file? get diffInfo");
    }
    if (decompressPlugin->open==0){
        if (getDecompressPlugin(diffInfo,decompressPlugin)){
        }else{
            LOG_ERR("can not decompress \"%s\" data ERROR!\n",out_diffInfos->diffInfo.compressType);
            check_on_error(HPATCH_COMPRESSTYPE_ERROR);
        }
    }
clear:
    _isInClear=hpatch_TRUE;
    check(!diffData->fileError,HPATCH_FILEREAD_ERROR,"read file");
    return result;
}

static void _printHDiffInfos(const _THDiffInfos* diffInfos,hpatch_BOOL isInDirDiff){
    const   hpatch_compressedDiffInfo* diffInfo=&diffInfos->diffInfo;
    {
        const char* typeTag="HDiff";
        printf("       diffDataType: %s %s\n",typeTag,isInDirDiff?"(in DirHDiff)":"");
    }
    if (diffInfo->oldDataSize==_kUnavailableSize)
        printf("  saved oldDataSize: unavailable\n");
    else
        printf("  saved oldDataSize: %" PRIu64 "\n",diffInfo->oldDataSize);
        printf("  saved newDataSize: %" PRIu64 "\n",diffInfo->newDataSize);
        printf("       compressType: \"%s\" (need decompress %d)\n",diffInfo->compressType,diffInfo->compressedCount);
}

#define _free_mem(p) { if (p) { vPortFree(p); p=0; } }

static TByte* getPatchMemCache(hpatch_BOOL isLoadOldAll,size_t patchCacheSize,size_t mustAppendMemSize,
                               hpatch_StreamPos_t oldDataSize,size_t* out_memCacheSize){
    TByte* temp_cache=0;
    size_t temp_cache_size;
    {
        hpatch_StreamPos_t limitCacheSize;
        const hpatch_StreamPos_t bestMaxCacheSize=oldDataSize+kPatchCacheSize_bestmin;
        if (isLoadOldAll){
            limitCacheSize=bestMaxCacheSize;
        }else{
            limitCacheSize=(patchCacheSize<kPatchCacheSize_min)?kPatchCacheSize_min:patchCacheSize;
            limitCacheSize=(limitCacheSize<bestMaxCacheSize)?limitCacheSize:bestMaxCacheSize;
        }
        if (limitCacheSize>(size_t)(_kNULL_SIZE-mustAppendMemSize))//too large
            limitCacheSize=(size_t)(_kNULL_SIZE-mustAppendMemSize);
        temp_cache_size=(size_t)limitCacheSize;
    }
    while (temp_cache_size>=kPatchCacheSize_min){
        temp_cache=(TByte*)pvPortMalloc(mustAppendMemSize+temp_cache_size);
        if (temp_cache) break;
        temp_cache_size>>=1;
    }
    *out_memCacheSize=(temp_cache)?(mustAppendMemSize+temp_cache_size):0;
    return temp_cache;
}

int delta_update(int filetype, size_t patchFileSize)
{
    int result = HPATCH_SUCCESS;
    int _isInClear = hpatch_FALSE;
    uint32_t time0 = xTaskGetTickCount();
    _THDiffInfos diffInfos={0};
    hpatch_TDecompress*  decompressPlugin=&diffInfos._decompressPlugin;
    hpatch_TFileStreamOutput newData;
    hpatch_TFileStreamInput diffData;
    hpatch_TFileStreamInput oldData;
    hpatch_TStreamInput* poldData=&oldData.base;
    TByte* temp_cache=0;
    size_t temp_cache_size;
    int patch_result = HPATCH_SUCCESS;
	char oldFileName[32];
	char diffFileName[32];
	char outNewFileName[32];
	SysInfo *sysinfo = GetSysInfo();
	SysInfo bak_sysinfo;

	memcpy(&bak_sysinfo, sysinfo, sizeof(SysInfo));

	//没有备份的程序不支持差分升级
	assert(filetype >= UPFILE_TYPE_WHOLE && filetype <= UPFILE_TYPE_STEPLDR);
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	assert(filetype != UPFILE_TYPE_FIRSTLDR);
#endif

	hpatch_setPatchFileSize(patchFileSize);

    hpatch_TFileStreamInput_init(&oldData);
    hpatch_TFileStreamInput_init(&diffData);
    hpatch_TFileStreamOutput_init(&newData);

#ifdef HPATCH_FILE_USE_FILESYSTEM
	strcpy(oldFileName, OTA_MOUNT_PATH);
	strcat(oldFileName, g_upfilename[filetype]);
	strcpy(diffData, oldFileName);
	strcpy(strrchr(diffData, '.'), "_patch.bin");
	strcpy(outNewFileName, oldFileName);
	strcpy(strrchr(outNewFileName, '.'), "_new.bin");
#else
	strcpy(oldFileName, hpatch_NofsFileName[filetype]);
	strcpy(diffFileName, "patch");
	strcpy(outNewFileName, "new");
	strcat(outNewFileName, oldFileName);
#endif

    {//open
        printf(    "old : \""); if (oldFileName) _log_info_utf8(oldFileName);
        printf("\"\ndiff: \""); _log_info_utf8(diffFileName);
        printf("\"\nout : \""); _log_info_utf8(outNewFileName);
        printf("\"\n");
        if ((0==oldFileName)||(0==strlen(oldFileName))){
            mem_as_hStreamInput(&oldData.base,0,0);
        }else{
            check(hpatch_TFileStreamInput_open(&oldData,oldFileName),
                  HPATCH_OPENREAD_ERROR,"open oldFile for read");
        }
        check(hpatch_TFileStreamInput_open(&diffData,diffFileName),
              HPATCH_OPENREAD_ERROR,"open diffFile for read");
    }
    printf("  input oldDataSize: %" PRIu64 "\n",poldData->streamSize);
    printf("       diffDataSize: %" PRIu64 "\n",diffData.base.streamSize);

    {//info
        int ret=_getHDiffInfos(&diffInfos,&diffData);
#if (_IS_NEED_PRINT_LOG)
        if ((ret!=HPATCH_SUCCESS)&&(ret!=HPATCH_COMPRESSTYPE_ERROR))
            check_on_error(ret);
        _printHDiffInfos(&diffInfos,hpatch_FALSE);
        printf("\n");
#endif
        if (ret!=HPATCH_SUCCESS)
            check_on_error(ret);
    }
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	/* EMMC JTAG烧录时由于没法知道真实文件大小stepldr_size的值不是真实值 */
	if (filetype == UPFILE_TYPE_STEPLDR)
		poldData->streamSize = diffInfos.diffInfo.oldDataSize;
#endif
    if (decompressPlugin->open==0) decompressPlugin=0;
    if ((poldData->streamSize!=diffInfos.diffInfo.oldDataSize)&&(diffInfos.diffInfo.oldDataSize!=_kUnavailableSize)){
        LOG_ERR("input oldFile oldDataSize %" PRIu64 " != diffFile saved oldDataSize %" PRIu64 " ERROR!\n",
                poldData->streamSize,diffInfos.diffInfo.oldDataSize);
        check_on_error(HPATCH_FILEDATA_ERROR);
    }

    check(hpatch_TFileStreamOutput_open(&newData, outNewFileName,diffInfos.diffInfo.newDataSize),
          HPATCH_OPENWRITE_ERROR,"open out newFile for write");

#ifdef HPATCH_FILE_CHECK_DEBUG
	newData.m_file->filebuf = pvPortMalloc(diffInfos.diffInfo.newDataSize);
#endif

	/* 这三种文件没有备份，需要先备份整个update.bin镜像再升级备份区域 */
	if (filetype >= UPFILE_TYPE_RESOURCE && filetype <= UPFILE_TYPE_APP) {
		if (diffInfos.diffInfo.newDataSize > get_subfile_maxsize(filetype)) {
			printf("Not have enough space to update subfile %s.\n", oldFileName);
			result = HPATCH_FILEWRITE_NO_SPACE_ERROR;
			goto clear;
		}
		if (backup_whole_image()) {
			printf("backup_whole_image fail.\n");
			result = HPATCH_FILEWRITE_ERROR;
			goto clear;
		}
	}

#ifndef HPATCH_FILE_USE_FILESYSTEM
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	sfud_erase(newData.m_file->sflash, newData.m_file->offset, diffInfos.diffInfo.newDataSize);
#endif
#endif

    {
        hpatch_StreamPos_t maxWindowSize=poldData->streamSize;
        hpatch_size_t      mustAppendMemSize=0;
        temp_cache=getPatchMemCache(hpatch_FALSE,kPatchCacheSize_default,mustAppendMemSize,maxWindowSize, &temp_cache_size);
    }
    check(temp_cache,HPATCH_MEM_ERROR,"alloc cache memory");
    {
        if (!patch_decompress_with_cache(&newData.base,poldData,&diffData.base,decompressPlugin,
                                         temp_cache,temp_cache+temp_cache_size))
            patch_result=HPATCH_HPATCH_ERROR;
    }
    if (patch_result!=HPATCH_SUCCESS){
        check(!oldData.fileError,HPATCH_FILEREAD_ERROR,"oldFile read");
        check(!diffData.fileError,HPATCH_FILEREAD_ERROR,"diffFile read");
        check_ferr(newData.fileError,HPATCH_FILEWRITE_ERROR,"out newFile write");
        if (decompressPlugin) check_dec(decompressPlugin->decError);
        check(hpatch_FALSE,patch_result,"patch run");
    }
    if (newData.out_length!=newData.base.streamSize){
        LOG_ERR("out newFile dataSize %" PRIu64 " != diffFile saved newDataSize %" PRIu64 " ERROR!\n",
               newData.out_length,newData.base.streamSize);
        check_on_error(HPATCH_FILEDATA_ERROR);
    }
    printf("  patch ok!\n");

	printf("checksum after patch...\n");
	if (get_upfile_checksum(filetype, diffInfos.diffInfo.newDataSize, 1, 1)) {
		if (filetype == UPFILE_TYPE_WHOLE) {
			UpFileHeader *header;
			uint32_t headersize;

			headersize = sizeof(UpFileHeader) + sizeof(UpFileInfo);
			header = pvPortMalloc(headersize);
			if (!header) {
				printf("patch malloc fail.\n");
				result = HPATCH_MEM_ERROR;
				goto clear;
			}
#if DEVICE_TYPE_SELECT != EMMC_FLASH
			sfud_read(newData.m_file->sflash, newData.m_file->offset, headersize, (void*)header);
#else
			emmc_read(newData.m_file->offset, headersize, (void*)header);
#endif
			if (header->magic != MKTAG('U', 'P', 'D', 'F')) {
				printf("Error! Wrong update file.\n");
				vPortFree(header);
				result = HPATCH_HPATCH_ERROR;
				goto clear;
			}
			bak_sysinfo.app_checksum = header->checksum;
			bak_sysinfo.app_size = header->files[0].size;
			vPortFree(header);
		} else if (filetype == UPFILE_TYPE_APP) {
			bak_sysinfo.app_size = diffInfos.diffInfo.newDataSize;
		} else if (filetype == UPFILE_TYPE_FIRSTLDR) {
			bak_sysinfo.loader_size = diffInfos.diffInfo.newDataSize;
		} else if (filetype == UPFILE_TYPE_STEPLDR) {
			bak_sysinfo.stepldr_size = diffInfos.diffInfo.newDataSize;
		}

		/* 升级这三个文件后需要更新UpFileHeader头信息以及app_checksum */
		if (filetype >= UPFILE_TYPE_RESOURCE && filetype <= UPFILE_TYPE_APP) {
			update_part_upfile_info(sysinfo, filetype, diffInfos.diffInfo.newDataSize);
			bak_sysinfo.app_checksum = get_upfile_checksum(UPFILE_TYPE_WHOLE, 0, 0, 1);
			if (sysinfo->image_offset == UPDATEFILE_MEDIA_OFFSET)
				bak_sysinfo.image_offset = UPDATEFILE_MEDIA_B_OFFSET;
			else
				bak_sysinfo.image_offset = UPDATEFILE_MEDIA_OFFSET;
		} else {
			set_upfile_offset(&bak_sysinfo, filetype, newData.m_file->offset);
		}
		memcpy(sysinfo, &bak_sysinfo, sizeof(SysInfo));
		SaveSysInfo();
		printf("patch %s ok.\n", oldFileName);
		wdt_cpu_reboot();
	} else {
		printf("checksum after patch fail.\n");
		result = HPATCH_HPATCH_ERROR;
	}

clear:
    _isInClear=hpatch_TRUE;
    check(hpatch_TFileStreamOutput_close(&newData),HPATCH_FILECLOSE_ERROR,"out newFile close");
    check(hpatch_TFileStreamInput_close(&diffData),HPATCH_FILECLOSE_ERROR,"diffFile close");
    check(hpatch_TFileStreamInput_close(&oldData),HPATCH_FILECLOSE_ERROR,"oldFile close");
    _free_mem(temp_cache);
    printf("\nhpatchz time: %d ms\n",(xTaskGetTickCount()-time0));

    return result;
}
#endif