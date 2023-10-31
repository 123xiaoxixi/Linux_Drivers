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

#include <linux/input.h>

#define TEMPLATE_CNT  1
#define TEMPLATE_NAME  "keyinput"

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

struct keyinput_dev {
    
    struct device_node *nd;
    struct irq_keydesc irqkey[KEY_NUM];
    struct timer_list timer;

    struct work_struct work;

    struct input_dev *inputdev;
    
};

struct keyinput_dev template;


irqreturn_t key0_irq_handler(int irq, void *dev_id) {
    // int value = 0;
    struct keyinput_dev *dev = (struct keyinput_dev *)dev_id;
    

    // dev->timer.data = (unsigned long)dev_id;
    // mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
    // tasklet_schedule(&dev->irqkey[0].tasklet);
    schedule_work(&dev->work);
    return IRQ_HANDLED;
}

static void timer_func(unsigned long arg) {
    int value = 0;
    struct keyinput_dev *dev = (struct keyinput_dev *)arg;
    value = gpio_get_value(dev->irqkey[0].gpio);
    if (value == 0) {       //上报按键值
        printk("KEY0 press\r\n");
        input_event(dev->inputdev, EV_KEY, KEY_0, 1);   //上报按键事件, EV_KEY事件type, KEY_0表示键值code, 1表示按下value
        input_sync(dev->inputdev);       //每次上报后都要sync
    } else if (value == 1) {  //释放按键
        printk("KEY0 release\r\n");
        input_event(dev->inputdev, EV_KEY, KEY_0, 0);
        input_sync(dev->inputdev);
        // atomic_set(&dev->keyvalue, 0x80 | dev->irqkey[0].value);
        // atomic_set(&dev->releasekey, 1);

    }
}

static void key_tasklet(unsigned long data) {
    struct keyinput_dev *dev = (struct keyinput_dev *)data;
    
    printk("key_tasklet\r\n");
    dev->timer.data = (unsigned long)data;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
}

static void key_work(struct work_struct *work) {
    printk("key_work\r\n");

    struct keyinput_dev *dev = container_of(work, struct keyinput_dev, work);
    dev->timer.data = (unsigned long)dev;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10)); 
}

//按键初始化
static int keyio_init(struct keyinput_dev *dev) {
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
        printk("gpio:%d\r\n", dev->irqkey[i].gpio);
    }
    for(i = 0; i < KEY_NUM; i++) {
        memset(dev->irqkey[i].name, 0, sizeof(dev->irqkey[i].name));
        sprintf(dev->irqkey[i].name, "KEY%d", i);
        gpio_request(dev->irqkey[i].gpio, dev->irqkey[i].name);
        gpio_direction_input(dev->irqkey[i].gpio);

        dev->irqkey[i].irq_num = gpio_to_irq(dev->irqkey[i].gpio);
        // dev->irqkey[i].irq_num = irq_of_parse_and_map(dev->nd, 0);       //执行有问题，返回的irq_num为0
        printk("irq_num:%d\r\n", dev->irqkey[i].irq_num);
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
        // tasklet_init(&dev->irqkey[i].tasklet, dev->irqkey[i].tasklet.func, (unsigned long)dev);
        INIT_WORK(&dev->work, key_work);
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

    ret = keyio_init(&template);
    if (ret < 0) {
        goto fail_keyinit;
    }

    //注册input_dev
    template.inputdev = input_allocate_device();
    if (template.inputdev == NULL) {
        ret = -EINVAL;
        goto fail_keyinit;
    }
    template.inputdev->name = TEMPLATE_NAME;
    __set_bit(EV_KEY, template.inputdev->evbit);    //按键事件
    __set_bit(EV_REP, template.inputdev->evbit);    //重复事件

    __set_bit(KEY_0, template.inputdev->keybit);    //按键值
    
    ret = input_register_device(template.inputdev);
    if (ret) {
        goto fail_regi;
    }
    return 0;
    
fail_regi:
    input_free_device(template.inputdev);
fail_keyinit:
    return ret;
}

static void __exit template_exit(void) {

    int i = 0;
    //注销input_dev
    input_unregister_device(template.inputdev);
    input_free_device(template.inputdev);

    //删除定时器
    del_timer_sync(&template.timer);

    for (i = 0; i < KEY_NUM; i++) {
        free_irq(template.irqkey[i].irq_num, &template);
    }
    for(i = 0; i < KEY_NUM; i++) 
        gpio_free(template.irqkey[i].gpio);
    
    
}

module_init(template_init);
module_exit(template_exit);

MODULE_LICENSE("GPL");
