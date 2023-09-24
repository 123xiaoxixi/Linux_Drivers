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

#define DTSLED_CNT  1

#define LEDOFF  0
#define LEDON   1

struct dtsled_dev {
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
};

static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *IMX6U_SW_MUX_GPIO1_IO03;
static void __iomem *IMX6U_SW_PAD_GPIO1_IO03;
static void __iomem *IMX6U_GPIO1_DR;
static void __iomem *IMX6U_GPIO1_GDIR;

struct dtsled_dev dtsled;

static int led_open(struct inode *inode, struct file *filp) {
    filp->private_data = &dtsled;
    return 0;
}

static int led_release(struct inode *inode, struct file *filp) {
    // unsigned int val = 0;
    struct dtsled_dev *dev = (struct dtsled_dev *)filp->private_data;

    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buff, size_t count, loff_t *loff) {
    struct dtsled_dev *dev = (struct dtsled_dev *)filp->private_data;

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

struct file_operations dtsled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .write = led_write,
};

static int __init dtsled_init(void) {
    int ret = 0;
    const char *str = NULL;
    dtsled.major = 0;
    u32 regdata[10];
    u8 i=0;
    u32 val = 0;

    


    if (dtsled.major) {
        dtsled.devid = MKDEV(dtsled.major, 0);
        ret = register_chrdev_region(dtsled.devid, DTSLED_CNT, "dtsled");
    } else {
        ret = alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, "dtsled");
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(dtsled.devid);
    }
    if (ret < 0) {
        goto fail_devid;
    }
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);
    ret = cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);
    if(ret < 0) 
        goto fail_cdev;
    dtsled.class = class_create(THIS_MODULE, "dtsled");
    if (IS_ERR(dtsled.class)) {
        goto fail_class;
    }
    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, "dtsled");
    if (IS_ERR(dtsled.device)) {
        goto fail_device;
    }

    dtsled.nd = of_find_node_by_path("/alphaled");
    if (dtsled.nd == NULL) {
        ret = -EINVAL;
        goto fail_findnd;
    }
    ret = of_property_read_string(dtsled.nd, "status", &str);
    if (ret) {
        goto fail_rs;
    }
    printk("status:%s\r\n", str);
    ret = of_property_read_string(dtsled.nd, "compatible", &str);
    if (ret) {
        goto fail_rs;
    }
    printk("compatible:%s\r\n", str);

    ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 10);
    if (ret) {
        goto fail_rs;
    }
    for(i=0;i<10;i++) {
        printk("%#x ", regdata[i]);
    }
    printk("\r\n");

    IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
    IMX6U_SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]);
    IMX6U_SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]);
    IMX6U_GPIO1_DR = ioremap(regdata[6], regdata[7]);
    IMX6U_GPIO1_GDIR = ioremap(regdata[8], regdata[9]);

    printk("1\r\n");

    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26);
    val |= (3 << 26);
    writel(val, IMX6U_CCM_CCGR1);
    printk("2\r\n");


    writel(0x5, IMX6U_SW_MUX_GPIO1_IO03);
    writel(0x10b0, IMX6U_SW_PAD_GPIO1_IO03);
    
    val = readl(IMX6U_GPIO1_GDIR);
    val |= 1<<3;
    writel(val, IMX6U_GPIO1_GDIR);

    val = readl(IMX6U_GPIO1_DR);
    val &= ~(1<<3);
    writel(val, IMX6U_GPIO1_DR);
    printk("3\r\n");

    return 0;
fail_rs:
fail_findnd:
    device_destroy(dtsled.class, dtsled.devid);
fail_device:
class_destroy(dtsled.class);
fail_class:
cdev_del(&dtsled.cdev);
fail_cdev:
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);
fail_devid:
    return ret;
}

static void __exit dtsled_exit(void) {
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(IMX6U_SW_MUX_GPIO1_IO03);
    iounmap(IMX6U_SW_PAD_GPIO1_IO03);
    iounmap(IMX6U_GPIO1_DR);
    iounmap(IMX6U_GPIO1_GDIR);

    device_destroy(dtsled.class, dtsled.devid);
    class_destroy(dtsled.class);
    cdev_del(&dtsled.cdev);
    unregister_chrdev_region(dtsled.devid, DTSLED_CNT);
}

module_init(dtsled_init);
module_exit(dtsled_exit);

MODULE_LICENSE("GPL");
