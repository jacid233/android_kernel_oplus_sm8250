/*
 * oppo_kevent.h - for kevent action upload upload to user layer
 *  author by wangzhenhua,Plf.Framework
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <linux/version.h>


#ifdef CONFIG_OPPO_KEVENT_TEST
ssize_t demo_kevent_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos);
#endif /* CONFIG_OPPO_KEVENT_TEST */

