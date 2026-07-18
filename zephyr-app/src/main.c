#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>

#define ADC_ADDR    0x48
#define CONV_REG    0x00
#define CONFIG_REG  0x01

static const struct device *i2c_dev;

static int write_reg(uint8_t reg, uint16_t val)
{
	uint8_t buf[3] = { reg, val >> 8, val & 0xFF };
	return i2c_write(i2c_dev, buf, sizeof(buf), ADC_ADDR);
}

static int read_reg(uint8_t reg, uint16_t *val)
{
	uint8_t rx[2];
	int ret = i2c_write_read(i2c_dev, ADC_ADDR, &reg, 1, rx, 2);
	if (ret == 0) {
		*val = (rx[0] << 8) | rx[1];
	}
	return ret;
}

static int16_t read_channel(uint8_t ch)
{
	uint16_t cfg = (1 << 15)
		     | ((0b100 | (ch & 3)) << 12)
		     | (0b001 << 9)
		     | (1 << 8)
		     | (0b100 << 5)
		     | 0b0000011;

	write_reg(CONFIG_REG, cfg);

	uint16_t status;
	int timeout = 100;
	do {
		if (read_reg(CONFIG_REG, &status) != 0) return 0;
		if (--timeout <= 0) return 0;
	} while (!(status & (1 << 15)));

	uint16_t raw;
	read_reg(CONV_REG, &raw);
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

void main(void)
{
	const struct device *cdc = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
	const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	uint32_t dtr = 0;

	gpio_pin_configure(gpio0, 15, GPIO_OUTPUT_ACTIVE);

	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (!device_is_ready(i2c_dev)) {
		printk("I2C0 not ready\n");
		return;
	}

	usb_enable(NULL);

	while (!dtr) {
		uart_line_ctrl_get(cdc, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}

	k_sleep(K_SECONDS(1));

	int addr = scan_adc();
	if (!addr) {
		printk("No ADC found\n");
		while (1) {
			gpio_pin_toggle(gpio0, 15);
			k_sleep(K_MSEC(200));
		}
	}
	printk("ADC found at 0x%02X\n", addr);

	uint16_t sum_lsb = 0;
	for (int i = 0; i < 10; i++) {
		int16_t v = read_channel(3);
		sum_lsb += v & 0x0F;
		k_sleep(K_MSEC(20));
	}
	if (sum_lsb == 0)
		printk("Device: ADS1015 (12-bit)\n");
	else
		printk("Device: ADS1115 (16-bit)\n");

	printk("CH0(JoyA-X)\tCH1(JoyA-Y)\tCH2(JoyB-X)\tCH3(JoyB-Y)\n");

	while (1) {
		gpio_pin_toggle(gpio0, 15);

		for (int ch = 0; ch < 4; ch++) {
			int16_t v = read_channel(ch);
			printk("%d\t", v);
		}
		printk("\n");

		k_sleep(K_MSEC(100));
	}
}
