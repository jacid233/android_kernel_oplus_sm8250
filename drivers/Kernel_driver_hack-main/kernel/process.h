#include <linux/kernel.h>

extern struct mm_struct *get_task_mm(struct task_struct *task);

uintptr_t get_module_base(pid_t pid, char* name);
