/*************************************************************************
 ** Copyright 2019-2029 OPPO Mobile Comm Corp., Ltd.
 ** VENDOR_EDIT, All rights reserved.
 **
 ** File: - gpio-oppo
 ** Description: gpio controller for oppo
 **
 ** Version: 1.0
 ** Date: 2020-06-10
 ** Author: ChenGuoyao@NETWORK.SIM
 ** TAG: OPLUS_FEATURE_ESIM
 ** -------------------------- Revision History: --------------------------
 ** ChenGuoyao@NETWORK.SIM 2020-06-10 1.0 OPLUS_FEATURE_ESIM
 *************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>

#define MAX_GPIOS 3
#define GPIO_DEVICE_NAME "oppo-gpio"
#define OPPO_GPIO_MAJOR 0


struct oppo_gpio_info {
	char *dts_desc; /* eg : qcom,oppo-gpio-esim */
	char *dev_node_desc; /* node : /dev/dev_node_desc */
	int   gpio;  /* -1 - invalid */
	int   gpio_mode;  /* 0 - input, 1 -output */
	int   gpio_status; /* 0 - low, 1 - high */
	dev_t devt;
	struct mutex gpio_lock;
};


struct oppo_gpio_dev {
	dev_t devt;
	int base_minor;
	struct cdev cdev;
	struct class *oppo_gpio_class;
	struct oppo_gpio_info *gpio_info;
};

struct sim2_det_data {
	struct pinctrl *pinctrl;
	int hw_sim2_det;
	int det_is_high;
	struct pinctrl_state *pull_high_state;
	struct pinctrl_state *pull_low_state;
	struct pinctrl_state *no_pull_state;
};

#define OPPO_GPIO_MAGIC 'E'
#define OPPO_GPIO_GET_OUTPUT_VALUE      _IOWR(OPPO_GPIO_MAGIC, 0, int)
#define OPPO_GPIO_GET_INPUT_VALUE       _IOWR(OPPO_GPIO_MAGIC, 1, int)
#define OPPO_GPIO_SET_OUTPUT_VALUE_HIGH _IOWR(OPPO_GPIO_MAGIC, 2, int)
#define OPPO_GPIO_SET_OUTPUT_VALUE_LOW  _IOWR(OPPO_GPIO_MAGIC, 3, int)
#define OPPO_GPIO_SET_OUTPUT_MODE       _IOWR(OPPO_GPIO_MAGIC, 4, int)
#define OPPO_GPIO_SET_INPUT_MODE        _IOWR(OPPO_GPIO_MAGIC, 5, int)
#define OPPO_GPIO_GET_GPIO_MODE         _IOWR(OPPO_GPIO_MAGIC, 6, int)

#define SIM2_DET_STATUS_PULL_LOW 0
#define SIM2_DET_STATUS_PULL_HIGH 1
#define SIM2_DET_STATUS_NO_PULL 2

struct oppo_gpio_info  oppo_gpio_info_table[MAX_GPIOS] = {
	[0] = {"qcom,oppo-gpio-esim", "esim-gpio", -1, 1, 0, 0},
	[1] = {"qcom,oppo-esim-det", "esim-det", -1, 1, 1, 0},
	[2] = {"qcom,oppo-sim2-det", "sim2-det", -1, 1, 1, 0} /* special function for detecting single-sim or dual-sim, compat with mtk */
};

static struct sim2_det_data sim2_det_info;

static int sim_det2_gpio_show(struct seq_file *m, void *v)
{
	int sim_det2_value = 0;

	if (0 == sim2_det_info.det_is_high) {
		sim_det2_value = SIM2_DET_STATUS_PULL_LOW;
		pr_notice("%s: sim_det2_value:%d,Single sim card\n", __func__);

	} else if (1 == sim2_det_info.det_is_high) {
		sim_det2_value = SIM2_DET_STATUS_PULL_HIGH;
		pr_notice("%s: sim_det2_value:%d,Dual sim card\n", __func__);

	} else {
		sim_det2_value = SIM2_DET_STATUS_NO_PULL;
		pr_notice("%s: sim_det2_value:%d,neet echo 1 > sim2_det\n", __func__);
	}

	sim2_det_info.det_is_high = -1;
	seq_printf(m, "%d\n", sim_det2_value);

	return 0;
}

static void set_sim2_gpio_status(int status)
{
	if (SIM2_DET_STATUS_PULL_LOW == status) {
		pinctrl_select_state(sim2_det_info.pinctrl, sim2_det_info.pull_low_state);

	} else if (SIM2_DET_STATUS_PULL_HIGH == status) {
		pinctrl_select_state(sim2_det_info.pinctrl, sim2_det_info.pull_high_state);

	} else if (SIM2_DET_STATUS_NO_PULL == status) {
		pinctrl_select_state(sim2_det_info.pinctrl, sim2_det_info.no_pull_state);
	}
}

