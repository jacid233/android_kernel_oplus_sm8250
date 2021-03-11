#ifndef _OPPO_SPECIAL_OPT
#define _OPPO_SPECIAL_OPT
#define HEAVY_LOAD_RUNTIME (64000000)
#define HEAVY_LOAD_SCALE (80)
extern bool is_heavy_load_task(struct task_struct *p);
#endif
