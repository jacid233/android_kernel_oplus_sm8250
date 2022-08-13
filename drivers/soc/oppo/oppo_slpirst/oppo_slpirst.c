#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#ifdef CONFIG_ARM
#include <linux/sched.h>
#else
#include <linux/wait.h>
#endif
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/poll.h>


#define SLPI_CRASH_REASON_BUF_LEN 128
static wait_queue_head_t slpi_crash_reason_wq;
static char slpi_reason_buf[SLPI_CRASH_REASON_BUF_LEN] = {0};
static slpi_crash_reason_flag = 0;
void slpi_crash_reason_set(char * buf)
{
        if ((buf != NULL) && (buf[0] != '\0')) {
                if (strlen(buf) >= SLPI_CRASH_REASON_BUF_LEN) {
                        memcpy(slpi_reason_buf, buf, SLPI_CRASH_REASON_BUF_LEN - 1);
                        slpi_reason_buf[SLPI_CRASH_REASON_BUF_LEN - 1] = '\0';
                } else {
                        strcpy(slpi_reason_buf, buf);
                }
                slpi_crash_reason_flag = 1;
                wake_up_interruptible(&slpi_crash_reason_wq);
        }
}
EXPORT_SYMBOL(slpi_crash_reason_set);

/*this write interface just use for test*/
static ssize_t slpi_crash_reason_write(struct file *file, const char __user * buf,
                size_t count, loff_t * ppos)
{
        /*just for test*/
        if (count > SLPI_CRASH_REASON_BUF_LEN) {
                count = SLPI_CRASH_REASON_BUF_LEN;
        }
        if (count > *ppos) {
                count -= *ppos;
        }
        else
                count = 0;

        printk("slpi_crash_reason_write is called\n");

        if (copy_from_user(slpi_reason_buf, buf, count)) {
                return -EFAULT;
        }
        *ppos += count;

        slpi_crash_reason_flag = 1;
        wake_up_interruptible(&slpi_crash_reason_wq);

        return count;
}


static unsigned int slpi_crash_reason_poll (struct file *file, struct poll_table_struct *pt)
{
        unsigned int ptr = 0;

        poll_wait(file, &slpi_crash_reason_wq, pt);

        if (slpi_crash_reason_flag) {
                ptr |= POLLIN | POLLRDNORM;
                slpi_crash_reason_flag = 0;
        }
        return ptr;
}

static ssize_t slpi_crash_reason_read(struct file *file, char __user *buf,
                size_t count, loff_t *ppos)
{
        size_t size = 0;

        if (count > SLPI_CRASH_REASON_BUF_LEN) {
                count = SLPI_CRASH_REASON_BUF_LEN;
        }
        size = count < strlen(slpi_reason_buf) ? count : strlen(slpi_reason_buf);

        if (size > *ppos) {
                size -= *ppos;
        }
        else
                size = 0;

        if (copy_to_user(buf, slpi_reason_buf, size)) {
                return -EFAULT;
        }
        /*slpi_crash_reason_flag = 0;*/
        *ppos += size;

        return size;
}

static int slpi_crash_reason_release (struct inode *inode, struct file *file)
{
        /*slpi_crash_reason_flag = 0;*/
        /*memset(slpi_reason_buf, 0, SLPI_CRASH_REASON_BUF_LEN);*/
        return 0;
}


static const struct file_operations slpi_crash_reason_device_fops = {
        .owner  = THIS_MODULE,
        .read   = slpi_crash_reason_read,
        .write        = slpi_crash_reason_write,
        .poll        = slpi_crash_reason_poll,
        .llseek = generic_file_llseek,
        .release = slpi_crash_reason_release,
};

static struct miscdevice slpi_crash_reason_device = {
        MISC_DYNAMIC_MINOR, "slpi_crash_reason", &slpi_crash_reason_device_fops
};




static int __init slpi_crash_reason_init(void)
{
        init_waitqueue_head(&slpi_crash_reason_wq);
        return misc_register(&slpi_crash_reason_device);
}

static void __exit slpi_crash_reason_exit(void)
{
        misc_deregister(&slpi_crash_reason_device);
}

module_init(slpi_crash_reason_init);
module_exit(slpi_crash_reason_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yajie Chen <yajie.chen@oppo.com>");
