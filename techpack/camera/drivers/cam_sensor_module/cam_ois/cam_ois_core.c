// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 */

#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/dma-contiguous.h>
#include <cam_sensor_cmn_header.h>
#include "cam_ois_core.h"
#include "cam_ois_soc.h"
#include "cam_sensor_util.h"
#include "cam_debug_util.h"
#include "cam_res_mgr_api.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"
#ifdef VENDOR_EDIT
#include "onsemi_fw/fw_download_interface.h"

/*add by hongbo.dai@Camera 20181215, for OIS bu63169*/
#define MODE_NOCONTINUE 1
#define MODE_CONTINUE 0
/*hongbo.dai@Camera.Drv, 2018/12/26, modify for [oppo ois]*/
#define MAX_LENGTH 160
#define CAMX_HALL_MAX_NUMBER 100

struct cam_sensor_i2c_reg_setting_array {
	struct cam_sensor_i2c_reg_array reg_setting[512];
	unsigned short size;
	enum camera_sensor_i2c_type addr_type;
	enum camera_sensor_i2c_type data_type;
	unsigned short delay;
};


struct cam_sensor_i2c_reg_setting_array bu63169_pll_settings = {
    .reg_setting =
	{
		{.reg_addr = 0x8262, .reg_data = 0xFF02, .delay = 0x00, .data_mask = 0x00}, \
		{.reg_addr = 0x8263, .reg_data = 0x9F05, .delay = 0x01, .data_mask = 0x00}, \
		{.reg_addr = 0x8264, .reg_data = 0x6040, .delay = 0x00, .data_mask = 0x00}, \
		{.reg_addr = 0x8260, .reg_data = 0x1130, .delay = 0x00, .data_mask = 0X00}, \
		{.reg_addr = 0x8265, .reg_data = 0x8000, .delay = 0x00, .data_mask = 0x00}, \
		{.reg_addr = 0x8261, .reg_data = 0x0280, .delay = 0x00, .data_mask = 0x00}, \
		{.reg_addr = 0x8261, .reg_data = 0x0380, .delay = 0x00, .data_mask = 0x00}, \
		{.reg_addr = 0x8261, .reg_data = 0x0988, .delay = 0x00, .data_mask = 0X00}, \
	},
    .size = 8,
    .addr_type = CAMERA_SENSOR_I2C_TYPE_WORD,
    .data_type = CAMERA_SENSOR_I2C_TYPE_WORD,
    .delay = 1,
};
static int RamWriteByte(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t addr, uint32_t data, unsigned short mdelay)
{
	int32_t rc = 0;
	int retry = 3;
	int i = 0;
	struct cam_sensor_i2c_reg_array i2c_write_setting = {
		.reg_addr = addr,
		.reg_data = data,
		.delay = mdelay,
		.data_mask = 0x00,
	};
	struct cam_sensor_i2c_reg_setting i2c_write = {
		.reg_setting = &i2c_write_setting,
		.size = 1,
		.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD,
		.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE,
		.delay = mdelay,
	};
	if (o_ctrl == NULL) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	for(i = 0; i < retry; i++)
	{
		rc = camera_io_dev_write(&(o_ctrl->io_master_info), &i2c_write);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "write 0x%04x failed, retry:%d", addr, i+1);
		} else {
			return rc;
		}
	}
	return rc;
}
static int RamWriteWord(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t addr, uint32_t data)
{
	int32_t rc = 0;
	int retry = 1;
	int i = 0;
	struct cam_sensor_i2c_reg_array i2c_write_setting = {
		.reg_addr = addr,
		.reg_data = data,
		.delay = 0x00,
		.data_mask = 0x00,
	};
	struct cam_sensor_i2c_reg_setting i2c_write = {
		.reg_setting = &i2c_write_setting,
		.size = 1,
		.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD,
		.data_type = CAMERA_SENSOR_I2C_TYPE_WORD,
		.delay = 0x00,
	};

	if (addr == 0x8c) {
		i2c_write.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
		i2c_write.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	}

	if (o_ctrl == NULL) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	for(i = 0; i < retry; i++)
	{
		rc = camera_io_dev_write(&(o_ctrl->io_master_info), &i2c_write);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "write 0x%x = 0x%0x failed, retry:%d !!!", addr, data, i+1);
		} else {
			CAM_DBG(CAM_OIS, "write 0x%x = 0x%0x", addr,data);
			return rc;
		}
	}
	return rc;
}
static int RamMultiWrite(struct cam_ois_ctrl_t *o_ctrl,
	struct cam_sensor_i2c_reg_setting *write_setting) {
	int rc = 0;
	int i = 0;
	for (i = 0; i < write_setting->size; i++) {
		rc = RamWriteWord(o_ctrl, write_setting->reg_setting[i].reg_addr,
			write_setting->reg_setting[i].reg_data);
	}
	return rc;
}

#endif
int32_t cam_ois_construct_default_power_setting(
	struct cam_sensor_power_ctrl_t *power_info)
{
	int rc = 0;

	power_info->power_setting_size = 1;
	power_info->power_setting =
		kzalloc(sizeof(struct cam_sensor_power_setting),
			GFP_KERNEL);
	if (!power_info->power_setting)
		return -ENOMEM;

	power_info->power_setting[0].seq_type = SENSOR_VAF;
	power_info->power_setting[0].seq_val = CAM_VAF;
	power_info->power_setting[0].config_val = 1;
	power_info->power_setting[0].delay = 2;

	power_info->power_down_setting_size = 1;
	power_info->power_down_setting =
		kzalloc(sizeof(struct cam_sensor_power_setting),
			GFP_KERNEL);
	if (!power_info->power_down_setting) {
		rc = -ENOMEM;
		goto free_power_settings;
	}

	power_info->power_down_setting[0].seq_type = SENSOR_VAF;
	power_info->power_down_setting[0].seq_val = CAM_VAF;
	power_info->power_down_setting[0].config_val = 0;

	return rc;

free_power_settings:
	kfree(power_info->power_setting);
	power_info->power_setting = NULL;
	power_info->power_setting_size = 0;
	return rc;
}


