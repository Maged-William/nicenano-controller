#include <zephyr/kernel.h>
#include "tps43_tapdrag.h"

#if CONFIG_TPS43_ENABLE && CONFIG_TPS43_TAPDRAG_ENABLE

enum drag_state {
	ST_IDLE,
	ST_TOUCH,
	ST_TAP_WAIT,
	ST_DRAGGING,
	ST_LOCK_WAIT,
};

static enum drag_state state;
static uint64_t tap_up_ms;
static uint64_t touch_start_ms;
static uint64_t lift_ms;

void tps43_tapdrag_init(void)
{
	state = ST_IDLE;
}

bool tps43_tapdrag_update(bool finger_down, uint64_t now_ms)
{
	switch (state) {

	case ST_IDLE:
		if (finger_down) {
			touch_start_ms = now_ms;
			state = ST_TOUCH;
		}
		break;

	case ST_TOUCH:
		if (!finger_down) {
			if ((now_ms - touch_start_ms) <= CONFIG_TPS43_TAP_MAX_TIME) {
				tap_up_ms = now_ms;
				state = ST_TAP_WAIT;
			} else {
				state = ST_IDLE;
			}
		}
		break;

	case ST_TAP_WAIT:
		if (finger_down) {
			touch_start_ms = now_ms;
			state = ST_DRAGGING;
		} else if ((now_ms - tap_up_ms) > CONFIG_TPS43_REDOWN_WINDOW) {
			state = ST_IDLE;
		}
		break;

	case ST_DRAGGING:
		if (!finger_down) {
			lift_ms = now_ms;
			state = ST_LOCK_WAIT;
		}
		break;

	case ST_LOCK_WAIT:
		if (finger_down) {
			state = ST_DRAGGING;
		} else if ((now_ms - lift_ms) > CONFIG_TPS43_DRAG_LOCK_TIMEOUT) {
			state = ST_IDLE;
		}
		break;
	}

	return state == ST_DRAGGING || state == ST_LOCK_WAIT;
}

#endif
