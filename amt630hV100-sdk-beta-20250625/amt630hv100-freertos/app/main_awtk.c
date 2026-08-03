/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include <unistd.h>
#if ENABLE_BD_USB_DVR_FUNC
#include <string.h>
#endif
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "board.h"
#include "chip.h"
#include "animation.h"
#include "sfud.h"
#include "romfile.h"
#include "updatefile.h"
#include "sysinfo.h"
#include "mmcsd_core.h"
#include "ff_stdio.h"

#if ENABLE_BD_USB_DVR_FUNC
#include "lcd.h"
#include "mfcapi.h"
#include "pxp.h"
#include "jpegdecapi.h"
#endif

#ifdef WIFI_SUPPORT
#include "carlink_ey.h"
#include "carlink_ec.h"
#include "ark_network.h"
#endif

#ifdef USE_ULOG
#ifdef ULOG_BACKEND_USING_CONSOLE
extern int ulog_console_backend_init(void);
#endif
#ifdef ULOG_EASYFLASH_BACKEND_ENABLE
#include "easyflash.h"
#include "ulog_easyflash.h"
#endif
#ifdef ULOG_FILE_BACKEND_ENABLE
#include "ulog_file.h"
#endif
#endif

#ifdef OTA_UPDATE_SUPPORT
#include "ota_update.h"
#endif

#include "config/hcn_config.h"
#include "mw_init/hcn_mw_init.h"
#include "msg_manage/hcn_msg_manage.h"
#include "dashboard_state/hcn_dev_state.h"
#include "uart_mcu_update/hcn_uart_mcu_update.h"

#ifdef HCN_ADC_KEY_ENABLE
#include "key_module/hcn_adc_key.h"
#endif

#ifdef HCN_WIFI_INIT_DELAY_ENABLE
#include "hal_wifi/hal_wifi.h"
#endif

#define WIFI_TEST			0
#define BT_TEST				0
#define SDMMC_TEST			0
#define USB_DEV_PLUGED		0
#define USB_DEV_UNPLUGED	1
//#define TASK_STATUS_MONITOR

#if ENABLE_BD_USB_DVR_FUNC
// USB DVR related defines
#define BD_CTRL_GET_ID          0x00
#define BD_CTRL_REC_START        0x01
#define BD_CTRL_REC_STOP        0x02
#define BD_CTRL_SNAP            0x03
#define BD_CTRL_SOS             0x04
#define BD_CTRL_GET_LIST        0x05
#define BD_CTRL_PB_START        0x06
#define BD_CTRL_PB_PAUSE        0x07    /*播放暂停*/
#define BD_CTRL_PB_STOP         0x08    /*播放停止*/
#define BD_CTRL_GET_STS         0x09
#define BD_CTRL_REC_TICK        0x0A    /*记录仪当前的录像秒数*/
#define BD_CTRL_SET_RES         0x0B    /*记录仪的录像分辨率 - 0:1080P,1:720P - 预留一般是默认不可选*/
#define BD_CTRL_SET_REC_TIME    0x0C    /*循环录像时长 1:1min,2:2min,3:3min,发送 0:(代表获取当前固件的对应设置)*/
#define BD_CTRL_MIC_ON          0x0D    /*MIC开关,0:off,1:on*/
#define BD_CTRL_STAMP_ON        0x0E    /*时间水印开关,0:off,1:on - 预留,默认开*/
#define BD_CTRL_KEY_ON          0x0F    /*预留*/
#define BD_CTRL_OFF_TIME        0x10    /*预留*/
#define BD_CTRL_FORMAT          0x11    /*格式化卡,需要先停止录像*/
#define BD_CTRL_DEFAULT         0x12    /*恢复出厂设置,需先停止录像*/
#define BD_CTRL_SET_TIME        0x13    /*设置时间*/
#define BD_CTRL_DEL_FILE        0x14    /*删除指定index的文件,一般文件列表界面操作*/
#define BD_CTRL_LOCK_FILE       0x15    /*加锁指定index的文件，一般文件列表界面操作*/
#define BD_CTRL_UNLOCK_FILE     0x16    /*解锁指定index的文件，一般文件列表界面操作*/
#define BD_CTRL_TOTAL_TIME      0x17    /*返回指定index录像文件的总时长,一般回放文件时使用*/
#define BD_CTRL_PB_FF           0x1e    /*回放视频快进*/
#define BD_CTRL_PB_FB           0x1f    /*回放视频快退*/
#define BD_CTRL_SENSOR_SEL      0x22   /*多路切换预览显示 0:前路,1:后路*/
#define BD_ANDROID_VIEW_SWITCH  0x9c    /*多路切换预览显示 0:前路,1:后路*/
#define BD_CTRL_GET_TF_CAPACITY  0x38    /* Query TF card total/free capacity (reply: 8B LE: total[0..3] free[4..7]) */

#define BD_MAX_DATA_LEN         (60*1024)
#define BD_FILE_JPG_BIT         0x8000
#define BD_FILE_BACK_BIT        0x4000
#define BD_HEADER_LEN           10
#define BD_JPG_OFFSET           17
#define JPG_FILE_NAME           "/usb/elene"
#define FIXED_BUF_SIZE          600

// DVR debug print control: 1=enable print, 0=disable print
#define DVR_DEBUG_PRINT         0
// JPEG decode debug print control: 1=enable, 0=disable (default off)
#define JPEG_DEBUG_PRINT        0
// DVR framerate statistics control: 1=enable, 0=disable
#define DVR_FRAMERATE_PRINT     1

typedef struct tag_st_bd_ctrl_if {
    uint16_t header_id;
    uint16_t cmd_id;
    uint16_t cmd_par;
    uint16_t data_len;
    uint16_t checksum;
    uint8_t trans_buf[BD_MAX_DATA_LEN];
} st_bd_ctrl_if_t;

typedef struct tag_dvr_capture {
    FF_FILE *h_cap;
    uint8_t *cap_blk_buf;
    uint8_t *cap_jpg_buf;
    uint32_t blksize;
    uint32_t file_size;
    st_bd_ctrl_if_t cap_ctrl;
    uint8_t running;
    uint8_t round_flag;
    uint32_t read_times;
    // Decode and display related
    void *mfc_handle;
    uint8_t *yuv_buf;
    uint8_t *dst_buf[2];  // Double buffer for display
    uint8_t dst_buf_index;  // Current display buffer index
    uint32_t yuv_buf_size;
    uint32_t dst_buf_size;
    uint8_t display_on;
    uint32_t current_jpg_size;  // Store current jpg size for decode
} st_dvr_capture_t;

// DVR control variables
static st_bd_ctrl_if_t dvr_ctrl_data;
static st_dvr_capture_t dvr_capture;
static TaskHandle_t dvr_task_handle = NULL;
static FF_FILE *dvr_fp = NULL;  // Global file pointer for elene
static uint8_t dvr_need_send_data = 0;  // Flag to indicate data needs to be sent

// DVR status variables
static uint8_t dvr_sd_status = 0;      // 0: no SD, 1: SD present
static uint8_t dvr_rec_status = 0;    // 0: not recording, 1: recording
static uint8_t dvr_lock_status = 0;   // 0: not locked, 1: locked
static uint8_t dvr_mic_status = 0;    // 0: mic off, 1: mic on
static uint8_t dvr_sd_error = 0;      // 0: normal, 1: error
static uint8_t dvr_sd_full = 0;        // 0: not full, 1: full
static uint8_t dvr_sensor_switch_enable = 1;  // enable auto sensor switch
static uint8_t dvr_view_mode = 0;  // 0:front, 1:rear, 2:f+r, 3:r+f, 4:hzh

// TF card capacity cache — stored in KiB to avoid uint32_t overflow on >4GiB cards.
static volatile uint32_t dvr_tf_total_kib = 0;
static volatile uint32_t dvr_tf_free_kib  = 0;
static volatile uint8_t  dvr_tf_capacity_valid = 0;

// DEL_FILE acknowledgement flag: set to 1 when DVR replies to a DEL_FILE command.
// Cleared by dvr_api_clear_del_ack() before sending DEL, polled by UI after.
static volatile uint8_t  dvr_del_ack_received = 0;

// DVR firmware version string cache (populated by BD_CTRL_GET_ID response).
// NUL-terminated. 32 bytes covers typical firmware version strings; truncated if longer.
#define DVR_VERSION_MAX_LEN  32
static volatile uint8_t  dvr_version_valid = 0;             /* 0=no response yet, 1=have a sample */
static char               dvr_version_buf[DVR_VERSION_MAX_LEN];

// File list related
#define BYTE_PER_FILE  6
static uint16_t dvr_video_list_count = 0;
static uint16_t dvr_photo_list_count = 0;

/*
 * Filename cache: the last fetched file list for each of the four
 * (camera, kind) buckets, populated by dvr_filelist_parser().
 *
 *   is_jpg = cmd_par & 0x8000   (0=video .avi, 1=photo .jpg)
 *   is_rear = cmd_par & 0x4000  (0=front, 1=rear)
 *
 * Names are kept as NUL-terminated strings, e.g. "MOVI0001.avi",
 * "LOCK0007.avi", "PICT0003.jpg".  The arrays are global so other
 * modules can read them via the dvr_api_*_list_* accessors.
 */
#define DVR_NAME_MAX         16   /* incl. trailing NUL */
#define DVR_VIDEO_LIST_MAX   128  /* must stay <= DVR_FETCH_MAX in dvr_view.c; 128 covers a full 744B GET_LIST (124 files) */
#define DVR_PHOTO_LIST_MAX   128  /* must stay <= DVR_FETCH_MAX in dvr_view.c */

static char dvr_video_list_f[DVR_VIDEO_LIST_MAX][DVR_NAME_MAX];
static uint16_t dvr_video_list_f_count = 0;
static char dvr_video_list_r[DVR_VIDEO_LIST_MAX][DVR_NAME_MAX];
static uint16_t dvr_video_list_r_count = 0;
static char dvr_photo_list_f[DVR_PHOTO_LIST_MAX][DVR_NAME_MAX];
static uint16_t dvr_photo_list_f_count = 0;
static char dvr_photo_list_r[DVR_PHOTO_LIST_MAX][DVR_NAME_MAX];
static uint16_t dvr_photo_list_r_count = 0;

// Display window configuration (for external control)
static int32_t dvr_display_x = 0;       // X coordinate of display window
static int32_t dvr_display_y = 0;       // Y coordinate of display window
static int32_t dvr_display_width = LCD_WIDTH;   // Width of display window
static int32_t dvr_display_height = LCD_HEIGHT; // Height of display window

// Preview mode control flag (for skip jpeg decode when UI not in preview)
static uint8_t dvr_preview_enable = 0;  // 0: skip jpeg decode, 1: enable jpeg decode

// Framerate statistics
static uint32_t dvr_frame_count = 0;
static uint32_t dvr_frame_count_last = 0;
static uint32_t dvr_last_stat_time = 0;
static uint32_t dvr_fps = 0;

// FPS print toggle (runtime, default ON). Controls whether the per-second
// `DVR: [FPS:xx] frame_count=xx` log line is emitted. Toggled via CLI.
static uint8_t dvr_fps_print_enable = 0;

// File list ready flag: set by dvr_filelist_parser() after USB response,
// cleared by UI thread after reading. This bridges the async gap between
// dvr_api_get_list() (fire) and dvr_file_list_populate() (consume).
static volatile uint8_t dvr_filelist_ready_flag = 0;

// Skip JPEG process flag (for file list operations)
static uint8_t dvr_skip_jpeg_process = 0;

// Auto-record: set to 1 by dvr_usb_task after init. When the first
// GET_STS response arrives with SD=1, rec_start is sent and flag cleared.
// This ensures recording starts only after the DVR chip has initialized
// its SD card, not before the USB elene file is even open.
static uint8_t dvr_auto_rec_pending = 0;

#endif
int carlink_aa_init();
int carlink_cp_init();

extern int usb_wait_stor_dev_pluged(uint32_t timeout);
extern void hub_usb_dev_reset(void);
extern int xm_vg_init (unsigned int heap_addr, unsigned int size);
#ifdef DELTA_UPDATE_SUPPORT
extern int delta_update(int filetype, size_t patchFileSize);
#endif

#ifdef AWTK

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
#if ENABLE_BD_USB_DVR_FUNC
static void dvr_filelist_parser(uint8_t *buf, int32_t len, uint16_t cmd_par);
void dvr_get_file_list(uint8_t mode);
void dvr_get_status(void);
static void dvr_pb_start(uint8_t mode, uint16_t index);
static void dvr_pb_pause(void);
static void dvr_pb_stop(void);
static void dvr_pb_ff(void);
static void dvr_pb_fb(void);
static void dvr_tf_capacity_parser(uint8_t *buf, int32_t len);
#endif
/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *  STATIC FUNCTIONS
 **********************/
#if ENABLE_BD_USB_DVR_FUNC

/*
 * Post-render alpha clear: called by VG Canvas AFTER all windows are
 * painted but BEFORE the framebuffer becomes visible.
 *
 * With navigator_to(), home_page stays alive underneath dvr_page.
 * AWTK renders home_page first (opaque wallpaper fills alpha=0xFF),
 * then dvr_page widgets on top.  This hook clears alpha in the DVR
 * preview rectangle so the VIDEO layer (OSD0) shows through.
 *
 * Registered via vg_set_post_render_hook() when preview is enabled;
 * unregistered when preview is disabled.
 */
extern void vg_set_post_render_hook(void (*hook)(unsigned int fb_base));

static uint32_t g_alpha_hook_call_count = 0;

static void dvr_alpha_clear_hook(unsigned int fb_base)
{
    if (!dvr_preview_enable) return;

    g_alpha_hook_call_count++;

    int x0 = dvr_display_x;
    int y0 = dvr_display_y;
    int x1 = x0 + dvr_display_width;
    int y1 = y0 + dvr_display_height;
    if (x1 > OSD_WIDTH)  x1 = OSD_WIDTH;
    if (y1 > OSD_HEIGHT) y1 = OSD_HEIGHT;

    /* The old code did a per-pixel READ-MODIFY-WRITE here (*p &= 0x00FFFFFF),
     * which measured 74-93ms over the 1024x500 region and stalled the LCD
     * render task for ~5 frames on every UI repaint -- that stall WAS the
     * preview flicker.  The hole rect is identical to the VIDEO layer rect
     * (info.x/y/w/h == dvr_display_*), so the UI-layer RGB under the hole is
     * never visible; we can do a straight 32-bit WRITE of 0x00000000
     * (alpha=0, RGB=0) instead of reading each pixel.  That removes 512K
     * memory reads.  0x00000000 is safe for both straight-alpha and additive
     * blend (RGB=0 contributes nothing either way).
     * If the row is full-width we can fill it as one contiguous run. */
    if (x0 == 0 && x1 == OSD_WIDTH) {
        /* contiguous block: y0..y1 full rows */
        uint32_t *p   = (uint32_t *)fb_base + (uint32_t)y0 * OSD_WIDTH;
        uint32_t *end = (uint32_t *)fb_base + (uint32_t)y1 * OSD_WIDTH;
        while (p < end) *p++ = 0x00000000;
    } else {
        for (int y = y0; y < y1; y++) {
            uint32_t *p = (uint32_t *)fb_base + (uint32_t)y * OSD_WIDTH + x0;
            for (int x = x0; x < x1; x++)
                *p++ = 0x00000000;
        }
    }
    CP15_clean_dcache_for_dma(fb_base + (uint32_t)y0 * OSD_WIDTH * 4,
                               fb_base + (uint32_t)y1 * OSD_WIDTH * 4);
}

