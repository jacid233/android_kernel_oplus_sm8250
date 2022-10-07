/***************************************************************
** Copyright (C),  2020,  OPPO Mobile Comm Corp.,  Ltd
** VENDOR_EDIT
** File : oppo_display_panel_common.c
** Description : oppo display panel common feature
** Version : 1.0
** Date : 2020/06/13
** Author : Li.Sheng@MULTIMEDIA.DISPLAY.LCD
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**  Li.Sheng       2020/06/13        1.0           Build this moudle
******************************************************************/
#include "oppo_display_panel_common.h"
#include <linux/notifier.h>
#include <linux/msm_drm_notify.h>
#if defined(OPLUS_FEATURE_PXLW_IRIS5)
/*Pixelworks@MULTIMEDIA.DISPLAY, 2020/06/02, Iris5 Feature*/
#include <video/mipi_display.h>
#include "../iris/dsi_iris5_api.h"
#include "../iris/dsi_iris5_lightup.h"
#include "../iris/dsi_iris5_loop_back.h"
#endif

int oppo_debug_max_brightness = 0;
int oppo_display_audio_ready = 0;
char oppo_rx_reg[PANEL_TX_MAX_BUF] = {0x0};
char oppo_rx_len = 0;
int lcd_closebl_flag = 0;
int spr_mode = 0;
int dynamic_osc_clock = 139600;
int mca_mode = 1;
extern int oppo_dimlayer_hbm;
extern int oppo_dimlayer_bl;

DEFINE_MUTEX(oplus_mca_lock);
DEFINE_MUTEX(oppo_spr_lock);

extern int msm_drm_notifier_call_chain(unsigned long val, void *v);
extern int __oppo_display_set_spr(int mode);
extern int dsi_display_spr_mode(struct dsi_display *display, int mode);

int dsi_panel_set_mca(struct dsi_panel *panel)
{
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	if (mca_mode == 1) {
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_MCA_ON);
	} else {
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_MCA_OFF);
	}

	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_HBM_ON cmds, rc=%d\n",
		       panel->name, rc);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_display_set_mca(struct dsi_display *display)
{
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_set_mca(display->panel);
	if (rc) {
		pr_err("[%s] failed to dsi_panel_set_mca, rc=%d\n",
			display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int __oplus_display_set_mca(int mode)
{
	mutex_lock(&oplus_mca_lock);
	if (mode != mca_mode) {
		mca_mode = mode;
	}
	mutex_unlock(&oplus_mca_lock);
	return 0;
}

int dsi_panel_spr_mode(struct dsi_panel *panel, int mode)
{
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	switch (mode) {
	case 0:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SPR_MODE0);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SPR_MODE0 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	case 1:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SPR_MODE1);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SPR_MODE1 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	case 2:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SPR_MODE2);
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SEED_MODE2 cmds, rc=%d\n",
					panel->name, rc);
		}
		break;
	default:
		rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_SPR_MODE0);  /*gaos edit default is spr mode 0*/
		if (rc) {
			pr_err("[%s] failed to send DSI_CMD_SPR_MODE0 cmds, rc=%d\n",
					panel->name, rc);
		}
		pr_err("[%s] seed mode Invalid %d\n",
			panel->name, mode);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_display_spr_mode(struct dsi_display *display, int mode) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_spr_mode(display->panel, mode);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_spr_on, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int __oppo_display_set_spr(int mode) {
	mutex_lock(&oppo_spr_lock);
	if(mode != spr_mode) {
		spr_mode = mode;
	}
	mutex_unlock(&oppo_spr_lock);
	return 0;
}

