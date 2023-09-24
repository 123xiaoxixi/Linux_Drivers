#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define CHRDEVBASE_MAJOR  (200)
#define CHRDEVBASE_NAME  "chrdevbase"

static char readbuf[100];
static char writebuf[100];

static char kerneldata[] = {"kernel data."};


static int chrdevbase_open(struct inode *inode, struct file *filp) {
    printk("chrdevbase_open() start\r\n");
    return 0;
}

static int chrdevbase_release(struct inode *inode, struct file *filp) {
    printk("chrdevbase_release() start\r\n");
    return 0;
}

static ssize_t chrdevbase_read(struct file *filp, __user char *buf, size_t count, loff_t *ppos) {
    int ret = 0;
    printk("chrdevbase_read() start\r\n");
    memcpy(readbuf, kerneldata, sizeof(kerneldata));
    ret = copy_to_user(buf, readbuf, count);
    if (ret < 0) {}
    return 0;
}

static ssize_t chrdevbase_write(struct file *filp, const __user char *buf, size_t count, loff_t *ppos) {
    int ret = 0;
    printk("chrdevbase_write() start\r\n");
    ret = copy_from_user(writebuf, buf, count);
    if (ret == 0) {
        printk("recieve data: %s\r\n", writebuf);
    }
    return 0;
}

static struct file_operations chrdevbase_fops = {
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .release = chrdevbase_release,
    .read = chrdevbase_read,
    .write = chrdevbase_write,
};

static int __init chrdevbase_init(void) {
    int ret = 0;
    printk("chrdevbase_init() start\r\n");
    ret = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);
    if (ret < 0) {
        printk("register_chrdev() failed\r\n");
    }
    return 0;

}

static void __exit chrdevbase_exit(void) {
    printk("chrdevbase_exit() start\r\n");
    unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);

}

module_init(chrdevbase_init);
module_exit(chrdevbase_exit);

MODULE_LICENSE("GPL");