/*
 * process_mm_reclaim.c
 *
 */
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched/task.h>
#include <linux/seq_file.h>
#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/rmap.h>
#include <linux/cred.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/mm_inline.h>
#include <linux/process_mm_reclaim.h>
#include <linux/swap.h>
#include <soc/oppo/oppo_project.h>
#include <linux/hugetlb.h>
#include <linux/huge_mm.h>
#include <asm/tlb.h>
#include <asm/tlbflush.h>

/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-12-25, check current need
 * cancel reclaim or not, please check task not NULL first.
 * If the reclaimed task has goto foreground, cancel reclaim immediately*/
#define RECLAIM_SCAN_REGION_LEN (400ul<<20)

enum reclaim_type {
	RECLAIM_FILE,
	RECLAIM_ANON,
	RECLAIM_ALL,
	RECLAIM_RANGE,
	/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-11-07,
	 * add three reclaim_type that only reclaim inactive pages */
	RECLAIM_INACTIVE_FILE,
	RECLAIM_INACTIVE_ANON,
	RECLAIM_INACTIVE,
};

struct reclaim_info {
	char comm[TASK_COMM_LEN];
	enum reclaim_type type;
	int nr_reclaimed;
	int nr_scanned;
	unsigned long delay_ms;
	unsigned long nvcsw;
	unsigned long nivcsw;
	unsigned long running_ms;
	unsigned long intr_ms;
	unsigned long stop_addr;
	int ahead;
	int release_sem_cnt;
};

static struct reclaim_info ri_task;

static unsigned long reclaim_ns;
static unsigned long reclaim_run_ns;
static unsigned long reclaim_intr_start_ns;
static unsigned long reclaim_intr_ns;

/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2019-02-26,
 * collect reclaimed_shrinked task schedule record
 */
void collect_reclaimed_task(struct task_struct *prev, struct task_struct *next)
{
	if (next->flags & PF_RECLAIM_SHRINK)
		reclaim_ns = sched_clock();

	if (prev->flags & PF_RECLAIM_SHRINK)
		reclaim_run_ns += sched_clock() - reclaim_ns;
}
EXPORT_SYMBOL(collect_reclaimed_task);

/* Kui.Zhang@PSW.TEC.Kernel.Performance, 2019/02/27
 * collect interrupt doing time during process reclaim, only effect in age test
 */
void record_irq_enter_ts(void)
{
	if (current->flags & PF_RECLAIM_SHRINK)
		reclaim_intr_start_ns = sched_clock();
}
EXPORT_SYMBOL(record_irq_enter_ts);

/* Kui.Zhang@PSW.TEC.Kernel.Performance, 2019/02/27
 * collect interrupt doing time during process reclaim, only effect in age test
 */
void record_irq_exit_ts(void)
{
	if (current->flags & PF_RECLAIM_SHRINK)
		reclaim_intr_ns += (unsigned long)(sched_clock() -
				reclaim_intr_start_ns);
}
EXPORT_SYMBOL(record_irq_exit_ts);

static int mm_reclaim_pte_range(pmd_t *pmd, unsigned long addr,
		unsigned long end, struct mm_walk *walk)
{
	struct reclaim_param *rp = walk->private;
	struct vm_area_struct *vma = rp->vma;
	pte_t *pte, ptent;
	spinlock_t *ptl;
	struct page *page;
	LIST_HEAD(page_list);
	int isolated;
	int reclaimed;
	int ret = 0;

	split_huge_pmd(vma, addr, pmd);
	if (pmd_trans_unstable(pmd) || !rp->nr_to_reclaim)
		return 0;
cont:
	isolated = 0;
	pte = pte_offset_map_lock(vma->vm_mm, pmd, addr, &ptl);
	for (; addr != end; pte++, addr += PAGE_SIZE) {
		/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-12-25, check whether the
		 * reclaim process should cancel*/
#if defined(VENDOR_EDIT) && defined(CONFIG_PROCESS_RECLAIM_ENHANCE)
		if (rp->reclaimed_task &&
				(ret = is_reclaim_addr_over(walk, addr))) {
			ret = -ret;
			break;
		}
#endif
		ptent = *pte;
		if (!pte_present(ptent))
			continue;

		page = vm_normal_page(vma, addr, ptent);
		if (!page)
			continue;

		/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-11-07,
		 * we don't reclaim page in active lru list */
		if (rp->inactive_lru && (PageActive(page) ||
					PageUnevictable(page)))
			continue;

		if (isolate_lru_page(page))
			continue;

		/* MADV_FREE clears pte dirty bit and then marks the page
		 * lazyfree (clear SwapBacked). Inbetween if this lazyfreed page
		 * is touched by user then it becomes dirty.  PPR in
		 * shrink_page_list in try_to_unmap finds the page dirty, marks
		 * it back as PageSwapBacked and skips reclaim. This can cause
		 * isolated count mismatch.
		 */
		if (PageAnon(page) && !PageSwapBacked(page)) {
			putback_lru_page(page);
			continue;
		}

		list_add(&page->lru, &page_list);
		inc_node_page_state(page, NR_ISOLATED_ANON +
				page_is_file_cache(page));
		isolated++;
		rp->nr_scanned++;
		if ((isolated >= SWAP_CLUSTER_MAX) || !rp->nr_to_reclaim)
			break;
	}
	pte_unmap_unlock(pte - 1, ptl);

	/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-12-25, check whether the
	 * reclaim process should cancel*/
	reclaimed = reclaim_pages_from_list(&page_list, vma, walk);

	rp->nr_reclaimed += reclaimed;
	rp->nr_to_reclaim -= reclaimed;
	if (rp->nr_to_reclaim < 0)
		rp->nr_to_reclaim = 0;

	/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-12-25, if want to cancel,
	 * if ret <0 means need jump out of the loop immediately
	 */
	if (ret < 0)
		return ret;
	if (!rp->nr_to_reclaim)
		return -PR_FULL;
	if (addr != end)
		goto cont;
	return 0;
}

