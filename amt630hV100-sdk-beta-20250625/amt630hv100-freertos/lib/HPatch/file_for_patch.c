//file_for_patch.c
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
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include "mmcsd_core.h"
#include "file_for_patch.h"
/*
#ifdef _MSC_VER
#   include <io.h>    //_chsize_s
#endif
*/

const char *hpatch_NofsFileName[] =
{
	"whole",
	"resource",
	"animation",
	"app",
	"firstldr",
	"stepldr",
	"lnchemmc",
};

#ifndef HPATCH_FILE_USE_FILESYSTEM
static uint32_t patchFileSize;

/* if define HPATCH_FILE_USE_FILESYSTEM, you must call this function
   to set the patch file size */
void hpatch_setPatchFileSize(uint32_t size)
{
	patchFileSize = size;
}
#endif

hpatch_inline static
hpatch_BOOL _import_fileSeek64(hpatch_FileHandle file,hpatch_StreamPos_t seekPos,int whence){
#ifdef HPATCH_FILE_USE_FILESYSTEM
    return ff_fseek(file,seekPos,whence)==0;
#else
	if (whence == SEEK_SET) {
		file->pos = seekPos;
	} else if (whence == SEEK_CUR) {
		if ((file->pos + seekPos) > file->size)
			return hpatch_FALSE;
		else
			file->pos += seekPos;
	} else if (whence == SEEK_END) {
		file->pos = file->size;
	}

	return hpatch_TRUE;
#endif
}

hpatch_inline static
hpatch_BOOL _import_fileSeek64To(hpatch_FileHandle file,hpatch_StreamPos_t seekPos){
    return _import_fileSeek64(file,seekPos,SEEK_SET);
}

hpatch_inline static
hpatch_BOOL _import_fileTell64(hpatch_FileHandle file,hpatch_StreamPos_t* outPos){
#ifdef HPATCH_FILE_USE_FILESYSTEM
    off_t pos=ff_ftell(file);
#else
	off_t pos = file->pos;
#endif
    *outPos=(hpatch_StreamPos_t)pos;
    return (pos>=0);
}

hpatch_inline static
hpatch_BOOL _import_fileClose(hpatch_FileHandle* pfile){
    hpatch_FileHandle file=*pfile;
    if (file){
        *pfile=0;
#ifdef HPATCH_FILE_USE_FILESYSTEM
        if (0!=ff_fclose(file))
            return hpatch_FALSE;
#else
		vPortFree(file);
#endif
    }
    return hpatch_TRUE;
}

#   define  _import_fileClose_No_errno(pfile) _import_fileClose(pfile)

hpatch_BOOL _import_fileRead(hpatch_FileHandle file,TByte* buf,TByte* buf_end){
    while (buf<buf_end) {
        size_t readLen=(size_t)(buf_end-buf);
        if (readLen>hpatch_kFileIOBestMaxSize) readLen=hpatch_kFileIOBestMaxSize;
#ifdef HPATCH_FILE_USE_FILESYSTEM
        if (readLen!=ff_fread(buf,1,readLen,file)) return hpatch_FALSE;
#else
#if DEVICE_TYPE_SELECT == EMMC_FLASH
		if (0 != emmc_read(file->offset + file->pos, readLen, buf))
			return hpatch_FALSE;
#else
		if (SFUD_SUCCESS != sfud_read(file->sflash, file->offset + file->pos, readLen, buf))
			return hpatch_FALSE;
#endif
#ifdef HPATCH_FILE_CHECK_DEBUG
		memcpy(file->filebuf + file->pos, buf, readLen);
#endif
		file->pos += readLen;
#endif
        buf+=readLen;
    }
    return buf==buf_end;
}

hpatch_BOOL _import_fileWrite(hpatch_FileHandle file,const TByte* data,const TByte* data_end){
    while (data<data_end) {
        size_t writeLen=(size_t)(data_end-data);
        if (writeLen>hpatch_kFileIOBestMaxSize) writeLen=hpatch_kFileIOBestMaxSize;
#ifdef HPATCH_FILE_USE_FILESYSTEM
        if (writeLen!=ff_fwrite(data,1,writeLen,file)) return hpatch_FALSE;
#else
#if DEVICE_TYPE_SELECT == EMMC_FLASH
		if (0 != emmc_write(file->offset + file->pos, writeLen, (uint8_t *)data))
			return hpatch_FALSE;
#else
		if (SFUD_SUCCESS != sfud_write(file->sflash, file->offset + file->pos, writeLen, data))
			return hpatch_FALSE;
#endif
#ifdef HPATCH_FILE_CHECK_DEBUG
		memcpy(file->filebuf + file->pos, data, writeLen);
#endif
		file->pos += writeLen;
#endif
        data+=writeLen;
    }
    return data==data_end;
}

