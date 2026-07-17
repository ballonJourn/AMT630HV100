#include "setting_clock.h"
#include <stdio.h>
#include "proxy/vehicle_time.h"
#include "proxy/vehicle_argument.h"

/* ════════════════════════════════════════════════════
 *  原有：时钟数字 widget
 * ════════════════════════════════════════════════════ */
const char* set_clock_widget_name[CLOCK_MAX] = {
    "clock_h_1" , "clock_h_2" , "clock_m_1" , "clock_m_2" 
} ;

char time_value[CLOCK_MAX] = { 0 } ;

static widget_t* set_clock_widget[CLOCK_MAX] = { NULL };

static clock_option_e option = CLOCK_H_1_OPTION ;

/* ════════════════════════════════════════════════════
 *  新增：二级子菜单 widget（调整时间 / 调整制式）
 * ════════════════════════════════════════════════════ */
static const char* clock_sub_widget_name[CLOCK_SUB_NUM_MAX] = {
    "clock_adjust_time" , "clock_adjust_format"
} ;

static widget_t* clock_sub_widget[CLOCK_SUB_NUM_MAX] = { NULL };
static clock_sub_menu_e  sub_menu_sel = CLOCK_SUB_ADJUST_TIME ;

/* 子 view 容器 */
static widget_t* clock_time_view   = NULL ;
static widget_t* clock_format_view = NULL ;

/* ════════════════════════════════════════════════════
 *  新增：制式选择 widget（24H / 12H）
 * ════════════════════════════════════════════════════ */
static const char* clock_fmt_widget_name[CLOCK_FMT_MAX] = {
    "clock_24h_option" , "clock_12h_option"
} ;

static widget_t* clock_fmt_widget[CLOCK_FMT_MAX] = { NULL };
static clock_fmt_option_e fmt_option = CLOCK_FMT_24H ;

/* 二级页面自身的标题和分割线（进入子view时需要隐藏） */
static widget_t* clock_title_label = NULL ;
static widget_t* clock_title_line  = NULL ;

/* ════════════════════════════════════════════════════ */

ret_t set_clock_view_init(widget_t* parent)
{
    if(parent == NULL) return RET_FAIL;

    /* 时钟数字 */
    for (size_t i = 0; i < CLOCK_MAX; i++){
        set_clock_widget[i] = widget_lookup(parent, set_clock_widget_name[i], TRUE);
    }

    /* 二级子菜单 */
    for (size_t i = 0; i < CLOCK_SUB_NUM_MAX; i++){
        clock_sub_widget[i] = widget_lookup(parent, clock_sub_widget_name[i], TRUE);
    }

    /* 子 view 容器 */
    clock_time_view   = widget_lookup(parent, "clock_time_view",   TRUE);
    clock_format_view = widget_lookup(parent, "clock_format_view", TRUE);

    /* 制式选择 */
    for (size_t i = 0; i < CLOCK_FMT_MAX; i++){
        clock_fmt_widget[i] = widget_lookup(parent, clock_fmt_widget_name[i], TRUE);
    }

    /* 二级页面标题和分割线 */
    clock_title_label = widget_lookup(parent, "clock_title", TRUE);
    clock_title_line  = widget_lookup(parent, "clock_line",  TRUE);

    return RET_OK ;
}


void get_label_clock(int32_t *min , int32_t *sec)
{
    if (min == NULL || sec == NULL) return ;

    int32_t h_1 = 0, h_2 = 0, m_1 = 0 , m_2 = 0;

    if(set_clock_widget[CLOCK_H_1_OPTION]){
       h_1 = widget_get_value_int(set_clock_widget[CLOCK_H_1_OPTION]);
    
    }
    if(set_clock_widget[CLOCK_H_2_OPTION]){
       h_2 = widget_get_value_int(set_clock_widget[CLOCK_H_2_OPTION]);

    }
    if(set_clock_widget[CLOCK_M_1_OPTION]){
       m_1 = widget_get_value_int(set_clock_widget[CLOCK_M_1_OPTION]);

    }
    if(set_clock_widget[CLOCK_M_2_OPTION]){
       m_2 = widget_get_value_int(set_clock_widget[CLOCK_M_2_OPTION]);
    }

    *min = h_1 * 10 + h_2;
    *sec = m_1 * 10 + m_2;   

    return ;
}

void refresh_clock(int min ,int sec)
{
    if(set_clock_widget[CLOCK_H_1_OPTION]){
        widget_set_value_int(set_clock_widget[CLOCK_H_1_OPTION], min / 10);
    
    }
    if(set_clock_widget[CLOCK_H_2_OPTION]){
        widget_set_value_int(set_clock_widget[CLOCK_H_2_OPTION], min % 10);

    }
    if(set_clock_widget[CLOCK_M_1_OPTION]){
        widget_set_value_int(set_clock_widget[CLOCK_M_1_OPTION], sec / 10);

    }
    if(set_clock_widget[CLOCK_M_2_OPTION]){
        widget_set_value_int(set_clock_widget[CLOCK_M_2_OPTION], sec % 10);
    }

    return ;
}

