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

#include <linux/wait.h>
#include <linux/ide.h>

#define TEMPLATE_CNT  1
#define TEMPLATE_NAME  "template"

#define LED_ON  1
#define LED_OFF 0

#define KEY_NUM         1
#define KEY0VALUE       0x01
#define INVAKEY         0xFF

struct irq_keydesc {
    int gpio;
    int irq_num;    //中断号
    unsigned char value; //键值
    char name[10];  
    irqreturn_t (*handler)(int, void *);
    struct tasklet_struct tasklet;
};

struct template_dev {
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    struct irq_keydesc irqkey[KEY_NUM];
    struct timer_list timer;
    atomic_t keyvalue;
    atomic_t releasekey;

    wait_queue_head_t r_wait;       //定义等待队列头
    
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

static ssize_t template_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt) {
    int ret = 0;
    unsigned char keyvalue, releasekey;
    struct template_dev *dev = (struct template_dev *)filp->private_data;

    printk("driver: start sleeping()\r\n");
    // wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey));       //使进程休眠
    DECLARE_WAITQUEUE(wait, current);
    add_wait_queue(&dev->r_wait, &wait);
    __set_current_state(TASK_INTERRUPTIBLE);
    schedule(); //切换,切换后进入休眠状态

    //唤醒后开始从这里执行
    if (signal_pending(current)) {  //判断是否是信号唤醒的
        ret = -ERESTARTSYS;
        goto data_err;
    }

    printk("driver: task start running\r\n");

    keyvalue = atomic_read(&dev->keyvalue);
    releasekey = atomic_read(&dev->releasekey);

    if (releasekey) {
        if (keyvalue & 0x80) {
            keyvalue &= ~0x80;
            ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
        } else {
            goto data_err;
        }
        atomic_set(&dev->releasekey, 0);
    } else {
        goto data_err;
    }

data_err:
    __set_current_state(TASK_RUNNING);
    remove_wait_queue(&dev->r_wait, &wait);
    return ret;
}

static struct file_operations template_fops = {
    .owner = THIS_MODULE,
    .open = template_open,
    .write = template_write,
    .release = template_release,
    .read = template_read,
};

irqreturn_t key0_irq_handler(int irq, void *dev_id) {
    // int value = 0;
    struct template_dev *dev = (struct template_dev *)dev_id;
    

    // dev->timer.data = (unsigned long)dev_id;
    // mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
    tasklet_schedule(&dev->irqkey[0].tasklet);
    return IRQ_HANDLED;
}

static void timer_func(unsigned long arg) {
    int value = 0;
    struct template_dev *dev = (struct template_dev *)arg;
    value = gpio_get_value(dev->irqkey[0].gpio);
    if (value == 0) {
        printk("KEY0 press\r\n");
        atomic_set(&dev->keyvalue, dev->irqkey[0].value);
    } else if (value == 1) {
        printk("KEY0 release\r\n");
        atomic_set(&dev->keyvalue, 0x80 | dev->irqkey[0].value);
        atomic_set(&dev->releasekey, 1);

    }

    //唤醒进程
    if (atomic_read(&dev->releasekey)) {
        printk("driver: start wake_up()\r\n");
        wake_up(&dev->r_wait);
        printk("driver: wake_up() finished\r\n");
    }
}

static void key_tasklet(unsigned long data) {
    struct template_dev *dev = (struct template_dev *)data;
    
    printk("key_tasklet\r\n");
    dev->timer.data = (unsigned long)data;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
}

//按键初始化
static int keyio_init(struct template_dev *dev) {
    int ret = 0;
    int i = 0;
    //按键初始化
    dev->nd = of_find_node_by_path("/key");
    if (dev->nd == NULL) {
        ret = -EINVAL;
        goto fail_nd;
    }
    for(i = 0; i < KEY_NUM; i++) {
        dev->irqkey[i].gpio = of_get_named_gpio(dev->nd, "key_gpios", i);
    }
    for(i = 0; i < KEY_NUM; i++) {
        memset(dev->irqkey[i].name, 0, sizeof(dev->irqkey[i].name));
        sprintf(dev->irqkey[i].name, "KEY%d", i);
        gpio_request(dev->irqkey[i].gpio, dev->irqkey[i].name);
        gpio_direction_input(dev->irqkey[i].gpio);

        dev->irqkey[i].irq_num = gpio_to_irq(dev->irqkey[i].gpio);
        // dev->irqkey[i].irq_num = irq_of_parse_and_map(dev->nd, 0);

    }

    dev->irqkey[0].handler = key0_irq_handler;
    dev->irqkey[0].value = KEY0VALUE;
    dev->irqkey[0].tasklet.func = key_tasklet;
    //按键中断初始化
    for(i = 0; i < KEY_NUM; i++) {
        ret = request_irq(dev->irqkey[i].irq_num, dev->irqkey[i].handler, 
                        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, dev->irqkey[i].name, dev);
        if (ret) {
            printk("irq %d request failed\r\n", dev->irqkey[i].irq_num);
            goto fail_irq;
        }
        tasklet_init(&dev->irqkey[i].tasklet, dev->irqkey[i].tasklet.func, (unsigned long)dev);
    }

    init_timer(&dev->timer);
    dev->timer.function = timer_func;
    // dev->timer.data = (unsigned long)dev;

    return 0;
fail_irq:
    for(i = 0; i < KEY_NUM; i++) 
        gpio_free(dev->irqkey[i].gpio);
fail_nd:
    return ret;
}

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

    ret = keyio_init(&template);
    if (ret < 0) {
        goto fail_keyinit;
    }
    atomic_set(&template.keyvalue, INVAKEY);
    atomic_set(&template.releasekey, 0);

    init_waitqueue_head(&template.r_wait);
    // template.nd = of_find_node_by_path("/template");
    // if (template.nd == NULL) {
    //     ret = -EINVAL;
    //     goto fail_findnd;
    // }

    return 0;

fail_keyinit:
// fail_findnd:
    device_destroy(template.class, template.devid);
    class_destroy(template.class);
    cdev_del(&template.cdev);
    unregister_chrdev_region(template.devid, TEMPLATE_CNT);
    return ret;
}

static void __exit template_exit(void) {

    int i = 0;
    //删除定时器
    del_timer_sync(&template.timer);

    for (i = 0; i < KEY_NUM; i++) {
        free_irq(template.irqkey[i].irq_num, &template);
    }
    for(i = 0; i < KEY_NUM; i++) 
        gpio_free(template.irqkey[i].gpio);
    device_destroy(template.class, template.devid);
    class_destroy(template.class);

    cdev_del(&template.cdev);
    unregister_chrdev_region(template.devid, TEMPLATE_CNT);
}

module_init(template_init);
module_exit(template_exit);

MODULE_LICENSE("GPL");