/**
 * cam_ois_get_dev_handle - get device handle
 * @o_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
static int cam_ois_get_dev_handle(struct cam_ois_ctrl_t *o_ctrl,
	void *arg)
{
	struct cam_sensor_acquire_dev    ois_acq_dev;
	struct cam_create_dev_hdl        bridge_params;
	struct cam_control              *cmd = (struct cam_control *)arg;

	if (o_ctrl->bridge_intf.device_hdl != -1) {
		CAM_ERR(CAM_OIS, "Device is already acquired");
		return -EFAULT;
	}
	if (copy_from_user(&ois_acq_dev, u64_to_user_ptr(cmd->handle),
		sizeof(ois_acq_dev)))
		return -EFAULT;

	bridge_params.session_hdl = ois_acq_dev.session_handle;
	bridge_params.ops = &o_ctrl->bridge_intf.ops;
	bridge_params.v4l2_sub_dev_flag = 0;
	bridge_params.media_entity_flag = 0;
	bridge_params.priv = o_ctrl;

	ois_acq_dev.device_handle =
		cam_create_device_hdl(&bridge_params);
	o_ctrl->bridge_intf.device_hdl = ois_acq_dev.device_handle;
	o_ctrl->bridge_intf.session_hdl = ois_acq_dev.session_handle;

	CAM_DBG(CAM_OIS, "Device Handle: %d", ois_acq_dev.device_handle);
	if (copy_to_user(u64_to_user_ptr(cmd->handle), &ois_acq_dev,
		sizeof(struct cam_sensor_acquire_dev))) {
		CAM_ERR(CAM_OIS, "ACQUIRE_DEV: copy to user failed");
		return -EFAULT;
	}
	return 0;
}

static int cam_ois_power_up(struct cam_ois_ctrl_t *o_ctrl)
{
	int                             rc = 0;
	struct cam_hw_soc_info          *soc_info =
		&o_ctrl->soc_info;
	struct cam_ois_soc_private *soc_private;
	struct cam_sensor_power_ctrl_t  *power_info;

	soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;

	if ((power_info->power_setting == NULL) &&
		(power_info->power_down_setting == NULL)) {
		CAM_INFO(CAM_OIS,
			"Using default power settings");
		rc = cam_ois_construct_default_power_setting(power_info);
		if (rc < 0) {
			CAM_ERR(CAM_OIS,
				"Construct default ois power setting failed.");
			return rc;
		}
	}

	/* Parse and fill vreg params for power up settings */
	rc = msm_camera_fill_vreg_params(
		soc_info,
		power_info->power_setting,
		power_info->power_setting_size);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"failed to fill vreg params for power up rc:%d", rc);
		return rc;
	}

	/* Parse and fill vreg params for power down settings*/
	rc = msm_camera_fill_vreg_params(
		soc_info,
		power_info->power_down_setting,
		power_info->power_down_setting_size);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"failed to fill vreg params for power down rc:%d", rc);
		return rc;
	}

	power_info->dev = soc_info->dev;

	rc = cam_sensor_core_power_up(power_info, soc_info);
	if (rc) {
		CAM_ERR(CAM_OIS, "failed in ois power up rc %d", rc);
		return rc;
	}

	rc = camera_io_init(&o_ctrl->io_master_info);
	if (rc)
		CAM_ERR(CAM_OIS, "cci_init failed: rc: %d", rc);

	InitOIS(o_ctrl);

	return rc;
}

/**
 * cam_ois_power_down - power down OIS device
 * @o_ctrl:     ctrl structure
 *
 * Returns success or failure
 */
static int cam_ois_power_down(struct cam_ois_ctrl_t *o_ctrl)
{
	int32_t                         rc = 0;
	struct cam_sensor_power_ctrl_t  *power_info;
	struct cam_hw_soc_info          *soc_info =
		&o_ctrl->soc_info;
	struct cam_ois_soc_private *soc_private;

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "failed: o_ctrl %pK", o_ctrl);
		return -EINVAL;
	}

	soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;
	soc_info = &o_ctrl->soc_info;

	if (!power_info) {
		CAM_ERR(CAM_OIS, "failed: power_info %pK", power_info);
		return -EINVAL;
	}
#ifdef VENDOR_EDIT
	/*we need to exit poll thread befor power down*/
	mutex_lock(&(o_ctrl->ois_poll_thread_mutex));
	forceExitpoll(o_ctrl);
	rc = cam_sensor_util_power_down(power_info, soc_info);
	mutex_unlock(&(o_ctrl->ois_poll_thread_mutex));
#else
	rc = cam_sensor_util_power_down(power_info, soc_info);
#endif
	if (rc) {
		CAM_ERR(CAM_OIS, "power down the core is failed:%d", rc);
		return rc;
	}

	camera_io_release(&o_ctrl->io_master_info);

	DeinitOIS(o_ctrl);

	return rc;
}

static int cam_ois_apply_settings(struct cam_ois_ctrl_t *o_ctrl,
	struct i2c_settings_array *i2c_set)
{
	struct i2c_settings_list *i2c_list;
	int32_t rc = 0;
	uint32_t i, size;
#ifdef VENDOR_EDIT
	/*add by hongbo.dai@camera 20181219, for OIS*/
	int mode = MODE_CONTINUE;
#endif

	if (o_ctrl == NULL || i2c_set == NULL) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	if (i2c_set->is_settings_valid != 1) {
		CAM_ERR(CAM_OIS, " Invalid settings");
		return -EINVAL;
	}
#ifdef VENDOR_EDIT
	/*add by hongbo.dai@camera 20181219, for bu63139 OIS*/
	if (strstr(o_ctrl->ois_name, "bu63169")) {
		mode = MODE_NOCONTINUE;
	}
#endif

	list_for_each_entry(i2c_list,
		&(i2c_set->list_head), list) {
		if (i2c_list->op_code ==  CAM_SENSOR_I2C_WRITE_RANDOM) {
#ifdef VENDOR_EDIT
			/*add by hongbo.dai@camera 20181219, for OIS*/
			if (mode == MODE_CONTINUE) {
				rc = camera_io_dev_write(&(o_ctrl->io_master_info),
					&(i2c_list->i2c_settings));
			} else {
				rc = RamMultiWrite(o_ctrl, &(i2c_list->i2c_settings));
			}
#else
			rc = camera_io_dev_write(&(o_ctrl->io_master_info),
				&(i2c_list->i2c_settings));
#endif
			if (rc < 0) {
				CAM_ERR(CAM_OIS,
					"Failed in Applying i2c wrt settings");
				return rc;
			}
		} else if (i2c_list->op_code == CAM_SENSOR_I2C_POLL) {
			size = i2c_list->i2c_settings.size;
			for (i = 0; i < size; i++) {
				rc = camera_io_dev_poll(
				&(o_ctrl->io_master_info),
				i2c_list->i2c_settings.reg_setting[i].reg_addr,
				i2c_list->i2c_settings.reg_setting[i].reg_data,
				i2c_list->i2c_settings.reg_setting[i].data_mask,
				i2c_list->i2c_settings.addr_type,
				i2c_list->i2c_settings.data_type,
				i2c_list->i2c_settings.reg_setting[i].delay);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
						"i2c poll apply setting Fail");
					return rc;
				}
			}
		}
	}

	return rc;
}

static int cam_ois_slaveInfo_pkt_parser(struct cam_ois_ctrl_t *o_ctrl,
	uint32_t *cmd_buf, size_t len)
{
	int32_t rc = 0;
	struct cam_cmd_ois_info *ois_info;

	if (!o_ctrl || !cmd_buf || len < sizeof(struct cam_cmd_ois_info)) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	ois_info = (struct cam_cmd_ois_info *)cmd_buf;
	if (o_ctrl->io_master_info.master_type == CCI_MASTER) {
		o_ctrl->io_master_info.cci_client->i2c_freq_mode =
			ois_info->i2c_freq_mode;
		o_ctrl->io_master_info.cci_client->sid =
			ois_info->slave_addr >> 1;
		o_ctrl->ois_fw_flag = ois_info->ois_fw_flag;
		o_ctrl->is_ois_calib = ois_info->is_ois_calib;
		memcpy(o_ctrl->ois_name, ois_info->ois_name, OIS_NAME_LEN);
		o_ctrl->ois_name[OIS_NAME_LEN - 1] = '\0';
		o_ctrl->io_master_info.cci_client->retries = 3;
		o_ctrl->io_master_info.cci_client->id_map = 0;
		memcpy(&(o_ctrl->opcode), &(ois_info->opcode),
			sizeof(struct cam_ois_opcode));
		CAM_DBG(CAM_OIS, "Slave addr: 0x%x Freq Mode: %d",
			ois_info->slave_addr, ois_info->i2c_freq_mode);
	} else if (o_ctrl->io_master_info.master_type == I2C_MASTER) {
		o_ctrl->io_master_info.client->addr = ois_info->slave_addr;
		CAM_DBG(CAM_OIS, "Slave addr: 0x%x", ois_info->slave_addr);
	} else {
		CAM_ERR(CAM_OIS, "Invalid Master type : %d",
			o_ctrl->io_master_info.master_type);
		rc = -EINVAL;
	}

	return rc;
}

