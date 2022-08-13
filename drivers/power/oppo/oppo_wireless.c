/************************************************************************************
** File:  oppo_p922x.c
** VENDOR_EDIT
** Copyright (C), 2008-2012, OPPO Mobile Comm Corp., Ltd
** 
** Description: 
**      for wpc charge
** 
** Version: 1.0
** Date created: 2019-03-16
** Author: huangtongfeng
** 
** --------------------------- Revision History: ------------------------------------------------------------
* <version>       <date>        <author>              			<desc>
* Revision 1.0    2019-03-16   huangtongfeng  		Created for wpc charge
************************************************************************************************************/

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>

#include <linux/debugfs.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/rtc.h>
#include <soc/oppo/device_info.h>

#include "oppo_vooc.h"
#include "oppo_gauge.h"
#include "oppo_charger.h"
#include "oppo_wpc.h"

static struct oppo_wpc_chip *g_wpc_chip = NULL;

int send_msg_timer = 0;

#define DEBUG_BY_FILE_OPS

int oppo_wpc_get_status(void)
{
    if(!g_wpc_chip) {
        return OPPO_WPC_NO_CHARGER_MODE;
    }

    if (g_wpc_chip->charge_online == true) {
        if (g_wpc_chip->fastchg_ing == true)
            return OPPO_WPC_FASTCHG_MODE;
        else
            return OPPO_WPC_NORMAL_MODE;
    }

    return OPPO_WPC_NO_CHARGER_MODE;
}

int oppo_wpc_set_wpc_enable(bool enable)
{
    if(!g_wpc_chip) {
		chg_err("g_wpc_chip is not ready\n");
        return 0;
    }
    g_wpc_chip->wpc_chg_enable = !!enable;
    oppo_wpc_set_wpc_en_val(enable);
    return 0;
}

int oppo_wpc_set_wired_otg_enable(bool enable)
{
    if(!g_wpc_chip) {
		chg_err("g_wpc_chip is not ready\n");
        return 0;
    }
    g_wpc_chip->wired_otg_enable = !!enable;
    oppo_wpc_set_wrx_en_value(enable);
    oppo_wpc_set_otg_en_val(enable);
    oppo_wpc_set_booster_en_val(enable);
    return 0;
}

int oppo_wpc_set_wpc_otg_enable(bool enable)
{
    if(!g_wpc_chip) {
		chg_err("g_wpc_chip is not ready\n");
        return 0;
    }
    g_wpc_chip->wpc_otg_switch = !!enable;
    if (enable == true) {
        oppo_chg_disable_charge();
        oppo_wpc_set_wrx_otg_value(enable);
        oppo_chg_set_charger_otg_enable(!!enable);
        msleep(1000);
        g_wpc_chip->wpc_ops->set_dischg_enable(g_wpc_chip , enable);
    } else {
        g_wpc_chip->wpc_ops->set_dischg_enable(g_wpc_chip , enable);
        msleep(100);
        oppo_chg_set_charger_otg_enable(!!enable);
        oppo_wpc_set_wrx_otg_value(enable);
    }
    return 0;
}

int oppo_wpc_set_wired_chg_enable(bool enable)
{
    if(!g_wpc_chip) {
		chg_err("g_wpc_chip is not ready\n");
        return 0;
    }
    g_wpc_chip->wired_chg_enable = !!enable;
    oppo_wpc_set_wpc_enable(enable);
}

int oppo_wpc_set_switch_default_mode(void)
{
    oppo_wpc_set_wpc_enable(true);
    oppo_wpc_set_wired_otg_enable(false);
    oppo_wpc_set_wpc_otg_enable(false);

    return 0;
}

int oppo_wpc_switch_mode(int work_mode, bool enable)
{
    if(!g_wpc_chip) {
		chg_err("g_wpc_chip is not ready\n");
        return 0;
    }
    g_wpc_chip->work_mode = work_mode;
	switch (work_mode){
		case OPPO_WPC_SWITCH_DEFAULT_MODE :
            oppo_wpc_set_switch_default_mode();
			break;
		case OPPO_WPC_SWITCH_WIRED_CHG_MODE :
            oppo_wpc_set_wired_chg_enable(enable);
            if (enable) {
                oppo_wpc_set_wired_otg_enable(false);
                g_wpc_chip->wpc_ops->set_dischg_enable(g_wpc_chip , enable);
                msleep(100);
                oppo_chg_set_charger_otg_enable(!!enable);
                oppo_wpc_set_wrx_otg_value(enable);
            } else {
                if (g_wpc_chip->wpc_otg_switch) {
                    oppo_chg_disable_charge();
                    oppo_wpc_set_wrx_otg_value(enable);
                    oppo_chg_set_charger_otg_enable(!!enable);
                    msleep(1000);
                    g_wpc_chip->wpc_ops->set_dischg_enable(g_wpc_chip , enable);
                }
            }
			break;
		case OPPO_WPC_SWITCH_WPC_CHG_MODE :
			break;
		case OPPO_WPC_SWITCH_WIRED_OTG_MODE :
            oppo_wpc_set_wired_otg_enable(enable);
			break;
		case OPPO_WPC_SWITCH_WPC_OTG_MODE :
            oppo_wpc_set_wpc_otg_enable(enable);
			break;
		case OPPO_WPC_SWITCH_OTHER_MODE :
            oppo_wpc_set_switch_default_mode();
			break;
	}
}

static int oppo_wpc_en_gpio_init(struct oppo_wpc_chip *chip)
{
    printk(KERN_ERR "[OPPO_CHG][%s]: tongfeng test start!\n", __func__);

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: smb2_chg not ready!\n", __func__);
		return -EINVAL;
	}

	chip->wpc_gpios.pinctrl = devm_pinctrl_get(chip->dev);

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)) {
		chg_err("get wpc pinctrl fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wpc_en_active = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wpc_en_active");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wpc_en_active)) {
		chg_err("get wpc_en_active fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wpc_en_sleep = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wpc_en_sleep");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wpc_en_sleep)) {
		chg_err("get wpc_en_sleep fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wpc_en_default = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wpc_en_default");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wpc_en_default)) {
		chg_err("get idt_en_default fail\n");
		return -EINVAL;
	}
    printk(KERN_ERR "[OPPO_CHG][%s]: tongfeng test start 555!\n", __func__);

	if (chip->wpc_gpios.wpc_en_gpio > 0) {
		gpio_direction_output(chip->wpc_gpios.wpc_en_gpio, 0);
	}

	pinctrl_select_state(chip->wpc_gpios.pinctrl, chip->wpc_gpios.wpc_en_default);
    printk(KERN_ERR "[OPPO_CHG][%s]: tongfeng test end!\n", __func__);

	return 0;
}

static void oppo_wpc_set_wpc_en_val(int value)  // 0  active, 1 inactive
{
    struct oppo_wpc_chip *chip = g_wpc_chip;

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: wpc not ready!\n", __func__);
		return;
	}

	if (chip->wpc_gpios.wpc_en_gpio <= 0) {
		chg_err("wpc_en_gpio not exist, return\n");
		return;
	}

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wpc_en_active)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wpc_en_sleep)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wpc_en_default)) {
		chg_err("pinctrl null, return\n");
		return;
	}

	if (value) {
		gpio_direction_output(chip->wpc_gpios.wpc_en_gpio, 1);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.wpc_en_active);
	} else {
		gpio_direction_output(chip->wpc_gpios.wpc_en_gpio, 0);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.wpc_en_default);
	}
	chg_err("set value:%d, gpio_val:%d\n", 
		value, gpio_get_value(chip->wpc_gpios.wpc_en_gpio));
}

int oppo_wpc_get_wpc_en_val(void)
{
    struct oppo_wpc_chip *chip = g_wpc_chip;

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: wpc not ready!\n", __func__);
		return -1;
	}

	if (chip->wpc_gpios.wpc_en_gpio <= 0) {
		chg_err("wpc_en_gpio not exist, return\n");
		return -1;
	}

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wpc_en_active)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wpc_en_sleep)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wpc_en_default)) {
		chg_err("pinctrl null, return\n");
		return -1;
	}

	return gpio_get_value(chip->wpc_gpios.wpc_en_gpio);
}

static irqreturn_t oppo_wpc_connect_int_handler(int irq, void *dev_id)
{
	struct oppo_wpc_chip *chip = dev_id;

	if (!chip) {
		chg_err(" oppo_wpc_chip is NULL\n");
	} else {
		schedule_delayed_work(&chip->wpc_connect_int_work, 0);
	}
	return IRQ_HANDLED;
}

static void oppo_wpc_con_irq_init(struct oppo_wpc_chip *chip)
{
	chip->wpc_gpios.wpc_con_irq = gpio_to_irq(chip->wpc_gpios.wpc_con_gpio);
    pr_err("WPC %s chip->wpc_gpios.wpc_con_irq[%d]\n",__func__, chip->wpc_gpios.wpc_con_irq);
}

