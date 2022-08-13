/***********************************************************
** Copyright (C), 2009-2019, OPPO Mobile Comm Corp., Ltd.
** VENDOR_EDIT
** File: - oppo_nwpower.c
** Description: BugID:2120730, Add for classify glink wakeup services and count IPA wakeup.
**
** Version: 1.0
** Date : 2019/07/31
** Author: Asiga@PSW.NW.DATA
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** Asiga 2019/07/31 1.0 build this module
****************************************************************/
#include <linux/types.h>
#include <linux/module.h>
#include <linux/qrtr.h>
#include <linux/ipc_logging.h>
#include <linux/atomic.h>
#include <linux/skbuff.h>
#include <linux/err.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <net/oppo_nwpower.h>

#define GLINK_MODEM_NODE_ID    0x3
#define GLINK_ADSP_NODE_ID     0x5
#define GLINK_CDSP_NODE_ID     0xa
#define GLINK_SLPI_NODE_ID     0x9
#define GLINK_NPU_NODE_ID      0xb

#define GLINK_IPA_SERVICE_ID   49
#define UNSL_MSG_MAX           20
#define RRCINTIMER_TRY_TIMES   8

#define QRTR_SERVICE_SUM       120
#define IP_SUM                 100
#define MODEM_WAKEUP_SRC_NUM   3
#define MODEM_IPA_WS_INDEX     1
#define MODEM_QMI_WS_INDEX     2

static DEFINE_SPINLOCK(oppo_qrtrhook_lock);
static atomic_t qrtr_wakeup_hook_boot = ATOMIC_INIT(0);
static u64 glink_wakeup_times = 0;
static u64 glink_adsp_wakeup_times = 0;
static u64 glink_cdsp_wakeup_times = 0;
static u64 glink_modem_wakeup_times = 0;
static u64 glink_slpi_wakeup_times = 0;
static u64 glink_npu_wakeup_times = 0;
static u64 service_wakeup_times[QRTR_SERVICE_SUM][4] = {{0}};
static struct timespec ts;
static u16 rrc_inactivitytime_try_time = RRCINTIMER_TRY_TIMES;
static u64 rrc_time[RRCINTIMER_TRY_TIMES][2] = {{0}};

static void oppo_ipv4_hook_work_callback(struct work_struct *work);
static void oppo_ipv4_tcpsynretrans_hook_work_callback(struct work_struct *work);
static void oppo_match_qrtr_new_service_port(int id, int port);
static void oppo_match_qrtr_del_service_port(int id);
static void oppo_match_glink_wakeup(int src_node);
static void oppo_print_qrtr_wakeup(bool unsl);
static void oppo_print_ipa_wakeup(bool unsl);
static void oppo_print_tcpsynretrans(bool unsl);
static void oppo_print_rrcinactivitytimer(bool unsl);
static void oppo_reset_qrtr_wakeup(void);
static void oppo_reset_ipa_wakeup(void);
static void oppo_reset_tcpsynretrans(void);
static void oppo_reset_rrcinactivitytimer(void);
static int oppo_nwpower_send_to_user(int msg_type,char *msg_data, int msg_len);
static void oppo_nwpower_netlink_rcv(struct sk_buff *skb);
static int oppo_nwpower_netlink_rcv_msg(struct sk_buff *skb, struct nlmsghdr *nlh, struct netlink_ext_ack *extack);
static int oppo_nwpower_netlink_init(void);
static void oppo_nwpower_netlink_exit(void);

extern atomic_t qrtr_first_msg;
extern atomic_t ipa_first_msg;
extern atomic_t pcie_rc2_first_msg_qmi;
extern atomic_t pcie_rc2_first_msg_ipa;
extern atomic_t ipa_wakeup_hook_boot;
extern atomic_t tcpsynretrans_hook_boot;
extern u64 wakeup_source_count_modem;
extern int modem_wakeup_src_count[MODEM_WAKEUP_SRC_NUM];

struct oppo_ipv4_hook_struct oppo_ipv4_hook = {
	.ipa_wakeup = {0},
};
DECLARE_WORK(oppo_ipv4_hook_work, oppo_ipv4_hook_work_callback);

struct oppo_ipv4_tcpsynretrans_hook_struct oppo_ipv4_tcpsynretrans_hook = {
	.tcp_retransmission = {0},
};
DECLARE_WORK(oppo_ipv4_tcpsynretrans_hook_work, oppo_ipv4_tcpsynretrans_hook_work_callback);

