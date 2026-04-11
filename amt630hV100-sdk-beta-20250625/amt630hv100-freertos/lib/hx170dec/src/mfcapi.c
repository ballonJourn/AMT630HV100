#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "dwl.h"
#include "h264decapi.h"
#include "jpegdecapi.h"
#include "jpegdeccontainer.h"
#include "mfcapi.h"
#include "mmu.h"
#include "include/amt630hv100.h"
#include "sysctl.h"
#include "semphr.h"

#define JPEG_BUF_COUNTS		2

static u32 g_picDecodeNumber = 0;
static u32 g_JpegYBusAddress = 0;
static u32 g_JpegCbCrBusAddress = 0;
static DWLLinearMem_t g_JpegYMemInfo[JPEG_BUF_COUNTS] = {0};
static u32 g_JpegMemIndex = 0;

static void MFCSwitchIramMode(int en)
{
#if 0
	static SemaphoreHandle_t MfcIramMutex = NULL;
	if(!MfcIramMutex)
		MfcIramMutex = xSemaphoreCreateMutex();

	if(MfcIramMutex)
		xSemaphoreTake(MfcIramMutex, portMAX_DELAY);

	//0 MFC ram use as iram; 1 MFC ram use MFC REF buffer
	u32 val = readl(REGS_SYSCTL_BASE + 0x78);
	if(en)
		val |= (1<<0);
	else
		val &= ~(1<<0);
	writel(val, REGS_SYSCTL_BASE + 0x78);

	if(MfcIramMutex)
		xSemaphoreGive(MfcIramMutex);
#else
	portENTER_CRITICAL();
	//0 MFC ram use as iram; 1 MFC ram use MFC REF buffer
	u32 val = readl(REGS_SYSCTL_BASE + 0x78);
	if(en)
		val |= (1<<0);
	else
		val &= ~(1<<0);
	writel(val, REGS_SYSCTL_BASE + 0x78);
	portEXIT_CRITICAL();
#endif
}

int MFCH264Decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	H264DecRet ret;
	H264DecRet picRet;
	H264DecInput decIn;
	H264DecOutput decOut;
	static H264DecInfo decInfo;
	H264DecPicture decPic;

	decIn.dataLen = inBuffer->size;
	decIn.pStream = (u8*)inBuffer->virtualAddress;
	decIn.streamBusAddress = inBuffer->busAddress;

	do
	{
		/* Picture ID is the picture number in decoding order */
		decIn.picId = g_picDecodeNumber;
		/* decode the stream data */
		ret = H264DecDecode(handle->decInst, &decIn, &decOut);
		switch(ret)
		{
		case H264DEC_HDRS_RDY:
			/* read stream info */
			H264DecGetInfo(handle->decInst, &decInfo);
			outBuffer->codedWidth = decInfo.picWidth;
			outBuffer->codedHeight = decInfo.picHeight;
			break;

		case H264DEC_ADVANCED_TOOLS:
			/* Arbitrary Slice Order/multiple slice groups noticed */
			/* in the stream. The decoder has to reallocate resources */
			break;

		case H264DEC_PIC_DECODED: /* a picture was decoded */
		case H264DEC_PENDING_FLUSH:
			g_picDecodeNumber++;

			do {
				if(outBuffer->num >= MAX_OUTFRAME_NUM)
					return 0;
    			picRet = H264DecNextPicture(handle->decInst, &decPic, 0);
    			if (picRet != H264DEC_PIC_RDY) {
    				break;
    			}
				outBuffer->buffer[outBuffer->num].keyPicture =
					decPic.isIdrPicture;
				outBuffer->buffer[outBuffer->num].yBusAddress =
					decPic.outputPictureBusAddress;
				outBuffer->buffer[outBuffer->num].pyVirAddress =
					decPic.pOutputPicture;
				outBuffer->frameWidth = decInfo.picWidth;
				outBuffer->frameHeight = decInfo.picHeight;
				outBuffer->num++;
  			} while (picRet == H264DEC_PIC_RDY);

			break;

		case H264DEC_STRM_PROCESSED:
			/* Input stream was processed but no picture is ready */
			break;

		case H264DEC_MEMFAIL:
			return -1;

		default:
			/* all other cases are errors where decoding cannot continue */
			return -1;
		}

		if(decOut.dataLeft == 0)
		{
			break;
		}
		else
		{
			/* data left undecoded */
			decIn.dataLen = decOut.dataLeft;
			decIn.pStream = decOut.pStrmCurrPos;
			decIn.streamBusAddress = decOut.strmCurrBusAddress;
		}
	}while ((ret != H264DEC_STRM_PROCESSED) && (decOut.dataLeft > 0));

	return 0;
}

