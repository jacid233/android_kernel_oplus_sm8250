/************************************************************************************
 ** VENDOR_EDIT
 ** Copyright (C), 2008-2018, OPPO Mobile Comm Corp., Ltd
 **
 ** Description:
 **      oppo_temp_cali_info.c (sw23)
 **
 ** Version: 1.0
 ** Date created: 10/21/2019
 ** Author: Chao.Zeng@PSW.BSP.Sensor
 ** --------------------------- Revision History: --------------------------------
 **  <author>      <data>            <desc>
 **  qiuzuolin       10/21/2019      create the file
 ************************************************************************************/

#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>

#include <soc/oppo/oppo_project.h>

struct oppo_press_cali_data {
	int offset;
	struct proc_dir_entry 		*proc_oppo_press;
};
static struct oppo_press_cali_data *gdata = NULL;

static ssize_t press_offset_read_proc(struct file *file, char __user *buf,
		size_t count,loff_t *off)
{
	char page[256] = {0};
	int len = 0;

	if (!gdata){
		return -ENOMEM;
	}

	len = sprintf(page,"%d",gdata->offset);

	if(len > *off)
		len -= *off;
	else
		len = 0;

	if(copy_to_user(buf,page,(len < count ? len : count))){
		return -EFAULT;
	}
	*off += len < count ? len : count;
	return (len < count ? len : count);
}
static ssize_t press_offset_write_proc(struct file *file, const char __user *buf,
		size_t count, loff_t *off)

{
	char page[256] = {0};
	int input = 0;

	if(!gdata){
		return -ENOMEM;
	}


	if (count > 256)
		count = 256;
	if(count > *off)
		count -= *off;
	else
		count = 0;

	if (copy_from_user(page, buf, count))
		return -EFAULT;
	*off += count;

	if (sscanf(page, "%d", &input) != 1) {
		count = -EINVAL;
		return count;
	}

	if(input != gdata->offset){
		gdata->offset= input;
	}

	return count;
}
static struct file_operations press_offset_fops = {
	.read = press_offset_read_proc,
	.write = press_offset_write_proc,
};

static int __init oppo_press_cali_data_init(void)
{
	int rc = 0;
	struct proc_dir_entry *pentry;

	struct oppo_press_cali_data *data = NULL;
	if(gdata){
		printk("%s:just can be call one time\n",__func__);
		return 0;
	}
	data = kzalloc(sizeof(struct oppo_press_cali_data),GFP_KERNEL);
	if(data == NULL){
		rc = -ENOMEM;
		printk("%s:kzalloc fail %d\n",__func__,rc);
		return rc;
	}
	gdata = data;
	gdata->offset = 0;

	if (gdata->proc_oppo_press) {
		printk("proc_oppo_press has alread inited\n");
		return 0;
	}

	gdata->proc_oppo_press =  proc_mkdir("oppoPressCali", NULL);
	if(!gdata->proc_oppo_press) {
		pr_err("can't create proc_oppo_press proc\n");
		rc = -EFAULT;
		return rc;
	}

	pentry = proc_create("offset",0666, gdata->proc_oppo_press,
		&press_offset_fops);
	if(!pentry) {
		pr_err("create offset proc failed.\n");
		rc = -EFAULT;
		return rc;
	}

	return 0;
}

void oppo_press_cali_data_clean(void)
{
	if (gdata){
		kfree(gdata);
		gdata = NULL;
	}
}
module_init(oppo_press_cali_data_init);
module_exit(oppo_press_cali_data_clean);
MODULE_DESCRIPTION("OPPO custom version");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("qiuzuolin <qiuzuolin@oppo.com>");