#include "tps43_edgescroll.h"

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

	if (!prev_touched) {
		edge_scroll_mode =
			(CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_PCT > 0 && abs_x < el) ||
			(CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_PCT > 0 && abs_x > er) ||
			(CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_PCT > 0 && abs_y < et) ||
			(CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_PCT > 0 && abs_y > eb);
	}

	prev_touched = touched;

	if (!edge_scroll_mode) {
		return false;
	}

	float sv = (float)rel_y;
	float sh = (float)rel_x;

#define EDGE_SPEED(pct, cond, num, denom, invert) \
	do { \
		if ((pct) > 0 && (cond)) { \
			float s = (float)(num) / (float)(denom); \
			sv *= s; sh *= s; \
			if (invert) { sv = -sv; sh = -sh; } \
		} \
	} while (0)

	EDGE_SPEED(CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_PCT,   abs_x < el,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_SPEED_NUM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_SPEED_DENOM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_INVERT);
	EDGE_SPEED(CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_PCT,  abs_x > er,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_SPEED_NUM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_SPEED_DENOM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_INVERT);
	EDGE_SPEED(CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_PCT,    abs_y < et,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_SPEED_NUM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_SPEED_DENOM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_INVERT);
	EDGE_SPEED(CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_PCT, abs_y > eb,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_SPEED_NUM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_SPEED_DENOM,
	           CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_INVERT);

#undef EDGE_SPEED

	if (!((CONFIG_ZMK_TPS43_INPUT_EDGE_LEFT_PCT > 0 && abs_x < el) ||
	      (CONFIG_ZMK_TPS43_INPUT_EDGE_RIGHT_PCT > 0 && abs_x > er) ||
	      (CONFIG_ZMK_TPS43_INPUT_EDGE_TOP_PCT > 0 && abs_y < et) ||
	      (CONFIG_ZMK_TPS43_INPUT_EDGE_BOTTOM_PCT > 0 && abs_y > eb))) {
		sv *= 0.1f;
		sh *= 0.1f;
	}

	if (wheel)  *wheel  = (int)sv;
	if (hwheel) *hwheel = (int)sh;

	return true;
}