#ifndef VENDOR_EDIT
/*hongbo.dai@Camera.driver, 2019/02/01, Add for sem1215s OIS fw update*/
#define PRJ_VERSION_PATH  "/proc/oppoVersion/prjVersion"
#define PCB_VERSION_PATH  "/proc/oppoVersion/pcbVersion"
static int getfileData(char *filename, char *context)
{
	struct file *mfile = NULL;
	ssize_t size = 0;
	loff_t offsize = 0;
	mm_segment_t old_fs;
	char project[10] = {0};

	memset(project, 0, sizeof(project));
	mfile = filp_open(filename, O_RDONLY, 0644);
	if (IS_ERR(mfile))
	{
		CAM_ERR(CAM_OIS, "%s fopen file %s failed !", __func__, filename);
		return (-1);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	offsize = 0;
	size = vfs_read(mfile, project, sizeof(project), &offsize);
	if (size < 0) {
		CAM_ERR(CAM_OIS, "fread file %s error size:%s", __func__, filename);
		set_fs(old_fs);
		filp_close(mfile, NULL);
		return (-1);
	}
	set_fs(old_fs);
	filp_close(mfile, NULL);

	CAM_ERR(CAM_OIS, "%s project:%s", __func__, project);
	memcpy(context, project, size);
	return 0;
}

static int getProject(char *project)
{
	int rc = 0;
	rc = getfileData(PRJ_VERSION_PATH, project);
	return rc;
}

static int getPcbVersion(char *pcbVersion)
{
	int rc = 0;
	rc = getfileData(PCB_VERSION_PATH, pcbVersion);
	return rc;
}
#endif
static int cam_ois_fw_download(struct cam_ois_ctrl_t *o_ctrl)
{
	uint16_t                           total_bytes = 0;
	uint8_t                           *ptr = NULL;
	int32_t                            rc = 0, cnt;
	uint32_t                           fw_size;
	const struct firmware             *fw = NULL;
	const char                        *fw_name_prog = NULL;
	const char                        *fw_name_coeff = NULL;
	char                               name_prog[32] = {0};
	char                               name_coeff[32] = {0};
	struct device                     *dev = &(o_ctrl->pdev->dev);
	struct cam_sensor_i2c_reg_setting  i2c_reg_setting;
	struct page                       *page = NULL;
	#ifdef VENDOR_EDIT
	int i = 0;
	#endif
	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	snprintf(name_coeff, 32, "%s.coeff", o_ctrl->ois_name);

	snprintf(name_prog, 32, "%s.prog", o_ctrl->ois_name);

	/* cast pointer as const pointer*/
	fw_name_prog = name_prog;
	fw_name_coeff = name_coeff;

	/* Load FW */
	rc = request_firmware(&fw, fw_name_prog, dev);
	if (rc) {
		CAM_ERR(CAM_OIS, "Failed to locate %s", fw_name_prog);
		return rc;
	}

	total_bytes = fw->size;
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.size = total_bytes;
	i2c_reg_setting.delay = 0;
	fw_size = PAGE_ALIGN(sizeof(struct cam_sensor_i2c_reg_array) *
		total_bytes) >> PAGE_SHIFT;
	page = cma_alloc(dev_get_cma_area((o_ctrl->soc_info.dev)),
		fw_size, 0, GFP_KERNEL);
	if (!page) {
		CAM_ERR(CAM_OIS, "Failed in allocating i2c_array");
		release_firmware(fw);
		return -ENOMEM;
	}

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *) (
		page_address(page));

#ifndef VENDOR_EDIT
		/*hongbo.dai@Camera.Driver, 2018/12/26, modify for [oppo ois]*/
	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;
		cnt++, ptr++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.prog;
		i2c_reg_setting.reg_setting[cnt].reg_data = *ptr;
		i2c_reg_setting.reg_setting[cnt].delay = 0;
		i2c_reg_setting.reg_setting[cnt].data_mask = 0;
	}

	rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
		&i2c_reg_setting, 1);
#else
	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;) {
		i2c_reg_setting.size = 0;
		for (i = 0; (i < MAX_LENGTH && cnt < total_bytes); i++,ptr++) {
			i2c_reg_setting.reg_setting[i].reg_addr =
				o_ctrl->opcode.prog;
			i2c_reg_setting.reg_setting[i].reg_data = *ptr;
			i2c_reg_setting.reg_setting[i].delay = 0;
			i2c_reg_setting.reg_setting[i].data_mask = 0;
			i2c_reg_setting.size++;
			cnt++;
		}
		i2c_reg_setting.delay = 0;
		if (i2c_reg_setting.size > 0) {
			rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
				&i2c_reg_setting, 1);
		}

	}
#endif

	if (rc < 0) {
		CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);
		goto release_firmware;
	}
	cma_release(dev_get_cma_area((o_ctrl->soc_info.dev)),
		page, fw_size);
	page = NULL;
	fw_size = 0;
	release_firmware(fw);

	rc = request_firmware(&fw, fw_name_coeff, dev);
	if (rc) {
		CAM_ERR(CAM_OIS, "Failed to locate %s", fw_name_coeff);
		return rc;
	}

	total_bytes = fw->size;
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.size = total_bytes;
	i2c_reg_setting.delay = 0;
	fw_size = PAGE_ALIGN(sizeof(struct cam_sensor_i2c_reg_array) *
		total_bytes) >> PAGE_SHIFT;
	page = cma_alloc(dev_get_cma_area((o_ctrl->soc_info.dev)),
		fw_size, 0, GFP_KERNEL);
	if (!page) {
		CAM_ERR(CAM_OIS, "Failed in allocating i2c_array");
		release_firmware(fw);
		return -ENOMEM;
	}

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *) (
		page_address(page));
#ifndef VENDOR_EDIT
	/*hongbo.dai@Camera.Driver, 2018/12/26, modify for [oppo ois]*/
	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;
		cnt++, ptr++) {
		i2c_reg_setting.reg_setting[cnt].reg_addr =
			o_ctrl->opcode.coeff;
		i2c_reg_setting.reg_setting[cnt].reg_data = *ptr;
		i2c_reg_setting.reg_setting[cnt].delay = 0;
		i2c_reg_setting.reg_setting[cnt].data_mask = 0;
	}

	rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
		&i2c_reg_setting, 1);
#else
	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;) {
		i2c_reg_setting.size = 0;
		for (i = 0; (i < MAX_LENGTH && cnt < total_bytes); i++,ptr++) {
			i2c_reg_setting.reg_setting[i].reg_addr =
				o_ctrl->opcode.coeff;
			i2c_reg_setting.reg_setting[i].reg_data = *ptr;
			i2c_reg_setting.reg_setting[i].delay = 0;
			i2c_reg_setting.reg_setting[i].data_mask = 0;
			i2c_reg_setting.size++;
			cnt++;
		}
		i2c_reg_setting.delay = 0;
		if (i2c_reg_setting.size > 0) {
			rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
				&i2c_reg_setting, 1);
		}
	}
#endif

	if (rc < 0)
		CAM_ERR(CAM_OIS, "OIS FW download failed %d", rc);