/* Restore alpha=0xFF on all framebuffers (called on preview exit) */
static void dvr_restore_ui_alpha_all(void)
{
    for (int i = 0; i < 3; i++) {
        uint8_t *addr = ark_lcd_get_fb_addr(i);
        if (!addr) continue;
        uint32_t *fb = (uint32_t *)addr;
        int x0 = dvr_display_x, y0 = dvr_display_y;
        int x1 = x0 + dvr_display_width, y1 = y0 + dvr_display_height;
        if (x1 > OSD_WIDTH)  x1 = OSD_WIDTH;
        if (y1 > OSD_HEIGHT) y1 = OSD_HEIGHT;
        for (int y = y0; y < y1; y++) {
            uint32_t *p = fb + y * OSD_WIDTH + x0;
            for (int x = x0; x < x1; x++)
                *p++ |= 0xFF000000;
        }
        CP15_clean_dcache_for_dma((uint32_t)addr + y0 * OSD_WIDTH * 4,
                                   (uint32_t)addr + y1 * OSD_WIDTH * 4);
    }
}

// Send data to DVR via elene file (write 512 bytes aligned)
// Caller should call dvr_seek_for_align() before this function
static void dvr_send_data(void)
{
    if (dvr_fp == NULL) {
        printf("DVR: elene file not opened for write\n");
        return;
    }

    ff_fwrite(&dvr_ctrl_data, 1, 512, dvr_fp);

    printf("DVR: Data sent successfully\n");
}

// Send normal command to DVR
// Sets flag and prepares data, actual sending happens in main loop
void dvr_send_normal_cmd(unsigned short cmd_id, unsigned short cmd_par)
{
    st_bd_ctrl_if_t *pctrl = &dvr_ctrl_data;

    pctrl->header_id = 0xaa55;
    pctrl->cmd_id = cmd_id;
    pctrl->cmd_par = cmd_par;
    pctrl->data_len = 0;
    pctrl->checksum = 0;
    dvr_need_send_data = 1;

    printf("DVR-TX>> cmd=0x%02X par=0x%04X frame[AA 55 %02X %02X %02X %02X 00 00 00 00]\n",
           cmd_id, cmd_par,
           cmd_id & 0xFF, (cmd_id >> 8) & 0xFF,
           cmd_par & 0xFF, (cmd_par >> 8) & 0xFF);
}

// Check if elene file exists on USB using ff_stat
static int dvr_check_elene_exist(void)
{
    FF_Stat_t stat;
    if (ff_stat(JPG_FILE_NAME, &stat) == 0) {
        return 1;
    }
    return 0;
}

// Initialize DVR capture - open elene file and allocate buffers
static int dvr_capture_init(st_dvr_capture_t *cap)
{
    int blksize;

    if (dvr_fp != NULL) {
        printf("DVR: elene already opened\n");
        cap->h_cap = dvr_fp;
    } else {
        dvr_fp = ff_fopen(JPG_FILE_NAME, "rb+");
        if (!dvr_fp) {
            printf("DVR: open elene failed\n");
            return -1;
        }
        cap->h_cap = dvr_fp;
        printf("DVR: elene opened, dvr_fp=0x%x\n", (uint32_t)dvr_fp);
    }

    blksize = 100 * 1024;  // Block size should be 100KB to match elene file format
    cap->blksize = blksize;

    cap->cap_blk_buf = pvPortMalloc(blksize + 512 + 300 * 1024);  // 100KB block + extra space for jpg_size
    if (!cap->cap_blk_buf) {
        printf("DVR: alloc blk_buf failed\n");
        if (dvr_fp && cap->h_cap != dvr_fp) ff_fclose(dvr_fp);
        dvr_fp = NULL;
        return -1;
    }

    cap->cap_jpg_buf = pvPortMalloc(350 * 1024);  // 350KB to handle max jpg_size + header
    if (!cap->cap_jpg_buf) {
        printf("DVR: alloc jpg_buf failed\n");
        vPortFree(cap->cap_blk_buf);
        if (dvr_fp && cap->h_cap != dvr_fp) ff_fclose(dvr_fp);
        dvr_fp = NULL;
        return -1;
    }

    // Initialize align seek variables
    cap->round_flag = 0;
    cap->read_times = 0;
    cap->file_size = ff_filelength(dvr_fp);
    printf("DVR: file size = %u\n", cap->file_size);

    // Initialize MFC for JPEG decode
    cap->mfc_handle = mfc_init(RAW_STRM_TYPE_JPEG);
    if (!cap->mfc_handle) {
        printf("DVR: mfc_init failed\n");
        vPortFree(cap->cap_blk_buf);
        vPortFree(cap->cap_jpg_buf);
        if (dvr_fp && cap->h_cap != dvr_fp) ff_fclose(dvr_fp);
        dvr_fp = NULL;
        return -1;
    }

    // Allocate YUV buffer for decode output (max 1280x720x1.5)
    cap->yuv_buf_size = 1280 * 720 * 2;
    cap->yuv_buf = pvPortMalloc(cap->yuv_buf_size);
    if (!cap->yuv_buf) {
        printf("DVR: yuv_buf alloc failed\n");
        mfc_uninit(cap->mfc_handle);
        vPortFree(cap->cap_blk_buf);
        vPortFree(cap->cap_jpg_buf);
        if (dvr_fp && cap->h_cap != dvr_fp) ff_fclose(dvr_fp);
        dvr_fp = NULL;
        return -1;
    }

    // Allocate display buffers (RGB565, double buffer 1280x720x2)
    cap->dst_buf_size = 1280 * 720 * 2;
    cap->dst_buf[0] = pvPortMalloc(cap->dst_buf_size);
    cap->dst_buf[1] = pvPortMalloc(cap->dst_buf_size);
    if (!cap->dst_buf[0] || !cap->dst_buf[1]) {
        printf("DVR: dst_buf alloc failed\n");
        if (cap->dst_buf[0]) vPortFree(cap->dst_buf[0]);
        if (cap->dst_buf[1]) vPortFree(cap->dst_buf[1]);
        vPortFree(cap->yuv_buf);
        mfc_uninit(cap->mfc_handle);
        vPortFree(cap->cap_blk_buf);
        vPortFree(cap->cap_jpg_buf);
        if (dvr_fp && cap->h_cap != dvr_fp) ff_fclose(dvr_fp);
        dvr_fp = NULL;
        return -1;
    }
    cap->dst_buf_index = 0;

    cap->display_on = 0;
    cap->running = 1;
    printf("DVR: capture init ok, blksize=%d\n", blksize);
    return 0;
}

// Close DVR capture and free resources (does not close global dvr_fp)
static void dvr_capture_deinit(st_dvr_capture_t *cap)
{
    cap->running = 0;
    cap->h_cap = NULL;  // Don't close global fp
    if (cap->cap_blk_buf) {
        vPortFree(cap->cap_blk_buf);
        cap->cap_blk_buf = NULL;
    }
    if (cap->cap_jpg_buf) {
        vPortFree(cap->cap_jpg_buf);
        cap->cap_jpg_buf = NULL;
    }
    if (cap->yuv_buf) {
        vPortFree(cap->yuv_buf);
        cap->yuv_buf = NULL;
    }
    if (cap->dst_buf[0]) {
        vPortFree(cap->dst_buf[0]);
        cap->dst_buf[0] = NULL;
    }
    if (cap->dst_buf[1]) {
        vPortFree(cap->dst_buf[1]);
        cap->dst_buf[1] = NULL;
    }
    if (cap->mfc_handle) {
        mfc_uninit(cap->mfc_handle);
        cap->mfc_handle = NULL;
    }
    cap->display_on = 0;
}

// Close global elene file
static void dvr_close_elene(void)
{
    if (dvr_fp) {
        ff_fclose(dvr_fp);
        dvr_fp = NULL;
        printf("DVR: elene closed\n");
    }
}

// Process received command from DVR
static void dvr_recv_cmd_process(st_bd_ctrl_if_t *pctrl)
{
    switch (pctrl->cmd_id) {
        case BD_CTRL_GET_ID:
            /* Cache the NUL-terminated version string from trans_buf. The
             * string is bounded by data_len (when set) and/or the first NUL
             * we encounter; we always leave one byte for trailing NUL. */
            {
                int32_t copy_len = pctrl->data_len;
                if (copy_len < 0) copy_len = 0;
                if (copy_len > (int32_t)(DVR_VERSION_MAX_LEN - 1)) {
                    copy_len = DVR_VERSION_MAX_LEN - 1;
                }
                /* If DVR sent a long string with no NUL in the first data_len
                 * bytes, force termination by writing a NUL. */
                memcpy(dvr_version_buf, pctrl->trans_buf, (size_t)copy_len);
                dvr_version_buf[copy_len] = '\0';
                dvr_version_valid = 1;
                printf("DVR version: %s\n", dvr_version_buf);
            }
            break;
        case BD_CTRL_GET_STS:
            dvr_sd_status = (pctrl->cmd_par & 0x01) ? 1 : 0;
            dvr_rec_status = (pctrl->cmd_par & 0x02) ? 1 : 0;
            dvr_lock_status = (pctrl->cmd_par & 0x04) ? 1 : 0;
            dvr_sd_error = (pctrl->cmd_par & 0x08) ? 1 : 0;
            dvr_sd_full = (pctrl->cmd_par & 0x10) ? 1 : 0;
            dvr_mic_status = (pctrl->cmd_par & 0x20) ? 1 : 0;
            printf("DVR STATUS: SD=%d Rec=%d Lock=%d Error=%d Full=%d MIC=%d\n",
                   dvr_sd_status, dvr_rec_status, dvr_lock_status,
                   dvr_sd_error, dvr_sd_full, dvr_mic_status);
            /* Auto-record: once SD card is ready and not already recording,
             * start loop recording automatically. Only fires once per
             * USB insertion (dvr_auto_rec_pending is set by task init). */
            if (dvr_auto_rec_pending && dvr_sd_status && !dvr_rec_status) {
                dvr_auto_rec_pending = 0;
                dvr_send_normal_cmd(BD_CTRL_REC_START, 0);
                printf("DVR: auto-rec started (SD ready)\n");
            } else if (dvr_auto_rec_pending && !dvr_sd_status) {
                /* SD not ready yet — re-query after a short delay.
                 * The next FPS tick (~1s) or manual getsts will retry.
                 * Schedule a re-query via the pending-command mechanism. */
                printf("DVR: SD not ready, will retry auto-rec\n");
            } else if (dvr_auto_rec_pending && dvr_rec_status) {
                /* Already recording (e.g. DVR chip auto-started) */
                dvr_auto_rec_pending = 0;
                printf("DVR: already recording, auto-rec skipped\n");
            }
            /* If version is still unknown, piggyback a GET_ID request.
             * GET_STS just arrived so the USB channel is responsive right
             * now — best moment to slip in a GET_ID and catch the reply
             * on the next 2ms read cycle. Only retry while preview is on
             * (USB reads are frequent); stops once version is cached. */
            if (!dvr_version_valid && dvr_preview_enable) {
                dvr_send_normal_cmd(BD_CTRL_GET_ID, 0);
                printf("DVR: version retry (piggyback on GET_STS reply)\n");
            }
            break;
        case BD_CTRL_SENSOR_SEL:
            printf("DVR RECV SENSOR_SEL: cmd_par=0x%02x\n", pctrl->cmd_par);
            break;
        case BD_ANDROID_VIEW_SWITCH:
            printf("DVR RECV VIEW_SWITCH: cmd_par=0x%02x\n", pctrl->cmd_par);
            dvr_view_mode = pctrl->cmd_par & 0x0F;
            break;
        case BD_CTRL_GET_LIST:
            printf("DVR: GET_LIST received, data_len=%d\n", pctrl->data_len);
            dvr_filelist_parser(pctrl->trans_buf, pctrl->data_len, pctrl->cmd_par);
            // Resume JPEG processing after file list response
            dvr_skip_jpeg_process = 0;
            break;
        case BD_CTRL_SET_RES:
            printf("DVR: SET_RES received, res=%d\n", pctrl->cmd_par);
            break;
        case BD_CTRL_SET_REC_TIME:
            printf("DVR: SET_REC_TIME received, time=%d min\n", pctrl->cmd_par);
            break;
        case BD_CTRL_MIC_ON:
            printf("DVR: MIC_ON received, onoff=%d\n", pctrl->cmd_par);
            dvr_mic_status = pctrl->cmd_par;
            break;
        case BD_CTRL_STAMP_ON:
            printf("DVR: STAMP_ON received, onoff=%d\n", pctrl->cmd_par);
            break;
        case BD_CTRL_FORMAT:
            printf("DVR: FORMAT received, status=%d\n", pctrl->cmd_par);
            break;
        case BD_CTRL_DEFAULT:
            printf("DVR: DEFAULT received, status=%d\n", pctrl->cmd_par);
            break;
        case BD_CTRL_SET_TIME:
            printf("DVR: SET_TIME received, status=%d\n", pctrl->cmd_par);
            break;
        case BD_CTRL_DEL_FILE:
            dvr_del_ack_received = 1;
            printf("DVR: DEL_FILE ACK received, par=0x%04X, status=%d\n", pctrl->cmd_par, pctrl->data_len);
            break;
        case BD_CTRL_LOCK_FILE:
            printf("DVR: LOCK_FILE received, index=%d, status=%d\n", pctrl->cmd_par, pctrl->data_len);
            break;
        case BD_CTRL_UNLOCK_FILE:
            printf("DVR: UNLOCK_FILE received, index=%d, status=%d\n", pctrl->cmd_par, pctrl->data_len);
            break;
        case BD_CTRL_TOTAL_TIME:
            printf("DVR: TOTAL_TIME received, index=%d, time=%d sec\n", pctrl->cmd_par, pctrl->data_len);
            break;
        case BD_CTRL_GET_TF_CAPACITY:
            printf("DVR: GET_TF_CAPACITY received, data_len=%d\n", pctrl->data_len);
            dvr_tf_capacity_parser(pctrl->trans_buf, pctrl->data_len);
            break;
        case BD_CTRL_PB_FF:
            printf("DVR: PB_FF received\n");
            break;
        case BD_CTRL_PB_FB:
            printf("DVR: PB_FB received\n");
            break;
        default:
            printf("DVR: unknown cmd 0x%02x\n", pctrl->cmd_id);
            break;
    }
}