static void oppo_wpc_con_eint_register(struct oppo_wpc_chip *chip)
{
	int ret = 0;

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: wpc not ready!\n", __func__);
		return;
	}

	ret = devm_request_threaded_irq(chip->dev, chip->wpc_gpios.wpc_con_irq,
			NULL, oppo_wpc_connect_int_handler, 
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "wpc_con_int", chip);
	if (ret < 0) {
		chg_err("Unable to request wpc_con_int irq: %d\n", ret);
	}
    printk(KERN_ERR "%s: !!!!! irq register\n", __FUNCTION__);

	ret = enable_irq_wake(chip->wpc_gpios.wpc_con_irq);
	if (ret != 0) {
		chg_err("enable_irq_wake: wpc_con_irq failed %d\n", ret);
	}
}

static int oppo_wpc_con_gpio_init(struct oppo_wpc_chip *chip)
{
	chip->wpc_gpios.pinctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)) {
		chg_err("get pinctrl fail\n");
		return -EINVAL;
	}

	//idt_con
	chip->wpc_gpios.wpc_con_active = 
			pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wpc_connect_active");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wpc_con_active)) {
		chg_err("get wpc_con_active fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wpc_con_sleep = 
			pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wpc_connect_sleep");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wpc_con_sleep)) {
		chg_err("get wpc_con_sleep fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wpc_con_default = 
			pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wpc_connect_default");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wpc_con_default)) {
		chg_err("get wpc_con_default fail\n");
		return -EINVAL;
	}
	

	if (chip->wpc_gpios.wpc_con_gpio > 0) {
		gpio_direction_input(chip->wpc_gpios.wpc_con_gpio);
	}

	pinctrl_select_state(chip->wpc_gpios.pinctrl, chip->wpc_gpios.wpc_con_active);

	return 0;
}

static int oppo_wpc_vbat_en_gpio_init(struct oppo_wpc_chip *chip)
{
	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return -EINVAL;
	}

	chip->wpc_gpios.pinctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)) {
		chg_err("get pinctrl fail\n");
		return -EINVAL;
	}

	//vbat_en
	chip->wpc_gpios.vbat_en_active = 
			pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "vbat_en_active");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.vbat_en_active)) {
		chg_err("get vbat_en_active fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.vbat_en_sleep = 
			pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "vbat_en_sleep");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.vbat_en_sleep)) {
		chg_err("get vbat_en_sleep fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.vbat_en_default = 
			pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "vbat_en_default");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.vbat_en_default)) {
		chg_err("get vbat_en_default fail\n");
		return -EINVAL;
	}

	gpio_direction_output(chip->wpc_gpios.vbat_en_gpio, 0);
	pinctrl_select_state(chip->wpc_gpios.pinctrl,
			chip->wpc_gpios.vbat_en_sleep);

	return 0;
}

void oppo_wpc_set_vbat_en_val(struct oppo_wpc_chip *chip, int value)
{    
	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return;
	}

	if (chip->wpc_gpios.vbat_en_gpio <= 0) {
		chg_err("vbat_en_gpio not exist, return\n");
		return;
	}

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.vbat_en_active)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.vbat_en_sleep)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.vbat_en_default)) {
		chg_err("pinctrl null, return\n");
		return;
	}

	if (value) {
		gpio_direction_output(chip->wpc_gpios.vbat_en_gpio, 1);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.vbat_en_default);
	} else {
		gpio_direction_output(chip->wpc_gpios.vbat_en_gpio, 0);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.vbat_en_sleep);
	}

	chg_err("set value:%d, gpio_val:%d\n", 
		value, gpio_get_value(chip->wpc_gpios.vbat_en_gpio));
}

int oppo_wpc_get_vbat_en_val(struct oppo_wpc_chip *chip)
{
	if (chip->wpc_gpios.vbat_en_gpio <= 0) {
		chg_err("vbat_en_gpio not exist, return\n");
		return -1;
	}

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.vbat_en_active)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.vbat_en_sleep)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.vbat_en_default)) {
		chg_err("pinctrl null, return\n");
		return -1;
	}

	return gpio_get_value(chip->wpc_gpios.vbat_en_gpio);
}

static int oppo_wpc_booster_en_gpio_init(struct oppo_wpc_chip *chip)
{

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return -EINVAL;
	}

	chip->wpc_gpios.pinctrl = devm_pinctrl_get(chip->dev);
	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)) {
		chg_err("get pinctrl fail\n");
		return -EINVAL;
	}

	//booster_en
	chip->wpc_gpios.booster_en_active = 
			pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "booster_en_active");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.booster_en_active)) {
		chg_err("get booster_en_active fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.booster_en_sleep = 
			pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "booster_en_sleep");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.booster_en_sleep)) {
		chg_err("get booster_en_sleep fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.booster_en_default = 
			pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "booster_en_default");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.booster_en_default)) {
		chg_err("get booster_en_default fail\n");
		return -EINVAL;
	}

	gpio_direction_output(chip->wpc_gpios.booster_en_gpio, 0);
	pinctrl_select_state(chip->wpc_gpios.pinctrl,
			chip->wpc_gpios.booster_en_sleep);

	chg_err("gpio_val:%d\n", gpio_get_value(chip->wpc_gpios.booster_en_gpio));

	return 0;
}

void oppo_wpc_set_booster_en_val(int value)
{    
    struct oppo_wpc_chip *chip = g_wpc_chip;

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return;
	}

	if (chip->wpc_gpios.booster_en_gpio <= 0) {
		chg_err("booster_en_gpio not exist, return\n");
		return;
	}

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.booster_en_active)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.booster_en_sleep)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.booster_en_default)) {
		chg_err("pinctrl null, return\n");
		return;
	}

	if (value) {
		gpio_direction_output(chip->wpc_gpios.booster_en_gpio, 1);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.booster_en_active);
	} else {
		gpio_direction_output(chip->wpc_gpios.booster_en_gpio, 0);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.booster_en_sleep);
	}

	chg_err("set value:%d, gpio_val:%d\n", 
		value, gpio_get_value(chip->wpc_gpios.booster_en_gpio));
}

int oppo_wpc_get_booster_en_val(void)
{
    struct oppo_wpc_chip *chip = g_wpc_chip;

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return;
	}
	if (chip->wpc_gpios.booster_en_gpio <= 0) {
		chg_err("booster_en_gpio not exist, return\n");
		return -1;
	}

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.booster_en_active)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.booster_en_sleep)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.booster_en_default)) {
		chg_err("pinctrl null, return\n");
		return -1;
	}

	return gpio_get_value(chip->wpc_gpios.booster_en_gpio);
}

static int oppo_wpc_otg_en_gpio_init(struct oppo_wpc_chip *chip)
{
    printk(KERN_ERR "[OPPO_CHG][%s]: tongfeng test start!\n", __func__);

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return -EINVAL;
	}

	chip->wpc_gpios.pinctrl = devm_pinctrl_get(chip->dev);

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)) {
		chg_err("get otg_en pinctrl fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.otg_en_active = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "otg_en_active");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.otg_en_active)) {
		chg_err("get otg_en_active fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.otg_en_sleep = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "otg_en_sleep");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.otg_en_sleep)) {
		chg_err("get otg_en fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.otg_en_default = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "otg_en_default");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.otg_en_default)) {
		chg_err("get otg_en_default fail\n");
		return -EINVAL;
	}

	pinctrl_select_state(chip->wpc_gpios.pinctrl, chip->wpc_gpios.otg_en_sleep);
	chg_err("gpio_val:%d\n", gpio_get_value(chip->wpc_gpios.otg_en_gpio));

	return 0;
}

void oppo_wpc_set_otg_en_val(int value)
{   
    struct oppo_wpc_chip *chip = g_wpc_chip;

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return;
	}

	if (chip->wpc_gpios.otg_en_gpio <= 0) {
		chg_err("otg_en_gpio not exist, return\n");
		return;
	}

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.otg_en_active)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.otg_en_sleep)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.otg_en_default)) {
		chg_err("pinctrl null, return\n");
		return;
	}

	if (value) {
		gpio_direction_output(chip->wpc_gpios.otg_en_gpio, 1);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.otg_en_active);
	} else {
		gpio_direction_output(chip->wpc_gpios.otg_en_gpio, 0);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.otg_en_default);
	}

	chg_err("set value:%d, gpio_val:%d\n", 
		value, gpio_get_value(chip->wpc_gpios.otg_en_gpio));
}

int oppo_wpc_get_otg_en_val(struct oppo_wpc_chip *chip)
{
	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return -1;
	}

	if (chip->wpc_gpios.otg_en_gpio <= 0) {
		chg_err("otg_en_gpio not exist, return\n");
		return -1;
	}

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.otg_en_active)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.otg_en_sleep)) {
		chg_err("pinctrl null, return\n");
		return -1;
	}

	return gpio_get_value(chip->wpc_gpios.otg_en_gpio);
}

