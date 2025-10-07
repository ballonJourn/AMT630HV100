/**
*
* @file hcn_utils.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/08 17:57
* @author och
*
*/
#ifndef __HCN_UTILS_H__
#define __HCN_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief  打印十六进制配置数据
 * @param function 函数名称
 * @param  prefix 前缀
 * @param data 需要打印的数据
 * @param length 数据长度(不超过260字节，否则会被截断)
 * @return 无
 */
void hcn_hex_config_data_print(char const *function, char *prefix, uint8_t *data,
                              uint8_t length);

/**
 * @brief  字符串转小写
 * @param  str 需要转换的字符串
 * @return 无
 */
void sting_2_lower(char *str);

/**
 * @brief  字符串转大写
 * @param  str 需要转换的字符串
 * @return 无
 */
void sting_2_upper(char *str);

/**
 * @brief  BCD码转十进制数据
 * @param  bcd bcd码数据
 * @return 十进制数据
 */
int bcd_2_decimal(int bcd);

/**
 * @brief  十进制数转BCD码
 * @param  decimal 十进制数据
 * @return BCD码
 */
int decimal_2_bcd( int decimal);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __HCN_UTILS_H__