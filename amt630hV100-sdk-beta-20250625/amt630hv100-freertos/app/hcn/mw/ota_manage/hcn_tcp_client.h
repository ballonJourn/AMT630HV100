/**
*
* @file hcn_tcp_client.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/12/16 09:13
* @author och
*
*/
#ifndef __HCN_TCP_CLIENT_H__
#define __HCN_TCP_CLIENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "ota_manage/hcn_ota_parse.h"

void start_tcp_client(void);
void stop_tcp_client(void);
uint32_t ota_calc_crc32(uint8_t *data, uint32_t len);
tcp_ota_state get_ota_state(void);
void set_ota_state(tcp_ota_state state);
int send_file_recv_state_ack(uint16_t file_len);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_TCP_CLIENT_H__