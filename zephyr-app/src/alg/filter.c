#include "filter.h"

#define HSSNF_T  (CONFIG_GYRO_MOUSE_HSSNF_T / 1000.0f)
#define HSSNF_K  (CONFIG_GYRO_MOUSE_HSSNF_K / 1000.0f)

float ramp_mid(float x, float z)
{
	if (x < z) return 0.0f;
	if (x > (1.0f - z)) return 1.0f;
	return (x - z) / (1.0f - 2.0f * z);
}

float hssnf(float t, float k, float x)
{
	float a = x - (x * k);
	float b = 1.0f - (x * k * (1.0f / t));
	return a / b;
}

float apply_hssnf(float x)
{
#if !CONFIG_GYRO_MOUSE_HSSNF_ENABLE
	return x;
#endif
	if (x > 0.0f && x < HSSNF_T)
		return hssnf(HSSNF_T, HSSNF_K, x);
	if (x < 0.0f && x > -HSSNF_T)
		return -hssnf(HSSNF_T, HSSNF_K, -x);
	return x;
}
