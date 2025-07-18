#ifndef __RTWLAN_CONFIG_H
#define __RTWLAN_CONFIG_H
#include "FreeRTOSConfig.h"
#define CONFIG_PLATFOMR_CUSTOMER_RTOS
#if !defined(G_CHIP_RLT8733BS)
#define DRV_NAME "RTL8821CS"
#define DRIVERVERSION "c9c2594a99792ec4c03af05362d0c1faa313be44"
#else
#define DRV_NAME "RTL8733BS"
#define DRIVERVERSION "0f8569076b834b99bf94959f0835756396004d90"
#endif

#define CONFIG_DEBUG	                    1

#define CONFIG_MP_INCLUDED                  1
#define CONFIG_MP_NORMAL_IWPRIV_SUPPORT     1
#if !defined(G_CHIP_RLT8733BS)
#define CONFIG_HARDWARE_8821C               1
#endif
#define CONFIG_ENABLE_P2P                   1
#define CONFIG_WPS                          1
#define CONFIG_WPS_AP                       1
#define CONFIG_ENABLE_WPS_AP                1

#endif
