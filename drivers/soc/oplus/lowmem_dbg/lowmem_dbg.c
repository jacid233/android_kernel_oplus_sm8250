/********************************************************************************
 ** Copyright (C), 2018-2020, OPPO Mobile Comm Corp., Ltd
 ** All rights reserved.
 **
 ** File: - lowmme_dbg.c
 ** Description:
 **     provide memory usage stat when device in low memory.
 **
 ** Version: 1.0
 ** Date: 2020-03-18
 ** Author: Hailong.Liu@BSP.Kernel.MM
 ** ------------------------------- Revision History: ----------------------------
 ** <author>                        <data>       <version>   <desc>
 ** ------------------------------------------------------------------------------
 ** Hailong.Liu@BSP.Kernel.MM       2020-06-17   1.0         Create this module
 ********************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/rcupdate.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/swap.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/ksm.h>
#include <linux/version.h>
#include <linux/dma-buf.h>
#include <linux/fdtable.h>
#include <soc/oplus/lowmem_dbg.h>
#include <linux/slub_def.h>

#include "../../../../mm/slab.h"

#ifdef CONFIG_MTK_ION
#include "ion_priv.h"
#endif /* CONFIG_MTK_ION */

#ifdef OPLUS_FEATURE_HEALTHINFO
/* Huacai.Zhou@PSW.BSP.Kernel.MM, 2018-06-26, add ion total used account*/
#include <linux/oppo_healthinfo/oppo_ion.h>
#endif /* OPLUS_FEATURE_HEALTHINFO */

static u64 last_jiffies;
static int lowmem_ram[] = {
	0,
	768 * 1024,	/* 3GB */
	1024 * 1024,	/* 4GB */
};

static int lowmem_watermark_low[] = {
	64 * 1024,	/* 256MB */
	128 * 1024,	/* 512MB */
	256 * 1024,	/* 1024MB */
};

struct lowmem_dbg_cfg {
	short service_adj;
	unsigned int dump_interval;
	u64 last_jiffies;
	u64 watermark_task_rss;
	/* all watermark use page meters */
	u64 watermark_ion;
	u64 watermark_slab;
	u64 watermark_anon;
	u64 watermark_low;
	u64 watermark_gpu;
	u64 watermark_critical;
};

static void lowmem_dump(struct work_struct *work);

static DEFINE_MUTEX(lowmem_dump_mutex);
static DECLARE_WORK(lowmem_dbg_work, lowmem_dump);
static DECLARE_WORK(lowmem_dbg_verbose_work, lowmem_dump);

static bool cmp_critical(void);

static bool cmp_anon(void);
static void dump_tasks_info(bool verbose);

static bool cmp_ion(void);
#ifdef CONFIG_MTK_ION
static void dump_ion_clients(bool verbose);
#else /* CONFIG_MTK_ION */
static void dump_dma_buf(bool verbose);
#endif /* CONFIG_MTK_ION */

#if defined(CONFIG_SLUB_DEBUG) || defined(CONFIG_SLUB_STAT_DEBUG)
static bool cmp_slab(void);
static void dump_slab(bool verbose);
#endif /* CONFIG_SLUB_DEBUG || CONFIG_SLUB_STAT_DEBUG */
#ifdef CONFIG_MTK_GPU_SUPPORT
static bool cmp_gpu(void);
static void dump_gpu(bool verbose);
#endif /* CONFIG_MTK_GPU_SUPPORT */
enum {
	MEM_SLAB,
	MEM_ANON,
	MEM_ION,
	MEM_GPU,
};

struct lowmem_dump_cfg {
	int type;
	char *name;
	void (*dump)(bool);
	bool (*cmp)(void);
};

static void dump_tasks_info(bool verbose);

static struct lowmem_dbg_cfg dbg_cfg;

static bool cmp_critical(void)
{
	struct lowmem_dbg_cfg *pcfg = &dbg_cfg;
	int other_file = global_node_page_state(NR_FILE_PAGES) -
		global_node_page_state(NR_SHMEM) -
		global_node_page_state(NR_UNEVICTABLE) -
		total_swapcache_pages();
	int other_free = global_zone_page_state(NR_FREE_PAGES) - totalreserve_pages;
	return other_free <= pcfg->watermark_critical &&
		other_file <= pcfg->watermark_critical;
}

