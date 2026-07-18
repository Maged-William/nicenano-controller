#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

/* ================================================================
 * Constants
 * ================================================================ */

/* --- ADS1015 ADC (I2C) --- */
#define ADC_ADDR        0x48
#define CONV_REG        0x00
#define CONFIG_REG      0x01

/* --- BMI160 SPI --- */
#define CS1_PIN         31   /* high-range sensor: gyro ±500°/s */
#define CS2_PIN         29   /* low-range  sensor: gyro ±125°/s */

#define BMI160_CHIPID      0x00
#define BMI160_PMU_STATUS  0x03
#define BMI160_DATA_8      0x0C
#define BMI160_ACCEL_CONF  0x40
#define BMI160_ACCEL_RANGE 0x41
#define BMI160_GYRO_CONF   0x42
#define BMI160_GYRO_RANGE  0x43
#define BMI160_CMD         0x7E

#define GYRO_RANGE_125  0x04
#define GYRO_RANGE_500  0x02
#define ACCEL_RANGE_2G  0x03
#define ODR_1600HZ      0x0C

/* --- Burst averaging --- */
#define BURST_TOTAL  128
#define BURST_HIGH   16
#define BURST_LOW    112

/* --- Loop timing: 250 Hz → 4 ms ticks --- */
#define TICK_PERIOD_MS  4
#define ADC_DECIMATION  25   /* read ADC every 25 ticks (10 Hz) */
#define LED_DECIMATION  100  /* toggle LED every 100 ticks (2.5 Hz) */

/* --- Sensitivity (empirical, will tune) --- */
#define GYRO_SENS  0.001f

/* --- HSSNF parameters --- */
#define HSSNF_T  1.0f
#define HSSNF_K  0.5f

#define MOUSE_REPORT_SIZE  4

/* ================================================================
 * Global state
 * ================================================================ */

static const struct device *i2c_dev;
static const struct device *spi_dev;
static const struct device *hid_dev;
static const struct device *gpio0;

static const struct spi_config spi_cfg = {
	.frequency = 8000000,
	.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
	.slave = 0,
};

static float mouse_acc_x;
static float mouse_acc_y;

/* Gyro calibration offsets (subtracted from raw readings) */
static float cal_s1x, cal_s1y, cal_s1z;
static float cal_s2x, cal_s2y, cal_s2z;

/* ================================================================
 * Math helpers
 * ================================================================ */

static inline float fast_fabs(float x) { return x < 0.0f ? -x : x; }

static float ramp_mid(float x, float z)
{
	if (x < z) return 0.0f;
	if (x > (1.0f - z)) return 1.0f;
	return (x - z) / (1.0f - 2.0f * z);
}

static float hssnf(float t, float k, float x)
{
	float a = x - (x * k);
	float b = 1.0f - (x * k * (1.0f / t));
	return a / b;
}

static float apply_hssnf(float x)
{
	if (x > 0.0f && x < HSSNF_T)
		return hssnf(HSSNF_T, HSSNF_K, x);
	if (x < 0.0f && x > -HSSNF_T)
		return -hssnf(HSSNF_T, HSSNF_K, -x);
	return x;
}

/* ================================================================
 * ADS1015 ADC (I2C)
 * ================================================================ */

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

/* ================================================================
 * BMI160 SPI
 * ================================================================ */

