//file_for_patch.h
// patch demo file tool
//
/*
 This is the HDiffPatch copyright.

 Copyright (c) 2012-2019 HouSisong All Rights Reserved.

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
#ifndef HPatch_file_for_patch_h
#define HPatch_file_for_patch_h

#include "patch_types.h"
#include "ff_stdio.h"

//#define HPATCH_FILE_USE_FILESYSTEM
#ifndef HPATCH_FILE_USE_FILESYSTEM
//#define HPATCH_FILE_CHECK_DEBUG
#endif

#   define  _set_errno_new(v)

#   define  set_ferr(_saved_errno,_throw_errno) /*save new errno*/\
                        do { if (_throw_errno) _saved_errno=_throw_errno; } while(0)
#   define  _update_ferr(fe)        do { fe=hpatch_TRUE; } while(0)
#   define  _update_ferrv(fe,v)     _update_ferr(fe)
#   define  mix_ferr(_saved_errno,_throw_errno)     set_ferr(_saved_errno,_throw_errno)

#ifdef __cplusplus
extern "C" {
#endif

static const char kPatch_dirSeparator = '/';

typedef unsigned char TByte;
#define hpatch_kFileIOBestMaxSize  (1<<20)
#define hpatch_kPathMaxSize  (1024*2)

hpatch_inline static
hpatch_BOOL hpatch_getIsDirName(const char* path_utf8){
    size_t len=strlen(path_utf8);
    return (len>0)&&(path_utf8[len-1]==kPatch_dirSeparator);
}

hpatch_inline static const char* findUntilEnd(const char* str,char c){
    const char* result=strchr(str,c);
    return (result!=0)?result:(str+strlen(str));
}


#define _path_noEndDirSeparator(dst_path,src_path)  \
    char  dst_path[hpatch_kPathMaxSize];      \
    {   size_t len=strlen(src_path);   \
        if (len>=hpatch_kPathMaxSize) { _set_errno_new(ENAMETOOLONG); return hpatch_FALSE;} /* error */ \
        if ((len>0)&&(src_path[len-1]==kPatch_dirSeparator)) --len; /* without '/' */\
        memcpy(dst_path,src_path,len); \
        dst_path[len]='\0';  } /* safe */

hpatch_inline static
hpatch_BOOL hpatch_getIsSamePath(const char* xPath_utf8,const char* yPath_utf8){
    _path_noEndDirSeparator(xPath,xPath_utf8);
    {   _path_noEndDirSeparator(yPath,yPath_utf8);
        if (0==strcmp(xPath,yPath)){
            return hpatch_TRUE;
        }else{
            // WARING!!! better return getCanonicalPath(xPath)==getCanonicalPath(yPath);
            return hpatch_FALSE;
        }
    }
}

hpatch_inline static
int hpatch_printPath_utf8(const char* pathTxt_utf8){
    return printf("%s",pathTxt_utf8);
}

hpatch_inline static
int hpatch_printStdErrPath_utf8(const char* pathTxt_utf8){
    return LOG_ERR("%s",pathTxt_utf8);
}

#ifdef HPATCH_FILE_USE_FILESYSTEM
typedef FF_FILE* hpatch_FileHandle;
#else
#include "ota_update.h"
#include "sfud.h"

extern const char *hpatch_NofsFileName[];

typedef struct {
	int filetype;
	int newfile;
	uint32_t offset;
	uint32_t size;
	uint32_t pos;
	uint32_t checksum;
	sfud_flash *sflash;
#ifdef HPATCH_FILE_CHECK_DEBUG
	uint8_t *filebuf;
#endif
} DEV_FILE;
typedef DEV_FILE *hpatch_FileHandle;
void hpatch_setPatchFileSize(uint32_t size);
#endif

typedef struct hpatch_TFileStreamInput{
    hpatch_TStreamInput base;
    hpatch_FileHandle   m_file;
    hpatch_StreamPos_t  m_fpos;
    hpatch_StreamPos_t  m_offset;
    hpatch_FileError_t  fileError;
} hpatch_TFileStreamInput;

hpatch_inline
static void hpatch_TFileStreamInput_init(hpatch_TFileStreamInput* self){
    memset(self,0,sizeof(hpatch_TFileStreamInput));
}
hpatch_BOOL hpatch_TFileStreamInput_open(hpatch_TFileStreamInput* self,const char* fileName_utf8);
hpatch_BOOL hpatch_TFileStreamInput_setOffset(hpatch_TFileStreamInput* self,hpatch_StreamPos_t offset);
hpatch_BOOL hpatch_TFileStreamInput_close(hpatch_TFileStreamInput* self);

typedef struct hpatch_TFileStreamOutput{ //is hpatch_TFileStreamInput !
    hpatch_TStreamOutput base; //is hpatch_TStreamInput + write
    hpatch_FileHandle   m_file;
    hpatch_StreamPos_t  m_fpos;
    hpatch_StreamPos_t  m_offset; //now not used
    hpatch_FileError_t  fileError; // 0: no error; other: saved errno value;
    //
    hpatch_BOOL         is_random_out;
    hpatch_BOOL         is_in_readModel;
    hpatch_StreamPos_t  out_length;
} hpatch_TFileStreamOutput;

hpatch_inline
static void hpatch_TFileStreamOutput_init(hpatch_TFileStreamOutput* self){
    memset(self,0,sizeof(hpatch_TFileStreamOutput));
}
hpatch_BOOL hpatch_TFileStreamOutput_open(hpatch_TFileStreamOutput* self,const char* fileName_utf8,
                                          hpatch_StreamPos_t max_file_length);
hpatch_inline static
void hpatch_TFileStreamOutput_setRandomOut(hpatch_TFileStreamOutput* self,hpatch_BOOL is_random_out){
    self->is_random_out=is_random_out;
}

hpatch_BOOL hpatch_TFileStreamOutput_flush(hpatch_TFileStreamOutput* self);
hpatch_BOOL hpatch_TFileStreamOutput_close(hpatch_TFileStreamOutput* self);

hpatch_BOOL hpatch_TFileStreamOutput_reopen(hpatch_TFileStreamOutput* self,const char* fileName_utf8,
                                            hpatch_StreamPos_t max_file_length);
//hpatch_BOOL hpatch_TFileStreamOutput_truncate(hpatch_TFileStreamOutput* self,hpatch_StreamPos_t new_file_length);

#ifdef __cplusplus
}
#endif
#endif