release_firmware:
	cma_release(dev_get_cma_area((o_ctrl->soc_info.dev)),
		page, fw_size);
	release_firmware(fw);

	return rc;
}
#ifdef VENDOR_EDIT
/*add by hongbo.dai@camera 20190117, for Tele OIS GyroOffset*/
static int cam_ois_read_gyrodata(struct cam_ois_ctrl_t *o_ctrl, uint32_t gyro_x_addr,
	uint32_t gyro_y_addr, uint32_t *gyro_data, bool need_convert)
{
	uint32_t gyro_offset = 0x00;
	uint32_t gyro_offset_x,  gyro_offset_y = 0x00;
	int rc = 0;

	CAM_INFO(CAM_OIS, "i2c_master:%d read gyro offset ", o_ctrl->io_master_info.cci_client->cci_i2c_master);

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), gyro_x_addr, &gyro_offset_x,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "read gyro offset_x fail");
	}
	if (need_convert) {
		gyro_offset_x = (gyro_offset_x & 0xFF) << 8 | (gyro_offset_x & 0xFF00) >> 8;
	}

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), gyro_y_addr, &gyro_offset_y,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "read gyro offset_y fail");
	}
	if (need_convert) {
		gyro_offset_y = (gyro_offset_y & 0xFF) << 8 | (gyro_offset_y & 0xFF00) >> 8;
	}

	gyro_offset = ((gyro_offset_y & 0xFFFF) << 16) | (gyro_offset_x & 0xFFFF);
	CAM_ERR(CAM_OIS, "final gyro_offset = 0x%x; gyro_x=0x%x, gyro_y=0x%x",
		gyro_offset, gyro_offset_x, gyro_offset_y);
	*gyro_data = gyro_offset;

	return rc;
}
/*add by hongbo.dai@camera 20190117, for Tele OIS GyroOffset*/
static int cam_ois_sem1215s_calibration(struct cam_ois_ctrl_t *o_ctrl)
{
	int32_t                            rc = 0;
	uint32_t                           data;
	uint32_t                           calib_data = 0x0;
	int                                calib_ret = 0;
	uint32_t                           gyro_offset = 0;
	int                                i = 0;
	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x0001, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0) {
		CAM_ERR(CAM_OIS, "read 0x0001 fail");
	}
	if (data != 0x01) {
		RamWriteByte(o_ctrl, 0x0000, 0x0, 50);
    }

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x0201, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0) {
		CAM_ERR(CAM_OIS, "read 0x0201 fail");
	}
	if (data != 0x01) {
		RamWriteByte(o_ctrl, 0x0200, 0x0, 10);
	}

	RamWriteByte(o_ctrl, 0x0600, 0x1, 100);
	for (i = 0; i < 5; i++) {
		rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x0600, &data,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (data == 0x00) {
        	break;
		}

		if((data != 0x00) && (i >= 5))
		{
			CAM_ERR(CAM_OIS, "Gyro Offset Cal FAIL ");

		}
	}

	cam_ois_read_gyrodata(o_ctrl, 0x0604, 0x0606, &gyro_offset, 1);

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x0004, &calib_data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if ((calib_data & 0x0100) == 0x0100) {
		CAM_ERR(CAM_OIS, "Gyro X Axis Offset Cal ERROR ");
		calib_ret = (0x1 << 16);
	}
	if ((calib_data & 0x0200) == 0x0200) {
		CAM_ERR(CAM_OIS, "Gyro Y Axis Offset Cal ERROR ");
		calib_ret |= (0x1);
	}

	if ((calib_data & (0x0100 | 0x0200)) == 0x0000)
	{
		RamWriteByte(o_ctrl, 0x300, 0x1, 100);
		for (i = 0; i < 5; i++) {
			rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x0300, &data,
				CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			if (data == 0x00) {
				break;
			} else if((data != 0x00) && (i >= 5)) {
				CAM_ERR(CAM_OIS, "Flash Save FAIL ");
			}
		}

		rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x0004, &calib_data,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
		if ((calib_data & 0x0040) != 0x00)
		{
			CAM_ERR(CAM_OIS, "Gyro Offset Cal ERROR ");
			calib_ret = (0x1 << 15);
		}
	}

	return calib_ret;
}
/*
static int cam_ois_bu63169_calibration(
	struct cam_ois_ctrl_t *o_ctrl) {
	int32_t   rc = 0;
	uint32_t  data = 0xFFFF;

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x8406, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "read 0x8406 fail");
	} else {
		CAM_ERR(CAM_OIS, "get 0x8406 = 0x%0x", data);
	}

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x8486, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "read 0x8486 fail");
	} else {
		CAM_ERR(CAM_OIS, "get 0x8486 = 0x%0x", data);
	}

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x8407, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "read 0x8407 fail");
	} else {
		CAM_ERR(CAM_OIS, "get 0x8407 = 0x%0x", data);
	}

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x8487, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "read 0x8487 fail");
	} else {
		CAM_ERR(CAM_OIS, "get 0x8487 = 0x%0x", data);
	}

	return rc;
}
*/
/*add by hongbo.dai@camera 20190220, for get OIS hall data for EIS*/
#define OIS_HALL_DATA_SIZE   52
static int cam_ois_bu63169_getmultiHall(
	struct cam_ois_ctrl_t *o_ctrl,
	OISHALL2EIS *hall_data)
{
	int32_t        rc = 0;
	uint8_t        data[OIS_HALL_DATA_SIZE] = {0x00};
	uint8_t        dataNum = 0;
	int            offset = 0;
	int32_t        i = 0;
	uint32_t       timeStamp = 0;
	uint32_t       mHalldata_X, mHalldata_Y, mdata[OIS_HALL_DATA_SIZE];
	struct timeval endTime, startTime;

	if (!o_ctrl) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	memset(hall_data, 0x00, sizeof(OISHALL2EIS));
	do_gettimeofday(&endTime);
	rc = camera_io_dev_read_seq(&(o_ctrl->io_master_info), 0x8A, data,
		CAMERA_SENSOR_I2C_TYPE_BYTE, CAMERA_SENSOR_I2C_TYPE_BYTE, OIS_HALL_DATA_SIZE);
	if (rc < 0) {
		CAM_ERR(CAM_OIS, "get mutil hall data fail");
		return -EINVAL;
	}

	dataNum = data[0];
	offset++;
	if (dataNum <= 0 || dataNum > HALL_MAX_NUMBER) {
		CAM_ERR(CAM_OIS, "get a wrong number of hall data:%d", dataNum);
		return -EINVAL;
	}

	for (i = 0; i < dataNum; i++) {
		mdata[i] = ((data[offset+3] & 0x00FF)
			| ((data[offset+2] << 8) & 0xFF00)
			| ((data[offset+1] << 16) & 0x00FF0000)
			| ((data[offset] << 24) & 0xFF000000));
		offset += 4;
	}

	timeStamp = ((uint32_t)(data[offset] << 8) | data[offset+1]);

	if ((long)(timeStamp * 1778) / 100 > endTime.tv_usec) {
		endTime.tv_sec = endTime.tv_sec - 1;
		endTime.tv_usec = (1000 * 1000 + endTime.tv_usec) - (timeStamp * 1778) / 100;
	} else {
		endTime.tv_usec = endTime.tv_usec - (timeStamp * 1778) / 100;
	}

	if (endTime.tv_usec < (long)(4000 * dataNum)) {
		startTime.tv_sec = endTime.tv_sec - 1;
		startTime.tv_usec = (1000 * 1000 + endTime.tv_usec) - (4000 * dataNum);
	} else {
		startTime.tv_sec = endTime.tv_sec;
		startTime.tv_usec = endTime.tv_usec - (4000 * dataNum);
	}

	for (i = 0; i < dataNum; i++) {
		mHalldata_X = ((mdata[i] >> 16) & 0xFFFF);
		mHalldata_Y = (mdata[i] & 0xFFFF);
		if ((startTime.tv_usec + 4000) / (1000 * 1000) >= 1) {
			startTime.tv_sec = startTime.tv_sec + 1;
			startTime.tv_usec = ((startTime.tv_usec + 4000) - 1000 * 1000);
		} else {
			startTime.tv_usec += 4000;
		}
		hall_data->datainfo[i].mHalldata = mdata[i];
		hall_data->datainfo[i].timeStampSec = startTime.tv_sec;
		hall_data->datainfo[i].timeStampUsec = startTime.tv_usec;
		CAM_DBG(CAM_OIS, "camxhalldata[%d] X:0x%04x  halldataY:0x%04x  sec = %d, us = %d", i,
			mHalldata_X,
			mHalldata_Y,
			hall_data->datainfo[i].timeStampSec,
			hall_data->datainfo[i].timeStampUsec);
	}

	return rc;
}


