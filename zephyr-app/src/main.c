#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>
#include "alg/filter.h"
#include "hid/mouse.h"
#include "drivers/ads1015.h"
#include "drivers/bmi160.h"
#include "drivers/tps43.h"
#include "drivers/tps43_tapdrag.h"
#include "alg/calibrate.h"
#include "alg/fusion.h"

/* ================================================================
 * Constants
 * ================================================================ */

/* --- CS pins for BMI160 --- */
#define CS1_PIN  CONFIG_GYRO_MOUSE_CS1_PIN
#define CS2_PIN  CONFIG_GYRO_MOUSE_CS2_PIN

/* --- Loop timing --- */
#define TICK_PERIOD_MS  CONFIG_GYRO_MOUSE_TICK_PERIOD_MS
#define ADC_DECIMATION  CONFIG_GYRO_MOUSE_ADC_DECIMATION
#define LED_DECIMATION  CONFIG_GYRO_MOUSE_LED_DECIMATION

/* --- Sensitivity --- */
#define GYRO_SENS  ((float)CONFIG_GYRO_MOUSE_SENSITIVITY_NUM / (float)CONFIG_GYRO_MOUSE_SENSITIVITY_DENOM)

/* --- Deadzone (scaled by 1000 in Kconfig) --- */
#define DEADZONE_THR  (CONFIG_GYRO_MOUSE_DEADZONE / 1000.0f)

/* ================================================================
 * Global state
 * ================================================================ */

static const struct device *i2c_dev;

/* ================================================================
 * Application constants
 * ================================================================ */

#if CONFIG_TPS43_ENABLE
#define TPS43_SENS  ((float)CONFIG_TPS43_SENSITIVITY_NUM / (float)CONFIG_TPS43_SENSITIVITY_DENOM)
#define TPS43_SCROLL_SENS  ((float)CONFIG_TPS43_SCROLL_SENS_NUM / (float)CONFIG_TPS43_SCROLL_SENS_DENOM)

static bool tps43_left_btn;
static bool tps43_left_btn_prev;
static int16_t tps43_dbg_dx;
static int16_t tps43_dbg_dy;
static uint8_t tps43_dbg_fingers;
static uint8_t tps43_prev_g0;
static uint8_t tps43_prev_g1;
static uint8_t tps43_prev_fingers;
#endif

/* ================================================================
 * Main
 * ================================================================ */

