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

int32_t vehicle_get_time_hour()
{
#if !ON_PC_CACLE
    SystemTime_t time = get_os_date_time() ;
    return time.tm_hour;
#endif
    return 0 ;
}

void vehicle_set_time(int hour, int32_t min)
{
#if !ON_PC_CACLE
    SystemTime_t time = get_os_date_time() ;
    time.tm_hour = hour ;
    time.tm_min = min ;
    set_os_date_time(time);
#endif
    return ;
}
