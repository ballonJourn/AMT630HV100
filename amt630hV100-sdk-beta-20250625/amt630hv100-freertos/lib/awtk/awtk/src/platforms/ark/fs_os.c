#include "tkc/fs.h"
#include "tkc/mem.h"

#ifdef HMI_AWTK

#ifdef USE_FREERTOSFAT_FS
#include "ff_stdio.h"

static int32_t fs_os_file_read(fs_file_t* file, void* buffer, uint32_t size) {
  FF_FILE* fp = (FF_FILE*)(file->data);

  return (int32_t)ff_fread(buffer, 1, size, fp);
}

static int32_t fs_os_file_write(fs_file_t* file, const void* buffer, uint32_t size) {
  FF_FILE* fp = (FF_FILE*)(file->data);

  return ff_fwrite(buffer, 1, size, fp);
}

static int32_t fs_os_file_printf(fs_file_t* file, const char* const format_str, va_list vl) {
  FF_FILE* fp = (FF_FILE*)(file->data);

  return ff_fprintf (fp, format_str, vl);
}

static ret_t fs_os_file_seek(fs_file_t* file, int32_t offset) {
  FF_FILE* fp = (FF_FILE*)(file->data);

  return ff_fseek(fp, offset, FF_SEEK_SET) == 0 ? RET_OK : RET_FAIL;
}

static int64_t fs_os_file_tell(fs_file_t* file) {
  FF_FILE* fp = (FF_FILE*)(file->data);

  return ff_ftell(fp);
}

static int64_t fs_os_file_size(fs_file_t* file) {
  FF_FILE* fp = (FF_FILE*)(file->data);
  return ff_filelength (fp);  
}


static bool_t fs_os_file_eof(fs_file_t* file) {
  FF_FILE* fp = (FF_FILE*)(file->data);

  return ff_feof(fp) != 0;
}

static ret_t fs_os_file_close(fs_file_t* file) {
  FF_FILE* fp = (FF_FILE*)(file->data);
  ff_fclose(fp);
  TKMEM_FREE(file);

  return RET_OK;
}

static const fs_file_vtable_t s_file_vtable = {.read = fs_os_file_read,
                                               .write = fs_os_file_write,
															.printf = fs_os_file_printf,  
                                               .seek = fs_os_file_seek,
                                               .tell = fs_os_file_tell,
                                               .size = fs_os_file_size,
                                               .eof = fs_os_file_eof,
                                               .close = fs_os_file_close};
                                               
static fs_file_t* fs_file_create(FF_FILE* fp) {
  fs_file_t* f = NULL;
  return_value_if_fail(fp != NULL, NULL);

  f = TKMEM_ZALLOC(fs_file_t);
  if (f != NULL) {
    f->vt = &s_file_vtable;
    f->data = fp;
  } else {
    ff_fclose(fp);
  }

  return f;
}

static fs_file_t* fs_os_open_file(fs_t* fs, const char* name, const char* mode) {
  (void)fs;
  return_value_if_fail(name != NULL && mode != NULL, NULL);
  return fs_file_create(ff_fopen(name, mode));
}

static ret_t fs_os_remove_file(fs_t* fs, const char* name) {
  (void)fs;
  return_value_if_fail(name != NULL, RET_FAIL);

  if(ff_remove (name) == 0)
  	 return RET_OK;
  else
    return RET_FAIL;
}

static ret_t fs_os_remove_dir(fs_t* fs, const char* name) {
  (void)fs;
  return_value_if_fail(name != NULL, RET_FAIL);
  if (ff_rmdir(name) == 0) {
    return RET_OK;
  } else {
    return RET_FAIL;
  }
}

static ret_t fs_os_create_dir(fs_t* fs, const char* name) {
  (void)fs;
  return_value_if_fail(name != NULL, RET_FAIL);

  if (ff_mkdir(name) == 0) {
    return RET_OK;
  } else {
     return RET_FAIL;
  }
}

static bool_t fs_os_file_exist(fs_t* fs, const char* name) {
  FF_Stat_t stat;
  return_value_if_fail(name != NULL, FALSE);
  if(ff_stat(name, &stat) == -1)
  	 return FALSE;
  if(stat.st_mode == FF_IFREG)
    return TRUE;
  return FALSE;
}

