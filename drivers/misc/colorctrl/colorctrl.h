/************************************************************
 * Copyright 2020 OPLUS Mobile Comm Corp., Ltd.
 * All rights reserved.
 *
 * Description  : driver for oplus color ctrl
 * History      : ( ID, Date, Author, Description)
 * Author       : Zengpeng.Chen@BSP.TP.Misc
 * Data         : 2020/08/06
 ************************************************************/
#ifndef H_COLORCTRL
#define H_COLORCTRL

#include <linux/printk.h>

#define COLOR_INFO(fmt, args...) \
    pr_info("COLOR-CTRL: %s:" fmt "\n", __func__, ##args)

#define GPIO_HIGH (1)
#define GPIO_LOW  (0)
#define PAGESIZE  512

typedef enum color_ctrl_type {
    LIGHT_BLUE,
    BLUE,
    TRANSPARENT,
    RECHARGE_BLUE,
    RECHARGE_TRANSPARENT,
} color_status;

typedef enum color_ctrl_temp_type {
    LOW_TEMP,
    ROOM_TEMP,
    HIGH_TEMP,
    ABNORMAL_TEMP,
} temp_status;

struct color_ctrl_hw_resource {
    unsigned int        sleep_en_gpio;
    unsigned int        si_in_1_gpio;
    unsigned int        si_in_2_gpio;
    struct regulator    *vm;        /*driver power*/
};

struct color_ctrl_device {
    struct platform_device          *pdev;
    struct device                   *dev;
    struct mutex                    rw_lock;
    struct proc_dir_entry           *prEntry_cr;
    struct color_ctrl_hw_resource   *hw_res;
    color_status                    color_status;
    temp_status                     temp_status;
    int                             platform_support_project[10];
    int                             project_num;
};

#endif
