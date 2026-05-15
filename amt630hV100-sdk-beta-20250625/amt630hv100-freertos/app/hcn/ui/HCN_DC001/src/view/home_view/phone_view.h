#ifndef HOME_PHONE_VIEW_H
#define HOME_PHONE_VIEW_H
#include "common.h"

enum home_phone_com{
    PHONE_STATE_IMG,
    PHONE_TIPS     ,
    PHONE_NUMBER   ,

    PHONE_NUM_MAX  ,
};

typedef enum {
    PHONE_NO_CONNECT  ,
    PHONE_CONNECT     ,

    PHONE_VIEW_MAX ,
}home_phone_view_e ;

typedef enum {
    CALL_INCOMMING ,  
    CALL_OUTGOING  ,  
    CALL_CALLING   ,

    CALL_STATE_MAX ,
}call_state_e ;


ret_t home_phone_view_init(widget_t* parent);

ret_t home_refresh_phone_state(call_state_e state);

ret_t home_refresh_phone_tips(call_state_e state);

ret_t home_refresh_phone_numName(const char *value);

//手机通话页面
ret_t home_refresh_phone_view(home_phone_view_e view)  ;
#endif