static ssize_t sim_det2_proc_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	char *buf = NULL;

	if (count > 2) {
		count = 2;
	}

	buf = kmalloc(sizeof(char) * (count + 1), GFP_KERNEL);

	if (!buf) {
		return -ENOMEM;
	}

	if (copy_from_user(buf, buffer, count)) {
		kfree(buf);
		return -EFAULT;
	}

	buf[count] = '\0';

	if (!strcmp(buf, "1")) {
		sim2_det_info.det_is_high = -1;

		set_sim2_gpio_status(SIM2_DET_STATUS_PULL_HIGH);
		msleep(2);
		sim2_det_info.det_is_high = gpio_get_value(sim2_det_info.hw_sim2_det);
		pr_notice("value_sim2_det_is_high:%d\n", sim2_det_info.det_is_high);

		set_sim2_gpio_status(SIM2_DET_STATUS_NO_PULL);

	} else {
		pr_err("SET sim2_det error:%s.\n", buf);
	}

	kfree(buf);

	return count;
}
static int sim_det2_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, sim_det2_gpio_show, NULL);
}

static const struct file_operations sim2_det_fops = {
	.open = sim_det2_proc_open,
	.read = seq_read,
	.write = sim_det2_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sim2_det_init(struct platform_device *pdev, int gpio)
{
	struct proc_dir_entry *pentry;

	if (!pdev) {
		pr_err("%s: invalid param!\n", __func__);
		return -1;
	}

	sim2_det_info.hw_sim2_det = gpio;

	sim2_det_info.pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR_OR_NULL(sim2_det_info.pinctrl)) {
		 pr_err("sim2_det_info.pinctrl is null\n");
		 return -1;
	}

	sim2_det_info.pull_high_state = pinctrl_lookup_state(sim2_det_info.pinctrl,
			"sim2_det_pull_high");

	if (IS_ERR_OR_NULL(sim2_det_info.pull_high_state)) {
		pr_err("%s: Failed to get the sim2 pull HIGH status\n", __func__);
		return -1;
	}

	sim2_det_info.pull_low_state = pinctrl_lookup_state(sim2_det_info.pinctrl,
			"sim2_det_pull_low");

	if (IS_ERR_OR_NULL(sim2_det_info.pull_low_state)) {
		pr_err("%s: Failed to get the sim2 pull LOW status\n", __func__);
		return -1;
	}

	sim2_det_info.no_pull_state = pinctrl_lookup_state(sim2_det_info.pinctrl,
			"sim2_det_no_pull");

	if (IS_ERR_OR_NULL(sim2_det_info.no_pull_state)) {
		pr_err("%s: Failed to get the sim2 No pull status\n", __func__);
		return -1;
	}

	pentry = proc_create("sim2_det", 0666, NULL, &sim2_det_fops);

	if (!pentry) {
		pr_err("%s: create /proc/sim2_det proc failed.\n", __func__);
		return -1;
	}

	return 0;
}


static long oppo_gpio_ioctl(struct file *filp, unsigned int cmd,
	unsigned long data)
{
	int retval = 0;
	void __user *arg = (void __user *)data;
	struct oppo_gpio_info *gpio_info;
	int gpio_status = -1;
	int cmd_abs = cmd - OPPO_GPIO_GET_OUTPUT_VALUE;

	/* pr_info("%s: enter\n", __func__); */

	gpio_info = filp->private_data;

	if (gpio_info == NULL) {
		return -EFAULT;
	}

	mutex_lock(&gpio_info->gpio_lock);

	pr_info("%s: dev = %s cmd = %d\n", __func__,
		gpio_info->dev_node_desc ? gpio_info->dev_node_desc : "NULL",
		cmd_abs > 0 ? cmd_abs : -cmd_abs);

	switch (cmd) {
	case OPPO_GPIO_GET_OUTPUT_VALUE:
		gpio_status = gpio_info->gpio_status;

		if (copy_to_user(arg, &gpio_status, sizeof(gpio_status))) {
			retval = -EFAULT;
		}
		break;

	case OPPO_GPIO_GET_INPUT_VALUE:
		gpio_status = gpio_get_value(gpio_info->gpio);
		gpio_info->gpio_status = gpio_status;

		if (copy_to_user(arg, &gpio_status, sizeof(gpio_status))) {
			retval = -EFAULT;
		}
		break;

	case OPPO_GPIO_SET_OUTPUT_VALUE_HIGH:
		gpio_direction_output(gpio_info->gpio, 1);
		gpio_info->gpio_status = 1;
		gpio_info->gpio_mode = 1;
		break;

	case OPPO_GPIO_SET_OUTPUT_VALUE_LOW:
		gpio_direction_output(gpio_info->gpio, 0);
		gpio_info->gpio_status = 0;
		gpio_info->gpio_mode = 1;
		break;

	case OPPO_GPIO_SET_OUTPUT_MODE:
		gpio_direction_output(gpio_info->gpio, gpio_info->gpio_status);
		gpio_info->gpio_mode = 1;
		break;

	case OPPO_GPIO_SET_INPUT_MODE:
		gpio_direction_input(gpio_info->gpio);
		gpio_info->gpio_mode = 0;
		break;

	case OPPO_GPIO_GET_GPIO_MODE:
		if (copy_to_user(arg, &gpio_info->gpio_mode, sizeof(gpio_info->gpio_mode))) {
			retval = -EFAULT;
		}
		break;

	default:
		retval = -EFAULT;
		break;
	}

	mutex_unlock(&gpio_info->gpio_lock);
	return retval;
}


