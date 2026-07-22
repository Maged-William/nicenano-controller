#include "tps43_tapdrag.h"

#include <zephyr/sys/printk.h>

#define TAP_TIMEOUT_MS              CONFIG_ZMK_TPS43_INPUT_TAP_TIMEOUT_MS
#define DOUBLE_CLICK_TIMEOUT_MS     CONFIG_ZMK_TPS43_INPUT_DOUBLE_CLICK_TIMEOUT_MS
#define DRAGLOCK_TIMEOUT_MS         CONFIG_ZMK_TPS43_INPUT_DRAGLOCK_TIMEOUT_MS
#define DROP_GRACE_MS               CONFIG_ZMK_TPS43_INPUT_DROP_GRACE_MS
#define TAP_MOVE_THRESH             CONFIG_ZMK_TPS43_INPUT_TAP_MOVE_THRESH
#define SAME_SPOT_THRESH            CONFIG_ZMK_TPS43_INPUT_SAME_SPOT_THRESH

enum tap_state {
	TAP_STATE_IDLE,
	TAP_STATE_TOUCH,
	TAP_STATE_TOUCH_2,
	TAP_STATE_FIRST_TAP,
	TAP_STATE_SECOND_TOUCH,
	TAP_STATE_1FG_DRAGGING,
	TAP_STATE_1FG_DRAG_WAIT,
	TAP_STATE_DEAD,
};

enum tap_event {
	TAP_EVENT_TOUCH,
	TAP_EVENT_RELEASE,
	TAP_EVENT_MOTION,
	TAP_EVENT_TIMEOUT,
};

static const char * const state_names[] = {
	"IDLE", "TOUCH", "TOUCH_2", "FIRST_TAP",
	"SECOND_TOUCH", "DRAGGING", "DRAG_WAIT", "DEAD"
};

static const char * const event_names[] = {
	"TOUCH", "RELEASE", "MOTION", "TIMEOUT"
};

static enum tap_state state;
static uint64_t touch_start_ms;
static uint16_t touch_start_x;
static uint16_t touch_start_y;
static uint64_t tap_ms;
static uint16_t tap_x;
static uint16_t tap_y;
static uint64_t lift_ms;
static bool button_down;
static uint64_t second_touch_start_ms;
static uint16_t second_touch_start_x;
static uint16_t second_touch_start_y;
static uint8_t prev_fg_count;

static enum tap_state prev_state = 0xff;

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
	prev_fg_count = 0xff;
	prev_state = 0xff;
	lift_ms = 0;
	second_touch_start_ms = 0;
	second_touch_start_x = 0;
	second_touch_start_y = 0;
}

