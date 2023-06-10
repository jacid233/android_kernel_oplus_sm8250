#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/kallsyms.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <linux/ksm.h>
///////////////////////////////////////////////////////////////////
#include <linux/slab.h> //kmalloc��kfree
#include <linux/device.h> //device_create�����豸�ļ���/dev/xxxxxxxx��
//////////////////////////////////////////////////////////////////
#include "cvector.h"
#include "proc.h"
#include "ver_control.h"

//////////////////////////////////////////////////////////////////

#define MAJOR_NUM 100
#define IOCTL_OPEN_PROCESS 							_IOR(MAJOR_NUM, 1, char*) //�򿪽���
#define IOCTL_CLOSE_HANDLE 							_IOR(MAJOR_NUM, 2, char*) //�رս���
#define IOCTL_GET_NUM_BRPS 							_IOR(MAJOR_NUM, 3, char*) //��ȡCPU֧��Ӳ��ִ�жϵ������
#define IOCTL_GET_NUM_WRPS 							_IOR(MAJOR_NUM, 4, char*) //��ȡCPU֧��Ӳ�����ʶϵ������
#define IOCTL_SET_HWBP_HIT_CONDITIONS		_IOR(MAJOR_NUM, 5, char*) //����Ӳ���ϵ����м�¼����
#define IOCTL_ADD_PROCESS_HWBP					_IOR(MAJOR_NUM, 6, char*) //���ý���Ӳ���ϵ�
#define IOCTL_DEL_PROCESS_HWBP					_IOR(MAJOR_NUM, 7, char*) //ɾ������Ӳ���ϵ�
#define IOCTL_GET_HWBP_HIT_ADDR_COUNT	_IOR(MAJOR_NUM, 8, char*) //��ȡӲ���ϵ����е�ַ����

//////////////////////////////////////////////////////////////////
static int g_hwBreakpointProc_major = 0; //��¼��̬��������豸��
static dev_t g_hwBreakpointProc_devno;

//hwBreakpointProcDev�豸�ṹ��
static struct hwBreakpointProcDev {
	struct cdev cdev; //cdev�ṹ��
};
static struct hwBreakpointProcDev *g_hwBreakpointProc_devp; //������cdev�豸�ṹ
static struct class *g_Class_devp; //�������豸��




//ȫ�ִ���Ӳ���ϵ�������
static cvector g_vHwBpHandle;


static struct HIT_INFO {
	size_t hit_addr; //���е�ַ
	size_t hit_count; //���д���
	struct pt_regs regs; //���һ�����еļĴ�������

};
static struct HW_BREAKPOINT_INFO {
	struct perf_event * sample_hbp; //Ӳ�Ͼ��
	cvector vHit; //������Ϣ����
};
//ȫ�ִ���Ӳ���ϵ�������Ϣ����
static cvector g_vHwBpInfo;
static struct semaphore g_lockHwBpInfoSem;




#pragma pack(1)
static struct HIT_CONDITIONS {
	char enable_regs[31];
	char enable_sp;
	char enable_pc;
	char enable_pstate;
	char enable_orig_x0;
	char enable_syscallno;
	uint64_t regs[31];
	uint64_t sp;
	uint64_t pc;
	uint64_t pstate;
	uint64_t orig_x0;
	uint64_t syscallno;
};
#pragma pack()
//ȫ��Ӳ���ϵ����м�¼����
static struct HIT_CONDITIONS g_hwBpHitConditions = { 0 };


static int hwBreakpointProc_open(struct inode *inode, struct file *filp) {
	//���豸�ṹ��ָ�븳ֵ���ļ�˽������ָ��
	filp->private_data = g_hwBreakpointProc_devp;
	return 0;
}

