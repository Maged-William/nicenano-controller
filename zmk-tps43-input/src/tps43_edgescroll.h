#ifndef TPS43_EDGESCROLL_H
#define TPS43_EDGESCROLL_H

#include <stdbool.h>
#include <stdint.h>

void tps43_edgescroll_init(uint16_t abs_max_x, uint16_t abs_max_y);
bool tps43_edgescroll_update(bool touched, uint16_t abs_x, uint16_t abs_y,
                             int16_t rel_x, int16_t rel_y,
                             int *wheel, int *hwheel);

#endif
