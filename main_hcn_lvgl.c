/**
 * @file main_hcn_lvgl.c
 * @brief HCN 仪表盘 LVGL 模式主入口
 *
 * 从 main_awtk.c 迁移，保留所有硬件/中间件初始化，
 * 将 AWTK gui_app_start() 替换为 LVGL lv_init() + HCN UI 启动。
 *
 * 编译守卫: HCN_SCREEN_ENABLE && !AWTK
 * (当定义 HCN_SCREEN_ENABLE 且不定义 AWTK 时编译)
 *
 * @date  2026-05-20
 * @note  M010 里程碑
 */

#if defined(HCN_SCREEN_ENABLE) && !defined(AWTK)

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "lvgl/lvgl.h"
#include "lv_drivers/display/arklcd.h"
#include "lv_lib_png/lv_png.h"

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

/* 兼容层 */
#include "ui/HCN_DC001/src/lvgl_compat/timer_compat.h"
#include "ui/HCN_DC001/src/lvgl_compat/screen_manager.h"
#include "ui/HCN_DC001/src/lvgl_compat/anim_compat.h"

/*********************
 *      DEFINES
 *********************/
#define WIFI_TEST           0
#define BT_TEST             0
#define SDMMC_TEST          0
#define USB_DEV_PLUGED      0
#define USB_DEV_UNPLUGED    1

/**
 * LVGL 任务栈大小 (单位: words)
 * AWTK 用 32768 words (128KB)
 * LVGL 本身需求远小, 但 HCN UI 层有深调用链和局部大 buffer,
 * 设 8192 words (32KB) 作为安全起始值。
 * 若 vApplicationStackOverflowHook 触发, 需增大此值。
 */
#define HCN_LVGL_TASK_STACK_SIZE  (8192)

/*********************
 *   EXTERN DECLS
 *********************/
int carlink_aa_init(void);
int carlink_cp_init(void);
extern int usb_wait_stor_dev_pluged(uint32_t timeout);
extern void hub_usb_dev_reset(void);
extern int xm_vg_init(unsigned int heap_addr, unsigned int size);
#ifdef DELTA_UPDATE_SUPPORT
extern int delta_update(int filetype, size_t patchFileSize);
#endif

/*********************
 *   VG HEAP
 *********************/
#ifdef VG_DRIVER
#pragma data_alignment=1024
#ifdef __HCN_CONFIG_H__
#define VG_HEAP_SIZE  HCN_VG_HEAP_SIZE
#else
#define VG_HEAP_SIZE  0xc00000
#endif
__no_init static uint8_t vgHeap[VG_HEAP_SIZE];
#endif

/*********************
 *   QR CODE BUFFER
 *********************/
#ifdef CARLINK_ENABLE
static char qr_text_buf[256] = {0};