static ssize_t hwBreakpointProc_read(struct file* filp, char __user* buf, size_t size, loff_t* ppos) {
#pragma pack(1)
	struct my_user_pt_regs {
		uint64_t regs[31];
		uint64_t sp;
		uint64_t pc;
		uint64_t pstate;
		uint64_t orig_x0;
		uint64_t syscallno;
	};
	struct USER_HIT_INFO {
		size_t hit_addr; //���е�ַ
		size_t hit_count; //���д���
		struct my_user_pt_regs regs; //���һ�����еļĴ�������
	};
#pragma pack()
	/*
	����ͨ��Э�飺

		���룺
		0-7�ֽڣ�Ӳ���ϵ���

		�����
		sizeof(USER_HIT_INFO) * n
	*/

	//struct hwBreakpointProcDev* devp = filp->private_data; //����豸�ṹ��ָ��

	//��ȡӲ���ϵ���
	char tem[8];
	struct perf_event * sample_hbp = NULL;
	ssize_t count = 0;
	size_t copy_pos;
	size_t end_pos;

	citerator iter;

	memset(&tem, 0, sizeof(tem));
	//�û��ռ�->�ں˿ռ�
	if (copy_from_user(tem, buf, 8)) {
		return -EFAULT;
	}
	//��ȡӲ���ϵ���
	memcpy(&sample_hbp, tem, sizeof(sample_hbp));
	printk_debug(KERN_INFO "sample_hbp *:%ld\n", sample_hbp);

	copy_pos = (size_t)buf;
	end_pos = (size_t)((size_t)buf + size);

	for (iter = cvector_begin(g_vHwBpInfo); iter != cvector_end(g_vHwBpInfo); iter = cvector_next(g_vHwBpInfo, iter)) {
		struct HW_BREAKPOINT_INFO * hwbpInfo = (struct HW_BREAKPOINT_INFO *)iter;

		if (hwbpInfo->sample_hbp == sample_hbp) {
			citerator child;
			for (child = cvector_begin(hwbpInfo->vHit); child != cvector_end(hwbpInfo->vHit); child = cvector_next(hwbpInfo->vHit, child)) {
				struct HIT_INFO * hitInfo = (struct HIT_INFO *)child;
				struct USER_HIT_INFO userHitInfo;
				int r;

				if (copy_pos >= end_pos) {
					break;
				}

				userHitInfo.hit_addr = hitInfo->hit_addr;
				userHitInfo.hit_count = hitInfo->hit_count;

				for (r = 0; r < 31; r++) {
					userHitInfo.regs.regs[r] = hitInfo->regs.regs[r];
				}
				userHitInfo.regs.sp = hitInfo->regs.sp;
				userHitInfo.regs.pc = hitInfo->regs.pc;
				userHitInfo.regs.pstate = hitInfo->regs.pstate;
				userHitInfo.regs.orig_x0 = hitInfo->regs.orig_x0;
				userHitInfo.regs.syscallno = hitInfo->regs.syscallno;

				//�ں˿ռ�->�û��ռ佻������
				if (copy_to_user((void*)copy_pos, &userHitInfo, sizeof(userHitInfo))) {
					break;
				}
				copy_pos += sizeof(userHitInfo);

				count++;
			}

			break;
		}
	}

	return count;
}

static ssize_t hwBreakpointProc_write(struct file* filp, const char __user* buf, size_t size, loff_t *ppos) {
	citerator iter;
	for (iter = cvector_begin(g_vHwBpInfo); iter != cvector_end(g_vHwBpInfo); iter = cvector_next(g_vHwBpInfo, iter)) {
		struct HW_BREAKPOINT_INFO * hwbpInfo = (struct HW_BREAKPOINT_INFO *)iter;
		cvector_destroy(hwbpInfo->vHit);
	}
	cvector_destroy(g_vHwBpInfo);
	g_vHwBpInfo = cvector_create(sizeof(struct HW_BREAKPOINT_INFO));

	return 1;
}

static loff_t hwBreakpointProc_llseek(struct file* filp, loff_t offset, int orig) {
	loff_t ret = 0; //���ص�λ��ƫ��

	switch (orig) {
	case SEEK_SET: //����ļ���ʼλ��ƫ��
		if (offset < 0) //offset���Ϸ�
		{
			ret = -EINVAL; //��Ч��ָ��
			break;
		}

		filp->f_pos = offset; //�����ļ�ָ��λ��
		ret = filp->f_pos; //���ص�λ��ƫ��
		break;

	case SEEK_CUR: //����ļ���ǰλ��ƫ��

		if ((filp->f_pos + offset) < 0) //ָ�벻�Ϸ�
		{
			ret = -EINVAL;//��Ч��ָ��
			break;
		}

		filp->f_pos += offset;
		ret = filp->f_pos;
		break;

	default:
		ret = -EINVAL; //��Ч��ָ��
		break;
	}

	return ret;
}

