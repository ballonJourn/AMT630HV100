/**
*
* @file hal_wifi.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/09 10:34
* @author och
*
*/

#include <FreeRTOS.h>
#include "board.h"
#include "hal_wifi/hal_wifi.h"
#include "config/hcn_config.h"
#include "log/hcn_log.h"
#include "pinctrl.h"
#include "carlink_ec.h"
#include "carlink_video.h"
#include "gpio.h"
#include "task.h"

#ifdef HCN_WIFI_INIT_DELAY_ENABLE

static TaskHandle_t wifi_init_task = NULL;

extern void reset_rescan();
extern int carlink_wifi_init();

static void wifi_init_thread(void *param) {
    int reset_sdio = false;
reset_wifi:
    if (reset_sdio) {
        ///< 复位时拉低121串口以及SDIO_CLK、SDIO通讯
        gpio_direction_output(WIFI_BT_SDO_CMD_GPIO, 0);
        gpio_direction_output(WIFI_BT_SDO_CLK_GPIO, 0);
        gpio_direction_output(WIFI_BT_SDO_D3_GPIO, 0);
        gpio_direction_output(WIFI_BT_SDO_D2_GPIO, 0);
        gpio_direction_output(WIFI_BT_SDO_D1_GPIO, 0);
        gpio_direction_output(WIFI_BT_SDO_D0_GPIO, 0);
        gpio_direction_output(WIFI_BT_UART_TX_GPIO, 0);
        gpio_direction_output(WIFI_BT_UART_RX_GPIO, 0);
        gpio_direction_output(WIFI_BT_UART1_CTS_GPIO, 0);
        gpio_direction_output(WIFI_BT_UART1_RTS_GPIO, 0);
    }

    gpio_direction_output(WIFI_BT_PWR_GPIO, 0);  ///< 控制wifi/bt的电源
    gpio_direction_output(WIFI_RESET_IO, 0);  ///< 控制wifi使能
    gpio_direction_output(BT_RESET_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(reset_sdio ? 1000 : 50));
    gpio_direction_output(WIFI_BT_PWR_GPIO, 1);  ///< 控制wifi/bt的电源
    vTaskDelay(pdMS_TO_TICKS(750));
    gpio_direction_output(WIFI_RESET_IO, 1);
    gpio_direction_output(BT_RESET_IO, 1);

    hcn_log_info("wifi_init_task_proc\n");
    pinctrl_set_group(PGRP_SDMMC0);
    mmc_init();

    if (reset_sdio) {
        reset_rescan();
    }

    if (carlink_wifi_init() == -1) {
        reset_sdio = true;
        goto reset_wifi;
    }

    extern void rtlk_wlan_set_wifi_log(uint8_t enable);
    rtlk_wlan_set_wifi_log(0); ///< 关闭或打开wifi打印

    //enable_btco_log();		///< 开启wifi btco

    set_carlink_display_info(0, 0, HCN_LCD_EC_WIDTH, HCN_LCD_EC_HEIGHT);
    set_carlink_video_info(HCN_LCD_EC_WIDTH, HCN_LCD_EC_HEIGHT, 30);
    carlink_ec_init(0, NULL);

    vTaskDelay(pdMS_TO_TICKS(200)); 
    hcn_log_info("wifi init ok!\n");

    vTaskDelete(NULL);

}

int hcn_wifi_init(void) {
    if (xTaskCreate(wifi_init_thread, "wifi_init", 2048, NULL,
                    configMAX_PRIORITIES / 3, &wifi_init_task) != pdPASS) {
        hcn_log_error("Wifi init thead failed!\n");
        return -1;
    }

    hcn_log_info("hcn wifi init ok!\n");

    return 0;
}

#endif