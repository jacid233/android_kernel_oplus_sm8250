#include <linux/sched.h>
#include <linux/reciprocal_div.h>
#include <../kernel/sched/sched.h>
#include "oppo_special_opt.h"

bool is_heavy_load_task(struct task_struct *p)
{
	int cpu;
	unsigned long thresh_load;
	struct reciprocal_value spc_rdiv = reciprocal_value(100);
	if (!sysctl_cpu_multi_thread || !p)
		return false;
	cpu = task_cpu(p);
	thresh_load = capacity_orig_of(cpu) * HEAVY_LOAD_SCALE;
	if (task_util(p) > reciprocal_divide(thresh_load,spc_rdiv))
		return true;
	return false;
}