// Decode JPEG and display to video layer
static int dvr_decode_and_display(st_dvr_capture_t *cap, uint32_t jpg_size)
{
    JpegHeaderInfo jpginfo = {0};
    uint32_t yaddr, uvaddr, vaddr;
    int format;
    int ret = -1;

    if (!cap->mfc_handle || !cap->yuv_buf || !cap->dst_buf[0] || !cap->dst_buf[1]) {
        return -1;
    }

    jpginfo.handle = cap->mfc_handle;
    jpginfo.jpg_addr = (uint32_t)cap->cap_jpg_buf;
    jpginfo.jpg_size = cap->current_jpg_size;  // Use actual total size, not header jpg_size
    jpginfo.dec_addry = (uint32_t)cap->yuv_buf;
    jpginfo.dec_size = cap->yuv_buf_size;

#if DVR_DEBUG_PRINT
    printf("DVR: decode jpg_addr=0x%x, jpg_size=%d, dec_addry=0x%x, dec_size=%d\n",
           jpginfo.jpg_addr, cap->current_jpg_size, jpginfo.dec_addry, cap->yuv_buf_size);
#endif
    // Debug: print JPEG header and bytes around expected footer
    // Footer (FF D9) should be at position (cap->current_jpg_size - 2) in cap_jpg_buf
    #if JPEG_DEBUG_PRINT
    printf("DVR: JPEG: hdr=%02x%02x%02x%02x, total=%d, last4=[%02x%02x%02x%02x]\n",
           ((uint8_t *)jpginfo.jpg_addr)[0], ((uint8_t *)jpginfo.jpg_addr)[1],
           ((uint8_t *)jpginfo.jpg_addr)[2], ((uint8_t *)jpginfo.jpg_addr)[3],
           cap->current_jpg_size,
           ((uint8_t *)jpginfo.jpg_addr)[cap->current_jpg_size - 4],
           ((uint8_t *)jpginfo.jpg_addr)[cap->current_jpg_size - 3],
           ((uint8_t *)jpginfo.jpg_addr)[cap->current_jpg_size - 2],
           ((uint8_t *)jpginfo.jpg_addr)[cap->current_jpg_size - 1]);
    #endif

    // Retry decode up to 3 times (like usb_detect.c)
    ret = -1;
    for (int retry = 0; retry < 3; retry++) {
        ret = mfc_jpegdec(&jpginfo);
        if (ret == 0) {
            break;  // Success
        }
        #if JPEG_DEBUG_PRINT
        printf("DVR: jpgdec retry %d, ret=%d\n", retry + 1, ret);
        #endif
        vTaskDelay(pdMS_TO_TICKS(2));  // Small delay before retry
    }

    if (ret < 0) {
#if DVR_DEBUG_PRINT
        printf("DVR: jpgdec failed, ret=%d, skip frame\n", ret);
#endif
        // Don't re-init MFC - it can cause crashes
        // Just skip this frame and continue
        return -1;
    }

    yaddr = jpginfo.dec_addry;
    uvaddr = jpginfo.dec_addru;
    vaddr = jpginfo.dec_addrv;

#if DVR_DEBUG_PRINT
    printf("DVR: dec_format=0x%x, dec_width=%d, dec_height=%d\n",
           jpginfo.dec_format, jpginfo.dec_width, jpginfo.dec_height);
#endif

    if (jpginfo.dec_format == JPEGDEC_YCbCr420_SEMIPLANAR) {
        format = PXP_SRC_FMT_YUV2P420;
    } else if (jpginfo.dec_format == JPEGDEC_YCbCr422_SEMIPLANAR) {
        format = PXP_SRC_FMT_YUV2P422;
    } else {
        #if JPEG_DEBUG_PRINT
        printf("DVR: Invalid yuv format 0x%x\n", jpginfo.dec_format);
        #endif
        return -1;
    }

    // Use double buffer - write to current buffer
    uint8_t cur_idx = cap->dst_buf_index;
    uint8_t next_idx = cur_idx ^ 1;  // Toggle between 0 and 1

    // Use PXP to convert YUV to RGB565 for display
    #if JPEG_DEBUG_PRINT
    printf("DVR: PXP: src=%dx%d, dst=0x%x, lcd=%dx%d\n",
           jpginfo.dec_width, jpginfo.dec_height, (uint32_t)cap->dst_buf[next_idx], LCD_WIDTH, LCD_HEIGHT);
    #endif
    ret = pxp_scaler_rotate(yaddr, uvaddr, vaddr, format, jpginfo.dec_width, jpginfo.dec_height,
                            (uint32_t)cap->dst_buf[next_idx], 0, PXP_OUT_FMT_RGB565, dvr_display_width, dvr_display_height, 0);
    if (ret) {
        #if JPEG_DEBUG_PRINT
        printf("DVR: pxp_scaler_rotate failed\n");
        #endif
        return -1;
    }

    LcdOsdInfo info = {0};
    info.x = dvr_display_x;
    info.y = dvr_display_y;
    info.width = dvr_display_width;
    info.height = dvr_display_height;
    info.format = LCD_OSD_FORAMT_RGB565;
    info.yaddr = (uint32_t)cap->dst_buf[next_idx];

    #if JPEG_DEBUG_PRINT
    printf("DVR: Display: addr=0x%x, fmt=0x%x, lcd=%dx%d\n",
           info.yaddr, info.format, info.width, info.height);
    #endif

    // Update buffer index for next frame
    cap->dst_buf_index = next_idx;

    /* Race guard: UI task may have called dvr_stop_preview() during decode */
    if (!dvr_preview_enable) {
        vTaskDelay(pdMS_TO_TICKS(100));
        return -1;
    }

    // Enable video layer (DVR window's transparent background lets it show)
    ark_lcd_osd_enable(LCD_VIDEO_LAYER, 1);
    ark_lcd_set_osd_info_atomic(LCD_VIDEO_LAYER, &info);
    ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);

    cap->display_on = 1;
    return 0;
}

// Seek with alignment for continuous reading (replaces simple seek to 0)
static void dvr_seek_for_align(st_dvr_capture_t *cap)
{
    uint32_t offset, limit;

    if (cap->round_flag == 0) {
        limit = cap->file_size;
    } else {
        limit = cap->file_size / 3;
    }
    offset = cap->read_times * 300 * 1024;

    if (offset + 300 * 1024 >= limit) {
        cap->round_flag = 1;
        cap->read_times = 0;
        offset = 100 * 300 * 1024;
        ff_fseek(cap->h_cap, offset, FF_SEEK_SET);
    } else {
        ff_fseek(cap->h_cap, offset, FF_SEEK_SET);
        cap->read_times++;
    }
}

// Main DVR capture process - read from elene file and distinguish protocol vs image
// Returns: 0=ok (image/jpeg data ready), -1=fail or command processed
static int dvr_capture_get_pic_process(st_dvr_capture_t *cap)
{
    FF_FILE *fp = cap->h_cap;
    size_t nb;
    st_bd_ctrl_if_t *pctrl;
    size_t jpg_size;
    uint32_t read_check_sum;
    uint32_t cnt, check_sum, i;

    if (!fp || !cap->cap_blk_buf) {
        return -1;
    }

    /* Early exit: skip USB read entirely when not in preview mode.
     * Previously the check was AFTER ff_fread (100KB USB read every 2ms),
     * wasting USB bandwidth and CPU even when DVR preview is off.
     * Command frames (0xAA55) are still needed, so we only skip when
     * not in preview AND no pending command send. */
    if (!dvr_preview_enable && !dvr_need_send_data && !dvr_skip_jpeg_process) {
        return -1;
    }

    /* Use aligned seek for continuous reading */
    dvr_seek_for_align(cap);

    /* Read block data (100KB) */
    nb = ff_fread(cap->cap_blk_buf, 1, cap->blksize, fp);
    if (nb != cap->blksize) {
        printf("DVR: read first block incomplete, expected %d, got %d\n", cap->blksize, nb);
        return -1;
    }

    pctrl = (st_bd_ctrl_if_t *)cap->cap_blk_buf;

    /* Check if it's a command (header 0xAA55) or image data */
    if (cap->cap_blk_buf[0] == 0x55 && cap->cap_blk_buf[1] == 0xAA) {
        /* It's a command frame */
        printf("DVR-RX<< cmd=0x%02X par=0x%04X data_len=%d frame[%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X]\n",
               pctrl->cmd_id, pctrl->cmd_par, pctrl->data_len,
               cap->cap_blk_buf[0], cap->cap_blk_buf[1],
               cap->cap_blk_buf[2], cap->cap_blk_buf[3],
               cap->cap_blk_buf[4], cap->cap_blk_buf[5],
               cap->cap_blk_buf[6], cap->cap_blk_buf[7],
               cap->cap_blk_buf[8], cap->cap_blk_buf[9]);
        dvr_recv_cmd_process(pctrl);
        return -1;
    }

    /* Skip JPEG processing when waiting for file list response */
    if (dvr_skip_jpeg_process) {
        ff_fseek(fp, 0, FF_SEEK_SET);
        return -1;
    }

    /* Skip JPEG decode when UI is not in preview mode (dvr_preview_enable == 0) */
    if (!dvr_preview_enable) {

        return -1;
    }

    /* It's image data - extract jpg_size from first 4 bytes */
    jpg_size = *((uint32_t *)cap->cap_blk_buf);
    read_check_sum = *((uint32_t *)(cap->cap_blk_buf + 8));

#if DVR_DEBUG_PRINT
    printf("DVR: jpg size=0x%x\n", jpg_size);
#endif

    /* Validate jpg_size - must be at least 1024 and at most 300KB */
    if (jpg_size < 1024 || jpg_size > 300 * 1024) {
        #if JPEG_DEBUG_PRINT
        printf("DVR: jpeg size invalid: 0x%x (min=1024, max=300KB)\n", jpg_size);
        #endif
        /* Read dummy data to maintain file position consistency */
        ff_fseek(fp, 100 * 1024, FF_SEEK_CUR);
        return -1;
    }

    /* Copy header part to jpeg buffer (skip BD_JPG_OFFSET = 17) */
    nb = cap->blksize - BD_JPG_OFFSET;
    if (nb > 300 * 1024) {
        #if JPEG_DEBUG_PRINT
        printf("DVR: block size too large: %d\n", nb);
        #endif
        return -1;
    }
    memcpy(cap->cap_jpg_buf, cap->cap_blk_buf + BD_JPG_OFFSET, nb);

    /* Check JPEG header (FF D8) */
    if (cap->cap_jpg_buf[0] != 0xFF || cap->cap_jpg_buf[1] != 0xD8) {
        #if JPEG_DEBUG_PRINT
        printf("DVR: invalid JPEG header\n");
        #endif
        /* Read dummy data to maintain file position consistency */
        ff_fseek(fp, 100 * 1024, FF_SEEK_CUR);
        return -1;
    }

    /* Read remaining JPEG data - read into cap_blk_buf + 512 */
    size_t read_bytes = ff_fread(cap->cap_blk_buf + 512, 1, jpg_size, fp);
    if (read_bytes != jpg_size) {
        #if JPEG_DEBUG_PRINT
        printf("DVR: read remaining JPEG failed, expected %d, got %d\n", jpg_size, read_bytes);
        #endif
        return -1;
    }

    /* Copy remaining data after first block (from offset 512 in cap_blk_buf) */
    memcpy(cap->cap_jpg_buf + nb, cap->cap_blk_buf + 512, jpg_size);

    /* Calculate checksum from cap_blk_buf + 512 + i*512 (like usb_detect.c) */
    cnt = (jpg_size - 511) >> 9;
    check_sum = 0;
    for (i = 1; i < cnt; i++) {
        uint32_t val = *((uint32_t *)(cap->cap_blk_buf + 512 + i * 512));
        check_sum += val;
    }

    /* Print checksum for debugging */
    #if JPEG_DEBUG_PRINT
    if ((check_sum & 0xFF) != (read_check_sum & 0xFF)) {
        printf("DVR: checksum warn, size:%d, calc:0x%08x, expected:0x%08x\n",
               jpg_size, check_sum, read_check_sum);
        /* Continue processing - checksum mismatch doesn't mean decode will fail */
    }
    #endif

    /* Find actual JPEG end by searching backwards for FF D9 */
    uint32_t actual_size = nb + jpg_size;
    int32_t search_start = (int32_t)actual_size - 200;  // Search last 200 bytes
    if (search_start < (int32_t)nb) search_start = nb;
    uint32_t found_pos = 0;

    // Search backwards from (actual_size - 2) to search_start
    for (int32_t i = (int32_t)actual_size - 2; i >= search_start; i--) {
        if (cap->cap_jpg_buf[i] == 0xFF && cap->cap_jpg_buf[i+1] == 0xD9) {
            found_pos = (uint32_t)i;
            break;
        }
    }

    #if JPEG_DEBUG_PRINT
    // Debug: show search range
    printf("DVR: footer search: range[%d to %d], found=%d\n",
           search_start, actual_size - 2, found_pos);
    #endif

    if (found_pos == 0) {
        #if JPEG_DEBUG_PRINT
        printf("DVR: JPEG footer not found, trying last 200 bytes scan...\n");
        #endif
        // Final fallback: search entire buffer for FF D9
        for (uint32_t i = actual_size - 2; i >= nb + 100; i--) {
            if (cap->cap_jpg_buf[i] == 0xFF && cap->cap_jpg_buf[i+1] == 0xD9) {
                found_pos = i;
                #if JPEG_DEBUG_PRINT
                printf("DVR: FF D9 found at pos=%d (deep search)\n", found_pos);
                #endif
                break;
            }
        }
        if (found_pos == 0) {
            #if JPEG_DEBUG_PRINT
            printf("DVR: JPEG footer FF D9 not found in buffer, using size=%d\n", actual_size);
            #endif
        }
    } else {
        actual_size = found_pos + 2;  // Include FF D9
        #if JPEG_DEBUG_PRINT
        printf("DVR: JPEG footer found at pos=%d, actual_size=%d\n", found_pos, actual_size);
        #endif
    }

    cap->current_jpg_size = actual_size;

    /* JPEG data is ready in cap_jpg_buf */
    return 0;
}

