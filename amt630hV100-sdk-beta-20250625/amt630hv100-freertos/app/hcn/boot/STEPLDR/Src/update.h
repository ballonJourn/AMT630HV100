#ifndef _UPDATE_H_
#define _UPDATE_H_

#define UPFILE_APP_MAGIC	0xe59ff030
#define APPLDR_CHECKSUM_OFFSET	20

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
	unsigned int magic;
	int hasBootlogo;
	int bootlogoDisplayTime;
	int aniCount;
	int aniWidth;
	int aniHeight;
	int aniFps;
	int aniDelayHideTime;
	unsigned int aniSize;
	unsigned int checksum;
	unsigned int reserved[2];
}BANIHEADER;

typedef enum {
	UPFILE_TYPE_WHOLE,
	UPFILE_TYPE_RESOURCE,
	UPFILE_TYPE_ANIMATION,
	UPFILE_TYPE_APP,
	UPFILE_TYPE_FIRSTLDR,
	UPFILE_TYPE_STEPLDR,
	UPFILE_TYPE_LNCHEMMC,
} eUpfileType;

#define MKTAG(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((unsigned)(d) << 24))

void update_logo_init(void);
void update_progress_set(int percent);

#endif
