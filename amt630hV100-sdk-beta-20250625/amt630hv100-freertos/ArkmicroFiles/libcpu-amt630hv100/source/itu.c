#include "FreeRTOS.h"
#include "board.h"
#include "chip.h"

#ifdef REVERSE_TRACK
#include "vg_driver.h"
#endif

#define ITU656IN_MODULE_EN							0x00
#define ITU656IN_IMR								0x04
#define ITU656IN_ICR								0x08
#define ITU656IN_ISR								0x0C
#define ITU656IN_LINE_NUM_PER_FIELD					0x10
#define ITU656IN_PIX_NUM_PER_LINE					0x14
#define ITU656IN_PIX_LINE_NUM_DELTA					0x18
#define ITU656IN_INPUT_SEL							0x1c
#define ITU656IN_SEP_MODE_SEL						0x20
#define ITU656IN_H_STRAT							0x24
#define ITU656IN_H_END								0x28
#define ITU656IN_H_WIDTH							0x2c
#define ITU656IN_V_START_0							0x30
#define ITU656IN_V_END_0							0x34
#define ITU656IN_V_START_1							0x38
#define ITU656IN_V_END_1							0x3C
#define ITU656IN_V_FIELD_0							0x40
#define ITU656IN_V_FIELD_1							0x44
#define ITU656IN_P_N_DETECT							0x48
#define ITU656IN_ENABLE_REG							0x4c
#define ITU656IN_HFZ								0x50
#define ITU656IN_SIZE								0x54
#define ITU656IN_TOTAL_PIX							0x58
#define ITU656IN_DRAM_DEST1							0x5c
#define ITU656IN_DRAM_DEST2							0x60
#define ITU656IN_TOTAL_PIX_OUT						0x64
#define ITU656IN_OUTLINE_NUM_PER_FIELD				0x68
#define ITU656IN_H_cut_num							0x6c
#define ITU656IN_V_cut_num							0x70
#define ITU656IN_DATA_ERROR_MODE					0x74
#define ITU656IN_MIRR_SET							0x78
#define ITU656IN_RESET								0x7c
#define ITU656IN_OUTPUT_TYPE						0x80
#define ITU656IN_PN_DETCET							0x84
#define ITU656IN_YUV_TYPESEL						0x88


#define ITU656IN_POP_ERR_INT						(1 << 10)
#define ITU656IN_SLICE_INT							(1 << 9)

#ifndef VIN_SMALL_MEM
#define ITU656IN_BUF_NUM		3
#else
#define ITU656IN_BUF_NUM		2
#endif

#define ITUFRAME_MAX_WIDTH		VIN_WIDTH
#define ITUFRAME_MAX_HEIGHT		VIN_HEIGHT

typedef struct {
	uint32_t yaddr;
	uint32_t uvaddr;
	int status;
} ItuBufInfo;

static uint32_t itubase = REGS_ITU_BASE;
static ItuBufInfo itubuf_info[ITU656IN_BUF_NUM] = {0};
static ItuConfigPara itu_para;
static TaskHandle_t itu_task;
static SemaphoreHandle_t itu_mutex = NULL;
static void *itu_buf = NULL;
static int itu_enable = 0;
static int itu_take_video = 0;

static void itu_push_addr(void)
{
	int i;
	
	for (i = 0; i < ITU656IN_BUF_NUM; i++) {
		if (!itubuf_info[i].status) {
			writel(itubuf_info[i].yaddr, itubase + ITU656IN_DRAM_DEST1);
			writel(itubuf_info[i].uvaddr, itubase + ITU656IN_DRAM_DEST1);
			itubuf_info[i].status = 1;
			break;
		}
	}	
}

static void itu_free_addr(uint32_t yaddr)
{
	int i;
	
	for (i = 0; i < ITU656IN_BUF_NUM; i++) {
		if (itubuf_info[i].yaddr == yaddr) {
			itubuf_info[i].status = 0;
			break;
		}
	}
}

static void itu_interupt_handler(void *param)
{
	unsigned int status = readl(itubase + ITU656IN_ISR);
	unsigned int yaddr;

	//printf("itu int status = 0x%x.\n", status);
	
	writel(status, itubase + ITU656IN_ICR);

	if (status & ITU656IN_POP_ERR_INT) {
		printf("itu pop err.\n");
		itu_push_addr();
	}

	if (status & ITU656IN_SLICE_INT) {
		yaddr = readl(itubase + ITU656IN_DRAM_DEST1);
		//printf("yaddr 0x%x.\n", yaddr);
		itu_free_addr(yaddr);
		itu_push_addr();
		xTaskNotifyFromISR(itu_task, yaddr, eSetValueWithOverwrite, 0);
	}
}

