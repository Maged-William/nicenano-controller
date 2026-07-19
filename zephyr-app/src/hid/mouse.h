#ifndef MOUSE_H
#define MOUSE_H

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <stdint.h>

extern float mouse_acc_x;
extern float mouse_acc_y;
extern int mouse_wheel;
extern int mouse_wheel_h;
extern uint8_t mouse_buttons;

void mouse_init(const struct device *hid_dev);
void send_mouse_report(int8_t dx, int8_t dy, int8_t w, int8_t wh);

#endif
