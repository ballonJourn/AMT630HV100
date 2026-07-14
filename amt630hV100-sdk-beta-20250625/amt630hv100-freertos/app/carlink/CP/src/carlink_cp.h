#ifndef _CARLINK_CP_H_
#define _CARLINK_CP_H_

#include <stdbool.h>

int carlink_cp_init();
void carlink_cp_enable(int enable);

bool bt_connected_but_cp_rejected(void);

#endif