hpatch_inline static
hpatch_BOOL _import_fileFlush(hpatch_FileHandle writedFile){
    //return (0==ff_fflush(writedFile));
    return hpatch_TRUE;
}

/* // retained data error
hpatch_BOOL _import_fileTruncate(hpatch_FileHandle file,hpatch_StreamPos_t new_file_length){
#ifdef _MSC_VER
    int fno=_fileno(file);
    if (fno==-1) return hpatch_FALSE;
    if (_chsize_s(fno,new_file_length)!=0) return hpatch_FALSE;
#else
    int fno=fileno(file);
    if (fno==-1) return hpatch_FALSE;
    if (ftruncate(fno,new_file_length)!=0) return hpatch_FALSE;
#endif
    return hpatch_TRUE;
}*/

#   define _FileModeType const char*
#   define _kFileReadMode  "rb"
#   define _kFileWriteMode "wb+"
#   define _kFileReadWriteMode "rb+"


hpatch_inline static
hpatch_FileHandle _import_fileOpen(const char* fileName_utf8,_FileModeType mode){
#ifdef HPATCH_FILE_USE_FILESYSTEM
    return ff_fopen(fileName_utf8,mode);
#else
	int filetype;
	hpatch_FileHandle file = pvPortMalloc(sizeof(DEV_FILE));
	if (file) {
		file->filetype = -1;
		if (!strcmp(fileName_utf8, "patch")) {
			assert (patchFileSize != 0);
			file->filetype = UPFILE_TYPE_NUM;
			file->newfile = 0;
			file->offset = OTA_MEDIA_OFFSET;
			file->size = patchFileSize;
			file->pos = 0;
			file->sflash = sfud_get_device(0);
#ifdef HPATCH_FILE_CHECK_DEBUG
			file->filebuf = pvPortMalloc(file->size);
#endif
			return file;
		} else if (!strncmp(fileName_utf8, "new", 3)) {
			for (filetype = 0; filetype < UPFILE_TYPE_NUM; filetype++) {
				if (!strcmp(hpatch_NofsFileName[filetype], fileName_utf8 + 3)) {
					file->filetype = filetype;
					file->newfile = 1;
					break;
				}
			}
		} else {
			for (filetype = 0; filetype < UPFILE_TYPE_NUM; filetype++) {
				if (!strcmp(hpatch_NofsFileName[filetype], fileName_utf8)) {
					file->filetype = filetype;
					file->newfile = 0;
					break;
				}
			}
		}
		if (file->filetype >= 0) {
			file->offset = get_upfile_offset(file->filetype, file->newfile);
			if (file->newfile)
				file->size = 0;
			else
				file->size = get_upfile_size(filetype);
			file->pos = 0;
			file->sflash = sfud_get_device(0);
#ifdef HPATCH_FILE_CHECK_DEBUG
			if (!file->newfile)
				file->filebuf = pvPortMalloc(file->size);
#endif
			return file;
		} else {
			vPortFree(file);
			return NULL;
		}
	}
	return NULL;
#endif
}

static hpatch_FileHandle _import_fileOpenWithSize(const char* fileName_utf8,_FileModeType mode,hpatch_StreamPos_t* out_fileSize){
    hpatch_FileHandle file=0;
    file=_import_fileOpen(fileName_utf8,mode);
    if ((out_fileSize==0)||(file==0)) return file;

    if (_import_fileSeek64(file,0,SEEK_END)){
        if (_import_fileTell64(file,out_fileSize)){
            if (_import_fileSeek64(file,0,SEEK_SET)){
                return file;
            }
        }
    }

    //error clear
    _import_fileClose(&file);
    return 0;
}

static hpatch_inline
hpatch_BOOL _import_fileOpenRead(const char* fileName_utf8,hpatch_FileHandle* out_fileHandle,
                                 hpatch_StreamPos_t* out_fileSize){
    hpatch_FileHandle file=0;
    assert(out_fileHandle!=0);
    if (out_fileHandle==0) { _set_errno_new(EINVAL); return hpatch_FALSE; }
    file=_import_fileOpenWithSize(fileName_utf8,_kFileReadMode,out_fileSize);
    if (file==0) return hpatch_FALSE;
    *out_fileHandle=file;
    return hpatch_TRUE;
}

