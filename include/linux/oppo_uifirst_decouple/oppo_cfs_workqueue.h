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
#ifndef _OPPO_WORKQUEUE_H_
#define _OPPO_WORKQUEUE_H_

struct worker;
#ifdef CONFIG_OPLUS_SYSTEM_KERNEL_QCOM
int is_uxwork(struct work_struct *work);
inline int set_uxwork(struct work_struct *work);
inline int unset_uxwork(struct work_struct *work);
inline void set_ux_worker_task(struct task_struct *task);
inline void reset_ux_worker_task(struct task_struct *task);
#else /* CONFIG_OPLUS_SYSTEM_KERNEL_QCOM */
static inline int is_uxwork(struct work_struct *work) { return false; }
static inline int set_uxwork(struct work_struct *work) { return false; }
static inline int unset_uxwork(struct work_struct *work) { return false; }
static inline void set_ux_worker_task(struct task_struct *task) {}
static inline void reset_ux_worker_task(struct task_struct *task) {}
#endif /* CONFIG_OPLUS_SYSTEM_KERNEL_QCOM */

#endif//_OPPO_WORKQUEUE_H_
#endif