// DVR task - continuously reads from elene file
static void dvr_usb_task(void *arg)
{
    st_dvr_capture_t *cap = (st_dvr_capture_t *)arg;

    printf("DVR: task started\n");

    if (dvr_capture_init(cap) != 0) {
        printf("DVR: init failed, task exit\n");
        dvr_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }


    // Send MIC on command once on startup
    // dvr_send_normal_cmd(BD_CTRL_MIC_ON, 0);

    // Note: dvr_get_status() and dvr_get_file_list() are called on demand, not in the main loop

    /* Auto-record: query status after task init; when SD=1 is first
     * seen in dvr_recv_cmd_process(), rec_start is sent automatically.
     * Flag is reset on task exit so the next USB insertion retriggers. */
    dvr_auto_rec_pending = 1;
    dvr_send_normal_cmd(BD_CTRL_GET_STS, 0);
    printf("DVR: init done, querying status for auto-rec\n");

    uint32_t last_switch_time = 0;
    uint32_t current_time;
#if DVR_FRAMERATE_PRINT
    dvr_last_stat_time = xTaskGetTickCount();
    dvr_frame_count = 0;
    dvr_frame_count_last = 0;
    dvr_fps = 0;
#endif
    printf("DVR: sensor switch test started, enable=%d\n", dvr_sensor_switch_enable);
    while (cap->running) {
        // Check if need to send command data
        if (dvr_need_send_data) {
            dvr_seek_for_align(cap);
            dvr_send_data();
            dvr_need_send_data = 0;
        }

        if (dvr_capture_get_pic_process(cap) == 0) {
            // Decode and display the JPEG
            dvr_decode_and_display(cap, cap->current_jpg_size);
#if DVR_FRAMERATE_PRINT
            dvr_frame_count++;
#endif
        }

#if DVR_FRAMERATE_PRINT
        // Calculate framerate every 1 second
        current_time = xTaskGetTickCount();
        if (current_time - dvr_last_stat_time >= 1000) {
            dvr_fps = dvr_frame_count - dvr_frame_count_last;
            dvr_frame_count_last = dvr_frame_count;
            dvr_last_stat_time = current_time;
            /* FPS print is gated by the runtime flag dvr_fps_print_enable
             * (CLI-controllable). Stats are always computed; only the
             * printf is suppressed when disabled. */
            if (dvr_fps_print_enable) {
                printf("DVR: [FPS:%lu] frame_count=%lu\n",
                       (unsigned long)dvr_fps,
                       (unsigned long)dvr_frame_count);
            }

            /* Auto-rec retry: if SD was not ready at task init, re-query
             * status every FPS tick (~1s) until SD comes up or 30s passes. */
            if (dvr_auto_rec_pending) {
                static uint8_t auto_rec_retry_count = 0;
                if (++auto_rec_retry_count <= 3) {
                    dvr_send_normal_cmd(BD_CTRL_GET_STS, 0);
                } else {
                    dvr_auto_rec_pending = 0;
                    auto_rec_retry_count = 0;
                    printf("DVR: auto-rec gave up after 3 retries (SD not ready)\n");
                }
            }

            /* System health report every 30 seconds */
            static uint32_t diag_interval = 0;
            if (++diag_interval >= 30) {
                diag_interval = 0;
                /* fb queue status */
                extern void fb_diag_get_counts(int*, int*, uint32_t*, uint32_t*);
                int fb_free_n = 0, fb_ready_n = 0;
                uint32_t vsync_n = 0, no_ready_n = 0;
                fb_diag_get_counts(&fb_free_n, &fb_ready_n, &vsync_n, &no_ready_n);
                /* FreeRTOS heap */
                size_t heap_free = xPortGetFreeHeapSize();
                size_t heap_min  = xPortGetMinimumEverFreeHeapSize();
                printf("[DIAG-SYS] tick=%lu fb:free=%d,ready=%d "
                       "vsync=%lu no_rdy=%lu "
                       "heap:cur=%u,min=%u "
                       "dvr_prev=%d hook=%lu\n",
                       (unsigned long)current_time,
                       fb_free_n, fb_ready_n,
                       (unsigned long)vsync_n, (unsigned long)no_ready_n,
                       (unsigned)heap_free, (unsigned)heap_min,
                       (int)dvr_preview_enable,
                       (unsigned long)g_alpha_hook_call_count);
            }
        }
#endif

        // Switch view mode every 5 seconds (only when previewing —
        // non-preview USB writes take ~450ms per command, wasting bandwidth)
        if (dvr_sensor_switch_enable && dvr_preview_enable) {
            current_time = xTaskGetTickCount();
            if (current_time - last_switch_time >= 5000) {  // 500 ticks = 5 seconds
                dvr_view_mode = (dvr_view_mode + 1) % 5;
                last_switch_time = current_time;
                printf("DVR: [TICK:%u] switch view mode=%d\n", last_switch_time, dvr_view_mode);
                // Use send_normal_cmd to set flag, actual send happens in next loop iteration
                dvr_send_normal_cmd(BD_ANDROID_VIEW_SWITCH, dvr_view_mode);
            }
        }

        /* Yield CPU. When not in preview, sleep longer to avoid
         * burning CPU/USB bandwidth on idle polling (was 2ms = 500
         * USB reads/sec of 100KB each, now 50ms = 20/sec idle check). */
        vTaskDelay(pdMS_TO_TICKS(dvr_preview_enable ? 2 : 50));
    }

    dvr_capture_deinit(cap);
    dvr_auto_rec_pending = 0;
    dvr_task_handle = NULL;
    printf("DVR: task exited\n");
    vTaskDelete(NULL);
}

// Start DVR USB task when elene file is detected
static void dvr_start_if_elene_exists(void)
{
    if (dvr_task_handle != NULL) {
        printf("DVR: task already running\n");
        return;
    }

    if (!dvr_check_elene_exist()) {
        printf("DVR: elene not found\n");
        return;
    }

    printf("DVR: elene found, starting task\n");

    dvr_capture.running = 0;
    dvr_capture.h_cap = NULL;
    dvr_capture.cap_blk_buf = NULL;
    dvr_capture.cap_jpg_buf = NULL;

    if (xTaskCreate(dvr_usb_task, "dvr_usb", configMINIMAL_STACK_SIZE * 16,
                    &dvr_capture, 10, &dvr_task_handle) != pdPASS) {
        printf("DVR: create task failed\n");
        dvr_task_handle = NULL;
    }
}

// Request DVR status from device
void dvr_get_status(void)
{
    if (dvr_fp != NULL) {
        dvr_send_normal_cmd(BD_CTRL_GET_STS, 0);
    }
}

// DVR status getter functions
uint8_t dvr_get_sd_status(void) { return dvr_sd_status; }
uint8_t dvr_get_rec_status(void) { return dvr_rec_status; }
uint8_t dvr_get_lock_status(void) { return dvr_lock_status; }
uint8_t dvr_get_mic_status(void) { return dvr_mic_status; }
uint8_t dvr_get_sd_error_status(void) { return dvr_sd_error; }
uint8_t dvr_get_sd_full_status(void) { return dvr_sd_full; }

// DVR sensor switch control functions
void dvr_set_sensor_switch_enable(uint8_t enable) { dvr_sensor_switch_enable = enable; }

// Store one filename in the appropriate (is_jpg, is_rear) bucket.
// Returns 1 if stored, 0 if the bucket is full (caller still counts it).
static uint8_t dvr_filelist_store(uint8_t is_jpg, uint8_t is_rear, const char *name)
{
    if (name == NULL) return 0;

    if (is_jpg) {
        if (is_rear) {
            if (dvr_photo_list_r_count < DVR_PHOTO_LIST_MAX) {
                strncpy(dvr_photo_list_r[dvr_photo_list_r_count], name, DVR_NAME_MAX - 1);
                dvr_photo_list_r[dvr_photo_list_r_count][DVR_NAME_MAX - 1] = '\0';
                dvr_photo_list_r_count++;
                return 1;
            }
        } else {
            if (dvr_photo_list_f_count < DVR_PHOTO_LIST_MAX) {
                strncpy(dvr_photo_list_f[dvr_photo_list_f_count], name, DVR_NAME_MAX - 1);
                dvr_photo_list_f[dvr_photo_list_f_count][DVR_NAME_MAX - 1] = '\0';
                dvr_photo_list_f_count++;
                return 1;
            }
        }
    } else {
        if (is_rear) {
            if (dvr_video_list_r_count < DVR_VIDEO_LIST_MAX) {
                strncpy(dvr_video_list_r[dvr_video_list_r_count], name, DVR_NAME_MAX - 1);
                dvr_video_list_r[dvr_video_list_r_count][DVR_NAME_MAX - 1] = '\0';
                dvr_video_list_r_count++;
                return 1;
            }
        } else {
            if (dvr_video_list_f_count < DVR_VIDEO_LIST_MAX) {
                strncpy(dvr_video_list_f[dvr_video_list_f_count], name, DVR_NAME_MAX - 1);
                dvr_video_list_f[dvr_video_list_f_count][DVR_NAME_MAX - 1] = '\0';
                dvr_video_list_f_count++;
                return 1;
            }
        }
    }
    return 0;
}

// Parse file list from DVR device response
static void dvr_filelist_parser(uint8_t *buf, int32_t len, uint16_t cmd_par)
{
    int32_t cnt;
    uint8_t *ptr8 = buf;
    uint32_t hash, year, mon, day, hour, min, sec, attrib;
    uint16_t name_idx;
    char fname[DVR_NAME_MAX];
    uint8_t is_jpg = (cmd_par & 0x8000) ? 1 : 0;
    uint8_t is_rear = (cmd_par & 0x4000) ? 1 : 0;

    dvr_video_list_count = 0;
    dvr_photo_list_count = 0;

    /* Only the bucket we are filling this pass is cleared; the other
     * three retain their last-known list until refreshed. */
    if (is_jpg) {
        if (is_rear) dvr_photo_list_r_count = 0;
        else         dvr_photo_list_f_count = 0;
    } else {
        if (is_rear) dvr_video_list_r_count = 0;
        else         dvr_video_list_f_count = 0;
    }

    printf("DVR: filelist parser, len=%d, is_jpg=%d, is_rear=%d\n",
           len, is_jpg, is_rear);

    for (cnt = 0; cnt < (len / BYTE_PER_FILE); cnt++) {
        // File index (little endian)
        name_idx = (*(ptr8 + 1) << 8) | (*ptr8);

        if (name_idx == 0xFFFF) {
            printf("DVR: list end\n");
            break;
        }

        // File modification time hash
        hash = (*(ptr8 + 5) << 24) | (*(ptr8 + 4) << 16) | (*(ptr8 + 3) << 8) | (*(ptr8 + 2));

        year = (hash >> 26) & 0x3F;
        mon = (hash >> 22) & 0x0F;
        day = (hash >> 17) & 0x1F;
        hour = (hash >> 12) & 0x1F;
        min = (hash >> 6) & 0x3F;
        sec = (hash >> 0) & 0x3F;

        year += 2000;
        attrib = sec & 0x01;  // Lock status

        ptr8 += BYTE_PER_FILE;

        if (is_jpg) {
            snprintf(fname, sizeof(fname), "PICT%04d.jpg", name_idx);
            printf("DVR: [PICT%04d.jpg] %04d_%02d_%02d %02d:%02d:%02d%s\n",
                   name_idx, year, mon, day, hour, min, sec,
                   attrib ? " [LOCKED]" : "");
            dvr_photo_list_count++;
        } else {
            snprintf(fname, sizeof(fname), "%s%04d.avi",
                     attrib ? "LOCK" : "MOVI", name_idx);
            printf("DVR: [%s%04d.avi] %04d_%02d_%02d %02d:%02d:%02d%s\n",
                   attrib ? "LOCK" : "MOVI", name_idx, year, mon, day, hour, min, sec,
                   attrib ? " [LOCKED]" : "");
            dvr_video_list_count++;
        }

        dvr_filelist_store(is_jpg, is_rear, fname);
    }

    printf("DVR: file list count - Video: %d, Photo: %d\n",
           dvr_video_list_count, dvr_photo_list_count);

    /* Signal UI thread that fresh data is available */
    dvr_filelist_ready_flag = 1;
}

// Parse TF card capacity response from DVR device.
// Wire format (little-endian, 8 bytes):
//   trans_buf[0..3]  = total KiB  (1024-byte units, NOT bytes)
//   trans_buf[4..7]  = free  KiB  (1024-byte units, NOT bytes)
// Note: the DVR firmware reports sizes in KiB (1024-byte blocks), matching
// the typical SD-card stat convention. We convert to bytes for the public API.
static void dvr_tf_capacity_parser(uint8_t *buf, int32_t len)
{
    uint32_t total_kib = 0;
    uint32_t free_kib  = 0;

    if (buf == NULL) { printf("DVR: GET_TF_CAPACITY: null buffer\n"); return; }

    if (len >= 4) {
        total_kib = ((uint32_t)buf[3] << 24) | ((uint32_t)buf[2] << 16) |
                    ((uint32_t)buf[1] <<  8) |  (uint32_t)buf[0];
    }
    if (len >= 8) {
        free_kib = ((uint32_t)buf[7] << 24) | ((uint32_t)buf[6] << 16) |
                   ((uint32_t)buf[5] <<  8) |  (uint32_t)buf[4];
    }

    dvr_tf_total_kib      = total_kib;
    dvr_tf_free_kib       = free_kib;
    dvr_tf_capacity_valid = 1;

    printf("DVR: TF [KiB] total=%u free=%u (%.2f GiB / %.2f GiB)\n",
           total_kib, free_kib,
           (double)total_kib / (1024.0*1024.0),
           (double)free_kib / (1024.0*1024.0));
}

// Get file list from DVR device
//   mode 0: front-camera video file list
//   mode 1: rear-camera  video file list
//   mode 2: front-camera photo file list
//   mode 3: rear-camera  photo file list
void dvr_get_file_list(uint8_t mode)
{
    uint16_t par = 0;

    if (mode == 0) {
        par = 0;        // Video file list F
    } else if (mode == 1) {
        par = 0x4000;   // Video file list R
    } else if (mode == 2) {
        par = 0x8000;   // Photo file list F
    } else if (mode == 3) {
        par = 0xC000;   // Photo file list R
    } else {
        printf("error: invalid mode\n");
        return;
    }

    // Set flag to skip JPEG processing during file list response
    dvr_skip_jpeg_process = 1;

    dvr_send_normal_cmd(BD_CTRL_GET_LIST, par);
    printf("DVR: get file list, mode=%s\n", mode == 0 ? "video" : "photo");
}

// Start playback of a specific file by index
//   mode 0: play front-camera video
//   mode 1: play rear-camera  video
//   mode 2: play front-camera photo
//   mode 3: play rear-camera  photo
static void dvr_pb_start(uint8_t mode, uint16_t index)
{
    uint16_t par = index;

    if (mode == 0) {
        par |= 0;        // Video file list F
    } else if (mode == 1) {
        par |= 0x4000;   // Video file list R
    } else if (mode == 2) {
        par |= 0x8000;   // Photo file list F
    } else if (mode == 3) {
        par |= 0xC000;   // Photo file list R
    } else {
        printf("error: invalid file\n");
        return;
    }

    dvr_send_normal_cmd(BD_CTRL_PB_START, par);
    printf("DVR: pb start, mode=%s, index=%d\n", mode == 0 ? "video" : "photo", index);
}

// Pause playback
static void dvr_pb_pause(void)
{
    dvr_send_normal_cmd(BD_CTRL_PB_PAUSE, 0);
    printf("DVR: pb pause\n");
}

// Stop playback
static void dvr_pb_stop(void)
{
    dvr_send_normal_cmd(BD_CTRL_PB_STOP, 0);
    printf("DVR: pb stop\n");
}

// Fast forward playback
static void dvr_pb_ff(void)
{
    dvr_send_normal_cmd(BD_CTRL_PB_FF, 0);
    printf("DVR: pb fast forward\n");
}

// Fast backward playback
static void dvr_pb_fb(void)
{
    dvr_send_normal_cmd(BD_CTRL_PB_FB, 0);
    printf("DVR: pb fast backward\n");
}

// Set recording resolution: 0=1080P, 1=720P
static void dvr_set_resolution(uint8_t res)
{
    dvr_send_normal_cmd(BD_CTRL_SET_RES, res);
    printf("DVR: set resolution: %s\n", res == 0 ? "1080P" : "720P");
}

// Set loop recording time: 1=1min, 2=2min, 3=3min, 0=get current setting
static void dvr_set_loop_time(uint8_t time)
{
    dvr_send_normal_cmd(BD_CTRL_SET_REC_TIME, time);
    printf("DVR: set loop time: %d min\n", time);
}

// Set MIC on/off: 0=off, 1=on
static void dvr_set_mic(uint8_t onoff)
{
    dvr_send_normal_cmd(BD_CTRL_MIC_ON, onoff);
    printf("DVR: set MIC: %s\n", onoff ? "on" : "off");
}

// Set timestamp watermark on/off: 0=off, 1=on
static void dvr_set_stamp(uint8_t onoff)
{
    dvr_send_normal_cmd(BD_CTRL_STAMP_ON, onoff);
    printf("DVR: set stamp: %s\n", onoff ? "on" : "off");
}

// Format SD card (must stop recording first)
static void dvr_format_card(void)
{
    dvr_send_normal_cmd(BD_CTRL_FORMAT, 0);
    printf("DVR: format card\n");
}

// Restore factory defaults (must stop recording first)
static void dvr_restore_default(void)
{
    dvr_send_normal_cmd(BD_CTRL_DEFAULT, 0);
    printf("DVR: restore factory default\n");
}

