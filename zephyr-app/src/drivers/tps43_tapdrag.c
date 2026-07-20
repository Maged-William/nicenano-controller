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
static uint64_t touch_start_ms;
static uint64_t tap_up_ms;
static uint64_t re_down_ms;
static uint64_t lift_ms;
static uint16_t touch_start_x;
static uint16_t touch_start_y;
static uint16_t tap_up_x;
static uint16_t tap_up_y;
static uint16_t last_x;
static uint16_t last_y;
static bool click_pulse_done;
static enum drag_state prev_state = 0xff;
static bool prev_finger_down;

static bool fd_debounced;
static uint8_t fd_debounce_cnt;

void tps43_tapdrag_init(void)
{
	state = ST_IDLE;
	click_pulse_done = false;
	fd_debounced = false;
	fd_debounce_cnt = 0;
}

static inline uint32_t abs_diff(uint16_t a, uint16_t b)
{
	return a > b ? (uint32_t)(a - b) : (uint32_t)(b - a);
}

static bool debounce_fd(bool raw)
{
	if (raw == fd_debounced) {
		fd_debounce_cnt = 0;
		return fd_debounced;
	}
	fd_debounce_cnt++;
	if (fd_debounce_cnt >= CONFIG_TPS43_FD_DEBOUNCE_TICKS) {
		fd_debounced = raw;
		fd_debounce_cnt = 0;
	}
	return fd_debounced;
}

bool tps43_tapdrag_update(bool finger_down, uint16_t abs_x, uint16_t abs_y, uint64_t now_ms)
{
	if (state != prev_state) {
		printk("TD: %d->%d fd=%d xy=%u,%u @%llu\n",
		       prev_state, state, finger_down, abs_x, abs_y, now_ms);
		prev_state = state;
	}
	if (finger_down != prev_finger_down) {
		printk("TD: fd %d->%d xy=%u,%u @%llu\n",
		       prev_finger_down, finger_down, abs_x, abs_y, now_ms);
		prev_finger_down = finger_down;
	}

	bool fd = debounce_fd(finger_down);

	switch (state) {

	case ST_IDLE:
		if (fd) {
			touch_start_ms = now_ms;
			touch_start_x = abs_x;
			touch_start_y = abs_y;
			state = ST_TOUCH;
		}
		return false;

	case ST_TOUCH:
		if (!fd) {
			if ((now_ms - touch_start_ms) <= CONFIG_TPS43_TAP_MAX_TIME) {
				tap_up_ms = now_ms;
				tap_up_x = abs_x;
				tap_up_y = abs_y;
				click_pulse_done = false;
				state = ST_TAP_WAIT;
				return true;
			}
			state = ST_IDLE;
			return false;
		}

		if (finger_down) {
			if (abs_diff(abs_x, touch_start_x) > CONFIG_TPS43_TAP_MOVE_THRESH ||
			    abs_diff(abs_y, touch_start_y) > CONFIG_TPS43_TAP_MOVE_THRESH) {
				state = ST_IDLE;
				return false;
			}
		}
		return false;

	case ST_TAP_WAIT:
		if (!click_pulse_done) {
			click_pulse_done = true;
			return false;
		}

		if (fd) {
			if (abs_diff(abs_x, tap_up_x) <= CONFIG_TPS43_SAME_SPOT_THRESH &&
			    abs_diff(abs_y, tap_up_y) <= CONFIG_TPS43_SAME_SPOT_THRESH) {
				last_x = abs_x;
				last_y = abs_y;
				re_down_ms = now_ms;
				state = ST_ARMED;
				return true;
			}
			state = ST_IDLE;
			return false;
		}

		if ((now_ms - tap_up_ms) > CONFIG_TPS43_REDOWN_WINDOW) {
			state = ST_IDLE;
			return false;
		}
		return false;

	case ST_ARMED:
		if (!fd) {
			state = ST_IDLE;
			return false;
		}

		if (finger_down &&
		    (abs_diff(abs_x, last_x) > CONFIG_TPS43_ARM_MOVE_THRESH ||
		     abs_diff(abs_y, last_y) > CONFIG_TPS43_ARM_MOVE_THRESH)) {
#if CONFIG_TPS43_CONFIRM_WINDOW > 0
			if ((now_ms - re_down_ms) <= CONFIG_TPS43_CONFIRM_WINDOW) {
				state = ST_DRAGGING;
			} else {
				state = ST_IDLE;
				return false;
			}
#else
			state = ST_DRAGGING;
#endif
		}
		return true;

	case ST_DRAGGING:
		if (!fd) {
			lift_ms = now_ms;
			state = ST_LOCK_WAIT;
		}
		return true;

	case ST_LOCK_WAIT:
		if (fd) {
			state = ST_DRAGGING;
			return true;
		}
		if ((now_ms - lift_ms) > CONFIG_TPS43_DRAG_LOCK_TIMEOUT) {
			state = ST_IDLE;
			return false;
		}
		return true;
	}

	return false;
}

#endif
