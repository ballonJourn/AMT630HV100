#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "board.h"
#include "chip.h"

#include "sfud.h"
#include "updatefile.h"
#include "mfcapi.h"
#include "jpegdecapi.h"
#include "dwl.h"
#include "animation.h"

#if DEVICE_TYPE_SELECT == EMMC_FLASH
#include "mmcsd_core.h"
#endif

static TaskHandle_t animation_task = NULL;
static SemaphoreHandle_t animation_mutex;
#if ANIMATION_POLICY == ANIMATION_USE_SMALL_MEM
static BANIHEADER ani_header;
static BANIHEADER *aniheader = &ani_header;
#else
static BANIHEADER *aniheader;
#endif
static unsigned int anioffset;
static unsigned int anisize;
static int ani_playing = 0;
static int ani_replay = 0;
static int ani_stop = 0;
static int ani_frames = 0;
static int ani_display_index = 0;
//static int ani_take_vdisbuf = 0;
static unsigned int ani_frame_addr;
static	uint32_t first_show_done = 0;


#if 0
#include "ff_stdio.h"

static int get_h264_single_file_from_sd(void *buf, int *len)
{
	#define READ_SIZE	(LCD_WIDTH*LCD_HEIGHT*2)
	char path_name[64] = "/sd/000.h264";
	static int count = 0;
	FF_FILE *fp;
	int ret = -1;

	sprintf(path_name, "/sd/%.3d.h264", count++);
	*len = 0;

	fp = ff_fopen(path_name, "rb");
	if (fp) {
		ret = ff_fread(buf, 1, READ_SIZE, fp);
		if (ret <= 0) {
			printf("read sdmmc file:%s failed, maybe read done\n", path_name);
			return -1;
		}
		*len = ret;
	} else {
		printf("### open %s fail.\n", path_name);
		count = 0;
		return -1;
	}
	if(fp)
		ff_fclose(fp);

	//printf("###read %s len:%d.\n", path_name, *len);

	return 0;
}

