/***************************************************************
** Copyright (C),  2020,  OPPO Mobile Comm Corp.,  Ltd
** VENDOR_EDIT
** File : oppo_aod.c
** Description : oppo aod feature
** Version : 1.0
** Date : 2020/04/23
** Author : Qianxu@MM.Display.LCD Driver
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**   Qianxu         2020/04/23        1.0           Build this moudle
******************************************************************/

#include "dsi_defs.h"
#include "oppo_aod.h"

int aod_light_mode = 0;
DEFINE_MUTEX(oppo_aod_light_mode_lock);

int __oppo_display_set_aod_light_mode(int mode) {
	mutex_lock(&oppo_aod_light_mode_lock);
	if(mode != aod_light_mode) {
		aod_light_mode = mode;
	}
	mutex_unlock(&oppo_aod_light_mode_lock);
	return 0;
}

int oppo_update_aod_light_mode_unlock(struct dsi_panel *panel)
{
	int rc = 0;
	enum dsi_cmd_set_type type;

	if (aod_light_mode == 1)
		type = DSI_CMD_AOD_LOW_LIGHT_MODE;
	else
		type = DSI_CMD_AOD_HIGH_LIGHT_MODE;

	rc = dsi_panel_tx_cmd_set(panel, type);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_AOD_LIGHT_MODE cmds, rc=%d\n",
		       panel->name, rc);
	}

	return rc;
}

int oppo_update_aod_light_mode(void)
{
	struct dsi_display *display = get_main_display();
	int ret = 0;

	if (!display || !display->panel) {
		printk(KERN_INFO "oppo_set_aod_light_mode and main display is null");
		return -EINVAL;
	}

	if (display->panel->is_hbm_enabled) {
		pr_err("%s error panel->is_hbm_enabled\n", __func__);
		return -EINVAL;
	}

	if (get_oppo_display_scene() != OPPO_DISPLAY_AOD_SCENE) {
		pr_err("%s error get_oppo_display_scene = %d, \n", __func__, get_oppo_display_scene());
		return -EFAULT;
	}
	mutex_lock(&display->display_lock);
	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	mutex_lock(&display->panel->panel_lock);

	if (!dsi_panel_initialized(display->panel)) {
		pr_err("dsi_panel_aod_low_light_mode is not init\n");
		ret = -EINVAL;
		goto error;
	}

	ret = oppo_update_aod_light_mode_unlock(display->panel);

	if (ret) {
		pr_err("failed to set aod light status ret=%d", ret);
		goto error;
	}

error:
	mutex_unlock(&display->panel->panel_lock);
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_ALL_CLKS, DSI_CLK_OFF);
	}
	mutex_unlock(&display->display_lock);

	return ret;
}

int oppo_panel_set_aod_light_mode(void *buf)
{
	unsigned int *temp_save = buf;

	__oppo_display_set_aod_light_mode(*temp_save);
	oppo_update_aod_light_mode();

	return 0;
}

int oppo_panel_get_aod_light_mode(void *buf)
{
	unsigned int *aod_mode = buf;
	(*aod_mode) = aod_light_mode;

	printk(KERN_INFO "oppo_get_aod_light_mode = %d\n",aod_light_mode);

	return 0;
}