/*
 * Handle hitting a HW-breakpoint.
 */
static void sample_hbp_handler(struct perf_event *bp,
	struct perf_sample_data *data,
	struct pt_regs *regs) {
	//printk_debug(KERN_INFO "hw_breakpoint HIT!!!!! %p %d\n", regs->pc, bp->id);
	size_t now_hit_addr = regs->pc;

	int exist_hbp = 0;
	citerator iter;
	int i = 0;


	//�ж��Ƿ������������ϵ�
	for (i = 0; i < 31; i++) {
		if (g_hwBpHitConditions.enable_regs[i]) {
			//�жϼĴ�����ֵ�Ƿ��������
			if (regs->regs[i] != g_hwBpHitConditions.regs[i]) {
				return;
			}
		}
	}
	if (g_hwBpHitConditions.enable_sp) {
		if (regs->sp != g_hwBpHitConditions.sp) {
			return;
		}
	}
	if (g_hwBpHitConditions.enable_pc) {
		if (regs->pc != g_hwBpHitConditions.pc) {
			return;
		}
	}
	if (g_hwBpHitConditions.enable_pstate) {
		if (regs->pstate != g_hwBpHitConditions.pstate) {
			return;
		}
	}
	if (g_hwBpHitConditions.enable_orig_x0) {
		if (regs->orig_x0 != g_hwBpHitConditions.orig_x0) {
			return;
		}
	}
	if (g_hwBpHitConditions.enable_syscallno) {
		if (regs->syscallno != g_hwBpHitConditions.syscallno) {
			return;
		}
	}


	//��ʼ��¼Ӳ���ϵ��������
	down(&g_lockHwBpInfoSem);
	for (iter = cvector_begin(g_vHwBpInfo); iter != cvector_end(g_vHwBpInfo); iter = cvector_next(g_vHwBpInfo, iter)) {
		struct HW_BREAKPOINT_INFO * hwbpInfo = (struct HW_BREAKPOINT_INFO *)iter;
		if (hwbpInfo->sample_hbp == bp) {
			exist_hbp = 1;
			int exist_hit = 0;
			citerator child;

			//printk_debug(KERN_INFO "hwbpInfo->sample_hbp==bp\n");
			for (child = cvector_begin(hwbpInfo->vHit); child != cvector_end(hwbpInfo->vHit); child = cvector_next(hwbpInfo->vHit, child)) {
				struct HIT_INFO * hitInfo = (struct HIT_INFO *)child;
				if (hitInfo->hit_addr == now_hit_addr) {
					exist_hit = 1;
					hitInfo->hit_count++;
					break;
				}
			}
			if (exist_hit == 0) {
				struct HIT_INFO hitInfo;
				memset(&hitInfo, 0, sizeof(hitInfo));
				hitInfo.hit_addr = now_hit_addr;
				memcpy(&hitInfo.regs, regs, sizeof(hitInfo.regs));
				cvector_pushback(hwbpInfo->vHit, &hitInfo);
			}
			break;
		}
	}

	if (exist_hbp == 0) {
		struct HW_BREAKPOINT_INFO hwbpInfo = { 0 };
		struct HIT_INFO hitInfo = { 0 };

		hwbpInfo.sample_hbp = bp;
		hwbpInfo.vHit = cvector_create(sizeof(struct HIT_INFO));


		hitInfo.hit_addr = now_hit_addr;
		memcpy(&hitInfo.regs, regs, sizeof(hitInfo.regs));
		//printk_debug(KERN_INFO "new hit_addr %p\n", now_hit_addr);
		cvector_pushback(hwbpInfo.vHit, &hitInfo);

		cvector_pushback(g_vHwBpInfo, &hwbpInfo);
	}
	up(&g_lockHwBpInfoSem);

}


