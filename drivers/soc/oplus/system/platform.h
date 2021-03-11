/*
 * Copyright (C), 2020, OPPO Mobile Comm Corp., Ltd.
 * File: - arch.c
 * Description: which platform you use.
 * Version: 1.0
 * Date: 2020/03/28
 * Author: Xiong.xing@BSP.Kernel.Stability
 *
 *----------------------Revision History: ---------------------------
 *   <author>        <date>         <version>         <desc>
 *  xiong.xing     2020/03/28        1.0              created
 *-------------------------------------------------------------------
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <linux/string.h>
#include <linux/bug.h>

/*
* arch_of("mtk") or arch_of("qcom")
*/
bool arch_of(const char *arch);

#endif