// Set system time
static void dvr_set_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    st_bd_ctrl_if_t *pctrl = &dvr_ctrl_data;
    uint16_t *ptr16 = (uint16_t *)pctrl->trans_buf;

    ptr16[0] = (month << 8) | year;
    ptr16[1] = (hour << 8) | day;
    ptr16[2] = (second << 8) | minute;

    pctrl->header_id = 0xAA55;
    pctrl->cmd_id = BD_CTRL_SET_TIME;
    pctrl->cmd_par = 0;
    pctrl->data_len = 6;
    pctrl->checksum = 0;
    dvr_need_send_data = 1;

    printf("DVR: set time: %04d-%02d-%02d %02d:%02d:%02d\n", year, month, day, hour, minute, second);
}

// Delete file by index
//   mode 0: delete front-camera video
//   mode 1: delete rear-camera  video
//   mode 2: delete front-camera photo
//   mode 3: delete rear-camera  photo
static void dvr_delete_file(uint8_t mode, uint16_t index)
{
    uint16_t par = index;

    if (mode == 0) {
        par |= 0;        // Video file list F
    } else if (mode == 1) {
        par |= 0x4000;   // Video file list R
    } else if (mode == 2) {
        par |= 0x8000;   // Photo file list F
    } else if (mode == 3) {
        par |= 0xC000;   // Photo file list R
    } else {
        printf("error: invalid file\n");
        return;
    }

    dvr_send_normal_cmd(BD_CTRL_DEL_FILE, par);
    printf("DVR: delete file, mode=%s, index=%d\n", mode == 0 ? "video" : "photo", index);
}

// Lock file by index
static void dvr_lock_file(uint16_t index)
{
    dvr_send_normal_cmd(BD_CTRL_LOCK_FILE, index);
    printf("DVR: lock file index: %d\n", index);
}

// Unlock file by index
static void dvr_unlock_file(uint16_t index)
{
    dvr_send_normal_cmd(BD_CTRL_UNLOCK_FILE, index);
    printf("DVR: unlock file index: %d\n", index);
}

// Get total time of file by index
static void dvr_get_total_time(uint16_t index)
{
    dvr_send_normal_cmd(BD_CTRL_TOTAL_TIME, index);
    printf("DVR: get total time for index: %d\n", index);
}

// Get file list counts
uint16_t dvr_get_video_list_count(void) { return dvr_video_list_count; }
uint16_t dvr_get_photo_list_count(void) { return dvr_photo_list_count; }

/*
 * Per-bucket file list accessors.
 *
 * Each "get_list_*" function returns the number of filenames in the
 * corresponding bucket and, if `out` is non-NULL, copies up to
 * `max_entries` names into `out` (each entry is DVR_NAME_MAX bytes
 * wide). The arrays are read-only to the caller.
 *
 * Bucket layout:
 *   F = front camera, R = rear camera
 *   video = .avi (mov/lock), photo = .jpg
 */
static inline void dvr_copy_names(char *out, const char (*src)[DVR_NAME_MAX],
                                  uint16_t count, uint16_t max_entries)
{
    uint16_t i;
    uint16_t n = (count < max_entries) ? count : max_entries;
    for (i = 0; i < n; i++) {
        memcpy(out + (size_t)i * DVR_NAME_MAX, src[i], DVR_NAME_MAX);
    }
}

uint16_t dvr_api_get_video_list_f(char *out, uint16_t max_entries)
{
    if (out && max_entries) dvr_copy_names(out, dvr_video_list_f, dvr_video_list_f_count, max_entries);
    return dvr_video_list_f_count;
}
uint16_t dvr_api_get_video_list_r(char *out, uint16_t max_entries)
{
    if (out && max_entries) dvr_copy_names(out, dvr_video_list_r, dvr_video_list_r_count, max_entries);
    return dvr_video_list_r_count;
}
uint16_t dvr_api_get_photo_list_f(char *out, uint16_t max_entries)
{
    if (out && max_entries) dvr_copy_names(out, dvr_photo_list_f, dvr_photo_list_f_count, max_entries);
    return dvr_photo_list_f_count;
}
uint16_t dvr_api_get_photo_list_r(char *out, uint16_t max_entries)
{
    if (out && max_entries) dvr_copy_names(out, dvr_photo_list_r, dvr_photo_list_r_count, max_entries);
    return dvr_photo_list_r_count;
}

uint16_t dvr_api_get_video_list_f_count(void) { return dvr_video_list_f_count; }
uint16_t dvr_api_get_video_list_r_count(void) { return dvr_video_list_r_count; }
uint16_t dvr_api_get_photo_list_f_count(void) { return dvr_photo_list_f_count; }
uint16_t dvr_api_get_photo_list_r_count(void) { return dvr_photo_list_r_count; }

/* File-list ready flag: set by dvr_filelist_parser() in USB-task context,
 * polled & cleared by UI thread after dvr_api_get_list(). */
uint8_t dvr_api_is_filelist_ready(void)  { return dvr_filelist_ready_flag; }
void    dvr_api_clear_filelist_ready(void){ dvr_filelist_ready_flag = 0; }

// Get view mode
uint8_t dvr_get_view_mode(void) { return dvr_view_mode; }

/**********************
 *  EXTERNAL DVR API FUNCTIONS
 *  These functions provide external access to DVR control commands
 **********************/

// DVR control API - Get DVR version ID (sends GET_ID; reply arrives async)
void dvr_api_get_id(void)
{
    dvr_send_normal_cmd(BD_CTRL_GET_ID, 0);
}

// DVR control API - Read the most recent DVR firmware version string.
//   buf:      caller-provided buffer; NUL-terminated on return.
//   buf_len:  size of buf in bytes (recommend >= DVR_VERSION_MAX_LEN).
//   Returns 1 if a valid version has been received, 0 otherwise.
//   The string is a snapshot — call again after dvr_api_get_id() to refresh.
uint8_t dvr_api_get_version(char *buf, uint32_t buf_len)
{
    if (buf == NULL || buf_len == 0) {
        return 0;
    }
    if (!dvr_version_valid) {
        buf[0] = '\0';
        return 0;
    }
    /* dvr_version_buf is written once and not concurrently mutated, so a
     * byte-by-byte copy (no locking) is safe and avoids hidden volatile
     * access on every character. */
    uint32_t i;
    for (i = 0; i + 1 < buf_len && dvr_version_buf[i] != '\0'; i++) {
        buf[i] = dvr_version_buf[i];
    }
    buf[i] = '\0';
    return 1;
}

// DVR control API - Check if DEL_FILE ACK has been received
uint8_t dvr_api_is_del_ack(void)
{
    return dvr_del_ack_received;
}

// DVR control API - Clear DEL_FILE ACK flag (call before sending DEL)
void dvr_api_clear_del_ack(void)
{
    dvr_del_ack_received = 0;
}

// DVR control API - Start recording
void dvr_api_rec_start(void)
{
    dvr_send_normal_cmd(BD_CTRL_REC_START, 0);
}

// DVR control API - Stop recording
void dvr_api_rec_stop(void)
{
    dvr_send_normal_cmd(BD_CTRL_REC_STOP, 0);
}

// DVR control API - Take snapshot/photo
void dvr_api_snap(void)
{
    dvr_send_normal_cmd(BD_CTRL_SNAP, 0);
}

// DVR control API - Emergency/SOS recording (lock current file)
void dvr_api_sos(void)
{
    dvr_send_normal_cmd(BD_CTRL_SOS, 0);
}

// DVR control API - Get file list (mode: 0=video, 1=photo)
void dvr_api_get_list(uint8_t mode)
{
    dvr_get_file_list(mode);
}

// DVR control API - Start playback (mode: 0=video, 1=photo, index: file index)
void dvr_api_pb_start(uint8_t mode, uint16_t index)
{
    dvr_pb_start(mode, index);
}

// DVR control API - Pause playback
void dvr_api_pb_pause(void)
{
    dvr_pb_pause();
}

// DVR control API - Stop playback
void dvr_api_pb_stop(void)
{
    dvr_pb_stop();
}

// DVR control API - Get DVR status
void dvr_api_get_status(void)
{
    dvr_get_status();
}

// DVR control API - Send a TF-card capacity query (no payload).
// Reply is parsed asynchronously by dvr_recv_cmd_process(); read back via
// dvr_api_get_tf_capacity() (or check the valid flag it returns).
void dvr_api_get_tf_capacity_query(void)
{
    dvr_send_normal_cmd(BD_CTRL_GET_TF_CAPACITY, 0);
}

// DVR control API - Read the most recent TF card capacity sample.
//   total_bytes / free_bytes are out-params; may be NULL.
//   Returns 1 if at least one valid sample has been received, 0 otherwise.
uint8_t dvr_api_get_tf_capacity(uint32_t *total_kib, uint32_t *free_kib)
{
    if (total_kib) *total_kib = dvr_tf_total_kib;
    if (free_kib)  *free_kib  = dvr_tf_free_kib;
    return dvr_tf_capacity_valid;
}

// DVR control API - View switch (mode: 0=front, 1=rear, 2=f+r, 3=r+f, 4=hzh)
void dvr_api_view_switch(uint8_t mode)
{
    dvr_send_normal_cmd(BD_ANDROID_VIEW_SWITCH, mode);
    dvr_view_mode = mode;
}

// DVR control API - Set resolution (0=1080P, 1=720P)
void dvr_api_set_res(uint8_t res)
{
    dvr_set_resolution(res);
}

// DVR control API - Set loop recording time (1=1min, 2=2min, 3=3min, 0=get current)
void dvr_api_set_loop_time(uint8_t time)
{
    dvr_set_loop_time(time);
}

// DVR control API - Set MIC on/off (0=off, 1=on)
void dvr_api_set_mic(uint8_t onoff)
{
    dvr_set_mic(onoff);
}

// DVR control API - Set timestamp watermark on/off (0=off, 1=on)
void dvr_api_set_stamp(uint8_t onoff)
{
    dvr_set_stamp(onoff);
}

// DVR control API - Format card
void dvr_api_format(void)
{
    dvr_format_card();
}

// DVR control API - Restore factory defaults
void dvr_api_restore_default(void)
{
    dvr_restore_default();
}

// DVR control API - Set system time
void dvr_api_set_time(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    dvr_set_time(year, month, day, hour, minute, second);
}

// DVR control API - Delete file by index (mode: 0=F video, 1=R video, 2=F photo, 3=R photo)
void dvr_api_del_file(uint8_t mode, uint16_t index)
{
    dvr_delete_file(mode, index);
}

// DVR control API - Lock file by index
void dvr_api_lock_file(uint16_t index)
{
    dvr_lock_file(index);
}

// DVR control API - Unlock file by index
void dvr_api_unlock_file(uint16_t index)
{
    dvr_unlock_file(index);
}

// DVR control API - Get total time of file by index
void dvr_api_get_total_time(uint16_t index)
{
    dvr_get_total_time(index);
}

// DVR control API - Fast forward playback
void dvr_api_pb_ff(void)
{
    dvr_pb_ff();
}

// DVR control API - Fast backward playback
void dvr_api_pb_fb(void)
{
    dvr_pb_fb();
}

// DVR control API - Sensor switch enable/disable
void dvr_api_set_sensor_switch(uint8_t enable)
{
    dvr_set_sensor_switch_enable(enable);
}

// DVR control API - Check if elene file exists
int dvr_api_check_exists(void)
{
    return dvr_check_elene_exist();
}

// DVR control API - Check if DVR task is running
int dvr_api_is_running(void)
{
    return (dvr_task_handle != NULL) ? 1 : 0;
}

// DVR control API - Set display window position and size
// x/y: window top-left coordinates
// width/height: window size
// Note: This affects the OSD layer display area, PXP scales content to fit
void dvr_api_set_display_window(int32_t x, int32_t y, int32_t width, int32_t height)
{
    dvr_display_x = x;
    dvr_display_y = y;
    dvr_display_width = width;
    dvr_display_height = height;
    printf("DVR: set display window - x:%d y:%d w:%d h:%d\n", x, y, width, height);
}

// DVR control API - Get display window position and size
void dvr_api_get_display_window(int32_t *x, int32_t *y, int32_t *width, int32_t *height)
{
    if (x) *x = dvr_display_x;
    if (y) *y = dvr_display_y;
    if (width) *width = dvr_display_width;
    if (height) *height = dvr_display_height;
    printf("DVR: get display window - x:%d y:%d w:%d h:%d\n", dvr_display_x, dvr_display_y, dvr_display_width, dvr_display_height);
}

// DVR control API - Reset display window to full screen
void dvr_api_reset_display_window(void)
{
    dvr_display_x = 0;
    dvr_display_y = 0;
    dvr_display_width = LCD_WIDTH;
    dvr_display_height = LCD_HEIGHT;
    printf("DVR: reset display window to full screen\n");
}

// DVR control API - Enable/disable preview mode (jpeg decode)
// enable: 0=disable preview (skip jpeg decode), 1=enable preview (run jpeg decode)
void dvr_api_set_preview_enable(uint8_t enable)
{
    dvr_preview_enable = enable;
    printf("DVR: preview enable set to %d\n", enable);

    if (enable) {
        /* Register post-render hook: clears alpha in preview region
         * every frame so VIDEO layer shows through home_page's opaque bg */
        vg_set_post_render_hook(dvr_alpha_clear_hook);
        printf("DVR: alpha-clear hook registered\n");
    } else {
        /* Disable VIDEO and remove hook back-to-back, no gap */
        ark_lcd_osd_enable(LCD_VIDEO_LAYER, 0);
        ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
        vg_set_post_render_hook(NULL);
        /* Alpha will be restored naturally by AWTK rendering home_page */
        printf("DVR: VIDEO off, hook removed\n");
    }
}

// DVR control API - Get preview mode status
uint8_t dvr_api_get_preview_enable(void)
{
    return dvr_preview_enable;
}

// DVR control API - Enable/disable the per-second FPS log line.
// enable: 0=suppress `DVR: [FPS:xx]` prints, 1=emit them. Default ON.
void dvr_api_set_fps_print(uint8_t enable)
{
    dvr_fps_print_enable = enable ? 1 : 0;
    printf("DVR: FPS print %s\n", dvr_fps_print_enable ? "ON" : "OFF");
}

// DVR control API - Query whether the FPS log line is enabled.
uint8_t dvr_api_get_fps_print(void)
{
    return dvr_fps_print_enable;
}

// DVR preview lifecycle - called by dvr_page_init() when DVR window opens
void dvr_start_preview(void)
{
    dvr_api_set_display_window(0, 0, 1024, 500);
    dvr_api_set_preview_enable(1);
    printf("DVR: start_preview (window 0,0,1024,500)\n");
}

