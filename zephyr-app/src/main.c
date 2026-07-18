#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>

void main(void)
{
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
    uint32_t dtr = 0;

    usb_enable(NULL);

    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }

    while (1) {
        printk("Hello World from Zephyr!\n");
        k_sleep(K_SECONDS(3));
    }
}