static int oppo_wpc_wrx_en_gpio_init(struct oppo_wpc_chip *chip)
{
    printk(KERN_ERR "[OPPO_CHG][%s]: tongfeng test start!\n", __func__);

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return -EINVAL;
	}

	chip->wpc_gpios.pinctrl = devm_pinctrl_get(chip->dev);

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)) {
		chg_err("get  wrx_en pinctrl fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wrx_en_active = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wrx_en_active");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wrx_en_active)) {
		chg_err("get wrx_en_active fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wrx_en_sleep = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wrx_en_sleep");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wrx_en_sleep)) {
		chg_err("get wrx_en_sleep fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wrx_en_default = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wrx_en_default");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wrx_en_default)) {
		chg_err("get wrx_en_default fail\n");
		return -EINVAL;
	}

	if (chip->wpc_gpios.wrx_en_gpio > 0) {
		gpio_direction_output(chip->wpc_gpios.wrx_en_gpio, 0);
	}
	pinctrl_select_state(chip->wpc_gpios.pinctrl, chip->wpc_gpios.wrx_en_default);
    printk(KERN_ERR "[OPPO_CHG][%s]: gpio value[%d]!\n", __func__,
		gpio_get_value(chip->wpc_gpios.wrx_en_gpio));

	return 0;
}

void oppo_wpc_set_wrx_en_value(int value)
{
    struct oppo_wpc_chip *chip = g_wpc_chip;

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return;
	}

	if (chip->wpc_gpios.wrx_en_gpio <= 0) {
		chg_err("wrx_en_gpio not exist, return\n");
		return;
	}

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wrx_en_active)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wrx_en_sleep)) {
		chg_err("pinctrl null, return\n");
		return;
	}

	if (value == 1) {
		gpio_direction_output(chip->wpc_gpios.wrx_en_gpio, 1);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.wrx_en_active);
	} else {
		///gpio_direction_output(chg->wrx_en_gpio, 0);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.wrx_en_sleep);
	}
	chg_err("set value:%d, gpio_val:%d\n", 
		value, gpio_get_value(chip->wpc_gpios.wrx_en_gpio));
}

static int oppo_wpc_wrx_otg_gpio_init(struct oppo_wpc_chip *chip)
{
    printk(KERN_ERR "[OPPO_CHG][%s]: tongfeng test start!\n", __func__);

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return -EINVAL;
	}

	chip->wpc_gpios.pinctrl = devm_pinctrl_get(chip->dev);

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)) {
		chg_err("get  wrx_otg pinctrl fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wrx_otg_active = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wrx_otg_active");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wrx_otg_active)) {
		chg_err("get wrx_otg_active fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wrx_otg_sleep = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wrx_otg_sleep");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wrx_otg_sleep)) {
		chg_err("get wrx_otg_sleep fail\n");
		return -EINVAL;
	}

	if (chip->wpc_gpios.wrx_otg_gpio > 0) {
		gpio_direction_output(chip->wpc_gpios.wrx_otg_gpio, 0);
	}
	pinctrl_select_state(chip->wpc_gpios.pinctrl, chip->wpc_gpios.wrx_otg_sleep);

	chg_err("init end, gpio_val:%d\n", 
		gpio_get_value(chip->wpc_gpios.wrx_otg_gpio));
	return 0;
}
void oppo_wpc_set_wrx_otg_value(int value)
{
    struct oppo_wpc_chip *chip = g_wpc_chip;

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: oppo_wpc_chip not ready!\n", __func__);
		return;
	}

	if (chip->wpc_gpios.wrx_otg_gpio <= 0) {
		chg_err("wrx_otg_gpio not exist, return\n");
		return;
	}

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wrx_otg_active)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wrx_otg_sleep)) {
		chg_err("pinctrl null, return\n");
		return;
	}

	if (value == 1) {
		gpio_direction_output(chip->wpc_gpios.wrx_otg_gpio, 1);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.wrx_otg_active);
	} else {
		gpio_direction_output(chip->wpc_gpios.wrx_otg_gpio, 0);
		pinctrl_select_state(chip->wpc_gpios.pinctrl,
				chip->wpc_gpios.wrx_otg_sleep);
	}
	chg_err("set value:%d, gpio_val:%d\n", 
		value, gpio_get_value(chip->wpc_gpios.wrx_otg_gpio));
}

static int oppo_wpc_wired_conn_gpio_init(struct oppo_wpc_chip *chip)
{
    printk(KERN_ERR "[OPPO_CHG][%s]: tongfeng test start!\n", __func__);

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: wpc not ready!\n", __func__);
		return -EINVAL;
	}

	chip->wpc_gpios.pinctrl = devm_pinctrl_get(chip->dev);

	if (IS_ERR_OR_NULL(chip->wpc_gpios.pinctrl)) {
		chg_err("get wired_conn pinctrl fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wired_conn_active = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wired_con_int_active");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wired_conn_active)) {
		chg_err("get wired_conn_active fail\n");
		return -EINVAL;
	}

	chip->wpc_gpios.wired_conn_sleep = pinctrl_lookup_state(chip->wpc_gpios.pinctrl, "wired_con_int_sleep");
	if (IS_ERR_OR_NULL(chip->wpc_gpios.wired_conn_sleep)) {
		chg_err("get wired_conn_sleep fail\n");
		return -EINVAL;
	}

	if (chip->wpc_gpios.wired_conn_gpio > 0) {
		gpio_direction_input(chip->wpc_gpios.wired_conn_gpio);
	}

	pinctrl_select_state(chip->wpc_gpios.pinctrl, chip->wpc_gpios.wired_conn_active);
	chg_err("end, gpio_val:%d\n", 
		 gpio_get_value(chip->wpc_gpios.wrx_otg_gpio));

	return 0;
}

static void oppo_wpc_wired_conn_irq_init(struct oppo_wpc_chip *chip)
{
	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: wpc not ready!\n", __func__);
		return;
	}

	chip->wpc_gpios.wired_conn_irq = gpio_to_irq(chip->wpc_gpios.wired_conn_gpio);
    printk(KERN_ERR "[OPPO_CHG][%s]: chip->wpc_gpios.wired_conn_irq[%d]!\n", __func__, chip->wpc_gpios.wired_conn_irq);
}

static void oppo_wpc_wired_conn_irq_register(struct oppo_wpc_chip *chip)
{
	int ret = 0;

	if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: wpc not ready!\n", __func__);
		return;
	}

	ret = devm_request_threaded_irq(chip->dev, chip->wpc_gpios.wired_conn_irq,
			NULL, oppo_wpc_wired_conn_change_handler, IRQF_TRIGGER_FALLING
			| IRQF_TRIGGER_RISING | IRQF_ONESHOT, "wired_conn-change", chip);
	if (ret < 0) {
		chg_err("Unable to request wired_conn-change irq: %d\n", ret);
	}
    printk(KERN_ERR "%s: !!!!! irq register\n", __FUNCTION__);

	ret = enable_irq_wake(chip->wpc_gpios.wired_conn_irq);
	if (ret != 0) {
		chg_err("enable_irq_wake: wired_conn_irq failed %d\n", ret);
	}
}

static bool oppo_wpc_wired_conn_check_is_gpio(struct oppo_wpc_chip *chip)
{
    if (!chip) {
		printk(KERN_ERR "[OPPO_CHG][%s]: wpc not ready!\n", __func__);
		return false;
	}

	if (gpio_is_valid(chip->wpc_gpios.wired_conn_gpio))
		return true;

	return false;
}

static int oppo_wpc_parse_param_dt(struct oppo_wpc_chip *chip)
{
    int rc = 0;
    struct device_node *node = chip->dev->of_node;

    chip->cp_support = of_property_read_bool(node, "qcom,cp_support");

    return rc;
}