int32_t cam_lc898128_write_data(struct cam_ois_ctrl_t * o_ctrl,void * arg)
{
	int32_t  rc = 0;

    struct cam_control    *cmd = (struct cam_control *)arg;
	struct cam_write_eeprom_t cam_write_eeprom;

	forceExitpoll(o_ctrl);

	memset(&cam_write_eeprom, 0, sizeof(struct cam_write_eeprom_t));
	if (copy_from_user(&cam_write_eeprom, (void __user *) cmd->handle, sizeof(struct cam_write_eeprom_t))) {
		CAM_ERR(CAM_OIS, "Failed Copy from User");
		return -EFAULT;
	}

	//disable write protection
	if (cam_write_eeprom.isWRP == 0x01) {
		WriteEEpromData(&cam_write_eeprom);
	}
	//ReadEEpromData(&cam_write_eeprom);

	return rc;
}

#endif
/**
 * cam_ois_pkt_parse - Parse csl packet
 * @o_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
static int cam_ois_pkt_parse(struct cam_ois_ctrl_t *o_ctrl, void *arg)
{
	int32_t                         rc = 0;
	int32_t                         i = 0;
	uint32_t                        total_cmd_buf_in_bytes = 0;
	struct common_header           *cmm_hdr = NULL;
	uintptr_t                       generic_ptr;
	struct cam_control             *ioctl_ctrl = NULL;
	struct cam_config_dev_cmd       dev_config;
	struct i2c_settings_array      *i2c_reg_settings = NULL;
	struct cam_cmd_buf_desc        *cmd_desc = NULL;
	uintptr_t                       generic_pkt_addr;
	size_t                          pkt_len;
	size_t                          remain_len = 0;
	struct cam_packet              *csl_packet = NULL;
	size_t                          len_of_buff = 0;
	uint32_t                       *offset = NULL, *cmd_buf;
	#ifdef VENDOR_EDIT
	/*add by hongbo.dai@camera 20181215, for set bu63139 OIS pll0*/
	struct cam_sensor_i2c_reg_setting sensor_setting;
	/*add by hongbo.dai@camera, for read ois reg data*/
	uint32_t                        reg_val;
	int                             is_enable_tele_ois_thread = 0;
	int                             is_enable_main_ois_thread = 0;
	#endif
	struct cam_ois_soc_private     *soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t  *power_info = &soc_private->power_info;

	ioctl_ctrl = (struct cam_control *)arg;
	if (copy_from_user(&dev_config,
		u64_to_user_ptr(ioctl_ctrl->handle),
		sizeof(dev_config)))
		return -EFAULT;

#ifdef VENDOR_EDIT
#define OIS_SET_THREAD_STATUS_MASK              0x1000
#define OIS_SET_MIAN_GET_HALL_DATA_THREAD_MASK  0x01
#define OIS_SET_TELE_GET_HALL_DATA_THREAD_MASK  0x02
	   // send a dummy configure informaintion for set ois get hall thread
	   if (ioctl_ctrl->reserved & OIS_SET_THREAD_STATUS_MASK) {
		   if (ioctl_ctrl->reserved & OIS_SET_MIAN_GET_HALL_DATA_THREAD_MASK){
			   is_enable_main_ois_thread = true;
		   }
		   if (ioctl_ctrl->reserved & OIS_SET_TELE_GET_HALL_DATA_THREAD_MASK){
			   is_enable_tele_ois_thread = true;
		   }
		   set_ois_thread_status(is_enable_main_ois_thread,is_enable_tele_ois_thread);
		   CAM_INFO(CAM_OIS,"set ois get hall data thread status : main:[%d] tele: [%d]",
			   is_enable_main_ois_thread,is_enable_tele_ois_thread);
		   return rc;
	   }
#endif

	rc = cam_mem_get_cpu_buf(dev_config.packet_handle,
		&generic_pkt_addr, &pkt_len);
	if (rc) {
		CAM_ERR(CAM_OIS,
			"error in converting command Handle Error: %d", rc);
		return rc;
	}

	remain_len = pkt_len;
	if ((sizeof(struct cam_packet) > pkt_len) ||
		((size_t)dev_config.offset >= pkt_len -
		sizeof(struct cam_packet))) {
		CAM_ERR(CAM_OIS,
			"Inval cam_packet strut size: %zu, len_of_buff: %zu",
			 sizeof(struct cam_packet), pkt_len);
		return -EINVAL;
	}

	remain_len -= (size_t)dev_config.offset;
	csl_packet = (struct cam_packet *)
		(generic_pkt_addr + (uint32_t)dev_config.offset);

	if (cam_packet_util_validate_packet(csl_packet,
		remain_len)) {
		CAM_ERR(CAM_OIS, "Invalid packet params");
		return -EINVAL;
	}


	switch (csl_packet->header.op_code & 0xFFFFFF) {
	case CAM_OIS_PACKET_OPCODE_INIT:
		offset = (uint32_t *)&csl_packet->payload;
		offset += (csl_packet->cmd_buf_offset / sizeof(uint32_t));
		cmd_desc = (struct cam_cmd_buf_desc *)(offset);

		/* Loop through multiple command buffers */
		for (i = 0; i < csl_packet->num_cmd_buf; i++) {
			total_cmd_buf_in_bytes = cmd_desc[i].length;
			if (!total_cmd_buf_in_bytes)
				continue;

			rc = cam_mem_get_cpu_buf(cmd_desc[i].mem_handle,
				&generic_ptr, &len_of_buff);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "Failed to get cpu buf : 0x%x",
					cmd_desc[i].mem_handle);
				return rc;
			}
			cmd_buf = (uint32_t *)generic_ptr;
			if (!cmd_buf) {
				CAM_ERR(CAM_OIS, "invalid cmd buf");
				return -EINVAL;
			}

			if ((len_of_buff < sizeof(struct common_header)) ||
				(cmd_desc[i].offset > (len_of_buff -
				sizeof(struct common_header)))) {
				CAM_ERR(CAM_OIS,
					"Invalid length for sensor cmd");
				return -EINVAL;
			}
			remain_len = len_of_buff - cmd_desc[i].offset;
			cmd_buf += cmd_desc[i].offset / sizeof(uint32_t);
			cmm_hdr = (struct common_header *)cmd_buf;

			switch (cmm_hdr->cmd_type) {
			case CAMERA_SENSOR_CMD_TYPE_I2C_INFO:
				rc = cam_ois_slaveInfo_pkt_parser(
					o_ctrl, cmd_buf, remain_len);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
					"Failed in parsing slave info");
					return rc;
				}
				break;
			case CAMERA_SENSOR_CMD_TYPE_PWR_UP:
			case CAMERA_SENSOR_CMD_TYPE_PWR_DOWN:
				CAM_DBG(CAM_OIS,
					"Received power settings buffer");
				rc = cam_sensor_update_power_settings(
					cmd_buf,
					total_cmd_buf_in_bytes,
					power_info, remain_len);
				if (rc) {
					CAM_ERR(CAM_OIS,
					"Failed: parse power settings");
					return rc;
				}
				break;
			default:
			if (o_ctrl->i2c_init_data.is_settings_valid == 0) {
				CAM_DBG(CAM_OIS,
				"Received init settings");
				i2c_reg_settings =
					&(o_ctrl->i2c_init_data);
				i2c_reg_settings->is_settings_valid = 1;
				i2c_reg_settings->request_id = 0;
				rc = cam_sensor_i2c_command_parser(
					&o_ctrl->io_master_info,
					i2c_reg_settings,
					&cmd_desc[i], 1, NULL);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
					"init parsing failed: %d", rc);
					return rc;
				}
			} else if ((o_ctrl->is_ois_calib != 0) &&
				(o_ctrl->i2c_calib_data.is_settings_valid ==
				0)) {
				CAM_DBG(CAM_OIS,
					"Received calib settings");
				i2c_reg_settings = &(o_ctrl->i2c_calib_data);
				i2c_reg_settings->is_settings_valid = 1;
				i2c_reg_settings->request_id = 0;
				rc = cam_sensor_i2c_command_parser(
					&o_ctrl->io_master_info,
					i2c_reg_settings,
					&cmd_desc[i], 1, NULL);
				if (rc < 0) {
					CAM_ERR(CAM_OIS,
						"Calib parsing failed: %d", rc);
					return rc;
				}
			}
			break;
			}
		}

		if (o_ctrl->cam_ois_state != CAM_OIS_CONFIG) {
			rc = cam_ois_power_up(o_ctrl);
			if (rc) {
				CAM_ERR(CAM_OIS, " OIS Power up failed");
				return rc;
			}
			o_ctrl->cam_ois_state = CAM_OIS_CONFIG;
		}
		#ifdef VENDOR_EDIT
		/*modify by hongbo.dai@camera 20191016, for get ois fw version*/
		if (strstr(o_ctrl->ois_name, "sem1215")) {
			OISRead(o_ctrl, 0x1008, &reg_val);
			CAM_ERR(CAM_OIS, "read OIS fw Version:0x%0x", reg_val);
		}
		/*add by hongbo.dai@camera 20181215, for set bu63139 OIS pll0*/
		if (strstr(o_ctrl->ois_name, "bu63169")) {
			CAM_ERR(CAM_OIS, "need to write pll0 settings");
			sensor_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
			sensor_setting.data_type = CAMERA_SENSOR_I2C_TYPE_WORD;
			sensor_setting.size = bu63169_pll_settings.size;
			sensor_setting.delay = bu63169_pll_settings.delay;
			sensor_setting.reg_setting = bu63169_pll_settings.reg_setting;

			CAM_ERR(CAM_OIS, "need to write pll0 settings");
			rc = RamMultiWrite(o_ctrl, &sensor_setting);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "write pll settings error");
				goto pwr_dwn;
			}
		}
		if (o_ctrl->ois_fw_flag) {
			if (strstr(o_ctrl->ois_name, "lc898")) {
				o_ctrl->ois_module_vendor = (o_ctrl->opcode.pheripheral & 0xFF00) >> 8;
				o_ctrl->ois_actuator_vendor = o_ctrl->opcode.pheripheral & 0xFF;
				rc = DownloadFW(o_ctrl);
			} else {
				rc = cam_ois_fw_download(o_ctrl);
			}

			if (rc) {
				CAM_ERR(CAM_OIS, "Failed OIS FW Download");
				goto pwr_dwn;
			}
		}