static int write_process_reclaim_info(char* msg)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos;

	if (likely(AGING != get_eng_version()))
	{
		return 0;
	}

	fp = filp_open("/sdcard/proces_reclaim_info.txt",
			O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		pr_err("KABLE create /sdcard/proces_reclaim_info.txt failed\n");
		return -1;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	pos = vfs_llseek(fp, 0, SEEK_END);
	vfs_write(fp, msg, strlen(msg), &pos);
	filp_close(fp, NULL);
	set_fs(fs);
	return 0;
}

/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2019-01-01,
 * Extract the reclaim core code for /proc/process_reclaim use*/
static ssize_t reclaim_task_write(struct task_struct* task, char *buffer)
{
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	enum reclaim_type type;
	char *type_buf;
	struct mm_walk reclaim_walk = {};
	unsigned long start = 0;
	unsigned long end = 0;
	struct reclaim_param rp;
	int err = 0;
	int scan_vma_cnt = 0;
	int nr_scanned = 0;
	int nr_reclaimed = 0;
	int release_sem_cnt = 0;
	unsigned long start_ns;
	unsigned long nvcsw_start, nivcsw_start;
	unsigned long run_ms;
	unsigned long before_scan_addr;
	struct timeval tv;
	char msg[256] = {0};

	/* Kui.Zhang@TEC.Kernel.Performance, 2019/03/04
	 * Do not reclaim self
	 */
	if (task == current->group_leader)
		goto out_err;

	type_buf = strstrip(buffer);
	if (!strcmp(type_buf, "file"))
		type = RECLAIM_FILE;
	else if (!strcmp(type_buf, "anon"))
		type = RECLAIM_ANON;
	else if (!strcmp(type_buf, "all"))
		type = RECLAIM_ALL;
#ifdef CONFIG_PROCESS_RECLAIM_ENHANCE
	/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-11-07,
	 * Check the input reclaim option is inactive
	 */
	else if (!strcmp(type_buf, "inactive"))
		type = RECLAIM_INACTIVE;
	else if (!strcmp(type_buf, "inactive_file"))
		type = RECLAIM_INACTIVE_FILE;
	else if (!strcmp(type_buf, "inactive_anon"))
		type = RECLAIM_INACTIVE_ANON;
#endif
	else if (isdigit(*type_buf))
		type = RECLAIM_RANGE;
	else
		goto out_err;

	if (type == RECLAIM_RANGE) {
		char *token;
		unsigned long long len, len_in, tmp;
		token = strsep(&type_buf, " ");
		if (!token)
			goto out_err;
		tmp = memparse(token, &token);
		if (tmp & ~PAGE_MASK || tmp > ULONG_MAX)
			goto out_err;
		start = tmp;

		token = strsep(&type_buf, " ");
		if (!token)
			goto out_err;
		len_in = memparse(token, &token);
		len = (len_in + ~PAGE_MASK) & PAGE_MASK;
		if (len > ULONG_MAX)
			goto out_err;
		/*
		 * Check to see whether len was rounded up from small -ve
		 * to zero.
		 */
		if (len_in && !len)
			goto out_err;

		end = start + len;
		if (end < start)
			goto out_err;
	}

	mm = get_task_mm(task);
	if (!mm)
		goto out;

#ifdef CONFIG_PROCESS_RECLAIM_ENHANCE
	/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-11-07,
	 * Flag that relcaim inactive pages only in mm_reclaim_pte_range
	 */
	if ((type == RECLAIM_INACTIVE) ||
			(type == RECLAIM_INACTIVE_FILE) ||
			(type == RECLAIM_INACTIVE_ANON))
		rp.inactive_lru = true;
	else
		rp.inactive_lru = false;
#endif

	reclaim_walk.mm = mm;
	reclaim_walk.pmd_entry = mm_reclaim_pte_range;
	reclaim_walk.private = &rp;

	/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-12-25,
	 * record the reclaimed task
	 */
	current->flags |= PF_RECLAIM_SHRINK;
	rp.reclaimed_task = task;
	current->reclaim.stop_jiffies = jiffies + RECLAIM_TIMEOUT_JIFFIES;
	if (unlikely(AGING == get_eng_version())) {
		start_ns = sched_clock();
		reclaim_run_ns = 0UL;
		reclaim_intr_ns = 0UL;
		reclaim_ns = start_ns;
		nvcsw_start = current->nvcsw;
		nivcsw_start = current->nivcsw;
		before_scan_addr = task->reclaim.stop_scan_addr;
	}

cont:
	rp.nr_to_reclaim = RECLAIM_PAGE_NUM;
	rp.nr_reclaimed = 0;
	rp.nr_scanned = 0;

	down_read(&mm->mmap_sem);
	if (type == RECLAIM_RANGE) {
		vma = find_vma(mm, start);
		while (vma) {
			if (vma->vm_start > end)
				break;
			if (is_vm_hugetlb_page(vma))
				continue;

			rp.vma = vma;
			walk_page_range(max(vma->vm_start, start),
					min(vma->vm_end, end),
					&reclaim_walk);
			vma = vma->vm_next;
		}
	} else {
		if (unlikely(AGING == get_eng_version())) {
			ri_task.stop_addr = task->reclaim.stop_scan_addr;
		}
		for (vma = mm->mmap; vma; vma = vma->vm_next) {
			if (vma->vm_end <= task->reclaim.stop_scan_addr)
				continue;

			if (is_vm_hugetlb_page(vma))
				continue;

			/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-11-07,
			 * Jump out of the reclaim flow immediately
			 */
			err = is_reclaim_addr_over(&reclaim_walk, vma->vm_start);
			if (err) {
				err = -err;
				break;
			}

#ifdef CONFIG_PROCESS_RECLAIM_ENHANCE
			/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-11-07,
			 * filter only reclaim anon pages
			 */
			if ((type == RECLAIM_ANON ||
						type == RECLAIM_INACTIVE_ANON) && vma->vm_file)
#else
				if (type == RECLAIM_ANON && vma->vm_file)
#endif
					continue;

#ifdef CONFIG_PROCESS_RECLAIM_ENHANCE
			/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-11-07,
			 * filter only reclaim file-backed pages
			 */
			if ((type == RECLAIM_FILE ||
						type == RECLAIM_INACTIVE_FILE) && !vma->vm_file)
#else
				if (type == RECLAIM_FILE && !vma->vm_file)
#endif
					continue;

			rp.vma = vma;
			if (unlikely(AGING == get_eng_version())) {
				scan_vma_cnt++;
			}
			if (vma->vm_start < task->reclaim.stop_scan_addr)
				err = walk_page_range(
						task->reclaim.stop_scan_addr,
						vma->vm_end, &reclaim_walk);
			else
				err = walk_page_range(vma->vm_start,
						vma->vm_end, &reclaim_walk);

			if (err < 0)
				break;
		}

		if (err != -PR_ADDR_OVER)
			task->reclaim.stop_scan_addr = vma ? vma->vm_start : 0;
	}

	flush_tlb_mm(mm);
	up_read(&mm->mmap_sem);

	if (unlikely(AGING == get_eng_version())) {
		release_sem_cnt++;

		// record the reclaim result
		nr_reclaimed += rp.nr_reclaimed;
		nr_scanned += rp.nr_scanned;
		run_ms = (sched_clock() - start_ns)/1000000UL;
		if (unlikely(run_ms > 500UL)) {
			do_gettimeofday(&tv);
			snprintf(msg, 256, "[%lu.%-6lu][ %s %d %d ],[ %d %d %d %d ],[ %lu %lu ],[ %lu %lu %lu ],[ %#lx %#lx %lu %u ]\n",
					tv.tv_sec, tv.tv_usec,
					task->comm, type, err,
					rp.nr_scanned, rp.nr_reclaimed,
					release_sem_cnt, scan_vma_cnt,
					current->nivcsw - nivcsw_start,
					current->nvcsw - nvcsw_start, run_ms,
					reclaim_run_ns/1000000UL,
					reclaim_intr_ns/1000000UL,
					ri_task.stop_addr, task->reclaim.stop_scan_addr,
					RECLAIM_PAGE_NUM,
					jiffies_to_msecs(RECLAIM_TIMEOUT_JIFFIES));
			(void)write_process_reclaim_info(msg);
		}
	}
	/* If not timeout and not reach the mmap end, continue
	*/
	if (((err == PR_PASS) || (err == -PR_ADDR_OVER) ||
				(err == -PR_FULL)) && vma)
		goto cont;

	if (unlikely(AGING == get_eng_version())) {
		memcpy(ri_task.comm, task->comm, TASK_COMM_LEN);
		ri_task.delay_ms = run_ms;
		ri_task.nr_reclaimed = nr_reclaimed;
		ri_task.nr_scanned = nr_scanned;
		ri_task.type = type;
		ri_task.nvcsw = current->nvcsw - nvcsw_start;
		ri_task.nivcsw = current->nivcsw - nivcsw_start;
		ri_task.running_ms = reclaim_run_ns/1000000UL;
		ri_task.intr_ms = reclaim_intr_ns/1000000UL;
		ri_task.ahead = err;
		ri_task.release_sem_cnt = release_sem_cnt;
		ri_task.stop_addr = before_scan_addr;
		do_gettimeofday(&tv);
		snprintf(msg, 256, "[%lu.%-6lu][ %s %d %d ],[ %d %d %d %d ],[ %lu %lu ],[ %lu %lu %lu ],[ %#lx %#lx %lu %u ]**\n",
				tv.tv_sec, tv.tv_usec,
				ri_task.comm, ri_task.type, ri_task.ahead,
				nr_scanned, nr_reclaimed, release_sem_cnt,
				scan_vma_cnt,
				ri_task.nivcsw, ri_task.nvcsw,
				ri_task.delay_ms, ri_task.running_ms, ri_task.intr_ms,
				before_scan_addr, task->reclaim.stop_scan_addr,
				RECLAIM_PAGE_NUM,
				jiffies_to_msecs(RECLAIM_TIMEOUT_JIFFIES));
		(void)write_process_reclaim_info(msg);
	}

	/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2018-12-25, clear the flags*/
	current->flags &= ~PF_RECLAIM_SHRINK;
	mmput(mm);
out:
	return 0;

out_err:
	return -EINVAL;
}

