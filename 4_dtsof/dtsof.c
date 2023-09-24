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

#if 0
backlight {
    compatible = "pwm-backlight";
    pwms = <&pwm1 0 5000000>;
    brightness-levels = <0 4 8 16 32 64 128 255>;
    default-brightness-level = <6>;
    status = "okay";
};
#endif

u32 *brival;
static int __init dtsof_init(void) {
    int ret = 0;
    struct device_node *bl_nd = NULL;
    struct property *comppro = NULL;
    const char *str = NULL;
    u32 value = 0;
    
    u32 elenum;
    u8 i = 0;

    bl_nd = of_find_node_by_path("/backlight");
    if (bl_nd == NULL) {
        ret = -EINVAL;
        goto fail_findnd;
    }
    comppro = of_find_property(bl_nd, "compatible", NULL);
    if (comppro == NULL) {
        ret = -EINVAL;
        goto fail_findpro;
    } else {
        printk("compatible:%s\r\n", (char *)comppro->value);
    }
    ret = of_property_read_string(bl_nd, "status", &str);
    if (ret) {
        goto fail_rs;
    }
    printk("status:%s\r\n", str);

    ret = of_property_read_u32(bl_nd, "default-brightness-level", &value);
    if (ret) {
        goto fail_u32;
    }
    printk("default-brightness-level:%d\r\n", value);

    elenum = of_property_count_elems_of_size(bl_nd, "brightness-levels", sizeof(u32));
    if (elenum < 0) {
        goto fail_ps;
    }
    printk("count:%d\r\n", elenum);
    brival = kmalloc(elenum*sizeof(u32), GFP_KERNEL);
    if (!brival) {
        ret = - EINVAL;
        goto fail_malloc;
    }
    ret = of_property_read_u32_array(bl_nd, "brightness-levels", brival, elenum);
    if (ret < 0) {
        goto fail_arr;
    }
    printk("brightness-levels[8]:");
    for (i = 0; i<elenum; i++)
        printk("%d ",brival[i]);
    printk("\r\n");

    return 0;

fail_arr:
fail_malloc:
fail_ps:
fail_u32:
fail_rs:
fail_findpro:
fail_findnd:
    return ret;
}

static void __exit dtsof_exit(void) {
    kfree(brival);
}

module_init(dtsof_init);
module_exit(dtsof_exit);
MODULE_LICENSE("GPL");