int oppo_wpc_gpio_dt_init(struct oppo_wpc_chip *chip)
{
    int rc = 0;
    struct device_node *node = chip->dev->of_node;

    chip->wpc_gpios.wpc_en_gpio = of_get_named_gpio(node, "qcom,wpc_en-gpio", 0);
    if (chip->wpc_gpios.wpc_en_gpio <= 0) {
        chg_err("Couldn't read qcom,wpc_en-gpio rc=%d, qcom,wpc_en-gpio:%d\n",
            rc, chip->wpc_gpios.wpc_en_gpio);
    } else {
        if (gpio_is_valid(chip->wpc_gpios.wpc_en_gpio)) {
            rc = gpio_request(chip->wpc_gpios.wpc_en_gpio, "wpc_en-gpio");
            if (rc) {
                chg_err("unable to request gpio [%d]\n", chip->wpc_gpios.wpc_en_gpio);
            } else {
                rc = oppo_wpc_en_gpio_init(chip);
                if (rc)
                    chg_err("unable to init wpc_en-gpio:%d\n", chip->wpc_gpios.wpc_en_gpio);
            }
        }
    }
    chg_err("WPC wpc_en-gpio\n");

    chip->wpc_gpios.wpc_con_gpio = of_get_named_gpio(node, "qcom,wpc_connect-gpio", 0);
	if (chip->wpc_gpios.wpc_con_gpio < 0) {
		pr_err("chip->wpc_gpios.wpc_con_gpio not specified\n");	
	} else {
		if (gpio_is_valid(chip->wpc_gpios.wpc_con_gpio)) {
			rc = gpio_request(chip->wpc_gpios.wpc_con_gpio, "wpc-connect-gpio");
			if (rc) {
				pr_err("unable to request gpio [%d]\n", chip->wpc_gpios.wpc_con_gpio);
			} else {
				rc = oppo_wpc_con_gpio_init(chip);
				if (rc)
					chg_err("unable to init wpc_con_gpio:%d\n", chip->wpc_gpios.wpc_con_gpio);
				else {
					oppo_wpc_con_irq_init(chip);
					oppo_wpc_con_eint_register(chip);
				}
			}
		}
		chg_err("chip->wpc_gpios.wpc_con_gpio =%d\n",chip->wpc_gpios.wpc_con_gpio);
	}

    chip->wpc_gpios.wired_conn_gpio = of_get_named_gpio(node, "qcom,wired_conn-gpio", 0);
	if (chip->wpc_gpios.wired_conn_gpio < 0) {
		pr_err("chip->wpc_gpios.wired_conn_gpio not specified\n");	
	} else {
		if (gpio_is_valid(chip->wpc_gpios.wired_conn_gpio)) {
			rc = gpio_request(chip->wpc_gpios.wired_conn_gpio, "wired_conn-gpio");
			if (rc) {
				pr_err("unable to request gpio [%d]\n", chip->wpc_gpios.wired_conn_gpio);
			} else {
				rc = oppo_wpc_wired_conn_gpio_init(chip);
				if (rc)
					chg_err("unable to init wired_conn_gpio:%d\n", chip->wpc_gpios.wired_conn_gpio);
				else {
					oppo_wpc_wired_conn_irq_init(chip);
					oppo_wpc_wired_conn_irq_register(chip);
				}
			}
		}
		chg_err("chip->wpc_gpios.wired_conn_gpio =%d\n",chip->wpc_gpios.wired_conn_gpio);
	}

 	chip->wpc_gpios.vbat_en_gpio = of_get_named_gpio(node, "qcom,vbat_en-gpio", 0);
	if (chip->wpc_gpios.vbat_en_gpio < 0) {
		pr_err("chip->vbat_en_gpio not specified\n");	
	} else {
		if (gpio_is_valid(chip->wpc_gpios.vbat_en_gpio)) {
			rc = gpio_request(chip->wpc_gpios.vbat_en_gpio, "vbat-en-gpio");
			if (rc) {
				pr_err("unable to request gpio vbat_en_gpio[%d]\n", chip->wpc_gpios.vbat_en_gpio);
			} else {
				rc = oppo_wpc_vbat_en_gpio_init(chip);
				if (rc) {
					chg_err("unable to init vbat_en_gpio:%d\n", chip->wpc_gpios.vbat_en_gpio);
				}
			}
		}
		pr_err("chip->wpc_gpios.vbat_en_gpio =%d\n",chip->wpc_gpios.vbat_en_gpio);
	}

 	chip->wpc_gpios.booster_en_gpio = of_get_named_gpio(node, "qcom,booster_en-gpio", 0);
	if (chip->wpc_gpios.booster_en_gpio < 0) {
		pr_err("chip->booster_en_gpio not specified\n");	
	} else {
		if (gpio_is_valid(chip->wpc_gpios.booster_en_gpio)) {
			rc = gpio_request(chip->wpc_gpios.booster_en_gpio, "booster-en-gpio");
			if (rc) {
				pr_err("unable to request gpio booster_en_gpio[%d]\n", chip->wpc_gpios.booster_en_gpio);
			} else {
				rc = oppo_wpc_booster_en_gpio_init(chip);
				if (rc) {
					chg_err("unable to init booster_en_gpio:%d\n", chip->wpc_gpios.booster_en_gpio);
				}
			}
		}
		pr_err("chip->wpc_gpios.booster_en_gpio =%d\n",chip->wpc_gpios.booster_en_gpio);
	}

 	chip->wpc_gpios.otg_en_gpio = of_get_named_gpio(node, "qcom,otg_en-gpio", 0);
	if (chip->wpc_gpios.otg_en_gpio < 0) {
		pr_err("chip->otg_en_gpio not specified\n");	
	} else {
		if (gpio_is_valid(chip->wpc_gpios.otg_en_gpio)) {
			rc = gpio_request(chip->wpc_gpios.otg_en_gpio, "otg-en-gpio");
			if (rc) {
				pr_err("unable to request gpio otg_en_gpio[%d]\n", chip->wpc_gpios.otg_en_gpio);
			} else {
				rc = oppo_wpc_otg_en_gpio_init(chip);
				if (rc) {
					chg_err("unable to init otg_en_gpio:%d\n", chip->wpc_gpios.otg_en_gpio);
				}
			}
		}
		pr_err("chip->wpc_gpios.otg_en_gpio =%d\n",chip->wpc_gpios.otg_en_gpio);
	}

 	chip->wpc_gpios.wrx_en_gpio = of_get_named_gpio(node, "qcom,wrx_en-gpio", 0);
	if (chip->wpc_gpios.wrx_en_gpio < 0) {
		pr_err("chip->wrx_en_gpio not specified\n");	
	} else {
		if (gpio_is_valid(chip->wpc_gpios.wrx_en_gpio)) {
			rc = gpio_request(chip->wpc_gpios.wrx_en_gpio, "wrx-en-gpio");
			if (rc) {
				pr_err("unable to request gpio wrx_en_gpio[%d]\n", chip->wpc_gpios.wrx_en_gpio);
			} else {
				rc = oppo_wpc_wrx_en_gpio_init(chip);
				if (rc) {
					chg_err("unable to init wrx_en_gpio:%d\n", chip->wpc_gpios.wrx_en_gpio);
				}
			}
		}
		pr_err("chip->wpc_gpios.wrx_en_gpio =%d\n",chip->wpc_gpios.wrx_en_gpio);
	}

 	chip->wpc_gpios.wrx_otg_gpio = of_get_named_gpio(node, "qcom,wrx_otg-gpio", 0);
	if (chip->wpc_gpios.wrx_otg_gpio < 0) {
		pr_err("chip->wrx_otg_gpio not specified\n");	
	} else {
		if (gpio_is_valid(chip->wpc_gpios.wrx_otg_gpio)) {
			rc = gpio_request(chip->wpc_gpios.wrx_otg_gpio, "wrx-otg-gpio");
			if (rc) {
				pr_err("unable to request gpio wrx_otg_gpio[%d]\n", chip->wpc_gpios.wrx_otg_gpio);
			} else {
				rc = oppo_wpc_wrx_otg_gpio_init(chip);
				if (rc) {
					chg_err("unable to init wrx_otg_gpio:%d\n", chip->wpc_gpios.wrx_otg_gpio);
				}
			}
		}
		pr_err("chip->wpc_gpios.wrx_otg_gpio =%d\n",chip->wpc_gpios.wrx_otg_gpio);
	}

	return 0;
}

static int oppo_wpc_get_con_gpio_val(struct oppo_wpc_chip *chip)
{
	if (chip->wpc_gpios.wpc_con_gpio <= 0) {
		chg_err("wpc_con_gpio not exist, return\n");
		return -1;
	}

	if (IS_ERR_OR_NULL(chip->pinctrl)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wpc_con_active)
		|| IS_ERR_OR_NULL(chip->wpc_gpios.wpc_con_sleep)) {
		chg_err("pinctrl null, return\n");
		return -1;
	}

	return gpio_get_value(chip->wpc_gpios.wpc_con_gpio);
}

static int oppo_wpc_init_connected_task(struct oppo_wpc_chip *chip)
{
	if (!chip->charge_online) {
		chip->charge_online = true;
		chg_err("tongfeng test  111\n");

        if (chip->wpc_ops->init_irq_registers)
            chip->wpc_ops->init_irq_registers(chip);

		if (chip) {
//			mp2650_hardware_init_for_wireless_charge();
            oppo_chg_turn_on_charging();
		}
		send_msg_timer = 2;  //tongfeng question//

        if (chip) {
            chg_err("<~WPC~> set terminate voltage: %d\n", WPC_TERMINATION_VOLTAGE_DEFAULT);
            chip->terminate_voltage = WPC_TERMINATION_VOLTAGE_DEFAULT;
            //mp2650_float_voltage_write(WPC_TERMINATION_VOLTAGE_DEFAULT + WPC_TERMINATION_VOLTAGE_OFFSET);  //tongfeng question// 这里为什么要叠加

            chg_err("<~WPC~> set terminate current: %d\n", WPC_TERMINATION_CURRENT);
            chip->terminate_current = WPC_TERMINATION_CURRENT;
            //mp2650_set_termchg_current(WPC_TERMINATION_CURRENT);
        }
	}

	return 0;
}

static int oppo_wpc_deinit_after_disconnected(struct oppo_wpc_chip *chip)
{
	if (chip->charge_online) {
		chip->charge_online = false;
		oppo_wpc_reset_variables(chip);
		chargepump_disable();
		oppo_wpc_set_vbat_en_val(chip, 0);
	}

	return 0;
}