// DVR preview lifecycle - called by dvr_page BACK key / window close
void dvr_stop_preview(void)
{
    /* 1. Set flag — DVR task's race guard will stop touching LCD layers */
    dvr_preview_enable = 0;

    /* 2. Wait for DVR task's in-flight frame to finish */
    if (dvr_task_handle != NULL) {
        vTaskDelay(pdMS_TO_TICKS(80));
    }

    /* 3. VIDEO off + hook remove back-to-back */
    ark_lcd_osd_enable(LCD_VIDEO_LAYER, 0);
    ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
    vg_set_post_render_hook(NULL);

    /* 4. Restore on-screen FB to eliminate DVR UI instantly.
     * Preview region (y0~y0+h): restore alpha to 0xFF (was cleared by hook).
     * Dock bar region (y0+h ~ 600): fill with opaque black to hide the
     * four bottom buttons immediately, rather than waiting for AWTK to
     * render the next frame. AWTK will paint home_page over both regions
     * within 1-2 frames. */
    {
        uint32_t on_screen_addr = 0;
        ark_lcd_get_osd_yaddr(LCD_UI_LAYER, &on_screen_addr);
        if (on_screen_addr) {
            uint32_t *fb = (uint32_t *)on_screen_addr;
            int x0 = dvr_display_x, y0 = dvr_display_y;
            int x1 = x0 + dvr_display_width;
            int y_dock_end = OSD_HEIGHT;  /* cover preview + dock = full page */
            if (x1 > OSD_WIDTH) x1 = OSD_WIDTH;
            /* Preview region: restore alpha only */
            int y_preview_end = y0 + dvr_display_height;
            if (y_preview_end > OSD_HEIGHT) y_preview_end = OSD_HEIGHT;
            for (int y = y0; y < y_preview_end; y++) {
                uint32_t *p = fb + y * OSD_WIDTH + x0;
                for (int x = x0; x < x1; x++)
                    *p++ |= 0xFF000000;
            }
            /* Dock region: fill opaque black to hide buttons */
            for (int y = y_preview_end; y < y_dock_end; y++) {
                uint32_t *p = fb + y * OSD_WIDTH + x0;
                for (int x = x0; x < x1; x++)
                    *p++ = 0xFF000000;
            }
            CP15_clean_dcache_for_dma(on_screen_addr + y0 * OSD_WIDTH * 4,
                                       on_screen_addr + y_dock_end * OSD_WIDTH * 4);
        }
    }

    printf("DVR: stop_preview done\n");
}

// DVR device online check
uint8_t dvr_is_device_online(void)
{
    return (dvr_task_handle != NULL) ? 1 : 0;
}
#endif

/* ═══════════════════════════════════════════════════════════════════════
 * Link-page (亿连) alpha-clear hook
 *
 * Mirrors DVR's post-render hook architecture.  link_page displays the
 * carlink H264 video stream via LCD_VIDEO_LAYER (OSD0) in a sub-region,
 * with AWTK UI widgets (speed, gear, power, battery, QR) on LCD_UI_LAYER
 * (OSD1) in a non-overlapping panel.
 *
 * Layout (Frame 4):
 *   top bar      : (0,  0)  1024×60   UI layer, opaque
 *   video region : (0, 60)   800×480  VIDEO layer, alpha=0 hole in UI
 *   right panel  : (800,60)  224×480  UI layer, opaque
 *   bottom bar   : (0,548)  1024×52   UI layer, opaque
 *
 * The hook clears alpha in the video region AFTER AWTK rendering completes,
 * so the LCD controller always sees a consistent frame — no flicker.
 * ═══════════════════════════════════════════════════════════════════════ */

#define LINK_VIDEO_X       0
#define LINK_VIDEO_Y       60
#define LINK_VIDEO_WIDTH   800
#define LINK_VIDEO_HEIGHT  480

static uint8_t link_preview_enable = 0;

/* Render gate: checked by h264_video_player_proc() BEFORE any LCD layer
 * operation.  Must be volatile — written by UI task, read by EY network task.
 * Closed (0) BEFORE disabling VIDEO layer, so even an in-flight frame that
 * already passed the link_preview_enable check will be stopped. */
volatile uint8_t g_link_render_gate = 0;

#include "vehicle_param/vehicle_param.h"

extern void vg_set_post_render_hook(void (*hook)(unsigned int fb_base));

/* Debounce counter: VEH_CARLINK_CONNECTED can transiently drop to 0 during
 * WiFi retransmission or H264 stream gap (~1 frame).  Without debounce, the
 * hook skips the alpha-clear for that frame → UI layer is opaque over VIDEO
 * layer → 1-frame black flash.  We require N consecutive not-connected
 * readings before actually stopping the alpha-clear. At 60fps render rate,
 * 6 frames ≈ 100ms — long enough to ride over transients, short enough to
 * stop quickly on real disconnect. */
#define LINK_CONNECTED_DEBOUNCE  6
static uint8_t link_connected_debounce_cnt = 0;

static void link_alpha_clear_hook(unsigned int fb_base)
{
    if (!link_preview_enable) return;

    /* Debounced connection check: once connected, keep punching the alpha
     * hole for LINK_CONNECTED_DEBOUNCE extra frames after disconnect is
     * first observed.  This prevents 1-frame flicker from WiFi transients. */
    if (vehicle_get_data(VEH_CARLINK_CONNECTED) == 1) {
        link_connected_debounce_cnt = 0;  /* reset on every connected frame */
    } else {
        if (link_connected_debounce_cnt < LINK_CONNECTED_DEBOUNCE) {
            link_connected_debounce_cnt++;
            /* Still in grace period — continue punching alpha hole */
        } else {
            /* Truly disconnected: stop punching.  AWTK's bg_color and QR
             * widget will paint the region (black bg + QR code visible). */
            return;
        }
    }

    int x0 = LINK_VIDEO_X;
    int y0 = LINK_VIDEO_Y;
    int x1 = x0 + LINK_VIDEO_WIDTH;
    int y1 = y0 + LINK_VIDEO_HEIGHT;
    if (x1 > OSD_WIDTH)  x1 = OSD_WIDTH;
    if (y1 > OSD_HEIGHT) y1 = OSD_HEIGHT;

    /* Full-width row: fill as one contiguous run (same optimisation as DVR).
     * Writes 0x00000000 (alpha=0, RGB=0) so VIDEO layer shows through. */
    if (x0 == 0 && x1 == OSD_WIDTH) {
        uint32_t *p   = (uint32_t *)fb_base + (uint32_t)y0 * OSD_WIDTH;
        uint32_t *end = (uint32_t *)fb_base + (uint32_t)y1 * OSD_WIDTH;
        while (p < end) *p++ = 0x00000000;
    } else {
        for (int y = y0; y < y1; y++) {
            uint32_t *p = (uint32_t *)fb_base + (uint32_t)y * OSD_WIDTH + x0;
            for (int x = x0; x < x1; x++)
                *p++ = 0x00000000;
        }
    }
    CP15_clean_dcache_for_dma(fb_base + (uint32_t)y0 * OSD_WIDTH * 4,
                               fb_base + (uint32_t)y1 * OSD_WIDTH * 4);
}

/* Restore alpha=0xFF in the link video region on ALL framebuffers.
 * Called on preview exit so the alpha hole doesn't persist in back-buffers
 * that AWTK might not immediately repaint (triple-buffering). Mirrors
 * dvr_restore_ui_alpha_all() for the DVR preview path. */
static void link_restore_ui_alpha_all(void)
{
    int x0 = LINK_VIDEO_X, y0 = LINK_VIDEO_Y;
    int x1 = x0 + LINK_VIDEO_WIDTH, y1 = y0 + LINK_VIDEO_HEIGHT;
    if (x1 > OSD_WIDTH)  x1 = OSD_WIDTH;
    if (y1 > OSD_HEIGHT) y1 = OSD_HEIGHT;
    for (int i = 0; i < 3; i++) {
        uint8_t *addr = ark_lcd_get_fb_addr(i);
        if (!addr) continue;
        uint32_t *fb = (uint32_t *)addr;
        for (int y = y0; y < y1; y++) {
            uint32_t *p = fb + y * OSD_WIDTH + x0;
            for (int x = x0; x < x1; x++)
                *p++ |= 0xFF000000;
        }
        CP15_clean_dcache_for_dma((uint32_t)addr + y0 * OSD_WIDTH * 4,
                                   (uint32_t)addr + y1 * OSD_WIDTH * 4);
    }
}

void link_api_set_preview_enable(uint8_t enable)
{
    printf("LINK: preview enable set to %d\n", enable);

    if (enable) {
        link_preview_enable = 1;
        /* Reset debounce counter for fresh session */
        link_connected_debounce_cnt = LINK_CONNECTED_DEBOUNCE;

        /* Position carlink VIDEO layer to match the alpha-clear region */
        set_carlink_display_info(LINK_VIDEO_X, LINK_VIDEO_Y,
                                 LINK_VIDEO_WIDTH, LINK_VIDEO_HEIGHT);

        /* ── Pre-fill all 3 framebuffers with opaque black in the video
         * region.  navigator_switch_to() keeps home_page alive underneath
         * link_page, and AWTK's triple-buffer may still hold home_page
         * wallpaper in one or two back-buffers that haven't been repainted
         * yet.  If the LCD controller shows one of those stale buffers
         * before AWTK gets around to repainting it, the user sees a
         * home_page "ghost" — exactly the A-level bug reported by QA.
         *
         * By pre-filling all 3 buffers with opaque black NOW, we ensure
         * every possible VSYNC swap shows black in this region until either
         * (a) AWTK repaints with link_page bg_color + QR (not connected),
         * or (b) the hook starts punching alpha holes (connected).
         *
         * Cost: 800×480×4 bytes × 3 buffers ≈ 4.6MB write, one-time at
         * page open.  Negligible vs. 30fps H264 decode bandwidth. */
        {
            int x0 = LINK_VIDEO_X, y0 = LINK_VIDEO_Y;
            int x1 = x0 + LINK_VIDEO_WIDTH, y1 = y0 + LINK_VIDEO_HEIGHT;
            if (x1 > OSD_WIDTH)  x1 = OSD_WIDTH;
            if (y1 > OSD_HEIGHT) y1 = OSD_HEIGHT;
            for (int i = 0; i < 3; i++) {
                uint8_t *addr = ark_lcd_get_fb_addr(i);
                if (!addr) continue;
                uint32_t *fb = (uint32_t *)addr;
                for (int y = y0; y < y1; y++) {
                    uint32_t *p = fb + y * OSD_WIDTH + x0;
                    for (int x = x0; x < x1; x++)
                        *p++ = 0xFF000000;  /* opaque black */
                }
                CP15_clean_dcache_for_dma((uint32_t)addr + y0 * OSD_WIDTH * 4,
                                           (uint32_t)addr + y1 * OSD_WIDTH * 4);
            }
        }

        vg_set_post_render_hook(link_alpha_clear_hook);
        /* Open render gate LAST — after all resources are configured,
         * so h264_video_player_proc never sees gate=1 with stale state. */
        g_link_render_gate = 1;
        printf("LINK: alpha-clear hook registered, video at (%d,%d,%d,%d)\n",
               LINK_VIDEO_X, LINK_VIDEO_Y, LINK_VIDEO_WIDTH, LINK_VIDEO_HEIGHT);
    } else {
        /* === CRITICAL SHUTDOWN SEQUENCE ===
         * The EY network task calls h264_video_player_proc() at ~30fps in a
         * separate FreeRTOS task.  If we disable LCD_VIDEO_LAYER while an
         * in-flight frame has already passed the preview_enable check, that
         * frame will re-enable VIDEO_LAYER after us → 1-frame flicker.
         *
         * Solution: close the render gate FIRST (volatile write, immediately
         * visible to EY task), then drain one frame period so any in-flight
         * frame completes or aborts at the gate check.  Only then touch LCD. */

        /* Step 1: Close render gate — EY task will abort at g_link_render_gate
         * check before any LCD operation, even if it already read
         * link_preview_enable==1. */
        g_link_render_gate = 0;

        /* Step 2: Set the preview flag — future h264_video_player_proc calls
         * will exit at the link_api_get_preview_enable() check. */
        link_preview_enable = 0;

        /* Step 3: Drain one video frame period (~35ms at 30fps) to guarantee
         * any in-flight h264_video_player_proc call has either completed its
         * LCD operations or aborted at the gate check. Without this delay,
         * an in-flight frame that entered the render loop before step 1
         * could still be executing pxp_scaler_rotate/LCD writes. */
        vTaskDelay(pdMS_TO_TICKS(35));

        /* Step 4: Now safe to disable VIDEO layer — no concurrent writer */
        ark_lcd_osd_enable(LCD_VIDEO_LAYER, 0);
        ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
        ark_lcd_osd_enable(LCD_UI_LAYER, 1);
        ark_lcd_set_osd_sync(LCD_UI_LAYER);

        /* Step 5: Remove post-render hook */
        vg_set_post_render_hook(NULL);

        /* Step 6: Restore alpha in ALL framebuffers so the alpha hole from
         * link_alpha_clear_hook doesn't persist in triple-buffered back-buffers
         * that AWTK may swap to before repainting the region. */
        link_restore_ui_alpha_all();

        /* Step 7: Restore carlink VIDEO layer geometry to full screen */
        set_carlink_display_info(0, 0, LCD_WIDTH, LCD_HEIGHT);
        printf("LINK: gate closed, VIDEO off, alpha restored, display full\n");
    }
}

uint8_t link_api_get_preview_enable(void)
{
    return link_preview_enable;
}
#ifdef VG_DRIVER
#pragma data_alignment=1024
#ifdef __HCN_CONFIG_H__
#define VG_HEAP_SIZE  HCN_VG_HEAP_SIZE
#else
#define VG_HEAP_SIZE	0xc00000
#endif
__no_init static uint8_t vgHeap[VG_HEAP_SIZE];
#endif

#ifdef CARLINK_ENABLE
static char qr_text_buf[256] = {0};	//手机互联二维码数据缓存

int get_qr_text_buf(char *buf, int len)
{
	int ret = -1;
	int qr_len = strlen(qr_text_buf);

	if ((qr_len > 0) && (len >= qr_len)) {
		strcpy(buf, qr_text_buf);
		ret = 0;
	}

	return ret;
}

void set_qr_text_buf(const char *str)
{
	if (strlen(str) < sizeof(qr_text_buf)) {
		strcpy(qr_text_buf, str);
	}
}
#endif

#if !defined(VG_ONLY) && !defined(AWTK)

#else
typedef int16_t lv_coord_t;
typedef uint8_t lv_indev_state_t;
typedef struct {
    lv_coord_t x;
    lv_coord_t y;
} lv_point_t;

enum { LV_INDEV_STATE_REL = 0, LV_INDEV_STATE_PR };

enum {
    LV_KEY_UP        = 17,  /*0x11*/
    LV_KEY_DOWN      = 18,  /*0x12*/
    LV_KEY_RIGHT     = 19,  /*0x13*/
    LV_KEY_LEFT      = 20,  /*0x14*/
    LV_KEY_ESC       = 27,  /*0x1B*/
    LV_KEY_DEL       = 127, /*0x7F*/
    LV_KEY_BACKSPACE = 8,   /*0x08*/
    LV_KEY_ENTER     = 10,  /*0x0A, '\n'*/
    LV_KEY_NEXT      = 9,   /*0x09, '\t'*/
    LV_KEY_PREV      = 11,  /*0x0B, '*/
    LV_KEY_HOME      = 2,   /*0x02, STX*/
    LV_KEY_END       = 3,   /*0x03, ETX*/
};

typedef struct {
    lv_point_t point; /**< For LV_INDEV_TYPE_POINTER the currently pressed point*/
    uint32_t key;     /**< For LV_INDEV_TYPE_KEYPAD the currently pressed key*/
    uint32_t btn_id;  /**< For LV_INDEV_TYPE_BUTTON the currently pressed button*/
    int16_t enc_diff; /**< For LV_INDEV_TYPE_ENCODER number of steps since the previous read*/

    lv_indev_state_t state; /**< LV_INDEV_STATE_REL or LV_INDEV_STATE_PR*/
} lv_indev_data_t;
#endif


/* define dummy function to avoid linker error */
void SendTouchInputEvent(void *indata)
{
}

void SendTouchInputEventFromISR(void *indata)
{
}

void SendKeypadInputEvent(void *indata)
{
}
extern void carlink_send_key_event(uint8_t key, bool pressed);

