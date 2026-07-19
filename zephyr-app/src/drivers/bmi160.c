#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include "bmi160.h"

static const struct device *spi_dev;
static const struct device *gpio_dev;

static const struct spi_config spi_cfg = {
	.frequency = CONFIG_GYRO_MOUSE_SPI_FREQ,
	.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
	.slave = 0,
};

void bmi160_bus_init(const struct device *spi, const struct device *gpio)
{
	spi_dev = spi;
	gpio_dev = gpio;
}

static void bmi160_write_reg(gpio_pin_t cs, uint8_t reg, uint8_t val)
{
	uint8_t tx_data[2] = { reg & 0x7F, val };
	const struct spi_buf tx_buf = { .buf = tx_data, .len = 2 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	gpio_pin_set(gpio_dev, cs, 0);
	spi_write(spi_dev, &spi_cfg, &tx);
	gpio_pin_set(gpio_dev, cs, 1);
}

static uint8_t bmi160_read_reg(gpio_pin_t cs, uint8_t reg)
{
	uint8_t tx_data[2] = { reg | 0x80, 0 };
	uint8_t rx_data[2] = { 0 };
	const struct spi_buf tx_buf = { .buf = tx_data, .len = 2 };
	const struct spi_buf rx_buf = { .buf = rx_data, .len = 2 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	gpio_pin_set(gpio_dev, cs, 0);
	spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
	gpio_pin_set(gpio_dev, cs, 1);

	return rx_data[1];
}

bool bmi160_sensor_init(gpio_pin_t cs, uint8_t gyro_range)
{
	bmi160_write_reg(cs, BMI160_CMD, 0xB6);
	k_sleep(K_MSEC(10));

	bmi160_read_reg(cs, 0x7F);
	k_sleep(K_MSEC(10));

	bmi160_write_reg(cs, BMI160_CMD, 0x11);
	k_sleep(K_MSEC(5));
	int timeout = 100;
	while ((bmi160_read_reg(cs, BMI160_PMU_STATUS) & 0x30) != 0x10 && timeout--) {
		k_sleep(K_MSEC(1));
	}
	if (timeout <= 0) return false;

	bmi160_write_reg(cs, BMI160_CMD, 0x15);
	k_sleep(K_MSEC(5));
	timeout = 500;
	while ((bmi160_read_reg(cs, BMI160_PMU_STATUS) & 0x0C) != 0x04 && timeout--) {
		k_sleep(K_MSEC(1));
	}
	if (timeout <= 0) return false;

	bmi160_write_reg(cs, BMI160_ACCEL_CONF, ODR_REG);
	bmi160_write_reg(cs, BMI160_GYRO_CONF, ODR_REG);
	bmi160_write_reg(cs, BMI160_ACCEL_RANGE, ACCEL_RANGE_2G);
	bmi160_write_reg(cs, BMI160_GYRO_RANGE, gyro_range);

	return bmi160_read_reg(cs, BMI160_CHIPID) == 0xD1;
}

void bmi160_read_gyro(gpio_pin_t cs, float *gx, float *gy, float *gz,
		      float ox, float oy, float oz)
{
	uint8_t tx_data[7] = { BMI160_DATA_8 | 0x80 };
	uint8_t rx_data[7] = { 0 };
	const struct spi_buf tx_buf = { .buf = tx_data, .len = 7 };
	const struct spi_buf rx_buf = { .buf = rx_data, .len = 7 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	gpio_pin_set(gpio_dev, cs, 0);
	spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
	gpio_pin_set(gpio_dev, cs, 1);

	int16_t x = (int16_t)(rx_data[2] << 8 | rx_data[1]);
	int16_t y = (int16_t)(rx_data[4] << 8 | rx_data[3]);
	int16_t z = (int16_t)(rx_data[6] << 8 | rx_data[5]);

	*gx = (float)x - ox;
	*gy = (float)y - oy;
	*gz = (float)z - oz;
}