static int32_t fs_os_get_file_size(fs_t* fs, const char* name) {
  FF_Stat_t st;
  return_value_if_fail(name != NULL, FALSE);

  if (ff_stat(name, &st) != -1) {
    return st.st_size;
  } else {
    return 0;
  }
}

static ret_t fs_os_file_rename(fs_t* fs, const char* name, const char* new_name) {
  (void)fs;
  return_value_if_fail(name != NULL && new_name != NULL, RET_BAD_PARAMS);

  return ff_rename(name, new_name, 0) == 0 ? RET_OK : RET_FAIL;
}

static bool_t fs_os_dir_exist(fs_t* fs, const char* name) {
  FF_Stat_t st;
  return_value_if_fail(name != NULL, FALSE);

  if (ff_stat(name, &st) == RET_OK) {
    if(st.st_mode == FF_IFDIR)
		 return TRUE;
	 else
		  return FALSE;
		 
  } else {
     return FALSE;
  }
}

static const fs_t s_os_fs = {.open_file = fs_os_open_file,
                             .remove_file = fs_os_remove_file,
                             .file_exist = fs_os_file_exist,
									  .file_rename = fs_os_file_rename,
									  .get_file_size = fs_os_get_file_size,
									  .dir_exist = fs_os_dir_exist,
									  .remove_dir = fs_os_remove_dir,
									  .create_dir = fs_os_create_dir,
									
                             };

fs_t* os_fs(void) {
  return (fs_t*)&s_os_fs;
}

#elif USE_LITTLEFS_FS
#include "lfs.h"
#include "sfud.h"
#include "updatefile.h"

typedef struct {
	unsigned int magic;
	unsigned int checksum;
	struct lfs_config cfg;
} LFSImageHeader;

static sfud_flash *sflash;
static uint32_t	startblk;
// variables used by the filesystem
static lfs_t lfs;
// configuration of the filesystem is provided by this struct
static struct lfs_config cfg;
static int lfs_init = 0;

static int32_t fs_os_file_read(fs_file_t* file, void* buffer, uint32_t size) {
	lfs_file_t* fp = (lfs_file_t*)(file->data);
  	return lfs_file_read(&lfs, fp, buffer, size);
}

/* static int32_t fs_os_file_write(fs_file_t* file, const void* buffer, uint32_t size) {
	lfs_file_t* fp = (lfs_file_t*)(file->data);
	return 0;
}

static int32_t fs_os_file_printf(fs_file_t* file, const char* const format_str, va_list vl) {
	lfs_file_t* fp = (lfs_file_t*)(file->data);
	return 0;
} */

static ret_t fs_os_file_seek(fs_file_t* file, int32_t offset) {
	lfs_file_t* fp = (lfs_file_t*)(file->data);
	return lfs_file_seek(&lfs, fp, offset, LFS_SEEK_SET) == offset ? RET_OK : RET_FAIL;
}

static int64_t fs_os_file_tell(fs_file_t* file) {
	lfs_file_t* fp = (lfs_file_t*)(file->data);
	return lfs_file_tell(&lfs, fp);
}

static int64_t fs_os_file_size(fs_file_t* file) {
	lfs_file_t* fp = (lfs_file_t*)(file->data);
  	return lfs_file_size(&lfs, fp);
}

static bool_t fs_os_file_eof(fs_file_t* file) {
	return fs_os_file_tell(file) == fs_os_file_size(file);
}

static ret_t fs_os_file_close(fs_file_t* file) {
	lfs_file_t* fp = (lfs_file_t*)(file->data);
	lfs_file_close(&lfs, fp);
	TKMEM_FREE(fp);
	TKMEM_FREE(file);

	return RET_OK;
}

static const fs_file_vtable_t s_file_vtable = {
	.read = fs_os_file_read,
	/* .write = fs_os_file_write,
	.printf = fs_os_file_printf, */  
	.seek = fs_os_file_seek,
	.tell = fs_os_file_tell,
	.size = fs_os_file_size,
	.eof = fs_os_file_eof,
	.close = fs_os_file_close
};
                                               