int itu_config(ItuConfigPara *para)
{
	uint32_t val;
	int ret = 0;
	int i;

	configASSERT(para->in_width <= ITUFRAME_MAX_WIDTH &&
		para->in_height <= ITUFRAME_MAX_HEIGHT);

	xSemaphoreTake(itu_mutex, portMAX_DELAY);
	
	writel(0xffffffff, itubase + ITU656IN_ICR);
	writel(1 << 2, itubase + ITU656IN_MODULE_EN);

	val = readl(itubase + ITU656IN_SEP_MODE_SEL);
	writel(val | (1 << 3), itubase + ITU656IN_SEP_MODE_SEL); //控制YUV顺序
	writel(0, itubase + ITU656IN_IMR);
	if (para->itu601) {
		val = readl(itubase + ITU656IN_SEP_MODE_SEL);
		val &= ~(1 << 2);
		writel(val, itubase + ITU656IN_SEP_MODE_SEL);
		writel(0, itubase + ITU656IN_INPUT_SEL);
		val = readl(itubase + ITU656IN_MODULE_EN);	
		writel(val | 1, itubase + ITU656IN_MODULE_EN);
	} else {
		writel(1, itubase + ITU656IN_INPUT_SEL);
	}
	writel((5 << 1), itubase + ITU656IN_ENABLE_REG);
	writel(0, itubase + ITU656IN_MIRR_SET); //镜像
	writel(para->yuv_type & 1, itubase + ITU656IN_YUV_TYPESEL); //0:y_uv 1:yuyv
#ifndef VIN_SMALL_MEM
	writel(para->out_format & 1, itubase + ITU656IN_OUTPUT_TYPE);  //0:420 1:422
#else
	writel(ITU_YUV420, itubase + ITU656IN_OUTPUT_TYPE);  //0:420 1:422
#endif

	writel(para->in_width << 16, itubase + ITU656IN_SIZE);
	//奇偶场中断(P:720*288,N:720*240)
	writel(para->in_width * para->in_height, itubase + ITU656IN_TOTAL_PIX_OUT);
	//slice 中断(P:720*288,N:720*240)
	writel(para->in_width * para->in_height, itubase + ITU656IN_TOTAL_PIX);
	writel(para->in_height, itubase + ITU656IN_OUTLINE_NUM_PER_FIELD);
	
	//reset fifo
	val = readl(itubase + ITU656IN_RESET);
	val |= 1 << 1;
	writel(val, itubase + ITU656IN_RESET);
	udelay(10);
	val &= ~(1 << 1);
	writel(val, itubase + ITU656IN_RESET);

	//push addr
	for(i = 0; i < ITU656IN_BUF_NUM; i++) {
#ifndef VIN_SMALL_MEM
		if (para->out_format == ITU_YUV422)
			itubuf_info[i].yaddr = (uint32_t)itu_buf + para->in_width * para->in_height * 2 * i;
		else
#endif
			itubuf_info[i].yaddr = (uint32_t)itu_buf + para->in_width * para->in_height * 3 / 2 * i;
		if (itubuf_info[i].yaddr == 0) {
			printf("no enough memory for itu frames.\n");
			ret = -ENOMEM;
			goto end;
		}
		itubuf_info[i].uvaddr = itubuf_info[i].yaddr + para->in_width * para->in_height;
			
		writel(itubuf_info[i].yaddr, itubase + ITU656IN_DRAM_DEST1);
		writel(itubuf_info[i].uvaddr, itubase + ITU656IN_DRAM_DEST1);
		itubuf_info[i].status = 1;
	}

	/* enable slice int */
	writel(ITU656IN_POP_ERR_INT | ITU656IN_SLICE_INT, itubase + ITU656IN_IMR);

	memcpy(&itu_para, para, sizeof(itu_para));
end:
	xSemaphoreGive(itu_mutex);

	return ret;
}

void itu_start(void)
{
	uint32_t val;

	//enable write data
	xSemaphoreTake(itu_mutex, portMAX_DELAY);
	val = readl(itubase + ITU656IN_MODULE_EN);
	val &= ~(1 << 2);
	writel(val, itubase + ITU656IN_MODULE_EN);
	val = readl(itubase + ITU656IN_ENABLE_REG);
	writel(val | 1, itubase + ITU656IN_ENABLE_REG);
	itu_enable = 1;
	xSemaphoreGive(itu_mutex);
}