static hpatch_inline
hpatch_BOOL _import_fileOpenCreateOrReWrite(const char* fileName_utf8,hpatch_FileHandle* out_fileHandle){
    hpatch_FileHandle file=0;
    assert(out_fileHandle!=0);
    if (out_fileHandle==0) { _set_errno_new(EINVAL); return hpatch_FALSE; }
    file=_import_fileOpen(fileName_utf8,_kFileWriteMode);
    if (file==0) return hpatch_FALSE;
    *out_fileHandle=file;
    return hpatch_TRUE;
}

static
hpatch_BOOL _import_fileReopenWrite(const char* fileName_utf8,hpatch_FileHandle* out_fileHandle,
                                    hpatch_StreamPos_t* out_curFileWritePos){
    hpatch_FileHandle file=0;
    hpatch_StreamPos_t curFileSize=0;
    assert(out_fileHandle!=0);
    if (out_fileHandle==0) { _set_errno_new(EINVAL); return hpatch_FALSE; }
    file=_import_fileOpenWithSize(fileName_utf8,_kFileReadWriteMode,&curFileSize);
    if (file==0) return hpatch_FALSE;
    if (out_curFileWritePos!=0) *out_curFileWritePos=curFileSize;
    if (!_import_fileSeek64To(file,curFileSize))
        { _import_fileClose_No_errno(&file); return hpatch_FALSE; }
    *out_fileHandle=file;
    return hpatch_TRUE;
}


#define _ferr_return()          { _update_ferr(self->fileError);   return hpatch_FALSE; }
#define _ferr_returnv(v)        { _update_ferrv(self->fileError,v); return hpatch_FALSE; }
#define _rw_ferr_return()       { self->m_fpos=hpatch_kNullStreamPos; _ferr_return(); }
#define _rw_ferr_returnv(v)     { self->m_fpos=hpatch_kNullStreamPos; _ferr_returnv(v); }

    static hpatch_BOOL _TFileStreamInput_read_file(const hpatch_TStreamInput* stream,hpatch_StreamPos_t readFromPos,
                                                   TByte* out_data,TByte* out_data_end){
        size_t readLen;
        hpatch_TFileStreamInput* self=(hpatch_TFileStreamInput*)stream->streamImport;
        assert(out_data<=out_data_end);
        readLen=(size_t)(out_data_end-out_data);
        if (readLen==0) return hpatch_TRUE;
        if ((readLen>self->base.streamSize)
            ||(readFromPos>self->base.streamSize-readLen)) _ferr_returnv(EFBIG);
        if (self->m_fpos!=readFromPos+self->m_offset){
            if (!_import_fileSeek64To(self->m_file,readFromPos+self->m_offset)) _rw_ferr_return();
        }
        if (!_import_fileRead(self->m_file,out_data,out_data+readLen)) _rw_ferr_return();
        self->m_fpos=readFromPos+self->m_offset+readLen;
        return hpatch_TRUE;
    }

hpatch_BOOL hpatch_TFileStreamInput_open(hpatch_TFileStreamInput* self,const char* fileName_utf8){
    assert(self->m_file==0);
    self->fileError=hpatch_FALSE;
    if (self->m_file) _ferr_returnv(EINVAL);
    if (!_import_fileOpenRead(fileName_utf8,&self->m_file,&self->base.streamSize))
        _ferr_return();

    self->base.streamImport=self;
    self->base.read=_TFileStreamInput_read_file;
    self->m_fpos=0;
    self->m_offset=0;
    return hpatch_TRUE;
}

hpatch_BOOL hpatch_TFileStreamInput_setOffset(hpatch_TFileStreamInput* self,hpatch_StreamPos_t offset){
    if (self->base.streamSize<offset)
        _ferr_returnv(EFBIG);
    self->m_offset+=offset;
    self->base.streamSize-=offset;
    return hpatch_TRUE;
}

