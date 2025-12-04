#include "vehicle_mile.h"
#include "vehicle_data.h"
#include "storage_param2/hcn_mile_param.h"
#if !ON_PC_CACLE
#include "log/hcn_log.h"
#include "uart_communicate/hcn_uart_send_cmd.h"
#endif

///< false:参数未准备好 true:已准备好
bool vehicle_get_mile_recovery()
{
#if !ON_PC_CACLE
    return get_recovery_mile_param() ;
#endif

    return true ;
}

//odo
uint32_t vehicle_get_mile_odo()
{
    uint32_t value ;
#if !ON_PC_CACLE
    if (get_hcn_mile_param(HCN_MILE_PARAM_ODO, &value) )
        return value ;
#endif
    return 0 ;

}

void vehicle_set_mile_odo(double value) 
{
    uint32_t __value = (uint32_t)value ;
#if !ON_PC_CACLE
    set_hcn_mile_param(HCN_MILE_PARAM_ODO, &__value) ;
#endif
    return ;
}

//tripA
uint32_t vehicle_get_mile_tripA()
{
    uint32_t value ;
#if !ON_PC_CACLE
    if (get_hcn_mile_param(HCN_MILE_PARAM_TRIP_A, &value) )
        return value ;
#endif
    return 0 ;

}

void vehicle_set_mile_tripA(double value) 
{
    uint32_t __value = (uint32_t)value ;
#if !ON_PC_CACLE
    set_hcn_mile_param(HCN_MILE_PARAM_TRIP_A, &__value) ;
#endif
    return ;
}

void vehicle_save_mile_tripA(void) 
{
    uint32_t trip_data = 0;
#if !ON_PC_CACLE
    if (get_hcn_mile_param(HCN_MILE_PARAM_TRIP_A, &trip_data)) {
        if (send_mcu_set_trip_a(trip_data) != 0) {
            hcn_log_error("Save trip a data error!\r\n");
        }
    } else {
        hcn_log_error("Get trip a param error!\r\n");
    }
#endif
}

//tripB
uint32_t vehicle_get_mile_tripB()
{
    uint32_t value ;
#if !ON_PC_CACLE
    if (get_hcn_mile_param(HCN_MILE_PARAM_TRIP_B, &value) )
        return value ;
#endif
    return 0 ;

}

void vehicle_set_mile_tripB(double value) 
{
    uint32_t __value = (uint32_t)value ;
#if !ON_PC_CACLE
    set_hcn_mile_param(HCN_MILE_PARAM_TRIP_B, &__value) ;
#endif
    return ;
}

//单次里程
static double mile_once = 0.0 ;
uint32_t vehicle_get_mile_once()
{
    return (uint32_t)mile_once;
}

void vehicle_set_mile_once(double value) 
{
    mile_once = value ;
    return ;
}

void vehicle_save_odo_mileage(void) 
{
    uint32_t odo_data = 0;
#if !ON_PC_CACLE
    if (get_hcn_mile_param(HCN_MILE_PARAM_ODO, &odo_data)) {
        if (send_mcu_set_odo(odo_data) != 0) {
            hcn_log_error("Save odo data error!\r\n");
        }
    } else {
        hcn_log_error("Get odo param error!\r\n");
    }
#endif
} 

