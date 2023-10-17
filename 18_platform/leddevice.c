#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>  
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/timer.h>

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_irq.h>

#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/ide.h>

#include <linux/platform_device.h>


#define CCM_CCGR1_BASE          (0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE  (0x020E0068)
#define SW_PAD_GPIO1_IO03_BASE  (0x020E02F4)
#define GPIO1_DR_BASE           (0x0209C000)
#define GPIO1_GDIR_BASE         (0x0209C004)

#define REGISTER_LENGTH				4

static struct resource led_resources[] = {
    {
        .start = CCM_CCGR1_BASE,
        .end = CCM_CCGR1_BASE + REGISTER_LENGTH - 1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = SW_MUX_GPIO1_IO03_BASE,
        .end = SW_MUX_GPIO1_IO03_BASE + REGISTER_LENGTH - 1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = SW_PAD_GPIO1_IO03_BASE,
        .end = SW_PAD_GPIO1_IO03_BASE + REGISTER_LENGTH - 1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = GPIO1_DR_BASE,
        .end = GPIO1_DR_BASE + REGISTER_LENGTH - 1,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = GPIO1_GDIR_BASE,
        .end = GPIO1_GDIR_BASE + REGISTER_LENGTH - 1,
        .flags = IORESOURCE_MEM,
    },
    
};

void leddevice_release(struct device *dev) {
    printk("leddevice_release\r\n");
}

static struct platform_device leddevice = {
    .name = "imx6ull-led",
    .id = -1,
    .dev = {
        .release = leddevice_release,
    },
    .num_resources = ARRAY_SIZE(led_resources),
    .resource = led_resources,
};

//设备加载
static int __init leddevice_init(void) {
    int ret = 0;
    ret = platform_device_register(&leddevice);

    return 0;
}

static void __exit leddevice_exit(void) {
    platform_device_unregister(&leddevice);
}

module_init(leddevice_init);
module_exit(leddevice_exit);
MODULE_LICENSE("GPL");


