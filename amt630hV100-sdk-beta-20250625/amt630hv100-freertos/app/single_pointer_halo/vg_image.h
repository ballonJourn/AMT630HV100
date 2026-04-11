#ifndef VG_IMAGE_H
#define VG_IMAGE_H

#define	VG_IMAGE_MAGIC		0x4D424756		// "VGBM"



typedef struct _VG_IMAGE_HEADER {
	unsigned int  magic;			// VG_IMAGE_MAGIC
	unsigned int  format;			// VG图像格式
	int  width;			// 像素宽度
	int  height;		// 像素高度
	int  stride;		// 每行字节长度
	unsigned int  checksum;		// checksum = 0xAA55AA55 - (magic + type + width + height + stride ) 
	unsigned int  rev[16 - 6];	// 保留字段, 全为0
} VG_IMAGE_HEADER;

#define	VG_IMAGE_DATA(image)		(((unsigned char *)(image)) + 64)	
#define	VG_IMAGE_FORMAT(image)		((VGImageFormat)(((VG_IMAGE_HEADER *)(image))->format))
#define	VG_IMAGE_WIDTH(image)		(((VG_IMAGE_HEADER *)(image))->width)
#define	VG_IMAGE_HEIGHT(image)		(((VG_IMAGE_HEADER *)(image))->height)
#define	VG_IMAGE_STRIDE(image)		(((VG_IMAGE_HEADER *)(image))->stride)

#endif	// #ifndef VG_IMAGE_H