int MFCJpegDecode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	JpegDecInst  decoder;
	JpegDecRet  infoRet;
	JpegDecInput DecIn;
	JpegDecImageInfo DecImgInf;
	JpegDecOutput DecOut;
	int ret = 0;
	int i;

	infoRet = JpegDecInit(&decoder);
	if(infoRet !=JPEGDEC_OK) {
		printf("JpegDecInit failure %d.\n", infoRet);
		return -1;
	}
	handle->decInst = (void*)decoder;

	DecIn.decImageType = JPEGDEC_IMAGE;
	DecIn.sliceMbSet = 0;

	DecIn.bufferSize = 0;
	DecIn.streamLength = inBuffer->size;
	DecIn.streamBuffer.busAddress= inBuffer->busAddress;
	DecIn.streamBuffer.pVirtualAddress = inBuffer->virtualAddress;

	g_JpegYBusAddress = 0;

	for (i = 0; i < JPEG_BUF_COUNTS; i++) {
		if(g_JpegYMemInfo[i].busAddress == 0)
		{
			if(DWLMallocLinear(((JpegDecContainer*)handle->decInst)->dwl, MAX_OUTFRAME_WIDTH*MAX_OUTFRAME_HEIGHT*2,
				 &g_JpegYMemInfo[i]) == DWL_ERROR)
			{
				printf("Jpeg DWLMallocLinear y failure.\n");
				goto end;
			}
		}
	}

	/* Get image information of the JFIF */
    infoRet = JpegDecGetImageInfo(handle->decInst,
                                   &DecIn,
                                   &DecImgInf);
	if(infoRet !=JPEGDEC_OK)
	{
		printf("\rJpegDecGetImageInfo failure.\n");
		goto end;
	}

	DecIn.pictureBufferY.busAddress = g_JpegYMemInfo[g_JpegMemIndex].busAddress;
	DecIn.pictureBufferY.pVirtualAddress = g_JpegYMemInfo[g_JpegMemIndex].virtualAddress;
	DecIn.pictureBufferCbCr.busAddress = g_JpegYMemInfo[g_JpegMemIndex].busAddress + DecImgInf.outputWidth * DecImgInf.outputHeight;
	DecIn.pictureBufferCbCr.pVirtualAddress = (u32*)((u32)g_JpegYMemInfo[g_JpegMemIndex].virtualAddress +
			DecImgInf.outputWidth * DecImgInf.outputHeight);
	DecIn.pictureBufferCr.busAddress = 0;
	DecIn.pictureBufferCr.pVirtualAddress = 0;

	g_JpegMemIndex = !g_JpegMemIndex;

	/* Decode JFIF */
    infoRet = JpegDecDecode(handle->decInst, &DecIn, &DecOut);
	if(infoRet != JPEGDEC_FRAME_READY)
	{
		printf("\rJpegDecDecode failure, ret=%d\n", infoRet);
		if (infoRet == JPEGDEC_MEMFAIL)
			ret = -1;
		goto end;
	}

	ret = 0;

	outBuffer->outputFormat = DecImgInf.outputFormat;
	outBuffer->codedWidth = DecImgInf.displayWidth;
	outBuffer->codedHeight = DecImgInf.displayHeight;
	outBuffer->buffer[0].keyPicture = 1;
	g_JpegYBusAddress = DecOut.outputPictureY.busAddress;
	g_JpegCbCrBusAddress = DecOut.outputPictureCbCr.busAddress;
	if (g_JpegYBusAddress) {
		outBuffer->num = 1;
		outBuffer->buffer[0].yBusAddress = g_JpegYBusAddress;
		outBuffer->buffer[0].cbcrBusAddress = g_JpegCbCrBusAddress;
		outBuffer->buffer[0].pyVirAddress = DecOut.outputPictureY.pVirtualAddress;
		outBuffer->buffer[0].pcbcrVirAddress = DecOut.outputPictureCbCr.pVirtualAddress;
		outBuffer->frameWidth = DecImgInf.outputWidth;
		outBuffer->frameHeight = DecImgInf.outputHeight;
	}

