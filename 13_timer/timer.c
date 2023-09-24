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
#include <linux/timer.h>
#include <linux/jiffies.h>

#define GPIOLED_CNT  1
#define GPIOLED_NAME  "gpioled"

#define LED_ON  1
#define LED_OFF 0

#define CLOSE_CMD   _IO(0xEF, 1)
#define OPEN_CMD    _IO(0xEF, 2)
#define SETPERIOD_CMD   _IOW(0xEF, 3, int)

struct gpioled_dev {
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int led_gpio;
    int dev_status;
    spinlock_t lock;

    struct timer_list timer;
    int timeperiod;
};

struct gpioled_dev gpioled;

static int led_open(struct inode *inode, struct file *filp) {
    
    unsigned long irqflag;
    // spin_lock(&gpioled.lock);
    spin_lock_irqsave(&gpioled.lock,irqflag);
    if (gpioled.dev_status) {
        // spin_unlock(&gpioled.lock);
        spin_unlock_irqrestore(&gpioled.lock,irqflag);
        return -EBUSY;
    }
    gpioled.dev_status++;   //標記被使用
    filp->private_data = &gpioled;

    // spin_unlock(&gpioled.lock);
    spin_unlock_irqrestore(&gpioled.lock,irqflag);
    return 0;
}

static int led_release(struct inode *inode, struct file *filp) {
    // unsigned int val = 0;
    // spin_lock(&gpioled.lock);
    unsigned long irqflag;
    spin_lock_irqsave(&gpioled.lock,irqflag);
    if (gpioled.dev_status) {
        gpioled.dev_status--;
    }
    struct gpioled_dev *dev = (struct gpioled_dev *)filp->private_data;

    // spin_unlock(&gpioled.lock);
    spin_unlock_irqrestore(&gpioled.lock,irqflag);
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buff, size_t count, loff_t *loff) {
    int ret = 0;
    unsigned char data[1];
    struct gpioled_dev *dev = (struct gpioled_dev *)filp->private_data;
    ret = copy_from_user(data, buff, count);
    if (ret < 0) {
        printk("copy_from_user failed\r\n");
        return -EINVAL;
    }
    if (data[0] == LED_ON) 
        gpio_set_value(dev->led_gpio, 0);
    else if (data[0] == LED_OFF)
        gpio_set_value(dev->led_gpio, 1);


    return 0;
}

long led_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    int value = 0;
    int ret = 0;
    struct gpioled_dev *dev = (struct gpioled_dev *)filp->private_data;
    switch(cmd) {
        case CLOSE_CMD:
            del_timer(&dev->timer);
            break;
        case OPEN_CMD:
            mod_timer(&dev->timer, msecs_to_jiffies(dev->timeperiod)+jiffies);
            break;
        case SETPERIOD_CMD:
            ret = copy_from_user(&value, (int *)arg, sizeof(int));
            if (ret < 0) {
                return -EINVAL;
            }
            dev->timeperiod = value;
            mod_timer(&dev->timer, msecs_to_jiffies(dev->timeperiod)+jiffies);
            break;
    }

    return 0;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
    .release = led_release,
    .unlocked_ioctl = led_unlocked_ioctl,
};

static void timer_func(unsigned long arg) {
    static int sta = 1;
    struct gpioled_dev *dev = (struct gpioled_dev *)arg;
    if (sta == 1) {
        gpio_set_value(dev->led_gpio, 0);
        sta = 0;
    } else {
        gpio_set_value(dev->led_gpio, 1);
        sta = 1;
    }
    mod_timer(&dev->timer,msecs_to_jiffies(dev->timeperiod)+jiffies);
}

static int __init led_init(void) {
    int ret = 0;

    spin_lock_init(&gpioled.lock);
    gpioled.dev_status = 0;
    gpioled.major = 0;

    if (gpioled.major > 0 ) {
        gpioled.devid = MKDEV(gpioled.major,0);
        register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
    } else {
        alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
        gpioled.major = MAJOR(gpioled.devid);
        gpioled.minor = MINOR(gpioled.devid);
    }
    printk("major = %d  minor = %d\r\n", gpioled.major, gpioled.minor);

    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &led_fops);
    cdev_add(&gpioled.cdev, gpioled.devid,GPIOLED_CNT);

    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if (IS_ERR(gpioled.class)) {
        return PTR_ERR(gpioled.class);
    }
    gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
    if (IS_ERR(gpioled.device)) {
        return PTR_ERR(gpioled.device);
    }

    gpioled.nd = of_find_node_by_path("/gpioled");
    if (gpioled.nd == NULL) {
        ret = -EINVAL;
        goto fail_findnd;
    }
    gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpios", 0);
    printk("gpioled.led_gpio = %d\r\n", gpioled.led_gpio);
    if (gpioled.led_gpio < 0) {
        ret = -EINVAL;
        goto fail_getgpio;
    }
    ret = gpio_request(gpioled.led_gpio, "led-gpio");
    if (ret) {
        printk("gpio_request failed\r\n");
        goto fail_getgpio;
    }
    ret = gpio_direction_output(gpioled.led_gpio, 1);
    if (ret)
        goto fail_output;
    gpio_set_value(gpioled.led_gpio, 1);

    gpioled.timeperiod = 500;
    init_timer(&gpioled.timer);
    gpioled.timer.expires = jiffies + msecs_to_jiffies(gpioled.timeperiod);
    gpioled.timer.function = timer_func;
    gpioled.timer.data = (unsigned long)&gpioled;

    add_timer(&gpioled.timer);

    return 0;

fail_output:
    gpio_free(gpioled.led_gpio);
fail_getgpio:
fail_findnd:
    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class);
    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
    return ret;
}

static void __exit led_exit(void) {
    del_timer(&gpioled.timer);

    gpio_set_value(gpioled.led_gpio, 1);

    gpio_free(gpioled.led_gpio);

    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class);

    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
