#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include "usb_os_adapter.h"
#include "board.h"


#ifdef USB_UVC_SUPPORT
#include "ff_stdio.h"
#include "usb.h"
#include "list.h"
#include "semphr.h"
#include "timers.h"
#include "lcd.h"
#include "pxp.h"
#include "timer.h"
#include "cp15/cp15.h"
#include "jpegdecapi.h"
#include "mfcapi.h"
#include "usb_uvc.h"


/* */
#define UVC_STREAM_EOH						(1 << 7)
#define UVC_STREAM_ERR						(1 << 6)
#define UVC_STREAM_STI						(1 << 5)
#define UVC_STREAM_RES						(1 << 4)
#define UVC_STREAM_SCR						(1 << 3)
#define UVC_STREAM_PTS						(1 << 2)
#define UVC_STREAM_EOF						(1 << 1)
#define UVC_STREAM_FID						(1 << 0)

#define UVC_ALIGN_8(x)						ALIGN(x, 8)
#define UVC_ALIGN_4(x)						ALIGN(x, 4)
#define UVC_HEADER_LEN						0x0c

#define UVC_MAX_ENDPOINT_LEN				0x2000
#define UVC_VIDEO_WIDTH						1024
#define UVC_VIDEO_HEIGHT					600
#define UVC_JPEG_FIFO_COUNT					4

#define UVC_JPEG_BUF_SIZE 					(UVC_VIDEO_WIDTH * UVC_VIDEO_HEIGHT)
#define UVC_YUV_BUF_SIZE					((UVC_ALIGN_8(UVC_VIDEO_WIDTH) * UVC_ALIGN_8(UVC_VIDEO_HEIGHT)) * 2)

#define UVC_DATA_RECEIVE_TASK_PRIORITY		(configMAX_PRIORITIES - 4)
#define UVC_JPEG_DECODE_TASK_PRIORITY		(configMAX_PRIORITIES - 5)

#define UVC_TEST


typedef struct _uvc_video_frame {
	ListItem_t   entry;
	uint8_t		*buf;
	uint8_t		*cur;
	unsigned int len;
	unsigned int frame_id;
} uvc_video_frame_t;

typedef struct _uvc_disp_area {
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
} uvc_disp_area_t;

typedef struct _uvc_probe_param {
	uint16_t	bmHint;
	uint8_t		bFormatIndex;
	uint8_t		bFrameIndex;
	uint32_t	dwFrameInterval;
	uint16_t	wKeyFrameRate;
	uint16_t	wPFrameRate;
	uint16_t	wCompQuality;
	uint16_t	wCompWindowSize;
	uint16_t	wDelay;
	uint32_t	dwMaxVideoFrameSize;
	uint32_t	dwMaxPayloadTransferSize;
} uvc_probe_param_t;

typedef struct _uvc_dev_info {
	struct usb_device *dev;
	TaskHandle_t dec_task_handle;
	TaskHandle_t rcv_task_handle;
	List_t frame_free_list;
	List_t frame_ready_list;
	SemaphoreHandle_t frame_list_mutex;
	QueueHandle_t jpeg_frame_queue;
	QueueHandle_t exit_queue;
	MFCHandle *mfc_handle;
	uint8_t *jpeg_buf;
	uint8_t *yuv_buf;
#ifdef USB_DMA
	uint8_t *dma_buf;
#endif
	int32_t pipe;
	uint32_t dev_ready;
	uint32_t dec_task_start;
	uint32_t uvc_start;
	uvc_disp_area_t disp_area;
	//
	struct usb_interface_descriptor pVideoInterface;
	struct usb_endpoint_descriptor pVideoEndpoint;
	uint8_t num_altsetting;
	uint8_t bEndpointNumber;
} uvc_dev_info_t;


static uvc_dev_info_t *g_uvc_info = NULL;

extern void usb_set_maxpacket_ep_ex(struct usb_device *dev, int if_idx, int alt_idx, int ep_idx);
static int uvc_jpeg_decode_and_display(uvc_dev_info_t *uvc, uvc_video_frame_t* frame);
static void uvc_frame_list_reset(uvc_dev_info_t *uvc);


static void uvc_parse_probe_param(uvc_probe_param_t *ProbeParam, uint8_t *pdata)
{
	ProbeParam->bmHint = (pdata[1]<<8) | pdata[0];
	ProbeParam->bFormatIndex = pdata[2];
	ProbeParam->bFrameIndex = pdata[3];
	ProbeParam->dwFrameInterval = (pdata[7]<<24)|(pdata[6]<<16)|(pdata[5]<<8) | pdata[4];
	ProbeParam->wKeyFrameRate = (pdata[9]<<8) | pdata[8];
	ProbeParam->wPFrameRate = (pdata[11]<<8) | pdata[10];
	ProbeParam->wCompQuality = (pdata[13]<<8) | pdata[12];
	ProbeParam->wCompWindowSize = (pdata[15]<<8) | pdata[14];
	ProbeParam->wDelay = (pdata[17]<<8) | pdata[16];
	ProbeParam->dwMaxVideoFrameSize = (pdata[21]<<24)|(pdata[20]<<16)|(pdata[19]<<8) | pdata[18];
	ProbeParam->dwMaxPayloadTransferSize = (pdata[21]<<25)|(pdata[24]<<16)|(pdata[23]<<8) | pdata[22];
}

static void uvc_dump_probe_param(uvc_probe_param_t *ProbeParam)
{
	printf("=== MGC_UvcGetProbeControl ===\r\n");
	printf("bmHint = 0x%x\r\n", ProbeParam->bmHint);
	printf("bFormatIndex = 0x%x\r\n", ProbeParam->bFormatIndex);
	printf("bFrameIndex = 0x%x\r\n", ProbeParam->bFrameIndex);
	printf("dwFrameInterval = 0x%x\r\n", ProbeParam->dwFrameInterval);
	printf("wKeyFrameRate = 0x%x\r\n", ProbeParam->wKeyFrameRate);
	printf("wPFrameRate = 0x%x\r\n", ProbeParam->wPFrameRate);
	printf("wCompQuality = 0x%x\r\n", ProbeParam->wCompQuality);
	printf("wCompWindowSize = 0x%x\r\n", ProbeParam->wCompWindowSize);
	printf("wDelay = 0x%x\r\n", ProbeParam->wDelay);
	printf("dwMaxVideoFrameSize = 0x%x\r\n", ProbeParam->dwMaxVideoFrameSize);
	printf("dwMaxPayloadTransferSize = 0x%x\r\n", ProbeParam->dwMaxPayloadTransferSize);
}

