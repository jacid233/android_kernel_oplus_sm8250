/*
 * block_root_check.c - for root action upload to user layer and reboot phone
 *  author by wangzhenhua,Plf.Framework
 */
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/reboot.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/selinux.h>

#include <linux/oppo_kevent.h>
#ifdef CONFIG_OPPO_KEVENT_UPLOAD

void oppo_root_check_succ(uid_t uid, uid_t euid, uid_t fsuid, uid_t callnum)
{
	struct kernel_packet_info *dcs_event;
	char dcs_stack[sizeof(struct kernel_packet_info) + 256];
	const char* dcs_event_tag = "kernel_event";
	const char* dcs_event_id = "root_check";
	char* dcs_event_payload = NULL;

	int ret = -1;
	char comm[TASK_COMM_LEN], nameofppid[TASK_COMM_LEN];//
	struct task_struct * parent_task = NULL;
	int ppid = -1;

	memset(comm, 0, TASK_COMM_LEN);
	memset(nameofppid, 0, TASK_COMM_LEN);

	ppid = task_ppid_nr(current);
	parent_task = rcu_dereference(current->real_parent);
	if (parent_task){
		get_task_comm(nameofppid, parent_task);
	}

	dcs_event = (struct kernel_packet_info*)dcs_stack;
	dcs_event->type = 0;
	strncpy(dcs_event->log_tag, dcs_event_tag, sizeof(dcs_event->log_tag));
	strncpy(dcs_event->event_id, dcs_event_id, sizeof(dcs_event->event_id));
	dcs_event_payload = kmalloc(256, GFP_ATOMIC);
	if (NULL == dcs_event_payload){
		return;
	}
	memset(dcs_event_payload, 0, 256);


	dcs_event->payload_length = snprintf(dcs_event_payload,256,
	    "%d$$old_euid@@%d$$old_fsuid@@%d$$sys_call_number@@%d$$addr_limit@@%lx$$curr_uid@@%d$$curr_euid@@%d$$curr_fsuid@@%d$$curr_name@@%s$$ppid@@%d$$ppidname@@%s$$enforce@@%d\n",
	    uid,euid,fsuid,callnum,
	    get_fs(),current_uid().val,current_euid().val,current_fsuid().val,get_task_comm(comm, current), ppid, nameofppid,selinux_is_enabled());
	printk(KERN_INFO "oppo_root_check_succ,payload:%s\n",dcs_event_payload);
	memcpy(dcs_event->payload, dcs_event_payload, strlen(dcs_event_payload));

	ret = kevent_send_to_user(dcs_event);
	if (ret != 0 ){
		printk(KERN_INFO "Send to user failed\n");
	}

	kfree(dcs_event_payload);

	msleep(5000);

	return;
}

#endif

#ifdef CONFIG_OPPO_ROOT_CHECK
void oppo_root_reboot(void)
{
	/*Ke.Li@ROM, Security, 2019-12-16, Root interception of the second phase of the optimization project:update the restart method, only kill the process of elevation*/
	printk(KERN_INFO "[OPPO_ROOT_CHECK]:Kill the process of escalation...");
	do_group_exit(SIGKILL);
}
#endif /* CONFIG_OPPO_ROOT_CHECK */