uint8_t clock_get_time_format(void)
{
    return vehicle_get_param_time_format() ;
}

/* ════════════════════════════════════════════════════
 *  二级子菜单 helper
 * ════════════════════════════════════════════════════ */
static void clock_sub_show_menu(bool_t show)
{
    /* 显示/隐藏二级子菜单选项 */
    for (size_t i = 0; i < CLOCK_SUB_NUM_MAX; i++) {
        if (clock_sub_widget[i])
            widget_set_visible(clock_sub_widget[i], show);
    }
    /* 同步显示/隐藏"时间"标题和分割线 */
    if (clock_title_label) widget_set_visible(clock_title_label, show);
    if (clock_title_line)  widget_set_visible(clock_title_line,  show);
}

static void clock_sub_set_focused(clock_sub_menu_e idx)
{
    for (size_t i = 0; i < CLOCK_SUB_NUM_MAX; i++) {
        if (clock_sub_widget[i]) {
            widget_set_state(clock_sub_widget[i],
                             (i == idx) ? STATE_SELECTE : STATE_NORMAL);
            widget_invalidate_force(clock_sub_widget[i], NULL);
        }
    }
    sub_menu_sel = idx ;
}

static void clock_sub_clean_state(void)
{
    for (size_t i = 0; i < CLOCK_SUB_NUM_MAX; i++) {
        if (clock_sub_widget[i]) {
            widget_set_state(clock_sub_widget[i], STATE_NORMAL);
            widget_invalidate_force(clock_sub_widget[i], NULL);
        }
    }
}

/* ════════════════════════════════════════════════════
 *  二级页面（MENU_LEVEL_2）—— 子菜单：调整时间 / 调整制式
 * ════════════════════════════════════════════════════ */
static void clock_menu_deal_set(void)
{
    if (sub_menu_sel == CLOCK_SUB_ADJUST_TIME) {
        /* 隐藏二级子菜单，显示时钟数字调节 view */
        clock_sub_show_menu(false);
        if (clock_time_view) widget_set_visible(clock_time_view, true);
        if (clock_format_view) widget_set_visible(clock_format_view, false);

        set_current_level(MENU_LEVEL_3);
        clock_view_set_focused_item(option);
    } else {
        /* 隐藏二级子菜单，显示制式选择 view */
        clock_sub_show_menu(false);
        if (clock_time_view) widget_set_visible(clock_time_view, false);
        if (clock_format_view) widget_set_visible(clock_format_view, true);

        set_current_level(MENU_LEVEL_3);
        clock_fmt_init();
    }
}

static void clock_menu_deal_back(void)
{
    set_current_level(MENU_LEVEL_1);
    clock_sub_clean_state();
}

static void clock_menu_deal_up(void)
{
    clock_sub_menu_e sel = (sub_menu_sel - 1 + CLOCK_SUB_NUM_MAX) % CLOCK_SUB_NUM_MAX ;
    clock_sub_set_focused(sel);
}

static void clock_menu_deal_down(void)
{
    clock_sub_menu_e sel = (sub_menu_sel + 1) % CLOCK_SUB_NUM_MAX ;
    clock_sub_set_focused(sel);
}

static short_click_deal clock_menu_click[] = {
    [KEY_SHORT_UP]   = clock_menu_deal_up  ,
    [KEY_SHORT_DOWN] = clock_menu_deal_down,
    [KEY_SHORT_SET]  = clock_menu_deal_set ,
    [KEY_SHORT_BACK] = clock_menu_deal_back,
};

void clock_init()
{
    /* 进入二级页面：显示子菜单，隐藏子 view */
    clock_sub_show_menu(true);
    if (clock_time_view) widget_set_visible(clock_time_view, false);
    if (clock_format_view) widget_set_visible(clock_format_view, false);

    clock_sub_set_focused(sub_menu_sel);

    return ;
}

void on_clock_deal_short_key(key_id_e key)
{
    printf("on_clock_deal_short_key = %d \n" ,key) ;
    if (key < sizeof(clock_menu_click) / sizeof(short_click_deal) 
        && clock_menu_click[key]) {
        clock_menu_click[key]();
    }

    return ;
}

/* ════════════════════════════════════════════════════
 *  三级页面 A（MENU_LEVEL_3）—— 调整时间：选择 H1/H2/M1/M2
 * ════════════════════════════════════════════════════ */