#ifdef VENDOR_EDIT
		if (strstr(o_ctrl->ois_name, "lc898") != NULL) {
			if (o_ctrl->is_ois_calib) {
				rc = cam_ois_apply_settings(o_ctrl,
					&o_ctrl->i2c_calib_data);
				if (rc) {
					CAM_ERR(CAM_OIS, "Cannot apply calib data");
					goto pwr_dwn;
				}
			}
			Initcheck128(o_ctrl);
		}
#endif
		/*Added by hongbo.dai@Cam.Drv, 20181215, for check ois status*/
		if (strstr(o_ctrl->ois_name, "bu63169")) {
			uint32_t sum_check = 0;
			rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x84F7, &sum_check,
			    CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "read 0x84F7 fail");
			} else {
				CAM_ERR(CAM_OIS, "0x84F7 = 0x%x", sum_check);
			}
			rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x84F6, &sum_check,
				CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "read 0x84F6 fail");
			} else {
				CAM_ERR(CAM_OIS, "0x84F6 = 0x%x", sum_check);
			}
		}
		#else
		if (o_ctrl->ois_fw_flag) {
			rc = cam_ois_fw_download(o_ctrl);
			if (rc) {
				CAM_ERR(CAM_OIS, "Failed OIS FW Download");
				goto pwr_dwn;
			}
		}
		#endif

		rc = cam_ois_apply_settings(o_ctrl, &o_ctrl->i2c_init_data);
#ifdef VENDOR_EDIT
/*Jindian.Guan@Camera.Drv, 2020/01/09, modify for cci timeout case:04398317 */
		if ((rc == -EAGAIN) &&
			(o_ctrl->io_master_info.master_type == CCI_MASTER)) {
			CAM_WARN(CAM_OIS,
				"CCI HW is restting: Reapplying INIT settings");
			usleep_range(5000, 5010);
			rc = cam_ois_apply_settings(o_ctrl,
				&o_ctrl->i2c_init_data);
		}
#endif
		if (rc < 0) {
			CAM_ERR(CAM_OIS,
				"Cannot apply Init settings: rc = %d",
				rc);
			goto pwr_dwn;
		}
#ifdef VENDOR_EDIT
		if (o_ctrl->is_ois_calib && strstr(o_ctrl->ois_name, "lc898") == NULL) {
#else
		if (o_ctrl->is_ois_calib) {
#endif
			rc = cam_ois_apply_settings(o_ctrl,
				&o_ctrl->i2c_calib_data);
			if (rc) {
				CAM_ERR(CAM_OIS, "Cannot apply calib data");
				goto pwr_dwn;
			}
		}

		rc = delete_request(&o_ctrl->i2c_init_data);
		if (rc < 0) {
			CAM_WARN(CAM_OIS,
				"Fail deleting Init data: rc: %d", rc);
			rc = 0;
		}
		rc = delete_request(&o_ctrl->i2c_calib_data);
		if (rc < 0) {
			CAM_WARN(CAM_OIS,
				"Fail deleting Calibration data: rc: %d", rc);
			rc = 0;
		}

		break;
	case CAM_OIS_PACKET_OPCODE_OIS_CONTROL:
		if (o_ctrl->cam_ois_state < CAM_OIS_CONFIG) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
				"Not in right state to control OIS: %d",
				o_ctrl->cam_ois_state);
			return rc;
		}
		offset = (uint32_t *)&csl_packet->payload;
		offset += (csl_packet->cmd_buf_offset / sizeof(uint32_t));
		cmd_desc = (struct cam_cmd_buf_desc *)(offset);
		i2c_reg_settings = &(o_ctrl->i2c_mode_data);
		i2c_reg_settings->is_settings_valid = 1;
		i2c_reg_settings->request_id = 0;
		rc = cam_sensor_i2c_command_parser(&o_ctrl->io_master_info,
			i2c_reg_settings,
			cmd_desc, 1, NULL);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "OIS pkt parsing failed: %d", rc);
			return rc;
		}

#ifdef VENDOR_EDIT
		if (strstr(o_ctrl->ois_name, "lc898")
			|| strstr(o_ctrl->ois_name, "sem1215")) {
			if (!IsOISReady(o_ctrl)) {
				CAM_ERR(CAM_OIS, "OIS is not ready, apply setting may fail");
			}
		}
