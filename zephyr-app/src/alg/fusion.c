#include "fusion.h"
#include "alg/filter.h"
#include "alg/calibrate.h"
#include "drivers/bmi160.h"

#define BURST_HIGH  CONFIG_GYRO_MOUSE_BURST_HIGH
#define BURST_LOW   CONFIG_GYRO_MOUSE_BURST_LOW

#define CROSSFADE_Z  (CONFIG_GYRO_MOUSE_CROSSFADE_Z / 1000.0f)

void read_fuse_gyro(float *fx, float *fy, float *fz)
{
	float gx_h = 0.0f, gy_h = 0.0f, gz_h = 0.0f;
	float gx_l = 0.0f, gy_l = 0.0f, gz_l = 0.0f;
	float tmp_x, tmp_y, tmp_z;

	for (int i = 0; i < BURST_HIGH; i++) {
		bmi160_read_gyro(CONFIG_GYRO_MOUSE_CS1_PIN, &tmp_x, &tmp_y, &tmp_z,
				 cal_s1x, cal_s1y, cal_s1z);
		gx_h += tmp_x;
		gy_h += tmp_y;
		gz_h += tmp_z;
	}
	gx_h /= (float)BURST_HIGH;
	gy_h /= (float)BURST_HIGH;
	gz_h /= (float)BURST_HIGH;

	for (int i = 0; i < BURST_LOW; i++) {
		bmi160_read_gyro(CONFIG_GYRO_MOUSE_CS2_PIN, &tmp_x, &tmp_y, &tmp_z,
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

	float w_high = ramp_mid(sat, CROSSFADE_Z);
	float w_low  = 1.0f - w_high;

	*fx = gx_h * w_high + (gx_l / 4.0f) * w_low;
	*fy = gy_h * w_high + (gy_l / 4.0f) * w_low;
	*fz = gz_h * w_high + (gz_l / 4.0f) * w_low;
}