end:

	JpegDecRelease((JpegDecInst)handle->decInst);

	return ret;
}

void JpegReleaseBuffer(void)
{
    int i;

    for(i = 0; i < JPEG_BUF_COUNTS; i++) {
		if(g_JpegYMemInfo[i].virtualAddress != NULL)
			//vPortFree(g_JpegYMemInfo[i].virtualAddress - 0x10000000/4);
			vPortFree((u32 *)UNCACHED_VIRT_TO_PHT(g_JpegYMemInfo[i].virtualAddress));

		memset(&g_JpegYMemInfo[i], 0, sizeof(g_JpegYMemInfo[i]));
    }
}

int mfc_jpegdec(JpegHeaderInfo *jpegInfo)
{
	JpegDecInst  decoder;
	JpegDecRet	infoRet;
	JpegDecInput DecIn;
	JpegDecImageInfo DecImgInf;
	JpegDecOutput DecOut;
	int outputBufferSize;
	int ret = -1;

	MFCSwitchIramMode(1);

	infoRet = JpegDecInit(&decoder);
	if(infoRet !=JPEGDEC_OK) {
		printf("JpegDecInit failure %d.\n", infoRet);
		goto exit;
	}
	jpegInfo->handle->decInst = (void*)decoder;

	DecIn.decImageType = JPEGDEC_IMAGE;
	DecIn.sliceMbSet = 0;
	DecIn.bufferSize = 0;
	DecIn.streamLength = jpegInfo->jpg_size;
	DecIn.streamBuffer.busAddress= jpegInfo->jpg_addr;
	DecIn.streamBuffer.pVirtualAddress = (u32 *)PHY_TO_UNCACHED_VIRT(jpegInfo->jpg_addr);

	/* Get image information of the JFIF */
	infoRet = JpegDecGetImageInfo(jpegInfo->handle->decInst,
								   &DecIn,
								   &DecImgInf);
	if(infoRet !=JPEGDEC_OK)
	{
		printf("\rJpegDecGetImageInfo failure.\n");
		goto end;
	}

	/* Check decode buffer size */
	if ((DecImgInf.outputFormat == JPEGDEC_YCbCr420_SEMIPLANAR) || (DecImgInf.outputFormat == JPEGDEC_YCbCr411_SEMIPLANAR)) {
		outputBufferSize = DecImgInf.outputWidth * DecImgInf.outputHeight * 3 / 2;
	} else if ((DecImgInf.outputFormat == JPEGDEC_YCbCr422_SEMIPLANAR) || (DecImgInf.outputFormat == JPEGDEC_YCbCr440)) {
		outputBufferSize = DecImgInf.outputWidth * DecImgInf.outputHeight * 2;
	} else if (DecImgInf.outputFormat == JPEGDEC_YCbCr444_SEMIPLANAR) {
		outputBufferSize = DecImgInf.outputWidth * DecImgInf.outputHeight * 3;
	} else if (DecImgInf.outputFormat == JPEGDEC_YCbCr400) {
		outputBufferSize = DecImgInf.outputWidth * DecImgInf.outputHeight;
	} else {
		printf("\rJpegDecDecode failure, invalid outputFormat:0x%x.\n", DecImgInf.outputFormat);
		ret = JPEGDEC_UNSUPPORTED;
		goto end;
	}

	if (jpegInfo->dec_size < outputBufferSize) {
		printf("\rJpegDecDecode failure, invalid dec_size:0x%x in 0x%x format\n", jpegInfo->dec_size, DecImgInf.outputFormat);
		ret = JPEGDEC_INVALID_INPUT_BUFFER_SIZE;
		goto end;
	}

	DecIn.pictureBufferY.busAddress = jpegInfo->dec_addry;
	DecIn.pictureBufferY.pVirtualAddress = (u32 *)PHY_TO_UNCACHED_VIRT(jpegInfo->dec_addry);
	DecIn.pictureBufferCbCr.busAddress = DecIn.pictureBufferY.busAddress + 
		DecImgInf.outputWidth * DecImgInf.outputHeight;
	DecIn.pictureBufferCbCr.pVirtualAddress = (u32*)((u32)DecIn.pictureBufferY.pVirtualAddress +
		DecImgInf.outputWidth * DecImgInf.outputHeight);
	DecIn.pictureBufferCr.busAddress = 0;
	DecIn.pictureBufferCr.pVirtualAddress = 0;

	/* Decode JFIF */
	infoRet = JpegDecDecode(jpegInfo->handle->decInst, &DecIn, &DecOut);
	if(infoRet != JPEGDEC_FRAME_READY)
	{
		printf("\rJpegDecDecode failure, ret=%d\n", infoRet);
		if (infoRet == JPEGDEC_MEMFAIL)
			ret = -1;
		goto end;
	}

	ret = 0;

	jpegInfo->dec_format = DecImgInf.outputFormat;
	jpegInfo->jpg_width = DecImgInf.displayWidth;
	jpegInfo->jpg_height = DecImgInf.displayHeight;
	if((jpegInfo->dec_format == JPEGDEC_YCbCr420_SEMIPLANAR) || (jpegInfo->dec_format == JPEGDEC_YCbCr422_SEMIPLANAR)) {
		jpegInfo->dec_width = (jpegInfo->jpg_width + 0xF) & (~0xF);
		jpegInfo->dec_height = (jpegInfo->jpg_height + 0xF) & (~0xF);
	} else {
		jpegInfo->dec_width = (jpegInfo->jpg_width + 0x7) & (~0x7);
		jpegInfo->dec_height = (jpegInfo->jpg_height + 0x7) & (~0x7);
	}
	jpegInfo->dec_addry = DecOut.outputPictureY.busAddress;
	jpegInfo->dec_addru = DecOut.outputPictureCbCr.busAddress;
	jpegInfo->dec_addrv = 0;

end:
	JpegDecRelease((JpegDecInst)jpegInfo->handle->decInst);

exit:
	MFCSwitchIramMode(0);

	return ret;
}