void clock_view_set_focused_item(clock_option_e focusedIndex)
{
    for (size_t i = 0; i < CLOCK_OPTION_NUM_MAX ; i++)
    {
        if (set_clock_widget[i])
        {
            if (i == focusedIndex)
                widget_set_state(set_clock_widget[i], STATE_SELECTE) ;
            else
                widget_set_state(set_clock_widget[i], STATE_NORMAL ) ;

                
            widget_invalidate_force(set_clock_widget[i] , NULL)  ;
        }
    }

    return ;
}

void clock_view_clean_state()
{
    for (size_t i = 0; i < CLOCK_OPTION_NUM_MAX ; i++)
    {
        if (set_clock_widget[i])
        {
            widget_set_state(set_clock_widget[i], STATE_NORMAL ) ;
            widget_invalidate_force(set_clock_widget[i] , NULL)  ;
        }
    }
}

int32_t option_current_value = 0 ;

static void setting_time_deal_set(void)
{
    set_current_level(MENU_LEVEL_3);  /* 保持 LEVEL_3，进入闪烁编辑 */
    clock_option_init();
    if (set_clock_widget[option])
    {
        option_current_value = widget_get_value_int(set_clock_widget[option] ) ;
    }
}

static void setting_time_deal_back(void)
{
    /* 回到二级子菜单 */
    set_current_level(MENU_LEVEL_2);
    clock_view_clean_state();
    if (clock_time_view) widget_set_visible(clock_time_view, false);
    clock_sub_show_menu(true);
    clock_sub_set_focused(sub_menu_sel);
}

static void setting_time_deal_up(void)
{
    option =  (option - 1 + CLOCK_OPTION_NUM_MAX) % CLOCK_OPTION_NUM_MAX ;
    clock_view_set_focused_item(option) ;
}

static void setting_time_deal_down(void)
{
    option =  (option + 1 ) % CLOCK_OPTION_NUM_MAX ;
    clock_view_set_focused_item(option) ;
}


static short_click_deal time_click[] = {
    [KEY_SHORT_UP]   = setting_time_deal_up  ,
    [KEY_SHORT_DOWN] = setting_time_deal_down,
    [KEY_SHORT_SET]  = setting_time_deal_set ,
    [KEY_SHORT_BACK] = setting_time_deal_back,
};

/* ════════════════════════════════════════════════════
 *  四级页面（闪烁编辑单个数字）—— 从 time 三级进入
 * ════════════════════════════════════════════════════ */
static uint32_t timer_clock_Id = 0 ;

void clock_ctrl_end()
{
    if (timer_clock_Id != 0 && timer_find(timer_clock_Id))
    {
        timer_remove(timer_clock_Id) ;
        timer_clock_Id = 0 ;
    }
    
    if (set_clock_widget[option])
        widget_set_visible(set_clock_widget[option] , true);
    
}

void clock_cacle(int direc)
{
    uint32_t option_time = widget_get_value_int(set_clock_widget[option]);
    uint32_t temp_time   = 0;
    int offset = direc > 0 ? 1 : -1 ;
    
    switch (option)
    {
        case CLOCK_H_1_OPTION:
            temp_time = widget_get_value_int(set_clock_widget[CLOCK_H_2_OPTION]);
            if (temp_time > 3 )
                option_time = (option_time + offset + 2) % 2 ;
            else
                option_time = (option_time + offset + 3) % 3 ;

            break;
        case CLOCK_H_2_OPTION:
            temp_time = widget_get_value_int(set_clock_widget[CLOCK_H_1_OPTION]);
            if (temp_time == 2 )
                option_time = (option_time + offset + 4) % 4 ;
            else
                option_time = (option_time + offset + 10) % 10 ;

            break;
        case CLOCK_M_1_OPTION:
            option_time = (option_time + offset + 6) % 6 ;

            break;
        case CLOCK_M_2_OPTION:
            option_time = (option_time + offset + 10) % 10 ;
        
            break;
        
        default:
            break;
    }

    widget_set_value_int(set_clock_widget[option] ,option_time) ;
}

static void edit_digit_deal_set(void)
{
    set_current_level(MENU_LEVEL_3);
 
    clock_ctrl_end();

    int32_t min = 0, sec = 0 ;
    get_label_clock(&min , &sec);
    vehicle_set_time(min, sec);
    return ;
}

static void edit_digit_deal_back(void)
{
    set_current_level(MENU_LEVEL_3);
    
    clock_ctrl_end();

    if (set_clock_widget[option])
    {
        widget_set_value_int(set_clock_widget[option] , option_current_value) ;
    }

    return ;
}

static void edit_digit_deal_up(void)
{
    clock_cacle(-1) ;
}

