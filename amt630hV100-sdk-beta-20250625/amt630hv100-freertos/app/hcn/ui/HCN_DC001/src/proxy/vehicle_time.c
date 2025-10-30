#include "vehicle_time.h"
#include "vehicle_data.h"

int32_t vehicle_get_time_min()
{
#if !ON_PC_CACLE
    SystemTime_t time = get_os_date_time() ;
    return time.tm_min ;
#endif
    return 0 ;
}

int32_t vehicle_get_time_sec()
{
#if !ON_PC_CACLE
    SystemTime_t time = get_os_date_time() ;
    return time.tm_sec;
#endif
    return 0 ;
}

void vehicle_set_time_min(int32_t min)
{
#if !ON_PC_CACLE
    SystemTime_t time = get_os_date_time() ;
    time.tm_min = min ;
    set_os_date_time(time);
#endif
    return ;
}

void vehicle_set_time_sec(int32_t sec)
{
#if !ON_PC_CACLE
    SystemTime_t time = get_os_date_time() ;
    time.tm_sec = sec ;
    set_os_date_time(time);
#endif
    return ;
}
