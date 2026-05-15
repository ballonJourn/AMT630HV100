#ifndef UPDATE_VIEW_H__
#define UPDATE_VIEW_H__

#include "awtk.h"
#include "ota_manage/hcn_ota.h"

enum update_com{
    UPDATE_BAR   ,
    UPDATE_STATE ,
    UPDATE_TYPE  ,
    UPDATE_ERROR ,
    UPDATE_NUM_MAX ,
};

ret_t update_view_init(widget_t* parent);

ret_t update_refresh_bar(int progress);

ret_t update_refresh_state(update_state_e state);

ret_t update_refresh_type(update_type_e type);

ret_t update_refresh_error(update_error_e error);

#endif