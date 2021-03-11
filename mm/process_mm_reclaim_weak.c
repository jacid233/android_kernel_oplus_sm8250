#include <linux/process_mm_reclaim.h>

int __weak create_process_reclaim_enable_proc(struct proc_dir_entry *parent)
{
	return 0;
}