int dsi_panel_read_panel_reg(struct dsi_display_ctrl *ctrl, struct dsi_panel *panel, u8 cmd, void *rbuf,  size_t len)
{
	int rc = 0;
	struct dsi_cmd_desc cmdsreq;
	u32 flags = 0;

	if (!panel || !ctrl || !ctrl->ctrl) {
		return -EINVAL;
	}

	if (!dsi_ctrl_validate_host_state(ctrl->ctrl)) {
		return 1;
	}

	/* acquire panel_lock to make sure no commands are in progress */
	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	memset(&cmdsreq, 0x0, sizeof(cmdsreq));
	cmdsreq.msg.type = 0x06;
	cmdsreq.msg.tx_buf = &cmd;
	cmdsreq.msg.tx_len = 1;
	cmdsreq.msg.rx_buf = rbuf;
	cmdsreq.msg.rx_len = len;
	cmdsreq.msg.flags |= MIPI_DSI_MSG_LASTCOMMAND;
	flags |= (DSI_CTRL_CMD_FETCH_MEMORY | DSI_CTRL_CMD_READ |
		DSI_CTRL_CMD_CUSTOM_DMA_SCHED |
		DSI_CTRL_CMD_LAST_COMMAND);

	rc = dsi_ctrl_cmd_transfer(ctrl->ctrl, &cmdsreq.msg, &flags);
	if (rc <= 0) {
		pr_err("%s, dsi_display_read_panel_reg rx cmd transfer failed rc=%d\n",
			__func__,
			rc);
		goto error;
	}
error:
	/* release panel_lock */
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_display_read_panel_reg(struct dsi_display *display, u8 cmd, void *data, size_t len) {
	int rc = 0;
	struct dsi_display_ctrl *m_ctrl;
	if (!display || !display->panel || data == NULL) {
		pr_err("%s, Invalid params\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

	m_ctrl = &display->ctrl[display->cmd_master_idx];

	if (display->tx_cmd_buf == NULL) {
		rc = dsi_host_alloc_cmd_tx_buffer(display);
		if (rc) {
			pr_err("%s, failed to allocate cmd tx buffer memory\n", __func__);
			goto done;
		}
	}

	rc = dsi_display_cmd_engine_enable(display);
	if (rc) {
		pr_err("%s, cmd engine enable failed\n", __func__);
		goto done;
	}

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_ALL_CLKS, DSI_CLK_ON);
	}

	rc = dsi_panel_read_panel_reg(m_ctrl, display->panel, cmd, data, len);
	if (rc < 0) {
		pr_err("%s, [%s] failed to read panel register, rc=%d,cmd=%d\n",
		       __func__,
		       display->name,
		       rc,
		       cmd);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
					  DSI_ALL_CLKS, DSI_CLK_OFF);
	}

	dsi_display_cmd_engine_disable(display);

done:
	mutex_unlock(&display->display_lock);
	pr_err("%s, return: %d\n", __func__, rc);
	return rc;
}

int oppo_display_panel_get_id(void *buf)
{
	struct dsi_display *display = get_main_display();
	int ret = 0;
	unsigned char read[30];
	struct panel_id *panel_rid = buf;

	if(get_oppo_display_power_status() == OPPO_DISPLAY_POWER_ON) {
		if(display == NULL) {
			printk(KERN_INFO "oppo_display_get_panel_id and main display is null");
			ret = -1;
			return ret;
		}

		ret = dsi_display_read_panel_reg(display, 0xDA, read, 1);
		if (ret < 0) {
			pr_err("failed to read DA ret=%d\n", ret);
			return -EINVAL;
		}
		panel_rid->DA = (uint32_t)read[0];

		ret = dsi_display_read_panel_reg(display, 0xDB, read, 1);
		if (ret < 0) {
			pr_err("failed to read DB ret=%d\n", ret);
			return -EINVAL;
		}
		panel_rid->DB = (uint32_t)read[0];

		ret = dsi_display_read_panel_reg(display, 0xDC, read, 1);
		if (ret < 0) {
			pr_err("failed to read DC ret=%d\n", ret);
			return -EINVAL;
		}
		panel_rid->DC = (uint32_t)read[0];
	} else {
		printk(KERN_ERR	 "%s oppo_display_get_panel_id, but now display panel status is not on\n", __func__);
		return -EINVAL;
	}

	return ret;
}

int oppo_display_panel_get_max_brightness(void *buf)
{
	uint32_t *max_brightness = buf;
	struct dsi_display *display = get_main_display();
	struct dsi_panel *panel = display->panel;

	if (oppo_debug_max_brightness == 0) {
		(*max_brightness) = panel->bl_config.bl_normal_max_level;
	} else {
		(*max_brightness) = oppo_debug_max_brightness;
	}

	return 0;
}

int oppo_display_panel_set_max_brightness(void *buf)
{
	uint32_t *max_brightness = buf;

	oppo_debug_max_brightness = (*max_brightness);

	return 0;
}

