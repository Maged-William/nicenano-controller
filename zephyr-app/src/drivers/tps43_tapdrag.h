#ifndef TPS43_TAPDRAG_H
#define TPS43_TAPDRAG_H

#include <zephyr/types.h>
#include <stdbool.h>
#include <stdint.h>

void tps43_tapdrag_init(void);

/* Returns left button state (true = pressed).
 * Sets *right_click to true when a right click event should fire.
 */
bool tps43_tapdrag_update(bool finger_down, uint8_t finger_count,
                          uint16_t abs_x, uint16_t abs_y, uint64_t now_ms,
                          bool *right_click);

#endif