//long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
//long (*compat_ioctl) (struct file *, unsigned int cmd, unsigned long arg)
static long hwBreakpointProc_ioctl(
	struct file *filp,
	unsigned int cmd,
	unsigned long arg) {
	//struct inode *inode = filp->f_dentry->d_inode;
	//struct inode *inode = filp->d_inode;
	struct inode *inode = inode = filp->f_inode;


	struct hwBreakpointProcDev* devp = filp->private_data; //����豸�ṹ��ָ��

	switch (cmd) {
	case IOCTL_OPEN_PROCESS: //�򿪽���
	{
		/*
		����ͨ��Э�飺

			���룺
			0-7�ֽڣ�����PID

			�����
			ioctl�ķ���ֵΪ0���ʾ�򿪽��̳ɹ�

			0-7�ֽڣ����̾��
		*/

		char buf[8];
		int64_t pid = 0;
		struct pid * proc_pid_struct = NULL;
		printk_debug(KERN_INFO "IOCTL_OPEN_PROCESS\n");

		memset(&buf, 0, sizeof(buf));
		//�û��ռ�->�ں˿ռ�
		if (copy_from_user((void*)buf, (void*)arg, 8)) {
			return -EINVAL;
		}
		//��ȡ����PID
		memcpy(&pid, buf, sizeof(pid));
		printk_debug(KERN_INFO "pid:%ld\n", pid);


		//������̾��
		proc_pid_struct = get_proc_pid_struct(pid);
		printk_debug(KERN_INFO "proc_pid_struct *:%ld\n", proc_pid_struct);

		if (!proc_pid_struct) {
			return -EINVAL;
		}

		memset(&buf, 0, sizeof(buf));
		memcpy(&buf, &proc_pid_struct, sizeof(proc_pid_struct));


		//�ں˿ռ�->�û��ռ佻������
		if (copy_to_user((void*)arg, &buf, 8)) {
			return -EINVAL;
		}

		return 0;
		break;
	}
	case IOCTL_CLOSE_HANDLE: //�رս���
	{
		/*
		����ͨ��Э�飺

			���룺
			0-7�ֽڣ����̾��

			�����
			ioctl�ķ���ֵΪ0���ʾ�رս��̳ɹ�
		*/

		//��ȡ���̾��
		char buf[8];
		struct pid * proc_pid_struct = NULL;

		printk_debug(KERN_INFO "IOCTL_CLOSE_HANDLE\n");

		memset(&buf, 0, sizeof(buf));
		//�û��ռ�->�ں˿ռ�
		if (copy_from_user((void*)buf, (void*)arg, 8)) {
			return -EFAULT;
		}
		//�رս��̾��
		memcpy(&proc_pid_struct, buf, sizeof(proc_pid_struct));
		printk_debug(KERN_INFO "proc_pid_struct *:%ld\n", proc_pid_struct);
		release_proc_pid_struct(proc_pid_struct);

		return 0;
		break;
	}
	case IOCTL_GET_NUM_BRPS: //��ȡCPU֧��Ӳ��ִ�жϵ������
	{
		/*
		����ͨ��Э�飺

			�����
			ioctl�ķ���ֵΪCPU֧��Ӳ��ִ�жϵ������

		*/
		printk_debug(KERN_INFO "IOCTL_GET_NUM_BRPS\n");

		return ((read_cpuid(ID_AA64DFR0_EL1) >> 12) & 0xf) + 1;
		break;
	}
	case IOCTL_GET_NUM_WRPS: //��ȡCPU֧��Ӳ�����ʶϵ������
	{
		/*
		����ͨ��Э�飺

			�����
			ioctl�ķ���ֵΪCPU֧��Ӳ�����ʶϵ������

		*/
		printk_debug(KERN_INFO "IOCTL_GET_NUM_WRPS\n");

		return  ((read_cpuid(ID_AA64DFR0_EL1) >> 20) & 0xf) + 1;
		break;
	}
	case IOCTL_SET_HWBP_HIT_CONDITIONS: //����Ӳ���ϵ����м�¼����
	{
		/*
		����ͨ��Э�飺

			���룺
			struct HIT_CONDITIONS

			�����

		*/

		//��ȡӲ���ϵ���
		char buf[sizeof(struct HIT_CONDITIONS)] = { 0 };

		printk_debug(KERN_INFO "IOCTL_SET_HWBP_HIT_CONDITIONS\n");

		//�û��ռ�->�ں˿ռ�
		if (copy_from_user((void*)buf, (void*)arg, sizeof(struct HIT_CONDITIONS))) {
			return -EFAULT;
		}
		memcpy(&g_hwBpHitConditions, &buf, sizeof(g_hwBpHitConditions));
		return 0;
		break;
	}
	case IOCTL_ADD_PROCESS_HWBP: //���ý���Ӳ���ϵ�
	{
		/*
		����ͨ��Э�飺

			���룺
			0-7�ֽڣ����̾��
			8-15�ֽڣ������ڴ��ַ
			16-23�ֽڣ�Ӳ���ϵ㳤��
			24-27�ֽڣ�Ӳ���ϵ�����

			�����
			ioctl�ķ���ֵΪ0���ʾ���ý���Ӳ���ϵ�ɹ�

			0-7�ֽڣ�Ӳ���ϵ���
		*/
		printk_debug(KERN_INFO "IOCTL_ADD_PROCESS_BP\n");

		//��ȡ���̾��
		char buf[28];
		struct pid * proc_pid_struct = NULL;
		size_t proc_virt_addr = 0;
		size_t hwBreakpoint_len = 4;
		unsigned int hwBreakpoint_type = 0;
		struct task_struct *task;
		struct perf_event_attr attr;
		struct perf_event * sample_hbp;

		memset(&buf, 0, sizeof(buf));
		//�û��ռ�->�ں˿ռ�
		if (copy_from_user((void*)buf, (void*)arg, 28)) {
			return -EFAULT;
		}
		//��ȡ���̾��
		memcpy(&proc_pid_struct, buf, sizeof(proc_pid_struct));
		printk_debug(KERN_INFO "proc_pid_struct *:%p\n", proc_pid_struct);

		//��ȡ���������ַ
		memcpy(&proc_virt_addr, (void*)((size_t)buf + (size_t)8), sizeof(proc_virt_addr));
		printk_debug(KERN_INFO "proc_virt_addr :%p\n", proc_virt_addr);


		//��ȡӲ���ϵ㳤��
		memcpy(&hwBreakpoint_len, (void*)((size_t)buf + (size_t)16), sizeof(hwBreakpoint_len));
		printk_debug(KERN_INFO "hwBreakpoint_len:%zu\n", hwBreakpoint_len);


		//��ȡӲ���ϵ�����
		memcpy(&hwBreakpoint_type, (void*)((size_t)buf + (size_t)24), sizeof(hwBreakpoint_type));
		printk_debug(KERN_INFO "hwBreakpoint_type:%d\n", hwBreakpoint_type);




		task = get_pid_task(proc_pid_struct, PIDTYPE_PID);
		if (!task) {
			printk_debug(KERN_INFO "get_pid_task failed.\n");
			return -EINVAL;
		}

		//hw_breakpoint_init(&attr);
		ptrace_breakpoint_init(&attr);


		/*
		 * Initialise fields to sane defaults
		 * (i.e. values that will pass validation).

		 */
		attr.bp_addr = proc_virt_addr;
		attr.bp_len = hwBreakpoint_len;
		attr.bp_type = hwBreakpoint_type;
		attr.disabled = 0;
		sample_hbp = register_user_hw_breakpoint(&attr, sample_hbp_handler, NULL, task);
		put_task_struct(task);

		if (IS_ERR((void __force *)sample_hbp)) {
			int ret = PTR_ERR((void __force *)sample_hbp);
			printk_debug(KERN_INFO "register_user_hw_breakpoint failed: %d\n", ret);
			return ret;
		}

		cvector_pushback(g_vHwBpHandle, &sample_hbp);


		//���Ӳ���ϵ���
		memset(&buf, 0, sizeof(buf));
		memcpy(&buf, &sample_hbp, sizeof(sample_hbp));
		//�ں˿ռ�->�û��ռ佻������
		if (copy_to_user((void*)arg, &buf, 8)) {
			return -EINVAL;
		}
		return 0;

		break;
	}
	case IOCTL_DEL_PROCESS_HWBP: //ɾ������Ӳ���ϵ�
	{
		/*
		����ͨ��Э�飺

			���룺
			0-7�ֽڣ�Ӳ���ϵ���

			�����
			ioctl�ķ���ֵΪ0���ʾɾ������Ӳ���ϵ�
		*/

		//��ȡӲ���ϵ���
		char buf[8];
		struct perf_event * sample_hbp = NULL;
		citerator iter;

		printk_debug(KERN_INFO "IOCTL_DEL_PROCESS_HWBP\n");

		memset(&buf, 0, sizeof(buf));
		//�û��ռ�->�ں˿ռ�
		if (copy_from_user((void*)buf, (void*)arg, 8)) {
			return -EFAULT;
		}
		//��ȡӲ���ϵ���
		memcpy(&sample_hbp, buf, sizeof(sample_hbp));
		printk_debug(KERN_INFO "sample_hbp *:%p\n", sample_hbp);

		//ɾ��Ӳ���ϵ�
		unregister_hw_breakpoint(sample_hbp);

		for (iter = cvector_begin(g_vHwBpHandle); iter != cvector_end(g_vHwBpHandle); iter = cvector_next(g_vHwBpHandle, iter)) {
			struct perf_event * sample_hbp = (struct perf_event *)*((size_t*)iter);
			cvector_rm(g_vHwBpHandle, iter);

			printk_debug(KERN_INFO "cvector_rm ok\n");
			break;
		}
		return 0;
		break;
	}
	case IOCTL_GET_HWBP_HIT_ADDR_COUNT: //��ȡӲ���ϵ����е�ַ����
	{
		/*
		����ͨ��Э�飺

			���룺
			0-7�ֽڣ�Ӳ���ϵ���

			�����
			ioctl�ķ���ֵΪӲ���ϵ����е�ַ����
		*/

		//��ȡӲ���ϵ���
		char buf[8];
		struct perf_event * sample_hbp = NULL;
		size_t count = 0;
		citerator iter;

		printk_debug(KERN_INFO "IOCTL_GET_HWBP_HIT_ADDR_COUNT\n");

		memset(&buf, 0, sizeof(buf));
		//�û��ռ�->�ں˿ռ�
		if (copy_from_user((void*)buf, (void*)arg, 8)) {
			return -EFAULT;
		}
		//��ȡӲ���ϵ���
		memcpy(&sample_hbp, buf, sizeof(sample_hbp));
		printk_debug(KERN_INFO "sample_hbp *:%p\n", sample_hbp);

		for (iter = cvector_begin(g_vHwBpInfo); iter != cvector_end(g_vHwBpInfo); iter = cvector_next(g_vHwBpInfo, iter)) {
			struct HW_BREAKPOINT_INFO * hwbpInfo = (struct HW_BREAKPOINT_INFO *)iter;

			if (hwbpInfo->sample_hbp == sample_hbp) {
				count = cvector_length(hwbpInfo->vHit);
				break;
			}
		}
		printk_debug(KERN_INFO "hit_count:%zu\n", count);
		return count;
		break;
	}
	default:
		return -EINVAL;
	}

	return 0;
}

