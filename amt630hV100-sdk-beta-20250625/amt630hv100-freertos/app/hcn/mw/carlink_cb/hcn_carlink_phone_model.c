/**
*
* @file hcn_carlink_phone_model.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/20 16:41
* @author och
*
*/

#include <stdio.h>
#include <string.h>
#include "carlink_cb/hcn_carlink_phone_model.h"
#include "log/hcn_log.h"
#include "cJSON.h"

#define IPHONE_MODEL_COUNT  (59)

/**
 * @brief iPhone型号对应字符串数组，需要IOS16.0及以上支持
 */
const char *iphone_model_string[IPHONE_MODEL_COUNT] = {
    "iPhone3,1",
    "iPhone3,2",
    "iPhone3,3",
    "iPhone4,1",
    "iPhone5,1",
    "iPhone5,2",
    "iPhone5,3",    
    "iPhone5,4",    
    "iPhone6,1",
    "iPhone6,2",
    "iPhone7,1",
    "iPhone7,2",
    "iPhone8,1",    
    "iPhone8,2",
    "iPhone8,4",
    "iPhone9,1",    
    "iPhone9,2",
    "iPhone9,3",
    "iPhone9,4",    
    "iPhone10,1",    
    "iPhone10,2",
    "iPhone10,4",    
    "iPhone10,5",    
    "iPhone10,3",    
    "iPhone10,6",    
    "iPhone11,2",    
    "iPhone11,4",    
    "iPhone11,6",    
    "iPhone11,8",    
    "iPhone12,1",    
    "iPhone12,3",    
    "iPhone12,5",    
    "iPhone12,8",    
    "iPhone13,1",    
    "iPhone13,2",    
    "iPhone13,3",    
    "iPhone13,4",    
    "iPhone14,4",    
    "iPhone14,5",    
    "iPhone14,2",    
    "iPhone14,3",
    "iPhone14,6",    
    "iPhone14,7",    
    "iPhone14,8",    
    "iPhone15,2",    
    "iPhone15,3", 
    "iPhone15,4",    
    "iPhone15,5",
    "iPhone16,1",
    "iPhone16,2",
    "iPhone17,1",
    "iPhone17,2",
    "iPhone17,3",
    "iPhone17,4",
    "iPhone17,5",
    "iPhone18,1",
    "iPhone18,2",
    "iPhone18,3",
    "iPhone18,4"
};

/**
 * @brief iPhone型号对应名称数组，需要IOS16.0及以上支持
 */
const char *iphone_model[IPHONE_MODEL_COUNT] = {
    "iPhone 4",
    "iPhone 4",   
    "iPhone 4",     
    "iPhone 4S",     
    "iPhone 5",     
    "iPhone 5",     
    "iPhone 5c", 
    "iPhone 5c",    
    "iPhone 5s",    
    "iPhone 5s",    
    "iPhone 6 Plus",    
    "iPhone 6",    
    "iPhone 6s",    
    "iPhone 6s Plus",    
    "iPhone SE",    
    "iPhone 7",    
    "iPhone 7 Plus",    
    "iPhone 7",    
    "iPhone 7 Plus",    
    "iPhone 8",    
    "iPhone 8 Plus",    
    "iPhone 8",    
    "iPhone 8 Plus",    
    "iPhone X",    
    "iPhone X",    
    "iPhone XS",    
    "iPhone XS Max",    
    "iPhone XS Max",    
    "iPhone XR",    
    "iPhone 11",    
    "iPhone 11 Pro",    
    "iPhone 11 Pro Max",    
    "iPhone SE2",    
    "iPhone 12 mini",
    "iPhone 12",    
    "iPhone 12 Pro",    
    "iPhone 12 Pro Max",    
    "iPhone 13 mini",    
    "iPhone 13",    
    "iPhone 13 Pro",    
    "iPhone 13 Pro Max",    
    "iPhone 13 SE 3",    
    "iPhone 14",    
    "iPhone 14 Plus",    
    "iPhone 14 Pro",    
    "iPhone 14 Pro Max",
    "iPhone 15", 
    "iPhone 15 Plus",
    "iPhone 15 Pro",
    "iPhone 15 Pro Max",
    "iPhone 16 Pro",
    "iPhone 16 Pro Max",
    "iPhone 16", 
    "iPhone 16 Plus",
    "iPhone 16e",
    "iPhone 17 Pro",
    "iPhone 17 Pro Max",
    "iPhone 17",
    "iPhone 17 Air"
};

static hcnPhoneInfo phone_msg;

const hcnPhoneInfo* get_phone_model_info(void) {
    return &phone_msg;
}

static void compare_phone_model(const char *phone_info) {
    if (phone_info == NULL) {
        hcn_log_error("compare_phone_model input is NULL\n");
        return;
    }

    const char undefine_iphone_mode[64] = {"iPhone 17 Air later models"};

    for (int i = 0; i < IPHONE_MODEL_COUNT; i++) {
        if (strncmp(iphone_model_string[i], phone_info, 32) == 0) {
            snprintf(phone_msg.phoneModels, sizeof(phone_msg.phoneModels), 
                    "%s", iphone_model[i]);
            return;
        }
    }

     if (strncmp("iPhone", phone_info, 6) == 0) {
        snprintf(phone_msg.phoneModels, sizeof(phone_msg.phoneModels), 
                "%s", undefine_iphone_mode);
    }
}

void parse_phone_model_info(const char *data) {
    if (data) {
        cJSON *json = NULL;
        cJSON *dev_name = NULL;
        cJSON *phone_model = NULL;
        cJSON *phone_os = NULL;
            
        json = cJSON_Parse(data);
        if (NULL == json) {
            hcn_log_error("onPhoneAppInfo cJSON_Parse error:%s\n",cJSON_GetErrorPtr());
            return;
        }
        
        dev_name  = cJSON_GetObjectItem(json, "bluetoothName");
        phone_model = cJSON_GetObjectItem(json, "phoneModel");
        phone_os = cJSON_GetObjectItem(json, "phoneOs");
        
        int phoneType = 0; 

        memset(&phone_msg, 0, sizeof(hcnPhoneInfo));
        if (strstr(phone_os->valuestring, "iOS") != NULL) {
            phoneType = 1; ///<ios
        } else {
            phoneType = 0; ///<android
        }

        if (phoneType == 1) {
            compare_phone_model(phone_model->valuestring);
        } else {
            snprintf(phone_msg.phoneModels,sizeof(phone_msg.phoneModels), 
                    "%s", phone_model->valuestring);             
        }

        snprintf(phone_msg.phoneDevName, sizeof(phone_msg.phoneDevName), "%s", 
                dev_name->valuestring); 
        phone_msg.phoneType = phoneType;

        cJSON_Delete(json);   
        
        hcn_log_info("\r\nphoneType:%d, phoneModels:%s, phoneDevName:%s\r\n", 
                phone_msg.phoneType, phone_msg.phoneModels, phone_msg.phoneDevName);
    }        
}         
