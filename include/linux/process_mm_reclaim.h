/* Kui.Zhang@PSW.BSP.Kernel.Performance,
 * Date: 2019-01-01
 * Extract the reclaim core code from task_mmu.c for /proc/process_reclaim
 */
#ifndef __PROCESS_MM_RECLAIM_H__
#define __PROCESS_MM_RECLAIM_H__
#ifdef CONFIG_PROCESS_RECLAIM_ENHANCE

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/mm.h>

#define PR_PASS		0
#define PR_SEM_OUT	1
#define PR_TASK_FG	2
#define PR_TIME_OUT	3
#define PR_ADDR_OVER	4
#define PR_FULL		5
#define PR_TASK_RUN	6
#define PR_TASK_DIE	7

#define RECLAIM_TIMEOUT_JIFFIES (HZ/3)
#define RECLAIM_PAGE_NUM 1024ul

extern void record_irq_exit_ts(void);
extern void record_irq_enter_ts(void);
extern void collect_reclaimed_task(struct task_struct *prev, struct task_struct *next);
extern int is_reclaim_should_cancel(struct mm_walk *walk);
extern int is_reclaim_addr_over(struct mm_walk *walk, unsigned long addr);
#endif /* CONFIG_PROCESS_RECLAIM_ENHANCE */
#endif /* __PROCESS_MM_RECLAIM_H__ */