void SendKeypadInputEventFromISR(void *indata)
{
#ifdef HCN_ADC_KEY_ENABLE
	lv_indev_data_t* input = (lv_indev_data_t *)indata;
#endif
#if 0
	if (input->key == 2) {
		printf("key = LV_KEY_HOME\r\n");
	} else if (input->key == 10) {
		printf("key = LV_KEY_ENTER\r\n");
	} else if (input->key == 27) {
		printf("key = LV_KEY_ESC\r\n");
	} else if (input->key == 17) {
		printf("key = LV_KEY_UP\r\n");
	} else if (input->key == 18) {
		printf("key = LV_KEY_DOWN\r\n");
	} else if (input->key == 19) {
		printf("key = LV_KEY_RIGHT\r\n");
	} else if (input->key == 20) {
		printf("key = LV_KEY_LEFT\r\n");
	}
#else
	#ifdef HCN_ADC_KEY_ENABLE
	send_keypad_event_isr(input->key, input->state);
	#endif
#endif
	//carlink_send_key_event((uint8_t)input->key, (bool)input->state);
}
#ifdef WIFI_SUPPORT
#if WIFI_TEST
//#define RELTECK_WIFI_AP_MODE

#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_DHCP.h"
#include "carlink_ey.h"
#include "carlink_ey_video.h"
#include "iperf_task.h"
#include "iot_wifi.h"
#include "FreeRTOS_DHCP_Server.h"
#ifdef RELTECK_WIFI_AP_MODE
static const uint8_t ucIPAddress[4] = {192, 168, 13, 1};
#else
static const uint8_t ucIPAddress[4] = {192, 168, 13, 37};
#endif
static const uint8_t ucNetMask[4] = {255, 255, 255, 0};
//static const uint8_t ucGatewayAddress[4] = {192, 168, 13, 1};
static const uint8_t ucGatewayAddress[4] = {192, 168, 13, 1};
static const uint8_t ucDNSServerAddress[4] = {8, 8, 8, 8};
//static const uint8_t ucMACAddress[6] = {0x00, 0x0c, 0x29, 0x5d, 0x2e, 0x03};
//static const uint8_t ucMACAddress[6] = {0x68, 0xb9, 0xd3, 0xc1, 0x28, 0x03};
static const uint8_t ucMACAddress[6] = {0x30, 0x4a, 0x26, 0x78, 0xfd, 0x12};
uint8_t wifi_data_buffer[65536] = {0};
void ark_test_h264_dec();

struct test_header
{
	uint16_t id;
	uint16_t payload_len;
};
#if 0
static int vCreateTCPServerSocket( void )
{
	SocketSet_t xFD_Set;
	struct freertos_sockaddr xAddress, xRemoteAddr;
	Socket_t xSockets = FREERTOS_INVALID_SOCKET, xClientSocket = FREERTOS_INVALID_SOCKET;
	socklen_t xClientLength = sizeof( xAddress );
	static const TickType_t xNoTimeOut = portMAX_DELAY;

	BaseType_t ret = -1;
	BaseType_t xResult;
	struct test_header header;
	uint8_t header_buf[4];
	uint8_t* header_buf_ptr;
	const int header_len = sizeof(struct test_header);
	int header_buf_len = 0;
	uint8_t *h264SrcBuf = NULL;
	uint8_t *h264SrcBufPtr = NULL;
	int32_t h264SrcSize = 0, h264SrcSizePos = 0;
	uint8_t err_flag = 0;int i;
	video_frame_s* frame = NULL;

	xFD_Set = FreeRTOS_CreateSocketSet();
	xSockets = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    configASSERT( xSockets != FREERTOS_INVALID_SOCKET );
    FreeRTOS_setsockopt( xSockets,
                         0,
                         FREERTOS_SO_RCVTIMEO,
                         &xNoTimeOut,
                         sizeof( xNoTimeOut ) );
    xAddress.sin_port = ( uint16_t ) 11111;
    xAddress.sin_port = FreeRTOS_htons( xAddress.sin_port );
    FreeRTOS_bind( xSockets, &xAddress, sizeof( xAddress ) );
    FreeRTOS_listen( xSockets, 1 );
	//ark_test_h264_dec();

	carlink_ey_video_init();

	while (1) {
		FreeRTOS_FD_CLR(xSockets, xFD_Set, eSELECT_READ);
		FreeRTOS_FD_SET(xSockets, xFD_Set, eSELECT_READ);
		if (xClientSocket && xClientSocket != FREERTOS_INVALID_SOCKET) {
			FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
			FreeRTOS_FD_SET( xClientSocket, xFD_Set, eSELECT_READ );
		}

    	xResult = FreeRTOS_select( xFD_Set, portMAX_DELAY );
        if (xResult < 0) {
			break;
        }

		if( FreeRTOS_FD_ISSET ( xSockets, xFD_Set ) ) {
			xClientSocket = FreeRTOS_accept( xSockets, &xRemoteAddr, &xClientLength);
			if( ( xClientSocket != NULL ) && ( xClientSocket != FREERTOS_INVALID_SOCKET ) ) {
				uint8_t pucBuffer[32] = {0};
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_FD_SET(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_GetRemoteAddress( xClientSocket, ( struct freertos_sockaddr * ) &xRemoteAddr );
				FreeRTOS_inet_ntoa(xRemoteAddr.sin_addr, pucBuffer );
				printf("Carlink: Received a connection from %s:%u\n", pucBuffer, FreeRTOS_ntohs(xRemoteAddr.sin_port));
			}
			continue;
        } else if( FreeRTOS_FD_ISSET ( xClientSocket, xFD_Set ) ) {
			header_buf_ptr = header_buf;
			header_buf_len = header_len;
			err_flag = 0;
			while (header_buf_len > 0) {
				err_flag = 0;
				ret = FreeRTOS_recv(xClientSocket, (void*)header_buf_ptr, header_buf_len, 0);
				if (ret < 0) {
					err_flag = 1;
					printf("FreeRTOS_recv header err:%d\r\n", ret);
					break;
				}
				header_buf_ptr += ret;
				header_buf_len -= ret;
			}
			if (err_flag) {
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_closesocket(xClientSocket);
				xClientSocket = FREERTOS_INVALID_SOCKET;
				video_frame_s* dummy = NULL;
				notify_h264_frame_ready(&dummy);
				continue;
			}
			/*printf("##header:");

			for (i = 0; i < header_len; i++) {
				printf("%02x ", header_buf[i]);
			}printf("\r\n");*/

			//READ_LE16(header_buf, header.id);
			//READ_LE16(header_buf + 2, header.payload_len);
			header.id = (header_buf[0] | (header_buf[1] << 8));
			header.payload_len = (header_buf[2] | (header_buf[3] << 8));
			printf("recv id:%d len:%d\r\n", header.id, header.payload_len);

			int retry_cnt = 0;
			h264SrcSize = header.payload_len;
get_retry:
			frame = get_h264_frame_buf();
			if (NULL == frame) {
				printf("h264 frame is empty\r\n");
				vTaskDelay(pdMS_TO_TICKS(10));
				goto get_retry;
				//continue;
			}

			h264SrcSizePos = h264SrcSize;
			h264SrcBufPtr  = frame->cur;
			h264SrcBuf     = frame->cur;
			frame->len     = h264SrcSize;
			err_flag       = 0;
			while (h264SrcSizePos > 0) {
				//printf("h264SrcSizePos:%d\r\n", h264SrcSizePos);
				ret = FreeRTOS_recv( xClientSocket, (void *)h264SrcBufPtr, h264SrcSizePos, 0);
				//printf("lBytes:%d h264SrcSizePos:%d\r\n", lBytes, h264SrcSizePos);
				if (ret < 0) {
					printf("FreeRTOS_recv err:%d\r\n", ret);
					err_flag = 1;
					break;
				}
				h264SrcBufPtr  += ret;
				h264SrcSizePos -= ret;
			}/*printf("read finished\r\n");

			printf("payload:");

			for (i = 0; i < 16; i++) {
				printf("%02x ", h264SrcBuf[i]);
			}printf("\r\n");*/

			if (err_flag) {
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_closesocket(xClientSocket);
				xClientSocket = FREERTOS_INVALID_SOCKET;
				video_frame_s* dummy = NULL;
				notify_h264_frame_ready(&dummy);
				continue;
			}
			notify_h264_frame_ready(&frame);
		}
	}


	FreeRTOS_closesocket(xClientSocket);
	FreeRTOS_closesocket(xSockets);
}
#else
static int vCreateTCPServerSocket( void )
{
	SocketSet_t xFD_Set;
	struct freertos_sockaddr xAddress, xRemoteAddr;
	Socket_t xSockets = FREERTOS_INVALID_SOCKET, xClientSocket = FREERTOS_INVALID_SOCKET;
	socklen_t xClientLength = sizeof( xAddress );
	static const TickType_t xNoTimeOut = portMAX_DELAY;
	BaseType_t ret = -1;
	BaseType_t xResult;

	xFD_Set = FreeRTOS_CreateSocketSet();
	xSockets = FreeRTOS_socket( FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP );
    configASSERT( xSockets != FREERTOS_INVALID_SOCKET );
    FreeRTOS_setsockopt( xSockets,
                         0,
                         FREERTOS_SO_RCVTIMEO,
                         &xNoTimeOut,
                         sizeof( xNoTimeOut ) );
    xAddress.sin_port = ( uint16_t ) 11111;
    xAddress.sin_port = FreeRTOS_htons( xAddress.sin_port );
    FreeRTOS_bind( xSockets, &xAddress, sizeof( xAddress ) );
    FreeRTOS_listen( xSockets, 1 );
	//ark_test_h264_dec();

	carlink_ey_video_init();

	while (1) {
		FreeRTOS_FD_CLR(xSockets, xFD_Set, eSELECT_READ);
		FreeRTOS_FD_SET(xSockets, xFD_Set, eSELECT_READ);
		if (xClientSocket && xClientSocket != FREERTOS_INVALID_SOCKET) {
			FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
			FreeRTOS_FD_SET( xClientSocket, xFD_Set, eSELECT_READ );
		}

    	xResult = FreeRTOS_select( xFD_Set, portMAX_DELAY );
        if (xResult < 0) {
			break;
        }

		if( FreeRTOS_FD_ISSET ( xSockets, xFD_Set ) ) {
			xClientSocket = FreeRTOS_accept( xSockets, &xRemoteAddr, &xClientLength);
			if( ( xClientSocket != NULL ) && ( xClientSocket != FREERTOS_INVALID_SOCKET ) ) {
				char pucBuffer[32] = {0};
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_FD_SET(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_GetRemoteAddress( xClientSocket, ( struct freertos_sockaddr * ) &xRemoteAddr );
				FreeRTOS_inet_ntoa(xRemoteAddr.sin_addr, pucBuffer );
				printf("Carlink: Received a connection from %s:%u\n", pucBuffer, FreeRTOS_ntohs(xRemoteAddr.sin_port));
			}
			continue;
        } else if( FreeRTOS_FD_ISSET ( xClientSocket, xFD_Set ) ) {

			ret = FreeRTOS_recv(xClientSocket, wifi_data_buffer, sizeof wifi_data_buffer, 0);
			if (ret > 0) {
				printf("recv buf size:%d\r\n", ret);
			} else {
				printf("FreeRTOS_recv err:%d\r\n", ret);
				FreeRTOS_FD_CLR(xClientSocket, xFD_Set, eSELECT_READ);
				FreeRTOS_closesocket(xClientSocket);
				xClientSocket = FREERTOS_INVALID_SOCKET;
				continue;
			}
		}
	}


	FreeRTOS_closesocket(xClientSocket);
	FreeRTOS_closesocket(xSockets);
        return 0;
}

#endif

#if ( ipconfigUSE_DHCP_HOOK != 0 )
eDHCPCallbackAnswer_t xApplicationDHCPHook2( eDHCPCallbackPhase_t eDHCPPhase,
                                            uint32_t ulIPAddress )
{
	eDHCPCallbackAnswer_t eReturn;
	//uint32_t ulStaticIPAddress, ulStaticNetMask;
    char ip_str[20] = {0};
        sprintf(ip_str, "%d.%d.%d.%d\r\n", (ulIPAddress >> 0) & 0xFF,
    	(ulIPAddress >> 8) & 0xFF, (ulIPAddress >> 16) & 0xFF, (ulIPAddress >> 24) & 0xFF);
    printf("\r\n  eDHCPPhase:%d  ulIPAddress:%s state:%d \r\n", eDHCPPhase, ip_str, getDhcpClientState());
	if (getDhcpClientState() == 0)
		return eDHCPStopNoChanges;

	switch( eDHCPPhase )
	{
	case eDHCPPhasePreDiscover  :
	  eReturn = eDHCPContinue;
	  break;

	case eDHCPPhasePreRequest  :
 #if 0
	  ulStaticIPAddress = FreeRTOS_inet_addr_quick( ucIPAddress[0],
	                                                ucIPAddress[1],
	                                                ucIPAddress[2],
	                                                ucIPAddress[3] );

	  ulStaticNetMask = FreeRTOS_inet_addr_quick( ucNetMask[0],
	                                              ucNetMask[1],
	                                              ucNetMask[2],
	                                              ucNetMask[3] );

	  ulStaticIPAddress &= ulStaticNetMask;
	  ulIPAddress &= ulStaticNetMask;
	  if( ulStaticIPAddress == ulIPAddress ) {
	    eReturn = eDHCPUseDefaults;
	  } else {
	    eReturn = eDHCPContinue;
	  }
     #else
     eReturn = eDHCPContinue;
     #endif
	  break;
	default :
	  eReturn = eDHCPContinue;
	  break;
	}

       return eReturn;
}
#endif

void wifi_test_event_handler( WIFIEvent_t * xEvent )
{
    WIFIEventType_t xEventType = xEvent->xEventType;

    if (0) {
    } else if (eWiFiEventConnected                    == xEventType) {// meter is sta
        printf("\r\n The meter is connected to ap \r\n");
    } else if (eWiFiEventDisconnected                == xEventType) {// meter is sta
        printf("\r\n The meter is disconnected from ap \r\n");
    } else if (eWiFiEventAPStationConnected       == xEventType) {// meter is ap
        printf("\r\n The meter in AP is connected by a sta \r\n");
    } else if (eWiFiEventAPStationDisconnected   == xEventType) {// meter is ap
        printf("\r\n The sta is disconnected from the meter \r\n");
    }
}

int wifi_sta_test_proc();
int wifi_ap_test_proc();
static void wifi_demo_test(void)
{
	BaseType_t ret = 0;
	unsigned int status;
	uint32_t IPAddress = (32 << 24) | (13 << 16) | (168 << 8) | (192 << 0);

	for (;;) {
		status = mmcsd_wait_sdio_ready((int32_t)portMAX_DELAY);
		if (status == MMCSD_HOST_PLUGED) {
			printf("detect sdio device\r\n");
			break;
		}
	}
#ifndef RELTECK_WIFI_AP_MODE
    setDhcpClientState(1);
#endif
       ret = ret;
	//vTaskDelay(pdMS_TO_TICKS(5000));//wait connect
	ret = FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress,ucDNSServerAddress, ucMACAddress);
	//ark_wlan_init();
#ifdef RELTECK_WIFI_AP_MODE
	wifi_ap_test_proc();
#else
         WIFI_RegisterEvent(eWiFiEventMax, wifi_test_event_handler);
	wifi_sta_test_proc();
#endif
	//vTaskDelay(pdMS_TO_TICKS(8000));
	while(0) {
		//printf("send ping\r\n");
		FreeRTOS_SendPingRequest(IPAddress, 8, 1000);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	vTaskDelay(pdMS_TO_TICKS(1000));
#ifdef RELTECK_WIFI_AP_MODE
	setDhcpClientState(0);
	IPAddress = (20 << 24) | (13 << 16) | (168 << 8) | (192 << 0);
	dhcpserver_start(ucIPAddress, IPAddress, 10);
#else
	setDhcpClientState(1);
#endif
	//vCreateTCPServerSocket();
	vIPerfInstall();
}
#endif
#endif

#ifdef SDMMC_SUPPORT
#if SDMMC_TEST
static void sdcard_read_thread(void *para)
{
	unsigned int status;

	for (;;) {
		status = mmcsd_wait_cd_changed(portMAX_DELAY);
		if (status == MMCSD_HOST_PLUGED) {
			printf("card inserted.\n");
#ifdef OTA_UPDATE_SUPPORT
			FF_FILE *fp = ff_fopen("/sd/update.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/sd", UPFILE_TYPE_WHOLE);
			}

#if DEVICE_TYPE_SELECT == EMMC_FLASH
			fp = ff_fopen("/sd/emmcldr.bin", "rb");
#else
			fp = ff_fopen("/sd/spildr.bin", "rb");
#endif
			if (fp) {
				ff_fclose(fp);
				update_from_media("/sd", UPFILE_TYPE_FIRSTLDR);
			}

			fp = ff_fopen("/sd/stepldr.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/sd", UPFILE_TYPE_STEPLDR);
			}

			fp = ff_fopen("/sd/lnchemmc.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/sd", UPFILE_TYPE_LNCHEMMC);
			}
#else
			FF_FILE *fp = ff_fopen("/sd/update.bin", "rb");
			if (fp) {
				UpFileHeader header;
				SysInfo *sysinfo = GetSysInfo();
				if (ff_fread(&header, 1, sizeof(header), fp) == sizeof(header)) {
					if (header.magic != MKTAG('U', 'P', 'D', 'F')) {
						printf("Wrong update file, don't update.\n");
					} else {
						if (header.checksum != sysinfo->app_checksum) {
							printf("found different update file(0x%x-0x%x), update...\n",
								header.checksum, sysinfo->app_checksum);
							sysinfo->update_media_type = UPDATE_MEDIA_SD;
							sysinfo->update_status = UPDATE_STATUS_START;
							SaveSysInfo();
							wdt_cpu_reboot();
						} else {
							printf("the update file version is same, don't update.\n");
						}
					}
				};
				ff_fclose(fp);
			} else {
				printf("open update.bin fail.\n");
			}

#ifdef HCN_OTA_UPDATE_ENABLE
			{
				FF_FILE *mcu_fp = ff_fopen("/usb/mcu_update.bin", "rb");
				if (mcu_fp) {
					printf("open mcu_update.bin success.\r\n");
					extern bool mcu_req_update_state(void);
					if (!mcu_req_update_state()) {
						mcu_update_init(1, mcu_fp);
					} else {
						mcu_req_update_init(1, mcu_fp);
					}
				} else {
					printf("open mcu_update.bin fail.\n");
				}
			}
#endif

#endif
		} else if (status == MMCSD_HOST_UNPLUGED) {
			printf("card removed.\n");
		}
	}
}

