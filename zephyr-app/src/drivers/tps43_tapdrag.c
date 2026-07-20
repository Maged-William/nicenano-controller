#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "tps43_tapdrag.h"

#if CONFIG_TPS43_ENABLE && CONFIG_TPS43_TAPDRAG_ENABLE

enum drag_state {
	ST_IDLE,
	ST_TOUCH,
	ST_TAP_WAIT,
	ST_ARMED,
	ST_DRAGGING,
	ST_LOCK_WAIT,
};

static enum drag_state state;

static uint16_t last_x;
static uint16_t last_y;
static uint16_t up_x;
static uint16_t up_y;
static uint16_t down_x;
static uint16_t down_y;

static uint64_t tap_up_ms;
static uint64_t touch_start_ms;
static uint64_t lift_ms;

/* A tap click needs to be held long enough for the host to register it.
 * 4ms tick → hold click for at least 8 ticks (32ms).
 */
#define CLICK_HOLD_MS  32

void tps43_tapdrag_init(void)
{
	state = ST_IDLE;
}

bool tps43_tapdrag_update(bool finger_down, uint16_t abs_x, uint16_t abs_y,
			  uint64_t now_ms)
{
	if (finger_down) {
		last_x = abs_x;
		last_y = abs_y;
	}

	switch (state) {

	case ST_IDLE:
		if (finger_down) {
			state = ST_TOUCH;
			touch_start_ms = now_ms;
		}
		break;

	case ST_TOUCH:
		if (!finger_down) {
			uint64_t held = now_ms - touch_start_ms;
			if (held <= CONFIG_TPS43_TAP_MAX_TIME) {
				up_x = last_x;
				up_y = last_y;
				tap_up_ms = now_ms;
				state = ST_TAP_WAIT;
			} else {
				state = ST_IDLE;
			}
		}
		break;

	case ST_TAP_WAIT:
		if (finger_down) {
			int32_t dx = (int32_t)last_x - (int32_t)up_x;
			int32_t dy = (int32_t)last_y - (int32_t)up_y;
			if (dx < 0) dx = -dx;
			if (dy < 0) dy = -dy;
			if (dx <= CONFIG_TPS43_SAME_SPOT_THRESH &&
			    dy <= CONFIG_TPS43_SAME_SPOT_THRESH) {
				down_x = last_x;
				down_y = last_y;
				touch_start_ms = now_ms;
				state = ST_ARMED;
			} else {
				state = ST_IDLE;
			}
		} else if ((now_ms - tap_up_ms) > CONFIG_TPS43_REDOWN_WINDOW) {
			state = ST_IDLE;
		}
		break;

	case ST_ARMED:
		if (!finger_down) {
			state = ST_IDLE;
			break;
		}
		{
			int32_t dx = (int32_t)last_x - (int32_t)down_x;
			int32_t dy = (int32_t)last_y - (int32_t)down_y;
			if (dx < 0) dx = -dx;
			if (dy < 0) dy = -dy;
			if (dx > CONFIG_TPS43_TAP_MOVE_THRESH ||
			    dy > CONFIG_TPS43_TAP_MOVE_THRESH) {
				if (CONFIG_TPS43_CONFIRM_WINDOW == 0 ||
				    (now_ms - touch_start_ms) <= CONFIG_TPS43_CONFIRM_WINDOW) {
					state = ST_DRAGGING;
				} else {
					state = ST_IDLE;
				}
			}
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

	/* Determine button return value based on state: */
	switch (state) {
	case ST_ARMED:
	case ST_DRAGGING:
	case ST_LOCK_WAIT:
		return true;
	case ST_TAP_WAIT:
		/* Hold click pulse for CLICK_HOLD_MS so the host registers it */
		if ((now_ms - tap_up_ms) < CLICK_HOLD_MS) {
			return true;
		}
		/* If finger came back down, button is re-held via state change above */
		return false;
	default:
		return false;
	}
}

#endif
