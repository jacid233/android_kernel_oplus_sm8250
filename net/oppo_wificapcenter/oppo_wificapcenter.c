/******************************************************************************
** Copyright (C), 2019-2029, OPPO Mobile Comm Corp., Ltd
** VENDOR_EDIT, All rights reserved.
** File: - oppowificap center.c
** Description: wificapcenter (wcc)
**
** Version: 1.0
** Date : 2020/07/05
** Author: XuFenghua@CONNECTIVITY.WIFI.BASIC.CAPCENTER.190453
** TAG: OPLUS_FEATURE_WIFI_CAPCENTER
** ------------------------------- Revision History: ----------------------------
** <author>                                <data>        <version>       <desc>
** ------------------------------------------------------------------------------
**XuFenghua@CONNECTIVITY.WIFI.BASIC.CAPCENTER.190453 2020/07/05     1.0      OPLUS_FEATURE_WIFI_CAPCENTER
 *******************************************************************************/

#include <linux/types.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/sysctl.h>
#include <net/route.h>
#include <net/ip.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/version.h>
#include <net/tcp.h>
#include <linux/random.h>
#include <net/sock.h>
#include <net/dst.h>
#include <linux/file.h>
#include <net/tcp_states.h>
#include <linux/netlink.h>
#include <net/sch_generic.h>
#include <net/pkt_sched.h>
#include <net/netfilter/nf_queue.h>
#include <linux/netfilter/xt_state.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_owner.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/ipv4/nf_conntrack_ipv4.h>

#define LOG_TAG "[oppo_wificapcenter] %s line:%d "
#define debug(fmt, args...) printk(LOG_TAG fmt, __FUNCTION__, __LINE__, ##args)

/*NLMSG_MIN_TYPE is 0x10,so we start at 0x11*/
/*If response event is close contact with request,*/
/*response  can be same with req to reduce msg*/
enum{
        /*common msg for sync and async from 0x11-0x29*/
        OPPO_COMMON_MSG_BASE                    = 0x11,
	OPPO_WIFI_CAP_CENTER_NOTIFY_PID	        = OPPO_COMMON_MSG_BASE,

        /*sync msg from 0x30-0x79;*/
	OPPO_SYNC_MSG_BASE                      = 0x30,
	OPPO_SAMPLE_SYNC_GET	                = OPPO_SYNC_MSG_BASE,
	OPPO_SAMPLE_SYNC_GET_NO_RESP	        = OPPO_SYNC_MSG_BASE + 1,

        /*async msg from 0x80-(max-1)*/
        OPPO_ASYNC_MSG_BASE                     = 0x80,
	OPPO_SAMPLE_ASYNC_GET	                = OPPO_ASYNC_MSG_BASE,

	OPPO_WIFI_CAP_CENTER_MAX                = 0x100,
};

static DEFINE_MUTEX(oppo_wcc_sync_nl_mutex);
static DEFINE_MUTEX(oppo_wcc_async_nl_mutex);
static struct ctl_table_header *oppo_table_hrd;
static rwlock_t oppo_sync_nl_lock;
static rwlock_t oppo_async_nl_lock;
static u32 oppo_wcc_debug = 1;
/*user space pid*/
static u32 oppo_sync_nl_pid = 0;
static u32 oppo_async_nl_pid = 0;
/*kernel sock*/
static struct sock *oppo_sync_nl_sock;
static struct sock *oppo_async_nl_sock;
static struct timer_list oppo_timer;
static int async_msg_type = 0;

/*check msg_type in range of sync & async, 1 stands in range, 0 not in range*/
static int check_msg_in_range(struct sock *nl_sock, int msg_type)
{
        debug("nl_sock: %p, msg_type:%d", nl_sock, msg_type);
        if (msg_type >= OPPO_COMMON_MSG_BASE && msg_type < OPPO_SYNC_MSG_BASE) {
                return 1;
        }

        /*not in common part*/
        if (nl_sock == oppo_sync_nl_sock) {
                if (msg_type >= OPPO_SYNC_MSG_BASE && msg_type< OPPO_ASYNC_MSG_BASE) {
                        return 1;
                } else {
                        return 0;
                }
        } else if (nl_sock == oppo_async_nl_sock) {
                if (msg_type >= OPPO_ASYNC_MSG_BASE && msg_type < OPPO_WIFI_CAP_CENTER_MAX) {
                        return 1;
                } else {
                        return 0;
                }
        } else {
                return 0;
        }
}