static void h264_decode_test_thread(void *param)
{
	#define H264DEC_SEMIPLANAR_YUV420	0x020001
	#define H264DEC_INBUF_SIZE			(LCD_WIDTH*LCD_HEIGHT*2)
	#define H264DEC_DISP_COUNTS			2

	DWLLinearMem_t inBuf;
	OutFrameBuffer outBuf = {0};
	uint32_t h264SrcSize = 0;
	MFCHandle *handle = NULL;
	uint32_t display_addr[H264DEC_DISP_COUNTS] = {0};
	uint32_t display_addr_index = 0;
	uint32_t first_show_done = 0;
	int ret;
	int i;

	void *h264SrcBuf = pvPortMalloc(H264DEC_INBUF_SIZE);
	if(!h264SrcBuf) {
		printf("%s, h264SrcBuf pvPortMalloc failed.\n", __func__);
		return ;
	}

	for(i=0; i<H264DEC_DISP_COUNTS; i++) {
		display_addr[i] = (uint32_t)pvPortMalloc(H264DEC_INBUF_SIZE);
		if(!display_addr[i] ) {
			printf("%s, display_addr[%d] pvPortMalloc failed.\n", __func__, i);
			return ;
		}
	}

	mmcsd_wait_cd_changed(portMAX_DELAY);

	handle = mfc_init(RAW_STRM_TYPE_H264);
	if(!handle) {
		printf("%s, mfc_init failed.\n", __func__);
		return ;
	}

	while(1) {
		//memset(&outBuf, 0, sizeof(OutFrameBuffer));
		//memset(h264SrcBuf, 0, H264DEC_INBUF_SIZE);
		mdelay(30);
#if 1	//get h264 info form sdcard.
		ret = get_h264_single_file_from_sd(h264SrcBuf, &h264SrcSize);
		if(ret != 0) {
			printf("%s, get_h264_single_file_from_sd failed.\n", __func__);
			continue;
		}
		inBuf.busAddress = (u32)h264SrcBuf;
		inBuf.virtualAddress = (u32 *)inBuf.busAddress;
		inBuf.size = h264SrcSize;
		CP15_clean_dcache_for_dma(inBuf.busAddress, inBuf.busAddress + inBuf.size);
#else
		//1. copy your h264 data to h264SrcBuf;
		//2. set your h264 data size to h264SrcSize;

		inBuf.busAddress = (u32)h264SrcBuf;
		inBuf.virtualAddress = (u32 *)inBuf.busAddress;
		inBuf.size = h264SrcSize;
#endif

		ret = mfc_decode(handle, &inBuf, &outBuf);
		if (ret < 0) {
			printf("animation decode fail.\n");
		} else {
			uint32_t align_width, align_height;
			uint32_t yaddr, uaddr, vaddr;

			for(i=0; i<outBuf.num; i++) {
				align_width = outBuf.frameWidth;
				align_height = outBuf.frameHeight;
				if(1/*outBuf.outputFormat == H264DEC_SEMIPLANAR_YUV420*/ && align_width && align_height)
				{
					memcpy((void *)display_addr[display_addr_index], outBuf.buffer[i].pyVirAddress, align_width * align_height * 3 / 2);
//					ret = dma_m2mcpy(display_addr[display_addr_index], (uint32_t)outBuf.buffer[i].yBusAddress, align_width * align_height * 3 / 2);
//					if(ret) {
//						printf("%s() dma_m2m_memcpy failed.\n", __func__);
//						continue;
//					}
					yaddr = display_addr[display_addr_index];
					uaddr = yaddr + align_width * align_height;
					vaddr = yaddr + align_width * align_height * 5/4;

					display_addr_index++;
					if(display_addr_index >= H264DEC_DISP_COUNTS) {
						display_addr_index = 0;
					}

					if(!first_show_done) {
						ark_lcd_set_osd_size(LCD_VIDEO_LAYER, /*outBuf.codedWidth, outBuf.codedHeight*/LCD_WIDTH, LCD_HEIGHT);
						ark_lcd_set_osd_format(LCD_VIDEO_LAYER, LCD_OSD_FORAMT_Y_UV420);
						ark_lcd_osd_enable(LCD_VIDEO_LAYER, 1);
						ark_lcd_osd_enable(LCD_UI_LAYER, 0);
						first_show_done = 1;
					}
					ark_lcd_set_osd_yaddr(LCD_VIDEO_LAYER, yaddr);
					ark_lcd_set_osd_uaddr(LCD_VIDEO_LAYER, uaddr);
					ark_lcd_set_osd_vaddr(LCD_VIDEO_LAYER, vaddr);
					ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
					ark_lcd_set_osd_sync(LCD_UI_LAYER);
					ark_lcd_wait_for_vsync();
				} else {
					continue;
				}
			}
		}
	}

	if(handle)
		mfc_uninit(handle);

	if(h264SrcBuf)
		vPortFree(h264SrcBuf);

	for(i=0; i<H264DEC_DISP_COUNTS; i++) {
		if(!display_addr[i]) {
			vPortFree((void *)display_addr[i]);
		}
	}

}

static int save_nv12_file_to_sd(void *buf, int len)
{
	char path_name[64] = "/sd/000.bin";
	static int count = 0;
	FF_FILE *fp;
	int ret = -1;

	sprintf(path_name, "/sd/%.3d.bin", count++);

	fp = ff_fopen(path_name, "w+");
	if (fp) {
		ret = ff_fwrite(buf, 1, len, fp);
		if (ret != len) {
			printf("read sdmmc file:%s failed.\n", path_name);
			ff_fclose(fp);
			return -1;
		}
	} else {
		printf("### open %s fail.\n", path_name);
		return -1;
	}
	ff_fclose(fp);

	return 0;
}

static int get_file_from_sd(void *buf, int *len)
{
	#define READ_SIZE	(LCD_WIDTH*LCD_HEIGHT*2)
#if LCD_BPP == 32
	char path_name[64] = "/sd/480x1280_rgb888.rgb";
#else
	char path_name[64] = "/sd/480x1280_rgb565.rgb";
#endif
	FF_FILE *fp;
	int ret = -1;

	*len = 0;
	fp = ff_fopen(path_name, "rb");
	if (fp) {
		ret = ff_fread(buf, 1, READ_SIZE, fp);
		if (ret <= 0) {
			printf("read sdmmc file:%s failed.\n", path_name);
			return -1;
		}
		*len = ret;
	} else {
		printf("### open %s fail.\n", path_name);
		return -1;
	}
	if(fp)
		ff_fclose(fp);


	return 0;
}