static void uvc_prepare_probe_data(uvc_probe_param_t *ProbeParam, uint8_t *out)
{
	out[0] = ProbeParam->bmHint & 0xff;
	out[1] = (ProbeParam->bmHint>>8) & 0xff;
	out[2] = ProbeParam->bFormatIndex & 0xff;	
	out[3] = ProbeParam->bFrameIndex & 0xff;
	out[4] = ProbeParam->dwFrameInterval & 0xff;
	out[5] = (ProbeParam->dwFrameInterval>>8) & 0xff;
	out[6] = (ProbeParam->dwFrameInterval>>16) & 0xff;
	out[7] = (ProbeParam->dwFrameInterval>>24) & 0xff;
	out[8] = ProbeParam->wKeyFrameRate & 0xff;
	out[9] = (ProbeParam->wKeyFrameRate>>8) & 0xff;
	out[10] = ProbeParam->wPFrameRate & 0xff;
	out[11] = (ProbeParam->wPFrameRate>>8) & 0xff;
	out[12] = ProbeParam->wCompQuality & 0xff;
	out[13] = (ProbeParam->wCompQuality>>8) & 0xff;
	out[14] = ProbeParam->wCompWindowSize & 0xff;
	out[15] = (ProbeParam->wCompWindowSize>>8) & 0xff;
	out[16] = ProbeParam->wDelay & 0xff;
	out[17] = (ProbeParam->wDelay>>8) & 0xff;
	out[18] = ProbeParam->dwMaxVideoFrameSize & 0xff;
	out[19] = (ProbeParam->dwMaxVideoFrameSize>>8) & 0xff;
	out[20] = (ProbeParam->dwMaxVideoFrameSize>>16) & 0xff;
	out[21] = (ProbeParam->dwMaxVideoFrameSize>>24) & 0xff;
	out[22] = ProbeParam->dwMaxPayloadTransferSize & 0xff;
	out[23] = (ProbeParam->dwMaxPayloadTransferSize>>8) & 0xff;
	out[24] = (ProbeParam->dwMaxPayloadTransferSize>>16) & 0xff;
	out[25] = (ProbeParam->dwMaxPayloadTransferSize>>24) & 0xff;
}

int uvc_select_max_endpoint(uvc_dev_info_t *uvc)
{
	struct usb_interface *iface;
	struct usb_interface_descriptor *if_desc;
	struct usb_endpoint_descriptor *ep_desc;
	struct usb_device *dev;
	int if_index;
	int alt_index;
	int ep_index;
	int ret = -1;

	if (!uvc) {
		printf("%s, Invalid uvc.\n", __func__);
		return -1;
	}

	dev = uvc->dev;
	uvc->num_altsetting = 0;
	uvc->bEndpointNumber = 0;
	memset(&uvc->pVideoInterface, 0, sizeof(struct usb_interface_descriptor));
	memset(&uvc->pVideoEndpoint, 0, sizeof(struct usb_endpoint_descriptor));

	for (if_index = 0; (if_index < dev->config.desc.bNumInterfaces) && (if_index < USB_MAXINTERFACES); if_index++) {
		int total_ep_index = 0;
		iface = &dev->config.if_desc[if_index];
		printf("num_altsetting is 0x%x\n", iface->num_altsetting);
		for (alt_index = 0; alt_index < iface->num_altsetting; alt_index++) {
			if (iface->num_altsetting > 1)
				if_desc = &iface->alt_intf[alt_index].desc;
			else
				if_desc = &iface->desc;
			if ((if_desc->bInterfaceClass == USB_CLASS_VIDEO) ||
				(if_desc->bInterfaceClass == USB_CLASS_VENDOR_SPEC)) {
#ifdef UVC_TEST
				printf("===== video interface index is 0x%x=====\n", if_index);
				printf("bLength is 0x%x\n", if_desc->bLength);
				printf("bDescriptorType is 0x%x\n", if_desc->bDescriptorType);
				printf("bInterfaceNumber is 0x%x\n", if_desc->bInterfaceNumber);
				printf("bAlternateSetting is 0x%x\n", if_desc->bAlternateSetting);
				printf("bNumEndpoints is 0x%x\n", if_desc->bNumEndpoints);
				printf("bInterfaceClass is 0x%x\n", if_desc->bInterfaceClass);
				printf("bInterfaceSubClass is 0x%x\n", if_desc->bInterfaceSubClass);
				printf("bInterfaceProtocol is 0x%x\n", if_desc->bInterfaceProtocol);
				printf("iInterface is 0x%x\n", if_desc->iInterface);
				printf("=======================================\n");
#endif
				for (ep_index = 0; ep_index < if_desc->bNumEndpoints; ep_index++) {
//					if (iface->num_altsetting > 1)
//						ep_desc = &iface->alt_intf->ep_desc[ep_index];
//					else
						//ep_desc = &iface->ep_desc[ep_index];
					ep_desc = &iface->ep_desc[total_ep_index++];

					if ((ep_desc->wMaxPacketSize <= UVC_MAX_ENDPOINT_LEN) &&
						(ep_desc->wMaxPacketSize > uvc->pVideoEndpoint.wMaxPacketSize) &&
						((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) != USB_ENDPOINT_XFER_CONTROL))
					{
						uvc->num_altsetting = iface->num_altsetting;
						//if
						uvc->pVideoInterface.bLength = if_desc->bLength;
						uvc->pVideoInterface.bDescriptorType = if_desc->bDescriptorType;
						uvc->pVideoInterface.bInterfaceNumber = if_desc->bInterfaceNumber;
						uvc->pVideoInterface.bAlternateSetting = if_desc->bAlternateSetting;
						uvc->pVideoInterface.bNumEndpoints = if_desc->bNumEndpoints;
						uvc->pVideoInterface.bInterfaceClass = if_desc->bInterfaceClass;
						uvc->pVideoInterface.bInterfaceSubClass = if_desc->bInterfaceSubClass;
						uvc->pVideoInterface.bInterfaceProtocol = if_desc->bInterfaceProtocol;
						uvc->pVideoInterface.iInterface = if_desc->iInterface;
						//ep
						uvc->bEndpointNumber = ep_index;
						uvc->pVideoEndpoint.bLength = ep_desc->bLength;
						uvc->pVideoEndpoint.bDescriptorType = ep_desc->bDescriptorType;
						uvc->pVideoEndpoint.bEndpointAddress = ep_desc->bEndpointAddress;
						uvc->pVideoEndpoint.bmAttributes = ep_desc->bmAttributes;
						uvc->pVideoEndpoint.wMaxPacketSize = ep_desc->wMaxPacketSize;
						uvc->pVideoEndpoint.bInterval = ep_desc->bInterval;
#ifdef UVC_TEST
						printf("	=====endpoint index is 0x%x=====\n", ep_index);
						printf("	bLength is 0x%x\n", ep_desc->bLength);
						printf("	bDescriptorType is 0x%x\n", ep_desc->bDescriptorType);
						printf("	bEndpointAddress is 0x%x\n", ep_desc->bEndpointAddress);
						printf("	bmAttributes is 0x%x\n", ep_desc->bmAttributes);
						printf("	wMaxPacketSize is 0x%x\n", ep_desc->wMaxPacketSize);
						printf("	bInterval is 0x%x\n", ep_desc->bInterval);
						printf("	=======================================\n");
#endif
						ret = 0;
					}
				}
			} else if (if_desc->bInterfaceClass == USB_CLASS_AUDIO) {

			}
		}
	}

	return ret;
}

