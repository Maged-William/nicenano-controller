#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include "ads1015.h"

#define ADC_ADDR        0x48
#define CONV_REG        0x00
#define CONFIG_REG      0x01

static const struct device *i2c_dev;

void ads1015_init(const struct device *i2c)
{
	i2c_dev = i2c;
}

static int adc_write_reg(uint8_t reg, uint16_t val)
{
	uint8_t buf[3] = { reg, val >> 8, val & 0xFF };
	return i2c_write(i2c_dev, buf, sizeof(buf), ADC_ADDR);
}

static int adc_read_reg(uint8_t reg, uint16_t *val)
{
	uint8_t rx[2];
	int ret = i2c_write_read(i2c_dev, ADC_ADDR, &reg, 1, rx, 2);
	if (ret == 0) {
		*val = (rx[0] << 8) | rx[1];
	}
	return ret;
}

int ads1015_scan(void)
{
	for (uint8_t addr = 0x48; addr <= 0x4B; addr++) {
		if (i2c_write(i2c_dev, NULL, 0, addr) == 0) {
			return addr;
		}
	}
	return 0;
}

int16_t ads1015_read_channel(uint8_t ch)
{
	uint16_t cfg = (1 << 15)
		| ((0b100 | (ch & 3)) << 12)
		| (0b001 << 9)
		| (1 << 8)
		| (0b100 << 5)
		| 0b0000011;

	adc_write_reg(CONFIG_REG, cfg);

	uint16_t status;
	int timeout = 100;
	do {
		if (adc_read_reg(CONFIG_REG, &status) != 0) return 0;
		if (--timeout <= 0) return 0;
	} while (!(status & (1 << 15)));

	uint16_t raw;
	adc_read_reg(CONV_REG, &raw);
	return (int16_t)raw;
}
