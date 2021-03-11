#ifdef CONFIG_OPPO_SECURE_GUARD
#include <linux/syscalls.h>
#include <linux/export.h>
#include <linux/capability.h>
#include <linux/mnt_namespace.h>
#include <linux/user_namespace.h>
#include <linux/uaccess.h>
#include <linux/file.h>
#include <linux/proc_ns.h>
#include <linux/magic.h>
#include <linux/bootmem.h>
#include <linux/task_work.h>
#include <linux/sched/task.h>

#ifdef CONFIG_OPPO_MOUNT_BLOCK
#include "oppo_guard_general.h"

#ifdef CONFIG_OPPO_KEVENT_UPLOAD
#include <linux/oppo_kevent.h>
#endif /* CONFIG_OPPO_KEVENT_UPLOAD */

#ifdef OPPO_DISALLOW_KEY_INTERFACES
int oppo_mount_block(const char __user *dir_name, unsigned long flags)
{
#ifdef CONFIG_OPPO_KEVENT_UPLOAD
	struct kernel_packet_info* dcs_event;
	char dcs_stack[sizeof(struct kernel_packet_info) + 256];
	const char* dcs_event_tag = "kernel_event";
	const char* dcs_event_id = "mount_report";
	char* dcs_event_payload = NULL;
#endif /* CONFIG_OPPO_KEVENT_UPLOAD */

/* Zhengkang.Ji@ROM.Frameworks.Security, 2018-04-05
 * System partition is not permitted to be mounted with "rw".
 */
 	char dname[16] = {0};
	if (dir_name != NULL && copy_from_user(dname,dir_name,8) == 0){
		if ((!strncmp(dname, "/system", 8) || !strncmp(dname, "/vendor", 8))&& !(flags & MS_RDONLY)
			&& (is_normal_boot_mode())) {
			printk(KERN_ERR "[OPPO]System partition is not permitted to be mounted as readwrite\n");
#ifdef CONFIG_OPPO_KEVENT_UPLOAD
/*Shengyang.Luo@Plt.Framework, 2017/12/27, add for mount report(root defence)*/
		printk(KERN_ERR "do_mount:kevent_send_to_user\n");

		dcs_event = (struct kernel_packet_info*)dcs_stack;
		dcs_event_payload = dcs_stack +
		sizeof(struct kernel_packet_info);

		dcs_event->type = 2;

		strncpy(dcs_event->log_tag, dcs_event_tag,
			sizeof(dcs_event->log_tag));
		strncpy(dcs_event->event_id, dcs_event_id,
			sizeof(dcs_event->event_id));

		dcs_event->payload_length = snprintf(dcs_event_payload, 256, "partition@@system");
		if (dcs_event->payload_length < 256) {
			dcs_event->payload_length += 1;
		}

		kevent_send_to_user(dcs_event);
#endif /* CONFIG_OPPO_KEVENT_UPLOAD */
			return -EPERM;
		}
	}

	return 0;
}
#else
int oppo_mount_block(const char __user *dir_name, unsigned long flags)
{
	return 0;
}
#endif /* OPPO_DISALLOW_KEY_INTERFACES */
#endif /* CONFIG_OPPO_MOUNT_BLOCK */
#endif /* CONFIG_OPPO_SECURE_GUARD */