//Netlink
enum{
	NW_POWER_ANDROID_PID                    = 0x11,
	NW_POWER_BOOT_MONITOR                   = 0x12,
	NW_POWER_STOP_MONITOR                   = 0x13,
	NW_POWER_STOP_MONITOR_UNSL              = 0x14,
	NW_POWER_UNSL_MONITOR                   = 0x15,
};
static DEFINE_MUTEX(netlink_mutex);
static u32 oppo_nwpower_pid = 0;
static struct sock *oppo_nwpower_sock;
//TOP5 QMI + TOP5 IPA + Glink
static u64 unsl_msg[UNSL_MSG_MAX] = {0};

static void oppo_ipv4_hook_work_callback(struct work_struct *work) {
	int i;
	int j;
	u32 count;
	u64 temp_sort;
	bool handle = false;
	wakeup_source_count_modem++;
	modem_wakeup_src_count[MODEM_IPA_WS_INDEX]++;
	for (i = 0; i < IP_SUM; ++i) {
		if (oppo_ipv4_hook.addr == (oppo_ipv4_hook.ipa_wakeup[i] & 0xFFFFFFFF)) {
			count = (oppo_ipv4_hook.ipa_wakeup[i] & 0xFFFC000000000000) >> 50;
			oppo_ipv4_hook.ipa_wakeup[i] = (u64)(++count) << 50 | (oppo_ipv4_hook.ipa_wakeup[i] & 0x3FFFFFFFFFFFF);
			handle = true;
			break;
		} else if (oppo_ipv4_hook.ipa_wakeup[i] == 0) {
			count = 1 << 18 | (from_kuid_munged(&init_user_ns, oppo_ipv4_hook.uid) & 0x3FFFF);
			oppo_ipv4_hook.ipa_wakeup[i] = (u64)count << 32 | oppo_ipv4_hook.addr;
			handle = true;
			break;
		}
	}
	if (!handle) {
		i = 99;
		count = 1 << 18 | (from_kuid_munged(&init_user_ns, oppo_ipv4_hook.uid) & 0x3FFFF);
		oppo_ipv4_hook.ipa_wakeup[99] = (u64)count << 32 | oppo_ipv4_hook.addr;
	}
	printk("[oppo_nwpower] IPAWakeup: IP: %d, UID: %d, Count: %d",
		oppo_ipv4_hook.addr, from_kuid_munged(&init_user_ns, oppo_ipv4_hook.uid), (oppo_ipv4_hook.ipa_wakeup[i] & 0xFFFC000000000000) >> 50);
	//Insert sort
	for (i = 1; i < IP_SUM; ++i) {
		temp_sort = oppo_ipv4_hook.ipa_wakeup[i];
		count = (temp_sort & 0xFFFC000000000000) >> 50;
		j = i - 1;
		while (j >=0 && count > ((oppo_ipv4_hook.ipa_wakeup[j] & 0xFFFC000000000000) >> 50)) {
			oppo_ipv4_hook.ipa_wakeup[j+1] = oppo_ipv4_hook.ipa_wakeup[j];
			--j;
		}
		oppo_ipv4_hook.ipa_wakeup[j+1] = temp_sort;
	}
	atomic_set(&ipa_first_msg, 0);
	atomic_set(&pcie_rc2_first_msg_ipa, 0);
}

static void oppo_ipv4_tcpsynretrans_hook_work_callback(struct work_struct *work) {
	int i;
	int j;
	u32 count;
	u64 temp_sort;
	bool handle = false;
	if (oppo_ipv4_tcpsynretrans_hook.addr == 0) return;
	for (i = 0; i < IP_SUM; ++i) {
		if (oppo_ipv4_tcpsynretrans_hook.addr == (oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i] & 0xFFFFFFFF)) {
			count = (oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i] & 0xFFFC000000000000) >> 50;
			oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i] = (u64)(++count) << 50 | (oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i] & 0x3FFFFFFFFFFFF);
			handle = true;
			break;
		} else if (oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i] == 0) {
			count = 1 << 18 | (from_kuid_munged(&init_user_ns, oppo_ipv4_tcpsynretrans_hook.uid) & 0x3FFFF);
			oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i] = (u64)count << 32 | oppo_ipv4_tcpsynretrans_hook.addr;
			handle = true;
			break;
		}
	}
	if (!handle) {
		i = 99;
		count = 1 << 18 | (from_kuid_munged(&init_user_ns, oppo_ipv4_tcpsynretrans_hook.uid) & 0x3FFFF);
		oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[99] = (u64)count << 32 | oppo_ipv4_tcpsynretrans_hook.addr;
	}
	printk("[oppo_nwpower] TCPSynRetrans: IP: %d, UID: %d, Count: %d",
		oppo_ipv4_tcpsynretrans_hook.addr, from_kuid_munged(&init_user_ns, oppo_ipv4_tcpsynretrans_hook.uid), (oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i] & 0xFFFC000000000000) >> 50);
	//Insert sort
	for (i = 1; i < IP_SUM; ++i) {
		temp_sort = oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i];
		count = (temp_sort & 0xFFFC000000000000) >> 50;
		j = i - 1;
		while (j >=0 && count > ((oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[j] & 0xFFFC000000000000) >> 50)) {
			oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[j+1] = oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[j];
			--j;
		}
		oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[j+1] = temp_sort;
	}
}

