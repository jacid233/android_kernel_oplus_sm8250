#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/timekeeping.h>
#include <linux/rtc.h>
#include "input-compat.h"
#include "soc/oppo/boot_mode.h"
#include <linux/uaccess.h>
#include <linux/slab.h>

//we should kmalloc&free this memory
char sau_pwkstp[4096][64]={{0}};
//static int sau_pwrkdump_init = 0;
//static loff_t sau_offset = 0;
int input_thread_ready = false;
bool is_silent_reboot_or_sau(void) {
    int boot_mode = get_boot_mode();
    return  MSM_BOOT_MODE__SILENCE == boot_mode || MSM_BOOT_MODE__SAU == boot_mode;
}


void oppo_sync_saupwk_event(unsigned int type, unsigned int code, int value)
{
//we bypass this logic to avoid any probable risk of mem boundary oversteped
/*
    struct timespec ts;
    struct rtc_time tm;

    if (type != EV_KEY)
	return;

    if((is_silent_reboot_or_sau()) && ((!input_thread_ready)) && code == KEY_POWER){
	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	snprintf(sau_pwkstp[sau_offset], sizeof(sau_pwkstp[sau_offset]),
		 "<%d,%d>@%d-%02d-%02d %02d:%02d:%02d.%03lu KER\n",
		 code, value, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		 tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec/1000000);
	pr_warn("[SAUPWK]:%s, is_silent_reboot_or_sau:%d, input_thread_ready:%d\n",
		sau_pwkstp[sau_offset], is_silent_reboot_or_sau(), input_thread_ready);
	if(sau_offset < 4095){
	    sau_offset += 1;
	    pr_warn("[SAUPWK]:num of sau_powerkey items:%d\n",(int)sau_offset);
	} else {
	    sau_offset += 0;
	    pr_warn("[SAUPWK]:we reach the end of sau_buffer. powerkey logs might missing\n");
	}
    }
*/
}


extern char sau_pwkstp[4096][64];
static int sau_pwkstp_seq_show(struct seq_file *seq, void*offset)
{
    int i = 0;
    int cnt = 0;
    int count = 0;

    for (i = 0; i < 4096; i++){
	//cnt = sprintf(buf+count, "%s", sau_pwkstp[i]);
	cnt = strlen(sau_pwkstp[i]);
	seq_printf(seq, "%s", sau_pwkstp[i]);
	printk("<0>[SAUPWK]:%s", sau_pwkstp[i]);
	count += cnt;
    }
    printk("<0>[SAUPWK]:count:%d\n",count);
    return 0;
}

extern void* PDE_DATA(const struct inode*inode);
static int sau_pwkstp_open(struct inode *inode, struct file* file)
{
    return single_open(file, sau_pwkstp_seq_show, PDE_DATA(inode));
}


static const struct file_operations oppo_sau_pwkstp_fops = {
    .owner   = THIS_MODULE,
    .open    = sau_pwkstp_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .release = single_release,
};


static ssize_t
sau_inputthreadready_write(struct file * file, const char __user * buffer,
			   size_t count, loff_t* f_pos)
{
    char * str_thrd_ready = kzalloc((count+1), GFP_KERNEL);
    if(!str_thrd_ready)
	return -ENOMEM;
    if(copy_from_user(str_thrd_ready, buffer, count)){
	kfree(str_thrd_ready);
	return EFAULT;
    }

    printk("<0>[SAUPWK]: setting input_thread_ready=%s\n", str_thrd_ready);
    if('0' == str_thrd_ready[0]){
	input_thread_ready = false;
    } else {
	input_thread_ready = true;
    }

    kfree(str_thrd_ready);
    return count;
}


static int sau_inputthreadready_seq_show(struct seq_file *seq, void*offset)
{
    seq_printf(seq, "%d\n", input_thread_ready);
    return 0;
}

static int sau_inputthreadready_open(struct inode *inode, struct file* file)
{
    return single_open(file, sau_inputthreadready_seq_show, PDE_DATA(inode));
}

static const struct file_operations oppo_sau_inputthreadready_fops = {
    .owner   = THIS_MODULE,
    .open    = sau_inputthreadready_open,
    .read    = seq_read,
    .llseek  = seq_lseek,
    .write   = sau_inputthreadready_write,
    .release = single_release,
};

static int __init
oppo_saupwk_during_init(void)
{
    struct proc_dir_entry * pentry;

    pentry = proc_create("sau_pwkstp", 0, NULL, &oppo_sau_pwkstp_fops);
    if(!pentry){
	pr_err("sau_pwkstp proc entory create failure.\n");
	return -ENOENT;
    }
    pr_warn("[SAUPWK]:sau_pwkstp proc entry create success.\n");

    pentry = proc_create("sau_inputthreadready", 0666, NULL, &oppo_sau_inputthreadready_fops);
    if(!pentry){
	pr_err("sau_inputthreadready proc entory create failure.\n");
	return -ENOENT;
    }
    pr_warn("[SAUPWK]:sau_inputthreadready proc entry create success.\n");

    return 0;
}


module_init(oppo_saupwk_during_init);