static int oppo_gpio_open(struct inode *inode, struct file *filp)
{
	struct cdev *cdev = inode->i_cdev;
	struct oppo_gpio_dev *dev = NULL;
	int i;

	/* pr_info("%s: enter\n", __func__); */

	dev =  container_of(cdev, struct oppo_gpio_dev, cdev);

	filp->private_data = NULL;

	for (i = 0; i < MAX_GPIOS && dev != NULL; i++) {
		if (dev->gpio_info[i].devt == inode->i_rdev) {
			filp->private_data = &(dev->gpio_info[i]);
			break;
		}
	}

	if (!filp->private_data) {
		pr_err("%s: can not find the gpio info\n", __func__);
		return -1;
	}

	return 0;
}


static ssize_t oppo_gpio_read(struct file *filp, char __user *buf, size_t siz,
	loff_t *ppos)
{
	struct oppo_gpio_info *gpio_info;
	char buff[128] = {0};
	ssize_t len = 0;

	/* pr_info("%s: enter\n", __func__); */

	gpio_info = filp->private_data;

	if (*ppos > 0) {
		return 0;
	}

	mutex_lock(&gpio_info->gpio_lock);

	len = sizeof(buff) / sizeof(buff[0]);

	if (gpio_info->gpio_mode == 0) {
		gpio_info->gpio_status = gpio_get_value(gpio_info->gpio);
	}

	len = snprintf(buff, len, "gpio = %d, gpio mode = %d, gpio status = %d\n",
			gpio_info->gpio,
			gpio_info->gpio_mode,
			gpio_info->gpio_status);

	if (copy_to_user(buf, buff, len)) {
		mutex_unlock(&gpio_info->gpio_lock);
		return -EFAULT;
	}

	mutex_unlock(&gpio_info->gpio_lock);

	*ppos += len;

	return len;
}


static const struct of_device_id oppo_gpio_dt_ids[] = {
	{ .compatible = "oppo,oppo-gpio" },
	{},
};


MODULE_DEVICE_TABLE(of, oppo_gpio_dt_ids);



static const struct file_operations oppo_gpio_fops = {
	.owner =    THIS_MODULE,
	.unlocked_ioctl = oppo_gpio_ioctl,
	.open           = oppo_gpio_open,
	.read           = oppo_gpio_read,
};


