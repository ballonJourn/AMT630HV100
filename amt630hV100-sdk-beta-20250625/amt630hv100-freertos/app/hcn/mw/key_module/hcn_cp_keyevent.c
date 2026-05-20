#include "hcn_cp_keyevent.h"
#include "hcn_key_common.h"
#include "vehicle_param/vehicle_param.h"

extern void carlink_send_key_event(uint8_t key, bool pressed);
void send_keyevent_to_cp(uint8_t keyevent) {
    switch (keyevent) {
        case MODE_KEY_SHORT_PR:
            //carlink_send_key_event(17, true);
            break;

        case MODE_KEY_LONG_PR:
            //carlink_send_key_event(27, false);
            break;

        case SET_KEY_SHORT_PR:
            //carlink_send_key_event(27, true);
            break;

        case SET_KEY_LONG_PR:
            carlink_send_key_event(20, true);
            break;

        case BACK_KEY_SHORT_PR:
            carlink_send_key_event(19, true);
            vehicle_set_data(VEH_CARLINK_CP_STATUS, 2);
            break;

        case BACK_KEY_LONG_PR:
            break;
  
        case UP_KEY_SHORT_PR:
            //carlink_send_key_event(18, true);
            break;

        case UP_KEY_LONG_PR:
            break;

        default:
            break;
    }   
}