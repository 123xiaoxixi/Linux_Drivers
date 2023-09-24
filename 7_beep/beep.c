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

#define GPIOBEEP_CNT  1
#define GPIOBEEP_NAME  "gpiobeep"

#define LED_ON  1
#define LED_OFF 0

struct beep_dev {
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int beep_gpio;
};

struct beep_dev gpiobeep;

static int led_open(struct inode *inode, struct file *filp) {
    filp->private_data = &gpiobeep;
    return 0;
}

static int led_release(struct inode *inode, struct file *filp) {
    // unsigned int val = 0;
    struct beep_dev *dev = (struct beep_dev *)filp->private_data;

    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buff, size_t count, loff_t *loff) {
    int ret = 0;
    unsigned char data[1];
    struct beep_dev *dev = (struct beep_dev *)filp->private_data;
    ret = copy_from_user(data, buff, count);
    if (ret < 0) {
        printk("copy_from_user failed\r\n");
        return -EINVAL;
    }
    if (data[0] == LED_ON) 
        gpio_set_value(dev->beep_gpio, 0);
    else if (data[0] == LED_OFF)
        gpio_set_value(dev->beep_gpio, 1);


    return 0;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
    .release = led_release,
};



static int __init led_init(void) {
    int ret = 0;

    gpiobeep.major = 0;

    if (gpiobeep.major > 0 ) {
        gpiobeep.devid = MKDEV(gpiobeep.major,0);
        register_chrdev_region(gpiobeep.devid, GPIOBEEP_CNT, GPIOBEEP_NAME);
    } else {
        alloc_chrdev_region(&gpiobeep.devid, 0, GPIOBEEP_CNT, GPIOBEEP_NAME);
        gpiobeep.major = MAJOR(gpiobeep.devid);
        gpiobeep.minor = MINOR(gpiobeep.devid);
    }
    printk("major = %d  minor = %d\r\n", gpiobeep.major, gpiobeep.minor);

    gpiobeep.cdev.owner = THIS_MODULE;
    cdev_init(&gpiobeep.cdev, &led_fops);
    cdev_add(&gpiobeep.cdev, gpiobeep.devid,GPIOBEEP_CNT);

    gpiobeep.class = class_create(THIS_MODULE, GPIOBEEP_NAME);
    if (IS_ERR(gpiobeep.class)) {
        return PTR_ERR(gpiobeep.class);
    }
    gpiobeep.device = device_create(gpiobeep.class, NULL, gpiobeep.devid, NULL, GPIOBEEP_NAME);
    if (IS_ERR(gpiobeep.device)) {
        return PTR_ERR(gpiobeep.device);
    }

    gpiobeep.nd = of_find_node_by_path("/beep");
    if (gpiobeep.nd == NULL) {
        ret = -EINVAL;
        goto fail_findnd;
    }
    gpiobeep.beep_gpio = of_get_named_gpio(gpiobeep.nd, "beep_gpios", 0);
    if (gpiobeep.beep_gpio < 0) {
        ret = -EINVAL;
        goto fail_getgpio;
    }
    ret = gpio_request(gpiobeep.beep_gpio, "beep-gpio");
    if (ret) {
        printk("gpio_request failed\r\n");
        goto fail_getgpio;
    }
    ret = gpio_direction_output(gpiobeep.beep_gpio, 0);
    if (ret < 0) {
        goto fail_output;
    }
    gpio_set_value(gpiobeep.beep_gpio, 1);
    

    return 0;

fail_output:
    gpio_free(gpiobeep.beep_gpio);
fail_getgpio:
fail_findnd:
    device_destroy(gpiobeep.class, gpiobeep.devid);
    class_destroy(gpiobeep.class);
    cdev_del(&gpiobeep.cdev);
    unregister_chrdev_region(gpiobeep.devid, GPIOBEEP_CNT);
    return ret;
}

static void __exit led_exit(void) {
    gpio_set_value(gpiobeep.beep_gpio, 1);
    
    gpio_free(gpiobeep.beep_gpio);
    device_destroy(gpiobeep.class, gpiobeep.devid);
    class_destroy(gpiobeep.class);

    cdev_del(&gpiobeep.cdev);
    unregister_chrdev_region(gpiobeep.devid, GPIOBEEP_CNT);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
