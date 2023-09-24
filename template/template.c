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

#define TEMPLATE_CNT  1
#define TEMPLATE_NAME  "template"

#define LED_ON  1
#define LED_OFF 0

struct template_dev {
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
};

struct template_dev template;

static int template_open(struct inode *inode, struct file *filp) {
    
    filp->private_data = &template;

    return 0;
}

static int template_release(struct inode *inode, struct file *filp) {

    struct template_dev *dev = (struct template_dev *)filp->private_data;

    return 0;
}

static ssize_t template_write(struct file *filp, const char __user *buff, size_t count, loff_t *loff) {
    int ret = 0;
    unsigned char data[1];
    struct template_dev *dev = (struct template_dev *)filp->private_data;
    ret = copy_from_user(data, buff, count);
    if (ret < 0) {
        printk("copy_from_user failed\r\n");
        return -EINVAL;
    }

    return 0;
}

static struct file_operations template_fops = {
    .owner = THIS_MODULE,
    .open = template_open,
    .write = template_write,
    .release = template_release,
};



static int __init template_init(void) {
    int ret = 0;

    template.major = 0;

    if (template.major > 0 ) {
        template.devid = MKDEV(template.major,0);
        register_chrdev_region(template.devid, TEMPLATE_CNT, TEMPLATE_NAME);
    } else {
        alloc_chrdev_region(&template.devid, 0, TEMPLATE_CNT, TEMPLATE_NAME);
        template.major = MAJOR(template.devid);
        template.minor = MINOR(template.devid);
    }
    printk("major = %d  minor = %d\r\n", template.major, template.minor);

    template.cdev.owner = THIS_MODULE;
    cdev_init(&template.cdev, &template_fops);
    cdev_add(&template.cdev, template.devid,TEMPLATE_CNT);

    template.class = class_create(THIS_MODULE, TEMPLATE_NAME);
    if (IS_ERR(template.class)) {
        return PTR_ERR(template.class);
    }
    template.device = device_create(template.class, NULL, template.devid, NULL, TEMPLATE_NAME);
    if (IS_ERR(template.device)) {
        return PTR_ERR(template.device);
    }

    // template.nd = of_find_node_by_path("/template");
    // if (template.nd == NULL) {
    //     ret = -EINVAL;
    //     goto fail_findnd;
    // }

    return 0;


// fail_findnd:
//     device_destroy(template.class, template.devid);
//     class_destroy(template.class);
//     cdev_del(&template.cdev);
//     unregister_chrdev_region(template.devid, TEMPLATE_CNT);
    return ret;
}

static void __exit template_exit(void) {

    device_destroy(template.class, template.devid);
    class_destroy(template.class);

    cdev_del(&template.cdev);
    unregister_chrdev_region(template.devid, TEMPLATE_CNT);
}

module_init(template_init);
module_exit(template_exit);

MODULE_LICENSE("GPL");