static int oppo_gpio_probe(struct platform_device *pdev)
{
	int status = -1;
	struct oppo_gpio_dev *gpio_data = NULL;
	struct device *oppo_gpio_device;
	int valid_gpio = 0;
	int i;

	pr_info("%s: enter\n", __func__);

	gpio_data = kzalloc(sizeof(*gpio_data), GFP_KERNEL);

	if (!gpio_data) {
		status = -ENOMEM;
		pr_err("%s: failed to alloc memory\n", __func__);
		goto err;
	}

	gpio_data->gpio_info = oppo_gpio_info_table;

	status = alloc_chrdev_region(&gpio_data->devt, 0, MAX_GPIOS, GPIO_DEVICE_NAME);

	if (status < 0) {
		pr_err("%s: failed to alloc chrdev\n", __func__);
		goto err_alloc;
	}

	gpio_data->base_minor = MINOR(gpio_data->devt);

	cdev_init(&gpio_data->cdev, &oppo_gpio_fops);
	status = cdev_add(&gpio_data->cdev, gpio_data->devt, MAX_GPIOS);

	if (status < 0) {
		pr_err("%s: failed to add chrdev\n", __func__);
		goto error_add;
	}

	gpio_data->oppo_gpio_class = class_create(THIS_MODULE, GPIO_DEVICE_NAME);

	if (IS_ERR(gpio_data->oppo_gpio_class)) {
		pr_err("%s: failed to create class\n", __func__);
		goto err_class;
	}

	for (i = 0; i < MAX_GPIOS; i++) {
		dev_t devt = MKDEV(MAJOR(gpio_data->devt), gpio_data->base_minor + i);

		if (!gpio_data->gpio_info[i].dts_desc) {
			continue;
		}

		gpio_data->gpio_info[i].gpio = of_get_named_gpio(pdev->dev.of_node,
				gpio_data->gpio_info[i].dts_desc, 0);

		if (gpio_is_valid(gpio_data->gpio_info[i].gpio)) {
			status = gpio_request(gpio_data->gpio_info[i].gpio,
					gpio_data->gpio_info[i].dev_node_desc ? gpio_data->gpio_info[i].dev_node_desc :
					"NULL");

			if (status) {
				pr_err("%s: failed to request gpio %d\n", __func__,
					gpio_data->gpio_info[i].gpio);
				gpio_data->gpio_info[i].gpio = -1;/* invalid */
				continue;
			}
		}

		valid_gpio += 1;

		if (gpio_data->gpio_info[i].dev_node_desc) {
			if(!strcmp(gpio_data->gpio_info[i].dev_node_desc, "sim2-det")) {
				if(0 != sim2_det_init(pdev, gpio_data->gpio_info[i].gpio)) {
					gpio_free(gpio_data->gpio_info[i].gpio);
					gpio_data->gpio_info[i].gpio = -1;
					valid_gpio -= 1;
				}
				continue;
			}

			oppo_gpio_device = device_create(gpio_data->oppo_gpio_class, NULL, devt, NULL,
					gpio_data->gpio_info[i].dev_node_desc);

			if (IS_ERR(oppo_gpio_device)) {
				pr_err("%s: failed to create device: %s\n", __func__,
					gpio_data->gpio_info[i].dev_node_desc);
				gpio_free(gpio_data->gpio_info[i].gpio);
				gpio_data->gpio_info[i].gpio = -1;/* invalid */
				valid_gpio -= 1;
				continue;
			}
		}

		gpio_data->gpio_info[i].devt = devt;

		mutex_init(&(gpio_data->gpio_info[i].gpio_lock));

		pr_info("%s: gpio_request: %d\n", __func__, gpio_data->gpio_info[i].gpio);

		/* init gpio */
		if (gpio_data->gpio_info[i].gpio_mode == 1) {
			gpio_direction_output(gpio_data->gpio_info[i].gpio,
				gpio_data->gpio_info[i].gpio_status);
		} else {
			gpio_direction_input(gpio_data->gpio_info[i].gpio);
			gpio_data->gpio_info[i].gpio_status = gpio_get_value(
					gpio_data->gpio_info[i].gpio);
		}
	}

	if (!valid_gpio) {
		pr_info("%s: no valid oppo gpio\n", __func__);
		goto err_no_gpio;
	}

	dev_set_drvdata(&pdev->dev, gpio_data);

	pr_info("%s: leave\n", __func__);

	return 0;

err_no_gpio:
	class_destroy(gpio_data->oppo_gpio_class);
err_class:
	cdev_del(&gpio_data->cdev);
error_add:
	unregister_chrdev_region(gpio_data->devt, MAX_GPIOS);
err_alloc:
	kfree(gpio_data);
err:
	return status;
}


static int oppo_gpio_remove(struct platform_device *pdev)
{
	struct oppo_gpio_dev *gpio_data = NULL;
	int i;

	gpio_data = dev_get_drvdata(&pdev->dev);

	if (gpio_data) {
		if (gpio_data->gpio_info) {
			for (i = 0; i < MAX_GPIOS; i++) {
				if (gpio_data->gpio_info[i].gpio > -1) {
					dev_t devt = MKDEV(MAJOR(gpio_data->devt), gpio_data->base_minor + i);
					gpio_free(gpio_data->gpio_info[i].gpio);
					device_destroy(gpio_data->oppo_gpio_class, devt);
				}
			}
		}

		class_destroy(gpio_data->oppo_gpio_class);
		cdev_del(&gpio_data->cdev);
		unregister_chrdev_region(gpio_data->devt, MAX_GPIOS);
		kfree(gpio_data);
	}

	return 0;
}


static struct platform_driver oppo_gpio_driver = {
	.probe = oppo_gpio_probe,
	.remove = oppo_gpio_remove,
	.driver = {
		.name = GPIO_DEVICE_NAME,
		.of_match_table = of_match_ptr(oppo_gpio_dt_ids),
	},
};

static int __init oppo_gpio_init(void)
{
	pr_info("%s: enter\n", __func__);

	return platform_driver_register(&oppo_gpio_driver);
}

static void __init oppo_gpio_exit(void)
{
	pr_info("%s: enter\n", __func__);

	platform_driver_unregister(&oppo_gpio_driver);
}


module_init(oppo_gpio_init);
module_exit(oppo_gpio_exit);


MODULE_AUTHOR("ChenGuoyao, <>");
MODULE_DESCRIPTION("oppo gpio controller");
MODULE_LICENSE("GPL");
MODULE_ALIAS("gpio:opp-gpio");
