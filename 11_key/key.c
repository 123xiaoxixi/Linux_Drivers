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

#define GPIOKEY_CNT  1
#define GPIOKEY_NAME  "gpiokey"

#define KEY0VALUE    0xF0
#define INVAKEY      0x0


struct key_dev {
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int key_gpio;
    atomic_t keyvalue;
};

struct key_dev gpiokey;

static int key_open(struct inode *inode, struct file *filp) {
    filp->private_data = &gpiokey;
    return 0;
}

static int key_release(struct inode *inode, struct file *filp) {
    // unsigned int val = 0;
    struct key_dev *dev = (struct key_dev *)filp->private_data;

    return 0;
}

static ssize_t key_write(struct file *filp, const char __user *buff, size_t count, loff_t *loff) {
    int ret = 0;
    unsigned char data[1];
    struct key_dev *dev = (struct key_dev *)filp->private_data;
    ret = copy_from_user(data, buff, count);
    if (ret < 0) {
        printk("copy_from_user failed\r\n");
        return -EINVAL;
    }


    return 0;
}

static ssize_t key_read(struct file *filp, char __user *buff, size_t count, loff_t *loff) {
    int ret = 0;
    int value;
    struct key_dev *dev = (struct key_dev*)filp->private_data;
    if (gpio_get_value(dev->key_gpio) == 0) {
        while(!gpio_get_value(dev->key_gpio));
        atomic_set(&dev->keyvalue, KEY0VALUE);
    } else {
        atomic_set(&dev->keyvalue, INVAKEY);
    }
    value = atomic_read(&dev->keyvalue);
    ret = copy_to_user(buff, &value, sizeof(value));

    return ret;
}

static struct file_operations key_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .write = key_write,
    .release = key_release,
    .read = key_read,
};

static int keyio_init(struct key_dev *dev) {
    int ret = 0;
    dev->nd = of_find_node_by_path("/key");
    if (dev->nd == NULL) {
        ret = -EINVAL;
        goto fail_nd;
    }
    dev->key_gpio = of_get_named_gpio(dev->nd, "key_gpios",0);
    if (dev->key_gpio < 0) {
        ret = -EINVAL;
        goto fail_getgpio;
    }
    ret = gpio_request(dev->key_gpio, "key0");
    if (ret) {
        ret = -EBUSY;
        printk("gpio_request failed\r\n");
        goto fail_gpioreq;
    }
    ret = gpio_direction_input(dev->key_gpio);
    if (ret < 0) {
        ret = -EINVAL;
        goto fail_set;
    } 
    return 0;
fail_set:
    gpio_free(dev->key_gpio);
fail_gpioreq:
fail_getgpio:
fail_nd:

    return ret;
}

static int __init key_init(void) {
    int ret = 0;

    atomic_set(&gpiokey.keyvalue,INVAKEY);
    gpiokey.major = 0;

    if (gpiokey.major > 0 ) {
        gpiokey.devid = MKDEV(gpiokey.major,0);
        register_chrdev_region(gpiokey.devid, GPIOKEY_CNT, GPIOKEY_NAME);
    } else {
        alloc_chrdev_region(&gpiokey.devid, 0, GPIOKEY_CNT, GPIOKEY_NAME);
        gpiokey.major = MAJOR(gpiokey.devid);
        gpiokey.minor = MINOR(gpiokey.devid);
    }
    printk("major = %d  minor = %d\r\n", gpiokey.major, gpiokey.minor);

    gpiokey.cdev.owner = THIS_MODULE;
    cdev_init(&gpiokey.cdev, &key_fops);
    cdev_add(&gpiokey.cdev, gpiokey.devid,GPIOKEY_CNT);

    gpiokey.class = class_create(THIS_MODULE, GPIOKEY_NAME);
    if (IS_ERR(gpiokey.class)) {
        return PTR_ERR(gpiokey.class);
    }
    gpiokey.device = device_create(gpiokey.class, NULL, gpiokey.devid, NULL, GPIOKEY_NAME);
    if (IS_ERR(gpiokey.device)) {
        return PTR_ERR(gpiokey.device);
    }
    
    ret = keyio_init(&gpiokey);
    if (ret < 0) {
        goto fail_findnd;
    }
    return 0;


fail_findnd:
    device_destroy(gpiokey.class, gpiokey.devid);
    class_destroy(gpiokey.class);
    cdev_del(&gpiokey.cdev);
    unregister_chrdev_region(gpiokey.devid, GPIOKEY_CNT);
    return ret;
}

static void __exit key_exit(void) {
    
    gpio_free(gpiokey.key_gpio);
    device_destroy(gpiokey.class, gpiokey.devid);
    class_destroy(gpiokey.class);

    cdev_del(&gpiokey.cdev);
    unregister_chrdev_region(gpiokey.devid, GPIOKEY_CNT);
}

module_init(key_init);
module_exit(key_exit);

MODULE_LICENSE("GPL");