static bool oppo_wpc_is_firmware_updating(void)
{
	if (!g_wpc_chip) {
		return 0;
	}
	
	return g_wpc_chip->wpc_fw_updating;
}

static void oppo_wpc_connect_int_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oppo_wpc_chip *chip = container_of(dwork, struct oppo_wpc_chip, wpc_connect_int_work);
    
    if (oppo_wpc_is_firmware_updating() == true) {
        return;
    }

	if (oppo_wpc_get_con_gpio_val(chip) == 1) {
		chg_err("<~WPC~> wpc dock has connected!>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		oppo_wpc_init_connected_task(chip);		
		schedule_delayed_work(&chip->wpc_task_work, WPC_TASK_INTERVAL);
	} else {
		chg_err("<~WPC~> wpc dock has disconnected!< < < < < < < < < < < < <\n");
		oppo_wpc_deinit_after_disconnected(chip);
	}
}

void oppo_wpc_init(struct oppo_wpc_chip *chip)
{
	INIT_DELAYED_WORK(&chip->wpc_connect_int_work, oppo_wpc_connect_int_func);
	INIT_DELAYED_WORK(&chip->wpc_task_work, oppo_wpc_task_work_process);
}


static void oppo_wpc_reset_variables(struct oppo_wpc_chip *chip)
{
	int i;
	
	chip->charge_status = WPC_CHG_STATUS_DEFAULT;
	chip->charge_online = false;
	chip->send_message = P9221_CMD_NULL;
	chip->adapter_type = ADAPTER_TYPE_UNKNOW;
	chip->charge_type = WPC_CHARGE_TYPE_DEFAULT;
	chip->charge_voltage = 0;
	chip->cc_value_limit = WPC_CHARGE_CURRENT_FASTCHG;
	chip->charge_current = 0;
	chip->CEP_is_OK = true;
	chip->temperature_status = TEMPERATURE_STATUS_NORMAL;
	chip->termination_status = WPC_TERMINATION_STATUS_UNDEFINE;
	chip->vrect = 0;
	chip->vout = 0;
	chip->iout = 0;
	chip->fastchg_ing = false;
	chip->epp_working = false;
	chip->idt_fw_updating = false;

	for (i = 0; i < 10; i++) {
		chip->iout_array[i] = 0;
	}
}

static int oppo_wpc_RXTX_message_process(struct oppo_wpc_chip *chip)
{
	send_msg_timer++;
	if (send_msg_timer > 2) {
		send_msg_timer = 0;

		if (chip->adapter_type == ADAPTER_TYPE_UNKNOW) {
            chip->wpc_ops->set_charger_type_dect(chip);
		} else if (chip->send_message == P9221_CMD_INTO_FASTCHAGE) {
            chip->wpc_ops->set_tx_charger_mode(chip);
		} else if (chip->send_message == P9221_CMD_INTO_USB_CHARGE) {
            chip->wpc_ops->set_tx_charger_mode(chip);
		} else if (chip->send_message == P9221_CMD_INTO_NORMAL_CHARGE) {
            chip->wpc_ops->set_tx_charger_mode(chip);
		}
	}

	switch (chip->tx_command) { 
	case P9237_RESPONE_ADAPTER_TYPE:
		chip->adapter_type = chip->tx_data;
		chg_err("<~WPC~> get adapter type = 0x%02X\n", chip->adapter_type);
		if ((chip->adapter_type == ADAPTER_TYPE_FASTCHAGE_VOOC)
			|| (chip->adapter_type == ADAPTER_TYPE_FASTCHAGE_SVOOC)){
			chip->fastchg_ing = true;
		}
		break;
		
	case P9237_RESPONE_INTO_FASTCHAGE:
		chip->charge_type = WPC_CHARGE_TYPE_FAST;
		chg_err("<~WPC~> enter charge type = WPC_CHARGE_TYPE_FAST\n");
		if (chip->send_message == P9221_CMD_INTO_FASTCHAGE) {
			chip->send_message = P9221_CMD_NULL;
		}
		break;
		
	case P9237_RESPONE_INTO_USB_CHARGE:
		chip->charge_type = WPC_CHARGE_TYPE_USB;
		chg_err("<~WPC~> enter charge type = WPC_CHARGE_TYPE_USB\n");
		if (chip->send_message == P9221_CMD_INTO_USB_CHARGE) {
			chip->send_message = P9221_CMD_NULL;
		}
		break;
		
	case P9237_RESPONE_INTO_NORMAL_CHARGER:
		chip->charge_type = WPC_CHARGE_TYPE_NORMAL;
		chg_err("<~WPC~> enter charge type = WPC_CHARGE_TYPE_NORMAL\n");
		if (chip->send_message == P9221_CMD_INTO_NORMAL_CHARGE) {
			chip->send_message = P9221_CMD_NULL;
		}
		break;

	case P9237_COMMAND_READY_FOR_EPP:
		chip->adapter_type == ADAPTER_TYPE_EPP;
		chg_err("<~WPC~> adapter_type = ADAPTER_TYPE_EPP\n");
		break;

	case P9237_COMMAND_WORKING_IN_EPP:
		chip->epp_working = true;		
		chg_err("<~WPC~> chip->epp_working = true\n");
		break;
		
	default:
		break;
	}

	chip->tx_command = P9237_RESPONE_NULL;
	return 0;
}


static void oppo_wpc_get_charging_info(struct oppo_wpc_chip *chip)
{
    chip->wpc_ops->get_vrect_iout(chip);
    return;
}

static int oppo_wpc_temperature_status_process(struct oppo_wpc_chip *chip)
{
    int tbat_temp = oppo_chg_get_chg_temperature();
	if ((tbat_temp >= TEMPERATURE_COLD_LOW_LEVEL) 
		&& (tbat_temp < TEMPERATURE_COLD_HIGH_LEVEL)) {
		if (chip->temperature_status != TEMPERATURE_STATUS_COLD) {
			chg_err("<~WPC~> temperature_status change to TEMPERATURE_STATUS_COLD\n");
			chip->temperature_status = TEMPERATURE_STATUS_COLD;
		} 
	} else if ((tbat_temp >= TEMPERATURE_COOL_LOW_LEVEL) 
				&& (tbat_temp < TEMPERATURE_COOL_HIGH_LEVEL)) {
		if (chip->temperature_status != TEMPERATURE_STATUS_COOL) {
			chg_err("<~WPC~> temperature_status change to TEMPERATURE_STATUS_COOL\n");
			chip->temperature_status = TEMPERATURE_STATUS_COOL;
		}
	} else if ((tbat_temp >= TEMPERATURE_NORMAL_LOW_LEVEL) 
				&& (tbat_temp < TEMPERATURE_NORMAL_HIGH_LEVEL)) {
		if (chip->temperature_status != TEMPERATURE_STATUS_NORMAL) {
			chg_err("<~WPC~> temperature_status change to TEMPERATURE_STATUS_NORMAL\n");
			chip->temperature_status = TEMPERATURE_STATUS_NORMAL;
		} 
	} else if ((tbat_temp >= TEMPERATURE_WARM_LOW_LEVEL) 
				&& (tbat_temp < TEMPERATURE_WARM_HIGH_LEVEL)) {
		if (chip->temperature_status != TEMPERATURE_STATUS_WARM) {
			chg_err("<~WPC~> temperature_status change to TEMPERATURE_STATUS_WARM\n");
			chip->temperature_status = TEMPERATURE_STATUS_WARM;
		} 
	} else if ((tbat_temp >= TEMPERATURE_HOT_LOW_LEVEL) 
				&& (tbat_temp < TEMPERATURE_HOT_HIGH_LEVEL)) {
		if (chip->temperature_status != TEMPERATURE_STATUS_HOT) {
			chg_err("<~WPC~> temperature_status change to TEMPERATURE_STATUS_HOT\n");
			chip->temperature_status = TEMPERATURE_STATUS_HOT;
		} 
	} else {
		if (chip->temperature_status != TEMPERATURE_STATUS_ABNORMAL) {
			chg_err("<~WPC~> temperature_status change to TEMPERATURE_STATUS_ABNORMAL\n");
			chip->temperature_status = TEMPERATURE_STATUS_ABNORMAL;
			chip->fastchg_ing = false;
		} 
	}

	return 0;
}

static bool is_allow_wpc_fast_chg(struct oppo_wpc_chip *chip)
{
    int batt_temp = 0;
    int batt_volt = 0;
    int batt_soc = 0;

    batt_temp = oppo_chg_get_chg_temperature();
    batt_volt = oppo_chg_get_batt_volt();
    batt_soc = oppo_chg_get_soc();

    if (batt_temp < TEMPERATURE_NORMAL_LOW_LEVEL)
        return false;
    else if(batt_temp > TEMPERATURE_NORMAL_HIGH_LEVEL)
        return false;

    if (batt_volt > P922X_CC2CV_CHG_THD_HI)
        return false;

    if (batt_soc < 1)
        return false;

    return true;
}

