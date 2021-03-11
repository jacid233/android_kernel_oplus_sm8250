/***********************************************************
** Copyright (C), 2008-2020, OPPO Mobile Comm Corp., Ltd.
** VENDOR_EDIT
** File: - drivers/soc/oppo/oppo_iomonitor/console.c
** Description:  code to support ext4 defrag
**
** Version: 1.0
** Date : 2020/02/29
** Author: yanwu@TECH.Storage.IO.Iomonitor, console to dump rencent kmsg
**
** ------------------ Revision History:------------------------
** <author> <data> <version > <desc>
** yanwu 2020/02/29 1.0  create the file
****************************************************************/

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/mm.h>

struct oppo_console_data {
	unsigned int len;
	unsigned int size:31;
	unsigned int overflow:1;
	char buf[0];
};

#define MEMFP (sizeof(struct oppo_console_data))

static unsigned int size = PAGE_SIZE * 2 - MEMFP;
module_param(size, uint, 0400);
MODULE_PARM_DESC(size, "size of log buffer to dump recent kernel log");
static struct oppo_console_data *ocd;
static void oppo_console_write(struct console *con, const char *s, unsigned n)
{
	unsigned len, pos;
	pos = ocd->len % ocd->size;
	ocd->len += n;
	if (!ocd->overflow && ocd->len > ocd->size)
		ocd->overflow = 1;
	if (unlikely(n > ocd->size)) {
		s += n - ocd->size;
		pos = (pos + n) % ocd->size;
		n = ocd->size;
	}
	len = ocd->size - pos;
	if (n > len) {
		memcpy(ocd->buf + pos, s, len);
		n -= len;
		memcpy(ocd->buf, s + len, n);
	} else {
		memcpy(ocd->buf + pos, s, n);
	}
}

static struct console oppo_console_dev = {
	.name = "oppocon",
	.write = oppo_console_write,
	.flags = CON_PRINTBUFFER | CON_ENABLED,
	.index = -1,
};

/**
 * dump_log() -- dump the recent log to buffer
 * @buf - buffer to save the kernel log
 * @len - size of the buffer
 * return the size of log dumped
 */
unsigned dump_log(char *buf, unsigned len)
{
	unsigned n, i;
	if (!ocd)
		return 0;
	n = ocd->overflow ? ocd->size : ocd->len;
	if (unlikely(len > n))
		len = n;
	i = (ocd->len - len) % ocd->size;
	n = ocd->size - i;
	if (n > len) {
		memcpy(buf, ocd->buf + i, len);
	} else {
		memcpy(buf, ocd->buf + i, n);
		memcpy(buf, ocd->buf, len - n);
	}
	return len;
}

EXPORT_SYMBOL(dump_log);

//#define DEBUG
#ifdef DEBUG
/* /proc/oppocon_dump */
#define OPPOCON_DUMP "oppocon_dump"
static void *oppocon_dump_start(struct seq_file *seq, loff_t * pos)
{
	if (*pos < 0 || *pos >= ocd->len)
		return NULL;
	if (*pos == 0 && ocd->overflow)
		*pos = ocd->len - ocd->size;
	return pos;
}

static void *oppocon_dump_next(struct seq_file *seq, void *v, loff_t * pos)
{
	++*pos;
	if (*pos < 0 || *pos >= ocd->len)
		return NULL;

	return pos;
}

static int oppocon_dump_show(struct seq_file *seq, void *v)
{
	unsigned idx = (*(loff_t *) v) % ocd->size;
	seq_putc(seq, ocd->buf[idx]);
	return 0;
}

static void oppocon_dump_stop(struct seq_file *seq, void *v)
{
}

const struct seq_operations oppocon_dump_ops = {
	.start = oppocon_dump_start,
	.next = oppocon_dump_next,
	.stop = oppocon_dump_stop,
	.show = oppocon_dump_show,
};
#endif

static int __init oppo_console_init(void)
{
	ocd = kvmalloc(MEMFP + size, GFP_KERNEL);
	if (!ocd) {
		pr_err("fail to allocate memory\n");
		return -ENOMEM;
	}
	ocd->size = size;
	ocd->overflow = 0;
	ocd->len = 0;
	register_console(&oppo_console_dev);
#ifdef DEBUG
	proc_create_seq(OPPOCON_DUMP, S_IRUGO, NULL, &oppocon_dump_ops);
#endif
	return 0;
}

void oppo_console_exit(void)
{
#ifdef DEBUG
	remove_proc_entry(OPPOCON_DUMP, NULL);
#endif
	unregister_console(&oppo_console_dev);
	kvfree(ocd);
}

module_init(oppo_console_init);
module_exit(oppo_console_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yanwu <>");
MODULE_DESCRIPTION("Oppo Console driver");