#endif

		rc = cam_ois_apply_settings(o_ctrl, i2c_reg_settings);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "Cannot apply mode settings");
			return rc;
		}

		rc = delete_request(i2c_reg_settings);
		if (rc < 0) {
			CAM_ERR(CAM_OIS,
				"Fail deleting Mode data: rc: %d", rc);
			return rc;
		}

		break;
	case CAM_OIS_PACKET_OPCODE_READ: {
		struct cam_buf_io_cfg *io_cfg;
		struct i2c_settings_array i2c_read_settings;

		if (o_ctrl->cam_ois_state < CAM_OIS_CONFIG) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
				"Not in right state to read OIS: %d",
				o_ctrl->cam_ois_state);
			return rc;
		}
		CAM_DBG(CAM_OIS, "number of I/O configs: %d:",
			csl_packet->num_io_configs);
		if (csl_packet->num_io_configs == 0) {
			CAM_ERR(CAM_OIS, "No I/O configs to process");
			rc = -EINVAL;
			return rc;
		}

		INIT_LIST_HEAD(&(i2c_read_settings.list_head));

		io_cfg = (struct cam_buf_io_cfg *) ((uint8_t *)
			&csl_packet->payload +
			csl_packet->io_configs_offset);

		if (io_cfg == NULL) {
			CAM_ERR(CAM_OIS, "I/O config is invalid(NULL)");
			rc = -EINVAL;
			return rc;
		}

		offset = (uint32_t *)&csl_packet->payload;
		offset += (csl_packet->cmd_buf_offset / sizeof(uint32_t));
		cmd_desc = (struct cam_cmd_buf_desc *)(offset);
		i2c_read_settings.is_settings_valid = 1;
		i2c_read_settings.request_id = 0;
		rc = cam_sensor_i2c_command_parser(&o_ctrl->io_master_info,
			&i2c_read_settings,
			cmd_desc, 1, io_cfg);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "OIS read pkt parsing failed: %d", rc);
			return rc;
		}

		rc = cam_sensor_i2c_read_data(
			&i2c_read_settings,
			&o_ctrl->io_master_info);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "cannot read data rc: %d", rc);
			delete_request(&i2c_read_settings);
			return rc;
		}

		rc = delete_request(&i2c_read_settings);
		if (rc < 0) {
			CAM_ERR(CAM_OIS,
				"Failed in deleting the read settings");
			return rc;
		}
		break;
	}
	default:
		CAM_ERR(CAM_OIS, "Invalid Opcode: %d",
			(csl_packet->header.op_code & 0xFFFFFF));
		return -EINVAL;
	}

	if (!rc)
		return rc;
pwr_dwn:
	cam_ois_power_down(o_ctrl);
	return rc;
}

void cam_ois_shutdown(struct cam_ois_ctrl_t *o_ctrl)
{
	int rc = 0;
	struct cam_ois_soc_private *soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	struct cam_sensor_power_ctrl_t *power_info = &soc_private->power_info;

	DeinitOIS(o_ctrl);

	if (o_ctrl->cam_ois_state == CAM_OIS_INIT)
		return;

	if (o_ctrl->cam_ois_state >= CAM_OIS_CONFIG) {
		rc = cam_ois_power_down(o_ctrl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "OIS Power down failed");
		o_ctrl->cam_ois_state = CAM_OIS_ACQUIRE;
	}

	if (o_ctrl->cam_ois_state >= CAM_OIS_ACQUIRE) {
		rc = cam_destroy_device_hdl(o_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "destroying the device hdl");
		o_ctrl->bridge_intf.device_hdl = -1;
		o_ctrl->bridge_intf.link_hdl = -1;
		o_ctrl->bridge_intf.session_hdl = -1;
	}

	if (o_ctrl->i2c_mode_data.is_settings_valid == 1)
		delete_request(&o_ctrl->i2c_mode_data);

	if (o_ctrl->i2c_calib_data.is_settings_valid == 1)
		delete_request(&o_ctrl->i2c_calib_data);

	if (o_ctrl->i2c_init_data.is_settings_valid == 1)
		delete_request(&o_ctrl->i2c_init_data);

	kfree(power_info->power_setting);
	kfree(power_info->power_down_setting);
	power_info->power_setting = NULL;
	power_info->power_down_setting = NULL;
	power_info->power_down_setting_size = 0;
	power_info->power_setting_size = 0;

	o_ctrl->cam_ois_state = CAM_OIS_INIT;
}

/**
 * cam_ois_driver_cmd - Handle ois cmds
 * @e_ctrl:     ctrl structure
 * @arg:        Camera control command argument
 *
 * Returns success or failure
 */
