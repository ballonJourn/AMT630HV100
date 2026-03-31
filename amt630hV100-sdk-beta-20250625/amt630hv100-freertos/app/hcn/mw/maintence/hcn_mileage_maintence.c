/**
*
* @file hcn_mileage_maintence.c
*
* @brief This message displayed in Doxygen Files index
*
* @ingroup PackageName
* (note: this needs exactly one @defgroup somewhere)
*
* @date	2025/10/07 14:23
* @author och
*
*/

#include "maintence/hcn_mileage_maintence.h"
#include "storage_param1/hcn_usr_param.h"
#include "vehicle_param/vehicle_param.h"

#ifdef HCN_MILEAGE_MAINTENCE_ENABLE

#define FIRST_MAINTENCE_MILEAGE (1000)      ///< 首保距离
#define NEXT_MAINTENCE_MILEAGE (3000)      ///< 其余保养距离

typedef struct {
    uint16_t count;         ///< 保养次数
    int main_mileage;      ///< 当前段保养距离
    uint32_t temp;
    uint32_t last_main_mileage;  ///< 上次保养保养时的总里程
    int remain_mileage;   ///< 剩余保养里程
    int travel_mileage;  ///< 保养里程范围内已行驶的距离
} mileage_maintenance_t;

static int last_mileage = 0;
static mileage_maintenance_t maintenance;

void clean_maintenance_state(void) {
    if (vehicle_get_data(VEH_MAINT_REMINDER)) {
        int odo = vehicle_get_data(VEH_MILEAGE_TOTAL);
        maintenance.temp = odo/10;

        get_hcn_usr_param(HCN_PARAM_MAINTAIN_COUNTS, &maintenance.count);
        maintenance.count += 1;

        set_hcn_usr_param(HCN_PARAM_LAST_MAINTAIN_MILEAGE, &maintenance.temp);
        set_hcn_usr_param(HCN_PARAM_MAINTAIN_COUNTS, &maintenance.count);
        vehicle_set_data(VEH_MAINT_REMINDER, 0);
    }
}

void update_maintence_mileage(int mileage) {
    if (get_recovery_usr_param() && (mileage != last_mileage)) {
        if (mileage <= 0) {
            return;
        }

        last_mileage = mileage;

        mileage_maintenance_t maintenance = {0};
        get_hcn_usr_param(HCN_PARAM_MAINTAIN_COUNTS, &maintenance.count);
        get_hcn_usr_param(HCN_PARAM_LAST_MAINTAIN_MILEAGE,
                         &maintenance.last_main_mileage);

        if (maintenance.count == 0) {
            maintenance.main_mileage = FIRST_MAINTENCE_MILEAGE;
        } else {
            maintenance.main_mileage = NEXT_MAINTENCE_MILEAGE;
        }

        if (mileage >= maintenance.last_main_mileage) {
            maintenance.travel_mileage =
                mileage - maintenance.last_main_mileage;
            if (maintenance.main_mileage >= maintenance.travel_mileage) {
                maintenance.remain_mileage =
                    maintenance.main_mileage - maintenance.travel_mileage;
                // to do save remain mileage
                if (maintenance.remain_mileage == 0) {
                    vehicle_set_data(VEH_MAINT_REMINDER, 1);
                }
            } else {
                vehicle_set_data(VEH_MAINT_REMINDER, 1);
            }
        } else {
            return;
        }
    }
}

#endif