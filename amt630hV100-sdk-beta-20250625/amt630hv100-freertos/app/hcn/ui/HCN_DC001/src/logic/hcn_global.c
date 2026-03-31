#include "hcn_global.h"
#include "view/set_view/set_view_interface.h"
#include "view/home_view/home_view_interface.h"
#include "proxy/vehicle_argument.h"
#include "proxy/vehicle_data.h"
#include "proxy/vehicle_mile.h"
#include "hcn_logic.h"

extern ret_t assets_set_global_theme(const char* name); 

extern ret_t stb_load_image(int32_t subtype, const uint8_t* buff, uint32_t buff_size, bitmap_t* image,
                            bool_t require_bgra, bool_t enable_bgr565, bool_t enable_rgb565         ) ;

static const char* country_language_str[LANGUAGE_OPTION_MAX] = {
    "zh_CN" , "en_US" 
};

ret_t global_refresh_unit(uint8_t unit)
{
    //速度
    int32_t value ;
    value = vehicle_get_data_speed() ;
    if (MPH == vehicle_get_param_unit())
            value *= KM_CONVERT_MILE ; 
    home_refresh_speed(value) ;
    home_refresh_unit(unit);

    // 里程
    global_refresh_mileage();
    home_refresh_mileage_unit(unit) ;

    //电量
    // home_refresh_electrical(uint32_t mileage) 
    home_refresh_electrical_unit(unit);

    //info窗口 参数需改
    // uint32_t u32_vlaue =  vehicle_get_mile_once();
    // if (MPH == vehicle_get_param_unit())
    //         u32_vlaue *= KM_CONVERT_MILE ; 
    // home_refresh_info_distance(u32_vlaue);


    return RET_OK ;
}

ret_t global_refresh_language(uint8_t value) 
{
    if (value > LANGUAGE_OPTION_MAX)
        return RET_FAIL ;
    
    char country [3] = {0};
    char language[3] = {0};
    strncpy(language, country_language_str[value] , 2);
    strncpy(country , country_language_str[value] + 3, 2);

    locale_info_change(locale_info(), language, country) ;

    return RET_OK ;
}

ret_t global_refresh_display(uint8_t value) 
{
    if (DIAPLAY_AUTO_OPTION == value)
        return RET_OK ;
    
    uint8_t temp = 0 ;
    assets_manager_t *am = assets_manager();
    if (tk_str_eq(am->theme, "default"))
        temp = DIAPLAY_NIGHT_OPTION ;
    else if (tk_str_eq(am->theme, "day"))
        temp = DIAPLAY_DAY_OPTION ;
    else {};

    if (value != temp)
        assets_set_global_theme(value == DIAPLAY_NIGHT_OPTION ? "default" : "day");
    else
        printf("The current theme and settings are the same");
    
    return RET_OK;
}

ret_t global_data_init(const timer_info_t *info)
{
    // printf("===================");
    (void)info ;
    
    static bool is_init_usr_param  = false ;
    static bool is_init_mile_param = false ;
    
    if (!is_init_usr_param && (true == vehicle_get_param_recovery()))
    {
        // 设置语言
        uint8_t value  ;
        value = vehicle_get_param_language();
        global_refresh_language(value) ;
        
        // 设置单位 // 设置里程程息 、 剩余里程
        value = vehicle_get_param_unit() ;
        global_refresh_unit(value);
        
        //显示主题 0：白天 1:黑夜 2:自动
        value = vehicle_get_param_display();
        if (0 == value)
            global_refresh_display(DIAPLAY_DAY_OPTION);
        else if(1 == value)
            global_refresh_display(DIAPLAY_NIGHT_OPTION);
        else 
            global_refresh_display(vehicle_get_data_current_display()) ;
        
        // 档位 
        value =  vehicle_get_data_gear() ;
        home_refresh_gear(value) ;
        
        // // 驾驶模式
        value = vehicle_get_data_drv_mode() ;
        home_refresh_drv_mode(value) ;
        
        refresh_ver(veicle_get_data_version());

        is_init_usr_param = true ; 

        printf("vehicle_get_param_recovery successed %s : %d\n" ,__FUNCTION__ , __LINE__);
    }


    if (!is_init_mile_param  && (true == vehicle_get_mile_recovery()) )
    {
        // 里程
        global_refresh_mileage();

        is_init_mile_param = true ;

        printf("vehicle_get_mile_recovery successed %s : %d\n" ,__FUNCTION__ , __LINE__);
    }


    if (is_init_usr_param && is_init_mile_param)
    {
        // info->ctx = 0 ;  //(timerID)
        // clean_timer_ID(REFRESH_TIMER_100_MS);
        return RET_REMOVE ;
    }
    
    return RET_REPEAT ;

}