static struct lowmem_dump_cfg dumpcfgs[] = {
	{
		MEM_ANON, "anon", dump_tasks_info, cmp_anon
	},

#if defined(CONFIG_SLUB_DEBUG) || defined(CONFIG_SLUB_STAT_DEBUG)
	{
		MEM_SLAB, "slab", dump_slab, cmp_slab
	},
#endif /* CONFIG_SLUB_DEBUG */

#ifdef CONFIG_MTK_ION
	{
		MEM_ION, "mtk-ion", dump_ion_clients, cmp_ion
	},
#else /* CONFIG_MTK_ION */
	{
		MEM_ION, "ion", dump_dma_buf, cmp_ion
	},
#endif /* CONFIG_MTK_ION */

#ifdef CONFIG_MTK_GPU_SUPPORT
	{
		MEM_GPU, "mtk-gpu", dump_gpu, cmp_gpu
	},
#endif /* CONFIG_MTK_GPU_SUPPORT */
};

static bool cmp_anon(void)
{
	struct lowmem_dbg_cfg *pcfg = &dbg_cfg;
	return global_node_page_state(NR_ACTIVE_ANON) +
		global_node_page_state(NR_INACTIVE_ANON) >= pcfg->watermark_anon;
}

static void dump_tasks_info(bool verbose)
{
	struct task_struct *p;
	struct task_struct *tsk;
	short tsk_oom_adj = 0;
	unsigned long tsk_nr_ptes = 0;
	char task_state = 0;
	char frozen_mark = ' ';
	struct lowmem_dbg_cfg *pcfg = &dbg_cfg;

	pr_info("[ pid ]   uid  tgid total_vm      rss    nptes     swap    sheme   adj s name\n");

	rcu_read_lock();
	for_each_process(p) {
		tsk = find_lock_task_mm(p);
		if (!tsk) {
			/*
			 * This is a kthread or all of p's threads have already
			 * detached their mm's.  There's no need to report
			 * them; they can't be oom killed anyway.
			 */
			continue;
		}

		tsk_oom_adj = tsk->signal->oom_score_adj;

		if (!verbose && tsk_oom_adj &&
		    (tsk_oom_adj <= pcfg->service_adj) &&
		    (get_mm_rss(tsk->mm) +
		     get_mm_counter(tsk->mm, MM_SWAPENTS)) < pcfg->watermark_task_rss) {
			task_unlock(tsk);
			continue;
		}

		/* tsk_nr_ptes = PTRS_PER_PTE * sizeof(pte_t) * atomic_long_read(&tsk->mm->nr_ptes); */
		/* consolidate page table accounting */
		tsk_nr_ptes = mm_pgtables_bytes(tsk->mm);
		task_state = task_state_to_char(tsk);
		/* check whether we have freezed a task. */
		frozen_mark = frozen(tsk) ? '*' : ' ';

		pr_info("[%5d] %5d %5d %8lu %8lu %8lu %8lu %8lu %5hd %c %s%c\n",
			tsk->pid,
			from_kuid(&init_user_ns, task_uid(tsk)),
			tsk->tgid, tsk->mm->total_vm,
			get_mm_rss(tsk->mm),
			tsk_nr_ptes / SZ_1K,
			get_mm_counter(tsk->mm, MM_SWAPENTS),
			get_mm_counter(tsk->mm, MM_SHMEMPAGES),
			tsk_oom_adj,
			task_state,
			tsk->comm,
			frozen_mark);
		task_unlock(tsk);
	}
	rcu_read_unlock();
}

static bool cmp_ion(void)
{
#ifdef OPLUS_FEATURE_HEALTHINFO
	struct lowmem_dbg_cfg *pcfg = &dbg_cfg;
	return (ion_total() >> PAGE_SHIFT) >= pcfg->watermark_ion;
#else /* OPLUS_FEATURE_HEALTHINFO */
	return false;
#endif /* OPLUS_FEATURE_HEALTHINFO */
}