static fs_file_t* fs_file_create(lfs_file_t* fp) {
	fs_file_t* f = NULL;
	return_value_if_fail(fp != NULL, NULL);

	f = TKMEM_ZALLOC(fs_file_t);
	if (f != NULL) {
		f->vt = &s_file_vtable;
		f->data = fp;
	} else {
		lfs_file_close(&lfs, fp);
		TKMEM_FREE(fp);
	}

	return f;
}

static fs_file_t* fs_os_open_file(fs_t* fs, const char* name, const char* mode) {
	(void)fs;
	return_value_if_fail(name != NULL && mode != NULL, NULL);
	lfs_file_t *file = TKMEM_ZALLOC(lfs_file_t);
#ifndef LFS_READONLY	
	if (lfs_file_open(&lfs, file, name, LFS_O_RDWR))
#else
	if (lfs_file_open(&lfs, file, name, LFS_O_RDONLY))
#endif
		return NULL;
	else
		return fs_file_create(file);
}

static bool_t fs_os_file_exist(fs_t* fs, const char* name) {
	return_value_if_fail(name != NULL, FALSE);

	lfs_file_t file;
  	if (!lfs_file_open(&lfs, &file, name, LFS_O_RDONLY)) {
		lfs_file_close(&lfs, &file);
		return TRUE;
	} else {
		return FALSE;
	}
}

static int32_t fs_os_get_file_size(fs_t* fs, const char* name) {
	return_value_if_fail(name != NULL, FALSE);

	lfs_file_t file;
	if (!lfs_file_open(&lfs, &file, name, LFS_O_RDONLY)) {
		int filesize = lfs_file_size(&lfs, &file);
		lfs_file_close(&lfs, &file);
		return filesize;
	} else {
		return 0;
	}
}

static bool_t fs_os_dir_exist(fs_t* fs, const char* name) {
	return_value_if_fail(name != NULL, FALSE);

	lfs_dir_t dir;
	if (!lfs_dir_open(&lfs, &dir, name)) {
		lfs_dir_close(&lfs, &dir);
		return TRUE;
	} else {
		return FALSE;
	}
}

static const fs_t s_os_fs = {
	.open_file = fs_os_open_file,
	.file_exist = fs_os_file_exist,
	.get_file_size = fs_os_get_file_size,
	.dir_exist = fs_os_dir_exist,
};

static int _lfs_flash_read(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, void *buffer, lfs_size_t size)
{
	block += startblk;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	if (sfud_read(sflash, block * c->block_size + off, size, buffer) == SFUD_SUCCESS)
		return LFS_ERR_OK;
	else
		return LFS_ERR_IO;
#else
	if (emmc_read(block * c->block_size + off, size, buffer) == SFUD_SUCCESS)
		return LFS_ERR_OK;
	else
		return LFS_ERR_IO;
#endif
}

static int _lfs_flash_prog(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size)
{
	block += startblk;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	if (sfud_write(sflash, block * c->block_size + off, size, buffer) == SFUD_SUCCESS)
		return LFS_ERR_OK;
	else
		return LFS_ERR_IO;
#endif
}

static int _lfs_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
#if DEVICE_TYPE_SELECT == EMMC_FLASH
	return LFS_ERR_OK;
#endif
	block += startblk;
	if (sfud_erase(sflash, block * c->block_size, c->block_size) == SFUD_SUCCESS)
		return LFS_ERR_OK;
	else
		return LFS_ERR_IO;
}

static int _lfs_flash_sync(const struct lfs_config* c)
{
    return LFS_ERR_OK;
}

