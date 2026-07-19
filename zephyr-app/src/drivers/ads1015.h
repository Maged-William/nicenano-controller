#ifndef ADS1015_H
#define ADS1015_H

#include <zephyr/device.h>
#include <stdint.h>
#include <stdbool.h>

void ads1015_init(const struct device *i2c);
int ads1015_scan(void);
int16_t ads1015_read_channel(uint8_t ch);

#endif
