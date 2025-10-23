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

void hcn_hex_config_data_print(char const *function, char *prefix, uint8_t *data,
                              uint8_t length) {
#define HCN_HEX_CONFIG_OUTPUT_LEN 260
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
