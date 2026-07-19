#ifndef FILTER_H
#define FILTER_H

#include <stdbool.h>

static inline float fast_fabs(float x) { return x < 0.0f ? -x : x; }

float ramp_mid(float x, float z);
float hssnf(float t, float k, float x);
float apply_hssnf(float x);

#endif
