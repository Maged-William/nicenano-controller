#include "tps43_edgescroll.h"

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

static uint16_t abs_max_x;
static uint16_t abs_max_y;
static bool edge_scroll_mode;
static bool prev_touched;

void tps43_edgescroll_init(uint16_t max_x, uint16_t max_y)
{
	abs_max_x = max_x;
	abs_max_y = max_y;
	edge_scroll_mode = false;
	prev_touched = false;
}

bool tps43_edgescroll_update(bool touched, uint16_t abs_x, uint16_t abs_y,
                             int16_t rel_x, int16_t rel_y,
                             int *wheel, int *hwheel)
{
#if CONFIG_ZMK_TPS43_INPUT_EDGESCROLL
	if (wheel)  *wheel = 0;
	if (hwheel) *hwheel = 0;

	if (!touched) {
		if (prev_touched) {
			edge_scroll_mode = false;
			prev_touched = false;
		}
		return false;
	}

	uint32_t el = (uint32_t)abs_max_x * CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_PCT / 100;
	uint32_t er = (uint32_t)abs_max_x * (100 - CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_PCT) / 100;
	uint32_t et = (uint32_t)abs_max_y * CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_PCT / 100;
	uint32_t eb = (uint32_t)abs_max_y * (100 - CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_PCT) / 100;

	bool now_in_edge =
		(CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_PCT > 0 && abs_x < el) ||
		(CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_PCT > 0 && abs_x > er) ||
		(CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_PCT > 0 && abs_y < et) ||
		(CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_PCT > 0 && abs_y > eb);

	if (!prev_touched) {
		edge_scroll_mode = now_in_edge;
		if (edge_scroll_mode) {
			printk("ES: entered mode xy=%u,%u\n", abs_x, abs_y);
		}
	}

	prev_touched = touched;

	if (!edge_scroll_mode) {
		return false;
	}

	float sv = (float)rel_y;
	float sh = (float)rel_x;

#define EDGE_SPEED(pct, cond, num, denom, invert, axis) \
	do { \
		if ((pct) > 0 && (cond)) { \
			float s = (float)(num) / (float)(denom); \
			if ((axis) == 0) { sh = 0; } \
			if ((axis) == 1) { sv = 0; } \
			sv *= s; sh *= s; \
			if (IS_ENABLED(invert)) { sv = -sv; sh = -sh; } \
		} \
	} while (0)

	EDGE_SPEED(CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_PCT,   abs_x < el,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_SPEED_NUM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_SPEED_DENOM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_INVERT,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_AXIS);
	EDGE_SPEED(CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_PCT,  abs_x > er,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_SPEED_NUM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_SPEED_DENOM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_INVERT,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_AXIS);
	EDGE_SPEED(CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_PCT,    abs_y < et,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_SPEED_NUM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_SPEED_DENOM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_INVERT,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_AXIS);
	EDGE_SPEED(CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_PCT, abs_y > eb,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_SPEED_NUM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_SPEED_DENOM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_INVERT,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_AXIS);

#undef EDGE_SPEED

	if (!now_in_edge) {
		sv *= 0.1f;
		sh *= 0.1f;
	}

	if (wheel)  *wheel  = (int)sv;
	if (hwheel) *hwheel = (int)sh;

	printk("ES: whl=%d hwhl=%d xy=%u,%u rel=%d,%d sv=%d sh=%d\n",
	       *wheel, *hwheel, abs_x, abs_y, rel_x, rel_y, (int)sv, (int)sh);

	return true;
#else
	if (wheel)  *wheel = 0;
	if (hwheel) *hwheel = 0;
	return false;
#endif
}
