#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include "tps43.h"

#if CONFIG_TPS43_ENABLE

static const struct device *i2c_dev;

bool tps43_found;
uint8_t tps43_regs[16];

/* ─── Low-level I2C helpers ─────────────────────── */

static int tps43_end_comm(void)
{
	uint8_t end_cmd[3] = { 0xEE, 0xEE, 0x00 };
	return i2c_write(i2c_dev, end_cmd, 3, TPS43_ADDR);
}

static int tps43_read_block(uint8_t *buf)
{
	uint8_t reg_ptr[2] = { 0x00, TPS43_GESTURE0 };
	return i2c_write_read(i2c_dev, TPS43_ADDR, reg_ptr, 2, buf, 16);
}

bool tps43_write_config(uint16_t reg, uint8_t val)
{
	uint8_t cmd[3] = { reg >> 8, reg & 0xFF, val };
	tps43_end_comm();
	int ret = i2c_write(i2c_dev, cmd, 3, TPS43_ADDR);
	tps43_end_comm();
	return ret == 0;
}

bool tps43_disable_gestures(void)
{
	if (!tps43_found)
		return false;
	if (!tps43_write_config(TPS43_CFG_SF_GESTURE, 0x00))
		return false;
	printk("TPS43 gesture engine disabled\n");
	return true;
}

bool tps43_init(const struct device *i2c)
{
	i2c_dev = i2c;

	uint8_t end_cmd[3] = { 0xEE, 0xEE, 0x00 };
	uint8_t reg_ptr[2] = { 0x00, TPS43_GESTURE0 };
	uint8_t test_buf[2];

	i2c_write(i2c_dev, end_cmd, 3, TPS43_ADDR);
	k_sleep(K_MSEC(100));

	tps43_found = false;
	for (int retry = 0; retry < 5; retry++) {
		if (i2c_write_read(i2c_dev, TPS43_ADDR, reg_ptr, 2, test_buf, 1) == 0) {
			tps43_found = true;
			break;
		}
		i2c_write(i2c_dev, end_cmd, 3, TPS43_ADDR);
		k_sleep(K_MSEC(200));
	}
	if (tps43_found) {
		i2c_write(i2c_dev, end_cmd, 3, TPS43_ADDR);
	}

	return tps43_found;
}

bool tps43_poll(int16_t *dx, int16_t *dy, bool *tap)
{
	if (!tps43_found) {
		uint8_t reg_ptr[2] = { 0x00, TPS43_GESTURE0 };
		uint8_t test_buf[1];
		tps43_end_comm();
		if (i2c_write_read(i2c_dev, TPS43_ADDR, reg_ptr, 2, test_buf, 1) == 0) {
			tps43_found = true;
			printk("TPS43 touchpad: found (late init)\n");
		}
		tps43_end_comm();
		*dx = 0;
		*dy = 0;
		*tap = false;
		return false;
	}

	if (tps43_read_block(tps43_regs) != 0) {
		tps43_end_comm();
		*dx = 0;
		*dy = 0;
		*tap = false;
		return false;
	}
	tps43_end_comm();

	*dx = (int16_t)((tps43_regs[TPS43_XREL_HIGH - TPS43_GESTURE0] << 8) |
	                tps43_regs[TPS43_XREL_LOW  - TPS43_GESTURE0]);
	*dy = (int16_t)((tps43_regs[TPS43_YREL_HIGH - TPS43_GESTURE0] << 8) |
	                tps43_regs[TPS43_YREL_LOW  - TPS43_GESTURE0]);
	*tap = (tps43_regs[0] & 0x01) != 0;

	return tps43_regs[TPS43_FINGER_COUNT - TPS43_GESTURE0] > 0;
}

#endif /* CONFIG_TPS43_ENABLE */
