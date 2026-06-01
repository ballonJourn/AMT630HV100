#include "hcn_cp_keyevent.h"
#include "hcn_key_common.h"
#include "vehicle_param/vehicle_param.h"
#include "FreeRTOS.h"
#include "task.h"

extern void carlink_send_key_event(uint8_t key, bool pressed);
void send_keyevent_to_cp(uint8_t keyevent) {
    switch (keyevent) {
        case MODE_KEY_SHORT_PR:
            carlink_send_key_event(17, true);
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
            carlink_send_key_event(18, true);
            break;

        case UP_KEY_LONG_PR:
            break;

        default:
            break;
    }   
}