void global_refresh_mileage()
{
    // 里程
    uint32_t u32_odo   = vehicle_get_mile_odo()  ;
    uint32_t u32_tripA = vehicle_get_mile_tripA();
    uint32_t u32_tripB = vehicle_get_mile_tripB();
    uint32_t u32_once  = vehicle_get_mile_once() ;
    if (MPH == vehicle_get_param_unit())
    {
        u32_odo   *= KM_CONVERT_MILE ; 
        u32_tripA *= KM_CONVERT_MILE ;
        u32_tripB *= KM_CONVERT_MILE ;
        u32_once  *= KM_CONVERT_MILE ;
    }
    home_refresh_odo ((double)u32_odo) ;
    home_refresh_trip((double)u32_tripA) ;
    home_refresh_info_distance(u32_once) ;

    return ;
}


// 判断一个字节是否是 UTF-8 编码的首字节
static int is_utf8_head(char c) 
{
    return ((c & 0xE0) == 0xC0) || ((c & 0xF0) == 0xE0) || ((c & 0xF8) == 0xF0);
}

// 返回 UTF-8 编码的一个字符所占用的字节数
static int utf8_char_len(char c)
{
    if ((c & 0xE0) == 0xC0) 
    {
        return 2;
    } 
    else if ((c & 0xF0) == 0xE0) 
    {
        return 3;
    } 
    else if ((c & 0xF8) == 0xF0) 
    {
        return 4;
    } 
    else 
    {
        return 1;
    }
}

// 截取一个 UTF-8 编码字符串的前限定个字节，保证不截断任何一个完整字符
bool truncate_utf8_string(char* str , int intercept_length) 
{
    int len = strlen(str);
    int i;
    int byte_count = 0;

    if((len > intercept_length) 
        && (len < APP_MESSAGE_CONTENT_INFO_LEN))
    {       
        for (i = 0; i < len && byte_count < intercept_length; i += utf8_char_len(str[i])) 
        {
            // 如果下一个字符会使得字节数超过限定，则直接退出循环
            if ((is_utf8_head(str[i]))
                && (byte_count + utf8_char_len(str[i]) > intercept_length))
            {
                break;  
            }
            byte_count += utf8_char_len(str[i]);
        }
        // 确保截取后的字符串以 '\0' 结尾
        str[byte_count ]    = '.';  
        str[byte_count + 1] = '.';  
        str[byte_count + 2] = '.';  
        str[byte_count + 3] = '\0';  
        return true;
    }
    else if(len <= intercept_length)
    {
        return true;
    }

    return false;
}

ret_t gloabl_load_image(uint8_t *buff ,  uint32_t length)
{
    if (NULL == buff || 0 == length)
        return RET_FAIL ;
    
    bitmap_t tmps = {0};

    ret_t ret = RET_FAIL;
    const asset_info_t* asset = assets_manager_ref(assets_manager(), ASSET_TYPE_IMAGE, BLUETOOTH_MUSIC_IMAGE );
    if (asset != NULL) 
    {
        printf("asset_info_t ref successed! size: %d ", asset->size);
        if (RET_OK == image_manager_get_bitmap(image_manager(), BLUETOOTH_MUSIC_IMAGE, &tmps)) 
        {
            image_manager_unload_bitmap(image_manager(), &tmps);
            printf(" asset_info_t release success \n") ;
        }
        assets_manager_clear_cache_ex(assets_manager() ,ASSET_TYPE_IMAGE , BLUETOOTH_MUSIC_IMAGE );
        assets_manager_unref(assets_manager(), asset);

    } 
    else 
    {
        printf("asset_info_t not exist ,need to preload size:%d\n", length);
    }
    ret = assets_manager_add_data(assets_manager(), BLUETOOTH_MUSIC_IMAGE , ASSET_TYPE_IMAGE, ASSET_TYPE_IMAGE_PNG, (uint8_t*)buff, length) ;

    if (ret == RET_OK)
    {
        printf("assets_manager_add_data successed \n") ;
    }

    return ret ;
    

}
