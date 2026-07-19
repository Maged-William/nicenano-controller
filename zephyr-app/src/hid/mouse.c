#include <zephyr/kernel.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/device.h>
#include "mouse.h"

static const struct device *hid_dev;

static const uint8_t hid_report_desc[] = {
	0x05, 0x01,        /* Usage Page (Generic Desktop) */
	0x09, 0x02,        /* Usage (Mouse) */
	0xA1, 0x01,        /* Collection (Application) */
	0x09, 0x01,        /*   Usage (Pointer) */
	0xA1, 0x00,        /*   Collection (Physical) */
	0x05, 0x09,        /*     Usage Page (Button) */
	0x19, 0x01,        /*     Usage Minimum (1) */
	0x29, 0x03,        /*     Usage Maximum (3) */
	0x15, 0x00,        /*     Logical Minimum (0) */
	0x25, 0x01,        /*     Logical Maximum (1) */
	0x95, 0x03,        /*     Report Count (3) */
	0x75, 0x01,        /*     Report Size (1) */
	0x81, 0x02,        /*     Input (Data,Var,Abs) */
	0x95, 0x01,        /*     Report Count (1) */
	0x75, 0x05,        /*     Report Size (5) */
	0x81, 0x03,        /*     Input (Const,Var,Abs) */
	0x05, 0x01,        /*     Usage Page (Generic Desktop) */
	0x09, 0x30,        /*     Usage (X) */
	0x09, 0x31,        /*     Usage (Y) */
	0x16, 0x00, 0x80,  /*     Logical Minimum (-128) */
	0x26, 0xFF, 0x7F,  /*     Logical Maximum (127) */
	0x75, 0x08,        /*     Report Size (8) */
	0x95, 0x02,        /*     Report Count (2) */
	0x81, 0x06,        /*     Input (Data,Var,Rel) */
	0x09, 0x38,        /*     Usage (Wheel) */
	0x15, 0x81,        /*     Logical Minimum (-127) */
	0x25, 0x7F,        /*     Logical Maximum (127) */
	0x75, 0x08,        /*     Report Size (8) */
	0x95, 0x01,        /*     Report Count (1) */
	0x81, 0x06,        /*     Input (Data,Var,Rel) */
	0x05, 0x0C,        /*     Usage Page (Consumer) */
	0x0A, 0x38, 0x02,  /*     Usage (AC Pan) */
	0x15, 0x81,        /*     Logical Minimum (-127) */
	0x25, 0x7F,        /*     Logical Maximum (127) */
	0x75, 0x08,        /*     Report Size (8) */
	0x95, 0x01,        /*     Report Count (1) */
	0x81, 0x06,        /*     Input (Data,Var,Rel) */
	0xC0,              /*   End Collection */
	0xC0               /* End Collection */
};

float mouse_acc_x;
float mouse_acc_y;
int mouse_wheel;
int mouse_wheel_h;
uint8_t mouse_buttons;

void mouse_init(const struct device *dev)
{
	hid_dev = dev;
	usb_hid_register_device(hid_dev, hid_report_desc, sizeof(hid_report_desc), NULL);
	usb_hid_init(hid_dev);
}

void send_mouse_report(int8_t dx, int8_t dy, int8_t w, int8_t wh)
{
	uint8_t report[5] = { mouse_buttons, (uint8_t)dx, (uint8_t)dy, (uint8_t)w, (uint8_t)wh };
	hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
}
