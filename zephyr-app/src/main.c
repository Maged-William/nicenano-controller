#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>

void main(void)
{
    usb_enable(NULL);

    while (1) {
        printk("Hello World from Zephyr!\n");
        k_sleep(K_SECONDS(3));
    }
}
