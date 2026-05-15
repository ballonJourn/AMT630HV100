#include "music_page_key.h"
#include "dock_music_ex_view.h"
#include "animation_ctrl.h"
#include "../view_manager.h"
#include "proxy/bluetooth_data.h"

static music_ex_focused_e focusedIndex = MUSIC_FOCUSED_STATE ;

void music_page_deal_key_down ()
{
    focusedIndex = (focusedIndex + 1 ) % MUSIC_FOCUSED_MAX ;
    music_ex_view_set_focused_item(focusedIndex) ;
}


void music_page_deal_key_up()
{
    focusedIndex = (focusedIndex - 1 + MUSIC_FOCUSED_MAX) % MUSIC_FOCUSED_MAX ;
    music_ex_view_set_focused_item(focusedIndex) ;
}


void music_page_deal_key_set()
{
    switch (focusedIndex)
    {
    case MUSIC_FOCUSED_STATE:
        printf("chaged playing state\n") ;
        vehicle_music_playpause();
        break;
    case MUSIC_FOCUSED_NEXT :
        vehicle_music_forward();
        printf("next\n");
        break;
    case MUSIC_FOCUSED_PREV :
        vehicle_music_backward();
        printf("prev\n");
        break;
    default:
        break;
    }
    
}

void music_page_deal_key_back()
{
    animation_play_in() ;
    set_current_level(MENU_LEVEL_0);
    focusedIndex = MUSIC_FOCUSED_STATE ;
}