#ifdef CONFIG_MTK_ION
extern struct ion_device *g_ion_device;
static void  dump_ion_clients(bool verbose)
{
	struct ion_device *dev = g_ion_device;
	struct rb_node *n, *m;
	unsigned int buffer_size = 0;
	size_t total_orphaned_size = 0;
	size_t total_size = 0;

	if (!down_read_trylock(&dev->lock)) {
		return;
	}
	for (n = rb_first(&dev->clients); n; n = rb_next(n)) {
		struct ion_client *client =
			rb_entry(n, struct ion_client, node);
		char task_comm[TASK_COMM_LEN];

		if (client->task) {
			get_task_comm(task_comm, client->task);
		}

		mutex_lock(&client->lock);
		for (m = rb_first(&client->handles); m; m = rb_next(m)) {
			struct ion_handle *handle = rb_entry(m, struct ion_handle,
							     node);
			buffer_size += (unsigned int)(handle->buffer->size);
		}
		if (!buffer_size) {
			mutex_unlock(&client->lock);
			continue;
		}
		pr_info("[%-5d] %-8d %-16s %-16s\n",
			client->pid,
			buffer_size / SZ_1K,
			client->task ? task_comm : "from_kernel",
			(*client->dbg_name) ? client->dbg_name : client->name);
		buffer_size = 0;
		mutex_unlock(&client->lock);
	}

	pr_info("orphaned allocation (info is from last known client):\n");
	mutex_lock(&dev->buffer_lock);
	for (n = rb_first(&dev->buffers); n; n = rb_next(n)) {
		struct ion_buffer *buffer = rb_entry(n, struct ion_buffer,
						     node);
		total_size += buffer->size;
		if (!buffer->handle_count) {
			pr_info("[%-5d] %-8d %-16s 0x%p %d %d\n",
				buffer->pid,
				buffer->size / SZ_1K,
				buffer->task_comm,
				buffer,
				buffer->kmap_cnt,
				atomic_read(&buffer->ref.refcount.refs));
			total_orphaned_size += buffer->size;
		}
	}
	mutex_unlock(&dev->buffer_lock);
	pr_info("orphaned: %zu total: %zu\n",
		total_orphaned_size, total_size);

	up_read(&dev->lock);
}
#else /* CONFIG_MTK_ION */
struct dma_info {
	struct task_struct *tsk;
	bool verbose;
	size_t sz;
};

static int acct_dma_dize(const void *data, struct file *file,
			 unsigned int n)
{
	struct dma_info *dmainfo = (struct dma_info *)data;
	struct dma_buf *dbuf;

	if (!oppo_is_dma_buf_file(file)) {
		return 0;
	}

	dbuf = file->private_data;
	if (dbuf->size && dmainfo->verbose) {
		pr_info("%-16s %-8s %-8ld kB\n", dmainfo->tsk->comm,
			dbuf->buf_name,
			dbuf->size / SZ_1K);
	}

	dmainfo->sz += dbuf->size;
	return 0;
}

static void dump_dma_buf(bool verbose)
{
	struct task_struct *task, *thread;
	struct files_struct *files;
	int ret = 0;

	rcu_read_lock();
	for_each_process(task) {
		struct files_struct *group_leader_files = NULL;
		struct dma_info dmainfo = {
			.verbose = verbose,
			.sz = 0,
		};

		for_each_thread(task, thread) {
			task_lock(thread);
			if (unlikely(!group_leader_files)) {
				group_leader_files = task->group_leader->files;
			}

			files = thread->files;
			if (files && (group_leader_files != files ||
				      thread == task->group_leader)) {
				dmainfo.tsk = thread;
				ret = iterate_fd(files, 0, acct_dma_dize,
						 &dmainfo);
			}
			task_unlock(thread);
		}

		if (ret || !dmainfo.sz) {
			continue;
		}

		pr_info("[%5d] %8lu kB %-16s\n",
			task->pid, dmainfo.sz / SZ_1K, task->comm);
	}
	rcu_read_unlock();
}
#endif /* CONFIG_MTK_ION */

#if defined(CONFIG_SLUB_DEBUG) || defined(CONFIG_SLUB_STAT_DEBUG)
static bool cmp_slab(void)
{
	struct lowmem_dbg_cfg *pcfg = &dbg_cfg;
	return global_node_page_state(NR_SLAB_UNRECLAIMABLE) >= pcfg->watermark_slab;
}