fs_t* os_fs(void) {
	if (!lfs_init) {
		uint32_t offset;
		LFSImageHeader header;

		sflash = sfud_get_device(0);
		offset = GetUpFileOffset(MKTAG('R', 'O', 'M', 'A'));
#if DEVICE_TYPE_SELECT != EMMC_FLASH
		sfud_read(sflash, offset, sizeof(header), (void*)&header);
#else
		emmc_read(offset, sizeof(header), (void*)&header);
#endif
		memcpy(&cfg, &header.cfg, sizeof(struct lfs_config));
		startblk = offset / cfg.block_size + 1;
		cfg.read = _lfs_flash_read;
		cfg.prog = _lfs_flash_prog;
		cfg.erase = _lfs_flash_erase;
		cfg.sync = _lfs_flash_sync;
		if(lfs_mount(&lfs, &cfg))
			return NULL;
		//lfs_file_t file;
		//int ret = lfs_file_open(&lfs, &file, "./assets.inc", LFS_O_RDWR);
		//printf("ret=%d.filesize=%d.\n", ret, lfs_file_size(&lfs, &file));
		lfs_init = 1;
	}
	
	return (fs_t*)&s_os_fs;
}

#elif USE_ROMFS_FS

#include "romfile.h"

static int32_t fs_os_file_read(fs_file_t* file, void* buffer, uint32_t size) {
	RomFile* fp = (RomFile*)(file->data);
  	return RomFileRead(fp, buffer, size);
}

static ret_t fs_os_file_seek(fs_file_t* file, int32_t offset) {
	RomFile* fp = (RomFile*)(file->data);
	return RomFileSeek(fp, offset, SEEK_SET) == offset ? RET_OK : RET_FAIL;
}

static int64_t fs_os_file_tell(fs_file_t* file) {
	RomFile* fp = (RomFile*)(file->data);
	return RomFileTell(fp);
}

static int64_t fs_os_file_size(fs_file_t* file) {
	RomFile* fp = (RomFile*)(file->data);
  	return RomFileGetSize(fp);
}

static bool_t fs_os_file_eof(fs_file_t* file) {
	return fs_os_file_tell(file) == fs_os_file_size(file);
}

static ret_t fs_os_file_close(fs_file_t* file) {
	RomFile* fp = (RomFile*)(file->data);
	RomFileClose(fp);
	TKMEM_FREE(file);

	return RET_OK;
}

static const fs_file_vtable_t s_file_vtable = {
	.read = fs_os_file_read,
	.seek = fs_os_file_seek,
	.tell = fs_os_file_tell,
	.size = fs_os_file_size,
	.eof = fs_os_file_eof,
	.close = fs_os_file_close
};
                                               
static fs_file_t* fs_file_create(RomFile* fp) {
	fs_file_t* f = NULL;
	return_value_if_fail(fp != NULL, NULL);

	f = TKMEM_ZALLOC(fs_file_t);
	if (f != NULL) {
		f->vt = &s_file_vtable;
		f->data = fp;
	} else {
		RomFileClose(fp);
	}

	return f;
}

static fs_file_t* fs_os_open_file(fs_t* fs, const char* name, const char* mode) {
	(void)fs;
	return_value_if_fail(name != NULL && mode != NULL, NULL);
	if (name[0] == '.' && name[1] == '/')
		name += 2;
	return fs_file_create(RomFileOpen(name));
}

static bool_t fs_os_file_exist(fs_t* fs, const char* name) {
	return_value_if_fail(name != NULL, FALSE);
	if (name[0] == '.' && name[1] == '/')
		name += 2;
	return RomFileExist(name);
}

static int32_t fs_os_get_file_size(fs_t* fs, const char* name) {
	return_value_if_fail(name != NULL, FALSE);
	if (name[0] == '.' && name[1] == '/')
		name += 2;
	RomFile *file = RomFileOpen(name);
	if (file) {
		int filesize = RomFileGetSize(file);
		RomFileClose(file);
		return filesize;
	} else {
		return 0;
	}
}

static bool_t fs_os_dir_exist(fs_t* fs, const char* name) {
	return_value_if_fail(name != NULL, FALSE);
	if (name[0] == '.' && name[1] == '/')
		name += 2;	
	return RomFileDirExist(name);
}

static const fs_t s_os_fs = {
	.open_file = fs_os_open_file,
	.file_exist = fs_os_file_exist,
	.get_file_size = fs_os_get_file_size,
	.dir_exist = fs_os_dir_exist,
};

fs_t* os_fs(void) {
	return (fs_t*)&s_os_fs;
}

#endif

#endif