int main(void)
{
	const struct device *cdc = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
	uint32_t dtr = 0;

	const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	gpio_pin_configure(gpio0, 15, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(gpio0, CS1_PIN, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure(gpio0, CS2_PIN, GPIO_OUTPUT_ACTIVE);

	i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	if (!device_is_ready(i2c_dev)) {
		while (1)
			;
	}
	i2c_configure(i2c_dev, I2C_SPEED_SET(I2C_SPEED_FAST));
	ads1015_init(i2c_dev);

	const struct device *spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
	if (!device_is_ready(spi_dev)) {
		while (1)
			;
	}
	bmi160_bus_init(spi_dev, gpio0);

	const struct device *hid_dev = device_get_binding("HID_0");
	if (!hid_dev) {
		while (1)
			;
	}

	mouse_init(hid_dev);
	usb_enable(NULL);

#if CONFIG_GYRO_MOUSE_DTR_TIMEOUT_MS > 0
	int dtr_polls = CONFIG_GYRO_MOUSE_DTR_TIMEOUT_MS / 100;
	while (!dtr && dtr_polls-- > 0) {
		uart_line_ctrl_get(cdc, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}
#endif

	k_sleep(K_SECONDS(1));

	int adc_addr = ads1015_scan();
	if (!adc_addr) {
		printk("No ADC found\n");
	} else {
		printk("ADC at 0x%02X\n", adc_addr);

		uint16_t sum_lsb = 0;
		for (int i = 0; i < 10; i++) {
			int16_t v = ads1015_read_channel(3);
			sum_lsb += v & 0x0F;
			k_sleep(K_MSEC(20));
		}
		printk(sum_lsb == 0 ? "ADS1015 (12-bit)\n" : "ADS1115 (16-bit)\n");
	}

#if CONFIG_TPS43_ENABLE
	tps43_init(i2c_dev);
	printk("TPS43 touchpad: %s\n", tps43_found ? "found" : "not found");
#if CONFIG_TPS43_TAPDRAG_ENABLE
	tps43_tapdrag_init();
	printk("TPS43 tap-drag FSM: enabled\n");
#endif
#endif

	bool bmi1 = bmi160_sensor_init(CS1_PIN, S1_GYRO_RANGE_REG);
	bool bmi2 = bmi160_sensor_init(CS2_PIN, S2_GYRO_RANGE_REG);
	printk("BMI160 S1(%ddps)=%d S2(%ddps)=%d\n",
	       CONFIG_GYRO_MOUSE_S1_RANGE, bmi1,
	       CONFIG_GYRO_MOUSE_S2_RANGE, bmi2);

#if CONFIG_GYRO_MOUSE_HARDCODED_CAL
		cal_s1x = CONFIG_GYRO_MOUSE_CAL_S1X;
		cal_s1y = CONFIG_GYRO_MOUSE_CAL_S1Y;
		cal_s1z = CONFIG_GYRO_MOUSE_CAL_S1Z;
		cal_s2x = CONFIG_GYRO_MOUSE_CAL_S2X;
		cal_s2y = CONFIG_GYRO_MOUSE_CAL_S2Y;
		cal_s2z = CONFIG_GYRO_MOUSE_CAL_S2Z;
		printk("Using hardcoded gyro calibration\n");
#elif CONFIG_GYRO_MOUSE_CALIBRATE_ON_BOOT
		calibrate_gyro();
#else
		printk("Gyro calibration disabled, using zero offsets\n");
#endif

	printk("Exp12: Dual-gyro HID mouse + TPS43 tap-drag touchpad at 250Hz\n");
	printk("tick\tFX\tFY\tFZ\tCH0\tCH1\tCH2\tCH3\tTP_X\tTP_Y\tTP_F\n");

	int64_t next_tick;
	int tick_count = 0;

	next_tick = k_uptime_get();

	while (1) {
		next_tick += TICK_PERIOD_MS;
		tick_count++;

		float fx = 0, fy = 0, fz = 0;

#if CONFIG_GYRO_MOUSE_ENABLE
		read_fuse_gyro(&fx, &fy, &fz);

		float gyro_v[3] = { fx, fy, fz };
		float mx = gyro_v[CONFIG_GYRO_MOUSE_X_SOURCE];
		float my = gyro_v[CONFIG_GYRO_MOUSE_Y_SOURCE];
#if CONFIG_GYRO_MOUSE_X_INVERT
		mx = -mx;
#endif
#if CONFIG_GYRO_MOUSE_Y_INVERT
		my = -my;
#endif

		mouse_acc_x += apply_hssnf(mx * GYRO_SENS);
		mouse_acc_y += apply_hssnf(my * GYRO_SENS);

		if (DEADZONE_THR > 0.0f) {
			if (fast_fabs(mouse_acc_x) < DEADZONE_THR) mouse_acc_x = 0.0f;
			if (fast_fabs(mouse_acc_y) < DEADZONE_THR) mouse_acc_y = 0.0f;
		}
#endif

#if CONFIG_TPS43_ENABLE
		{
			int16_t tdx = 0, tdy = 0;
			bool tap = false;
			bool touched = tps43_poll(&tdx, &tdy, &tap);
			tps43_dbg_dx = tdx;
			tps43_dbg_dy = tdy;
			uint8_t g0 = tps43_regs[0];
			uint8_t g1 = tps43_regs[1];
			uint8_t fingers = tps43_regs[TPS43_FINGER_COUNT - TPS43_GESTURE0];
			tps43_dbg_fingers = touched ? fingers : 0;

			if (g0 != tps43_prev_g0 || g1 != tps43_prev_g1 || fingers != tps43_prev_fingers) {
				printk("GST g0=0x%02X g1=0x%02X f=%d\n", g0, g1, fingers);
				tps43_prev_g0 = g0;
				tps43_prev_g1 = g1;
				tps43_prev_fingers = fingers;
			}

			if (touched) {
				if (fingers >= 2) {
					mouse_wheel   += (int)((float)tdy * TPS43_SCROLL_SENS);
					mouse_wheel_h += (int)((float)tdx * TPS43_SCROLL_SENS);
				} else {
					float mv_x = (float)tdx * TPS43_SENS;
					float mv_y = (float)tdy * TPS43_SENS;
#if CONFIG_TPS43_INVERT_X
					mv_x = -mv_x;
#endif
#if CONFIG_TPS43_INVERT_Y
					mv_y = -mv_y;
#endif
					mouse_acc_x += mv_x;
					mouse_acc_y += mv_y;
				}
			}
#if CONFIG_TPS43_TAPDRAG_ENABLE
			{
				uint16_t abs_x = ((uint16_t)tps43_regs[TPS43_XABS_HIGH - TPS43_GESTURE0] << 8) |
				                  tps43_regs[TPS43_XABS_LOW  - TPS43_GESTURE0];
				uint16_t abs_y = ((uint16_t)tps43_regs[TPS43_YABS_HIGH - TPS43_GESTURE0] << 8) |
				                  tps43_regs[TPS43_YABS_LOW  - TPS43_GESTURE0];
				tps43_left_btn = tps43_tapdrag_update(touched, abs_x, abs_y, k_uptime_get());
				if (tps43_left_btn != tps43_left_btn_prev) {
					if (tps43_left_btn) {
						mouse_buttons |= 1;
					} else {
						mouse_buttons &= ~1;
					}
					tps43_left_btn_prev = tps43_left_btn;
					printk("BTN: %s @%llu\n", tps43_left_btn ? "DOWN" : "UP", k_uptime_get());
				}
			}
#elif CONFIG_TPS43_TAP_ENABLE
			tps43_left_btn = tap;
			if (tps43_left_btn != tps43_left_btn_prev) {
				if (tps43_left_btn) {
					mouse_buttons |= 1;
				} else {
					mouse_buttons &= ~1;
				}
				tps43_left_btn_prev = tps43_left_btn;
			}
#endif
		}
#endif

		int dx = (int)mouse_acc_x;
		int dy = (int)mouse_acc_y;
		mouse_acc_x -= (float)dx;
		mouse_acc_y -= (float)dy;

		if (dx > 127) dx = 127;
		if (dx < -128) dx = -128;
		if (dy > 127) dy = 127;
		if (dy < -128) dy = -128;

		int w = mouse_wheel;
		if (w > 127) w = 127;
		if (w < -128) w = -128;
		mouse_wheel -= w;

		int wh = mouse_wheel_h;
		if (wh > 127) wh = 127;
		if (wh < -128) wh = -128;
		mouse_wheel_h -= wh;

		send_mouse_report((int8_t)dx, (int8_t)dy, (int8_t)w, (int8_t)wh);

		if (tick_count % ADC_DECIMATION == 0) {
			int16_t ch0 = ads1015_read_channel(0);
			int16_t ch1 = ads1015_read_channel(1);
			int16_t ch2 = ads1015_read_channel(2);
			int16_t ch3 = ads1015_read_channel(3);
			printk("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
			       tick_count, (int)fx, (int)fy, (int)fz,
			       ch0, ch1, ch2, ch3,
#if CONFIG_TPS43_ENABLE
			       tps43_dbg_dx, tps43_dbg_dy, tps43_dbg_fingers,
			       mouse_buttons, dx, dy
#else
			       0, 0, 0, 0, 0, 0
#endif
			       );
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
