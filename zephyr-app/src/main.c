#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

#define RECT_SIZE  100
#define STEP_DELAY K_MSEC(10)

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
	0xC0,              /*   End Collection */
	0xC0               /* End Collection */
};

static void send_mouse_move(int8_t dx, int8_t dy)
{
	uint8_t report[4] = { 0, (uint8_t)dx, (uint8_t)dy, 0 };
	hid_int_ep_write(hid_dev, report, sizeof(report), NULL);
}

void main(void)
{
	const struct device *cdc = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
	const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	uint32_t dtr = 0;
	int x = 0, y = 0;

	gpio_pin_configure(gpio0, 15, GPIO_OUTPUT_ACTIVE);

	hid_dev = DEVICE_DT_GET(DT_NODELABEL(hid0));
	if (!device_is_ready(hid_dev)) {
		printk("HID device not ready\n");
		return;
	}

	usb_hid_register_device(hid_dev, hid_report_desc, sizeof(hid_report_desc), NULL);
	usb_hid_init(hid_dev);

	usb_enable(NULL);

	while (!dtr) {
		uart_line_ctrl_get(cdc, UART_LINE_CTRL_DTR, &dtr);
		k_sleep(K_MSEC(100));
	}

	k_sleep(K_SECONDS(1));

	printk("Exp07: HID Mouse starting - rectangle %dx%d\n", RECT_SIZE, RECT_SIZE);

	while (1) {
		gpio_pin_toggle(gpio0, 15);

		for (int i = 0; i < RECT_SIZE; i++) {
			send_mouse_move(1, 0);
			x++;
			printk("X:%d Y:%d\n", x, y);
			k_sleep(STEP_DELAY);
		}
		for (int i = 0; i < RECT_SIZE; i++) {
			send_mouse_move(0, 1);
			y++;
			printk("X:%d Y:%d\n", x, y);
			k_sleep(STEP_DELAY);
		}
		for (int i = 0; i < RECT_SIZE; i++) {
			send_mouse_move(-1, 0);
			x--;
			printk("X:%d Y:%d\n", x, y);
			k_sleep(STEP_DELAY);
		}
		for (int i = 0; i < RECT_SIZE; i++) {
			send_mouse_move(0, -1);
			y--;
			printk("X:%d Y:%d\n", x, y);
			k_sleep(STEP_DELAY);
		}
	}
}
