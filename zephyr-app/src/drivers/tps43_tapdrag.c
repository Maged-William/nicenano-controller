#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "tps43_tapdrag.h"

#if CONFIG_TPS43_ENABLE && CONFIG_TPS43_TAPDRAG_ENABLE

#define TAP_TIMEOUT_MS          CONFIG_TPS43_TAP_TIMEOUT_MS
#define DRAG_TIMEOUT_MS        CONFIG_TPS43_DRAG_TIMEOUT_MS
#define DRAGLOCK_TIMEOUT_MS    CONFIG_TPS43_DRAGLOCK_TIMEOUT_MS
#define DROP_GRACE_MS          CONFIG_TPS43_DROP_GRACE_MS
#define TAP_MOVE_THRESH        CONFIG_TPS43_TAP_MOVE_THRESH
#define SAME_SPOT_THRESH       CONFIG_TPS43_SAME_SPOT_THRESH
#define RELEASE_DEBOUNCE_MS    CONFIG_TPS43_RELEASE_DEBOUNCE_MS

enum tap_state {
	TAP_STATE_IDLE,
	TAP_STATE_TOUCH,
	TAP_STATE_1FG_TAPPED,
	TAP_STATE_1FG_DRAG_OR_DC,
	TAP_STATE_1FG_DRAGGING,
	TAP_STATE_1FG_DRAG_WAIT,
	TAP_STATE_DEAD,
};

static enum tap_state state;
static uint64_t touch_start_ms;
static uint16_t touch_start_x;
static uint16_t touch_start_y;
static uint64_t tap_ms;
static uint16_t tap_x;
static uint16_t tap_y;
static uint64_t re_down_ms;
static uint16_t re_down_x;
static uint16_t re_down_y;
static uint64_t lift_ms;
static uint32_t drag_wait_timeout_ms;
static bool button_down;

static enum tap_state prev_state = 0xff;
static bool prev_was_touch;
static uint64_t finger_lost_ms;

static inline uint32_t abs_diff(uint16_t a, uint16_t b)
{
	return a > b ? (uint32_t)(a - b) : (uint32_t)(b - a);
}

static inline bool within_thresh(uint16_t a, uint16_t b, uint16_t thresh)
{
	return abs_diff(a, b) <= thresh;
}

void tps43_tapdrag_init(void)
{
	state = TAP_STATE_IDLE;
	button_down = false;
	prev_state = 0xff;
	prev_was_touch = false;
	finger_lost_ms = 0;
}

