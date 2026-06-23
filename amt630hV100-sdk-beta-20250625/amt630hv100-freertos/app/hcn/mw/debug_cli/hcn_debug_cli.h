/**
 * @file hcn_debug_cli.h
 * @brief 雷达调试命令 - 注册到 FreeRTOS+CLI 框架
 * @date 2026/06/05
 *
 * 支持命令 (在串口终端输入):
 *   radar speed <N>     设置模拟车速 (如: radar speed 30), -1取消
 *   radar bsd <N>       设置BSD启动速度 (如: radar bsd 0)
 *   radar info          打印当前雷达状态
 */
#ifndef __HCN_DEBUG_CLI_H__
#define __HCN_DEBUG_CLI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "config/hcn_config.h"

#ifdef HCN_MMWAVE_RADAR_ENABLE

/**
 * @brief 注册雷达调试命令到 FreeRTOS+CLI
 * @return 0:成功
 */
int debug_cli_init(void);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __HCN_DEBUG_CLI_H__ */