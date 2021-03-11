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

#include <linux/string.h>
#include <linux/bug.h>
#include "platform.h"

static bool arch_of_qcom(void)
{
#ifdef CONFIG_OPLUS_SYSTEM_KERNEL_QCOM
	return true;
#endif
	return false;
}

static bool arch_of_mtk(void)
{
#ifdef CONFIG_OPLUS_SYSTEM_KERNEL_MTK
	return true;
#endif
	return false;
}

bool arch_of(const char *arch)
{
	if (!strncasecmp(arch, "qcom", strlen("qcom")))
		return arch_of_qcom();

	if (!strncasecmp(arch, "mtk", strlen("mtk")))
		return arch_of_mtk();

	BUG();

	return false;
}