/* send to user space */
static int oppo_wcc_send_to_user(struct sock *oppo_sock,
        u32 oppo_pid, int msg_type, char *payload, int payload_len)
{
	int ret = 0;
	struct sk_buff *skbuff;
	struct nlmsghdr *nlh;

	if (!check_msg_in_range(oppo_sock, msg_type)) {
	        debug("msg_type:%d not in range\n", nlh->nlmsg_type);
	        return -1;
	}

	/*allocate new buffer cache */
	skbuff = alloc_skb(NLMSG_SPACE(payload_len), GFP_ATOMIC);
	if (skbuff == NULL) {
		printk("oppo_wcc_netlink: skbuff alloc_skb failed\n");
		return -1;
	}

	/* fill in the data structure */
	nlh = nlmsg_put(skbuff, 0, 0, msg_type, NLMSG_ALIGN(payload_len), 0);
	if (nlh == NULL) {
		printk("oppo_wcc_netlink:nlmsg_put failaure\n");
		nlmsg_free(skbuff);
		return -1;
	}

	/*compute nlmsg length*/
	nlh->nlmsg_len = NLMSG_HDRLEN + NLMSG_ALIGN(payload_len);

	if(NULL != payload) {
		memcpy((char *)NLMSG_DATA(nlh), payload, payload_len);
	}

	/* set control field,sender's pid */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 0))
	NETLINK_CB(skbuff).pid = 0;
#else
	NETLINK_CB(skbuff).portid = 0;
#endif

	NETLINK_CB(skbuff).dst_group = 0;

	/* send data */
	if(oppo_pid) {
		ret = netlink_unicast(oppo_sock, skbuff, oppo_pid, MSG_DONTWAIT);
	} else {
		printk(KERN_ERR "oppo_wcc_netlink: can not unicast skbuff, oppo_pid=0\n");
		kfree_skb(skbuff);
	}
	if(ret < 0) {
		printk(KERN_ERR "oppo_wcc_netlink: can not unicast skbuff,ret = %d\n", ret);
		return 1;
	}

	return 0;
}

static void oppo_wcc_sample_resp(struct sock *oppo_sock, u32 oppo_pid, int msg_type)
{
        int payload[4];
        payload[0] = 5;
        payload[1] = 6;
        payload[2] = 7;
        payload[3] = 8;

        oppo_wcc_send_to_user(oppo_sock, oppo_pid, msg_type, (char *)payload, sizeof(payload));
        if (oppo_wcc_debug) {
                debug("msg_type = %d, sample_resp =%d%d%d%d\n", msg_type, payload[0], payload[1], payload[2], payload[3]);
        }

        return;
}

static int oppo_wcc_sample_sync_get(struct nlmsghdr *nlh)
{
	u32 *data = (u32 *)NLMSG_DATA(nlh);

	debug("sample_sync_get: %u%u%u%u\n", data[0], data[1], data[2], data[3]);
	oppo_wcc_sample_resp(oppo_sync_nl_sock, oppo_sync_nl_pid, OPPO_SAMPLE_SYNC_GET);
	return 0;
}

static int oppo_wcc_sample_sync_get_no_resp(struct nlmsghdr *nlh)
{
	u32 *data = (u32 *)NLMSG_DATA(nlh);

	debug("sample_sync_get_no_resp: %u%u%u%u\n", data[0], data[1], data[2], data[3]);
	return 0;
}


static int oppo_wcc_sample_async_get(struct nlmsghdr *nlh)
{
	u32 *data = (u32 *)NLMSG_DATA(nlh);

        async_msg_type = OPPO_SAMPLE_ASYNC_GET;
	oppo_timer.expires = jiffies + HZ;/* timer expires in ~1s*/
	add_timer(&oppo_timer);

	debug("sample_async_set: %u%u%u%u\n", data[0], data[1], data[2], data[3]);

	return 0;
}

static void oppo_wcc_timer_function(void)
{
        if (async_msg_type == OPPO_SAMPLE_ASYNC_GET) {
                oppo_wcc_sample_resp(oppo_async_nl_sock, oppo_async_nl_pid, OPPO_SAMPLE_ASYNC_GET);
                async_msg_type = 0;
        }
}

static void oppo_wcc_timer_init(void)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
	init_timer(&oppo_timer);
	oppo_timer.function = (void*)oppo_wcc_timer_function;
#else
	timer_setup(&oppo_timer, (void*)oppo_wcc_timer_function, 0);
#endif
}

static void oppo_wcc_timer_fini(void)
{
	del_timer(&oppo_timer);
}

static int oppo_wcc_sync_nl_rcv_msg(struct sk_buff *skb, struct nlmsghdr *nlh, struct netlink_ext_ack *extack)
{
	u32 portid = NETLINK_CB(skb).portid;

	if (nlh->nlmsg_type == OPPO_WIFI_CAP_CENTER_NOTIFY_PID) {
	        oppo_sync_nl_pid = portid;
	        debug("oppo_sync_nl_pid pid=%d\n", oppo_sync_nl_pid);
	}
	/* only recv msg from target pid*/
	if (portid != oppo_sync_nl_pid) {
		return -1;
	}
	if (!check_msg_in_range(oppo_sync_nl_sock, nlh->nlmsg_type)) {
	        debug("msg_type:%d not in range\n", nlh->nlmsg_type);
	        return -1;
	}

        switch (nlh->nlmsg_type) {
        case OPPO_SAMPLE_SYNC_GET:
                oppo_wcc_sample_sync_get(nlh);
                break;
        case OPPO_SAMPLE_SYNC_GET_NO_RESP:
                oppo_wcc_sample_sync_get_no_resp(nlh);
                break;
        default:
                return -EINVAL;
        }

	return 0;
}