/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2019-05-23,
 * If count < 0 means write sem locked
 */
static inline int rwsem_is_wlocked(struct rw_semaphore *sem)
{
	return atomic_long_read(&sem->count) < 0;
}

static inline int _is_reclaim_should_cancel(struct mm_walk *walk)
{
	struct mm_struct *mm = walk->mm;
	struct task_struct *task;

	if (!mm)
		return 0;

	task = ((struct reclaim_param *)(walk->private))->reclaimed_task;
	if (!task)
		return 0;

	if (mm != task->mm)
		return PR_TASK_DIE;
	if (rwsem_is_wlocked(&mm->mmap_sem))
		return PR_SEM_OUT;
	if (task_is_fg(task))
		return PR_TASK_FG;
	if (task->state == TASK_RUNNING)
		return PR_TASK_RUN;
	if (time_is_before_eq_jiffies(current->reclaim.stop_jiffies))
		return PR_TIME_OUT;

	return 0;
}

int is_reclaim_should_cancel(struct mm_walk *walk)
{
	struct task_struct *task;

	if (!current_is_reclaimer() || !walk->private)
		return 0;

	task = ((struct reclaim_param *)(walk->private))->reclaimed_task;
	if (!task)
		return 0;

	return _is_reclaim_should_cancel(walk);
}

