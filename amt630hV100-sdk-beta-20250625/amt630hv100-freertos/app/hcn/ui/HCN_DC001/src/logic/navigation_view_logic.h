#ifndef NAVATION_VIEW_LOGIC_H
#define NAVATION_VIEW_LOGIC_H

#include "carlink_cb/hcn_easy_navi.h"

void update_qr();

void navigation_view_update() ;

ret_t parse_navigation_data(const hcnNavigationHudInfo *_navigation_info) ;

#endif