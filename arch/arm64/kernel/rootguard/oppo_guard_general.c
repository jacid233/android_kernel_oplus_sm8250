#ifdef CONFIG_OPPO_SECURE_GUARD
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>

/*Yi.Zhang@Security MTK and Qcom get_boot_mode differ 2020-06-18*/
#ifdef CONFIG_OPLUS_SYSTEM_KERNEL_QCOM
#include <soc/oppo/boot_mode.h>
bool is_normal_boot_mode(void)
{
	return MSM_BOOT_MODE__NORMAL == get_boot_mode();
}
#else
#define MTK_NORMAL_BOOT	0
extern unsigned int get_boot_mode(void);
bool is_normal_boot_mode(void)
{
	return MTK_NORMAL_BOOT == get_boot_mode();
}
#endif

/*Yi.Zhang@Security add for detect bootloader unlock state 2020-06-18*/
enum{
        BOOT_STATE__GREEN,
        BOOT_STATE__ORANGE,
        BOOT_STATE__YELLOW,
        BOOT_STATE__RED,
};


static int __ro_after_init g_boot_state  = BOOT_STATE__GREEN;


bool is_unlocked(void)
{
	return  BOOT_STATE__ORANGE== g_boot_state;
}

static int __init boot_state_init(void)
{
	char * substr = strstr(boot_command_line, "androidboot.verifiedbootstate=");
	if (substr) {
   		substr += strlen("androidboot.verifiedbootstate=");
        if (strncmp(substr, "green", 5) == 0) {
        	g_boot_state = BOOT_STATE__GREEN;
        } else if (strncmp(substr, "orange", 6) == 0) {
       		g_boot_state = BOOT_STATE__ORANGE;
        } else if (strncmp(substr, "yellow", 6) == 0) {
        	g_boot_state = BOOT_STATE__YELLOW;
        } else if (strncmp(substr, "red", 3) == 0) {
        	g_boot_state = BOOT_STATE__RED;
       	}
	}

	return 0;
}

static void __exit boot_state_exit()
{
	return ;
}

module_init(boot_state_init);
module_exit(boot_state_exit);
MODULE_LICENSE("GPL");
#endif/*OPPO_GUARD_GENERAL_H_*/