static void dump_slab(bool verbose)
{
	if (likely(!verbose)) {
		unsigned long slab_pages = 0;
		struct kmem_cache *cachep = NULL;
		struct kmem_cache *max_cachep = NULL;
		struct kmem_cache *prev_max_cachep = NULL;

		mutex_lock(&slab_mutex);
		list_for_each_entry(cachep, &slab_caches, list) {
			struct slabinfo sinfo;
			unsigned long scratch;

			memset(&sinfo, 0, sizeof(sinfo));
			get_slabinfo(cachep, &sinfo);
			scratch = sinfo.num_slabs << sinfo.cache_order;

			if (slab_pages < scratch) {
				slab_pages = scratch;
				prev_max_cachep = max_cachep;
				max_cachep = cachep;
			}
		}

		if (max_cachep || prev_max_cachep) {
			pr_info("name            <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab> : tunables <limit> <batchcount> <sharedfactor> : slabdata <active_slabs> <num_slabs> <sharedavail>");
		}

		if (max_cachep) {
			struct slabinfo sinfo;

			memset(&sinfo, 0, sizeof(sinfo));
			/* TODO maybe we can cache slabinfo to achieve
			 * better performance */
			get_slabinfo(max_cachep, &sinfo);

			pr_info("%-17s %6lu %6lu %6u %4u %4d : tunables %4u %4u %4u : slabdata %6lu %6lu %6lu\n",
				max_cachep->name, sinfo.active_objs,
				sinfo.num_objs, max_cachep->size,
				sinfo.objects_per_slab,
				(1 << sinfo.cache_order),
				sinfo.limit, sinfo.batchcount, sinfo.shared,
				sinfo.active_slabs, sinfo.num_slabs,
				sinfo.shared_avail);
		}

		if (prev_max_cachep) {
			struct slabinfo sinfo;

			memset(&sinfo, 0, sizeof(sinfo));
			/* TODO maybe we can cache slabinfo to achieve
			 * better performance */
			get_slabinfo(prev_max_cachep, &sinfo);

			pr_info("%-17s %6lu %6lu %6u %4u %4d : tunables %4u %4u %4u : slabdata %6lu %6lu %6lu\n",
				prev_max_cachep->name, sinfo.active_objs,
				sinfo.num_objs, prev_max_cachep->size,
				sinfo.objects_per_slab,
				(1 << sinfo.cache_order),
				sinfo.limit, sinfo.batchcount, sinfo.shared,
				sinfo.active_slabs, sinfo.num_slabs,
				sinfo.shared_avail);
		}
		mutex_unlock(&slab_mutex);

	} else {
		struct kmem_cache *cachep = NULL;

		pr_info("# name            <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab> : tunables <limit> <batchcount> <sharedfactor> : slabdata <active_slabs> <num_slabs> <sharedavail>");

		mutex_lock(&slab_mutex);
		list_for_each_entry(cachep, &slab_caches, list) {
			struct slabinfo sinfo;

			memset(&sinfo, 0, sizeof(sinfo));
			get_slabinfo(cachep, &sinfo);

			pr_info("%-17s %6lu %6lu %6u %4u %4d : tunables %4u %4u %4u : slabdata %6lu %6lu %6lu\n",
				cachep->name, sinfo.active_objs,
				sinfo.num_objs, cachep->size,
				sinfo.objects_per_slab,
				(1 << sinfo.cache_order),
				sinfo.limit, sinfo.batchcount, sinfo.shared,
				sinfo.active_slabs, sinfo.num_slabs,
				sinfo.shared_avail);
		}
		mutex_unlock(&slab_mutex);
	}
}
#endif /* CONFIG_SLUB_DEBUG && CONFIG_SLUB_STAT_DEBUG */

#if IS_ENABLED(CONFIG_MTK_GPU_SUPPORT)
extern void mtk_dump_gpu_memory_usage(void) __attribute__((weak));
extern bool mtk_get_gpu_memory_usage(unsigned int *pMemUsage);
static bool cmp_gpu(void)
{
	unsigned int gpuuse = 0;
	if (mtk_get_gpu_memory_usage(&gpuuse)) {
		pr_info("gpu_usage: %d kB", gpuuse / SZ_1K);
	}
	/* always return false cause of mtk gpu info don't neeed merge */
	return false;
}

static void dump_gpu(bool verbose)
{
	mtk_dump_gpu_memory_usage();
}
#endif /* CONFIG_MTK_GPU_SUPPORT */