extern void oppo_match_qrtr_service_port(int type, int id, int port) {
	unsigned long flags;
	spin_lock_irqsave(&oppo_qrtrhook_lock, flags);
	if (type == QRTR_TYPE_NEW_SERVER) {
		oppo_match_qrtr_new_service_port(id, port);
	} else if (type == QRTR_TYPE_DEL_SERVER) {
		oppo_match_qrtr_del_service_port(id);
	}
	spin_unlock_irqrestore(&oppo_qrtrhook_lock, flags);
}

static void oppo_match_qrtr_new_service_port(int id, int port) {
	int i;
	for (i = 0; i < QRTR_SERVICE_SUM; ++i) {
		if (service_wakeup_times[i][0] == 0) {
			service_wakeup_times[i][0] = 1;
			service_wakeup_times[i][1] = id;
			service_wakeup_times[i][2] = port;
			//printk("[oppo_nwpower] QrtrNewService[%d]: ServiceID: %d, PortID: %d", i, id, port);
			break;
		} else {
			if (service_wakeup_times[i][1] == id && service_wakeup_times[i][2] == port) {
				//printk("[oppo_nwpower] QrtrNewService[%d]: Ignore.");
				break;
			}
		}
	}
}

static void oppo_match_qrtr_del_service_port(int id) {
	int i;
	for (i = 0; i < QRTR_SERVICE_SUM; ++i) {
		if (service_wakeup_times[i][0] == 1 && service_wakeup_times[i][1] == id) {
			service_wakeup_times[i][0] = 0;
			//printk("[oppo_nwpower] QrtrDelService[%d]: ServiceID: %d", i, id);
			break;
		}
	}
}

extern void oppo_match_qrtr_wakeup(int src_node, int src_port, int dst_port, unsigned int arg1, unsigned int arg2) {
	int i;
	unsigned long flags;
	if (atomic_read(&qrtr_wakeup_hook_boot) == 1 && (atomic_read(&qrtr_first_msg) == 1 || atomic_read(&pcie_rc2_first_msg_qmi) == 1)) {
		spin_lock_irqsave(&oppo_qrtrhook_lock, flags);
		for (i = 0; i < QRTR_SERVICE_SUM; ++i) {
			if (service_wakeup_times[i][0] == 1 && (service_wakeup_times[i][2] == src_port || service_wakeup_times[i][2] == dst_port)) {
				service_wakeup_times[i][3] += 1;
				if (service_wakeup_times[i][1] == GLINK_IPA_SERVICE_ID) {
					//try to get RRC inactivitytimer
					if (rrc_inactivitytime_try_time > 0) {
						rrc_inactivitytime_try_time--;
						ts = current_kernel_time();
						rrc_time[rrc_inactivitytime_try_time][0] = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
					}
					printk("[oppo_nwpower] QrtrWakeup: ServiceID: %d(IPA), NodeID: %d, PortID: %d, Msg: [%08x %08x], Count: %d",
						service_wakeup_times[i][1], src_node, service_wakeup_times[i][2], arg1, arg2, service_wakeup_times[i][3]);
				} else {
					//try to get RRC inactivitytimer
					if (service_wakeup_times[i][1] == 1 && rrc_inactivitytime_try_time >= 0 && rrc_inactivitytime_try_time < RRCINTIMER_TRY_TIMES && rrc_time[rrc_inactivitytime_try_time][0] != 0) {
						ts = current_kernel_time();
						rrc_time[rrc_inactivitytime_try_time][1] = (ts.tv_sec * 1000 + ts.tv_nsec / 1000000) - rrc_time[rrc_inactivitytime_try_time][0];
						rrc_time[rrc_inactivitytime_try_time][0] = 0;
					}
					printk("[oppo_nwpower] QrtrWakeup: ServiceID: %d, NodeID: %d, PortID: %d, Msg: [%08x %08x], Count: %d",
						service_wakeup_times[i][1], src_node, service_wakeup_times[i][2], arg1, arg2, service_wakeup_times[i][3]);
				}
				break;
			}
		}
		oppo_match_glink_wakeup(src_node);
		atomic_set(&qrtr_first_msg, 0);
		atomic_set(&pcie_rc2_first_msg_qmi, 0);
		spin_unlock_irqrestore(&oppo_qrtrhook_lock, flags);
	}
}

