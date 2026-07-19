#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include "tps43.h"

#if CONFIG_TPS43_ENABLE

static const struct device *i2c_dev;

static uint8_t tps43_prev_g0;
static uint8_t tps43_prev_g1;
static uint8_t tps43_prev_fingers;

bool tps43_found;
uint8_t tps43_regs[16];

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

static int tps43_read_block(uint8_t *buf)
{
	uint8_t reg_ptr[2] = { 0x00, TPS43_GESTURE0 };
	return i2c_write_read(i2c_dev, TPS43_ADDR, reg_ptr, 2, buf, 16);
}

static int tps43_end_comm(void)
{
	uint8_t end_cmd[3] = { 0xEE, 0xEE, 0x00 };
	return i2c_write(i2c_dev, end_cmd, 3, TPS43_ADDR);
}

bool tps43_poll(int16_t *dx, int16_t *dy, bool *tap)
{
	if (tps43_read_block(tps43_regs) != 0) {
		tps43_end_comm();
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