bool tps43_tapdrag_update(bool finger_down, uint16_t abs_x, uint16_t abs_y, uint64_t now_ms,
                          bool *double_click)
{
	if (finger_down) {
		finger_lost_ms = 0;
	} else if (finger_lost_ms == 0) {
		finger_lost_ms = now_ms;
	}
	if (!finger_down && finger_lost_ms > 0 &&
	    (now_ms - finger_lost_ms) < RELEASE_DEBOUNCE_MS) {
		finger_down = true;
	}

	bool was_down = prev_was_touch;
	prev_was_touch = finger_down;

	if (double_click) *double_click = false;

	enum { EV_TOUCH, EV_RELEASE, EV_MOTION, EV_TIMEOUT } event;
	if (finger_down && !was_down) {
		event = EV_TOUCH;
	} else if (!finger_down && was_down) {
		event = EV_RELEASE;
	} else if (finger_down && was_down) {
		event = EV_MOTION;
	} else {
		event = EV_TIMEOUT;
	}

	if (state != prev_state || event == EV_TOUCH || event == EV_RELEASE) {
		static const char * const sn[] = {
			"IDLE", "TOUCH", "TAPPED", "DC_DRAG", "DRAG", "DWAIT", "DEAD"
		};
		static const char * const en[] = {"TOUCH","RELEASE","MOTION","TIMEOUT"};
		printk("TD: %s ev=%s fg=%d xy=%u,%u @%llu\n",
		       sn[state], en[event], finger_down, abs_x, abs_y, now_ms);
		prev_state = state;
	}

	switch (state) {

	case TAP_STATE_IDLE:
		if (event == EV_TOUCH) {
			touch_start_ms = now_ms;
			touch_start_x = abs_x;
			touch_start_y = abs_y;
			state = TAP_STATE_TOUCH;
		}
		return false;

	case TAP_STATE_TOUCH:
		if (event == EV_RELEASE) {
			uint32_t held = (uint32_t)(now_ms - touch_start_ms);
			bool moved = !within_thresh(abs_x, touch_start_x, TAP_MOVE_THRESH) ||
			             !within_thresh(abs_y, touch_start_y, TAP_MOVE_THRESH);
			if (held <= TAP_TIMEOUT_MS && !moved) {
				button_down = true;
				tap_ms = now_ms;
				tap_x = abs_x;
				tap_y = abs_y;
				state = TAP_STATE_1FG_TAPPED;
				return true;
			}
			state = TAP_STATE_IDLE;
			return false;
		}
		if (event == EV_MOTION) {
			if (!within_thresh(abs_x, touch_start_x, TAP_MOVE_THRESH) ||
			    !within_thresh(abs_y, touch_start_y, TAP_MOVE_THRESH)) {
				state = TAP_STATE_DEAD;
			}
			return false;
		}
		if (event == EV_TIMEOUT) {
			uint32_t held = (uint32_t)(now_ms - touch_start_ms);
			if (held > TAP_TIMEOUT_MS) {
				state = TAP_STATE_DEAD;
			}
			return false;
		}
		return false;

	case TAP_STATE_1FG_TAPPED:
		if (event == EV_TOUCH) {
			uint32_t since_tap = (uint32_t)(now_ms - tap_ms);
			if (since_tap <= DRAG_TIMEOUT_MS &&
			    within_thresh(abs_x, tap_x, SAME_SPOT_THRESH) &&
			    within_thresh(abs_y, tap_y, SAME_SPOT_THRESH)) {
				re_down_ms = now_ms;
				re_down_x = abs_x;
				re_down_y = abs_y;
				state = TAP_STATE_1FG_DRAG_OR_DC;
				return true;
			}
			button_down = false;
			state = TAP_STATE_IDLE;
			return false;
		}
		if (event == EV_TIMEOUT) {
			uint32_t since_tap = (uint32_t)(now_ms - tap_ms);
			if (since_tap > DRAG_TIMEOUT_MS) {
				button_down = false;
				state = TAP_STATE_IDLE;
				return false;
			}
			return true;
		}
		if (event == EV_RELEASE) {
			return true;
		}
		return true;

	case TAP_STATE_1FG_DRAG_OR_DC:
		if (event == EV_RELEASE) {
			uint32_t since_re = (uint32_t)(now_ms - re_down_ms);
			bool moved = !within_thresh(abs_x, re_down_x, TAP_MOVE_THRESH) ||
			             !within_thresh(abs_y, re_down_y, TAP_MOVE_THRESH);
			if (since_re <= TAP_TIMEOUT_MS && !moved) {
				button_down = false;
				if (double_click) *double_click = true;
				state = TAP_STATE_IDLE;
				return false;
			}
			button_down = false;
			state = TAP_STATE_IDLE;
			return false;
		}
		if (event == EV_MOTION) {
			state = TAP_STATE_1FG_DRAGGING;
			return true;
		}
		if (event == EV_TIMEOUT) {
			uint32_t since_re = (uint32_t)(now_ms - re_down_ms);
			if (since_re > TAP_TIMEOUT_MS) {
				button_down = false;
				state = TAP_STATE_IDLE;
				return false;
			}
			return true;
		}
		return true;

	case TAP_STATE_1FG_DRAGGING:
		if (event == EV_RELEASE) {
			lift_ms = now_ms;
#ifdef CONFIG_TPS43_DRAGLOCK_ENABLE
			drag_wait_timeout_ms = DRAGLOCK_TIMEOUT_MS;
#else
			drag_wait_timeout_ms = DROP_GRACE_MS;
#endif
			if (drag_wait_timeout_ms > 0) {
				state = TAP_STATE_1FG_DRAG_WAIT;
				return true;
			}
			button_down = false;
			state = TAP_STATE_IDLE;
			return false;
		}
		if (event == EV_TOUCH || event == EV_MOTION) {
			return true;
		}
		return true;

	case TAP_STATE_1FG_DRAG_WAIT:
		if (event == EV_TOUCH) {
			uint32_t since_lift = (uint32_t)(now_ms - lift_ms);
			if (since_lift <= drag_wait_timeout_ms) {
				state = TAP_STATE_1FG_DRAGGING;
				return true;
			}
			button_down = false;
			state = TAP_STATE_IDLE;
			return false;
		}
		if (event == EV_TIMEOUT) {
			uint32_t since_lift = (uint32_t)(now_ms - lift_ms);
			if (since_lift > drag_wait_timeout_ms) {
				button_down = false;
				state = TAP_STATE_IDLE;
				return false;
			}
			return true;
		}
		return true;

	case TAP_STATE_DEAD:
		if (event == EV_RELEASE) {
			state = TAP_STATE_IDLE;
		}
		return false;
	}

	return false;
}

#endif