int oppo_display_panel_get_vendor(void *buf)
{
	struct panel_info *p_info = buf;
	struct dsi_display *display = get_main_display();
	char *vendor = (char *)display->panel->oppo_priv.vendor_name;
	char *manu_name = (char *)display->panel->oppo_priv.manufacture_name;

	if (!display || !display->panel ||
	    !display->panel->oppo_priv.vendor_name ||
	    !display->panel->oppo_priv.manufacture_name) {
		pr_err("failed to config lcd proc device");
		return -EINVAL;
	}

	memcpy(p_info->version, vendor, strlen(vendor) > 31?31:(strlen(vendor)+1));
	memcpy(p_info->manufacture, manu_name, strlen(manu_name) > 31?31:(strlen(manu_name)+1));

	return 0;
}

int oppo_display_panel_get_ccd_check(void *buf)
{
	struct dsi_display *display = get_main_display();
	struct mipi_dsi_device *mipi_device;
	int rc = 0;
	unsigned int *ccd_check = buf;
	if (!display || !display->panel) {
		pr_err("failed for: %s %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	if (get_oppo_display_power_status() != OPPO_DISPLAY_POWER_ON) {
		printk(KERN_ERR"%s display panel in off status\n", __func__);
		return -EFAULT;
	}

	if (display->panel->panel_mode != DSI_OP_CMD_MODE) {
		pr_err("only supported for command mode\n");
		return -EFAULT;
	}

	if (!(display && display->panel->oppo_priv.vendor_name) ||
		!strcmp(display->panel->oppo_priv.vendor_name, "NT37800")) {
		(*ccd_check) = 0;
		goto end;
	}

	mipi_device = &display->panel->mipi_device;

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	if (!dsi_panel_initialized(display->panel)) {
		rc = -EINVAL;
		goto unlock;
	}

	rc = dsi_display_cmd_engine_enable(display);
	if (rc) {
		pr_err("%s, cmd engine enable failed\n", __func__);
		goto unlock;
	}

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	if (!strcmp(display->panel->oppo_priv.vendor_name, "AMB655UV01")) {
		{
			char value[] = { 0x5A, 0x5A };
			rc = mipi_dsi_dcs_write(mipi_device, 0xF0, value, sizeof(value));
		}
		{
			char value[] = { 0x44, 0x50 };
			rc = mipi_dsi_dcs_write(mipi_device, 0xE7, value, sizeof(value));
		}
		usleep_range(1000, 1100);
		{
			char value[] = { 0x03 };
			rc = mipi_dsi_dcs_write(mipi_device, 0xB0, value, sizeof(value));
		}
	} else {
		{
			char value[] = { 0x5A, 0x5A };
			rc = mipi_dsi_dcs_write(mipi_device, 0xF0, value, sizeof(value));
		}
		{
			char value[] = { 0x02 };
			rc = mipi_dsi_dcs_write(mipi_device, 0xB0, value, sizeof(value));
		}
		{
			char value[] = { 0x44, 0x50 };
			rc = mipi_dsi_dcs_write(mipi_device, 0xCC, value, sizeof(value));
		}
		usleep_range(1000, 1100);
		{
			char value[] = { 0x05 };
			rc = mipi_dsi_dcs_write(mipi_device, 0xB0, value, sizeof(value));
		}
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}
	dsi_display_cmd_engine_disable(display);

	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);
	if (!strcmp(display->panel->oppo_priv.vendor_name, "AMB655UV01")) {
		{
			unsigned char read[10];

			rc = dsi_display_read_panel_reg(display, 0xE1, read, 1);

			pr_err("read ccd_check value = 0x%x rc=%d\n", read[0], rc);
			(*ccd_check) = read[0];
		}
	} else {
		{
			unsigned char read[10];

			rc = dsi_display_read_panel_reg(display, 0xCC, read, 1);

			pr_err("read ccd_check value = 0x%x rc=%d\n", read[0], rc);
			(*ccd_check) = read[0];
		}
	}

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	if (!dsi_panel_initialized(display->panel)) {
		rc = -EINVAL;
		goto unlock;
	}

	rc = dsi_display_cmd_engine_enable(display);
	if (rc) {
		pr_err("%s, cmd engine enable failed\n", __func__);
		goto unlock;
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_ON);
	}

	{
		char value[] = { 0xA5, 0xA5 };
		rc = mipi_dsi_dcs_write(mipi_device, 0xF0, value, sizeof(value));
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}

	dsi_display_cmd_engine_disable(display);
