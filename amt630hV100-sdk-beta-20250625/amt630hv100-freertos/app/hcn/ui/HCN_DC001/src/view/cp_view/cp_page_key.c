#include "cp_page_key.h"
#include "key_module/hcn_key_common.h"
#include "vehicle_param/vehicle_param.h"
#include "../view_manager.h"
#include "common/navigator.h"
#include "FreeRTOS.h"
#include "task.h"


extern void carlink_send_key_event(uint8_t key, bool pressed);
static void send_keyevent_to_cp(uint8_t keyevent) {
    switch (keyevent) {
        case MODE_KEY_SHORT_PR:
            carlink_send_key_event(18, true);
            break;

        case MODE_KEY_LONG_PR:
            //carlink_send_key_event(27, false);
            break;

        case SET_KEY_SHORT_PR:///< 确定按键
            carlink_send_key_event(27, true);
            vTaskDelay(pdMS_TO_TICKS(50));
            carlink_send_key_event(27, false);
            break;

        case SET_KEY_LONG_PR: ///< 回到home界面
            carlink_send_key_event(28, true);
            vTaskDelay(pdMS_TO_TICKS(50));
            carlink_send_key_event(28, false);
            break;

        case BACK_KEY_SHORT_PR:
            carlink_send_key_event(19, true);
            vTaskDelay(pdMS_TO_TICKS(50));
            carlink_send_key_event(19, false);
            break;

        case BACK_KEY_LONG_PR:
            carlink_send_key_event(31, true);
            vTaskDelay(pdMS_TO_TICKS(50));
            carlink_send_key_event(31, false);
            vehicle_set_data(VEH_CARLINK_CP_STATUS, 2); // 设置为断开状态，防止cp界面被唤起
            break;
  
        case UP_KEY_SHORT_PR:
            carlink_send_key_event(17, true);
            break;

        case UP_KEY_LONG_PR:
            break;

        default:
            break;
    }   
}

/**
 * @brief 判断CarPlay是否处于已连接且手机端已打开状态
 */
static bool cp_is_active(void)
{
    return (vehicle_get_data(VEH_CARLINK_CP_STATUS) == 1)
        && (vehicle_get_data(VEH_CARLINK_CP_PHONE_DEV_STATUS) == 1);
}

void cp_page_deal_key_down ()
{
    printf("line:%d==key cp status:%d %d\r\n",__LINE__, vehicle_get_data(VEH_CARLINK_CP_STATUS), 
    vehicle_get_data(VEH_CARLINK_CP_PHONE_DEV_STATUS));
    if (cp_is_active()) {
        printf("send cp down key event\r\n");
        send_keyevent_to_cp(MODE_KEY_SHORT_PR);
    }
    return ;
}

void cp_page_deal_key_up ()
{
    printf("line:%d==key cp status:%d %d\r\n",__LINE__, vehicle_get_data(VEH_CARLINK_CP_STATUS), 
    vehicle_get_data(VEH_CARLINK_CP_PHONE_DEV_STATUS));
    if (cp_is_active()) {
        printf("send cp up key event\r\n");
        send_keyevent_to_cp(UP_KEY_SHORT_PR);
    }
    return ;
}

void cp_page_deal_key_set ()
{
    printf("line:%d==key cp status:%d %d\r\n",__LINE__, vehicle_get_data(VEH_CARLINK_CP_STATUS), 
    vehicle_get_data(VEH_CARLINK_CP_PHONE_DEV_STATUS));
    if (cp_is_active()) {
        printf("send cp set key event\r\n");
        send_keyevent_to_cp(SET_KEY_SHORT_PR);
    }

    return ;
}

void cp_page_deal_key_back ()
{
    if (cp_is_active()) {
        printf("send cp back key event\r\n");
        send_keyevent_to_cp(BACK_KEY_SHORT_PR);
    } else {
        navigator_back_to_home();
    }
    return ;
}

void cp_page_deal_key_set_long()
{
    if (cp_is_active()) {
        printf("send cp SET_KEY_LONG_PR key event\r\n");
        send_keyevent_to_cp(SET_KEY_LONG_PR);
    }
}

void cp_page_deal_key_back_long()
{
    if (cp_is_active()) {
        printf("send cp BACK_KEY_LONG_PR key event\r\n");
        send_keyevent_to_cp(BACK_KEY_LONG_PR);
    }
}