void mipi_disp_test(void)
{
	u32 len;
	int ret;
	u32 *disp_addr = (u32 *)pvPortMalloc(LCD_WIDTH*LCD_HEIGHT*2);
	if(!disp_addr) {
		printf("disp_addr pvPortMalloc failed.\n");
		return ;
	}

	mmcsd_wait_cd_changed(portMAX_DELAY);
	get_file_from_sd(disp_addr, &len);
	if(!len) {
		printf("get_file_from_sd failed.\n");
		return ;
	}
	CP15_clean_dcache_for_dma((uint32_t)disp_addr, (uint32_t)disp_addr + LCD_WIDTH*LCD_HEIGHT*2);

	u32 layer = LCD_OSD0;
	u32 width = VIDEO_DISPLAY_WIDTH;
	u32 height = VIDEO_DISPLAY_HEIGHT;
	u32 format;
#if LCD_BPP == 32
	format = LCD_OSD_FORAMT_ARGB888;
#else
	format = LCD_OSD_FORAMT_RGB565;
#endif

	while(1) {
		ark_lcd_osd_enable(LCD_OSD0, 0);
		ark_lcd_osd_enable(LCD_OSD1, 0);
		ark_lcd_set_osd_sync(LCD_OSD0);
		ark_lcd_set_osd_sync(LCD_OSD1);

		ark_lcd_set_osd_size(layer, width, height);
		ark_lcd_set_osd_format(layer, format);
		ark_lcd_osd_enable(layer, 1);
		ark_lcd_set_osd_yaddr(layer, (u32)disp_addr);
		ark_lcd_set_osd_sync(layer);
		ark_lcd_wait_for_vsync();
	}
}
#endif
#define ANIMA_LCD_WIDTH		800
#define ANIMA_LCD_HEIGHT	480