bool tps43_tapdrag_update(bool finger_down, uint8_t finger_count,
                          uint16_t abs_x, uint16_t abs_y, uint64_t now_ms,
                          bool *right_click, bool *double_click)
{
	enum tap_event event;
	bool was_down = (prev_fg_count > 0);
	uint8_t prev_fg = prev_fg_count;
	prev_fg_count = finger_down ? finger_count : 0;

	if (right_click) *right_click = false;
	if (double_click) *double_click = false;

	if (finger_down && !was_down) {
		event = TAP_EVENT_TOUCH;
	} else if (!finger_down && was_down) {
		event = TAP_EVENT_RELEASE;
	} else if (finger_down && was_down) {
		if (finger_count != prev_fg) {
			event = TAP_EVENT_TOUCH;
		} else {
			event = TAP_EVENT_MOTION;
		}
	} else {
		event = TAP_EVENT_TIMEOUT;
	}

	if (state != prev_state || event == TAP_EVENT_TOUCH || event == TAP_EVENT_RELEASE) {
		printk("TD: %s ev=%s fg=%d,%d xy=%u,%u @%llu\n",
		       state_names[state], event_names[event],
		       prev_fg, finger_count, abs_x, abs_y, now_ms);
		prev_state = state;
	}

	switch (state) {

	case TAP_STATE_IDLE:
		if (event == TAP_EVENT_TOUCH) {
			touch_start_ms = now_ms;
			touch_start_x = abs_x;
			touch_start_y = abs_y;
			state = (finger_count >= 2) ? TAP_STATE_TOUCH_2 : TAP_STATE_TOUCH;
		}
		return false;

	case TAP_STATE_TOUCH:
		if (event == TAP_EVENT_RELEASE) {
			uint32_t held = (uint32_t)(now_ms - touch_start_ms);
			bool moved = !within_thresh(abs_x, touch_start_x, TAP_MOVE_THRESH) ||
			             !within_thresh(abs_y, touch_start_y, TAP_MOVE_THRESH);
			if (held <= TAP_TIMEOUT_MS && !moved) {
				button_down = true;
				tap_ms = now_ms;
				tap_x = abs_x;
				tap_y = abs_y;
				state = TAP_STATE_FIRST_TAP;
				return true;
			}
			state = TAP_STATE_IDLE;
			return false;
		}
		if (event == TAP_EVENT_MOTION) {
			if (!within_thresh(abs_x, touch_start_x, TAP_MOVE_THRESH) ||
			    !within_thresh(abs_y, touch_start_y, TAP_MOVE_THRESH)) {
				state = TAP_STATE_DEAD;
			}
			return false;
		}
		if (event == TAP_EVENT_TIMEOUT) {
			if ((uint32_t)(now_ms - touch_start_ms) > TAP_TIMEOUT_MS) {
				state = TAP_STATE_DEAD;
			}
			return false;
		}
		if (event == TAP_EVENT_TOUCH && finger_count >= 2) {
			state = TAP_STATE_TOUCH_2;
		}
		return false;

	case TAP_STATE_TOUCH_2:
		if (event == TAP_EVENT_RELEASE) {
			if ((uint32_t)(now_ms - touch_start_ms) <= TAP_TIMEOUT_MS) {
				if (right_click) *right_click = true;
			}
			state = TAP_STATE_IDLE;
			return false;
		}
		if (event == TAP_EVENT_TIMEOUT) {
			if ((uint32_t)(now_ms - touch_start_ms) > TAP_TIMEOUT_MS) {
				state = TAP_STATE_DEAD;
			}
			return false;
		}
		return false;

	case TAP_STATE_FIRST_TAP:
		if (event == TAP_EVENT_TIMEOUT) {
			if (button_down) {
				button_down = false;
				return false;
			}
			if ((uint32_t)(now_ms - tap_ms) > DOUBLE_CLICK_TIMEOUT_MS) {
				state = TAP_STATE_IDLE;
			}
			return false;
		}
		if (event == TAP_EVENT_TOUCH) {
			uint32_t since_tap = (uint32_t)(now_ms - tap_ms);
			if (since_tap <= DOUBLE_CLICK_TIMEOUT_MS) {
				if (button_down) {
					button_down = false;
				}
				second_touch_start_ms = now_ms;
				second_touch_start_x = abs_x;
				second_touch_start_y = abs_y;
				state = TAP_STATE_SECOND_TOUCH;
			} else {
				touch_start_ms = now_ms;
				touch_start_x = abs_x;
				touch_start_y = abs_y;
				state = TAP_STATE_TOUCH;
			}
			return false;
		}
		if (event == TAP_EVENT_RELEASE) {
			return false;
		}
		return false;

	case TAP_STATE_SECOND_TOUCH:
		if (event == TAP_EVENT_RELEASE) {
			uint32_t held = (uint32_t)(now_ms - second_touch_start_ms);
			bool moved = !within_thresh(abs_x, second_touch_start_x, TAP_MOVE_THRESH) ||
			             !within_thresh(abs_y, second_touch_start_y, TAP_MOVE_THRESH);
			if (held <= TAP_TIMEOUT_MS && !moved &&
			    within_thresh(abs_x, tap_x, SAME_SPOT_THRESH) &&
			    within_thresh(abs_y, tap_y, SAME_SPOT_THRESH)) {
				if (double_click) *double_click = true;
			}
			state = TAP_STATE_IDLE;
			return false;
		}
		if (event == TAP_EVENT_MOTION) {
			if (!within_thresh(abs_x, second_touch_start_x, TAP_MOVE_THRESH) ||
			    !within_thresh(abs_y, second_touch_start_y, TAP_MOVE_THRESH)) {
				button_down = true;
				lift_ms = 0;
				state = TAP_STATE_1FG_DRAGGING;
				return true;
			}
			if ((uint32_t)(now_ms - second_touch_start_ms) > TAP_TIMEOUT_MS) {
				button_down = true;
				lift_ms = 0;
				state = TAP_STATE_1FG_DRAGGING;
				return true;
			}
			return false;
		}
		if (event == TAP_EVENT_TOUCH && finger_count >= 2) {
			state = TAP_STATE_TOUCH_2;
			return false;
		}
		return false;

	case TAP_STATE_1FG_DRAGGING:
		if (event == TAP_EVENT_RELEASE) {
			lift_ms = now_ms;
			return true;
		}
		if (event == TAP_EVENT_TOUCH) {
			if (lift_ms != 0) {
				lift_ms = 0;
			}
			return true;
		}
		if (event == TAP_EVENT_MOTION) {
			return true;
		}
		if (lift_ms != 0 && (now_ms - lift_ms) > DROP_GRACE_MS) {
#ifdef CONFIG_ZMK_TPS43_INPUT_DRAGLOCK_ENABLE
			state = TAP_STATE_1FG_DRAG_WAIT;
#else
			button_down = false;
			state = TAP_STATE_IDLE;
			return false;
#endif
		}
		return true;

	case TAP_STATE_1FG_DRAG_WAIT:
		if (event == TAP_EVENT_TOUCH) {
			if ((uint32_t)(now_ms - lift_ms) <= DRAGLOCK_TIMEOUT_MS) {
				state = TAP_STATE_1FG_DRAGGING;
				return true;
			}
			button_down = false;
			state = TAP_STATE_IDLE;
			return false;
		}
		if (event == TAP_EVENT_TIMEOUT) {
			if ((uint32_t)(now_ms - lift_ms) > DRAGLOCK_TIMEOUT_MS) {
				button_down = false;
				state = TAP_STATE_IDLE;
				return false;
			}
			return true;
		}
		return true;

	case TAP_STATE_DEAD:
		if (event == TAP_EVENT_RELEASE) {
			state = TAP_STATE_IDLE;
		}
		return false;
	}

	return false;
}
