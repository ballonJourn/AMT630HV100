#ifndef _SYSINFO_H
#define _SYSINFO_H

#define UPDATE_MEDIA_SD			0
#define UPDATE_MEDIA_USB		1
#define UPDATE_MEDIA_UART		2
#define UPDATE_MEDIA_WIFI		3

#define UPDATE_STATUS_START		0
#define UPDATE_STATUS_END		1


typedef struct {
	unsigned int app_checksum;
	unsigned int stepldr_offset;
	unsigned int stepldr_size;
	unsigned int update_media_type;
	unsigned int update_status;
	unsigned int app_size;
	unsigned int image_offset;
	unsigned int loader_offset;
	unsigned int loader_size;
	unsigned int reserved[12];
	unsigned int checksum;
} SysInfo;


SysInfo *GetSysInfo(void);
int ReadSysInfo(void);
void SaveSysInfo(void);


#endif