int get_qr_text_buf(char *buf, int len)
{
    int ret = -1;
    int qr_len = strlen(qr_text_buf);

    if ((qr_len > 0) && (len > qr_len)) {
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

/*********************
 *  INPUT HANDLING
 *********************/

/* Touch/Keypad input queues (same pattern as main_lvgl.c) */
#define INPUT_QUEUE_LEN  16

static QueueHandle_t touch_input_mq = NULL;
static QueueHandle_t keypad_input_mq = NULL;

void SendTouchInputEvent(lv_indev_data_t *indata)
{
    if (touch_input_mq && indata) {
        xQueueSend(touch_input_mq, indata, 0);
    }
}

void SendTouchInputEventFromISR(lv_indev_data_t *indata)
{
    if (touch_input_mq && indata) {
        BaseType_t woken = pdFALSE;
        xQueueSendFromISR(touch_input_mq, indata, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void SendKeypadInputEvent(lv_indev_data_t *indata)
{
    if (keypad_input_mq && indata) {
        xQueueSend(keypad_input_mq, indata, 0);
    }
}

extern void carlink_send_key_event(uint8_t key, bool pressed);

void SendKeypadInputEventFromISR(void *indata)
{
#ifdef HCN_ADC_KEY_ENABLE
    lv_indev_data_t *input = (lv_indev_data_t *)indata;
    send_keypad_event_isr(input->key, input->state);
#endif
}

/*********************
 *  LVGL INPUT DRIVER CALLBACKS
 *********************/

/* Touch input read callback for lv_indev */
static bool touch_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    lv_indev_data_t buf;
    if (touch_input_mq && xQueueReceive(touch_input_mq, &buf, 0) == pdPASS) {
        data->point.x = buf.point.x;
        data->point.y = buf.point.y;
        data->state = buf.state;
        /* Check if more data pending */
        return (uxQueueMessagesWaiting(touch_input_mq) > 0);
    }
    data->state = LV_INDEV_STATE_REL;
    return false;
}

/* Keypad input read callback for lv_indev */
static bool keypad_input_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    lv_indev_data_t buf;
    if (keypad_input_mq && xQueueReceive(keypad_input_mq, &buf, 0) == pdPASS) {
        data->key = buf.key;
        data->state = buf.state;
        return (uxQueueMessagesWaiting(keypad_input_mq) > 0);
    }
    data->state = LV_INDEV_STATE_REL;
    return false;
}

/*********************
 *  SD CARD THREAD (from main_awtk.c)
 *********************/
#ifdef SDMMC_SUPPORT
static StackType_t sdcard_stack[configMINIMAL_STACK_SIZE];
static StaticTask_t sdcard_tcb;

static void sdcard_read_thread(void *param)
{
    (void)param;
    int sd_ready = 0;

    printf("[HCN] SD card thread started\n");
    while (1) {
        int status = mmcsd_wait_cd_changed(rt_tick_from_millisecond(500));
        if (status == MMCSD_HOST_PLUGED) {
            if (!sd_ready) {
                /* Mount filesystem */
                printf("[HCN] SD card inserted\n");
#ifdef OTA_UPDATE_SUPPORT
                ota_update();
#endif
                sd_ready = 1;
            }
        } else if (status == MMCSD_HOST_UNPLUGED) {
            sd_ready = 0;
            printf("[HCN] SD card removed\n");
        }
    }
}
#endif

/*********************
 *  USB THREAD (from main_awtk.c)
 *********************/
#ifdef USB_SUPPORT
static void usb_read_thread(void *param)
{
    (void)param;
    printf("[HCN] USB thread started\n");

    while (1) {
        int ret = usb_wait_stor_dev_pluged(portMAX_DELAY);
        if (ret == USB_DEV_PLUGED) {
            printf("[HCN] USB device plugged\n");
#ifdef OTA_UPDATE_SUPPORT
            ota_update();
#endif
        } else if (ret == USB_DEV_UNPLUGED) {
            printf("[HCN] USB device unplugged\n");
        }
    }
}
#endif

/*********************
 *  HCN LVGL UI ENTRY
 *********************/

/**
 * Forward declaration of HCN application init
 * Implemented in application.c (M021)
 */
extern int application_init(void);

/**
 * Initialize LVGL display/input drivers using SDK's hal_init pattern
 */
extern void hal_init(void);

static void hcn_lvgl_thread(void *param)
{
    (void)param;

    printf("[HCN-LVGL] Thread started (stack=%d words)\n", HCN_LVGL_TASK_STACK_SIZE);

    /* --- Hardware init (from main_awtk.c awtk_thread) --- */

#ifdef USE_ULOG
#ifdef ULOG_BACKEND_USING_CONSOLE
    ulog_console_backend_init();
#endif
#endif

    /* SPI NOR Flash */
    sfud_init();

    /* SD card thread */
#ifdef SDMMC_SUPPORT
    mmcsd_core_init();
    mmcsd_change_detect();
    if (xTaskCreateStatic(sdcard_read_thread,
                          "sdread", configMINIMAL_STACK_SIZE,
                          NULL, 8, sdcard_stack, &sdcard_tcb) == NULL) {
        printf("[HCN] WARN: sdcard task create failed\n");
    }
#endif

    /* EasyFlash */
#ifdef ULOG_EASYFLASH_BACKEND_ENABLE
    easyflash_init();
#endif

    /* USB */
#ifdef USB_SUPPORT
    extern int get_usb_mode(void);
    extern int ark_network_init(void);

    if (get_usb_mode()) {
        ark_network_init();
    } else {
        if (xTaskCreate(usb_read_thread, "usbread",
                        configMINIMAL_STACK_SIZE * 16, NULL, 8, NULL) != pdPASS) {
            printf("[HCN] WARN: usb task create failed\n");
        }
    }
#endif

    /* WiFi / CarLink */
#ifdef WIFI_SUPPORT
#if CARLINK_EY
    set_carlink_display_info(0, 0, LCD_WIDTH, LCD_HEIGHT);
    set_carlink_video_info(LCD_WIDTH, LCD_HEIGHT, 30);
    carlink_ey_init();
#endif
#if CARLINK_EC
#ifndef HCN_WIFI_INIT_DELAY_ENABLE
    set_carlink_display_info(0, 0, LCD_WIDTH, LCD_HEIGHT);
    set_carlink_video_info(LCD_WIDTH, LCD_HEIGHT, 30);
    carlink_ec_init(0, NULL);
#else
    hcn_wifi_init();
#endif
#endif
#if CARLINK_CP
    carlink_cp_init();
#endif
#if CARLINK_AA
    carlink_aa_init();
#endif
#endif

    /* HCN middleware (UART, CAN, BT, etc.) */
    hcn_mw_init();

    /* Read ROM file system (AWTK resources — will be replaced by LVGL assets) */
    ReadRomFile();

    /* Touch panel */
#ifdef TP_SUPPORT
    extern int tp_init(void);
    tp_init();
#endif

    /* --- LVGL init --- */
    lv_init();
    hal_init();

#if defined(VG_DRIVER) && !defined(LVGL_VG_GPU)
    xm_vg_init((unsigned int)vgHeap, VG_HEAP_SIZE);
#endif

    lv_png_init();

    /* Register touch input device */
    {
        lv_indev_drv_t drv;
        lv_indev_drv_init(&drv);
        drv.type = LV_INDEV_TYPE_POINTER;
        drv.read_cb = touch_input_read;
        lv_indev_drv_register(&drv);
    }

    /* Register keypad input device */
    {
        lv_indev_drv_t drv;
        lv_indev_drv_init(&drv);
        drv.type = LV_INDEV_TYPE_KEYPAD;
        drv.read_cb = keypad_input_read;
        lv_indev_t *kb_indev = lv_indev_drv_register(&drv);

        /* Create a default group for keypad navigation */
        lv_group_t *g = lv_group_create();
        lv_indev_set_group(kb_indev, g);
        lv_group_set_default(g);
    }

    /* --- Init compat layers --- */
    timer_compat_init();
    screen_mgr_init();
    anim_compat_init();

    printf("[HCN-LVGL] LVGL initialized, starting HCN UI...\n");

    /* --- Launch HCN application UI --- */
    application_init();

    printf("[HCN-LVGL] HCN UI started, entering main loop\n");

    /* --- Main loop: drive LVGL task handler --- */
    while (1) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(5));  /* 5ms → 200 FPS max */
    }
}

/*********************
 *  ENTRY POINT
 *********************/

void vRegisterSampleCLICommands(void);
void vUARTCommandConsoleStart(uint16_t usStackSize, UBaseType_t uxPriority);

/**
 * main_lvgl() — called from main.c when !AWTK && !VG_ONLY
 *
 * This function replaces both main_awtk() and the SDK's main_lvgl()
 * for HCN dashboard mode.
 */
void main_lvgl(void)
{
    /* CLI console */
    vUARTCommandConsoleStart((configMINIMAL_STACK_SIZE * 3), tskIDLE_PRIORITY);
    vRegisterSampleCLICommands();

    /* Create HCN LVGL thread */
    if (xTaskCreate(hcn_lvgl_thread, "lvgl",
                    HCN_LVGL_TASK_STACK_SIZE, NULL,
                    tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        printf("[HCN-LVGL] FATAL: cannot create lvgl task\n");
    }

    /* Create input queues */
    touch_input_mq = xQueueCreate(INPUT_QUEUE_LEN, sizeof(lv_indev_data_t));
    if (touch_input_mq == NULL) {
        printf("[HCN-LVGL] FATAL: touch queue create failed\n");
    }

    keypad_input_mq = xQueueCreate(INPUT_QUEUE_LEN, sizeof(lv_indev_data_t));
    if (keypad_input_mq == NULL) {
        printf("[HCN-LVGL] FATAL: keypad queue create failed\n");
    }
}

#endif /* HCN_SCREEN_ENABLE && !AWTK */
