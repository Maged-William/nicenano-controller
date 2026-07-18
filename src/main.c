#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

void main(void)
{
    while (1) {
        printk("Hello World from Zephyr!\n");
        k_sleep(K_SECONDS(3));
    }
}