static void animation_thread(void *param)
{
	MFCHandle *mfc_handle = NULL;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
	sfud_flash *sflash;
#endif
	uint32_t size;
	uint32_t stick, etick, delaytick;
	uint32_t display_addr;
	uint32_t *tmp_addr = NULL;


	anioffset = GetUpFileOffset(MKTAG('B', 'A', 'N', 'I'));
	anisize = GetUpFileSize(MKTAG('B', 'A', 'N', 'I'));
	if (anisize > 0) {
		void *anibuf = pvPortMalloc(anisize);
		if (anibuf) {
#if DEVICE_TYPE_SELECT != EMMC_FLASH
			sflash = sfud_get_device(0);
			sfud_read(sflash, anioffset, anisize, anibuf);
#else
			emmc_read(anioffset, anisize, anibuf);
#endif
			aniheader = (BANIHEADER*)anibuf;

			if (aniheader->magic != MKTAG('B', 'A', 'N', 'I')) {
				printf("Read animation file error!\n");
				vPortFree(anibuf);
				vTaskDelete(NULL);
				return;
			}
			CP15_clean_dcache_for_dma((uint32_t)anibuf, (uint32_t)anibuf + anisize);
		} else {
			printf("Error! No enough memory for animation file.\n");
			vTaskDelete(NULL);
			return;
		}
	} else {
		printf("Warning! Not found animation in update file.\n");
		vTaskDelete(NULL);
		return;
	}

	ani_frame_addr = (uint32_t)aniheader + sizeof(BANIHEADER);
	for (;;) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		xSemaphoreTake(animation_mutex, portMAX_DELAY);
		if (ani_replay) {
			ani_frame_addr = (uint32_t)aniheader + sizeof(BANIHEADER);
			ani_frames = 0;
			ani_replay = 0;
		}
		stick = xTaskGetTickCount();
		ani_playing = 1;

		size = *(uint32_t*)ani_frame_addr;

		if (xVideoDisplayBufTake(pdMS_TO_TICKS(10)) == pdTRUE) {
			if(!mfc_handle)
				mfc_handle = mfc_init(RAW_STRM_TYPE_JPEG);
			if(!mfc_handle) {
				printf("%s, mfc_init failed.\n", __func__);
				continue ;
			}

			display_addr = ulVideoDisplayBufGet();

			JpegHeaderInfo jpegInfo = {0};
			jpegInfo.handle = mfc_handle;
			jpegInfo.jpg_addr = ani_frame_addr + 8;
			jpegInfo.jpg_size = size;
			jpegInfo.dec_addry = display_addr;
			jpegInfo.dec_size = ulVideoDisplayBufGetSize();

#if LCD_ROTATE_ANGLE != LCD_ROTATE_ANGLE_0
			if(!tmp_addr) {
				tmp_addr = (u32 *)pvPortMalloc(jpegInfo.dec_size);
				if(!tmp_addr) {
					printf("tmp_addr pvPortMalloc failed.\n");
				}
			}
			if(tmp_addr) {
				jpegInfo.dec_addry = VIRT_TO_PHY((u32)tmp_addr);
			}
#endif
			if (mfc_jpegdec(&jpegInfo) != 0) {
				printf("animation decode %d frame fail.\n", ani_frames);
				vVideoDisplayBufFree(display_addr);
			} else {
				int pxp_format = PXP_SRC_FMT_YUV2P420;
				int lcd_format = LCD_OSD_FORAMT_Y_UV420;

				/* avoid warnning */
				(void) pxp_format;
				(void) lcd_format;

				/* YUV format only support y_uv420/y_u_v420/y_uv422/y_u_v422 */
				if (jpegInfo.dec_format == JPEGDEC_YCbCr420_SEMIPLANAR) {
					pxp_format = PXP_SRC_FMT_YUV2P420;
					lcd_format = LCD_OSD_FORAMT_Y_UV420;
				} else if (jpegInfo.dec_format == JPEGDEC_YCbCr422_SEMIPLANAR) {
					pxp_format = PXP_SRC_FMT_YUV2P422;
					lcd_format = LCD_OSD_FORAMT_Y_UV422;
				} else {
					printf("%s Invalid jpegdec format.\n", __func__);
					goto next_frame;
				}

				if(!first_show_done) {
#if LCD_ROTATE_ANGLE == LCD_ROTATE_ANGLE_0
					int align_width = (ANIMA_LCD_WIDTH + 0xF) & (~0xF);
					int align_height = (ANIMA_LCD_HEIGHT + 0xF) & (~0xF);

					if((align_width == jpegInfo.dec_width) && (align_height == jpegInfo.dec_height)) {
						ark_lcd_set_osd_size(LCD_VIDEO_LAYER, ANIMA_LCD_WIDTH, ANIMA_LCD_HEIGHT);
					} else {
						ark_lcd_set_osd_size(LCD_VIDEO_LAYER, jpegInfo.dec_width, jpegInfo.dec_height);
					}
					ark_lcd_set_osd_format(LCD_VIDEO_LAYER, lcd_format);
#else
					ark_lcd_set_osd_size(LCD_VIDEO_LAYER, ANIMA_LCD_WIDTH, ANIMA_LCD_HEIGHT);
					ark_lcd_set_osd_format(LCD_VIDEO_LAYER, LCD_OSD_FORAMT_RGB565);
#endif
					printf("animation first frame show!\r\n");
					ark_lcd_osd_enable(LCD_VIDEO_LAYER, 1);
					ark_lcd_osd_enable(LCD_UI_LAYER, 0);
					first_show_done = 1;
				}
#if LCD_ROTATE_ANGLE == LCD_ROTATE_ANGLE_0
				ark_lcd_set_osd_yaddr(LCD_VIDEO_LAYER, jpegInfo.dec_addry);
				ark_lcd_set_osd_uaddr(LCD_VIDEO_LAYER, jpegInfo.dec_addru);
				ark_lcd_set_osd_vaddr(LCD_VIDEO_LAYER, jpegInfo.dec_addrv);
#else
				pxp_scaler_rotate(jpegInfo.dec_addry, jpegInfo.dec_addru, jpegInfo.dec_addrv,
					pxp_format, jpegInfo.dec_width, jpegInfo.dec_height,
					display_addr, 0, PXP_OUT_FMT_RGB565, LCD_WIDTH, LCD_HEIGHT, LCD_ROTATE_ANGLE);
				ark_lcd_set_osd_yaddr(LCD_VIDEO_LAYER, display_addr);
#endif
				ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
				ark_lcd_set_osd_sync(LCD_UI_LAYER);
				ark_lcd_wait_for_vsync();
				vVideoDisplayBufRender(display_addr);
//				ani_take_vdisbuf = 1;
			}
		}

next_frame:
		ani_frame_addr += size;
		ani_display_index = !ani_display_index;

		if (ani_stop || (++ani_frames >= aniheader->aniCount + aniheader->hasBootlogo ? 1 : 0)
#if ANIMATION_POLICY == ANIMATION_USE_SMALL_MEM
			|| (ani_frame_addr > anioffset + anisize))
#else
			|| (ani_frame_addr > (uint32_t)aniheader + anisize))