int is_reclaim_addr_over(struct mm_walk *walk, unsigned long addr)
{
	struct task_struct *task;

	if (!current_is_reclaimer() || !walk->private)
		return 0;

	task = ((struct reclaim_param *)(walk->private))->reclaimed_task;
	if (!task)
		return 0;

	if (task->reclaim.stop_scan_addr + RECLAIM_SCAN_REGION_LEN <= addr) {
		task->reclaim.stop_scan_addr = addr;
		return PR_ADDR_OVER;
	}

	return _is_reclaim_should_cancel(walk);
}

/* Kui.Zhang@PSW.BSP.Kernel.Performance, 2019-01-01,
 * Create /proc/process_reclaim interface for process reclaim.
 * Because /proc/$pid/reclaim has deifferent permissiones of different processes,
 * and can not set to 0444 because that have security risk.
 * Use /proc/process_reclaim and setting with selinux */
#ifdef CONFIG_PROC_FS
#define PROCESS_RECLAIM_CMD_LEN 64
static int process_reclaim_enable = 1;
module_param_named(process_reclaim_enable, process_reclaim_enable, int, 0644);

static ssize_t proc_reclaim_write(struct file *file, const char __user *buffer,
		size_t count, loff_t *ppos)
{
	char kbuf[PROCESS_RECLAIM_CMD_LEN];
	char *act_str;
	char *end;
	long value;
	pid_t tsk_pid;
	struct task_struct* tsk;
	ssize_t ret = 0;

	if (!process_reclaim_enable) {
		pr_warn("Process memory reclaim is disabled!\n");
		return -EFAULT;
	}

	if (count > PROCESS_RECLAIM_CMD_LEN)
		return -EINVAL;

	memset(kbuf, 0, PROCESS_RECLAIM_CMD_LEN);
	if (copy_from_user(&kbuf, buffer, count))
		return -EFAULT;
	kbuf[PROCESS_RECLAIM_CMD_LEN - 1] = '\0';

	act_str = strstrip(kbuf);
	if (*act_str <= '0' || *act_str > '9') {
		pr_err("process_reclaim write [%s] pid format is invalid.\n", kbuf);
		return -EINVAL;
	}

	value = simple_strtol(act_str, &end, 10);
	if (value < 0 || value > INT_MAX) {
		pr_err("process_reclaim write [%s] is invalid.\n", kbuf);
		return -EINVAL;
	}

	tsk_pid = (pid_t)value;

	if (end == (act_str + strlen(act_str))) {
		pr_err("process_reclaim write [%s] do not set reclaim type.\n", kbuf);
		return -EINVAL;
	}

	if (*end != ' ' && *end != '	') {
		pr_err("process_reclaim write [%s] format is wrong.\n", kbuf);
		return -EINVAL;
	}

	end = strstrip(end);
	rcu_read_lock();
	tsk = find_task_by_vpid(tsk_pid);
	if (!tsk) {
		rcu_read_unlock();
		pr_err("process_reclaim can not find task of pid:%d\n", tsk_pid);
		return -ESRCH;
	}

	if (tsk != tsk->group_leader)
		tsk = tsk->group_leader;
	get_task_struct(tsk);
	rcu_read_unlock();

	ret = reclaim_task_write(tsk, end);

	put_task_struct(tsk);
	if (ret < 0) {
		pr_err("process_reclaim failed, command [%s]\n", kbuf);
		return ret;
	}

	return count;
}

