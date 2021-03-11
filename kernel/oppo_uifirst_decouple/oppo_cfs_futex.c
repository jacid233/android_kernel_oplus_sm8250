/**********************************************************************************
* Copyright (c), 2008-2019 , Guangdong OPPO Mobile Comm Corp., Ltd.
* VENDOR_EDIT
* File: oppo_cfs_futex.c
* Description: UI First
* Version: 2.0
* Date: 2019-10-01
* Author: Liujie.Xie@TECH.BSP.Kernel.Sched
* ------------------------------ Revision History: --------------------------------
* <version>           <date>                <author>                            <desc>
* Revision 1.0        2019-05-22       Liujie.Xie@TECH.BSP.Kernel.Sched      Created for UI First
* Revision 2.0        2019-10-01       Liujie.Xie@TECH.BSP.Kernel.Sched      Add for UI First 2.0
***********************************************************************************/

#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/pid.h>
#include "oppo_cfs_common.h"

#define CREATE_TRACE_POINTS
#include "oplus_sched_trace.h"

struct task_struct* get_futex_owner_by_pid(u32 owner_tid) {
    struct task_struct* futex_owner = NULL;

    if (owner_tid > 0 && owner_tid <= PID_MAX_DEFAULT) {
        rcu_read_lock();
        futex_owner = find_task_by_vpid(owner_tid);
        rcu_read_unlock();
        if (futex_owner == NULL) {
            ux_warn("failed to find task by pid(curr:%-12s pid:%d)\n", current->comm, owner_tid);
        }
    }

    return futex_owner;
}

struct task_struct *get_futex_owner(u32 owner_tid) {
    struct task_struct *futex_owner = NULL;

    if (owner_tid > 0 && owner_tid <= PID_MAX_DEFAULT) {
        rcu_read_lock();
        futex_owner = find_task_by_vpid(owner_tid);
        rcu_read_unlock();
        if (futex_owner == NULL) {
            ux_warn("failed to find task by pid(curr:%-12s pid:%d)\n", current->comm, owner_tid);
        }
    }

    return futex_owner;
}

void futex_dynamic_ux_enqueue(struct task_struct *owner, struct task_struct *task)
{
	bool is_ux = false;
	is_ux = test_set_dynamic_ux(task);

	if (is_ux && owner && !test_task_ux(owner)) {
		dynamic_ux_enqueue(owner, DYNAMIC_UX_FUTEX, task->ux_depth);
	}
}

void futex_dynamic_ux_dequeue(struct task_struct *task)
{
	if (test_dynamic_ux(task, DYNAMIC_UX_FUTEX)) {
		dynamic_ux_dequeue(task, DYNAMIC_UX_FUTEX);
	}
}

void futex_dynamic_ux_enqueue_refs(struct task_struct *owner, struct task_struct *task)
{
    bool is_ux = test_set_dynamic_ux(task);

    if (is_ux && owner) {
        bool is_owner_ux = test_task_ux(owner);
        if (!is_owner_ux){
            dynamic_ux_enqueue(owner, DYNAMIC_UX_FUTEX, task->ux_depth);
        } else {
            dynamic_ux_inc(owner, DYNAMIC_UX_FUTEX);
        }
    }

    if (owner)
    trace_oplus_tp_sched_change_ux(test_task_ux(owner) ? 3 : 0, owner->pid);
}

void futex_dynamic_ux_dequeue_refs(struct task_struct *task, int value)
{

    if (test_dynamic_ux(task, DYNAMIC_UX_FUTEX)) {
        dynamic_ux_dequeue_refs(task, DYNAMIC_UX_FUTEX, value);
    }

    if (task)
    trace_oplus_tp_sched_change_ux(test_task_ux(task) ? 3 : 0, task->pid);
}