static int uvc_camera_init(uvc_dev_info_t *uvc)
{
	struct usb_device *dev;
	struct usb_interface *iface;
	uvc_probe_param_t ProbeParam = {0};
	uint32_t interface_idx, alternate_idx, endpoint_idx;
	uint32_t flag_video_ctrl = 0;
	uint32_t flag_video_stream = 0;
	uint8_t ep_in = 0;
	uint8_t probe_buf[128] = {0};
	uint8_t *pdata = NULL;
	int i, ret = -1;

	if (!uvc) {
		printf("%s, Invalid uvc\n", __func__);
		return -1;
	}

	dev = uvc->dev;
	if (!dev) {
		printf("%s, Invalid dev\n", __func__);
		return -1;
	}

	for (i = 0; i < USB_MAXINTERFACES; i++) {
		iface = &dev->config.if_desc[i];
		if ((iface->desc.bInterfaceClass == USB_CLASS_VIDEO) ||
			(iface->desc.bInterfaceClass == USB_CLASS_VENDOR_SPEC)) {
			if (iface->desc.bInterfaceSubClass == 0x01) {
				flag_video_ctrl = 1;
			} else if (iface->desc.bInterfaceSubClass == 0x02) {
				flag_video_stream = 1;
			}
		}
	}

	if (!flag_video_ctrl || !flag_video_stream) {
		goto exit;
	}
	printf("UVC device fond.\n");

	ret = uvc_select_max_endpoint(uvc);
	if (ret < 0) {
		printf("%s, uvc_select_max_endpoint failed\n", __func__);
		goto exit;
	}

	interface_idx = uvc->pVideoInterface.bInterfaceNumber;
	alternate_idx = uvc->pVideoInterface.bAlternateSetting;
	endpoint_idx = uvc->bEndpointNumber;
	printf("uvc camera select: if:%d, alt:%d, ep:%d, wMaxPacketSize:0x%x\n",
		interface_idx, alternate_idx, endpoint_idx, uvc->pVideoEndpoint.wMaxPacketSize);
	if ((uvc->pVideoEndpoint.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) != USB_ENDPOINT_XFER_CONTROL) {
		if (uvc->pVideoEndpoint.bEndpointAddress & USB_DIR_IN) {
			usb_set_maxpacket_ep_ex(dev, interface_idx, alternate_idx, endpoint_idx);
		}
	}

	if (uvc->num_altsetting == 1) {
		ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0), USB_REQ_SET_INTERFACE, USB_RECIP_INTERFACE,
			alternate_idx, interface_idx, NULL, 0, USB_CNTL_TIMEOUT * 5);
	} else {
		ret = usb_set_interface(dev, 1, 0);
	}
	if (ret < 0) {
		goto exit;
	}
	mdelay(10);

	/* Get Cur */
	ret= usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), 0x81, 0xA1, 0x100, 0x1, probe_buf, 0x1A, 3000);
	if (ret < 0) {
		goto exit;
	}

	uvc_parse_probe_param(&ProbeParam, probe_buf);
	uvc_dump_probe_param(&ProbeParam);

	/* 客户设置自己的摄像头参数配置 */
#if 0	//wy uvc camera(iso)
	ProbeParam.bFormatIndex				= 1;	//mjpeg
	ProbeParam.bFrameIndex				= 8;	//8:
#elif 0	//ly uvc camera(bulk)
	ProbeParam.bFormatIndex 			= 1;	//mjpeg
	ProbeParam.bFrameIndex				= 3;	//1:640x360; 2:640x480; 3:1024x600
#elif 0	//ly-board uvc camera(bulk)
	ProbeParam.bFormatIndex 			= 2;	//mjpeg
	ProbeParam.bFrameIndex				= 2;	//1:640x480; 2:1024x600
#elif 1	//ly uvc camera(iso)
	ProbeParam.bFormatIndex 			= 2;	//mjpeg
	ProbeParam.bFrameIndex				= 2;	//1:1280x720; 2:640x480
#else	//hdmi to uvc camera(iso)
	ProbeParam.bFormatIndex 			= 1;	//1:mjpeg; 2:yuy2
	ProbeParam.bFrameIndex				= 0x07;	//1:1920x1080; 6:720P; 7:1024x768; 0x0b:640x480