static void oppo_match_glink_wakeup(int src_node) {
	glink_wakeup_times++;
	if (src_node == GLINK_MODEM_NODE_ID) {
		//wakeup_source_count_modem++;
		//modem_wakeup_src_count[MODEM_QMI_WS_INDEX]++;
		glink_modem_wakeup_times++;
	} else if (src_node == GLINK_ADSP_NODE_ID) {
		//wakeup_source_count_adsp++;
		glink_adsp_wakeup_times++;
	} else if (src_node == GLINK_CDSP_NODE_ID) {
		//wakeup_source_count_cdsp++;
		glink_cdsp_wakeup_times++;
	} else if (src_node == GLINK_SLPI_NODE_ID) {
		glink_slpi_wakeup_times++;
	} else if (src_node == GLINK_NPU_NODE_ID) {
		glink_npu_wakeup_times++;
	}
	printk("[oppo_nwpower] GlinkWakeup: NodeID: %d, Modem: %d, Adsp: %d, Cdsp: %d, Slpi: %d, Npu: %d, Glink: %d",
		src_node, glink_modem_wakeup_times, glink_adsp_wakeup_times, glink_cdsp_wakeup_times,
		glink_slpi_wakeup_times, glink_npu_wakeup_times, glink_wakeup_times);
}

static void oppo_print_qrtr_wakeup(bool unsl) {
	u64 temp[5][4] = {{0}};
	u64 max_count = 0;
	u64 max_count_id = 0;
	int j;
	int i;
	int k;
	for (j = 0; j < 5; ++j) {
		for (i = 0; i < QRTR_SERVICE_SUM; ++i) {
			if (service_wakeup_times[i][0] == 1 && service_wakeup_times[i][3] > max_count) {
				max_count = service_wakeup_times[i][3];
				max_count_id = i;
			}
		}
		for (k = 0;k < 4; ++k) {
			temp[j][k] = service_wakeup_times[max_count_id][k];
		}
		max_count = 0;
		service_wakeup_times[max_count_id][3] = 0;
		if (unsl) {
			if (temp[j][3] > 0) unsl_msg[j] = temp[j][2] << 32 | ((u32)temp[j][1] << 16 | (u16)temp[j][3]);
		}
		if (temp[j][3] > 0) printk("[oppo_nwpower] QrtrWakeupMax[%d]: ServiceID: %d, PortID: %d, Count: %d",
			j, temp[j][1], temp[j][2], temp[j][3]);
	}
	if (unsl) {
		unsl_msg[10] = (glink_modem_wakeup_times << 48) |
			((glink_adsp_wakeup_times & 0xFFFF) << 32) |
			((glink_cdsp_wakeup_times & 0xFFFF) << 16) |
			(glink_wakeup_times & 0xFFFF);
	}
	printk("[oppo_nwpower] GlinkWakeupMax: Modem: %d, Adsp: %d, Cdsp: %d, Slpi: %d, Npu: %d, Glink: %d",
		glink_modem_wakeup_times, glink_adsp_wakeup_times, glink_cdsp_wakeup_times,
		glink_slpi_wakeup_times, glink_npu_wakeup_times, glink_wakeup_times);
}

static void oppo_print_ipa_wakeup(bool unsl) {
	int i;
	u32 count;
	for (i = 0; i < 5;++i) {
		if (unsl) {
			unsl_msg[5+i] = oppo_ipv4_hook.ipa_wakeup[i];
		}
		count = (oppo_ipv4_hook.ipa_wakeup[i] & 0xFFFC000000000000) >> 50;
		if (count > 0) {
			printk("[oppo_nwpower] IPAWakeupMAX[%d]: IP: %d, UID: %d, Count: %d",
				i, oppo_ipv4_hook.ipa_wakeup[i] & 0xFFFFFFFF,
				(oppo_ipv4_hook.ipa_wakeup[i] & 0x3FFFF00000000) >> 32, count);
		}
	}
}

