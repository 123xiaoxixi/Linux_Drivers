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
#include <linux/miscdevice.h>

#define  BEEP_ON   1
#define  BEEP_OFF   0
#define MISCBEEP_NAME  "miscbeep"
#define MISCBEEP_MINOR  144

struct miscbeep_dev {
    int beep_gpio;
    struct device_node *nd;
};

struct miscbeep_dev miscbeep;

static const struct of_device_id beep_of_match[] = {
    {.compatible = "alientek,beep"},
    {},
};

static int beep_open(struct inode *inode, struct file *filp) {
    filp->private_data = &miscbeep;
    return 0;
}

static int beep_release(struct inode *inode, struct file *filp) {
    // unsigned int val = 0;
    // struct miscbeep_dev *dev = (struct miscbeep_dev *)filp->private_data;

    return 0;
}

static ssize_t beep_write(struct file *filp, const char __user *buff, size_t count, loff_t *loff) {

    int ret = 0;
    unsigned char data[1];
    struct miscbeep_dev *dev = (struct miscbeep_dev *)filp->private_data;
    ret = copy_from_user(data, buff, count);
    if (ret < 0) {
        printk("copy_from_user failed\r\n");
        return -EINVAL;
    }
    if (data[0] == BEEP_ON) 
        gpio_set_value(dev->beep_gpio, 0);
    else if (data[0] == BEEP_OFF)
        gpio_set_value(dev->beep_gpio, 1);

    return 0;
}

struct file_operations miscbeep_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .write = beep_write,
    .release = beep_release,
};

static struct miscdevice miscdevice_beep = {
    .minor = MISCBEEP_MINOR,
    .name = MISCBEEP_NAME,
    .fops = &miscbeep_fops,
};

int miscbeep_probe(struct platform_device *dev) {
    int ret = 0;
    printk("probe1\r\n");
    miscbeep.nd = dev->dev.of_node;
    miscbeep.beep_gpio = of_get_named_gpio(miscbeep.nd, "beep_gpios", 0);
    if (miscbeep.beep_gpio < 0) {
        ret = -EINVAL;
        goto fail_findgpio;
    }
    ret = gpio_request(miscbeep.beep_gpio, "beep-gpio");
    if (ret) {
        printk("gpio request failed\r\n");
        ret = -EINVAL;
        goto fail_req;
    }
    ret = gpio_direction_output(miscbeep.beep_gpio, 1);
    if (ret < 0) {
        goto fail_setoutput;
    }

    //misc注册
    ret = misc_register(&miscdevice_beep);
    if (ret < 0) {
        goto fail_setoutput;
    }

    return 0;
fail_setoutput:
    gpio_free(miscbeep.beep_gpio);
fail_req:
fail_findgpio:
    return ret;
}

int miscbeep_remove(struct platform_device *dev) {
    misc_deregister(&miscdevice_beep);
    gpio_set_value(miscbeep.beep_gpio, 1);
    gpio_free(miscbeep.beep_gpio);
    return 0;
}

static struct platform_driver miscbeep_driver = {
    .driver = {
        .name = "alientek,beep",
        .of_match_table = beep_of_match,
    },
    .probe = miscbeep_probe,
    .remove = miscbeep_remove,
};



static int __init miscbeep_init(void) {
    return platform_driver_register(&miscbeep_driver);
}

static void __exit miscbeep_exit(void) {
    
    platform_driver_unregister(&miscbeep_driver);
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);
MODULE_LICENSE("GPL");

