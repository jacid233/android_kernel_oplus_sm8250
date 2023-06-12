#include <linux/kernel.h>

extern void mmput(struct mm_struct *);

uintptr_t get_module_base(pid_t pid, char* name);