//	ProbeParam.dwMaxVideoFrameSize		= 0x003f4800;//0x96000;
//	ProbeParam.dwFrameInterval			= 0x0007A120;//30fps:0x051615; 20fps:0x0007A120; 10fps:0x000F4240; 5fps:0x001E8480
//	ProbeParam.dwMaxPayloadTransferSize = 0x00001400;
#endif

	/* Set Cur */
	pdata = probe_buf;
	uvc_prepare_probe_data(&ProbeParam, pdata);
	ret= usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x1, 0x21, 0x100, 0x1, pdata, 0x1A, 3000);
	if (ret < 0) {
		goto exit;
	}

	/* Get Cur */
	ret= usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), 0x81, 0xA1, 0x100, 0x1, probe_buf, 0x1A, 3000);
	if (ret < 0) {
		goto exit;
	}
	uvc_parse_probe_param(&ProbeParam, probe_buf);
	uvc_dump_probe_param(&ProbeParam);
	mdelay(10);

	/* Set Commit */
	pdata = probe_buf;
	uvc_prepare_probe_data(&ProbeParam, pdata);
	ret= usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x1, 0x21, 0x200, 0x1, pdata, 0x1A, 10000);
	ret= usb_control_msg(dev, usb_sndctrlpipe(dev, 0), 0x1, 0x21, 0x200, 0x1, pdata, 0x1A, 10000);	//avoid abnormal.
	if (ret < 0) {
		goto exit;
	}
	mdelay(10);

	ret = usb_set_interface(dev, interface_idx, alternate_idx);
	if (ret < 0) {
		goto exit;
	}

	ep_in = uvc->pVideoEndpoint.bEndpointAddress;
	if ((uvc->pVideoEndpoint.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC) {
		uvc->pipe = usb_rcvisocpipe(dev, ep_in);
	} else if ((uvc->pVideoEndpoint.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
		uvc->pipe = usb_rcvbulkpipe(dev, ep_in);
	} else if ((uvc->pVideoEndpoint.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) {
		uvc->pipe = usb_rcvintpipe(dev, ep_in);
	}
exit:
	return ret;
}

static uvc_video_frame_t* uvc_get_free_node(uvc_dev_info_t *uvc)
{
	uvc_video_frame_t* frame = NULL;
	ListItem_t* item;

	xSemaphoreTake(uvc->frame_list_mutex, portMAX_DELAY);
	if (listLIST_IS_EMPTY(&uvc->frame_free_list)) {
#ifdef UVC_TEST
		printf("### uvc no free node, ready node count:%d\n", uvc->frame_ready_list.uxNumberOfItems);
#endif
#if 0	//reuse ready node.
		if (!listLIST_IS_EMPTY(&uvc->frame_ready_list)) {
			item = listGET_HEAD_ENTRY(&uvc->frame_ready_list);
			uxListRemove(item);
			frame = listGET_LIST_ITEM_OWNER(item);
		}
#endif
		xSemaphoreGive(uvc->frame_list_mutex);
		return frame;
	}
	item = listGET_HEAD_ENTRY(&uvc->frame_free_list);
	uxListRemove(item);
	frame = listGET_LIST_ITEM_OWNER(item);
	xSemaphoreGive(uvc->frame_list_mutex);

	return frame;
}

#if 0
/* 采用数据拼接的方式：
 *	即直接使用目的缓存接收每一微帧数据，每次接收完一个微帧，备份末尾12个字节（帧格式头长度为12字节），
 *	下一微帧直接覆盖上一微帧的末尾12个字节，然后再把上一微帧备份的12字节拷贝回来，这样每一微帧仅拷贝
 *	12字节。这种方式可以减少拷贝时间，节省带宽，图像分辨率过大时不易花屏。
 */

static int uvc_iso_data_proc(uvc_dev_info_t *uvc)
{
	struct usb_device *dev;
	uvc_video_frame_t* frame = NULL;
	int payload_len = 0, frame_len = 0;
	int buf_len = UVC_JPEG_BUF_SIZE;
	int8_t header_backup_buf[UVC_HEADER_LEN] = {0};
	uint8_t header_len;
	uint8_t bfh;
//	int8_t fid = -1;
//	int jpeg_header_fond = 0;
	int ret;

	if (!uvc) {
		printf("%s, Invalid uvc\n", __func__);
		goto exit;
	}
	dev = uvc->dev;
	if (!dev) {
		printf("%s, Invalid dev\n", __func__);
		goto exit;
	}

	while (uvc->dev_ready) {
		if (!frame) {
			frame = uvc_get_free_node(uvc);
			if (!frame) {
				msleep(30);
				continue;
			}
			frame->cur = frame->buf;
			frame->len = 0;
		}

#ifdef USB_DMA
		uint8_t *align_buffer = frame->cur;

		if ((unsigned int)frame->cur & 3) {
			if (uvc->dma_buf) {
				align_buffer = (uint8_t *)UVC_ALIGN_4((unsigned int)uvc->dma_buf);
			}
		}
		ret = usb_iso_msg(dev, uvc->pipe, (void*)align_buffer, buf_len, &payload_len, pdMS_TO_TICKS(100));
		if (((unsigned int)frame->cur & 3) && (payload_len > 0)) {
			memcpy(frame->cur, align_buffer, payload_len);
		}
#else
		ret = usb_iso_msg(dev, uvc->pipe, (void*)frame->cur, buf_len, &payload_len, pdMS_TO_TICKS(100));
#endif
		if (ret < 0) {
			printf("%s send receive cmd failed.\n", __func__);
			goto exit;
		}
		if (payload_len <= 0) {
			msleep(30);
			continue;
		}

		header_len = frame->cur[0];
		bfh = frame->cur[1];
#if 0
		if ((header_len != UVC_HEADER_LEN) || (payload_len <= header_len) || ((bfh & 0xf0) != 0x80)) {
			//max payload len: 0xbfe
			if ((payload_len == header_len) && (bfh & UVC_STREAM_EOF)) {
				goto end_of_frame;
			}
			//msleep(1);
			continue;
		}

		if ((bfh & UVC_STREAM_FID) != fid) {
			fid = (bfh & UVC_STREAM_FID);
			jpeg_header_fond = 1;
			printf("### FID: 0x%.2x, 0x%.2x\n", frame->cur[12], frame->cur[13]);
		}
		frame_len = payload_len - header_len;

		if (jpeg_header_fond == 0) {
			printf("### uvc drop frame, fid lost.\n");
			//msleep(5);
			goto end_of_frame;
		}
	
		if (frame->len + frame_len > UVC_JPEG_BUF_SIZE) {
			printf("### %s, uvc jpeg buf overflow.\n", __func__);
			frame->cur = frame->buf;
			frame->len = 0;
			jpeg_header_fond = 0;
			continue;
		}

		memcpy(frame->cur, header_backup_buf, UVC_HEADER_LEN);
		frame->cur += frame_len;
		frame->len += frame_len;
		memcpy(header_backup_buf, frame->cur, UVC_HEADER_LEN);

end_of_frame:
		if (bfh & UVC_STREAM_EOF) {
			printf("### EOF\n");
			xSemaphoreTake(uvc->frame_list_mutex, portMAX_DELAY);
			if ((frame->buf[12] == 0xff) && (frame->buf[13] == 0xd8) &&
				(frame->buf[frame->len+10] == 0xff) && (frame->buf[frame->len+11] == 0xd9)) {
				CP15_clean_dcache_for_dma((uint32_t)frame->buf, (uint32_t)frame->buf + frame->len + UVC_HEADER_LEN);
				vListInsertEnd(&uvc->frame_ready_list, &frame->entry);
				xSemaphoreGive(uvc->frame_list_mutex);
				xQueueSend(uvc->jpeg_frame_queue, NULL, 0);
			} else {
				vListInsertEnd(&uvc->frame_free_list, &frame->entry);
				xSemaphoreGive(uvc->frame_list_mutex);
			}
			frame = NULL;
			jpeg_header_fond = 0;
		}
		//msleep(1);
#else
		if ((header_len != UVC_HEADER_LEN) || (payload_len <= header_len) || ((bfh & 0xf0) != 0x80)) {
			//msleep(1);
			continue;
		}

		if ((frame->cur[12] == 0xff) && (frame->cur[13] == 0xd8)) {
			if (frame->cur != frame->buf) {
				printf("### %s, uvc lost EOF, drop frame\n", __func__);
				memcpy(frame->buf, frame->cur, payload_len);
				frame->cur = frame->buf;
				frame->len = 0;
			}
			//printf("### FID\n");
 		}

		frame_len = payload_len - header_len;
		if (frame->len + frame_len > UVC_JPEG_BUF_SIZE) {
			printf("###### %s, uvc jpeg buf overflow.\n", __func__);
			frame->cur = frame->buf;
			frame->len = 0;
			msleep(5);
			continue;
		} else if (frame_len > 0) {
			memcpy(frame->cur, header_backup_buf, UVC_HEADER_LEN);
			frame->cur += frame_len;
			frame->len += frame_len;
			memcpy(header_backup_buf, frame->cur, UVC_HEADER_LEN);
		}

		if ((frame->cur[10] == 0xff) && (frame->cur[11] == 0xd9)) {
			//printf("### EOF\n");
			xSemaphoreTake(uvc->frame_list_mutex, portMAX_DELAY);
			if ((frame->buf[12] == 0xff) && (frame->buf[13] == 0xd8)) {
				CP15_clean_dcache_for_dma((uint32_t)frame->buf, (uint32_t)frame->buf + frame->len + UVC_HEADER_LEN);
				vListInsertEnd(&uvc->frame_ready_list, &frame->entry);
				xSemaphoreGive(uvc->frame_list_mutex);
				xQueueSend(uvc->jpeg_frame_queue, NULL, 0);
			} else {
				printf("### %s, uvc lost FID, drop frame\n", __func__);
				vListInsertEnd(&uvc->frame_free_list, &frame->entry);
				xSemaphoreGive(uvc->frame_list_mutex);
			}
			frame = NULL;
		}
#endif
	}
exit:
	if (frame) {
		frame->cur = frame->buf;
		frame->len = 0;
		xSemaphoreTake(uvc->frame_list_mutex, portMAX_DELAY);
		vListInsertEnd(&uvc->frame_free_list, &frame->entry);
		xSemaphoreGive(uvc->frame_list_mutex);
		frame = NULL;
	}
	xQueueSend(uvc->jpeg_frame_queue, NULL, 0);
	xQueueSend(uvc->exit_queue, NULL, 0);
	printf("%s exit.\n", __func__);

	return 0;
}
#else
/* 采用数据拷贝的方式：
 *	即对接收到的每一微帧数据都进行拷贝， 然后拼成一帧图片，效率相对较低。
 *	图像分辨率过大时，可能会花屏或者闪屏。
 */
static int uvc_iso_data_proc(uvc_dev_info_t *uvc)
{
	struct usb_device *dev;
	uvc_video_frame_t* frame = NULL;
	int payload_len = 0, frame_len = 0;
	int buf_len = UVC_JPEG_BUF_SIZE;
	uint8_t header_len;
	uint8_t *dma_buf = NULL;
	uint8_t bfh;
	int8_t fid = -1;
	int ret;

	if (!uvc) {
		printf("%s, Invalid uvc\n", __func__);
		goto exit;
	}
	dev = uvc->dev;
	if (!dev) {
		printf("%s, Invalid dev\n", __func__);
		goto exit;
	}

#ifdef USB_DMA
	dma_buf = uvc->dma_buf;
#else
	dma_buf = pvPortMalloc(buf_len);
	if (dma_buf == NULL) {
		printf("%s, malloc dma_buf failed.\n", __func__);
		goto exit;
	}
#endif

	while (uvc->dev_ready) {
		if (!frame) {
			frame = uvc_get_free_node(uvc);
			if (!frame) {
				msleep(30);
				continue;
			}
			frame->cur = frame->buf + UVC_HEADER_LEN;
			frame->len = 0;
		}
		ret = usb_iso_msg(dev, uvc->pipe, (void*)dma_buf, buf_len, &payload_len, pdMS_TO_TICKS(100));
		if (ret < 0) {
			printf("%s send receive cmd failed.\n", __func__);
			goto exit;
		}
		if (payload_len <= 0) {
			msleep(1);
			continue;
		}
		header_len = dma_buf[0];
		bfh = dma_buf[1];
		if ((header_len != UVC_HEADER_LEN) || (payload_len < UVC_HEADER_LEN) || ((bfh & 0xf0) != 0x80)) {
			msleep(30);
			continue;
		}

		frame_len = payload_len - UVC_HEADER_LEN;
		if (frame->len + frame_len > UVC_JPEG_BUF_SIZE) {
			printf("### %s, uvc jpeg buf overflow.\n", __func__);
			frame->cur = frame->buf + UVC_HEADER_LEN;
			frame->len = 0;
			if ((bfh & UVC_STREAM_FID) != fid) {
				fid = (bfh & UVC_STREAM_FID);
			}
			continue;
		}

		if ((bfh & UVC_STREAM_FID) != fid) {
			//printf("### FID\n");
			fid = (bfh & UVC_STREAM_FID);
			frame->cur = frame->buf + UVC_HEADER_LEN;
			frame->len = 0;
		}

		if (frame_len > 0) {
			memcpy(frame->cur, dma_buf+UVC_HEADER_LEN, frame_len);
			frame->cur += frame_len;
			frame->len += frame_len;
		}

		if (bfh & UVC_STREAM_EOF) {
			//printf("### EOF\n");
			xSemaphoreTake(uvc->frame_list_mutex, portMAX_DELAY);
			if ((frame->buf[12] == 0xff) && (frame->buf[13] == 0xd8) &&
				(frame->buf[frame->len+10] == 0xff) && (frame->buf[frame->len+11] == 0xd9)) {
				CP15_clean_dcache_for_dma((uint32_t)frame->buf, (uint32_t)frame->buf + frame->len);
				vListInsertEnd(&uvc->frame_ready_list, &frame->entry);
				xSemaphoreGive(uvc->frame_list_mutex);
				xQueueSend(uvc->jpeg_frame_queue, NULL, 0);
			} else {
				vListInsertEnd(&uvc->frame_free_list, &frame->entry);
				xSemaphoreGive(uvc->frame_list_mutex);
				printf("drop one frame\n");
			}
			frame = NULL;
		}
#if 0	//free cpu.
		if (frame_len == 0) {
			msleep(1);
		}
#endif
	}
exit:
	if (frame) {
		frame->cur = frame->buf;
		frame->len = 0;
		xSemaphoreTake(uvc->frame_list_mutex, portMAX_DELAY);
		vListInsertEnd(&uvc->frame_free_list, &frame->entry);
		xSemaphoreGive(uvc->frame_list_mutex);
		frame = NULL;
	}
	xQueueSend(uvc->jpeg_frame_queue, NULL, 0);
	xQueueSend(uvc->exit_queue, NULL, 0);

#ifndef USB_DMA
	if (dma_buf)
		vPortFree(dma_buf);
#endif
	printf("%s exit.\n", __func__);

	return 0;
}
#endif

static int uvc_bulk_msg(struct usb_device *dev, unsigned int pipe, void *data, int len, int *actual_length, int timeout)
{
	int err;
#if 1
	if (len < 0)
		return -1;

	dev->status = USB_ST_NOT_PROC; /*not yet processed */
	err = submit_bulk_msg(dev, pipe, data, len, actual_length, timeout);
	if (err < 0)
		return err;
#else
	int transfer_len = 0, total_len = 0, act_len = 0;
	char *pdata = data;
#if 0
#define BULK_BUF_LEN	4096
	ALLOC_CACHE_ALIGN_BUFFER(char, dma_buf, BULK_BUF_LEN);
#else
#define BULK_BUF_LEN	16384
	static char *dma_buf = NULL;
	if (!dma_buf) {
		dma_buf = pvPortMalloc(BULK_BUF_LEN);
	}
#endif
	if (len < 0)
		return -1;

	while (len > 0) {
		dev->status = USB_ST_NOT_PROC; /*not yet processed */
		if (len < BULK_BUF_LEN)
			transfer_len = len;
		else
			transfer_len = BULK_BUF_LEN;
//		if (0 == gconnect_flag && dev->parent != NULL) {
//			dev->status = -1;
//			break;
//		}
		err = submit_bulk_msg(dev, pipe, dma_buf, transfer_len, &act_len, USB_CNTL_TIMEOUT * 5);
		if (err < 0)
			break;
		memcpy(pdata, dma_buf, act_len);
		total_len	+= act_len;
		pdata		+= act_len;
		len			-= act_len;

		if (act_len != transfer_len) {
			break;
		}
	}
	*actual_length = total_len;
#endif
	if (dev->status)
		return -1;

	return 0;
}

static int uvc_bulk_data_proc(uvc_dev_info_t *uvc)
{
#define JPGDEC_AFTER_EOF
	struct usb_device *dev;
	uvc_video_frame_t *frame = NULL;
	int payload_len = 0, frame_len = 0;
	const int buf_len = UVC_JPEG_BUF_SIZE;
	uint8_t header_backup_buf[12] = {0};
	uint8_t header_len;
	uint8_t bfh;
	uint8_t fid;
	int ret;

	if (!uvc) {
		printf("%s, Invalid uvc\n", __func__);
		goto exit;
	}
	dev = uvc->dev;
	if (!dev) {
		printf("%s, Invalid dev\n", __func__);
		goto exit;
	}

	while (uvc->dev_ready) {
		if (!uvc->uvc_start) {
			msleep(100);
			continue;
		}

		if (!frame) {
			frame = uvc_get_free_node(uvc);
			if (!frame) {
				msleep(30);
				continue;
			}
			frame->cur = frame->buf;
			frame->len = 0;
		}

		payload_len = 0;
#ifdef USB_DMA
		uint8_t *align_buffer = frame->cur;

		if ((unsigned int)frame->cur & 3) {
			if (uvc->dma_buf) {
				align_buffer = (uint8_t *)UVC_ALIGN_4((unsigned int)uvc->dma_buf);
			}
		}
		ret = uvc_bulk_msg(dev, uvc->pipe, (void*)align_buffer, buf_len, &payload_len, pdMS_TO_TICKS(100));
		if (((unsigned int)frame->cur & 3) && (payload_len > 0)) {
			memcpy(frame->cur, align_buffer, payload_len);
		}
#else
		ret = uvc_bulk_msg(dev, uvc->pipe, (void*)frame->cur, buf_len, &payload_len, pdMS_TO_TICKS(100));
#endif
		if (ret < 0) {
			printf("%s send receive cmd failed.\n", __func__);
			goto exit;
		}
		if (payload_len <= 0) {
			//msleep(30);
			continue;
		}

		header_len = frame->cur[0];
		bfh = frame->cur[1];
		if ((header_len != UVC_HEADER_LEN) || (payload_len < header_len)) {
			continue;
		}

		if ((bfh & UVC_STREAM_FID) != fid) {
			fid = (bfh & UVC_STREAM_FID);
			//printf("### FID: 0x%.2x, 0x%.2x\n", frame->cur[12], frame->cur[13]);
#if 1
			if (frame->cur != frame->buf) {
				printf("### uvc lost EOF, drop frame\n");
				memcpy(frame->buf, frame->cur, payload_len);
				frame->cur = frame->buf;
				frame->len = 0;
			}
#endif
		}
		frame_len = payload_len - header_len;
		if (frame_len > 0) {
			if (frame->len + frame_len > UVC_JPEG_BUF_SIZE) {
				printf("### %s, uvc jpeg buf overflow.\n", __func__);
				frame->cur = frame->buf;
				frame->len = 0;
				continue;
			}
			if (frame->cur != frame->buf)
				memcpy(frame->cur, header_backup_buf, UVC_HEADER_LEN);
			frame->cur += frame_len;
			frame->len += frame_len;
			memcpy(header_backup_buf, frame->cur, UVC_HEADER_LEN);
		} else {
			if (frame->cur != frame->buf)
				memcpy(frame->cur, header_backup_buf, UVC_HEADER_LEN);
		}

		if (bfh & UVC_STREAM_EOF) {
			//printf("### EOF\n");
#ifdef JPGDEC_AFTER_EOF
			if ((frame->buf[12] == 0xff) && (frame->buf[13] == 0xd8) &&
				(frame->buf[frame->len+10] == 0xff) && (frame->buf[frame->len+11] == 0xd9)) {
				if (uvc->uvc_start) {
					CP15_clean_dcache_for_dma((uint32_t)frame->buf, (uint32_t)frame->buf + frame->len + UVC_HEADER_LEN);
					uvc_jpeg_decode_and_display(uvc, frame);
				}
			} else {
				printf("### %s, uvc lost FID or EOF, drop frame\n", __func__);
			}
			frame->cur = frame->buf;
			frame->len = 0;
#else
			xSemaphoreTake(uvc->frame_list_mutex, portMAX_DELAY);
			if ((frame->buf[12] == 0xff) && (frame->buf[13] == 0xd8) &&
				(frame->buf[frame->len+10] == 0xff) && (frame->buf[frame->len+11] == 0xd9)) {
				CP15_clean_dcache_for_dma((uint32_t)frame->buf, (uint32_t)frame->buf + frame->len + UVC_HEADER_LEN);
				vListInsertEnd(&uvc->frame_ready_list, &frame->entry);
				xQueueSend(uvc->jpeg_frame_queue, NULL, 0);
			} else {
				printf("### %s, uvc lost FID, drop frame\n", __func__);
				vListInsertEnd(&uvc->frame_free_list, &frame->entry);
			}
			xSemaphoreGive(uvc->frame_list_mutex);
			frame = NULL;
			msleep(20);
#endif
		}
	}
exit:
	if (frame) {
		frame->cur = frame->buf;
		frame->len = 0;
		xSemaphoreTake(uvc->frame_list_mutex, portMAX_DELAY);
		vListInsertEnd(&uvc->frame_free_list, &frame->entry);
		xSemaphoreGive(uvc->frame_list_mutex);
		frame = NULL;
	}
	xQueueSend(uvc->jpeg_frame_queue, NULL, 0);
	xQueueSend(uvc->exit_queue, NULL, 0);
	printf("%s exit.\n", __func__);
#undef JPGDEC_AFTER_EOF

	return 0;
}

static void uvc_data_receive_task(void *parameters)
{
	uvc_dev_info_t *uvc = (uvc_dev_info_t *)parameters;

	if (!uvc) {
		printf("%s, Invalid uvc\n", __func__);
		goto exit;
	}
	if (!uvc->dev) {
		printf("%s, Invalid dev\n", __func__);
		goto exit;
	}

	xSemaphoreTake(uvc->frame_list_mutex, portMAX_DELAY);
	uvc_frame_list_reset(uvc);
	xSemaphoreGive(uvc->frame_list_mutex);
	xQueueReset(uvc->jpeg_frame_queue);

#ifdef UVC_TEST
	uvc_start();
#endif
	switch (uvc->pVideoEndpoint.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
		case USB_ENDPOINT_XFER_ISOC:
			uvc_iso_data_proc(uvc);
			break;
		case USB_ENDPOINT_XFER_BULK:
			uvc_bulk_data_proc(uvc);
			break;
		case USB_ENDPOINT_XFER_INT:
			break;
		default:
			break;
	}
#ifdef UVC_TEST
	uvc_stop();
#endif

exit:
	vTaskDelete(NULL);
}

static int uvc_jpeg_decode_and_display(uvc_dev_info_t *uvc, uvc_video_frame_t* frame)
{
	JpegHeaderInfo jpginfo = {0};
	uint32_t yaddr, uvaddr, vaddr, dstaddr;
	int format;
	int ret = -1;

	if (xVideoDisplayBufTake(pdMS_TO_TICKS(10)) == pdPASS) {
		if (!uvc->mfc_handle)
			uvc->mfc_handle = mfc_init(RAW_STRM_TYPE_JPEG);
		if (!uvc->mfc_handle) {
			printf("%s, mfc_init failed.\n", __func__);
			goto done;
		}

		jpginfo.handle = uvc->mfc_handle;
		jpginfo.jpg_addr = (uint32_t)frame->buf + UVC_HEADER_LEN;
		jpginfo.jpg_size = frame->len;
		jpginfo.dec_addry = (uint32_t)uvc->yuv_buf;
		jpginfo.dec_size = UVC_YUV_BUF_SIZE;

		ret = mfc_jpegdec(&jpginfo);
		if (ret < 0) {
			printf("%s, jpgdec failed.\n", __func__);
		} else {
			yaddr = jpginfo.dec_addry;
			uvaddr = jpginfo.dec_addru;
			vaddr = jpginfo.dec_addrv;
			if (jpginfo.dec_format == JPEGDEC_YCbCr420_SEMIPLANAR) {
				format = PXP_SRC_FMT_YUV2P420;
			} else if (jpginfo.dec_format == JPEGDEC_YCbCr422_SEMIPLANAR) {
				format = PXP_SRC_FMT_YUV2P422;
			} else {
				printf("%s Invalid yuv format.\n", __func__);
				ret = -1;
				goto done;
			}

			if (uvc->uvc_start) {
				dstaddr = ulVideoDisplayBufGet();
				ret = pxp_scaler_rotate(yaddr, uvaddr, vaddr, format, jpginfo.dec_width, jpginfo.dec_height,
					dstaddr, 0, PXP_OUT_FMT_RGB565, LCD_WIDTH, LCD_HEIGHT, 0);
				if (ret) {
					goto done;
				}
		
				LcdOsdInfo info = {0};
				info.x = uvc->disp_area.x;
				info.y = uvc->disp_area.y;
				info.width = uvc->disp_area.w;
				info.height = uvc->disp_area.h;
				info.format = LCD_OSD_FORAMT_RGB565;
				info.yaddr = dstaddr;

				if (!ark_lcd_get_osd_info_atomic_isactive(LCD_VIDEO_LAYER)) {
					ark_lcd_wait_for_vsync();
				}
				ark_lcd_set_osd_info_atomic(LCD_VIDEO_LAYER, &info);
				ark_lcd_osd_enable(LCD_VIDEO_LAYER, 1);
				ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
				ark_lcd_osd_enable(LCD_UI_LAYER, 0);
				ark_lcd_set_osd_sync(LCD_UI_LAYER);
				vVideoDisplayBufRender(dstaddr);
			}
			ret = 0;
		}
done:
		vVideoDisplayBufGive();
	}

	return ret;
}

static void uvc_jpeg_decode_task(void *parameters)
{
	uvc_dev_info_t *uvc = (uvc_dev_info_t *)parameters;
	uvc_video_frame_t* frame;
	ListItem_t* item;

	if (!uvc) {
		printf("%s, Invalid uvc\n", __func__);
		vTaskDelete(NULL);
		return;
	}

	while (uvc->dec_task_start) {
		xQueueReceive(uvc->jpeg_frame_queue, NULL, portMAX_DELAY);
		if (uvc->dec_task_start == 0) {
			break;
		}

		xSemaphoreTake(uvc->frame_list_mutex, portMAX_DELAY);
		if (listLIST_IS_EMPTY(&uvc->frame_ready_list)) {
			xSemaphoreGive(uvc->frame_list_mutex);
			continue;
		}
		item = listGET_HEAD_ENTRY(&uvc->frame_ready_list);
		uxListRemove(item);
		frame = listGET_LIST_ITEM_OWNER(item);
		xSemaphoreGive(uvc->frame_list_mutex);

		if (uvc->uvc_start) {
			uvc_jpeg_decode_and_display(uvc, frame);
			frame->buf[12] = frame->buf[13] = 0;
			frame->buf[frame->len-1] = frame->buf[frame->len-2] = 0;
		}

		xSemaphoreTake(uvc->frame_list_mutex, portMAX_DELAY);
		frame->cur = frame->buf;
		frame->len = 0;
		vListInsertEnd(&uvc->frame_free_list, &frame->entry);
		xSemaphoreGive(uvc->frame_list_mutex);
	}

	if (uvc->mfc_handle) {
		mfc_uninit(uvc->mfc_handle);
		uvc->mfc_handle = NULL;
	}
	printf("%s exit.\n", __func__);
	vTaskDelete(NULL);
}

static void uvc_frame_list_reset(uvc_dev_info_t *uvc)
{
	uvc_video_frame_t* frame = NULL;
	ListItem_t *pxListItem;

	list_for_each_entry(pxListItem, frame, &uvc->frame_ready_list) {
		ListItem_t * item = listGET_HEAD_ENTRY(&uvc->frame_ready_list);
		uxListRemove(item);
		frame->cur = frame->buf;
		frame->len = 0;
		vListInsertEnd(&uvc->frame_free_list, &frame->entry);
	}

	list_for_each_entry(pxListItem, frame, &uvc->frame_free_list) {
		frame->cur = frame->buf;
		frame->len = 0;
	}
}

static int uvc_frame_buf_init(uvc_dev_info_t *uvc)
{
	static uvc_video_frame_t jpeg_fifos[UVC_JPEG_FIFO_COUNT] = {0};
	static int g_uvc_init_flag = 0;
	uvc_video_frame_t *buffer;
	int i;

	if (!uvc) {
		printf("%s, Invalid uvc\n", __func__);
		return 0;
	}

	if (g_uvc_init_flag) {
		return 0;
	}

#ifdef USB_DMA
	uvc->dma_buf = pvPortMalloc(UVC_JPEG_BUF_SIZE);
	if (uvc->dma_buf == NULL) {
		printf("%s, pvPortMalloc jpeg buf failed.\n", __func__);
	}
#endif

	uvc->yuv_buf = pvPortMalloc(UVC_YUV_BUF_SIZE);
	if (!uvc->yuv_buf) {
		printf("%s, pvPortMalloc jpeg yuv buf failed\n", __func__);
		goto exit;
	}
	memset((void *)uvc->yuv_buf, 0, UVC_YUV_BUF_SIZE);

	uvc->jpeg_buf = pvPortMalloc(UVC_JPEG_FIFO_COUNT * UVC_JPEG_BUF_SIZE);
	if (uvc->jpeg_buf == NULL) {
		printf("%s, pvPortMalloc jpeg buf failed.\n", __func__);
		goto exit;
	}
	memset((void *)uvc->jpeg_buf, 0, UVC_JPEG_FIFO_COUNT * UVC_JPEG_BUF_SIZE);

	vListInitialise(&uvc->frame_free_list);
	vListInitialise(&uvc->frame_ready_list);

	buffer = (uvc_video_frame_t *)jpeg_fifos;
	for (i = 0; i < UVC_JPEG_FIFO_COUNT; i++) {
		buffer->buf = (uint8_t *)UVC_ALIGN_4(((unsigned int)uvc->jpeg_buf + i * UVC_JPEG_BUF_SIZE));
		buffer->cur = buffer->buf;
		buffer->len = 0;
		buffer->frame_id = i;
		vListInitialiseItem(&buffer->entry);
		vListInsertEnd(&uvc->frame_free_list, &buffer->entry);
		listSET_LIST_ITEM_OWNER(&buffer->entry, buffer);
		buffer++;
	}
	g_uvc_init_flag = 0;

	return 0;
exit:
#ifdef USB_DMA
	if (uvc->dma_buf) {
		vPortFree(uvc->dma_buf);
		uvc->dma_buf = NULL;
	}
#endif
	if (uvc->yuv_buf) {
		vPortFree(uvc->yuv_buf);
		uvc->yuv_buf = NULL;
	}
	if (uvc->jpeg_buf) {
		vPortFree(uvc->jpeg_buf);
		uvc->jpeg_buf = NULL;
	}

	return -1;
}

static int usb_uvc_probe(struct usb_device *dev)
{
	uvc_dev_info_t *uvc = g_uvc_info;
	int ret;

	if (!uvc) {
		printf("%s, Invalid uvc\n", __func__);
		return 0;
	}
	uvc->dev = dev;

	ret = uvc_camera_init(uvc);
	if (ret < 0) {
		return 0;
	}

	uvc->dev_ready = 1;
	xTaskCreate(uvc_data_receive_task, "uvc_data_receive_task", configMINIMAL_STACK_SIZE * 4,
					(void*)uvc, UVC_DATA_RECEIVE_TASK_PRIORITY, &uvc->rcv_task_handle);
	return 1;
}

int uvc_set_disp_pos(int x, int y)
{
	uvc_dev_info_t *uvc = g_uvc_info;
	if (uvc) {
		if ((x < LCD_WIDTH) && (y < LCD_HEIGHT)) {
			uvc->disp_area.x = x;
			uvc->disp_area.y = y;
		} else {
			printf("%s, Invalid x:%d or y:%d\n", __func__, x, y);
			return -1;
		}
	}
	return 0;
}

int uvc_set_disp_size(int width, int height)
{
	uvc_dev_info_t *uvc = g_uvc_info;
	if (uvc) {
		if (((uint32_t)width <= LCD_WIDTH) && ((uint32_t)height <= LCD_HEIGHT)) {
			uvc->disp_area.w = width;
			uvc->disp_area.h = height;
			//uvc->disp_area.w -= width;
			//uvc->disp_area.h -= height;
 		} else {
			printf("%s, Invalid width:%d or height:%d\n", __func__, width, height);
			return -1;
		}
	}
	return 0;
}

int uvc_start(void)
{
	uvc_dev_info_t *uvc = g_uvc_info;
	if (uvc) {
		uvc->uvc_start = 1;
		return 0;
	}
	return -1;
}

int uvc_stop(void)
{
	uvc_dev_info_t *uvc = g_uvc_info;
	int ret = -1;

	if (uvc) {
		uvc->uvc_start = 0;
		ret = 0;
	}

	//test and wait for display done.
	if (xVideoDisplayBufTake(pdMS_TO_TICKS(1000)) == pdPASS) {
		vVideoDisplayBufGive();
	}

	//osd layer switch.
	ark_lcd_osd_enable(LCD_VIDEO_LAYER, 0);
	ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
	ark_lcd_osd_enable(LCD_UI_LAYER, 1);
	ark_lcd_set_osd_sync(LCD_UI_LAYER);

	return ret;
}

int usb_uvc_scan(void)
{
	struct usb_device *dev;
	int ret = 0;
	int i;

	for (i = 0; i < USB_MAX_DEVICE; i++) {
		dev = usb_get_dev_index(i);
		if (!dev) {
			continue;
		}
		if (usb_uvc_probe(dev)) {
			ret = 1;
			break;
		}
	}

	return ret;
}

void usb_uvc_disconnect(void)
{
	uvc_dev_info_t *uvc = g_uvc_info;

	if (!uvc) {
		printf("%s, Invalid uvc\n", __func__);
		return;
	}

	printf("%s exit +++.\n", __func__);
	if (uvc->dev_ready != 0) {
		uvc->dev_ready = 0;
		xQueueReceive(uvc->exit_queue, NULL, portMAX_DELAY);
	}
	xQueueReset(uvc->exit_queue);
	printf("%s exit ---.\n", __func__);
}

int usb_uvc_init(void)
{
	uvc_dev_info_t *uvc;
	int ret;

	uvc = pvPortMalloc(sizeof(uvc_dev_info_t));
	if (!uvc) {
		printf("%s uvc pvPortMalloc failed\n", __func__);
		return -1;
	}
	memset(uvc, 0, sizeof(uvc_dev_info_t));

	ret = uvc_frame_buf_init(uvc);
	if (ret < 0) {
		printf("%s, uvc_frame_buf_init failed, uvc exit.\n", __func__);
		vPortFree(uvc);
		return -1;
	}

	uvc->frame_list_mutex = xSemaphoreCreateMutex();
	uvc->jpeg_frame_queue = xQueueCreate(UVC_JPEG_FIFO_COUNT, 0);
	uvc->exit_queue = xQueueCreate(1, 0);
	uvc->dec_task_start = 1;
	uvc->disp_area.x = 0;
	uvc->disp_area.y = 0;
	uvc->disp_area.w = LCD_WIDTH;
	uvc->disp_area.h = LCD_HEIGHT;

	g_uvc_info = uvc;

	ret = xTaskCreate(uvc_jpeg_decode_task, "uvc_jpeg_decode_task", configMINIMAL_STACK_SIZE,
						(void *)uvc, UVC_JPEG_DECODE_TASK_PRIORITY, &uvc->dec_task_handle);
	return ret;
}
#endif
