/**********************************************************************************
* Copyright (c), 2008-2019 , Guangdong OPPO Mobile Comm Corp., Ltd.
* VENDOR_EDIT
* File: oppo_cfs_workqueue.c
* Description: UI First
* Version: 3.0
* Date: 2020-06-25
* Author: caichen@TECH.BSP.Kernel.Sched
* ------------------------------ Revision History: --------------------------------
* <version>           <date>                <author>                            <desc>
* Revision 1.0      2020-06-25     caichen@TECH.BSP.Kernel.Sched      Created for UI First workqueue
***********************************************************************************/
#ifdef OPLUS_FEATURE_UIFIRST
#ifdef CONFIG_OPLUS_SYSTEM_KERNEL_QCOM

#include <linux/workqueue.h>
#include <linux/sched.h>

int is_uxwork(struct work_struct *work){

	if(!sysctl_uifirst_enabled)
		return 0;

	return work->ux_work;
}

inline int set_uxwork(struct work_struct *work){
	if(!sysctl_uifirst_enabled)
		return false;
	return work->ux_work = 1;
}

inline int unset_uxwork(struct work_struct *work){
	if(!sysctl_uifirst_enabled)
		return false;
	return work->ux_work = 0;
}

inline void set_ux_worker_task(struct task_struct *task)
{
	task->static_ux = 1;
}

inline void reset_ux_worker_task(struct task_struct *task)
{
	task->static_ux = 0;
}

#endif /* CONFIG_OPLUS_SYSTEM_KERNEL_QCOM */
#endif