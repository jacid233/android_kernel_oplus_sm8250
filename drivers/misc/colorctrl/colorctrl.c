/************************************************************
 * Copyright 2020 OPLUS Mobile Comm Corp., Ltd.
 * All rights reserved.
 *
 * Description  : driver for oplus color ctrl
 * History      : ( ID, Date, Author, Description)
 * Author       : Zengpeng.Chen@BSP.TP.Misc
 * Data         : 2020/08/06
 ************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/regulator/consumer.h>
#include <soc/oppo/oppo_project.h>
#include "colorctrl.h"

#define DRIVER_NAME "color-ctrl"

static const struct of_device_id color_ctrl_match_table[] = {
    {.compatible = "oplus,color-ctrl"},
    {}
};

static int colorctrl_power_control(struct color_ctrl_hw_resource *hw_res, int vol, bool on)
{
    int ret = 0;

    if (on) {
        if (!IS_ERR_OR_NULL(hw_res->vm)) {
            COLOR_INFO("enable the vm power to voltage : %d.", vol);
            ret = regulator_set_voltage(hw_res->vm, vol, vol);
            if (ret) {
                COLOR_INFO("Regulator vm set voltage failed, ret = %d", ret);
                return ret;
            }
            ret = regulator_enable(hw_res->vm);
            if (ret) {
                COLOR_INFO("Regulator vm enable failed, ret = %d", ret);
                return ret;
            }
        }
    } else {
        if (!IS_ERR_OR_NULL(hw_res->vm)) {
            COLOR_INFO("disable the vm power.");
            ret = regulator_disable(hw_res->vm);
            if (ret) {
                COLOR_INFO("Regulator vm disable failed, ret = %d", ret);
                return ret;
            }
        }
    }

    return ret;
};

static void blue_color_control_operation(struct color_ctrl_device *cd, bool recharge)
{
    struct color_ctrl_hw_resource *hw_res = NULL;
    int ret = 0;

    if (!cd || !cd->hw_res) {
        COLOR_INFO("no dev or resources find, return.");
        return;
    }

    hw_res = cd->hw_res;

    switch (cd->temp_status) {
    case LOW_TEMP : {
        ret = colorctrl_power_control(cd->hw_res, 1000000, true);
        if (ret) {
            COLOR_INFO("enable power failed.");
            goto OUT;
        }
        break;
    }
    case ROOM_TEMP : {
        ret = colorctrl_power_control(cd->hw_res, 800000, true);
        if (ret) {
            COLOR_INFO("enable power failed.");
            goto OUT;
        }
        break;
    }
    case HIGH_TEMP : {
        ret = colorctrl_power_control(cd->hw_res, 600000, true);
        if (ret) {
            COLOR_INFO("enable power failed.");
            goto OUT;
        }
        break;
    }
    case ABNORMAL_TEMP : {
        COLOR_INFO("abnormal temperature occur, do not change color.");
        ret = -1;
        goto OUT;
        break;
    }
    default :
        break;
    }

    ret = gpio_direction_output(hw_res->si_in_1_gpio, GPIO_HIGH);
    ret |= gpio_direction_output(hw_res->si_in_2_gpio, GPIO_LOW);
    ret |= gpio_direction_output(hw_res->sleep_en_gpio, GPIO_HIGH);
    if (recharge) {
        msleep(3000);
    } else {
        msleep(10000);
    }
    ret |= gpio_direction_output(hw_res->sleep_en_gpio, GPIO_LOW);
    if (ret) {
        COLOR_INFO("Config gpio status failed.");
    }
    ret |= colorctrl_power_control(cd->hw_res, 600000, false);

OUT:
    COLOR_INFO("%s color to blue %s.", recharge ? "recharge" : "change", ret ? "failed" : "success");


    return;
};

static void transparent_color_control_operation(struct color_ctrl_device *cd, bool recharge)
{
    struct color_ctrl_hw_resource *hw_res = NULL;
    int ret = 0;

    if (!cd || !cd->hw_res) {
        COLOR_INFO("no dev or resources find, return.");
        return;
    }

    hw_res = cd->hw_res;

    switch (cd->temp_status) {
    case LOW_TEMP : {
        ret = colorctrl_power_control(cd->hw_res, 1200000, true);
        if (ret) {
            COLOR_INFO("enable power failed.");
            goto OUT;
        }
        break;
    }
    case ROOM_TEMP : {
        ret = colorctrl_power_control(cd->hw_res, 1000000, true);
        if (ret) {
            COLOR_INFO("enable power failed.");
            goto OUT;
        }
        break;
    }
    case HIGH_TEMP : {
        ret = colorctrl_power_control(cd->hw_res, 600000, true);
        if (ret) {
            COLOR_INFO("enable power failed.");
            goto OUT;
        }
        break;
    }
    case ABNORMAL_TEMP : {
        COLOR_INFO("abnormal temperature occur, do not change color.");
        goto OUT;
        break;
    }
    default :
        break;
    }

    ret = gpio_direction_output(hw_res->si_in_1_gpio, GPIO_LOW);
    ret |= gpio_direction_output(hw_res->si_in_2_gpio, GPIO_HIGH);
    ret |= gpio_direction_output(hw_res->sleep_en_gpio, GPIO_HIGH);
    if (recharge) {
        msleep(3000);
    } else {
        msleep(10000);
    }
    ret |= gpio_direction_output(hw_res->sleep_en_gpio, GPIO_LOW);
    if (ret) {
        COLOR_INFO("Config gpio status failed.");
    }
    ret |= colorctrl_power_control(cd->hw_res, 600000, false);

OUT:
    COLOR_INFO("%s color to transparent %s.", recharge ? "recharge" : "change", ret ? "failed" : "success");

    return;
};

static void light_blue_color_control_operation(struct color_ctrl_device *cd)
{
    struct color_ctrl_hw_resource *hw_res = NULL;
    int ret = 0;

    if (!cd || !cd->hw_res) {
        COLOR_INFO("No dev or resources find, return.");
        return;
    }

    hw_res = cd->hw_res;

    ret = gpio_direction_output(hw_res->si_in_1_gpio, GPIO_HIGH);
    ret |= gpio_direction_output(hw_res->si_in_2_gpio, GPIO_HIGH);
    ret |= gpio_direction_output(hw_res->sleep_en_gpio, GPIO_HIGH);
    msleep(10000);
    ret |= gpio_direction_output(hw_res->sleep_en_gpio, GPIO_LOW);
    if (ret) {
        COLOR_INFO("Config gpio status failed.");
    }

    COLOR_INFO("change color to light blue %s.", ret ? "failed" : "success");

    return;
};

static ssize_t proc_color_control_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    char buf[8] = {0};
    int temp = 0;
    struct color_ctrl_device *cd = PDE_DATA(file_inode(file));

    if (!cd || count > 2) {
        return count;
    }

    if (copy_from_user(buf, buffer, count)) {
        COLOR_INFO("read proc input error.");
        return count;
    }
    sscanf(buf, "%d", &temp);
    COLOR_INFO("write value: %d.", temp);

    mutex_lock(&cd->rw_lock);

    if (0 <= temp && temp <= 4) {
        cd->color_status = temp;
    } else {
        COLOR_INFO("not support change color type.");
        mutex_unlock(&cd->rw_lock);
        return count;
    }

    switch (temp) {
    case LIGHT_BLUE : {
        light_blue_color_control_operation(cd);
        break;
    }
    case BLUE : {
        blue_color_control_operation(cd, false);
        break;
    }
    case TRANSPARENT : {
        transparent_color_control_operation(cd, false);
        break;
    }
    case RECHARGE_BLUE : {
        blue_color_control_operation(cd, true);
        break;
    }
    case RECHARGE_TRANSPARENT: {
        transparent_color_control_operation(cd, true);
        break;
    }
    default :
        COLOR_INFO("not support color status.");
        break;
    }

    mutex_unlock(&cd->rw_lock);

    return count;
};

static ssize_t proc_color_control_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    ssize_t ret = 0;
    char page[PAGESIZE] = {0};
    struct color_ctrl_device *cd = PDE_DATA(file_inode(file));

    snprintf(page, PAGESIZE - 1, "%d\n", cd->color_status);
    ret = simple_read_from_buffer(user_buf, count, ppos, page, strlen(page));

    COLOR_INFO("read value: %d.", cd->color_status);

    return ret;
};

static const struct file_operations proc_color_control_ops = {
    .read  = proc_color_control_read,
    .write = proc_color_control_write,
    .open  = simple_open,
    .owner = THIS_MODULE,
};

static ssize_t proc_temperature_control_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
    char buf[8] = {0};
    int temp = 0;
    struct color_ctrl_device *cd = PDE_DATA(file_inode(file));

    if (!cd || count > 2) {
        return count;
    }

    if (copy_from_user(buf, buffer, count)) {
        COLOR_INFO("read proc input error.");
        return count;
    }
    sscanf(buf, "%d", &temp);
    COLOR_INFO("write value: %d.", temp);

    mutex_lock(&cd->rw_lock);

    if (0 <= temp && temp <= 3) {
        cd->temp_status = temp;
    } else {
        COLOR_INFO("not support temperature type");
    }

    if (cd->temp_status == ABNORMAL_TEMP) {
        COLOR_INFO("current temperature is abnormal !");
    }

    mutex_unlock(&cd->rw_lock);

    return count;
};

static ssize_t proc_temperature_control_read(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
{
    ssize_t ret = 0;
    char page[PAGESIZE] = {0};
    struct color_ctrl_device *cd = PDE_DATA(file_inode(file));

    snprintf(page, PAGESIZE - 1, "%d\n", cd->temp_status);
    ret = simple_read_from_buffer(user_buf, count, ppos, page, strlen(page));

    COLOR_INFO("read value: %d.", cd->temp_status);

    return ret;
};

static const struct file_operations proc_temperature_control_ops = {
    .read  = proc_temperature_control_read,
    .write = proc_temperature_control_write,
    .open  = simple_open,
    .owner = THIS_MODULE,
};

static int init_color_ctrl_proc(struct color_ctrl_device *cd)
{
    int ret = 0;
    struct proc_dir_entry *prEntry_cr = NULL;
    struct proc_dir_entry *prEntry_tmp = NULL;

    COLOR_INFO("entry");

    //proc files-step1:/proc/colorctrl
    prEntry_cr = proc_mkdir("colorctrl", NULL);
    if (prEntry_cr == NULL) {
        ret = -ENOMEM;
        COLOR_INFO("Couldn't create color ctrl proc entry");
    }

    //proc files-step2:/proc/touchpanel/color_ctrl (color control interface)
    prEntry_tmp = proc_create_data("color_ctrl", 0644, prEntry_cr, &proc_color_control_ops, cd);
    if (prEntry_tmp == NULL) {
        ret = -ENOMEM;
        COLOR_INFO("Couldn't create color_ctrl proc entry");
    }

    //proc files-step2:/proc/touchpanel/temperature (color control temperature interface)
    prEntry_tmp = proc_create_data("temperature", 0644, prEntry_cr, &proc_temperature_control_ops, cd);
    if (prEntry_tmp == NULL) {
        ret = -ENOMEM;
        COLOR_INFO("Couldn't create temperature proc entry");
    }

    cd->prEntry_cr = prEntry_cr;

    return ret;
};

static int color_ctrl_parse_dt(struct device *dev, struct color_ctrl_device *cd)
{
    int ret = 0, i = 0;
    int prj_id = 0;
    struct device_node *np = dev->of_node;
    struct color_ctrl_hw_resource *hw_res = cd->hw_res;

    if(!np || !hw_res) {
        COLOR_INFO("Don't has device of_node.");
        return -1;
    }

    cd->project_num  = of_property_count_u32_elems(np, "platform_support_project");
    if (cd->project_num <= 0) {
        COLOR_INFO("project not specified, need to config the support project.");
        return -1;
    } else {
        ret = of_property_read_u32_array(np, "platform_support_project", cd->platform_support_project, cd->project_num);
        if (ret) {
            COLOR_INFO("platform_support_project not specified.");
            return -1;
        }
        prj_id = get_project();
        for (i = 0; i < cd->project_num; i++) {
            if (prj_id == cd->platform_support_project[i]) {
                COLOR_INFO("driver match the project.");
                break;
            }
        }
        if (i == cd->project_num) {
            COLOR_INFO("driver does not match the project.");
            return -1;
        }
    }

    hw_res->sleep_en_gpio = of_get_named_gpio(np, "gpio-sleep_en", 0);
    if ((!gpio_is_valid(hw_res->sleep_en_gpio))) {
        COLOR_INFO("parse gpio-sleep_en fail.");
        return -1;
    }

    hw_res->si_in_1_gpio = of_get_named_gpio(np, "gpio-si_in_1", 0);
    if ((!gpio_is_valid(hw_res->si_in_1_gpio))) {
        COLOR_INFO("parse gpio-si_in_1 fail.");
        return -1;
    }

    hw_res->si_in_2_gpio = of_get_named_gpio(np, "gpio-si_in_2", 0);
    if ((!gpio_is_valid(hw_res->si_in_2_gpio))) {
        COLOR_INFO("parse gpio-si_in_2 fail.");
        return -1;
    }

    hw_res->vm = devm_regulator_get(dev, "vm");
    if (IS_ERR_OR_NULL(hw_res->vm)) {
        COLOR_INFO("Regulator vm get failed.");
        return -1;
    }

    COLOR_INFO("Parse dt ok, get gpios:[sleep_en_gpio:%d si_in_1_gpio:%d si_in_2_gpio:%d]",
        hw_res->sleep_en_gpio, hw_res->si_in_1_gpio, hw_res->si_in_2_gpio);

    return 0;
}

static int color_ctrl_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct color_ctrl_hw_resource *hw_res = NULL;
    struct color_ctrl_device *cd = NULL;

    COLOR_INFO("start to probe color ctr driver.");

    /*malloc memory for hardware resource */
    if(pdev->dev.of_node) {
        hw_res = devm_kzalloc(&pdev->dev, sizeof(struct color_ctrl_hw_resource), GFP_KERNEL);
        if(!hw_res) {
            ret = -ENOMEM;
            COLOR_INFO("Malloc memory for hardware resoure fail.");
            goto PROBE_ERR;
        }
    } else {
        hw_res = pdev->dev.platform_data;
    }

    /*malloc memory for color ctrl device*/
    cd = devm_kzalloc(&pdev->dev, sizeof(struct color_ctrl_device), GFP_KERNEL);
    if(!cd) {
        COLOR_INFO("Malloc memory for color ctr device fail.");
        ret = -ENOMEM;
        goto PROBE_ERR;
    }

    cd->hw_res = hw_res;

    ret = color_ctrl_parse_dt(&pdev->dev, cd);
    if (ret) {
        COLOR_INFO("parse dts fail.");
        goto PROBE_ERR;
    }

    /*Request and config these gpios*/
    if (gpio_is_valid(hw_res->sleep_en_gpio)) {
        ret = devm_gpio_request(&pdev->dev, hw_res->sleep_en_gpio, "sleep_en_gpio");
        if(ret) {
            COLOR_INFO("request sleep_en_gpio fail.");
            goto PROBE_ERR;
        } else {
            /*Enable the sleep en gpio.*/
            ret = gpio_direction_output(hw_res->sleep_en_gpio, GPIO_LOW);
            if(ret) {
                COLOR_INFO("Config sleep_en_gpio gpio output direction fail.");
                goto PROBE_ERR;
            }
        }
    } else {
        hw_res->sleep_en_gpio = -EINVAL;
        COLOR_INFO("sleep_en_gpio gpio is invalid.");
        goto PROBE_ERR;
    }

    if (gpio_is_valid(hw_res->si_in_1_gpio)) {
        ret = devm_gpio_request(&pdev->dev, hw_res->si_in_1_gpio, "si_in_1_gpio");
        if(ret) {
            COLOR_INFO("request si_in_1_gpio fail.");
            goto PROBE_ERR;
        } else {
            ret = gpio_direction_output(hw_res->si_in_1_gpio, GPIO_HIGH);
            if(ret) {
                COLOR_INFO("Config si_in_1_gpio gpio output direction fail.");
                goto PROBE_ERR;
            }
        }
    } else {
        hw_res->si_in_1_gpio = -EINVAL;
        COLOR_INFO("si_in_1_gpio gpio is invalid.");
        goto PROBE_ERR;
    }

    if (gpio_is_valid(hw_res->si_in_2_gpio)) {
        ret = devm_gpio_request(&pdev->dev, hw_res->si_in_2_gpio, "si_in_2_gpio");
        if(ret) {
            COLOR_INFO("request si_in_2_gpio fail.");
            goto PROBE_ERR;
        } else {
            ret = gpio_direction_output(hw_res->si_in_2_gpio, GPIO_LOW);
            if(ret) {
                COLOR_INFO("Config si_in_2_gpio gpio output direction fail.");
                goto PROBE_ERR;
            }
        }
    } else {
        hw_res->si_in_2_gpio = -EINVAL;
        COLOR_INFO("si_in_2_gpio gpio is invalid.");
        goto PROBE_ERR;
    }

    cd->pdev = pdev;
    cd->dev = &pdev->dev;
    cd->color_status = LIGHT_BLUE;
    cd->temp_status = ROOM_TEMP;
    mutex_init(&cd->rw_lock);
    platform_set_drvdata(pdev, cd);

    ret = init_color_ctrl_proc(cd);
    if (ret) {
        COLOR_INFO("creat color ctrl proc error.");
        goto PROBE_ERR;
    }

    COLOR_INFO("color ctrl device probe : normal end.");
    return ret;

PROBE_ERR:
    COLOR_INFO("color ctrl device probe error.");
    return ret;
}

static int color_ctrl_remove(struct platform_device *dev)
{
    struct color_ctrl_device *cd = platform_get_drvdata(dev);

    COLOR_INFO("start remove the color ctrl platform dev.");

    if (cd) {
        proc_remove(cd->prEntry_cr);
        cd->prEntry_cr = NULL;
    }

    return 0;
}

static struct platform_driver color_ctrl_driver = {
    .probe = color_ctrl_probe,
    .remove = color_ctrl_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name  = DRIVER_NAME,
        .of_match_table = color_ctrl_match_table,
    },
};

module_platform_driver(color_ctrl_driver);

MODULE_DESCRIPTION("Color Ctrl Driver Module");
MODULE_AUTHOR("Zengpeng.Chen");
MODULE_LICENSE("GPL v2");