static void sdcard_read_demo(void)
{
	static StaticTask_t xSDReadTaskTCB;
	static StackType_t uxSDReadTaskStack[configMINIMAL_STACK_SIZE * 2];

	if (xTaskCreateStatic(sdcard_read_thread,
							"sdread",
							configMINIMAL_STACK_SIZE * 2,
							NULL,
							1,
							uxSDReadTaskStack,
							&xSDReadTaskTCB) == NULL) {
		printf("create sdread task fail.\n");
	}
}
#endif
#endif

#ifdef USB_SUPPORT
static void usb_read_thread(void *para)
{
	unsigned int status;

	for (;;) {
		status = usb_wait_stor_dev_pluged(portMAX_DELAY);
		if (status == USB_DEV_PLUGED) {
			printf("usb dev inserted.\n");
			hcn_usb_status_change(USB_STATUS_INSERTED);

			#if ENABLE_BD_USB_DVR_FUNC
			// Check if elene file exists and start DVR task if found
			// #ifdef USB_SUPPORT
			dvr_api_set_display_window(0, 0, 1024, 500);
			dvr_start_if_elene_exists();
			// #endif
			#endif

#ifdef OTA_UPDATE_SUPPORT
#ifdef DELTA_UPDATE_SUPPORT
			//Demo从U盘读取patch文件来模拟接收patch文件
			int filetype;
			char filename[32];
			FF_FILE *fp;
			size_t filesize;
			uint8_t *filebuf;
			size_t leftsize;
			size_t wsize;
			uint32_t offset;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
			sfud_flash *sflash = sfud_get_device(0);
#endif

			filebuf = pvPortMalloc(0x10000);
			if (!filebuf) {
				printf("%s filebuf malloc fail.\n", __func__);
				continue;
			}
			for (filetype = UPFILE_TYPE_WHOLE; filetype <= UPFILE_TYPE_STEPLDR; filetype++) {
				strcpy(filename, "/usb/");
				strcat(filename, g_upfilename[filetype]);
				strcpy(strrchr(filename, '.'), "_patch.bin");
				fp = ff_fopen(filename, "rb");
				if (!fp) {
					printf("not found patch file %s.\n", filename);
					continue;
				}
				offset = OTA_MEDIA_OFFSET;
				filesize = ff_filelength(fp);
				leftsize = filesize;
#if DEVICE_TYPE_SELECT != EMMC_FLASH
				sfud_erase(sflash, OTA_MEDIA_OFFSET, filesize);
#endif
				while (leftsize) {
					wsize = leftsize > 0x10000 ? 0x10000 : leftsize;
					ff_fread(filebuf, 1, wsize, fp);
#if DEVICE_TYPE_SELECT == EMMC_FLASH
					emmc_write(offset, wsize, filebuf);
#else
					sfud_write(sflash, offset, wsize, filebuf);
#endif
					offset += wsize;
					leftsize -= wsize;
				}
				ff_fclose(fp);
				delta_update(filetype, filesize);
			}
			vPortFree(filebuf);
#else
			FF_FILE *fp = ff_fopen("/usb/update.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/usb", UPFILE_TYPE_WHOLE);
			}

#if DEVICE_TYPE_SELECT == EMMC_FLASH
			fp = ff_fopen("/usb/emmcldr.bin", "rb");
#else
			fp = ff_fopen("/usb/spildr.bin", "rb");
#endif
			if (fp) {
				ff_fclose(fp);
				update_from_media("/usb", UPFILE_TYPE_FIRSTLDR);
			}

			fp = ff_fopen("/usb/stepldr.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/usb", UPFILE_TYPE_STEPLDR);
			}

			fp = ff_fopen("/usb/lnchemmc.bin", "rb");
			if (fp) {
				ff_fclose(fp);
				update_from_media("/usb", UPFILE_TYPE_LNCHEMMC);
			}
#endif
#else
			FF_FILE *fp = ff_fopen("/usb/update.bin", "rb");
			if (fp) {
				UpFileHeader header;
				SysInfo *sysinfo = GetSysInfo();
				if (ff_fread(&header, 1, sizeof(header), fp) == sizeof(header)) {
					if (header.magic != MKTAG('U', 'P', 'D', 'F')) {
						printf("Wrong update file, don't update.\n");
					} else {
						if (header.checksum != sysinfo->app_checksum) {
							printf("found different update file(0x%x-0x%x), update...\n",
								header.checksum, sysinfo->app_checksum);
							sysinfo->update_media_type = UPDATE_MEDIA_USB;
							sysinfo->update_status = UPDATE_STATUS_START;
							SaveSysInfo();
							hub_usb_dev_reset();
							vTaskDelay(500);
							wdt_cpu_reboot();
						} else {
							printf("the update file version is same, don't update.\n");
						}
					}
				};
				ff_fclose(fp);
			} else {
				printf("open update.bin fail.\n");
			}

#ifdef HCN_OTA_UPDATE_ENABLE
			{
				FF_FILE *mcu_fp = ff_fopen("/usb/mcu_update.bin", "rb");
				if (mcu_fp) {
					printf("open mcu_update.bin success.\r\n");
					extern bool mcu_req_update_state(void);
					if (!mcu_req_update_state()) {
						mcu_update_init(1, mcu_fp);
					} else {
						mcu_req_update_init(1, mcu_fp);
					}
				} else {
					printf("open mcu_update.bin fail.\n");
				}
			}
#endif

#endif
		} else if (status == USB_DEV_UNPLUGED) {
			hcn_usb_status_change(USB_STATUS_REMOVED);
			extern void set_update_state_reset(void);
			set_update_state_reset();
			printf("usb removed.\n");
			#if ENABLE_BD_USB_DVR_FUNC
			if (dvr_task_handle != NULL) {
				dvr_capture.running = 0;
				dvr_capture_deinit(&dvr_capture);
				dvr_close_elene();
				dvr_task_handle = NULL;
				// Restore UI layer and disable video layer
				ark_lcd_osd_enable(LCD_VIDEO_LAYER, 0);
				ark_lcd_set_osd_sync(LCD_VIDEO_LAYER);
				ark_lcd_osd_enable(LCD_UI_LAYER, 1);
				ark_lcd_set_osd_sync(LCD_UI_LAYER);
				printf("DVR: task deleted, resources freed, UI layer restored\n");
			}
			#endif
		}
	}
}

static void usb_read_demo(void)
{
	if (xTaskCreate(usb_read_thread, "usbread", configMINIMAL_STACK_SIZE * 16, NULL,
			1, NULL) != pdPASS) {
		printf("create usbread task fail.\n");
	}
}
#endif
#ifdef USB_SUPPORT
extern int get_usb_mode();
extern int ark_network_init(void);
#endif

void awtk_thread(void *data)
{
	printf("awtk thread start.\n");

#if DEVICE_TYPE_SELECT != EMMC_FLASH
	/* initialize the spi flash */
	sfud_init();
#ifdef SPI0_QSPI_MODE
	sfud_qspi_fast_read_enable(sfud_get_device(0), 4);
#endif
#else
	mmcsd_wait_mmc_ready(portMAX_DELAY);
#endif

#ifdef USE_ULOG
	ulog_init();
#ifdef ULOG_BACKEND_USING_CONSOLE
	ulog_console_backend_init();
#endif
#ifdef ULOG_EASYFLASH_BACKEND_ENABLE
	easyflash_init();
	ulog_ef_backend_init();
#endif
#ifdef ULOG_FILE_BACKEND_ENABLE
	usb_wait_stor_dev_pluged(portMAX_DELAY);
	ulog_file_backend_init();
#endif
	TRACE_INFO("use ulog.\n");
	//read_all_flash_log();
#endif

	/* read sysinfo */
	ReadSysInfo();

	GetUpFileInfo();

	/* initialize carback */
#ifdef CARBACK_DETECT
	// carback_init();
#endif

	/* play animation */
#if ANIMATION_POLICY != ANIMATION_NONE
	animation_init();
	animation_start();
#endif

	/* uart rx demo */
	// uart_rx_demo();

	/* can demo */
	//can_demo();

	/* read sd card demo */
#ifdef SDMMC_SUPPORT
#if SDMMC_TEST
	sdcard_read_demo();
#endif
#endif

#ifdef USB_SUPPORT
	extern int get_usb_mode();
	extern int ark_network_init(void);
	extern void ncm_update_demo();
	extern void ncm_log_demo();
	extern void wifi_update_demo(void);
	if (get_usb_mode()) {
		ark_network_init();
#ifdef NCM_UPDATE_SUPPORT
		ncm_update_demo();
#endif
#ifdef NCM_LOG_SUPPORT
		ncm_log_demo();
#endif
	} else {
#ifdef WIFI_UPDATE_SUPPORT
		wifi_update_demo();
#else
		usb_read_demo();
#endif
	}
#endif


#ifdef WIFI_SUPPORT
#if WIFI_TEST
	wifi_demo_test();
#else
#if USE_LWIP && (0 == CARLINK_EY && 0 == CARLINK_EC)
    //ark_network_init();
#endif

#if CARLINK_EY
	set_carlink_display_info(0, 0, LCD_WIDTH, LCD_HEIGHT);
	set_carlink_video_info(LCD_WIDTH, LCD_HEIGHT, 30);
	carlink_ey_init();
#endif
/* 延时wifi初始化:任何基于wifi的互联(EC/CP/AA)都需要先建立wifi。
 * 原先 hcn_wifi_init() 仅在 CARLINK_EC 下调用,关闭EC后会导致CP无wifi可用。*/
#if (CARLINK_EC || CARLINK_CP || CARLINK_AA) && defined(HCN_WIFI_INIT_DELAY_ENABLE)
	hcn_wifi_init();
#endif

#if CARLINK_EC && !defined(HCN_WIFI_INIT_DELAY_ENABLE)
	set_carlink_display_info(0, 0, LCD_WIDTH, LCD_HEIGHT);
	set_carlink_video_info(LCD_WIDTH, LCD_HEIGHT, 30);
	carlink_ec_init(0, NULL);
#endif

#if CARLINK_AA
	carlink_aa_init();
#endif
#endif
#endif

	hcn_mw_init();

	/* read romfile */
	ReadRomFile();

#ifdef TP_SUPPORT
	extern int tp_init(void);
	tp_init();
#endif

#ifdef VG_DRIVER
	xm_vg_init((unsigned int)vgHeap, VG_HEAP_SIZE);
#else
	extern int gui_app_start(int lcd_w, int lcd_h);
	gui_app_start (OSD_WIDTH, OSD_HEIGHT);
#endif

// ark_lcd_osd_enable(LCD_UI_LAYER,0);


    while(1) {
#ifdef TASK_STATUS_MONITOR
		static uint32_t idletick = 0;
		uint8_t CPU_RunInfo[1024];

		if (xTaskGetTickCount() - idletick > configTICK_RATE_HZ * 10) {
			memset(CPU_RunInfo,0,1024);
			vTaskList((char *)&CPU_RunInfo); //获取任务运行时间信息
			printf("---------------------------------------------\r\n");
			printf("Task          State   Priority  Stack    #\r\n");
			printf("%s", CPU_RunInfo);
			printf("---------------------------------------------\r\n");
			memset(CPU_RunInfo,0,1024);
			vTaskGetRunTimeStats((char *)&CPU_RunInfo);
			printf("Task          Abs Time          % Time\r\n");
			printf("%s", CPU_RunInfo);
			printf("---------------------------------------------\r\n\n");
			idletick = xTaskGetTickCount();
		}
#endif
        vTaskDelay(pdMS_TO_TICKS(1000));       /*Just to let the system breath*/
    }
}

void vRegisterSampleCLICommands( void );
void vUARTCommandConsoleStart( uint16_t usStackSize, UBaseType_t uxPriority );
void main_awtk(void)
{
	/* Start the task that manages the command console for FreeRTOS+CLI. */
	vUARTCommandConsoleStart( ( configMINIMAL_STACK_SIZE * 3 ), tskIDLE_PRIORITY );

	/* Register the standard CLI commands. */
	vRegisterSampleCLICommands();

	xTaskCreate(awtk_thread, "awtk", 32768, NULL,
		tskIDLE_PRIORITY + 1, NULL);
}
#endif