#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/gpio.h>

void main(void)
{
    const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    gpio_pin_configure(gpio0, 15, GPIO_OUTPUT_ACTIVE);

    usb_enable(NULL);

    while (1) {
        gpio_pin_toggle(gpio0, 15);
        printk("Hello World from Zephyr!\n");
        k_sleep(K_SECONDS(1));
    }
}
