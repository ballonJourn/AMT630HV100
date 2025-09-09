/**
*
* @file hcn_read_eeprom.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/02 09:42
* @author och
*
*/
#ifndef __HCN_READ_EEPROM_H__
#define __HCN_READ_EEPROM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define E2PROM_WP_GPIO      (4)
#define E2PROM_CAPACITY_BYTE  (1024)  ///< e2prom容量是1024字节
#define E2PROM_PAGE_SIZE      (16)   ///< e2prom页大小为16字节

#define T24C08A_DEV_ADDR    (0x50)

int e2prom_test(uint16_t addr,int size,uint16_t start_data);

int e2prom_byte_read (uint16_t addr, uint8_t *buf, uint8_t length);
int e2prom_byte_write (uint16_t addr,uint8_t *buf, uint8_t length);

int e2prom_write_data(uint16_t addr, uint8_t *buf, int size);
int e2prom_read_data(uint16_t addr, uint8_t *buf, int size);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_READ_EEPROM_H__