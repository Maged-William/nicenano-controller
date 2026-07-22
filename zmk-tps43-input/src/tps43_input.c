#define DT_DRV_COMPAT zmk_tps43_input

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include "tps43_regs.h"
#include "tps43_tapdrag.h"
#include "tps43_edgescroll.h"

LOG_MODULE_REGISTER(tps43_input, CONFIG_ZMK_TPS43_INPUT_LOG_LEVEL);

struct tps43_input_config {
	const struct device *i2c_bus;
	uint16_t i2c_addr;
	uint32_t interval_ms;
};

struct tps43_input_data {
	struct k_work_delayable work;
	const struct device *dev;
	uint8_t regs[16];
	bool found;
	bool prev_button_down;
};

static int tps43_end_comm(const struct device *i2c, uint16_t addr)
{
	uint8_t end_cmd[3] = { 0xEE, 0xEE, 0x00 };
	return i2c_write(i2c, end_cmd, 3, addr);
}

static int tps43_read_block(const struct device *i2c, uint16_t addr, uint8_t *buf)
{
	uint8_t reg_ptr[2] = { 0x00, TPS43_GESTURE0 };
	return i2c_write_read(i2c, addr, reg_ptr, 2, buf, 16);
}

static int tps43_write_config(const struct device *i2c, uint16_t addr, uint16_t reg, uint8_t val)
{
	uint8_t cmd[3] = { reg >> 8, reg & 0xFF, val };
	tps43_end_comm(i2c, addr);
	int ret = i2c_write(i2c, cmd, 3, addr);
	tps43_end_comm(i2c, addr);
	return ret;
}

static bool tps43_probe(const struct device *i2c, uint16_t addr)
{
	uint8_t end_cmd[3] = { 0xEE, 0xEE, 0x00 };
	uint8_t reg_ptr[2] = { 0x00, TPS43_GESTURE0 };
	uint8_t test_buf[1];

	i2c_write(i2c, end_cmd, 3, addr);
	k_sleep(K_MSEC(100));

	for (int retry = 0; retry < 5; retry++) {
		if (i2c_write_read(i2c, addr, reg_ptr, 2, test_buf, 1) == 0) {
			i2c_write(i2c, end_cmd, 3, addr);
			return true;
		}
		i2c_write(i2c, end_cmd, 3, addr);
		k_sleep(K_MSEC(200));
	}
	return false;
}

static void tps43_poll_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct tps43_input_data *data =
		CONTAINER_OF(dwork, struct tps43_input_data, work);
	const struct tps43_input_config *cfg = data->dev->config;
	const struct device *dev = data->dev;
	const struct device *i2c = cfg->i2c_bus;
	uint16_t addr = cfg->i2c_addr;

	if (!data->found) {
		uint8_t reg_ptr[2] = { 0x00, TPS43_GESTURE0 };
		uint8_t test_buf[1];
		tps43_end_comm(i2c, addr);
		if (i2c_write_read(i2c, addr, reg_ptr, 2, test_buf, 1) == 0) {
			data->found = true;
			LOG_INF("TPS43 found (late init)");
		}
		tps43_end_comm(i2c, addr);
		k_work_schedule(&data->work, K_MSEC(cfg->interval_ms));
		return;
	}

	if (tps43_read_block(i2c, addr, data->regs) != 0) {
		tps43_end_comm(i2c, addr);
		k_work_schedule(&data->work, K_MSEC(cfg->interval_ms));
		return;
	}
	tps43_end_comm(i2c, addr);

	int16_t rel_x = (int16_t)((data->regs[TPS43_XREL_HIGH - TPS43_GESTURE0] << 8) |
	                          data->regs[TPS43_XREL_LOW  - TPS43_GESTURE0]);
	int16_t rel_y = (int16_t)((data->regs[TPS43_YREL_HIGH - TPS43_GESTURE0] << 8) |
	                          data->regs[TPS43_YREL_LOW  - TPS43_GESTURE0]);
	uint16_t abs_x = ((uint16_t)data->regs[TPS43_XABS_HIGH - TPS43_GESTURE0] << 8) |
	                  data->regs[TPS43_XABS_LOW  - TPS43_GESTURE0];
	uint16_t abs_y = ((uint16_t)data->regs[TPS43_YABS_HIGH - TPS43_GESTURE0] << 8) |
	                  data->regs[TPS43_YABS_LOW  - TPS43_GESTURE0];
	uint8_t finger_count = data->regs[TPS43_FINGER_COUNT - TPS43_GESTURE0];
	bool touched = finger_count > 0;

	input_report(dev, INPUT_EV_KEY, INPUT_BTN_TOUCH, touched ? 1 : 0, false, K_NO_WAIT);

#if CONFIG_ZMK_TPS43_INPUT_EDGESCROLL
	int edge_wheel = 0, edge_hwheel = 0;
	bool in_edge = tps43_edgescroll_update(touched, abs_x, abs_y, rel_x, rel_y,
	                                       &edge_wheel, &edge_hwheel);
#else
	bool in_edge = false;
#endif

