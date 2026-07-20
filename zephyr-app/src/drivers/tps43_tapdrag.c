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
static uint16_t last_valid_x;
static uint16_t last_valid_y;
static bool click_pulse_done;
static enum drag_state prev_state = 0xff;
static bool prev_finger_down;

static uint64_t glitch_grace_until;

#define ABS_INVALID 0xFFFF

void tps43_tapdrag_init(void)
{
	state = ST_IDLE;
	click_pulse_done = false;
	glitch_grace_until = 0;
}

static inline uint32_t abs_diff(uint16_t a, uint16_t b)
{
	return a > b ? (uint32_t)(a - b) : (uint32_t)(b - a);
}

static inline bool is_lift(bool finger_down, uint16_t abs_x, uint16_t abs_y)
{
	return !finger_down && abs_x == ABS_INVALID && abs_y == ABS_INVALID;
}

static inline bool is_touch(bool finger_down, uint16_t abs_x, uint16_t abs_y)
{
	return finger_down && abs_x != ABS_INVALID && abs_y != ABS_INVALID;
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

	if (is_touch(finger_down, abs_x, abs_y)) {
		last_valid_x = abs_x;
		last_valid_y = abs_y;
	}

	/* Gesture0 glitch detection: TPS43 fires hardware tap (gesture0)
	 * during a sustained touch, which temporarily clears FINGER_COUNT
	 * to 0 while abs registers still hold a valid position.
	 * When we see !finger_down + valid abs, set a grace period to
	 * wait for the TPS43 to recover before treating it as a lift.
	 * A real lift always has abs=0xFFFF immediately. */
	if (!finger_down && abs_x != ABS_INVALID && abs_y != ABS_INVALID) {
		if (state == ST_ARMED || state == ST_DRAGGING) {
			glitch_grace_until = now_ms + CONFIG_TPS43_GLITCH_GRACE_MS;
		}
	}
	if (is_touch(finger_down, abs_x, abs_y)) {
		glitch_grace_until = 0;
	}

	switch (state) {

	case ST_IDLE:
		if (is_touch(finger_down, abs_x, abs_y)) {
			touch_start_ms = now_ms;
			touch_start_x = abs_x;
			touch_start_y = abs_y;
			state = ST_TOUCH;
		}
		return false;

	case ST_TOUCH:
		if (is_lift(finger_down, abs_x, abs_y)) {
			if ((now_ms - touch_start_ms) <= CONFIG_TPS43_TAP_MAX_TIME) {
				tap_up_ms = now_ms;
				tap_up_x = last_valid_x;
				tap_up_y = last_valid_y;
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

		if (is_touch(finger_down, abs_x, abs_y)) {
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
		if (!finger_down && now_ms >= glitch_grace_until) {
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
		if (!finger_down && now_ms >= glitch_grace_until) {
			lift_ms = now_ms;
			state = ST_LOCK_WAIT;
		}
		return true;

	case ST_LOCK_WAIT:
		if (is_touch(finger_down, abs_x, abs_y)) {
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