static void oppo_print_tcpsynretrans(bool unsl) {
	int i;
	u32 count;
	for (i = 0; i < 3;++i) {
		if (unsl) {
			unsl_msg[11+i] = oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i];
		}
		count = (oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i] & 0xFFFC000000000000) >> 50;
		if (count > 0) {
			printk("[oppo_nwpower] TCPSynRetransMAX[%d]: IP: %d, UID: %d, Count: %d",
				i, oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i] & 0xFFFFFFFF,
				(oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i] & 0x3FFFF00000000) >> 32, count);
		}
	}
}

static void oppo_print_rrcinactivitytimer(bool unsl) {
	if (unsl) {
		if (RRCINTIMER_TRY_TIMES == 8) {
			unsl_msg[14] = (rrc_time[0][1]/1000 << 48) |
			((rrc_time[1][1]/1000 & 0xFFFF) << 32) |
			((rrc_time[2][1]/1000 & 0xFFFF) << 16) |
			(rrc_time[3][1]/1000 & 0xFFFF);
			unsl_msg[15] = (rrc_time[4][1]/1000 << 48) |
			((rrc_time[5][1]/1000 & 0xFFFF) << 32) |
			((rrc_time[6][1]/1000 & 0xFFFF) << 16) |
			(rrc_time[7][1]/1000 & 0xFFFF);
		}
	}
	printk("[oppo_nwpower] RRCInactivityTimer: [%d, %d, %d, %d, %d, %d, %d, %d]",
		rrc_time[0][1]/1000, rrc_time[1][1]/1000, rrc_time[2][1]/1000, rrc_time[3][1]/1000,
		rrc_time[4][1]/1000, rrc_time[5][1]/1000, rrc_time[6][1]/1000, rrc_time[7][1]/1000);
}

static void oppo_reset_qrtr_wakeup() {
	int i;
	for (i = 0; i < QRTR_SERVICE_SUM; ++i) {
		if (service_wakeup_times[i][0] == 1) {
			service_wakeup_times[i][3] = 0;
		}
	}
	glink_wakeup_times = 0;
	glink_adsp_wakeup_times = 0;
	glink_cdsp_wakeup_times = 0;
	glink_modem_wakeup_times = 0;
	glink_slpi_wakeup_times = 0;
	glink_npu_wakeup_times = 0;
}

static void oppo_reset_ipa_wakeup() {
	int i;
	for (i = 0; i < IP_SUM; ++i) {
		oppo_ipv4_hook.ipa_wakeup[i] = 0;
	}
}

static void oppo_reset_tcpsynretrans() {
	int i;
	for (i = 0; i < IP_SUM; ++i) {
		oppo_ipv4_tcpsynretrans_hook.tcp_retransmission[i] = 0;
	}
}

static void oppo_reset_rrcinactivitytimer() {
	int i;
	rrc_inactivitytime_try_time = RRCINTIMER_TRY_TIMES;
	for (i = 0; i < RRCINTIMER_TRY_TIMES; ++i) {
		rrc_time[i][0] = 0;
		rrc_time[i][1] = 0;
	}
}

extern void oppo_nwpower_hook_on(bool normal) {
	//if own Netlink is ok, ignore sla
	if (normal && oppo_nwpower_pid > 0) {
		return;
	}
	atomic_set(&qrtr_wakeup_hook_boot, 1);
	atomic_set(&ipa_wakeup_hook_boot, 1);
	atomic_set(&tcpsynretrans_hook_boot, 1);
}

extern void oppo_nwpower_hook_off(bool normal, bool unsl) {
	//if own Netlink is ok, ignore sla
	unsigned long flags;
	if (normal && oppo_nwpower_pid > 0) {
		return;
	}
	atomic_set(&qrtr_wakeup_hook_boot, 0);
	atomic_set(&ipa_wakeup_hook_boot, 0);
	atomic_set(&tcpsynretrans_hook_boot, 0);

	spin_lock_irqsave(&oppo_qrtrhook_lock, flags);
	oppo_print_qrtr_wakeup(unsl);
	oppo_reset_qrtr_wakeup();
	spin_unlock_irqrestore(&oppo_qrtrhook_lock, flags);

	oppo_print_ipa_wakeup(unsl);
	oppo_reset_ipa_wakeup();

	oppo_print_tcpsynretrans(unsl);
	oppo_reset_tcpsynretrans();

	oppo_print_rrcinactivitytimer(unsl);
	oppo_reset_rrcinactivitytimer();

	if (unsl) {
		oppo_nwpower_send_to_user(NW_POWER_UNSL_MONITOR, (char*)unsl_msg, sizeof(unsl_msg));
		memset(unsl_msg, 0x0, sizeof(unsl_msg));
	}
}