#if CONFIG_ZMK_TPS43_INPUT_TAPDRAG
	{
		bool rc = false, dc = false;
		uint8_t fg = touched ? finger_count : 0;
		bool left_down = tps43_tapdrag_update(touched, fg, abs_x, abs_y,
		                                      k_uptime_get(), &rc, &dc);

		if (left_down != data->prev_button_down) {
			data->prev_button_down = left_down;
			input_report(dev, INPUT_EV_KEY, INPUT_BTN_LEFT, left_down ? 1 : 0, false, K_NO_WAIT);
			printk("BTN_LEFT: %s\n", left_down ? "DOWN" : "UP");
		}

		if (rc) {
			input_report(dev, INPUT_EV_KEY, INPUT_BTN_RIGHT, 1, true, K_NO_WAIT);
			input_report(dev, INPUT_EV_KEY, INPUT_BTN_RIGHT, 0, true, K_NO_WAIT);
			printk("BTN_RIGHT: click\n");
		}

		if (dc) {
			input_report(dev, INPUT_EV_KEY, INPUT_BTN_LEFT, 1, true, K_NO_WAIT);
			input_report(dev, INPUT_EV_KEY, INPUT_BTN_LEFT, 0, true, K_NO_WAIT);
			input_report(dev, INPUT_EV_KEY, INPUT_BTN_LEFT, 1, true, K_NO_WAIT);
			input_report(dev, INPUT_EV_KEY, INPUT_BTN_LEFT, 0, true, K_NO_WAIT);
			printk("BTN_LEFT: double-click\n");
		}
	}
#else
	(void)abs_x;
	(void)abs_y;
	(void)finger_count;
#endif

	if (in_edge) {
		if (edge_wheel) {
			input_report(dev, INPUT_EV_REL, INPUT_REL_WHEEL, edge_wheel, false, K_NO_WAIT);
		}
		if (edge_hwheel) {
			input_report(dev, INPUT_EV_REL, INPUT_REL_HWHEEL, edge_hwheel, true, K_NO_WAIT);
		}
	} else if (touched) {
		float sens_num = CONFIG_ZMK_TPS43_INPUT_SENSITIVITY_NUM;
		float sens_den = CONFIG_ZMK_TPS43_INPUT_SENSITIVITY_DENOM;
		float mx = (float)rel_x * sens_num / sens_den;
		float my = (float)rel_y * sens_num / sens_den;

#if CONFIG_ZMK_TPS43_INPUT_INVERT_X
		mx = -mx;
#endif
#if CONFIG_ZMK_TPS43_INPUT_INVERT_Y
		my = -my;
#endif

		input_report(dev, INPUT_EV_REL, INPUT_REL_X, (int)mx, false, K_NO_WAIT);
		input_report(dev, INPUT_EV_REL, INPUT_REL_Y, (int)my, true, K_NO_WAIT);

		printk("TP: rel=%d,%d abs=%u,%u fg=%u mvt=%d,%d\n",
		       rel_x, rel_y, abs_x, abs_y, finger_count, (int)mx, (int)my);
	}

	k_work_schedule(&data->work, K_MSEC(cfg->interval_ms));
}

static int tps43_input_init(const struct device *dev)
{
	const struct tps43_input_config *cfg = dev->config;
	struct tps43_input_data *data = dev->data;

	data->dev = dev;
	data->found = false;
	data->prev_button_down = false;

	if (!device_is_ready(cfg->i2c_bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	LOG_INF("Probing TPS43 at 0x%02x...", cfg->i2c_addr);
	if (!tps43_probe(cfg->i2c_bus, cfg->i2c_addr)) {
		LOG_WRN("TPS43 not found at 0x%02x, will retry", cfg->i2c_addr);
	} else {
		data->found = true;
		LOG_INF("TPS43 found at 0x%02x", cfg->i2c_addr);

		if (tps43_write_config(cfg->i2c_bus, cfg->i2c_addr,
		                       TPS43_CFG_SF_GESTURE, 0x00) == 0) {
			LOG_INF("TPS43 gesture engine disabled");
		} else {
			LOG_WRN("Failed to disable gesture engine");
		}
	}

#if CONFIG_ZMK_TPS43_INPUT_TAPDRAG
	tps43_tapdrag_init();
#endif
#if CONFIG_ZMK_TPS43_INPUT_EDGESCROLL
	tps43_edgescroll_init(CONFIG_ZMK_TPS43_INPUT_ABS_MAX_X,
	                      CONFIG_ZMK_TPS43_INPUT_ABS_MAX_Y);
#endif

	k_work_init_delayable(&data->work, tps43_poll_handler);
	k_work_schedule(&data->work, K_MSEC(200));

	return 0;
}

#define TPS43_INIT(n) \
	static const struct tps43_input_config tps43_input_config_##n = { \
		.i2c_bus = DEVICE_DT_GET(DT_INST_BUS(n)), \
		.i2c_addr = DT_INST_REG_ADDR(n), \
		.interval_ms = DT_INST_PROP_OR(n, update_interval_ms, \
		                                CONFIG_ZMK_TPS43_INPUT_INTERVAL_MS), \
	}; \
	static struct tps43_input_data tps43_input_data_##n; \
	DEVICE_DT_INST_DEFINE(n, tps43_input_init, NULL, \
	                      &tps43_input_data_##n, \
	                      &tps43_input_config_##n, \
	                      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, \
	                      NULL);

DT_INST_FOREACH_STATUS_OKAY(TPS43_INIT)
