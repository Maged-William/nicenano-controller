#ifndef TPS43_TAPDRAG_H
#define TPS43_TAPDRAG_H

#include <zephyr/types.h>
#include <stdbool.h>
#include <stdint.h>

void tps43_tapdrag_init(void);
bool tps43_tapdrag_update(bool finger_down, uint64_t now_ms);

#endif
