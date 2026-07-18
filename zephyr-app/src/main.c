#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <stdbool.h>

/* === ADS1015 ADC (I2C) === */
#define ADC_ADDR    0x48
#define CONV_REG    0x00
#define CONFIG_REG  0x01

static const struct device *i2c_dev;

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

static int16_t adc_read_channel(uint8_t ch)
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

static int scan_adc(void)
{
	for (uint8_t addr = 0x48; addr <= 0x4B; addr++) {
		if (i2c_write(i2c_dev, NULL, 0, addr) == 0) {
			return addr;
		}
	}
	return 0;
}

/* === BMI160 IMU (SPI) === */
#define CS1_PIN 31
#define CS2_PIN 29

#define BMI160_CHIPID     0x00
#define BMI160_PMU_STATUS 0x03
#define BMI160_DATA_8     0x0C
#define BMI160_ACCEL_CONF 0x41
#define BMI160_GYRO_CONF  0x43
#define BMI160_CMD        0x7E

static const struct device *spi_dev;

static const struct spi_config spi_cfg = {
	.frequency = 1000000,
	.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
	.slave = 0,
};

static void bmi160_write_reg(const struct device *gpio, gpio_pin_t cs,
			     uint8_t reg, uint8_t val)
{
	uint8_t tx_data[2] = { reg & 0x7F, val };
	const struct spi_buf tx_buf = { .buf = tx_data, .len = 2 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	gpio_pin_set(gpio, cs, 0);
	spi_write(spi_dev, &spi_cfg, &tx);
	gpio_pin_set(gpio, cs, 1);
}

static uint8_t bmi160_read_reg(const struct device *gpio, gpio_pin_t cs,
			       uint8_t reg)
{
	uint8_t tx_data[2] = { reg | 0x80, 0 };
	uint8_t rx_data[2] = { 0 };
	const struct spi_buf tx_buf = { .buf = tx_data, .len = 2 };
	const struct spi_buf rx_buf = { .buf = rx_data, .len = 2 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	gpio_pin_set(gpio, cs, 0);
	spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
	gpio_pin_set(gpio, cs, 1);

	return rx_data[1];
}

static bool bmi160_init(const struct device *gpio, gpio_pin_t cs)
{
	bmi160_write_reg(gpio, cs, BMI160_CMD, 0xB6);
	k_sleep(K_MSEC(10));

	bmi160_read_reg(gpio, cs, 0x7F);
	k_sleep(K_MSEC(10));

	bmi160_write_reg(gpio, cs, BMI160_CMD, 0x11);
	k_sleep(K_MSEC(5));
	int timeout = 100;
	while ((bmi160_read_reg(gpio, cs, BMI160_PMU_STATUS) & 0x30) != 0x10 && timeout--) {
		k_sleep(K_MSEC(1));
	}
	if (timeout <= 0) return false;

	bmi160_write_reg(gpio, cs, BMI160_CMD, 0x15);
	k_sleep(K_MSEC(5));
	timeout = 500;
	while ((bmi160_read_reg(gpio, cs, BMI160_PMU_STATUS) & 0x0C) != 0x04 && timeout--) {
		k_sleep(K_MSEC(1));
	}
	if (timeout <= 0) return false;

	bmi160_write_reg(gpio, cs, BMI160_ACCEL_CONF, 0x03);
	bmi160_write_reg(gpio, cs, BMI160_GYRO_CONF, 0x03);

	return bmi160_read_reg(gpio, cs, BMI160_CHIPID) == 0xD1;
}

static void bmi160_read_data(const struct device *gpio, gpio_pin_t cs,
			     int16_t *gx, int16_t *gy, int16_t *gz,
			     int16_t *ax, int16_t *ay, int16_t *az)
{
	uint8_t tx_data[13] = { BMI160_DATA_8 | 0x80 };
	uint8_t rx_data[13] = { 0 };
	const struct spi_buf tx_buf = { .buf = tx_data, .len = 13 };
	const struct spi_buf rx_buf = { .buf = rx_data, .len = 13 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	gpio_pin_set(gpio, cs, 0);
	spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
	gpio_pin_set(gpio, cs, 1);

	*gx = (int16_t)(rx_data[2] << 8 | rx_data[1]);
	*gy = (int16_t)(rx_data[4] << 8 | rx_data[3]);
	*gz = (int16_t)(rx_data[6] << 8 | rx_data[5]);
	*ax = (int16_t)(rx_data[8] << 8 | rx_data[7]);
	*ay = (int16_t)(rx_data[10] << 8 | rx_data[9]);
	*az = (int16_t)(rx_data[12] << 8 | rx_data[11]);
}

/* === Main === */
int main(void)
{
	const struct device *cdc = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
	const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	uint32_t dtr = 0;

	gpio_pin_configure(gpio0, 15, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(gpio0, CS1_PIN, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(gpio0, CS2_PIN, GPIO_OUTPUT_ACTIVE);

	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (!device_is_ready(i2c_dev)) {
		printk("I2C0 not ready\n");
		return 1;
	}

	spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
	if (!device_is_ready(spi_dev)) {
		printk("SPI1 not ready\n");
		return 1;
	}

	usb_enable(NULL);

	while (!dtr) {
		uart_line_ctrl_get(cdc, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}

	k_sleep(K_SECONDS(1));

	int adc_addr = scan_adc();
	if (!adc_addr) {
		printk("No ADC found\n");
		while (1) {
			gpio_pin_toggle(gpio0, 15);
			k_sleep(K_MSEC(200));
		}
	}
	printk("ADC found at 0x%02X\n", adc_addr);

	uint16_t sum_lsb = 0;
	for (int i = 0; i < 10; i++) {
		int16_t v = adc_read_channel(3);
		sum_lsb += v & 0x0F;
		k_sleep(K_MSEC(20));
	}
	if (sum_lsb == 0)
		printk("Device: ADS1015 (12-bit)\n");
	else
		printk("Device: ADS1115 (16-bit)\n");

	bool bmi1 = bmi160_init(gpio0, CS1_PIN);
	bool bmi2 = bmi160_init(gpio0, CS2_PIN);
	printk("BMI160 S1=%d S2=%d\n", bmi1, bmi2);

	printk("CH0\tCH1\tCH2\tCH3\t");
	printk("S1_gX\tS1_gY\tS1_gZ\tS1_aX\tS1_aY\tS1_aZ\t");
	printk("S2_gX\tS2_gY\tS2_gZ\tS2_aX\tS2_aY\tS2_aZ\n");

	int16_t ax1, ay1, az1, gx1, gy1, gz1;
	int16_t ax2, ay2, az2, gx2, gy2, gz2;

	while (1) {
		gpio_pin_toggle(gpio0, 15);

		for (int ch = 0; ch < 4; ch++) {
			int16_t v = adc_read_channel(ch);
			printk("%d\t", v);
		}

		bmi160_read_data(gpio0, CS1_PIN, &gx1, &gy1, &gz1, &ax1, &ay1, &az1);
		bmi160_read_data(gpio0, CS2_PIN, &gx2, &gy2, &gz2, &ax2, &ay2, &az2);

		printk("%d\t%d\t%d\t%d\t%d\t%d\t",
		       gx1, gy1, gz1, ax1, ay1, az1);
		printk("%d\t%d\t%d\t%d\t%d\t%d\n",
		       gx2, gy2, gz2, ax2, ay2, az2);

		k_sleep(K_MSEC(100));
	}

	return 0;
}
