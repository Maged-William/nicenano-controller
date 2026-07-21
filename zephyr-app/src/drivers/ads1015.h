#ifndef ADS1015_H
#define ADS1015_H

#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>

void ads1015_init(const struct device *i2c);
int ads1015_scan(void);
int16_t ads1015_read_channel(uint8_t ch);

#define ADS1015_RATE_128   0
#define ADS1015_RATE_250   1
#define ADS1015_RATE_490   2
#define ADS1015_RATE_920   3
#define ADS1015_RATE_1600  4
#define ADS1015_RATE_2400  5
#define ADS1015_RATE_3300  6

void ads1015_set_data_rate(uint8_t rate);
void ads1015_calibrate(uint8_t ch, int samples);
int16_t ads1015_read_calibrated(uint8_t ch);

#endif