static void bmi160_write_reg(gpio_pin_t cs, uint8_t reg, uint8_t val)
{
	uint8_t tx_data[2] = { reg & 0x7F, val };
	const struct spi_buf tx_buf = { .buf = tx_data, .len = 2 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

	gpio_pin_set(gpio0, cs, 0);
	spi_write(spi_dev, &spi_cfg, &tx);
	gpio_pin_set(gpio0, cs, 1);
}

static uint8_t bmi160_read_reg(gpio_pin_t cs, uint8_t reg)
{
	uint8_t tx_data[2] = { reg | 0x80, 0 };
	uint8_t rx_data[2] = { 0 };
	const struct spi_buf tx_buf = { .buf = tx_data, .len = 2 };
	const struct spi_buf rx_buf = { .buf = rx_data, .len = 2 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	gpio_pin_set(gpio0, cs, 0);
	spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
	gpio_pin_set(gpio0, cs, 1);

	return rx_data[1];
}

static bool bmi160_init(gpio_pin_t cs, uint8_t gyro_range)
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

	bmi160_write_reg(cs, BMI160_ACCEL_CONF, ODR_1600HZ);
	bmi160_write_reg(cs, BMI160_GYRO_CONF, ODR_1600HZ);
	bmi160_write_reg(cs, BMI160_ACCEL_RANGE, ACCEL_RANGE_2G);
	bmi160_write_reg(cs, BMI160_GYRO_RANGE, gyro_range);

	return bmi160_read_reg(cs, BMI160_CHIPID) == 0xD1;
}

/* Read gyro X/Y/Z only (6 bytes, registers 0x0C–0x11) via burst */
static void bmi160_read_gyro(gpio_pin_t cs, float *gx, float *gy, float *gz,
			     float ox, float oy, float oz)
{
	uint8_t tx_data[7] = { BMI160_DATA_8 | 0x80 };
	uint8_t rx_data[7] = { 0 };
	const struct spi_buf tx_buf = { .buf = tx_data, .len = 7 };
	const struct spi_buf rx_buf = { .buf = rx_data, .len = 7 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	gpio_pin_set(gpio0, cs, 0);
	spi_transceive(spi_dev, &spi_cfg, &tx, &rx);
	gpio_pin_set(gpio0, cs, 1);

	int16_t x = (int16_t)(rx_data[2] << 8 | rx_data[1]);
	int16_t y = (int16_t)(rx_data[4] << 8 | rx_data[3]);
	int16_t z = (int16_t)(rx_data[6] << 8 | rx_data[5]);

	*gx = (float)x - ox;
	*gy = (float)y - oy;
	*gz = (float)z - oz;
}

/* ================================================================
 * Gyro fusion: burst averaging + saturation-weighted crossfade
 * ================================================================ */

static void read_fuse_gyro(float *fx, float *fy, float *fz)
{
	float gx_h = 0.0f, gy_h = 0.0f, gz_h = 0.0f;
	float gx_l = 0.0f, gy_l = 0.0f, gz_l = 0.0f;
	float tmp_x, tmp_y, tmp_z;

	for (int i = 0; i < BURST_HIGH; i++) {
		bmi160_read_gyro(CS1_PIN, &tmp_x, &tmp_y, &tmp_z,
				 cal_s1x, cal_s1y, cal_s1z);
		gx_h += tmp_x;
		gy_h += tmp_y;
		gz_h += tmp_z;
	}
	gx_h /= (float)BURST_HIGH;
	gy_h /= (float)BURST_HIGH;
	gz_h /= (float)BURST_HIGH;

	for (int i = 0; i < BURST_LOW; i++) {
		bmi160_read_gyro(CS2_PIN, &tmp_x, &tmp_y, &tmp_z,
				 cal_s2x, cal_s2y, cal_s2z);
		gx_l += tmp_x;
		gy_l += tmp_y;
		gz_l += tmp_z;
	}
	gx_l /= (float)BURST_LOW;
	gy_l /= (float)BURST_LOW;
	gz_l /= (float)BURST_LOW;

	float sat = (fast_fabs(gx_l) > fast_fabs(gy_l)) ? fast_fabs(gx_l) : fast_fabs(gy_l);
	sat /= 32768.0f;

	float w_high = ramp_mid(sat, 0.2f);
	float w_low  = 1.0f - w_high;

	*fx = gx_h * w_high + (gx_l / 4.0f) * w_low;
	*fy = gy_h * w_high + (gy_l / 4.0f) * w_low;
	*fz = gz_h * w_high + (gz_l / 4.0f) * w_low;
}

/* ================================================================
 * Gyro calibration (stationary offset measurement)
 * ================================================================ */

#define CAL_SAMPLES  500

static void calibrate_gyro(void)
{
	float sx1 = 0, sy1 = 0, sz1 = 0;
	float sx2 = 0, sy2 = 0, sz2 = 0;
	float tx, ty, tz;

	printk("Calibrating gyro (hold still)... ");
	for (int i = 0; i < CAL_SAMPLES; i++) {
		bmi160_read_gyro(CS1_PIN, &tx, &ty, &tz, 0, 0, 0);
		sx1 += tx; sy1 += ty; sz1 += tz;
		bmi160_read_gyro(CS2_PIN, &tx, &ty, &tz, 0, 0, 0);
		sx2 += tx; sy2 += ty; sz2 += tz;
		k_busy_wait(4000);
	}

	cal_s1x = sx1 / (float)CAL_SAMPLES;
	cal_s1y = sy1 / (float)CAL_SAMPLES;
	cal_s1z = sz1 / (float)CAL_SAMPLES;
	cal_s2x = sx2 / (float)CAL_SAMPLES;
	cal_s2y = sy2 / (float)CAL_SAMPLES;
	cal_s2z = sz2 / (float)CAL_SAMPLES;

	printk("done\n");
}

/* ================================================================
 * HID Mouse
 * ================================================================ */

static const uint8_t hid_report_desc[] = {
	0x05, 0x01,        /* Usage Page (Generic Desktop) */
	0x09, 0x02,        /* Usage (Mouse) */
	0xA1, 0x01,        /* Collection (Application) */
	0x09, 0x01,        /*   Usage (Pointer) */
	0xA1, 0x00,        /*   Collection (Physical) */
	0x05, 0x09,        /*     Usage Page (Button) */
	0x19, 0x01,        /*     Usage Minimum (1) */
	0x29, 0x03,        /*     Usage Maximum (3) */
	0x15, 0x00,        /*     Logical Minimum (0) */
	0x25, 0x01,        /*     Logical Maximum (1) */
	0x95, 0x03,        /*     Report Count (3) */
	0x75, 0x01,        /*     Report Size (1) */
	0x81, 0x02,        /*     Input (Data,Var,Abs) */
	0x95, 0x01,        /*     Report Count (1) */
	0x75, 0x05,        /*     Report Size (5) */
	0x81, 0x03,        /*     Input (Const,Var,Abs) */
	0x05, 0x01,        /*     Usage Page (Generic Desktop) */
	0x09, 0x30,        /*     Usage (X) */
	0x09, 0x31,        /*     Usage (Y) */
	0x16, 0x00, 0x80,  /*     Logical Minimum (-128) */
	0x26, 0xFF, 0x7F,  /*     Logical Maximum (127) */
	0x75, 0x08,        /*     Report Size (8) */
	0x95, 0x02,        /*     Report Count (2) */
	0x81, 0x06,        /*     Input (Data,Var,Rel) */
	0x09, 0x38,        /*     Usage (Wheel) */
	0x15, 0x81,        /*     Logical Minimum (-127) */
	0x25, 0x7F,        /*     Logical Maximum (127) */
	0x75, 0x08,        /*     Report Size (8) */
	0x95, 0x01,        /*     Report Count (1) */
	0x81, 0x06,        /*     Input (Data,Var,Rel) */
	0xC0,              /*   End Collection */
	0xC0               /* End Collection */
};

static void send_mouse_move(int8_t dx, int8_t dy)
{
	uint8_t report[4] = { 0, (uint8_t)dx, (uint8_t)dy, 0 };
	hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
}

/* ================================================================
 * Main
 * ================================================================ */

int main(void)
{
	const struct device *cdc = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
	uint32_t dtr = 0;

	gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	gpio_pin_configure(gpio0, 15, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(gpio0, CS1_PIN, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(gpio0, CS2_PIN, GPIO_OUTPUT_ACTIVE);

	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (!device_is_ready(i2c_dev)) {
		while (1)
			;
	}
	i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_FAST));

	spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
	if (!device_is_ready(spi_dev)) {
		while (1)
			;
	}

	hid_dev = device_get_binding("HID_0");
	if (!hid_dev) {
		while (1)
			;
	}

	usb_hid_register_device(hid_dev, hid_report_desc, sizeof(hid_report_desc), NULL);
	usb_hid_init(hid_dev);

	usb_enable(NULL);

	while (!dtr) {
		uart_line_ctrl_get(cdc, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}

	k_sleep(K_SECONDS(1));

	int adc_addr = scan_adc();
	if (!adc_addr) {
		printk("No ADC found\n");
	} else {
		printk("ADC at 0x%02X\n", adc_addr);

		uint16_t sum_lsb = 0;
		for (int i = 0; i < 10; i++) {
			int16_t v = adc_read_channel(3);
			sum_lsb += v & 0x0F;
			k_sleep(K_MSEC(20));
		}
		printk(sum_lsb == 0 ? "ADS1015 (12-bit)\n" : "ADS1115 (16-bit)\n");
	}

	bool bmi1 = bmi160_init(CS1_PIN, GYRO_RANGE_500);
	bool bmi2 = bmi160_init(CS2_PIN, GYRO_RANGE_125);
	printk("BMI160 S1(500dps)=%d S2(125dps)=%d\n", bmi1, bmi2);

	calibrate_gyro();

	printk("Exp10: Dual-gyro HID mouse running at 250Hz\n");
	printk("tick\tFX\tFY\tFZ\tCH0\tCH1\tCH2\tCH3\n");

	int64_t next_tick;
	int tick_count = 0;

	next_tick = k_uptime_get();

	while (1) {
		next_tick += TICK_PERIOD_MS;
		tick_count++;

		float fx, fy, fz;
		read_fuse_gyro(&fx, &fy, &fz);

		mouse_acc_x += apply_hssnf(fx * GYRO_SENS);
		mouse_acc_y += apply_hssnf(fy * GYRO_SENS);

		if (fast_fabs(mouse_acc_x) < 0.5f) mouse_acc_x = 0.0f;
		if (fast_fabs(mouse_acc_y) < 0.5f) mouse_acc_y = 0.0f;

		int dx = (int)mouse_acc_x;
		int dy = (int)mouse_acc_y;
		mouse_acc_x -= (float)dx;
		mouse_acc_y -= (float)dy;

		if (dx > 127) dx = 127;
		if (dx < -128) dx = -128;
		if (dy > 127) dy = 127;
		if (dy < -128) dy = -128;

		send_mouse_move((int8_t)dx, (int8_t)dy);

		if (tick_count % ADC_DECIMATION == 0) {
			int16_t ch0 = adc_read_channel(0);
			int16_t ch1 = adc_read_channel(1);
			int16_t ch2 = adc_read_channel(2);
			int16_t ch3 = adc_read_channel(3);
			printk("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
			       tick_count, (int)fx, (int)fy, (int)fz,
			       ch0, ch1, ch2, ch3);
		}

		if (tick_count % LED_DECIMATION == 0) {
			gpio_pin_toggle(gpio0, 15);
		}

		int64_t now = k_uptime_get();
		if (next_tick > now) {
			k_sleep(K_MSEC(next_tick - now));
		}
	}

	return 0;
}
