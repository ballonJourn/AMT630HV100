#ifndef __MFCAPI_H__
#define __MFCAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dwl.h"

#define RAW_STRM_TYPE_H264				104
#define RAW_STRM_TYPE_JPEG				110
#define RAW_STRM_TYPE_H264_NOREORDER	(RAW_STRM_TYPE_H264 | (1 << 8))

#define MAX_OUTFRAME_NUM	8

#define MAX_OUTFRAME_WIDTH	1280
#define MAX_OUTFRAME_HEIGHT	720

typedef struct
{
	u32 yBusAddress;
	u32 cbcrBusAddress;
	u32 crBusAddress;
	const void *pyVirAddress;
	const void *pcbcrVirAddress;
	const void *pcrVirAddress;
	i32 keyPicture;
}FrameBuffer;

typedef struct
{
	i32 num;
	i32 codedWidth;
	i32 codedHeight;
	i32 frameWidth;
	i32 frameHeight;
	i32 outputFormat;   /* JPEGDEC_YCbCr400/JPEGDEC_YCbCr420/JPEGDEC_YCbCr422 */
	FrameBuffer buffer[MAX_OUTFRAME_NUM];
}OutFrameBuffer;

typedef enum{
	MFCDEC_RET_OK = 1,
	MFCDEC_RET_DECFAIL = 0,
	MFCDEC_RET_MEMFAIL = -1,
	MFCDEC_RET_LOCKFAIL = -2,
}MFCDecodeRet;

typedef struct
{
	void *decInst;
	int streamType;
	int delayInit;
	int outputFormat;
}MFCHandle;

typedef struct {
	MFCHandle *handle;
	uint32_t jpg_addr;
	uint32_t jpg_size;
	uint32_t dec_addry;
	uint32_t dec_addru;
	uint32_t dec_addrv;
	uint32_t dec_size;
	uint16_t jpg_width;
	uint16_t jpg_height;
	uint16_t dec_width;
	uint16_t dec_height;
	uint32_t dec_format;
} JpegHeaderInfo;


MFCHandle *mfc_init(int streamType);

int mfc_decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer);

int mfc_jpegdec(JpegHeaderInfo *jpegInfo);

void mfc_uninit(MFCHandle *handle);

int mfc_pp_init(MFCHandle *handle, int outWidth, int outHeight, int outFormat);

#ifdef __cplusplus
}
#endif

#endif
