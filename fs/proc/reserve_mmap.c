/*************************************************************************
 * File Name: fs/proc/reserve_mmap.c
 * Author: zhangkui
 * Mail: zhangkui@oppo.com
 * Created Time: Tue 05 Nov 2019 10:29:20 PM CST
 ************************************************************************/

/* Kui.Zhang@PSW.TEC.KERNEL.Performance, 2019/03/18,
 * show the task's reserved area info
 */
int proc_pid_reserve_area(struct seq_file *m, struct pid_namespace *ns,
			struct pid *pid, struct task_struct *task)
{
	struct mm_struct *mm = get_task_mm(task);
	struct vm_area_struct *resrve_vma;

	if (mm) {
		resrve_vma = mm->reserve_vma;
		if (resrve_vma)
			seq_printf(m, "%#lx\t%#lx\t%d\n",
					resrve_vma->vm_start,
					resrve_vma->vm_end - resrve_vma->vm_start,
					mm->reserve_map_count);
		else
			seq_printf(m, "%#lx\t%#lx\t%d\n", 0, 0, 0);

		mmput(mm);
	}
	return 0;

}
EXPORT_SYMBOL(proc_pid_reserve_area);

/* Kui.Zhang@PSW.TEC.KERNEL.Performance, 2019/03/18,
 * interfaces for reading the reserved mmaps
 */
static void *reserve_vma_m_start(struct seq_file *m, loff_t *ppos)
{
	struct proc_maps_private *priv = m->private;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	unsigned int pos = *ppos;

	priv->task = get_proc_task(priv->inode);
	if (!priv->task)
		return ERR_PTR(-ESRCH);

	mm = priv->mm;
	if (!mm || !atomic_inc_not_zero(&mm->mm_users))
		return NULL;

	down_read(&mm->mmap_sem);
	hold_task_mempolicy(priv);

	m->version = 0;
	if (pos < mm->reserve_map_count) {
		for (vma = mm->reserve_mmap; pos; pos--)
			vma = vma->vm_next;
		return vma;
	}

	vma_stop(priv);
	return NULL;
}

static void *reserve_vma_m_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct proc_maps_private *priv = m->private;
	struct vm_area_struct *next;
	struct vm_area_struct *area = (struct vm_area_struct *)v;

	(*pos)++;

	next = area->vm_next;
	if (next == NULL)
		vma_stop(priv);

	return next;
}

/* Kui.Zhang@PSW.TEC.KERNEL.Performance, 2019/03/18,
 * interfaces for reading the reserved mmaps
 */
static const struct seq_operations proc_pid_rmaps_op = {
	.start	= reserve_vma_m_start,
	.next	= reserve_vma_m_next,
	.stop	= m_stop,
	.show	= show_map
};

/* Kui.Zhang@PSW.TEC.KERNEL.Performance, 2019/03/18,
 * interface for reading the reserved mmaps
 */
static int pid_rmaps_open(struct inode *inode, struct file *file)
{
	return do_maps_open(inode, file, &proc_pid_rmaps_op);
}

/* Kui.Zhang@PSW.TEC.KERNEL.Performance, 2019/03/18,
 * interfaces for reading the reserved mmaps
 */
const struct file_operations proc_pid_rmaps_operations = {
	.open		= pid_rmaps_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= proc_map_release,
};
EXPORT_SYMBOL(proc_pid_rmaps_operations);