unlock:

	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);
end:
	pr_err("[%s] ccd_check = %d\n",  display->panel->oppo_priv.vendor_name, (*ccd_check));
	return 0;
}

int oppo_display_panel_get_serial_number(void *buf)
{
	int ret = 0, i;
	unsigned char read[30] = {0};
	struct panel_serial_number* panel_rnum = buf;
	struct dsi_display *display = get_main_display();
	unsigned base_index = 10;

	panel_rnum->date = 0;
	panel_rnum->precise_time = 0;

	if (!display) {
		printk(KERN_INFO "oppo_display_get_panel_serial_number and main display is null");
		return -1;
	}

	if (get_oppo_display_power_status() != OPPO_DISPLAY_POWER_ON) {
		printk(KERN_ERR"%s display panel in off status\n", __func__);
		return ret;
	}

	/*
	 * for some unknown reason, the panel_serial_info may read dummy,
	 * retry when found panel_serial_info is abnormal.
	 */
	for (i = 0;i < 10; i++) {
		ret = dsi_display_read_panel_reg(get_main_display(), 0xA1, read, 17);
		if (ret < 0) {
			ret = scnprintf(buf, PAGE_SIZE,
					"Get panel serial number failed, reason:%d", ret);
			msleep(20);
			continue;
		}

		/*  0xA1			   11th		12th	13th	14th	15th
		 *  HEX				0x32		0x0C	0x0B	0x29	0x37
		 *  Bit		   [D7:D4][D3:D0] [D5:D0] [D5:D0] [D5:D0] [D5:D0]
		 *  exp			  3	  2	   C	   B	   29	  37
		 *  Yyyy,mm,dd	  2014   2m	  12d	 11h	 41min   55sec
		 *  panel_rnum.data[24:21][20:16] [15:8]  [7:0]
		 *  panel_rnum:precise_time					  [31:24] [23:16] [reserved]
		*/
		panel_rnum->date =
			(((read[base_index] & 0xF0) << 20)
			|((read[base_index] & 0x0F) << 16)
			|((read[base_index+1] & 0x1F) << 8)
			|(read[base_index+2] & 0x1F));

		panel_rnum->precise_time =
			(((read[base_index+3] & 0x3F) << 24)
			|((read[base_index+4] & 0x3F) << 16)
			|(read[base_index+5] << 8)
			|(read[base_index+6]));

		if (!read[base_index]) {
			/*
			 * the panel we use always large than 2011, so
			 * force retry when year is 2011
			 */
			msleep(20);
			continue;
		}

		break;
	}

	return 0;
}

int oplus_display_set_audio_ready(void *data) {
	uint32_t *audio_ready = data;

	oppo_display_audio_ready = (*audio_ready);
	printk("%s oppo_display_audio_ready = %d\n", __func__, oppo_display_audio_ready);

	return 0;
}

int oplus_display_dump_info(void *data) {
	int ret = 0;
	struct dsi_display * temp_display;
	struct display_timing_info *timing_info = data;

	temp_display = get_main_display();

	if (temp_display == NULL) {
		printk(KERN_INFO "oppo_display_dump_info and main display is null");
		ret = -1;
		return ret;
	}

	if(temp_display->modes == NULL) {
		printk(KERN_INFO "oppo_display_dump_info and display modes is null");
		ret = -1;
		return ret;
	}

	timing_info->h_active = temp_display->modes->timing.h_active;
	timing_info->v_active = temp_display->modes->timing.v_active;
	timing_info->refresh_rate = temp_display->modes->timing.refresh_rate;
	timing_info->clk_rate_hz_l32 = (uint32_t)(temp_display->modes->timing.clk_rate_hz & 0x00000000FFFFFFFF);
	timing_info->clk_rate_hz_h32 = (uint32_t)(temp_display->modes->timing.clk_rate_hz >> 32);

	return 0;
}

