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

#define NEWCHRLED_NAME  "platled"

static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *IMX6U_SW_MUX_GPIO1_IO03;
static void __iomem *IMX6U_SW_PAD_GPIO1_IO03;
static void __iomem *IMX6U_GPIO1_DR;
static void __iomem *IMX6U_GPIO1_GDIR;

#define LEDOFF  0
#define LEDON   1

struct newchrled_dev {
    struct cdev cdev;
    dev_t devid;
    int major;
    int minor;
    struct class *class;
    struct device *device;
};

struct newchrled_dev newchrled;

static int led_open(struct inode *inode, struct file *filp) {
    filp->private_data = &newchrled;
    return 0;
}

static int led_release(struct inode *inode, struct file *filp) {
    // unsigned int val = 0;
    struct newchrled_dev *dev = (struct newchrled_dev *)filp->private_data;

    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buff, size_t count, loff_t *loff) {
    unsigned int val;
    int retvalue;
    unsigned char databuf[1];
    retvalue = copy_from_user(databuf, buff, count);
    if (retvalue < 0) {
        printk("copy_from_user() failed\r\n");
        return -EFAULT;
    }
    if (databuf[0] == LEDON) {
        val = readl(IMX6U_GPIO1_DR);
        val &= ~(1<<3);
        writel(val, IMX6U_GPIO1_DR);
    } else if (databuf[0] == LEDOFF) {
        val = readl(IMX6U_GPIO1_DR);
        val |= (1<<3);
        writel(val, IMX6U_GPIO1_DR);
    }
    return 0;
}

static struct file_operations led_ops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .write = led_write,
};

static int led_probe(struct platform_device *dev) {
    struct resource *ledsource[5];
    int i = 0;
    int ret = 0;
    unsigned int val = 0;

    printk("led driver probe\r\n");
    for (i = 0; i < 5; i++) {
        ledsource[i] = platform_get_resource(dev,IORESOURCE_MEM, i);    //获取资源
        if (ledsource[i] == NULL)
            return -EINVAL;
    }
    
    IMX6U_CCM_CCGR1 = ioremap(ledsource[0]->start, resource_size(ledsource[0]));
    IMX6U_SW_MUX_GPIO1_IO03 = ioremap(ledsource[1]->start, resource_size(ledsource[1]));
    IMX6U_SW_PAD_GPIO1_IO03 = ioremap(ledsource[2]->start, resource_size(ledsource[2]));
    IMX6U_GPIO1_DR = ioremap(ledsource[3]->start, resource_size(ledsource[3]));
    IMX6U_GPIO1_GDIR = ioremap(ledsource[4]->start, resource_size(ledsource[4]));

    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26);
    val |= (3 << 26);
    writel(val, IMX6U_CCM_CCGR1);

    writel(0x5, IMX6U_SW_MUX_GPIO1_IO03);
    writel(0x10b0, IMX6U_SW_PAD_GPIO1_IO03);
    
    val = readl(IMX6U_GPIO1_GDIR);
    val |= 1<<3;
    writel(val, IMX6U_GPIO1_GDIR);

    val = readl(IMX6U_GPIO1_DR);
    val &= ~(1<<3);
    writel(val, IMX6U_GPIO1_DR);

    if (newchrled.major) {
        newchrled.devid = MKDEV(newchrled.major, 0);
        ret = register_chrdev_region(newchrled.devid, 1, NEWCHRLED_NAME);
    } else {
        ret = alloc_chrdev_region(&newchrled.devid, 0, 1, NEWCHRLED_NAME);
        newchrled.major = MAJOR(newchrled.devid);
        newchrled.minor = MINOR(newchrled.devid);
        printk("device id: major = %d minor = %d \r\n", newchrled.major, newchrled.minor);
    }
    if (ret < 0) {
        printk("newchrled chrdev_region error\r\n");
        return -1;
    }
    newchrled.cdev.owner = THIS_MODULE;
    cdev_init(&newchrled.cdev, &led_ops);
    cdev_add(&newchrled.cdev, newchrled.devid, 1);

    newchrled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
    newchrled.device = device_create(newchrled.class, NULL, newchrled.devid, NULL, NEWCHRLED_NAME);
    
    return 0;
}

static int led_remove(struct platform_device *dev) {
    
    unsigned int val = 0;
    printk("led driver remove\r\n");
    val = readl(IMX6U_GPIO1_DR);
    val |= (1<<3);
    writel(val, IMX6U_GPIO1_DR);

    iounmap(IMX6U_CCM_CCGR1);
    iounmap(IMX6U_SW_MUX_GPIO1_IO03);
    iounmap(IMX6U_SW_PAD_GPIO1_IO03);
    iounmap(IMX6U_GPIO1_DR);
    iounmap(IMX6U_GPIO1_GDIR);

    device_destroy(newchrled.class, newchrled.devid);
    class_destroy(newchrled.class);

    cdev_del(&newchrled.cdev);
    
    unregister_chrdev_region(newchrled.devid, 1);
    return 0;
}

//platform驱动结构体
static struct platform_driver led_driver = {
    .driver = {
        .name = "imx6ull-led",
    },
    .probe = led_probe,
    .remove = led_remove,
};

//驱动加载
static int __init leddriver_init(void) {    
    return platform_driver_register(&led_driver);
}

static void __exit leddriver_exit(void) {
    platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");


