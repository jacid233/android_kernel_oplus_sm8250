/**********************************************************************************
* Copyright (c), 2008-2020 , Guangdong OPPO Mobile Comm Corp., Ltd.
* VENDOR_EDIT
* File: oppo_jank_monitor.h
* Description: Jank Monitor
* Version: 1.0
* Date: 2020-05-07
* Author: Liujie.Xie@TECH.BSP.Kernel.Sched
* ------------------------------ Revision History: --------------------------------
* <version>           <date>                <author>                            <desc>
* Revision 1.0        2020-05-07       Liujie.Xie@TECH.BSP.Kernel.Sched      Created for Jank Monitor
***********************************************************************************/

#ifndef _OPPO_JANK_MONITOR_H_
#define _OPPO_JANK_MONITOR_H_

enum {
    JANK_TRACE_RUNNABLE = 0,
    JANK_TRACE_DSTATE,
    JANK_TRACE_SSTATE,
    JANK_TRACE_RUNNING,
};

struct jank_d_state {
	u64 iowait_ns;
	u64 downread_ns;
	u64 downwrite_ns;
	u64 mutex_ns;
	u64 other_ns;
	int cnt;
};

struct jank_s_state{
	u64 binder_ns;
	u64 epoll_ns;
	u64 futex_ns;
	u64 other_ns;
	int cnt;
};

struct oppo_jank_monitor_info {
	u64 runnable_state;
	u64 ltt_running_state; /* ns */
	u64 big_running_state; /* ns */
	struct jank_d_state d_state;
	struct jank_s_state s_state;
};

extern const struct file_operations proc_jank_trace_operations;

#endif /*_OPPO_JANK_MONITOR_H_*/