static int hwBreakpointProc_release(struct inode *inode, struct file *filp) {
	return 0;
}



static const struct file_operations hwBreakpointProc_fops =
{
  .owner = THIS_MODULE,
  .open = hwBreakpointProc_open, //���豸����
  .release = hwBreakpointProc_release, //�ͷ��豸����
  .read = hwBreakpointProc_read, //���豸����
  .write = hwBreakpointProc_write, //д�豸����
  .llseek = hwBreakpointProc_llseek, //��λƫ��������

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
	.ioctl = hwBreakpointProc_ioctl, //���ƺ���
#else
	.compat_ioctl = hwBreakpointProc_ioctl, //64λ��������ʵ������ӿڣ�32λ������ܵ�������
	.unlocked_ioctl = hwBreakpointProc_ioctl, //���ƺ���
#endif
};

static int __init hwBreakpointProc_dev_init(void) {
	int result;
	int err;

	//��̬�����豸��
	result = alloc_chrdev_region(&g_hwBreakpointProc_devno, 0, 1, DEV_FILENAME);
	g_hwBreakpointProc_major = MAJOR(g_hwBreakpointProc_devno);


	if (result < 0) {
		printk(KERN_EMERG "hwBreakpointProc alloc_chrdev_region failed %d\n", result);
		return result;
	}

	// 2.��̬�����豸�ṹ����ڴ�
	g_hwBreakpointProc_devp = kmalloc(sizeof(struct hwBreakpointProcDev), GFP_KERNEL);
	if (!g_hwBreakpointProc_devp) {
		//����ʧ��
		result = -ENOMEM;
		goto _fail;
	}
	memset(g_hwBreakpointProc_devp, 0, sizeof(struct hwBreakpointProcDev));

	//3.��ʼ���������cdev�ṹ��
	cdev_init(&g_hwBreakpointProc_devp->cdev, &hwBreakpointProc_fops); //��ʼ��cdev�豸
	g_hwBreakpointProc_devp->cdev.owner = THIS_MODULE; //ʹ�����������ڸ�ģ��
	g_hwBreakpointProc_devp->cdev.ops = &hwBreakpointProc_fops; //cdev����file_operationsָ��
	//��cdevע�ᵽϵͳ��
	err = cdev_add(&g_hwBreakpointProc_devp->cdev, g_hwBreakpointProc_devno, 1);
	if (err) {
		printk(KERN_NOTICE "Error in cdev_add()\n");
		result = -EFAULT;
		goto _fail;
	}

	//��ʼ���ź���
	sema_init(&g_lockHwBpInfoSem, 1);


	//4.�����豸�ļ���λ����/dev/xxxxx��
	g_Class_devp = class_create(THIS_MODULE, DEV_FILENAME); //�����豸��
	device_create(g_Class_devp, NULL, g_hwBreakpointProc_devno, NULL, "%s", DEV_FILENAME); //�����豸�ļ�


	//����ȫ�ִ���Ӳ���ϵ���������
	g_vHwBpHandle = cvector_create(sizeof(struct perf_event *));
	g_vHwBpInfo = cvector_create(sizeof(struct HW_BREAKPOINT_INFO));


#ifdef DEBUG_PRINTK
	printk(KERN_EMERG "Hello, %s debug\n", DEV_FILENAME);
	//test1();
	//test2();
	//test3();
	//test4();
#else
	printk(KERN_EMERG "Hello, %s\n", DEV_FILENAME);
#endif


	return 0;

_fail:
	unregister_chrdev_region(g_hwBreakpointProc_devno, 1);
	return result;
}

