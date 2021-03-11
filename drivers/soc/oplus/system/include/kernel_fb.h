/***************************************************************
** Copyright (C),  2019,  OPPO Mobile Comm Corp.,  Ltd
** VENDOR_EDIT
** File : oppo_bsp_kevent_fb.h
** Description : BSP kevent fb data
** Version : 1.0
** Date : 2019/2/25
** Author : Wen.Luo@PSW.BSP.Kernel.Stability
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**   Wen.Luo          2019/02/25        1.0           Build this moudle
**   Xiong.xing       2020/03/14        2.0           Change this moudle
**   tangjh           2020/07/28        3.0           add sensors module support
******************************************************************/

#ifndef __KERNEL_FEEDBACK_H
#define __KERNEL_FEEDBACK_H

typedef enum {
	FB_STABILITY = 0,
	FB_FS,
	FB_STORAGE,
	FB_SENSOR,
	FB_BOOT,
	FB_MAX = FB_BOOT,
} fb_tag;

#define FB_STABILITY_ID_CRASH	"202007272030"


#define FB_SENSOR_ID_CRASH	"202007272040"
#define FB_SENSOR_ID_QMI	"202007272041"

#ifdef CONFIG_OPPO_KEVENT_UPLOAD
int oplus_kevent_fb(fb_tag tag_id, const char *event_id, unsigned char *payload);
int oplus_kevent_fb_str(fb_tag tag_id, const char *event_id, unsigned char *str);
#else
int kevent_send_to_user(struct kernel_packet_info *userinfo) {return 0;}
int oplus_kevent_fb(fb_tag tag_id, const char *event_id, unsigned char *payload) {return 0;}
int oplus_kevent_fb_str(fb_tag tag_id, const char *event_id, unsigned char *str) {return 0;}
#endif
#endif


