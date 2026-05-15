#include "phone_view.h"

const char* home_phone_widget_name[PHONE_NUM_MAX] = {
    "phone_state_img" , "phone_tips" , "phone_num" 
} ;

const char* home_phone_view_name[PHONE_VIEW_MAX] = {
    "bt_no_connected" , "bt_connected"
} ;

const char* call_state_image_str[CALL_STATE_MAX] = {
    "icon_incoming_call" , "icon_outgoing_call" , "icon_answer"
} ;

static widget_t* home_phone_widget[PHONE_NUM_MAX] = { NULL };

static widget_t* home_phone_view[PHONE_VIEW_MAX] = { NULL };

ret_t home_phone_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;
    for (size_t i = 0; i < PHONE_NUM_MAX; i++){
        home_phone_widget[i] = widget_lookup(parent, home_phone_widget_name[i], TRUE);
    }

    for (size_t i = 0; i < PHONE_VIEW_MAX; i++){
        home_phone_view[i] = widget_lookup(parent, home_phone_view_name[i], TRUE);
    }

    return RET_OK ;
}

ret_t home_refresh_phone_state(call_state_e state)
{
    state = tk_min(state , CALL_CALLING);

    if (home_phone_widget[PHONE_STATE_IMG])
    {
       image_set_image(home_phone_widget[PHONE_STATE_IMG], call_state_image_str[state]);
    }

    home_refresh_phone_tips(state) ;

    return RET_OK ;
}

ret_t home_refresh_phone_tips(call_state_e state)
{
    const char *format = NULL ;
    
    switch (state)
    {
    case CALL_INCOMMING:
        format = locale_info_tr(locale_info()  ,"incoming_tips" ) ;
        break;
    case CALL_OUTGOING:
        format = locale_info_tr(locale_info()  ,"outgoing_tips" ) ;
        break;
    case CALL_CALLING:
        format = locale_info_tr(locale_info()  ,"calling_tips" ) ;
        break;
    default:
        break;
    }
    // printf("format = %s\n",format);
    if (home_phone_widget[PHONE_TIPS])
    {
        rich_text_set_text(home_phone_widget[PHONE_TIPS] , format);
    }
    


    return RET_OK ;
}

ret_t home_refresh_phone_numName(const char *value)
{
    if (home_phone_widget[PHONE_NUMBER])
    {
        widget_set_text_utf8(home_phone_widget[PHONE_NUMBER], value);
    }

    return RET_OK ;
}


//手机通话页面
ret_t home_refresh_phone_view(home_phone_view_e view)  
{
    for (size_t i = 0; i < PHONE_VIEW_MAX ; i++)
    {
        if (home_phone_view[i])
        {
            if (view == i)
                widget_set_visible(home_phone_view[i] , true) ;
            else
                widget_set_visible(home_phone_view[i] , false) ;
        }

    }
    
    return RET_OK ; 
}