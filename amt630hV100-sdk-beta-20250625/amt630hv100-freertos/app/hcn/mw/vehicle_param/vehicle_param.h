/**
*
* @file vehicle_param.h
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/09/05 10:14
* @author och
*
*/
#ifndef __VEHICLE_PARAM_H__
#define __VEHICLE_PARAM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ///< Battery and Fuel Data
    VEH_VOLTAGE_BATTERY = 0x00,    ///< Battery voltage, precision 0.1
    VEH_FUEL_CONSUMPTION_A,        ///< Fuel consumption A, precision 0.1
    VEH_FUEL_CONSUMPTION_B,        ///< Fuel consumption B, precision 0.1
    VEH_FUEL_CONSUMPTION_INSTANT,  ///< Instant fuel consumption, precision 0.001

    ///< Speed Data
    VEH_SPEED_AVG_A,    ///< Average speed A (km/h), precision 0.1
    VEH_SPEED_AVG_B,    ///< Average speed B (km/h), precision 0.1
    VEH_SPEED_CURRENT,  ///< Current vehicle speed (km/h)
    VEH_SPEED_ENGINE,   ///< Engine speed （rpm）

    ///< Power
    VEH_TRAM_POWR,              ///< Tram powr, unit:KW
    VEH_TRAM_REMAIN_BATTARY,    ///< Tram remain,percentage(0-100)

    ///< Mileage Data
    VEH_MILEAGE_ENDURANCE_A,    ///< Endurance mileage A, precision 0.1
    VEH_MILEAGE_ENDURANCE_B,    ///< Endurance mileage A, precision 0.1
    VEG_AVG_FUEL_CONSUMPTION_A, ///< Average fuel consumption A, precision 0.1
    VEG_AVG_FUEL_CONSUMPTION_B, ///<Average fuel consumption B, precision 0.1
    VEH_MILEAGE_TOTAL,          ///< Total mileage (km), precision 0.1
    VEH_MILEAGE_SUB_A,          ///< Subtotal mileage A (km), precision 0.1
    VEH_MILEAGE_SUB_B,          ///< Subtotal mileage B (km), precision 0.1

    ///< Temperature Data
    VEH_TEMP_WATER,        ///< Water temperature (℃ ), precision 0.1
    VEH_TEMP_WATER_LEVEL,  ///< Coolant temperature level, 0-5 levels, 0:open circuit 
    VEH_TEMP_ENGINE_TEMP,  ///< engine temp(℃),precision 0.1

    ///< Light and Indicator Status
    VEH_LIGHT_HIGH_BEAM,        ///< High beam, 0: off, 1: on
    VEH_LIGHT_LOW_BEAM,         ///< Low beam, 0: off, 1: on
    VEH_INDICATOR_TURN_LEFT,    ///< Left turn signal, 0: off, 1: on
    VEH_INDICATOR_TURN_RIGHT,   ///< Right turn signal, 0: off, 1: on
    VEH_LIGHT_LOCATION,         ///< Location LED status, 0: off, 1: on
    VEH_LIGHT_OIL_PRESSURE,     ///< Oil pressure lamp, 0: off, 1: on
    VEH_LIGHT_ENGINE_FAULT,     ///< Engine fault light, 0: off, 1: on
    VEH_LIGHT_TIRE,             ///< Tire alarm 0: off, 1: on
    VEH_LIGHT_AUTO_START_STOP,  ///< 0: off, 1: on
    VEH_LIGHT_ABS,              ///< 0: off, no show , 1: ABS on
    VEH_LIGHT_WIFI,             ///< 0: off, 1: on
    VEH_AUTO_HEADLIGH,          ///< 0：off, 1:on
    VEH_LIGHT_BRAKE,            ///< 0：off, 1:on
    VEH_LIGHT_READY,            ///< 0：off, 1:on
    VEH_LIGHT_GPS,             ///< 0：off, 1:on

    ///< Gear and Oil Data
    VEH_GEAR_POSITION,  ///< Gear position, 0: neutral 1:D gear 2:R gear
    VEH_OIL_LEVEL,      ///< Oil level, scale value: 0-5
    VEH_OIL_RESISTANCE,  ///< Oil resistance 0x0:short circuit  0xFFF:open circuit;

    ///< Ignition and Side Stand Status
    VEH_IGNITION_SIGNAL,  ///< Ignition electric signal, 0: OFF, 1: ON
    VEH_IGNITION_STATUS,  ///< Ignition status, 0: OFF, 1: ON
    VEH_TILT_SWITCH,      ///< Veh tilt switch 0: OFF, 1: ON
    VEH_SIDE_STAND,       ///< Side stand status, 0: not deployed, 1: deployed

    ///< Maintenance Reminder
    VEH_MAINT_REMINDER,  ///< Maintenance reminder, 0: hidden, 1: required

    ///< WIFI
    VEH_CARLINK_CONNECTED,  ///< 0:disconnect  1:connect
    VEH_CARLINK_URL_STATUS,  ///< carlink url status 0: no url 1: has url
 
    ///< Bluetooth and Phone Status
    VEH_BT_DEV_STATE,  ///< Bluetooth device state, >=1: powered on and working
    VEH_BT_SWITCH_STATUS,     ///< Bluetooth switch status, 0: off, 1: on
    VEH_BT_CONNECTED_STATUS,  ///< Bluetooth connection status, 0: disconnected,
                              ///< 1: connected
    VEH_BT_CONECTED_REMOTE_DEV, ///< Bluetooth connection remote bt dev: 0: disconnected, 1: connected
    VEH_BT_PHONEBOOK_STATE,  ///< Phonebook state, 0: not downloaded, 1:
                             ///< downloading, 2: completed
    VEH_BT_PHONEBOOK_COUNT,  ///< Phonebook contact count
    VEH_BT_CALLLOG_STATE,  ///< Call log state, 0: not downloaded, 1: downloading,
                           ///< 2: completed
    VEH_BT_CALLLOG_COUNT,  ///< Call log count
    VEH_BT_CALL_STATE,     ///< Bluetooth call state, 0: unsupported, 1-6: various
                           ///< states
    VEH_BT_PHONE_SIGNAL,   ///< Phone signal level, 0-5
    VEH_BT_PHONE_BATTERY,  ///< Phone battery level, 0-5

    ///< Weather and Navigation
    VEH_QUETY_WEATHER_STATUS, ///< Query weather status 0: no 1: yes
    VEH_WEATHER_TYPE,  ///< Weather type, 0: none, 1-7: various weather types
    VEH_ENV_TEMP,   ///< Environmental temperature (℃)
    VEH_NAV_MODE,      ///< Navigation mode, 0: none, 1: basic, 2: full screen
    VEH_EASY_NAV_STATUS,    ///< Navigation status, 0: easy navi off, 1: easy navi on

    ///< Miscellaneous
    VEH_LICENSE_AUTH_STATUS,  ///< License authentication status, 0: not registered, 1:
                            ///< registered
    VEH_ACC_OFF_STATUS,  ///< ACC off status, 0: connected, 1: disconnected

    ///< Traction Control System (TCS)
    VEH_TCS_ENABLED,  ///< TCS allow status, 0: not allowed, 1: allowed
    VEH_TCS_MODE,     ///< TCS mode, 0:3 levels 0x01:2 levles 0x02：1 level 0x03：0FF
    VEH_TCS_SLASH_WARING, ///< TCS slash warning lamp 0x00:off 1:on
    VEH_TCS_WARNING,   ///< TCS warning lamp, 0: OFF, 1: ON
    VEH_TCS_SYS_STATE,  ///< TCS system state 0x0:init 0x01:Normal 0x02:Fault 0x03：Diagnostic

    ///< ABS
    VEH_ABS_MODE,       ///< 0:3 levels 0x01:2 levles 0x02：1 level 0x03：0FF
    VEH_ABS_ENALE,      ///< abs allow status, 0: not allowed, 1: allowed
    VEH_DRIVE_MODE,     ///< drive mode:0x0:normal 0x01:eco 0x02:sport 

    ///< Launch Control
    VEH_LAUNCH_CONTROL_STATUS,  ///< Launch control status, 0: OFF, 1: ON
    VEH_LAUNCH_CONTROL_SWITCH,  ///< Launch control switch, 0: OFF, 1: ON

    ///< altitude and compass
    VEH_ALTITUDE_INFO,          ///< Altitude unit:m  precision 0.1
    VEH_COMPASS,                ///< 0:north 1:west 2:south 3：east
    VEH_COMPASS_COURSE,         ///< angle  precision 0.1
    
    ///< clean odo status
    VEH_CLEAR_ODO,             ///< clear odo status:0:no  1:yes
    VEH_TBOX_SET_MILE,         ///< Tbox set odo or trip: 0:No 1:yes

    ///< display info
    VEH_CUR_DISPALY_MODE,    ///< auto display mode  0：day 1:night

    ///< Cruise infp 
    VEH_CRUISE_MAIN_SWITCH, ///< Cruise main switch 0: OFF, 1: ON
    VEH_CRUISE_STATUS,      ///< Cruise status 0: OFF, 1: ON
    VEH_CRUISE_CONTROL_SPEED, ///< Cruise control speed Km/h
    
    VEH_DATA_END,
} veh_data_e;

void vehicle_set_data(veh_data_e id, int32_t value);
int32_t vehicle_get_data(veh_data_e id);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __VEHICLE_PARAM_H__