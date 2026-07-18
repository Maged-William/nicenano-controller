#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

void main(void)
{
    const struct device *cdc = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
    const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    uint32_t dtr = 0;

    gpio_pin_configure(gpio0, 15, GPIO_OUTPUT_ACTIVE);

    usb_enable(NULL);

    while (!dtr) {
        uart_line_ctrl_get(cdc, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }

    while (1) {
        gpio_pin_toggle(gpio0, 15);
        printk("Hello World from Zephyr!\n");
        k_sleep(K_SECONDS(3));
    }
}
