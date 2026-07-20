#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
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
	(void)abs_x;
	(void)abs_y;

	/* === debug: print state transitions === */
	static enum drag_state prev_state = (enum drag_state)-1;

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
				/* tap detected: click, start timer1 */
				tap_up_ms = now_ms;
				state = ST_TAP_WAIT;
			} else {
				/* held too long → just a press, not a tap */
				state = ST_IDLE;
			}
		}
		break;

	case ST_TAP_WAIT:
		if (finger_down) {
			/* re-touch within window → arm the drag immediately */
			touch_start_ms = now_ms;
			state = ST_DRAGGING;
		} else if ((now_ms - tap_up_ms) > CONFIG_TPS43_REDOWN_WINDOW) {
			/* timer1 expired → plain tap, nothing more */
			state = ST_IDLE;
		}
		break;

	case ST_DRAGGING:
		if (!finger_down) {
			/* lifted mid-drag → start drag lock timer */
			lift_ms = now_ms;
			state = ST_LOCK_WAIT;
		}
		break;

	case ST_LOCK_WAIT:
		if (finger_down) {
			/* re-touched in time → keep dragging */
			state = ST_DRAGGING;
		} else if ((now_ms - lift_ms) > CONFIG_TPS43_DRAG_LOCK_TIMEOUT) {
			/* timer3 expired → end drag */
			state = ST_IDLE;
		}
		break;
	}

	if (state != prev_state) {
		static const char *names[] = {
			"IDLE", "TOUCH", "TAP_WAIT", "DRAGGING", "LOCK_WAIT"
		};
		uint8_t n = (uint8_t)state;
		if (n < sizeof(names) / sizeof(names[0])) {
			printk("TAPDRAG %s (f=%d)\n", names[n], finger_down);
		}
		prev_state = state;
	}

	/* Determine button return value based on state: */
	switch (state) {
	case ST_DRAGGING:
	case ST_LOCK_WAIT:
		return true;
	case ST_TAP_WAIT:
		/* Hold click pulse for CLICK_HOLD_MS so the host registers it */
		return (now_ms - tap_up_ms) < CLICK_HOLD_MS;
	default:
		return false;
	}
}

#endif