static int oppo_wpc_charge_status_process(struct oppo_wpc_chip *chip)
{
	static int work_delay = 0;
	static int batt_vol_4450_cnt = 0;
	static int batt_vol_4350_cnt = 0;
	static int cep_nonzero_cnt = 0;
	static int adjust_current_delay = 0;
	static bool flag_chgpump_reach_41degree = false;
	int temp_value = 0;
	static bool flag_temperature_abnormal = false;
	static int fastchg_current_max;
	static int fastchg_current_min;
    int batt_ichging = 0;
    int batt_temp = 0;
    int batt_volt = 0;
    int batt_soc = 0;

	if (chip->temperature_status == TEMPERATURE_STATUS_ABNORMAL) {
		chg_err("<~WPC~> The temperature is abnormal, stop charge!\n");
		if (chip->charge_current != WPC_CHARGE_CURRENT_ZERO) {
			//p922x_set_rx_charge_current(chip, WPC_CHARGE_CURRENT_ZERO);
            oppo_chg_disable_charge();
		}

		if (chip->charge_voltage != WPC_CHARGE_VOLTAGE_DEFAULT) {
			//p922x_set_rx_charge_voltage(chip, WPC_CHARGE_VOLTAGE_DEFAULT);
            chip->wpc_ops->set_rx_charge_voltage(chip, WPC_CHARGE_VOLTAGE_DEFAULT);
		}

		return 0;
	}

	if ((chip->temperature_status == TEMPERATURE_STATUS_COLD)
		|| (chip->temperature_status == TEMPERATURE_STATUS_COOL)
		|| (chip->temperature_status == TEMPERATURE_STATUS_HOT)) {
		if (chip->charge_status != WPC_CHG_STATUS_DEFAULT) {
			chg_err("<~WPC~> The temperature is cold, cool, or hot, go back default charge!\n");
			//when the temperature is warm, if WPC is in fastchg, don't go back default directly.
			//The WPC fastchg will process itself.
			chip->charge_status = WPC_CHG_STATUS_DEFAULT;
			chip->send_message = P9221_CMD_INTO_NORMAL_CHARGE;
			work_delay = 0;
		}
	}

	if (work_delay > 0) {
		//chg_err("<~WPC~> work delay %d\n", work_delay);
		work_delay--;
		return 0;
	}

	switch (chip->charge_status) {
	case WPC_CHG_STATUS_DEFAULT:
		if (chip->charge_current != WPC_CHARGE_CURRENT_DEFAULT) {
			p922x_set_rx_charge_current(chip, WPC_CHARGE_CURRENT_DEFAULT);
		}

		if (chip->charge_voltage != WPC_CHARGE_VOLTAGE_DEFAULT) {
			p922x_set_rx_charge_voltage(chip, WPC_CHARGE_VOLTAGE_DEFAULT);
		}

		if (chip->temperature_status == TEMPERATURE_STATUS_NORMAL) {
			if ((g_oppo_chip->batt_volt_min > P922X_PRE2CC_CHG_THD_LO)
				&& (g_oppo_chip->batt_volt_max < P922X_CC2CV_CHG_THD_HI)
				&& (g_oppo_chip->soc <= 90)) {
				if (chip->adapter_type == ADAPTER_TYPE_FASTCHAGE_VOOC) {                    
#ifdef WPC_USE_SP6001
					oppo_wpc_set_vbat_en_val(1);
#endif
					chargepump_set_for_EPP();    //enable chargepump
					chargepump_enable();
				
					chip->charge_status = WPC_CHG_STATUS_READY_FOR_VOOC;
					break;
				} else if (chip->adapter_type == ADAPTER_TYPE_FASTCHAGE_SVOOC) {
#ifdef WPC_USE_SP6001
					oppo_wpc_set_vbat_en_val(1);
#endif
					chargepump_set_for_EPP();	 //enable chargepump
					chargepump_enable();

					chip->charge_status = WPC_CHG_STATUS_READY_FOR_SVOOC;
					break;
				} else if (chip->adapter_type == ADAPTER_TYPE_EPP) {
					chip->charge_status = WPC_CHG_STATUS_READY_FOR_EPP;
					break;
				}
			}
		}
		break;

	case WPC_CHG_STATUS_READY_FOR_EPP:
		chg_err("<~WPC~> ..........WPC_CHG_STATUS_READY_FOR_EPP..........\n");		
		chargepump_disable();
		mp2650_enable_charging();
		p922x_set_rx_charge_current(chip, 1000);
		mp2650_input_current_limit_write(1000);
		chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_EPP_1A;
		break;

	case WPC_CHG_STATUS_EPP_1A:
		chg_err("<~WPC~> ..........WPC_CHG_STATUS_EPP_1A..........\n");
		if (chip->p922x_chg_status.epp_working) {
			p922x_set_rx_charge_current(chip, 1200);
			mp2650_input_current_limit_write(1200);
			chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_EPP_WORKING;
		}
		break;
		
	case WPC_CHG_STATUS_EPP_WORKING:
		break;

	case WPC_CHG_STATUS_READY_FOR_VOOC:
		chg_err("<~WPC~> ..........WPC_CHG_STATUS_READY_FOR_VOOC..........\n");
		mp2650_enable_charging();
		p922x_set_rx_charge_current(chip, WPC_CHARGE_CURRENT_FASTCHG_INT);
		p922x_set_rx_charge_voltage(chip, WPC_CHARGE_VOLTAGE_VOOC_INIT);
		mp2650_input_current_limit_write(WPC_CHARGE_CURRENT_LIMIT_200MA);
		chip->p922x_chg_status.send_message = P9221_CMD_INTO_FASTCHAGE;
#ifndef WPC_USE_SP6001
		p922x_set_tx_charger_fastcharge(chip);
#endif
		send_msg_timer = 0;
		p922x_begin_CEP_detect(chip);
		chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_WAITING_FOR_TX_INTO_FASTCHG;
		break;

	case WPC_CHG_STATUS_READY_FOR_SVOOC:
		chg_err("<~WPC~> ..........WPC_CHG_STATUS_READY_FOR_SVOOC..........\n");
		mp2650_enable_charging();
		p922x_set_rx_charge_current(chip, WPC_CHARGE_CURRENT_FASTCHG_INT);
		p922x_set_rx_charge_voltage(chip, WPC_CHARGE_VOLTAGE_SVOOC_INIT);
		mp2650_input_current_limit_write(WPC_CHARGE_CURRENT_LIMIT_200MA);
		chip->p922x_chg_status.send_message = P9221_CMD_INTO_FASTCHAGE;
#ifndef WPC_USE_SP6001
		p922x_set_tx_charger_fastcharge(chip);
#endif
		send_msg_timer = 0;
		p922x_begin_CEP_detect(chip);
		chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_WAITING_FOR_TX_INTO_FASTCHG;
		break;

	case WPC_CHG_STATUS_WAITING_FOR_TX_INTO_FASTCHG:
		chg_err("<~WPC~> ..........WPC_CHG_STATUS_WAITING_FOR_TX_INTO_FASTCHG..........\n");
		if ((p922x_get_CEP_flag(chip) == 0) && (chip->p922x_chg_status.charge_type == WPC_CHARGE_TYPE_FAST)) {
			if (chip->p922x_chg_status.ftm_mode) {
				chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_READY_FOR_FTM;
			} else {
				chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_INCREASE_VOLTAGE;
			}
		} 
		break;

	case WPC_CHG_STATUS_INCREASE_VOLTAGE:
		chg_err("<~WPC~> ..........WPC_CHG_STATUS_INCREASE_VOLTAGE..........\n");
		if (p922x_get_CEP_flag(chip) == 0) {
			chg_err("<~WPC~> .g_oppo_chip->batt_volt * 4 + 500: %d\n", g_oppo_chip->batt_volt * 4 + 500);
			temp_value = p922x_increase_vc_by_step(chip, 
													chip->p922x_chg_status.charge_voltage, 
													g_oppo_chip->batt_volt * 4 + 500, 
													WPC_CHARGE_VOLTAGE_CHANGE_STEP_1V);
			if (temp_value != 0) {
				p922x_set_rx_charge_voltage(chip, temp_value);
				msleep(100);
				p922x_begin_CEP_detect(chip);
			} else {
				chargepump_set_for_LDO();
				
				flag_temperature_abnormal = false;
				adjust_current_delay = 0;
				batt_vol_4350_cnt = 0;
				batt_vol_4450_cnt = 0;
				flag_chgpump_reach_41degree = false;
				chip->p922x_chg_status.charge_level = CHARGE_LEVEL_UNKNOW;
				chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_FAST_CHARGING_FROM_CHGPUMP;
			}
		}
		break;

	case WPC_CHG_STATUS_FAST_CHARGING_FROM_CHGPUMP:
		if((chip->p922x_chg_status.iout > 500)) {
			if(chip->p922x_chg_status.charge_current > WPC_CHARGE_CURRENT_ZERO) {
			    chg_err("<~WPC~> Iout > 500mA & ChargeCurrent > 200mA. Disable 2650\n");
			    p922x_set_rx_charge_current(chip, WPC_CHARGE_CURRENT_ZERO);
				mp2650_disable_charging();
                chip->p922x_chg_status.wpc_4370_chg = false;
			}
		}

		if (g_oppo_chip->batt_volt >= 4500 
            || ((-1 * g_oppo_chip->icharging) < 1000 && g_oppo_chip->batt_volt >= 4370 && chip->p922x_chg_status.wpc_4370_chg == false)) {
			batt_vol_4450_cnt++;
			if (batt_vol_4450_cnt < 5) {
				;//break;
			}

			chg_err("<~WPC~> The max cell vol >= 4500mV.g_oppo_chip->icharging[%d]\n", g_oppo_chip->icharging);

			batt_vol_4450_cnt = 0;

			chg_err("<~WPC~> Turn to MP2650 control\n");
            chip->p922x_chg_status.wpc_4370_chg = false;
			p922x_ready_for_mp2650_charge(chip);
			chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_FAST_CHARGING;
			break;
		} else if ((batt_vol_4350_cnt >= 5) 
		        || (g_oppo_chip->batt_volt >= 4370 || chip->p922x_chg_status.wpc_4370_chg)) {
			batt_vol_4450_cnt = 0;

            chip->p922x_chg_status.wpc_4370_chg = true;
            fastchg_current_max = WPC_CHARGE_600MA_UPPER_LIMIT;
            fastchg_current_min = WPC_CHARGE_500MA_LOWER_LIMIT;
            //break;  // add by huangtongfeng
            #if 0
			if (batt_vol_4350_cnt < 5) {
				batt_vol_4350_cnt++;
				break;
			}

			chg_err("<~WPC~> The vbatt has reached 4350mV.\n");
			
			if (chip->p922x_chg_status.charge_voltage != WPC_CHARGE_VOLTAGE_CHGPUMP_CV) {
				p922x_set_rx_charge_voltage(chip, WPC_CHARGE_VOLTAGE_CHGPUMP_CV);
			} else if ((g_oppo_chip->temperature >= 400) || (chip->p922x_chg_status.iout < 600)){
				chg_err("<~WPC~> Turn to MP2650 control\n");
				p922x_ready_for_mp2650_charge(chip);
				chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_FAST_CHARGING;
			}
			
			break;
            #endif
		} else{
			batt_vol_4450_cnt = 0;
		}
		chg_err("<~WPC~> chip->p922x_chg_status.wpc_4370_chg[%d]\n", chip->p922x_chg_status.wpc_4370_chg);

		if(adjust_current_delay > 0) {
			adjust_current_delay--;
		} else  if (chip->p922x_chg_status.wpc_4370_chg == false){
			if (g_oppo_chip->temperature >= 400) {
				chg_err("<~WPC~> The tempearture is >= 40\n");

				if (!flag_chgpump_reach_41degree) {
					fastchg_current_max = WPC_CHARGE_600MA_UPPER_LIMIT;
					fastchg_current_min = WPC_CHARGE_600MA_LOWER_LIMIT;
					chip->p922x_chg_status.charge_level = CHARGE_LEVEL_ABOVE_41_DEGREE;
					flag_chgpump_reach_41degree = true;
					adjust_current_delay = WPC_ADJUST_CV_DELAY;
				} else {
					chg_err("<~WPC~> Turn to MP2650 control\n");
					p922x_ready_for_mp2650_charge(chip);
					chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_FAST_CHARGING;
					break;
				}
			} else if (g_oppo_chip->temperature >= 370) {
				chg_err("<~WPC~> The tempearture is >= 37 an < 40\n");
				if ((chip->p922x_chg_status.charge_level == CHARGE_LEVEL_BELOW_39_DEGREE)
					|| (chip->p922x_chg_status.charge_level == CHARGE_LEVEL_UNKNOW)){
					chg_err("<~WPC~> Turn to 800mA charge\n");
					fastchg_current_max = WPC_CHARGE_800MA_UPPER_LIMIT;
					fastchg_current_min = WPC_CHARGE_800MA_LOWER_LIMIT;
					chip->p922x_chg_status.charge_level = CHARGE_LEVEL_39_TO_41_DEGREE;
					adjust_current_delay = WPC_ADJUST_CV_DELAY;
				}
			} else {
				chg_err("<~WPC~> The tempearture is < 37\n");
				if ((chip->p922x_chg_status.charge_level == CHARGE_LEVEL_39_TO_41_DEGREE)
					|| (chip->p922x_chg_status.charge_level == CHARGE_LEVEL_ABOVE_41_DEGREE)){
					if(g_oppo_chip->temperature < 360) {
						chg_err("<~WPC~> The tempearture is < 36, change to 1A charge\n");
						fastchg_current_max = WPC_CHARGE_1A_UPPER_LIMIT;
						fastchg_current_min = WPC_CHARGE_1A_LOWER_LIMIT;
						chip->p922x_chg_status.charge_level = CHARGE_LEVEL_BELOW_39_DEGREE;
						adjust_current_delay = WPC_ADJUST_CV_DELAY;
					}
				} else if (chip->p922x_chg_status.charge_level != CHARGE_LEVEL_BELOW_39_DEGREE) {
					chg_err("<~WPC~> Turn to 1A charge\n");
					fastchg_current_max = WPC_CHARGE_1A_UPPER_LIMIT;
					fastchg_current_min = WPC_CHARGE_1A_LOWER_LIMIT;
					chip->p922x_chg_status.charge_level = CHARGE_LEVEL_BELOW_39_DEGREE;
					adjust_current_delay = WPC_ADJUST_CV_DELAY;
				}
			} 
		}
		
		if(p922x_get_CEP_now_flag(chip) == 0) {
			if (chip->p922x_chg_status.iout > fastchg_current_max) {
				chg_err("<~WPC~> The Iout > %d.\n", fastchg_current_max);
	
				if (chip->p922x_chg_status.charge_voltage > WPC_CHARGE_VOLTAGE_CHGPUMP_MIN) {
					temp_value = chip->p922x_chg_status.iout - fastchg_current_max;
					if(temp_value > 200) {
						p922x_set_rx_charge_voltage(chip, chip->p922x_chg_status.charge_voltage - 200);
					} else if (temp_value > 50) {
						p922x_set_rx_charge_voltage(chip, chip->p922x_chg_status.charge_voltage - 100);
					} else {
						p922x_set_rx_charge_voltage(chip, chip->p922x_chg_status.charge_voltage - 20);
					}

					work_delay = WPC_ADJUST_CV_DELAY; 
					break;
				}
			} else if (chip->p922x_chg_status.iout < fastchg_current_min) {
				chg_err("<~WPC~> The Iout < %d.\n", fastchg_current_min);

				if (chip->p922x_chg_status.charge_voltage < WPC_CHARGE_VOLTAGE_CHGPUMP_MAX) {
					temp_value = fastchg_current_min - chip->p922x_chg_status.iout;
					if(temp_value > 200) {
						p922x_set_rx_charge_voltage(chip, chip->p922x_chg_status.charge_voltage + 200);
					} else if (temp_value > 50) {
						p922x_set_rx_charge_voltage(chip, chip->p922x_chg_status.charge_voltage + 100);
					} else {
						p922x_set_rx_charge_voltage(chip, chip->p922x_chg_status.charge_voltage + 20);
					}
					
					work_delay = WPC_ADJUST_CV_DELAY; 
					break;
				}
			} 
		}
		break;
		
	case WPC_CHG_STATUS_INCREASE_CURRENT:
		chg_err("<~WPC~> ..........WPC_CHG_STATUS_INCREASE_CURRENT..........\n");
		temp_value = p922x_increase_vc_by_step(chip, 
												chip->p922x_chg_status.charge_current, 
												chip->p922x_chg_status.cc_value_limit, 
												WPC_CHARGE_CURRENT_CHANGE_STEP_200MA);
		if (temp_value != 0) {
			p922x_set_rx_charge_current(chip, temp_value);
			work_delay = WPC_INCREASE_CURRENT_DELAY; 
			chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_ADJUST_VOL_AFTER_INC_CURRENT;
		} else {
			chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_FAST_CHARGING;
		}
		break;

	case WPC_CHG_STATUS_ADJUST_VOL_AFTER_INC_CURRENT:
		chg_err("<~WPC~> ..........WPC_CHG_STATUS_ADJUST_VOL_AFTER_INC_CURRENT..........\n");
		if (chip->p922x_chg_status.iout > WPC_CHARGE_IOUT_HIGH_LEVEL) {
			chg_err("<~WPC~>  IDT Iout > 1050mA!\n");		
			temp_value = p922x_increase_vc_by_step(chip, 
													chip->p922x_chg_status.charge_voltage, 
													WPC_CHARGE_VOLTAGE_FASTCHG_MAX, 
													WPC_CHARGE_VOLTAGE_CHANGE_STEP_1V);
			if (temp_value != 0) {
				p922x_set_rx_charge_voltage(chip, temp_value);
			} else {					
				chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_FAST_CHARGING;
			}
		} else {
			chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_INCREASE_CURRENT;
		}
		break;

	case WPC_CHG_STATUS_FAST_CHARGING:
		if (g_oppo_chip->temperature >= 400) {
			chg_err("<~WPC~> The tempearture is >= 40\n");
			flag_temperature_abnormal = true;
			
			temp_value = p922x_decrease_vc_by_step(chip, 
													chip->p922x_chg_status.charge_current, 
													WPC_CHARGE_CURRENT_DEFAULT, 
													WPC_CHARGE_CURRENT_CHANGE_STEP_200MA);
			if (temp_value != 0) {
				p922x_set_rx_charge_current(chip, temp_value);

				work_delay = WPC_ADJUST_CV_DELAY;
				break;
			}
//		} else if (g_oppo_chip->temperature >= 390) {	
//			flag_temperature_abnormal = true;
			
//			if (chip->p922x_chg_status.charge_current > WPC_CHARGE_CURRENT_FASTCHG_MID) {				
//				chg_err("<~WPC~> The tempearture is >= 39. Set current %dmA.\n", WPC_CHARGE_CURRENT_FASTCHG_MID);
//				p922x_set_rx_charge_current(chip, WPC_CHARGE_CURRENT_FASTCHG_MID);

//				work_delay = WPC_ADJUST_CV_DELAY;
//				break;
//			}
		} else if (g_oppo_chip->temperature < 370) {
			if (flag_temperature_abnormal) {
				flag_temperature_abnormal = false;

				if (chip->p922x_chg_status.charge_current < WPC_CHARGE_CURRENT_FASTCHG) {
					chg_err("<~WPC~> The tempearture is < 37. Icrease current.\n");
					chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_INCREASE_CURRENT;
					break;
				}
			}
		} 

		if ((chip->p922x_chg_status.charge_current <= WPC_CHARGE_CURRENT_DEFAULT) 
			&& (chip->p922x_chg_status.charge_voltage > WPC_CHARGE_VOLTAGE_FASTCHG)) {
			chg_err("<~WPC~> The charge current <= 500mA. The charge voltage turn to 12V\n");
			p922x_set_rx_charge_voltage(chip, WPC_CHARGE_VOLTAGE_FASTCHG);
			work_delay = WPC_ADJUST_CV_DELAY; 
			break;
		}

		if (g_oppo_chip->temperature < 400) {
			if(p922x_get_CEP_now_flag(chip) == 0) {
				if (chip->p922x_chg_status.iout > WPC_CHARGE_1A_UPPER_LIMIT) {
					chg_err("<~WPC~> The Iout > 1050mA.\n");

					if (chip->p922x_chg_status.charge_voltage >= WPC_CHARGE_VOLTAGE_FASTCHG_MAX) {
						temp_value = p922x_decrease_vc_by_step(chip, 
																chip->p922x_chg_status.charge_current, 
																WPC_CHARGE_CURRENT_DEFAULT, 
																WPC_CHARGE_CURRENT_CHANGE_STEP_50MA);
						if (temp_value != 0) {
							p922x_set_rx_charge_current(chip, temp_value);
							work_delay = WPC_ADJUST_CV_DELAY; 
							break;
						}
					} else {
						temp_value = p922x_increase_vc_by_step(chip, 
																chip->p922x_chg_status.charge_voltage, 
																WPC_CHARGE_VOLTAGE_FASTCHG_MAX, 
																WPC_CHARGE_VOLTAGE_CHANGE_STEP_20MV);
						if (temp_value != 0) {
							p922x_set_rx_charge_voltage(chip, temp_value);
							work_delay = WPC_ADJUST_CV_DELAY;
							break;
						}
					}
				} else if (chip->p922x_chg_status.iout < WPC_CHARGE_1A_LOWER_LIMIT) {
					chg_err("<~WPC~> The Iout < 950mA.\n");

					if (chip->p922x_chg_status.charge_voltage > WPC_CHARGE_VOLTAGE_FASTCHG) {
						temp_value = p922x_decrease_vc_by_step(chip, 
																chip->p922x_chg_status.charge_voltage, 
																WPC_CHARGE_VOLTAGE_FASTCHG, 
																WPC_CHARGE_VOLTAGE_CHANGE_STEP_20MV);
						if (temp_value != 0) {
							p922x_set_rx_charge_voltage(chip, temp_value);
							work_delay = WPC_ADJUST_CV_DELAY; 
							break;
						}
					} else {
						temp_value = p922x_increase_vc_by_step(chip, 
																chip->p922x_chg_status.charge_current, 
																chip->p922x_chg_status.cc_value_limit, 
																WPC_CHARGE_CURRENT_CHANGE_STEP_50MA);
						if (temp_value != 0) {
							p922x_set_rx_charge_current(chip, temp_value);
							work_delay = WPC_ADJUST_CV_DELAY;
							break;
						}
					}
				} else {

				}

				cep_nonzero_cnt = 0;
			}
			else
			{
				cep_nonzero_cnt++;
				if(cep_nonzero_cnt >= 3) {
					chg_err("<~WPC~> CEP nonzero >= 3.\n");

					if (chip->p922x_chg_status.charge_current > WPC_CHARGE_CURRENT_FASTCHG_MID) {
						temp_value = p922x_decrease_vc_by_step(chip, 
																chip->p922x_chg_status.charge_current, 
																WPC_CHARGE_CURRENT_FASTCHG_MID, 
																WPC_CHARGE_CURRENT_CHANGE_STEP_50MA);
						if (temp_value != 0) {
							p922x_set_rx_charge_current(chip, temp_value);
							//work_delay = WPC_ADJUST_CV_DELAY; 
							break;
						}
					} else if (chip->p922x_chg_status.charge_voltage > WPC_CHARGE_VOLTAGE_FASTCHG) {
						temp_value = p922x_decrease_vc_by_step(chip, 
																chip->p922x_chg_status.charge_voltage, 
																WPC_CHARGE_VOLTAGE_FASTCHG, 
																WPC_CHARGE_VOLTAGE_CHANGE_STEP_20MV);
						if (temp_value != 0) {
							p922x_set_rx_charge_voltage(chip, temp_value);
							//work_delay = WPC_ADJUST_CV_DELAY; 
							break;
						}
					} else {
						p922x_set_rx_charge_current(chip, WPC_CHARGE_CURRENT_DEFAULT);
						//work_delay = WPC_ADJUST_CV_DELAY; 
						break;
					}

					cep_nonzero_cnt = 0;
				}
			}
		}

		if (g_oppo_chip->batt_volt >= 4450) {
			batt_vol_4450_cnt++;
			if (batt_vol_4450_cnt < 5) {
				break;
			}

			chg_err("<~WPC~> The max cell vol >= 4450mV.\n");
			
			batt_vol_4450_cnt = 0;

			temp_value = p922x_decrease_vc_by_step(chip, 
													chip->p922x_chg_status.charge_current, 
													WPC_CHARGE_CURRENT_FASTCHG_END, 
													WPC_CHARGE_CURRENT_CHANGE_STEP_200MA);
			if (temp_value != 0) {
				chip->p922x_chg_status.cc_value_limit = temp_value;
				p922x_set_rx_charge_current(chip, temp_value);
			
				work_delay = WPC_ADJUST_CV_DELAY;
				break;
			}
		} else {
			batt_vol_4450_cnt = 0;
		}
		
		break;

	case WPC_CHG_STATUS_READY_FOR_FTM:
		chg_err("<~WPC~> ..........WPC_CHG_STATUS_READY_FOR_FTM..........\n");
		if (p922x_get_CEP_flag(chip) == 0) {
			temp_value = p922x_increase_vc_by_step(chip, 
													chip->p922x_chg_status.charge_voltage, 
													WPC_CHARGE_VOLTAGE_FTM, 
													WPC_CHARGE_VOLTAGE_CHANGE_STEP_1V);
			if (temp_value != 0) {
				p922x_set_rx_charge_voltage(chip, temp_value);
				msleep(100);
				p922x_begin_CEP_detect(chip);
			} else {
				mp2650_disable_charging();
				chargepump_set_for_LDO();
				
				chip->p922x_chg_status.charge_status = WPC_CHG_STATUS_FTM_WORKING;
			}
		}	
		break;
		
	case WPC_CHG_STATUS_FTM_WORKING:
		chg_err("<~WPC~> ..........WPC_CHG_STATUS_FTM_WORKING..........\n");
		break;
		
	default:
		break;
	}
	
	return 0;
}


static void oppo_wpc_task_work_process(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct oppo_wpc_chip *chip = container_of(dwork, struct oppo_wpc_chip, wpc_task_work);
	chg_err("tongfeng test charge_online[%d]\n", chip->charge_online);

	if (chip->charge_online) {
        oppo_wpc_get_charging_info(chip);
		//p922x_get_vrect_iout(chip);

		oppo_wpc_RXTX_message_process(chip);
		oppo_wpc_temperature_status_process(chip);
		oppo_wpc_charge_status_process(chip);   ///////////////// at here
		p922x_terminate_and_recharge_process(chip);
	}

	if (chip->charge_online) {
		/* run again after interval */
		schedule_delayed_work(&chip->wpc_task_work, WPC_TASK_INTERVAL);
	}
}