#endif
		{
			printf("animation end.\n");
			ani_stop = 0;
			if (aniheader->aniCount > 0)
				vTaskDelay(aniheader->aniDelayHideTime);
			else if (aniheader->hasBootlogo)
				vTaskDelay(aniheader->bootlogoDisplayTime);
//			if (ani_take_vdisbuf) {
				ark_lcd_osd_enable(LCD_VIDEO_LAYER, 0);
				ark_lcd_osd_enable(LCD_UI_LAYER, 1);
				ark_lcd_set_osd_sync(LCD_UI_LAYER);
				ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
//			}
			vVideoDisplayBufFree(display_addr);
			vVideoDisplayBufGive();
			ani_playing = 0;
			if (ani_replay)
				xTaskNotifyGive(animation_task);
			xSemaphoreGive(animation_mutex);

			if(mfc_handle) {
				mfc_uninit(mfc_handle);
				mfc_handle = NULL;
			}
			if(tmp_addr) {
				vPortFree(tmp_addr);
				tmp_addr = NULL;
			}
			continue;
		}

		vVideoDisplayBufGive();
		xSemaphoreGive(animation_mutex);
		etick = xTaskGetTickCount();

		if (aniheader->hasBootlogo && ani_frames == 1)
			delaytick = pdMS_TO_TICKS(aniheader->bootlogoDisplayTime) > (etick - stick) ?
				pdMS_TO_TICKS(aniheader->bootlogoDisplayTime) - (etick - stick) : 0;
		else
			delaytick = pdMS_TO_TICKS(1000 / aniheader->aniFps) > (etick - stick) ?
				pdMS_TO_TICKS(1000 / aniheader->aniFps) - (etick - stick) : 0;

		if (delaytick)
			vTaskDelay(delaytick);

		xTaskNotifyGive(animation_task);
	}
}

int animation_init(void)
{
	if (animation_task) {
		printf("animation is already inited.\n");
		return 0;
	}

	animation_mutex = xSemaphoreCreateMutex();

	/* Create a task to play animation */
	//if (xTaskCreate(h264_decode_test_thread, "h264_decode_test", configMINIMAL_STACK_SIZE*2, NULL,
	if (xTaskCreate(animation_thread, "animation", configMINIMAL_STACK_SIZE, NULL,
		configMAX_PRIORITIES - 2, &animation_task) != pdPASS) {
		printf("create animation task fail.\n");
		animation_task = NULL;
		return -1;
	}

	return 0;
}

void animation_start(void)
{
	xSemaphoreTake(animation_mutex, portMAX_DELAY);
	first_show_done=0;
	if (ani_playing) {
		printf("animation is already playing.\n");
		xSemaphoreGive(animation_mutex);
		return;
	}

#if ANIMATION_POLICY == ANIMATION_USE_SMALL_MEM
	ani_frame_addr = anioffset + sizeof(BANIHEADER);
#else
	ani_frame_addr = (uint32_t)aniheader + sizeof(BANIHEADER);
#endif
	ani_frames = 0;
	if (animation_task)
		xTaskNotifyGive(animation_task);

	xSemaphoreGive(animation_mutex);
}

/* replay animation even if animation is already playing */
void animation_restart(void)
{
	xSemaphoreTake(animation_mutex, portMAX_DELAY);
	first_show_done=0;
	ani_stop=0;
	if (ani_playing) {
		ani_replay = 1;
		xSemaphoreGive(animation_mutex);
		return;
	}

#if ANIMATION_POLICY == ANIMATION_USE_SMALL_MEM
	ani_frame_addr = anioffset + sizeof(BANIHEADER);
#else
	ani_frame_addr = (uint32_t)aniheader + sizeof(BANIHEADER);
#endif
	ani_frames = 0;
	if (animation_task)
		xTaskNotifyGive(animation_task);

	xSemaphoreGive(animation_mutex);
}

void animation_stop(void)
{
	xSemaphoreTake(animation_mutex, portMAX_DELAY);
	ani_stop = 1;
	xSemaphoreGive(animation_mutex);
}

int get_animation_status(void)
{
	return ani_playing;
}