static int reclaim_info_show(struct seq_file *s, void *unused)
{
	seq_printf(s, "[ %s %d %d ],[ %d %d ],[ %lu %lu ],[ %lu %lu %lu ],[ %#lx %lu %u ]\n",
			ri_task.comm, ri_task.type, ri_task.ahead,
			ri_task.nr_scanned, ri_task.nr_reclaimed,
			ri_task.nivcsw, ri_task.nvcsw,
			ri_task.delay_ms, ri_task.running_ms, ri_task.intr_ms,
			ri_task.stop_addr, RECLAIM_PAGE_NUM,
			jiffies_to_msecs(RECLAIM_TIMEOUT_JIFFIES));
	return 0;
}

static int process_reclaim_open(struct inode *inode, struct file *file)
{
	return single_open(file, reclaim_info_show, NULL);
}

static struct file_operations process_reclaim_w_fops = {
	.write = proc_reclaim_write,
	.llseek = noop_llseek,
};

static struct file_operations process_reclaim_rw_fops = {
	.open = process_reclaim_open,
	.read = seq_read,
	.write = proc_reclaim_write,
	.llseek = noop_llseek,
	.release = single_release,
};

static inline void process_mm_reclaim_init_procfs(void)
{
	umode_t file_mode;
	struct file_operations *fp;

	if (unlikely(AGING == get_eng_version())) {
		fp = &process_reclaim_rw_fops;
		file_mode = 0666;
	} else {
		fp = &process_reclaim_w_fops;
		file_mode = 0222;
	}

	if (!proc_create("process_reclaim", file_mode, NULL, fp))
		pr_err("Failed to register proc interface\n");
}
#else /* CONFIG_PROC_FS */
static inline void process_mm_reclaim_init_procfs(void)
{
}
#endif

static int __init process_reclaim_proc_init(void)
{
	process_mm_reclaim_init_procfs();
	return 0;
}

late_initcall(process_reclaim_proc_init);
MODULE_LICENSE("GPL");