static void edit_digit_deal_down(void)
{
    clock_cacle(1) ;
}


static short_click_deal edit_digit_click[] = {
    [KEY_SHORT_UP]   = edit_digit_deal_up  ,
    [KEY_SHORT_DOWN] = edit_digit_deal_down,
    [KEY_SHORT_SET]  = edit_digit_deal_set ,
    [KEY_SHORT_BACK] = edit_digit_deal_back,
};

ret_t on_timer_flicker(const timer_info_t* timer)
{
    (void)timer ;
    if (set_clock_widget[option])
        widget_set_visible(set_clock_widget[option] , !set_clock_widget[option]->visible) ;
    
    return RET_REPEAT ;
}


void clock_option_init()
{
    timer_clock_Id = timer_add(on_timer_flicker , NULL , 500) ;
    return ;
}

/* ════════════════════════════════════════════════════
 *  MENU_LEVEL_3 统一分发
 *  set_page_key.c 中 option_entry[CLOCK] 调用此函数
 *  内部根据 sub_menu_sel 分流到 time 或 format 处理器
 * ════════════════════════════════════════════════════ */
void on_clock_option_deal_short_key(key_id_e key)
{
    printf("on_clock_option_deal_short_key = %d sub=%d\n" , key, sub_menu_sel) ;

    if (sub_menu_sel == CLOCK_SUB_ADJUST_TIME) {
        /* 判断是否在闪烁编辑中（timer_clock_Id != 0） */
        if (timer_clock_Id != 0) {
            /* 四级：编辑单个数字 */
            if (key < sizeof(edit_digit_click) / sizeof(short_click_deal)
                && edit_digit_click[key]) {
                edit_digit_click[key]();
            }
        } else {
            /* 三级：选择 H1/H2/M1/M2 */
            if (key < sizeof(time_click) / sizeof(short_click_deal)
                && time_click[key]) {
                time_click[key]();
            }
        }
    } else {
        /* 三级：制式选择 24H / 12H */
        on_clock_fmt_deal_short_key(key);
    }

    return ;
}

/* ════════════════════════════════════════════════════
 *  三级页面 B（MENU_LEVEL_3）—— 调整制式
 * ════════════════════════════════════════════════════ */
static void clock_fmt_set_focused(clock_fmt_option_e idx)
{
    for (size_t i = 0; i < CLOCK_FMT_MAX; i++) {
        if (clock_fmt_widget[i]) {
            widget_set_state(clock_fmt_widget[i],
                             (i == idx) ? STATE_SELECTE : STATE_NORMAL);
            widget_invalidate_force(clock_fmt_widget[i], NULL);
        }
    }
    fmt_option = idx ;
}

static void clock_fmt_clean_state(void)
{
    for (size_t i = 0; i < CLOCK_FMT_MAX; i++) {
        if (clock_fmt_widget[i]) {
            widget_set_state(clock_fmt_widget[i], STATE_NORMAL);
            widget_invalidate_force(clock_fmt_widget[i], NULL);
        }
    }
}

void clock_fmt_init()
{
    uint8_t cur = vehicle_get_param_time_format();
    fmt_option = (cur == 1) ? CLOCK_FMT_12H : CLOCK_FMT_24H ;
    clock_fmt_set_focused(fmt_option);
}

static void fmt_deal_set(void)
{
    printf("clock_fmt set = %d\n", fmt_option);
    vehicle_set_param_time_format( (fmt_option == CLOCK_FMT_12H) ? 1 : 0 );
}

static void fmt_deal_back(void)
{
    set_current_level(MENU_LEVEL_2);
    clock_fmt_clean_state();
    if (clock_format_view) widget_set_visible(clock_format_view, false);
    clock_sub_show_menu(true);
    clock_sub_set_focused(sub_menu_sel);
}

static void fmt_deal_up(void)
{
    clock_fmt_option_e sel = (fmt_option - 1 + CLOCK_FMT_MAX) % CLOCK_FMT_MAX ;
    clock_fmt_set_focused(sel);
}

static void fmt_deal_down(void)
{
    clock_fmt_option_e sel = (fmt_option + 1) % CLOCK_FMT_MAX ;
    clock_fmt_set_focused(sel);
}

static short_click_deal fmt_click[] = {
    [KEY_SHORT_UP]   = fmt_deal_up  ,
    [KEY_SHORT_DOWN] = fmt_deal_down,
    [KEY_SHORT_SET]  = fmt_deal_set ,
    [KEY_SHORT_BACK] = fmt_deal_back,
};

void on_clock_fmt_deal_short_key(key_id_e key)
{
    if (key < sizeof(fmt_click) / sizeof(short_click_deal)
        && fmt_click[key]) {
        fmt_click[key]();
    }
}