static int oppo_wcc_async_nl_rcv_msg(struct sk_buff *skb, struct nlmsghdr *nlh, struct netlink_ext_ack *extack)
{
	u32 portid = NETLINK_CB(skb).portid;

	if (nlh->nlmsg_type == OPPO_WIFI_CAP_CENTER_NOTIFY_PID) {
	        oppo_async_nl_pid = portid;
	        debug("oppo_async_nl_pid pid=%d\n", oppo_async_nl_pid);
	}
	/* only recv msg from target pid*/
	if (portid != oppo_async_nl_pid) {
		return -1;
	}
	if (!check_msg_in_range(oppo_async_nl_sock, nlh->nlmsg_type)) {
	        debug("msg_type:%d not in range\n", nlh->nlmsg_type);
	        return -1;
	}

        switch (nlh->nlmsg_type) {
        case OPPO_SAMPLE_ASYNC_GET:
                oppo_wcc_sample_async_get(nlh);
                break;
        default:
                return -EINVAL;
        }

	return 0;
}

static void oppo_wcc_sync_nl_rcv(struct sk_buff *skb)
{
	mutex_lock(&oppo_wcc_sync_nl_mutex);
	netlink_rcv_skb(skb, &oppo_wcc_sync_nl_rcv_msg);
	mutex_unlock(&oppo_wcc_sync_nl_mutex);
}

static void oppo_wcc_async_nl_rcv(struct sk_buff *skb)
{
	mutex_lock(&oppo_wcc_async_nl_mutex);
	netlink_rcv_skb(skb, &oppo_wcc_async_nl_rcv_msg);
	mutex_unlock(&oppo_wcc_async_nl_mutex);
}

static int oppo_wcc_netlink_init(void)
{
	struct netlink_kernel_cfg cfg1 = {
		.input	= oppo_wcc_sync_nl_rcv,
	};
	struct netlink_kernel_cfg cfg2 = {
		.input	= oppo_wcc_async_nl_rcv,
	};

	oppo_sync_nl_sock = netlink_kernel_create(&init_net, NETLINK_OPPO_WIFI_CAP_CENTER_SYNC, &cfg1);
	oppo_async_nl_sock = netlink_kernel_create(&init_net, NETLINK_OPPO_WIFI_CAP_CENTER_ASYNC, &cfg2);
	debug("oppo_sync_nl_sock = %p,  oppo_async_nl_sock = %p\n", oppo_sync_nl_sock, oppo_async_nl_sock);

	if (oppo_sync_nl_sock == NULL || oppo_async_nl_sock == NULL) {
		return -ENOMEM;
	} else {
	        return 0;
        }
}

static void oppo_wcc_netlink_exit(void)
{
	netlink_kernel_release(oppo_sync_nl_sock);
	oppo_sync_nl_sock = NULL;

	netlink_kernel_release(oppo_async_nl_sock);
	oppo_async_nl_sock = NULL;
}

static struct ctl_table oppo_wcc_sysctl_table[] = {
	{
		.procname	= "oplus_wcc_debug",
		.data		= &oppo_wcc_debug,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec,
	},
	{ }
};

static int oppo_wcc_sysctl_init(void)
{
	oppo_table_hrd = register_net_sysctl
	        (&init_net, "net/oplus_wcc", oppo_wcc_sysctl_table);
	return oppo_table_hrd == NULL ? -ENOMEM : 0;
}

static void oppo_wcc_sysctl_fini(void)
{
	if(oppo_table_hrd) {
		unregister_net_sysctl_table(oppo_table_hrd);
		oppo_table_hrd = NULL;
	}
}

static int __init oppo_wcc_init(void)
{
	int ret = 0;
	rwlock_init(&oppo_sync_nl_lock);
	rwlock_init(&oppo_async_nl_lock);

	ret = oppo_wcc_netlink_init();
	if (ret < 0) {
		debug("oppo_wcc_init module failed to init netlink.\n");
	} else {
		debug("oppo_wcc_init module init netlink successfully.\n");
	}

	ret = oppo_wcc_sysctl_init();
	if (ret < 0) {
		debug("oppo_wcc_init module failed to init sysctl.\n");
	}
        else {
                debug("oppo_wcc_init module init sysctl successfully.\n");
        }

	oppo_wcc_timer_init();

	return ret;
}

static void __exit oppo_wcc_fini(void)
{
	oppo_wcc_sysctl_fini();
	oppo_wcc_netlink_exit();
	oppo_wcc_timer_fini();
}

module_init(oppo_wcc_init);
module_exit(oppo_wcc_fini);

