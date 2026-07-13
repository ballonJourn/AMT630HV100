/**
 * @file hcn_debug_cli.c
 * @brief 雷达调试命令 - 注册到 FreeRTOS+CLI 框架
 *
 * 使用方式: 在串口终端输入命令后回车
 *   radar speed 30     → 模拟30km/h车速发给雷达
 *   radar speed -1     → 取消模拟，恢复真实车速
 *   radar bsd 0        → 将BSD启动速度设为0km/h
 *   radar target       → 切换到目标数据模式
 *   radar warn         → 切回预警数据模式
 *   radar info         → 打印雷达当前预警数据
 *
 * @date 2026/06/05
 */

#include <FreeRTOS.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "debug_cli/hcn_debug_cli.h"
#include "mmwave_radar/hcn_mmwave_radar.h"
#include "FreeRTOS_CLI.h"

#ifdef HCN_MMWAVE_RADAR_ENABLE

/**
 * @brief 参数匹配辅助宏
 *        FreeRTOS_CLIGetParameter 返回的是原始字符串中的指针+长度，
 *        不以 '\0' 结尾，所以必须同时比较长度和内容
 */
#define PARAM_MATCH(param, paramLen, keyword) \
    ((paramLen) == (BaseType_t)strlen(keyword) && \
     strncmp((param), (keyword), (paramLen)) == 0)

static BaseType_t prvRadarCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                  const char *pcCommandString)
{
    BaseType_t xP1Len = 0, xP2Len = 0;
    const char *p1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xP1Len);
    const char *p2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xP2Len);

    pcWriteBuffer[0] = '\0';

    if (p1 == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen,
            "\r\nUsage:\r\n"
            "  radar info       Show radar status\r\n"
            "  radar speed <N>  Fake speed (km/h), -1=off\r\n"
            "  radar bsd <N>    Set BSD start speed\r\n"
            "  radar target     Switch to target data\r\n"
            "  radar warn       Switch to warning data\r\n");
        return pdFALSE;
    }

    ///< radar info
    if (PARAM_MATCH(p1, xP1Len, "info")) {
        const radar_warn_data_t *w = mmwave_radar_get_warn_data();
        const radar_target_data_t *t = mmwave_radar_get_target_data();
        bool online = mmwave_radar_is_online();
        int n = 0;
        n += snprintf(pcWriteBuffer + n, xWriteBufferLen - n,
            "\r\n--- Radar Info ---\r\n"
            "  Online:     %s\r\n"
            "  warn_flags: 0x%02X",
            online ? "YES" : "NO", w->warn_flags);
        if (w->warn_flags & RADAR_WARN_BSD_LEFT)  n += snprintf(pcWriteBuffer + n, xWriteBufferLen - n, " [BSD_L]");
        if (w->warn_flags & RADAR_WARN_BSD_RIGHT) n += snprintf(pcWriteBuffer + n, xWriteBufferLen - n, " [BSD_R]");
        if (w->warn_flags & RADAR_WARN_CVW_LEFT)  n += snprintf(pcWriteBuffer + n, xWriteBufferLen - n, " [CVW_L]");
        if (w->warn_flags & RADAR_WARN_CVW_RIGHT) n += snprintf(pcWriteBuffer + n, xWriteBufferLen - n, " [CVW_R]");
        if (w->warn_flags & RADAR_WARN_RCW)       n += snprintf(pcWriteBuffer + n, xWriteBufferLen - n, " [RCW]");
        n += snprintf(pcWriteBuffer + n, xWriteBufferLen - n,
            "\r\n"
            "  speed:      %d km/h\r\n"
            "  bsd_start:  %d km/h\r\n"
            "  L dist/spd: %dm / %dkm/h\r\n"
            "  R dist/spd: %dm / %dkm/h\r\n"
            "  B dist/spd: %dm / %dkm/h\r\n"
            "  --- Targets: %d ---\r\n",
            w->current_speed, w->bsd_start_speed,
            w->left_distance, w->left_speed,
            w->right_distance, w->right_speed,
            w->rear_distance, w->rear_speed,
            t->target_num);
        for (int i = 0; i < t->target_num && i < 3; i++) {
            n += snprintf(pcWriteBuffer + n, xWriteBufferLen - n,
                "  T%d: x=%.1fm y=%.1fm v=%dm/s\r\n",
                i,
                t->targets[i].x * 0.1f,
                t->targets[i].y * 0.1f,
                t->targets[i].speed);
        }
        return pdFALSE;
    }

    ///< radar speed <N>
    if (PARAM_MATCH(p1, xP1Len, "speed")) {
        if (p2 == NULL) {
            snprintf(pcWriteBuffer, xWriteBufferLen,
                "\r\nUsage: radar speed <N>  (-1=off, 0~255=km/h)\r\n");
            return pdFALSE;
        }
        int val = atoi(p2);
        mmwave_radar_dbg_set_speed(val);
        snprintf(pcWriteBuffer, xWriteBufferLen,
            "\r\nRadar: speed override %s%s\r\n",
            val < 0 ? "OFF" : "= ",
            val < 0 ? "" : p2);
        return pdFALSE;
    }

    ///< radar bsd <N>
    if (PARAM_MATCH(p1, xP1Len, "bsd")) {
        if (p2 == NULL) {
            snprintf(pcWriteBuffer, xWriteBufferLen,
                "\r\nUsage: radar bsd <N>  (0~255 km/h)\r\n");
            return pdFALSE;
        }
        int val = atoi(p2);
        mmwave_radar_dbg_set_bsd_speed(val);
        snprintf(pcWriteBuffer, xWriteBufferLen,
            "\r\nRadar: BSD start speed = %d km/h\r\n", val);
        return pdFALSE;
    }

    ///< radar target
    if (PARAM_MATCH(p1, xP1Len, "target")) {
        mmwave_radar_set_data_type(0);
        snprintf(pcWriteBuffer, xWriteBufferLen,
            "\r\nRadar: switched to TARGET data mode\r\n"
            "Watch log for 'targets=N' lines\r\n");
        return pdFALSE;
    }

    ///< radar warn
    if (PARAM_MATCH(p1, xP1Len, "warn")) {
        mmwave_radar_set_data_type(1);
        snprintf(pcWriteBuffer, xWriteBufferLen,
            "\r\nRadar: switched to WARNING data mode\r\n");
        return pdFALSE;
    }

    snprintf(pcWriteBuffer, xWriteBufferLen,
        "\r\nUnknown '%.*s'. Try: info/speed/bsd/target/warn\r\n",
        (int)xP1Len, p1);
    return pdFALSE;
}

static const CLI_Command_Definition_t xRadarCommand =
{
    "radar",
    "\r\nradar <sub-cmd> [value]:\r\n"
    "  radar info       Show radar status\r\n"
    "  radar speed <N>  Fake speed (km/h), -1=off\r\n"
    "  radar bsd <N>    Set BSD start speed\r\n"
    "  radar target     Switch to target data\r\n"
    "  radar warn       Switch to warning data\r\n",
    prvRadarCommand,
    -1
};

int debug_cli_init(void)
{
    FreeRTOS_CLIRegisterCommand(&xRadarCommand);
    printf("\r\n[CLI] Radar debug commands registered\r\n");
    return 0;
}

#endif /* HCN_MMWAVE_RADAR_ENABLE */