void itu_stop(void)
{
	uint32_t val;
	
	//disable write data
	xSemaphoreTake(itu_mutex, portMAX_DELAY);
	val = readl(itubase + ITU656IN_MODULE_EN);
	val |= (1 << 2);
	writel(val, itubase + ITU656IN_MODULE_EN);
	val = readl(itubase + ITU656IN_ENABLE_REG);
	writel(val & ~1, itubase + ITU656IN_ENABLE_REG);
	itu_enable = 0;
#ifndef REVERSE_UI
	ark_lcd_osd_enable(LCD_VIDEO_LAYER, 0);
	ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
	ark_lcd_osd_enable(LCD_OSD1, 1);
	ark_lcd_set_osd_sync(LCD_OSD1);
#endif
	xTaskNotify(itu_task, 0, eSetValueWithOverwrite);
	xSemaphoreGive(itu_mutex);
}

void itu_display_thread(void *param)
{
	uint32_t ulNotifiedValue;
	uint32_t yaddr, uvaddr, dstaddr;
	int format;
		
	for (;;) {
		xTaskNotifyWait( 0x00, /* Don't clear any notification bits on entry. */
			0xffffffff, /* Reset the notification value to 0 on exit. */
			&ulNotifiedValue, /* Notified value pass out in ulNotifiedValue. */
			portMAX_DELAY);
		//printf("received address 0x%x.\n", ulNotifiedValue);

		xSemaphoreTake(itu_mutex, portMAX_DELAY);
		if (!itu_enable) {
			xSemaphoreGive(itu_mutex);
			vVideoDisplayBufFree(dstaddr);
			/* 倒车显示优先级高，等倒车结束之后才释放视频显示资源 */
			if (itu_take_video) {
				vVideoDisplayBufGive();
				itu_take_video = 0;
			}
			continue;
		}

		/* 获取视频显示资源 */
		if (!itu_take_video) {
			xVideoDisplayBufTake(portMAX_DELAY);
			itu_take_video = 1;
		}

		if (!ark_lcd_get_osd_info_atomic_isactive(LCD_VIDEO_LAYER)) {
			ark_lcd_wait_for_vsync();
		}

		/* 通过该接口获取视频显示共用的缓存可以避免切换过程中的黑屏、
		   花屏等问题 */
		dstaddr = ulVideoDisplayBufGet();
		
		yaddr = ulNotifiedValue;
		uvaddr = yaddr + itu_para.in_width * itu_para.in_height;
#ifndef VIN_SMALL_MEM
		if (itu_para.out_format == ITU_YUV422)
			format = PXP_SRC_FMT_YUV2P422;
		else
#endif
			format = PXP_SRC_FMT_YUV2P420;
		pxp_scaler_rotate(yaddr, uvaddr, 0, format, itu_para.in_width, itu_para.in_height,
			dstaddr, 0, PXP_OUT_FMT_RGB565, itu_para.out_width, itu_para.out_height, LCD_ROTATE_ANGLE);
#ifdef REVERSE_TRACK
		extern int get_reverse_track_index(void);
		static int index = 58;
		index = get_reverse_track_index();
		xm_vg_set_gpu_fb_addr(dstaddr);
		xm_vg_draw_prepare(&index);
		xm_vg_draw_start();
#endif
		LcdOsdInfo info = {0};
		info.x = itu_para.out_x;
		info.y = itu_para.out_y;
		info.width = itu_para.out_width;
		info.height = itu_para.out_height;
		info.format = LCD_OSD_FORAMT_RGB565;
		info.yaddr = dstaddr;
		ark_lcd_set_osd_info_atomic(LCD_VIDEO_LAYER, &info);
		ark_lcd_osd_enable(LCD_VIDEO_LAYER, 1);
		ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
#ifndef REVERSE_UI
		ark_lcd_osd_enable(LCD_OSD1, 0);
		ark_lcd_set_osd_sync(LCD_OSD1);
#endif
		vVideoDisplayBufRender(dstaddr);
		xSemaphoreGive(itu_mutex);
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

int itu_init(void)
{
	sys_soft_reset(softreset_itu);

	itu_mutex = xSemaphoreCreateMutex();

#ifndef VIN_SMALL_MEM
	itu_buf = pvPortMalloc(ITUFRAME_MAX_WIDTH * ITUFRAME_MAX_HEIGHT * 2 * ITU656IN_BUF_NUM);
#else
	itu_buf = pvPortMalloc(ITUFRAME_MAX_WIDTH * ITUFRAME_MAX_HEIGHT * 3 / 2 * ITU656IN_BUF_NUM);
#endif
	if (!itu_buf) {
		printf("Itu malloc memory fail.\n");
		return -ENOMEM;
	}

	if (xTaskCreate(itu_display_thread, "itudis", configMINIMAL_STACK_SIZE, NULL,
		configMAX_PRIORITIES - 2, &itu_task) != pdPASS) {
		printf("create itu display task fail.\n");
		return -1;
	}

	request_irq(ITU_IRQn, 0, itu_interupt_handler, NULL);

	return 0;
}