int oplus_display_get_panel_dsc(void *data) {
	int ret = 0;
	uint32_t *reg_read = data;
	unsigned char read[30];

	if(get_oppo_display_power_status() == OPPO_DISPLAY_POWER_ON) {
		if(get_main_display() == NULL) {
			printk(KERN_INFO "oppo_display_get_panel_dsc and main display is null");
			ret = -1;
			return ret;
		}

		ret = dsi_display_read_panel_reg(get_main_display(), 0x03, read, 1);
		if (ret < 0) {
			printk(KERN_ERR  "%s read panel dsc reg error = %d!\n", __func__, ret);
			ret = -1;
		} else {
			(*reg_read) = read[0];
			ret = 0;
		}
	} else {
		printk(KERN_ERR	 "%s but now display panel status is not on\n", __func__);
		ret = -1;
	}

	return ret;
}

int oplus_display_get_closebl_flag(void *data)
{
	uint32_t *closebl_flag = data;

	(*closebl_flag) = lcd_closebl_flag;
	printk(KERN_INFO "oppo_display_get_closebl_flag = %d\n", lcd_closebl_flag);

	return 0;
}

int oplus_display_set_closebl_flag(void *data)
{
	uint32_t *closebl = data;

	pr_err("lcd_closebl_flag = %d\n", (*closebl));
	if (1 != (*closebl))
		lcd_closebl_flag = 0;
	pr_err("oppo_display_set_closebl_flag = %d\n", lcd_closebl_flag);

	return 0;
}

#if defined(OPLUS_FEATURE_PXLW_IRIS5)
/* Pixelworks@MULTIMEDIA.DISPLAY, 2020/06/02, Iris5 Feature*/
int iris_panel_dcs_type_set(struct dsi_cmd_desc *cmd, void *data, size_t len)
{
	switch (len) {
	case 0:
		return -EINVAL;
	case 1:
		cmd->msg.type = MIPI_DSI_DCS_SHORT_WRITE;
		break;
	case 2:
		cmd->msg.type = MIPI_DSI_DCS_SHORT_WRITE_PARAM;
		break;
	default:
		cmd->msg.type = MIPI_DSI_DCS_LONG_WRITE;
		break;
	}
	cmd->msg.tx_len = len;
	cmd->msg.tx_buf = data;
	return 0;
}

int iris_panel_dcs_write_wrapper(struct dsi_panel *panel, void *data, size_t len)
{
	int rc = 0;
	struct dsi_panel_cmd_set cmdset;
	struct dsi_cmd_desc dsi_cmd =
		{{0, MIPI_DSI_DCS_SHORT_WRITE, 0, 0, 0, 0, NULL, 0, NULL}, 1, 0};

	memset(&cmdset, 0x00, sizeof(cmdset));
	cmdset.cmds = &dsi_cmd;
	cmdset.count = 1;
	rc = iris_panel_dcs_type_set(&dsi_cmd, data, len);
	if (rc < 0) {
		pr_err("%s: invalid dsi cmd len\n", __func__);
		return rc;
	}

	rc = iris5_panel_cmd_passthrough(panel, &cmdset);
	if (rc < 0) {
		pr_err("%s: send panel command failed\n", __func__);
		return rc;
	}
	return rc;
}

int iris_panel_dcs_read_wrapper(struct dsi_display *display, u8 cmd, void *rbuf, size_t rlen)
{
	int rc = 0;
	struct dsi_panel_cmd_set cmdset;
	struct dsi_cmd_desc dsi_cmd =
		{{0, MIPI_DSI_DCS_READ, MIPI_DSI_MSG_REQ_ACK, 0, 0, 1, &cmd, rlen, rbuf}, 1, 0};
	struct dsi_panel *panel;

	if (!display || !display->panel) {
		pr_err("%s, Invalid params\n", __func__);
		return -EINVAL;
	}

	memset(&cmdset, 0x00, sizeof(cmdset));
	cmdset.cmds = &dsi_cmd;
	cmdset.count = 1;

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_ALL_CLKS, DSI_CLK_ON);
	}

	panel = display->panel;
	mutex_lock(&display->display_lock);
	mutex_lock(&panel->panel_lock);

	rc = iris5_panel_cmd_passthrough(panel, &cmdset);
	mutex_unlock(&panel->panel_lock);
	mutex_unlock(&display->display_lock);

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_ALL_CLKS, DSI_CLK_OFF);
	}

	if (rc < 0) {
		pr_err("%s, [%s] failed to read panel register, rc=%d,cmd=%d\n",
		       __func__,
		       display->name,
		       rc,
		       cmd);
		return rc;
	}
	pr_err("%s, return: %d\n", __func__, rc);
	return rc;
}
#endif