int cam_ois_driver_cmd(struct cam_ois_ctrl_t *o_ctrl, void *arg)
{
	int                              rc = 0;
	struct cam_ois_query_cap_t       ois_cap = {0};
	struct cam_control              *cmd = (struct cam_control *)arg;
	struct cam_ois_soc_private      *soc_private = NULL;
	struct cam_sensor_power_ctrl_t  *power_info = NULL;
	#ifdef VENDOR_EDIT
	/*Added by hongbo.dai@Cam.Drv, 20181215, for read gyro and hall val*/
	uint32_t gyro_x_addr = 0;
	uint32_t gyro_y_addr = 0;
	uint32_t hall_x_addr = 0;
	uint32_t hall_y_addr = 0;
	#endif

	if (!o_ctrl || !cmd) {
		CAM_ERR(CAM_OIS, "Invalid arguments");
		return -EINVAL;
	}

	if (cmd->handle_type != CAM_HANDLE_USER_POINTER) {
		CAM_ERR(CAM_OIS, "Invalid handle type: %d",
			cmd->handle_type);
		return -EINVAL;
	}

	soc_private =
		(struct cam_ois_soc_private *)o_ctrl->soc_info.soc_private;
	power_info = &soc_private->power_info;

	mutex_lock(&(o_ctrl->ois_mutex));
	switch (cmd->op_code) {
	case CAM_QUERY_CAP:
		ois_cap.slot_info = o_ctrl->soc_info.index;

		if (copy_to_user(u64_to_user_ptr(cmd->handle),
			&ois_cap,
			sizeof(struct cam_ois_query_cap_t))) {
			CAM_ERR(CAM_OIS, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}
		CAM_DBG(CAM_OIS, "ois_cap: ID: %d", ois_cap.slot_info);
		break;
	case CAM_ACQUIRE_DEV:
		rc = cam_ois_get_dev_handle(o_ctrl, arg);
		if (rc) {
			CAM_ERR(CAM_OIS, "Failed to acquire dev");
			goto release_mutex;
		}

		o_ctrl->cam_ois_state = CAM_OIS_ACQUIRE;
		break;
	case CAM_START_DEV:
		if (o_ctrl->cam_ois_state != CAM_OIS_CONFIG) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
			"Not in right state for start : %d",
			o_ctrl->cam_ois_state);
			goto release_mutex;
		}
		o_ctrl->cam_ois_state = CAM_OIS_START;
		break;
	case CAM_CONFIG_DEV:
		rc = cam_ois_pkt_parse(o_ctrl, arg);
		if (rc) {
			CAM_ERR(CAM_OIS, "Failed in ois pkt Parsing");
			goto release_mutex;
		}
		break;
	case CAM_RELEASE_DEV:
		CAM_INFO(CAM_OIS, "CAM_RELEASE_DEV: %d",
			o_ctrl->cam_ois_state);

		if (o_ctrl->cam_ois_state == CAM_OIS_START) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
				"Cant release ois: in start state");
			goto release_mutex;
		}

		if (o_ctrl->cam_ois_state == CAM_OIS_CONFIG) {
			rc = cam_ois_power_down(o_ctrl);
			if (rc < 0) {
				CAM_ERR(CAM_OIS, "OIS Power down failed");
				goto release_mutex;
			}
		}

		if (o_ctrl->bridge_intf.device_hdl == -1) {
			CAM_ERR(CAM_OIS, "link hdl: %d device hdl: %d",
				o_ctrl->bridge_intf.device_hdl,
				o_ctrl->bridge_intf.link_hdl);
			rc = -EINVAL;
			goto release_mutex;
		}
		rc = cam_destroy_device_hdl(o_ctrl->bridge_intf.device_hdl);
		if (rc < 0)
			CAM_ERR(CAM_OIS, "destroying the device hdl");
		o_ctrl->bridge_intf.device_hdl = -1;
		o_ctrl->bridge_intf.link_hdl = -1;
		o_ctrl->bridge_intf.session_hdl = -1;
		o_ctrl->cam_ois_state = CAM_OIS_INIT;

		kfree(power_info->power_setting);
		kfree(power_info->power_down_setting);
		power_info->power_setting = NULL;
		power_info->power_down_setting = NULL;
		power_info->power_down_setting_size = 0;
		power_info->power_setting_size = 0;

		if (o_ctrl->i2c_mode_data.is_settings_valid == 1)
			delete_request(&o_ctrl->i2c_mode_data);

		if (o_ctrl->i2c_calib_data.is_settings_valid == 1)
			delete_request(&o_ctrl->i2c_calib_data);

		if (o_ctrl->i2c_init_data.is_settings_valid == 1)
			delete_request(&o_ctrl->i2c_init_data);

		break;
	case CAM_STOP_DEV:
		if (o_ctrl->cam_ois_state != CAM_OIS_START) {
			rc = -EINVAL;
			CAM_WARN(CAM_OIS,
			"Not in right state for stop : %d",
			o_ctrl->cam_ois_state);
		}
		o_ctrl->cam_ois_state = CAM_OIS_CONFIG;
		break;
	#ifdef VENDOR_EDIT
	/*Added by Zhengrong.Zhang@Cam.Drv, 20180421, for [ois calibration]*/
	case CAM_GET_OIS_GYRO_OFFSET: {
		uint32_t gyro_offset = 0;
		bool m_convert = 0;
		/*add by hongbo.dai@camera 20181225, for get gyro position*/
		if (strstr(o_ctrl->ois_name, "lc898")) {
			gyro_x_addr = 0x0220;
			gyro_y_addr = 0x0224;
		} else if (strstr(o_ctrl->ois_name, "bu63169")) {
			gyro_x_addr = 0x8455;
			gyro_y_addr = 0x8456;
		} else { //we need to change these later for tele camera
			gyro_x_addr = 0x0604;
			gyro_y_addr = 0x0606;
			m_convert = 1;
		}
		cam_ois_read_gyrodata(o_ctrl, gyro_x_addr, gyro_y_addr, &gyro_offset, m_convert);

		if (copy_to_user((void __user *) cmd->handle, &gyro_offset,
			sizeof(gyro_offset))) {
			CAM_ERR(CAM_OIS, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}
		break;
	}

	case CAM_GET_OIS_HALL_POSITION: {
		uint32_t hall_position = 0;
		uint32_t hall_position_x = 0;
		uint32_t hall_position_y = 0;
		/*add by hongbo.dai@camera 20181225, for get hall position*/
		if (strstr(o_ctrl->ois_name, "lc898")) {
			hall_x_addr = 0x0178;
			hall_y_addr = 0x017C;
		} else if (strstr(o_ctrl->ois_name, "bu63169")) {
			hall_x_addr = 0x843f;
			hall_y_addr = 0x84bf;
		} else {  //we need to change these later for tele camera
			hall_x_addr = 0x0B10;
			hall_y_addr = 0x0B12;
		}

		rc = camera_io_dev_read(&(o_ctrl->io_master_info), hall_x_addr, &hall_position_x,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "read hall_position_x fail");
		}

		rc = camera_io_dev_read(&(o_ctrl->io_master_info), hall_y_addr, &hall_position_y,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
		if (rc < 0) {
			CAM_ERR(CAM_OIS, "read hall_position_y fail");
		}

		hall_position = ((hall_position_y & 0xFFFF) << 16) | (hall_position_x & 0xFFFF);
		CAM_INFO(CAM_OIS, "final hall_position = 0x%x; hall_position_x=0x%x, hall_position_y=0x%x",
			hall_position, hall_position_x, hall_position_y);

		if (copy_to_user((void __user *) cmd->handle, &hall_position,
			sizeof(hall_position))) {
			CAM_ERR(CAM_OIS, "Failed Copy hall data to User");
			rc = -EFAULT;
			goto release_mutex;
		}
		break;
	}
	case CAM_OIS_GYRO_OFFSET_CALIBRATION: {
		uint32_t result = 0;
		if (MASTER_1 == o_ctrl->io_master_info.cci_client->cci_i2c_master) {
			result = cam_ois_sem1215s_calibration(o_ctrl);
		} else {
			//result = cam_ois_bu63169_calibration(o_ctrl);
		}
		if (copy_to_user((void __user *) cmd->handle, &result,
			sizeof(result))) {
			CAM_ERR(CAM_OIS, "Failed Copy to User");
			rc = -EFAULT;
			goto release_mutex;
		}
		break;
	}
	case CAM_GET_OIS_EIS_HALL: {
			int get_hall_version;
			get_hall_version = cmd->reserved ;
			if (o_ctrl->cam_ois_state == CAM_OIS_START && strstr(o_ctrl->ois_name, "lc898") != NULL) {
				if (get_hall_version == GET_HALL_DATA_VERSION_V2){
					ReadOISHALLDataV2(o_ctrl, u64_to_user_ptr(cmd->handle));
				} else if (get_hall_version == GET_HALL_DATA_VERSION_V3){
					ReadOISHALLDataV3(o_ctrl, u64_to_user_ptr(cmd->handle));
				} else {
					ReadOISHALLData(o_ctrl, u64_to_user_ptr(cmd->handle));
				}
			} else if (strstr(o_ctrl->ois_name, "bu63169") != NULL) {
				OISHALL2EIS halldata;
				cam_ois_bu63169_getmultiHall(o_ctrl, &halldata);
				rc = copy_to_user((void __user *) cmd->handle, &halldata, sizeof(OISHALL2EIS));
				if (rc != 0) {
					CAM_ERR(CAM_OIS, "Failed Copy multi hall data to User:%d !!!", rc);
					rc = -EFAULT;
					goto release_mutex;
				}
			} else if (o_ctrl->cam_ois_state == CAM_OIS_START
			           && strstr(o_ctrl->ois_name, "sem1215s_ois") != NULL) {
				if (get_hall_version == GET_HALL_DATA_VERSION_V2){
					Sem1215sReadOISHALLDataV2(o_ctrl, u64_to_user_ptr(cmd->handle));
				}
				else {
					Sem1215sReadOISHALLData(o_ctrl, u64_to_user_ptr(cmd->handle));
				}
			}else {
				CAM_DBG(CAM_OIS, "OIS in wrong state %d", o_ctrl->cam_ois_state);
			}
			break;
		}
	case CAM_WRITE_CALIBRATION_DATA:
	case CAM_WRITE_AE_SYNC_DATA:
		CAM_DBG(CAM_OIS, "CAM_WRITE_DATA");
		if (strstr(o_ctrl->ois_name, "lc898") != NULL) {
			rc = cam_lc898128_write_data(o_ctrl, arg);
			if (rc) {
				CAM_ERR(CAM_EEPROM, "Failed in write AE sync data");
				goto release_mutex;
			}
		}
		break;
	#endif
	default:
		CAM_ERR(CAM_OIS, "invalid opcode");
		goto release_mutex;
	}
release_mutex:
	mutex_unlock(&(o_ctrl->ois_mutex));
	return rc;
}
