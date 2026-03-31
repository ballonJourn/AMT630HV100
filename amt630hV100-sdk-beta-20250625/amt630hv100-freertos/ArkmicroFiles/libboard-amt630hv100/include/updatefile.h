#ifndef _UPDATEFILE_H
#define _UPDATEFILE_H

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))

typedef struct {
	unsigned int magic;
	unsigned int offset;
	unsigned int size;
} UpFileInfo;

typedef struct {
	unsigned int magic;
	unsigned int filenum;
	unsigned int size;
	unsigned int checksum;
	unsigned int reserved1;
	unsigned int reserved2;
	UpFileInfo files[];
} UpFileHeader;

int GetUpFileInfo(void);
uint32_t GetUpFileOffset(uint32_t magic);
uint32_t GetUpFileSize(uint32_t magic);

#endif
