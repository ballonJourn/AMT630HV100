#include "vehicle_mile.h"
#include "vehicle_data.h"
#include "storage_param2/hcn_mile_param.h"

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