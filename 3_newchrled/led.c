#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define NEWCHRLED_NAME  "newchrled"

#define CCM_CCGR1_BASE          (0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE  (0x020E0068)
#define SW_PAD_GPIO1_IO03_BASE  (0x020E02F4)
#define GPIO1_DR_BASE           (0x0209C000)
#define GPIO1_GDIR_BASE         (0x0209C004)

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

static int __init led_init(void) {
    int ret = 0;
    unsigned int val = 0;

    IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
    IMX6U_SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
    IMX6U_SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
    IMX6U_GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
    IMX6U_GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);

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

static void __exit led_exit(void) {
    unsigned int val = 0;
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
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