MFCHandle* mfc_init(int streamType)
{
	MFCHandle *handle = NULL;
	void *decInst = NULL;

	switch(streamType) {
	case RAW_STRM_TYPE_H264: {
		H264DecInst decoder;
		H264DecRet infoRet;

		g_picDecodeNumber = 0;
		infoRet = H264DecInit(&decoder, 0, 0, 1);
		if(infoRet != H264DEC_OK) {
			printf("H264DecInit failure.\n");
			return NULL;
		}
		decInst = (void*)decoder;
		break;
	}

	case RAW_STRM_TYPE_H264_NOREORDER: {
		H264DecInst decoder;
		H264DecRet infoRet;

		g_picDecodeNumber = 0;
		infoRet = H264DecInit(&decoder, 1, 0, 1);
		if(infoRet != H264DEC_OK) {
			printf("H264DecInit failure.\n");
			return NULL;
		}
		decInst = (void*)decoder;
		break;
	}

    case RAW_STRM_TYPE_JPEG:
		break;

	default:
		printf("MFCDecode unsupported format.\n");
		return NULL;
	}

	handle = (MFCHandle*)pvPortMalloc(sizeof(MFCHandle));
	if (!handle)
		return NULL;
	memset(handle, 0, sizeof(MFCHandle));
	handle->decInst = decInst;
	handle->streamType = streamType;

	return handle;
}

int mfc_decode(MFCHandle *handle, DWLLinearMem_t *inBuffer, OutFrameBuffer *outBuffer)
{
	int ret = 0;

	if (handle == NULL) {
		printf("Invalid handle.\n");
		return -1;
	}
	MFCSwitchIramMode(1);

	outBuffer->num = 0;
	switch(handle->streamType) {
		case RAW_STRM_TYPE_H264:
		case RAW_STRM_TYPE_H264_NOREORDER:
			ret= MFCH264Decode(handle, inBuffer, outBuffer);
			break;
	    case RAW_STRM_TYPE_JPEG:
			ret= MFCJpegDecode(handle, inBuffer, outBuffer);
			break;
		default:
			printf("MFCDecode unsupported format.\n");
			ret = -1;
			break;
	}
	MFCSwitchIramMode(0);

	return ret;
}

void mfc_uninit(MFCHandle *handle)
{
	if (handle == NULL) {
		printf("Invalid handle.\n");
		return ;
	}

	switch(handle->streamType)
	{
		case RAW_STRM_TYPE_H264:
		case RAW_STRM_TYPE_H264_NOREORDER:
			if(handle->decInst)
				H264DecRelease((H264DecInst)handle->decInst);
			break;

        case RAW_STRM_TYPE_JPEG:
			JpegReleaseBuffer();
			break;

		default:
			printf("MFCRelease unsupported format.\n");
			break;
	}

	vPortFree(handle);
}
