/***********************************************************
** Copyright (C), 2009-2019, OPPO Mobile Comm Corp., Ltd.
** VENDOR_EDIT
** File: - oppo_nwpower.h
** Description: BugID:2120730, Add for OPLUS_FEATURE_NWPOWER
**
** Version: 1.0
** Date : 2019/07/31
** Author: Asiga@PSW.NW.DATA
** TAG: OPLUS_ARCH_EXTENDS
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** Asiga 2019/07/31 1.0 build this module
****************************************************************/
#ifndef __OPPO_NWPOWER_H_
#define __OPPO_NWPOWER_H_

#include <net/sock.h>
#include <linux/skbuff.h>

#define OPPO_TCP_TYPE_V4               1
#define OPPO_TCP_TYPE_V6               2
#define OPPO_NET_OUTPUT                0
#define OPPO_NET_INPUT                 1

#define OPPO_NW_WAKEUP_SUM                 8
#define OPPO_NW_MPSS                       0
#define OPPO_NW_QRTR                       1
#define OPPO_NW_MD                         2
#define OPPO_NW_WIFI                       3
#define OPPO_NW_TCP_IN                     4
#define OPPO_NW_TCP_OUT                    5
#define OPPO_NW_TCP_RE_IN                  6
#define OPPO_NW_TCP_RE_OUT                 7

//Add for Mpss wakeup
extern void oppo_match_modem_wakeup(void);
extern void oppo_match_wlan_wakeup(void);

//Add for QMI wakeup
extern void oppo_match_qrtr_service_port(int type, int id, int port);
extern void oppo_match_qrtr_wakeup(int src_node, int src_port, int dst_port, unsigned int arg1, unsigned int arg2);
extern void oppo_update_qrtr_flag(int val);

//Add for IPA wakeup(tcp input)
extern void oppo_match_ipa_ip_wakeup(int type, struct sk_buff *skb);
extern void oppo_match_ipa_tcp_wakeup(int type, struct sock *sk);
extern void oppo_ipa_schedule_work(void);

//Add for tcp output
extern void oppo_match_tcp_output(struct sock *sk);

//Add for tcp retrans
extern void oppo_match_tcp_input_retrans(struct sock *sk);
extern void oppo_match_tcp_output_retrans(struct sock *sk);

//#ifdef OPLUS_FEATURE_NWPOWER_NETCONTROLLER
//PengHao@NETWORK.POWER.26376, 2020/04/27, add for oppo net controller
extern bool oppo_check_socket_in_blacklist(int is_input, struct socket *sock);
//#endif /* OPLUS_FEATURE_NWPOWER_NETCONTROLLER */

#endif /* __OPPO_NWPOWER_H_ */