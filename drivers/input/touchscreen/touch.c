/***************************************************
 * File:touch.c
 * VENDOR_EDIT
 * Copyright (c)  2008- 2030  Oppo Mobile communication Corp.ltd.
 * Description:
 *             tp dev
 * Version:1.0:
 * Date created:2016/09/02
 * Author: hao.wang@Bsp.Driver
 * TAG: BSP.TP.Init
*/

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/serio.h>
#include <linux/regulator/consumer.h>
#include "oppo_touchscreen/tp_devices.h"
#include "oppo_touchscreen/touchpanel_common.h"
//#include <soc/oppo/oppo_project.h>

#define MAX_LIMIT_DATA_LENGTH         100

/*if can not compile success, please update vendor/oppo_touchsreen*/
struct tp_dev_name tp_dev_names[] = {
     {TP_OFILM, "OFILM"},
     {TP_BIEL, "BIEL"},
     {TP_TRULY, "TRULY"},
     {TP_BOE, "BOE"},
     {TP_G2Y, "G2Y"},
     {TP_TPK, "TPK"},
     {TP_JDI, "JDI"},
     {TP_TIANMA, "TIANMA"},
     {TP_SAMSUNG, "SAMSUNG"},
     {TP_DSJM, "DSJM"},
     {TP_BOE_B8, "BOEB8"},
     {TP_UNKNOWN, "UNKNOWN"},
};

int g_tp_dev_vendor = TP_UNKNOWN;
typedef enum {
    TP_INDEX_NULL,
    SAMSUNG_Y791,
    BOE_S3908,
    SAMSUNG_Y771
} TP_USED_INDEX;
TP_USED_INDEX tp_used_index  = TP_INDEX_NULL;



#define GET_TP_DEV_NAME(tp_type) ((tp_dev_names[tp_type].type == (tp_type))?tp_dev_names[tp_type].name:"UNMATCH")

bool __init tp_judge_ic_match(char *tp_ic_name)
{
    pr_err("[TP] tp_ic_name = %s \n", tp_ic_name);
    //pr_err("[TP] boot_command_line = %s \n", boot_command_line);

    if (strstr(tp_ic_name, "sec-s6sy791")
        && strstr(boot_command_line, "mdss_dsi_oppo19065_samsung_1440_3168_dsc_cmd")) {
        g_tp_dev_vendor = TP_SAMSUNG;
        tp_used_index = SAMSUNG_Y791;
        return true;
    }

    if (strstr(tp_ic_name, "synaptics-s3908")
        && strstr(boot_command_line, "mdss_dsi_oppo19161boe_nt37800_1080_2400_cmd")) {
        g_tp_dev_vendor = TP_BOE;
        tp_used_index = BOE_S3908;
        return true;
    }

    if (strstr(tp_ic_name, "sec-s6sy771")
        && strstr(boot_command_line, "mdss_dsi_oppo19161samsung_amb655uv01_1080_2400_cmd")) {
        g_tp_dev_vendor = TP_SAMSUNG;
        tp_used_index = SAMSUNG_Y771;
        return true;
    }
    pr_err("[TP] Driver does not match the project\n");


    pr_err("Lcd module not found\n");

    return false;
}


int tp_util_get_vendor(struct hw_resource *hw_res, struct panel_info *panel_data)
{
    char* vendor;
    int prj_id = 0;

    panel_data->test_limit_name = kzalloc(MAX_LIMIT_DATA_LENGTH, GFP_KERNEL);
    if (panel_data->test_limit_name == NULL) {
        pr_err("[TP]panel_data.test_limit_name kzalloc error\n");
    }

    panel_data->tp_type = TP_SAMSUNG;
    prj_id = panel_data->project_id;
    switch (prj_id) {
    case 19065:
        panel_data->tp_type = TP_SAMSUNG;
        panel_data->vid_len = 2;
        memcpy(panel_data->manufacture_info.version, "SL", 2);
        break;
    case 19161:
        if (tp_used_index == BOE_S3908) {
            panel_data->tp_type = g_tp_dev_vendor;
            panel_data->vid_len = 2;
            memcpy(panel_data->manufacture_info.version, "BS", 2);

        } else if (tp_used_index == SAMSUNG_Y771) {
            panel_data->tp_type = g_tp_dev_vendor;
            panel_data->vid_len = 2;
            memcpy(panel_data->manufacture_info.version, "SL", 2);
        }

    }

    if (panel_data->tp_type == TP_UNKNOWN) {
        pr_err("[TP]%s type is unknown\n", __func__);
        return 0;
    }

    vendor = GET_TP_DEV_NAME(panel_data->tp_type);

    strcpy(panel_data->manufacture_info.manufacture, vendor);
    snprintf(panel_data->fw_name, MAX_FW_NAME_LENGTH,
            "tp/%d/FW_%s_%s.img",
            prj_id, panel_data->chip_name, vendor);

    if (panel_data->test_limit_name) {
        snprintf(panel_data->test_limit_name, MAX_LIMIT_DATA_LENGTH,
            "tp/%d/LIMIT_%s_%s.img",
            prj_id, panel_data->chip_name, vendor);
    }

    panel_data->manufacture_info.fw_path = panel_data->fw_name;

    pr_info("[TP]vendor:%s fw:%s limit:%s\n",
        vendor,
        panel_data->fw_name,
        panel_data->test_limit_name==NULL?"NO Limit":panel_data->test_limit_name);
    return 0;
}

int preconfig_power_control(struct touchpanel_data *ts)
{/*
    int pcb_verison = -1;

    pcb_verison = get_PCB_Version();
    if (is_project(OPPO_17107) || is_project(OPPO_17127)) {
        if ((is_project(OPPO_17107) && ((pcb_verison < 4) || (pcb_verison == 5))) || (is_project(OPPO_17127) && (pcb_verison == 1))) {
        } else {
            pr_err("[TP]set voltage to 3.0v\n");
            ts->hw_res.vdd_volt = 3000000;
        }
    }
   */
    return 0;
}
EXPORT_SYMBOL(preconfig_power_control);

int reconfig_power_control(struct touchpanel_data *ts)
{
/*
    int pcb_verison = -1;

    pcb_verison = get_PCB_Version();
    if (is_project(OPPO_17107) || is_project(OPPO_17127)) {
        if ((is_project(OPPO_17107) && ((pcb_verison < 4) || (pcb_verison == 5))) || (is_project(OPPO_17127) && (pcb_verison == 1))) {
            pr_err("[TP]2v8 gpio free\n");
            if (gpio_is_valid(ts->hw_res.enable2v8_gpio)) {
                gpio_free(ts->hw_res.enable2v8_gpio);
                ts->hw_res.enable2v8_gpio = -1;
            }
        } else {
            pr_err("[TP]2v8 regulator put\n");
            if (!IS_ERR_OR_NULL(ts->hw_res.vdd_2v8)) {
                regulator_put(ts->hw_res.vdd_2v8);
                ts->hw_res.vdd_2v8 = NULL;
            }
        }
    }
*/
    return 0;
}
EXPORT_SYMBOL(reconfig_power_control);

