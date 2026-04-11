#ifndef _ROMFILE_H
#define _ROMFILE_H

#define ROMFILE_NAME_MAX_LEN	64

typedef struct {
	char name[ROMFILE_NAME_MAX_LEN];
	unsigned int offset;
	unsigned int size;
} RomFileInfo;

typedef struct {
	unsigned int magic;
	unsigned int filenum;
	unsigned int romsize;
	unsigned int checksum;
	RomFileInfo files[];
} RomHeader;

typedef struct {
	char name[ROMFILE_NAME_MAX_LEN];
	void *buf;
	int cached_filenum;
	int life;
} RomFileCache;

typedef struct {
	void *buf;
	int index;
	int size;
	int pos;
	RomFileCache *cache;
} RomFile;

int ReadRomFile(void);
RomFile *RomFileOpen(const char *name);
size_t RomFileRead(RomFile *file, void *buf, size_t size);
int RomFileSeek(RomFile *file, int offset, int whence);
void RomFileClose(RomFile *file);
const char * RomFileGetExt(const char * path);
int RomFileTell(RomFile *file);
int RomFileGetSize(RomFile *file);
int RomFileExist(const char *name);
int RomFileDirExist(const char *name);

#endif