hpatch_BOOL hpatch_TFileStreamInput_close(hpatch_TFileStreamInput* self){
    if (!_import_fileClose(&self->m_file)) _ferr_return();
    return hpatch_TRUE;
}


    static hpatch_BOOL _TFileStreamOutput_write_file(const hpatch_TStreamOutput* stream,hpatch_StreamPos_t writeToPos,
                                                     const TByte* data,const TByte* data_end){
        size_t writeLen;
        hpatch_TFileStreamOutput* self=(hpatch_TFileStreamOutput*)stream->streamImport;
        assert(data<=data_end);
        assert(self->m_offset==0);
        writeLen=(size_t)(data_end-data);
        if (writeLen==0) return hpatch_TRUE;
        if ((writeLen>self->base.streamSize)
            ||(writeToPos>self->base.streamSize-writeLen)) _ferr_returnv(EFBIG);
        if (self->is_in_readModel){
            self->is_in_readModel=hpatch_FALSE;
            self->m_fpos=hpatch_kNullStreamPos;
        }
        if (writeToPos!=self->m_fpos){
            if (self->is_random_out){
                if (!_import_fileFlush(self->m_file)) _rw_ferr_return(); //for lseek64 safe
                if (!_import_fileSeek64To(self->m_file,writeToPos)) _rw_ferr_return();
                self->m_fpos=writeToPos;
            }else{
                _ferr_returnv(ERANGE); //must continue write at self->m_fpos
            }
        }
        if (!_import_fileWrite(self->m_file,data,data+writeLen)) _rw_ferr_return();
        self->m_fpos=writeToPos+writeLen;
        self->out_length=(self->out_length>=self->m_fpos)?self->out_length:self->m_fpos;
        return hpatch_TRUE;
    }
    static hpatch_BOOL _hpatch_TFileStreamOutput_read_file(const hpatch_TStreamOutput* stream,
                                                           hpatch_StreamPos_t readFromPos,
                                                           TByte* out_data,TByte* out_data_end){
         //hpatch_TFileStreamOutput is A hpatch_TFileStreamInput !
        hpatch_TFileStreamOutput* self=(hpatch_TFileStreamOutput*)stream->streamImport;
        const hpatch_TStreamInput* in_stream=(const hpatch_TStreamInput*)stream;
        if (!self->is_in_readModel){
            if (!hpatch_TFileStreamOutput_flush(self)) _rw_ferr_return();
            self->is_in_readModel=hpatch_TRUE;
            self->m_fpos=hpatch_kNullStreamPos;
        }
        return _TFileStreamInput_read_file(in_stream,readFromPos,out_data,out_data_end);
    }
hpatch_BOOL hpatch_TFileStreamOutput_open(hpatch_TFileStreamOutput* self,const char* fileName_utf8,
                                          hpatch_StreamPos_t max_file_length){
    assert(self->m_file==0);
    self->fileError=hpatch_FALSE;
    if (self->m_file) _ferr_returnv(EINVAL);
    if (!_import_fileOpenCreateOrReWrite(fileName_utf8,&self->m_file))
        _ferr_return();

    self->base.streamImport=self;
    self->base.streamSize=max_file_length;
    self->base.read_writed=_hpatch_TFileStreamOutput_read_file;
    self->base.write=_TFileStreamOutput_write_file;
    self->m_fpos=0;
    self->m_offset=0;
    self->is_in_readModel=hpatch_FALSE;
    self->is_random_out=hpatch_FALSE;
    self->out_length=0;
    return hpatch_TRUE;
}
hpatch_BOOL hpatch_TFileStreamOutput_reopen(hpatch_TFileStreamOutput* self,const char* fileName_utf8,
                                            hpatch_StreamPos_t max_file_length){
    hpatch_StreamPos_t curFileWritePos=0;
    assert(self->m_file==0);
    self->fileError=hpatch_FALSE;
    if (self->m_file) _ferr_returnv(EINVAL);
    if (!_import_fileReopenWrite(fileName_utf8,&self->m_file,&curFileWritePos))
       _ferr_return();
    if (curFileWritePos>max_file_length){
        //note: now not support reset file length to max_file_length
        _import_fileClose(&self->m_file);
        _ferr_returnv(EFBIG);
    }
    self->base.streamImport=self;
    self->base.streamSize=max_file_length;
    self->base.read_writed=_hpatch_TFileStreamOutput_read_file;
    self->base.write=_TFileStreamOutput_write_file;
    self->m_fpos=curFileWritePos;
    self->m_offset=0;
    self->is_in_readModel=hpatch_FALSE;
    self->is_random_out=hpatch_FALSE;
    self->out_length=curFileWritePos;
    return hpatch_TRUE;
}

/*
hpatch_BOOL hpatch_TFileStreamOutput_truncate(hpatch_TFileStreamOutput* self,hpatch_StreamPos_t new_file_length){
    if (!_import_fileTruncate(self->m_file,new_file_length))
        _ferr_return();
    return hpatch_TRUE;
}*/

hpatch_BOOL hpatch_TFileStreamOutput_flush(hpatch_TFileStreamOutput* self){
    if (!_import_fileFlush(self->m_file))
        _ferr_return();
    return hpatch_TRUE;
}

hpatch_BOOL hpatch_TFileStreamOutput_close(hpatch_TFileStreamOutput* self){
    if (!_import_fileClose(&self->m_file))
        _ferr_return();
    return hpatch_TRUE;
}