int oplus_display_get_panel_reg(void *data) {
	struct dsi_display *display = get_main_display();
	struct panel_reg_get *panel_reg = data;
	uint32_t u32_bytes = sizeof(uint32_t)/sizeof(char);

	if (!display) {
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

	memcpy(panel_reg->reg_rw, oppo_rx_reg, oppo_rx_len);
	panel_reg->lens = (oppo_rx_len%u32_bytes) == 0 ? (oppo_rx_len/u32_bytes) : ((oppo_rx_len/u32_bytes) + 1);

	mutex_unlock(&display->display_lock);

	return 0;
}

int oplus_display_set_panel_reg(void *data) {
	char reg[PANEL_TX_MAX_BUF] = {0x0};
	char payload[PANEL_TX_MAX_BUF] = {0x0};
	u32 index = 0, value = 0;
	int ret = 0;
	int len = 0;
	struct dsi_display *display = get_main_display();
	struct panel_reg_rw *reg_rw = data;

	if (!display || !display->panel) {
		pr_err("debug for: %s %d\n", __func__, __LINE__);
		return -EFAULT;
	}

	if (reg_rw->lens > PANEL_REG_MAX_LENS) {
		pr_err("error: wrong input reg len\n");
		return -EINVAL;
	}

	if (reg_rw->rw_flags == REG_READ) {
		value = reg_rw->cmd;
		len = reg_rw->lens;

#if defined(OPLUS_FEATURE_PXLW_IRIS5)
		if (iris_get_feature() && (iris5_abypass_mode_get(get_main_display()->panel) == PASS_THROUGH_MODE))
			iris_panel_dcs_read_wrapper(get_main_display(), value, reg, len);
		else
			dsi_display_read_panel_reg(get_main_display(), value, reg, len);
#else
		dsi_display_read_panel_reg(get_main_display(), value, reg, len);
#endif

		for (index; index < len; index++) {
			printk("%x ", reg[index]);
		}
		mutex_lock(&display->display_lock);
		memcpy(oppo_rx_reg, reg, PANEL_TX_MAX_BUF);
		oppo_rx_len = len;
		mutex_unlock(&display->display_lock);
		return 0;
	}

	if (reg_rw->rw_flags == REG_WRITE) {
		memcpy(payload, reg_rw->value, reg_rw->lens);
		reg[0] = reg_rw->cmd;
		for (index; index < len; index++) {
			reg[index + 1] = payload[index];
		}

		if(get_oppo_display_power_status() == OPPO_DISPLAY_POWER_ON) {
				/* enable the clk vote for CMD mode panels */
			mutex_lock(&display->display_lock);
			mutex_lock(&display->panel->panel_lock);

			if (display->panel->panel_initialized) {
				if (display->config.panel_mode == DSI_OP_CMD_MODE) {
					dsi_display_clk_ctrl(display->dsi_clk_handle,
							DSI_ALL_CLKS, DSI_CLK_ON);
				}
#if defined(OPLUS_FEATURE_PXLW_IRIS5)
				if (iris_get_feature() && (iris5_abypass_mode_get(display->panel) == PASS_THROUGH_MODE))
					ret = iris_panel_dcs_write_wrapper(display->panel, reg, len+1);
				else
					ret = mipi_dsi_dcs_write(&display->panel->mipi_device, reg[0],
								 payload, len);
#else
				ret = mipi_dsi_dcs_write(&display->panel->mipi_device, reg[0],
							 payload, len);
#endif

				if (display->config.panel_mode == DSI_OP_CMD_MODE) {
					dsi_display_clk_ctrl(display->dsi_clk_handle,
							DSI_ALL_CLKS, DSI_CLK_OFF);
				}
			}

			mutex_unlock(&display->panel->panel_lock);
			mutex_unlock(&display->display_lock);

			if (ret < 0) {
				return ret;
			}
		}
		return 0;
	}
	printk("%s error: please check the args!\n", __func__);
	return -1;
}

int oplus_display_notify_panel_blank(void *data) {
	struct msm_drm_notifier notifier_data;
	int blank;
	uint32_t *temp_save_user = data;
	int temp_save = (*temp_save_user);

	printk(KERN_INFO "%s oppo_display_notify_panel_blank = %d\n", __func__, temp_save);

	if(temp_save == 1) {
		blank = MSM_DRM_BLANK_UNBLANK;
		notifier_data.data = &blank;
		notifier_data.id = 0;
		msm_drm_notifier_call_chain(MSM_DRM_EARLY_EVENT_BLANK,
						   &notifier_data);
		msm_drm_notifier_call_chain(MSM_DRM_EVENT_BLANK,
						   &notifier_data);
	} else if (temp_save == 0) {
		blank = MSM_DRM_BLANK_POWERDOWN;
		notifier_data.data = &blank;
		notifier_data.id = 0;
		msm_drm_notifier_call_chain(MSM_DRM_EARLY_EVENT_BLANK,
						   &notifier_data);
	}
	return 0;
}

int oplus_display_get_spr(void *data) {
	uint32_t *spr_mode_user = data;

	printk(KERN_INFO "oppo_display_get_spr = %d\n", spr_mode);
	*spr_mode_user = spr_mode;

	return 0;
}

int oplus_display_set_spr(void *data) {
	uint32_t *temp_save_user = data;
	int temp_save = (*temp_save_user);

	printk(KERN_INFO "%s oppo_display_set_spr = %d\n", __func__, temp_save);

	__oppo_display_set_spr(temp_save);
	if(get_oppo_display_power_status() == OPPO_DISPLAY_POWER_ON) {
		if(get_main_display() == NULL) {
			printk(KERN_INFO "oppo_display_set_spr and main display is null");
			return 0;
		}

		dsi_display_spr_mode(get_main_display(), spr_mode);
	} else {
		printk(KERN_ERR	 "%s oppo_display_set_spr = %d, but now display panel status is not on\n", __func__, temp_save);
	}
	return 0;
}

int oplus_display_get_roundcorner(void *data)
{
	uint32_t *round_corner = data;
	struct dsi_display *display = get_main_display();
	bool roundcorner = true;

	if (display && display->name &&
	    !strcmp(display->name, "qcom,mdss_dsi_oppo19101boe_nt37800_1080_2400_cmd"))
		roundcorner = false;

	*round_corner = roundcorner;

	return 0;
}

int oplus_display_get_dynamic_osc_clock(void *data)
{
	struct dsi_display *display = get_main_display();
	uint32_t *osc_clock = data;

	if (!display) {
		pr_err("failed for: %s %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

	*osc_clock = dynamic_osc_clock;
	pr_debug("%s: read dsi clk rate %d\n", __func__,
			dynamic_osc_clock);

	mutex_unlock(&display->display_lock);

	return 0;
}

int oplus_display_set_dynamic_osc_clock(void *data)
{
	struct dsi_display *display = get_main_display();
	uint32_t *osc_clk_user = data;
	int osc_clk = *osc_clk_user;
	int rc = 0;

	if (!display) {
		pr_err("failed for: %s %d\n", __func__, __LINE__);
		return -EINVAL;
	}

	if(get_oppo_display_power_status() != OPPO_DISPLAY_POWER_ON) {
		printk(KERN_ERR"%s display panel in off status\n", __func__);
		return -EFAULT;
	}

	if (display->panel->panel_mode != DSI_OP_CMD_MODE) {
		pr_err("only supported for command mode\n");
		return -EFAULT;
	}

	pr_info("%s: osc clk param value: '%d'\n", __func__, osc_clk);

	mutex_lock(&display->display_lock);
	mutex_lock(&display->panel->panel_lock);

	if (!dsi_panel_initialized(display->panel)) {
		rc = -EINVAL;
		goto unlock;
	}

	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_ON);
	}

	if (osc_clk == 139600) {
		rc = dsi_panel_tx_cmd_set(display->panel, DSI_CMD_OSC_CLK_MODEO0);
	} else {
		rc = dsi_panel_tx_cmd_set(display->panel, DSI_CMD_OSC_CLK_MODEO1);
	}
	if (rc)
		pr_err("Failed to configure osc dynamic clk\n");

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}
	dynamic_osc_clock = osc_clk;

unlock:
	mutex_unlock(&display->panel->panel_lock);
	mutex_unlock(&display->display_lock);

	return rc;
}
