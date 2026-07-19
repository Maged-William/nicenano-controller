#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "calibrate.h"
#include "drivers/bmi160.h"

float cal_s1x, cal_s1y, cal_s1z;
float cal_s2x, cal_s2y, cal_s2z;

void calibrate_gyro(void)
{
	float sx1 = 0, sy1 = 0, sz1 = 0;
	float sx2 = 0, sy2 = 0, sz2 = 0;
	float tx, ty, tz;

	printk("Calibrating gyro (hold still)... ");
	for (int i = 0; i < CONFIG_GYRO_MOUSE_CAL_SAMPLES; i++) {
		bmi160_read_gyro(CONFIG_GYRO_MOUSE_CS1_PIN, &tx, &ty, &tz, 0, 0, 0);
		sx1 += tx; sy1 += ty; sz1 += tz;
		bmi160_read_gyro(CONFIG_GYRO_MOUSE_CS2_PIN, &tx, &ty, &tz, 0, 0, 0);
		sx2 += tx; sy2 += ty; sz2 += tz;
		k_busy_wait(4000);
	}

	cal_s1x = sx1 / (float)CONFIG_GYRO_MOUSE_CAL_SAMPLES;
	cal_s1y = sy1 / (float)CONFIG_GYRO_MOUSE_CAL_SAMPLES;
	cal_s1z = sz1 / (float)CONFIG_GYRO_MOUSE_CAL_SAMPLES;
	cal_s2x = sx2 / (float)CONFIG_GYRO_MOUSE_CAL_SAMPLES;
	cal_s2y = sy2 / (float)CONFIG_GYRO_MOUSE_CAL_SAMPLES;
	cal_s2z = sz2 / (float)CONFIG_GYRO_MOUSE_CAL_SAMPLES;

	printk("done\n");
}
