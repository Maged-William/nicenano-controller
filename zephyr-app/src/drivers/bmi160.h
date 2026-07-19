#ifndef BMI160_H
#define BMI160_H

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <stdbool.h>
#include <stdint.h>

/* Register map */
#define BMI160_CHIPID      0x00
#define BMI160_PMU_STATUS  0x03
#define BMI160_DATA_8      0x0C
#define BMI160_ACCEL_CONF  0x40
#define BMI160_ACCEL_RANGE 0x41
#define BMI160_GYRO_CONF   0x42
#define BMI160_GYRO_RANGE  0x43
#define BMI160_CMD         0x7E

#define ACCEL_RANGE_2G  0x03

/* Gyro range to register value */
#if CONFIG_GYRO_MOUSE_S1_RANGE == 125
#define S1_GYRO_RANGE_REG  0x04
#elif CONFIG_GYRO_MOUSE_S1_RANGE == 250
#define S1_GYRO_RANGE_REG  0x03
#elif CONFIG_GYRO_MOUSE_S1_RANGE == 500
#define S1_GYRO_RANGE_REG  0x02
#elif CONFIG_GYRO_MOUSE_S1_RANGE == 1000
#define S1_GYRO_RANGE_REG  0x01
#elif CONFIG_GYRO_MOUSE_S1_RANGE == 2000
#define S1_GYRO_RANGE_REG  0x00
#else
#error "Invalid GYRO_MOUSE_S1_RANGE"
#endif

#if CONFIG_GYRO_MOUSE_S2_RANGE == 125
#define S2_GYRO_RANGE_REG  0x04
#elif CONFIG_GYRO_MOUSE_S2_RANGE == 250
#define S2_GYRO_RANGE_REG  0x03
#elif CONFIG_GYRO_MOUSE_S2_RANGE == 500
#define S2_GYRO_RANGE_REG  0x02
#elif CONFIG_GYRO_MOUSE_S2_RANGE == 1000
#define S2_GYRO_RANGE_REG  0x01
#elif CONFIG_GYRO_MOUSE_S2_RANGE == 2000
#define S2_GYRO_RANGE_REG  0x00
#else
#error "Invalid GYRO_MOUSE_S2_RANGE"
#endif

#if CONFIG_GYRO_MOUSE_ODR == 25
#define ODR_REG  0x06
#elif CONFIG_GYRO_MOUSE_ODR == 50
#define ODR_REG  0x07
#elif CONFIG_GYRO_MOUSE_ODR == 100
#define ODR_REG  0x08
#elif CONFIG_GYRO_MOUSE_ODR == 200
#define ODR_REG  0x09
#elif CONFIG_GYRO_MOUSE_ODR == 400
#define ODR_REG  0x0A
#elif CONFIG_GYRO_MOUSE_ODR == 800
#define ODR_REG  0x0B
#elif CONFIG_GYRO_MOUSE_ODR == 1600
#define ODR_REG  0x0C
#else
#error "Invalid GYRO_MOUSE_ODR"
#endif

void bmi160_bus_init(const struct device *spi, const struct device *gpio);
bool bmi160_sensor_init(gpio_pin_t cs, uint8_t gyro_range);
void bmi160_read_gyro(gpio_pin_t cs, float *gx, float *gy, float *gz,
		      float ox, float oy, float oz);

#endif