static int oppo_nwpower_send_to_user(int msg_type,char *msg_data, int msg_len) {
	int ret = 0;
	struct sk_buff *skb;
	struct nlmsghdr *nlh;
	if (oppo_nwpower_pid == 0) {
		printk("[oppo_nwpower] netlink: oppo_nwpower_pid = 0.\n");
		return -1;
	}
	skb = alloc_skb(NLMSG_SPACE(msg_len), GFP_ATOMIC);
	if (skb == NULL) {
		printk("[oppo_nwpower] netlink: alloc_skb failed.\n");
		return -2;
	}
	nlh = nlmsg_put(skb, 0, 0, msg_type, NLMSG_ALIGN(msg_len), 0);
	if (nlh == NULL) {
		printk("[oppo_nwpower] netlink: nlmsg_put failed.\n");
		nlmsg_free(skb);
		return -3;
	}
	nlh->nlmsg_len = NLMSG_HDRLEN + NLMSG_ALIGN(msg_len);
	if(msg_data != NULL) {
		memcpy((char*)NLMSG_DATA(nlh), msg_data, msg_len);
	}
	NETLINK_CB(skb).portid = 0;
	NETLINK_CB(skb).dst_group = 0;
	ret = netlink_unicast(oppo_nwpower_sock, skb, oppo_nwpower_pid, MSG_DONTWAIT);
	if(ret < 0) {
		printk(KERN_ERR "[oppo_nwpower] netlink: netlink_unicast failed, ret = %d.\n",ret);
		return -4;
	}
	return 0;
}

static void oppo_nwpower_netlink_rcv(struct sk_buff *skb) {
	mutex_lock(&netlink_mutex);
	netlink_rcv_skb(skb, &oppo_nwpower_netlink_rcv_msg);
	mutex_unlock(&netlink_mutex);
}

static int oppo_nwpower_netlink_rcv_msg(struct sk_buff *skb, struct nlmsghdr *nlh, struct netlink_ext_ack *extack) {
	int ret = 0;
	switch (nlh->nlmsg_type) {
		case NW_POWER_ANDROID_PID:
			oppo_nwpower_pid = NETLINK_CB(skb).portid;
			printk("[oppo_nwpower] netlink: oppo_nwpower_pid = %d.\n",oppo_nwpower_pid);
			break;
		case NW_POWER_BOOT_MONITOR:
			oppo_nwpower_hook_on(false);
			printk("[oppo_nwpower] netlink: hook_on.\n");
			break;
		case NW_POWER_STOP_MONITOR:
			oppo_nwpower_hook_off(false, false);
			printk("[oppo_nwpower] netlink: hook_off.\n");
			break;
		case NW_POWER_STOP_MONITOR_UNSL:
			oppo_nwpower_hook_off(false, false);
			printk("[oppo_nwpower] netlink: hook_off_unsl.");
			break;
		default:
			return -EINVAL;
	}
	return ret;
}

static int oppo_nwpower_netlink_init(void) {
	struct netlink_kernel_cfg cfg = {
		.input = oppo_nwpower_netlink_rcv,
	};
	oppo_nwpower_sock = netlink_kernel_create(&init_net, NETLINK_OPPO_NWPOWERSTATE, &cfg);
	return oppo_nwpower_sock == NULL ? -ENOMEM : 0;
}

static void oppo_nwpower_netlink_exit(void) {
	netlink_kernel_release(oppo_nwpower_sock);
	oppo_nwpower_sock = NULL;
}

static int __init oppo_nwpower_init(void) {
	int ret = 0;
	ret = oppo_nwpower_netlink_init();
	if (ret < 0) {
		printk("[oppo_nwpower] netlink: failed to init netlink.\n");
	} else {
		printk("[oppo_nwpower] netlink: init netlink successfully.\n");
	}
	return ret;
}

static void __exit oppo_nwpower_fini(void) {
	oppo_nwpower_netlink_exit();
}

module_init(oppo_nwpower_init);
module_exit(oppo_nwpower_fini);