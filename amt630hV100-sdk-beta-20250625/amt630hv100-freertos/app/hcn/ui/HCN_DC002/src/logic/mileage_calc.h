#ifndef MILEAGE_CALC_H_
#define MILEAGE_CALC_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void mileage_calc_init();
void mileage_clear_trip();
void mileage_clear_odo();
bool get_mileage_state();
void set_mileage_state(bool state);
void on_mileage_changed();
#endif


