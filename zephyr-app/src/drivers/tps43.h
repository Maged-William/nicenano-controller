#ifndef TPS43_H
#define TPS43_H

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <stdbool.h>
#include <stdint.h>

#define TPS43_ADDR         0x74
#define TPS43_GESTURE0     0x0D
#define TPS43_GESTURE1     0x0E
#define TPS43_SYS_INFO0    0x0F
#define TPS43_SYS_INFO1    0x10
#define TPS43_FINGER_COUNT 0x11
#define TPS43_XREL_HIGH    0x12
#define TPS43_XREL_LOW     0x13
#define TPS43_YREL_HIGH    0x14
#define TPS43_YREL_LOW     0x15
#define TPS43_XABS_HIGH    0x16
#define TPS43_XABS_LOW     0x17
#define TPS43_YABS_HIGH    0x18
#define TPS43_YABS_LOW     0x19
#define TPS43_STRENGTH_HIGH 0x1A
#define TPS43_STRENGTH_LOW  0x1B
#define TPS43_TOUCH_AREA   0x1C

/* Configuration registers (0x06xx) */
#define TPS43_CFG_RESET        0x0600
#define TPS43_CFG_SF_GESTURE   0x06B7
#define TPS43_CFG_TAP_TIME     0x06B9
#define TPS43_CFG_HOLD_TIME    0x06BD

#if CONFIG_TPS43_ENABLE

extern bool tps43_found;
extern uint8_t tps43_regs[16];

bool tps43_init(const struct device *i2c);
bool tps43_poll(int16_t *dx, int16_t *dy, bool *tap);
bool tps43_write_config(uint16_t reg, uint8_t val);
bool tps43_disable_gestures(void);

#endif
#endif