static void lowmem_dump(struct work_struct *work)
{
	bool critical = (work == &lowmem_dbg_verbose_work);
	int i = 0;

	mutex_lock(&lowmem_dump_mutex);
	pr_info("====> lowmem_dbg start critical: %d\n", critical);
	show_mem(SHOW_MEM_FILTER_NODES, NULL);

	for (i = 0; i < ARRAY_SIZE(dumpcfgs); i++) {
		struct lowmem_dump_cfg *dcfg = dumpcfgs + i;
		char *name = dcfg->name;

		if (!dcfg->dump) {
			continue;
		}
		if (name) {
			pr_info("====> dump %s start\n", name);
		}
		if (critical) {
			dcfg->dump(true);
		} else {
			dcfg->dump(dcfg->cmp ? dcfg->cmp() : false);
		}
		if (name) {
			pr_info("====> dump %s end\n", name);
		}
	}
	pr_info("====> lowmem_dbg end\n");
	mutex_unlock(&lowmem_dump_mutex);
}

static void dump_lowmem_dbg_cfg() {
	struct lowmem_dbg_cfg *pcfg = &dbg_cfg;
	pr_info("lowmem_dbg cfg service_adj %d dump_interval %u\n",
		pcfg->service_adj,
		pcfg->dump_interval);
	pr_info("lowmem_dbg cfg watermark(PAGE) task_rss %lu, ion %lu, gpu %lu slab %lu anon %lu low %lu, critical %lu",
		pcfg->watermark_task_rss,
		pcfg->watermark_ion,
		pcfg->watermark_gpu,
		pcfg->watermark_slab,
		pcfg->watermark_anon,
		pcfg->watermark_low,
		pcfg->watermark_critical);
}

void oppo_lowmem_dbg()
{
	u64 now = get_jiffies_64();
	struct lowmem_dbg_cfg *pcfg = &dbg_cfg;

	if (time_before64(now, (last_jiffies + pcfg->dump_interval))) {
		return;
	}

	last_jiffies = now;
	if (cmp_critical()) {
		schedule_work(&lowmem_dbg_verbose_work);
	} else {
		schedule_work(&lowmem_dbg_work);
	}
}

static unsigned long lowmem_count(struct shrinker *s,
				  struct shrink_control *sc)
{
	return global_node_page_state(NR_ACTIVE_ANON) +
		global_node_page_state(NR_ACTIVE_FILE) +
		global_node_page_state(NR_INACTIVE_ANON) +
		global_node_page_state(NR_INACTIVE_FILE);
}

static unsigned long lowmem_scan(struct shrinker *s, struct shrink_control *sc)
{
	static atomic_t atomic_lmk = ATOMIC_INIT(0);
	struct lowmem_dbg_cfg *pcfg = &dbg_cfg;

	if (si_mem_available() > pcfg->watermark_low) {
		return 0;
	}

	if (atomic_inc_return(&atomic_lmk) > 1) {
		atomic_dec(&atomic_lmk);
		return 0;
	}

	oppo_lowmem_dbg();
	atomic_dec(&atomic_lmk);
	return 0;
}

static struct shrinker lowmem_shrinker = {
	.scan_objects = lowmem_scan,
	.count_objects = lowmem_count,
	.seeks = DEFAULT_SEEKS
};

static __init int oppo_lowmem_dbg_init(void)
{
	int ret;
	struct lowmem_dbg_cfg *pcfg = &dbg_cfg;
	int array_size = ARRAY_SIZE(lowmem_ram), i;

	/* This is a process holding an application service */
	pcfg->service_adj = 500;
	pcfg->dump_interval = 15 * HZ;

	/* init watermark */
	pcfg->watermark_task_rss = SZ_256M >> PAGE_SHIFT;
	pcfg->watermark_ion = SZ_2G >> PAGE_SHIFT;
	pcfg->watermark_anon = totalram_pages / 3;
	pcfg->watermark_slab = SZ_1G >> PAGE_SHIFT;
	pcfg->watermark_critical = SZ_128M >> PAGE_SHIFT;
	pcfg->watermark_gpu = SZ_1G >> PAGE_SHIFT;
	pcfg->watermark_low = lowmem_watermark_low[0];
	for (i = array_size - 1; i >= 0; i--) {
		if (totalram_pages >= lowmem_ram[i]) {
			pcfg->watermark_low = lowmem_watermark_low[i];
			break;
		}
	}

	ret = register_shrinker(&lowmem_shrinker);
	dump_lowmem_dbg_cfg();
	return 0;
}
device_initcall(oppo_lowmem_dbg_init);
