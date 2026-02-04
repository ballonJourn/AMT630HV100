/**
*
* @file hcn_utils.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/08 17:58
* @author och
*
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "utils/hcn_utils.h"
#include "log/hcn_log.h"

static char build_date_time[32] = {0};

void hcn_hex_config_data_print(char const *function, char *prefix, uint8_t *data,
                              uint16_t length) {
#define HCN_HEX_CONFIG_OUTPUT_LEN 300
    char buffer[HCN_HEX_CONFIG_OUTPUT_LEN];
    buffer[HCN_HEX_CONFIG_OUTPUT_LEN - 1] = 0;

    if ((strlen(prefix) + strlen(function)) > (HCN_HEX_CONFIG_OUTPUT_LEN - 2)) {
        printf("hcn_hex_config_data_print: function + prefix is too long!\n");
        return;
    } else {
        sprintf(buffer, "%s", function);
        sprintf(buffer + strlen(buffer), "%s", prefix);
    }

    for (int i = 0; i < length; i++) {
        int remain_len = HCN_HEX_CONFIG_OUTPUT_LEN - strlen(buffer) - 2;
        if (remain_len < 3) {
            break;
        }
        sprintf(buffer + strlen(buffer), "%02X ", data[i]);
    }
    
    sprintf(buffer + strlen(buffer), "\n");
    printf("%s", buffer);
}

void sting_2_lower(char *str)
{
    if (str == NULL) {
        return;   
    }

    while (*str) {  
        *str = tolower((unsigned char)*str);  
        str++;  
    }  
}

void sting_2_upper(char *str)
{
    if (str == NULL) {
        return;   
    }

    while (*str) {  
        *str = toupper((unsigned char)*str);  
        str++;  
    }  
}

int bcd_2_decimal(int bcd) {
    return (bcd - (bcd >>4) * 6);
}

int decimal_2_bcd( int decimal) {
    return (decimal + (decimal/10) * 6);
}

char *substring(char *dst, char *src, int start, int len) {
    char *p = dst;
    char *q = src;
    
    int length = strlen(src);
    if (start >= length || start < 0) {
        return NULL;
    }

    if (len > length) len = length - start;
    q += start;

    while (len--) {
        *(p++) = *(q++);
    }
    *(p++) = '\0';

    return dst;
}

uint16_t string_split(char *dest_str, char *token, 
                char out_str[][TEXT_PARAM_LEN], 
                int out_str_len) {
    char *result = NULL;
    result = strtok(dest_str, token);

    uint16_t i = 0;
    while (result != NULL && i < out_str_len) {
        uint16_t result_len = (uint16_t)strlen(result);
        strncpy(out_str[i], result, result_len < TEXT_PARAM_LEN ? \
                result_len : TEXT_PARAM_LEN - 1);
        result = strtok(NULL, token);
        i++;
    }
    
    return i;
}

const char *get_build_date_time(void) {
	return build_date_time;
}

void set_build_date_time(void) {
    const char *pMonth[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", \
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    ///< 取编译日期
    const char Date[12] = __DATE__;  

#ifdef GET_BUILD_TIME_ENABLE
	char Time[10] = __TIME__;
#endif

    uint8_t i;
	int month = 1;
    for (i = 0; i < 12; i++) {
		if (memcmp(Date, pMonth[i], 3) == 0)
			month = i + 1;
	}

    ///<Date[9]为2位年份，Date[7]为完整年份
    int year = (uint16_t)atoi(Date + 7); 
    int day = (uint8_t)atoi(Date + 4);

#ifdef GET_BUILD_TIME_ENABLE
	char *hour = strtok(Time, ":");
    char *min = strtok(NULL, ":");
    char *sec = strtok(NULL, ":");
    snprintf(build_date_time, sizeof(build_date_time), "%04d%02d%02d_%s%s%s", \
         year, month, day, hour, min, sec);
#endif
 	snprintf(build_date_time, sizeof(build_date_time), \
        "%04d%02d%02d", year, month, day);

	printf("Soc build version time: %s\r\n", build_date_time);
}

int is_valid_date(int year, int mon, int day) {
    if (year < 2000 || year > 2099) {
        return 0;
    } 
    
    if (mon < 1 || mon > 12) {
        return 0;
    } 
    
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 400 == 0) || (year % 4 == 0 && year % 100 != 0)) {
        days_in_month[1] = 29; ///< 闰年2月29天
    } else {
        days_in_month[1] = 28; ///< 平年2月28天
    }
    
    ///< 检查日期
    if (day < 1 || day > days_in_month[mon - 1]) {
        return 0;
    }

    return 1;
}

///< MCU-DC001-GD_24.11.13V0 或者 MCU-DC001-GD_24.11.13
int parse_mcu_ver_format_by_strtok(const char *ver, parse_info_t *info) {
    if (ver == NULL || info == NULL) {
        return -1;
    }

     const char *pstr = ver;

    ///< 查找下划线
    while (*pstr && *pstr != '_') {
        pstr++;
    }

    if (!*pstr) {
        return 0;
    }

    ///< 跳过下划线
    pstr++; 

    ///< 解析日期时间
    if (!isdigit(*pstr) || !isdigit(*(pstr + 1)) || *(pstr + 2) != '.') {
        return 0;
    }

    info->year = 2000 + ((pstr[0] - '0') * 10 + (pstr[1] - '0'));
    pstr += 3;

    if (!isdigit(*pstr) || !isdigit(*(pstr + 1)) || *(pstr + 2) != '.') {
        return 0;
    }

    info->month = ((pstr[0] - '0') * 10 + (pstr[1] - '0'));
    pstr += 3;

    if (!isdigit(*pstr) || !isdigit(*(pstr + 1)) || *(pstr + 2) != '.') {
        return 0;
    }

    info->day = ((pstr[0] - '0') * 10 + (pstr[1] - '0'));
    pstr += 2;
 
    int version_size = sizeof(info->version);
    ///< 解析版本如有
    if (info->version && (version_size > 0)) {
        if (*pstr == 'V' || *pstr == 'v') {
            pstr++;
            int i = 0;
            while (*pstr && i < version_size -1) {
                info->version[i++] = *pstr++;
            }
            info->version[i] = '\0';
        } else {
            info->version[0] = '\0';
        }
    }
    
    return 1;
}