static void __exit hwBreakpointProc_dev_exit(void) {
	//ɾ��ʣ���Ӳ���ϵ�
	citerator iter;
	for (iter = cvector_begin(g_vHwBpHandle); iter != cvector_end(g_vHwBpHandle); iter = cvector_next(g_vHwBpHandle, iter)) {
		struct perf_event * sample_hbp = (struct perf_event *)*((size_t*)iter);
		unregister_hw_breakpoint(sample_hbp);
	}
	cvector_destroy(g_vHwBpHandle);

	for (iter = cvector_begin(g_vHwBpInfo); iter != cvector_end(g_vHwBpInfo); iter = cvector_next(g_vHwBpInfo, iter)) {
		struct HW_BREAKPOINT_INFO * hwbpInfo = (struct HW_BREAKPOINT_INFO *)iter;
		cvector_destroy(hwbpInfo->vHit);
	}
	cvector_destroy(g_vHwBpInfo);


	device_destroy(g_Class_devp, g_hwBreakpointProc_devno); //ɾ���豸�ļ���λ����/dev/xxxxx��
	class_destroy(g_Class_devp); //ɾ���豸��

	cdev_del(&g_hwBreakpointProc_devp->cdev); //ע��cdev
	kfree(g_hwBreakpointProc_devp); // �ͷ��豸�ṹ���ڴ�
	unregister_chrdev_region(g_hwBreakpointProc_devno, 1); //�ͷ��豸��


	printk(KERN_EMERG "Goodbye, %s\n", DEV_FILENAME);

}


#ifdef CONFIG_MODULE_GUIDE_ENTRY
module_init(hwBreakpointProc_dev_init);
module_exit(hwBreakpointProc_dev_exit);
#endif

MODULE_AUTHOR("Linux");
MODULE_DESCRIPTION("Linux default module");
MODULE_LICENSE("GPL");
