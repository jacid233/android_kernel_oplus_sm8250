// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
 */

#include <linux/module.h>
#include <cam_sensor_cmn_header.h>
#include "cam_sensor_util.h"
#include "cam_soc_util.h"
#include "cam_trace.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"
#include "oplus_cam_sensor_core.h"
#include "cam_sensor_core.h"

#ifdef VENDOR_EDIT
/*add by hongbo.dai@camera 20190505, for OIS firmware update*/
#include <linux/firmware.h>
#include <linux/dma-contiguous.h>


#define MAX_LENGTH 128


/*hongbo.dai@Camera.driver, 2019/02/01, Add for sem1215s OIS fw update*/
#define PRJ_VERSION_PATH  "/proc/oppoVersion/prjName"
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
		CAM_ERR(CAM_SENSOR, "%s fopen file %s failed !", __func__, filename);
		return (-1);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	offsize = 0;
	size = vfs_read(mfile, project, sizeof(project), &offsize);
	if (size < 0) {
		CAM_ERR(CAM_SENSOR, "fread file %s error size:%s", __func__, filename);
		set_fs(old_fs);
		filp_close(mfile, NULL);
		return (-1);
	}
	set_fs(old_fs);
	filp_close(mfile, NULL);

	CAM_ERR(CAM_SENSOR, "%s project:%s", __func__, project);
	memcpy(context, project, size);
	return 0;
}

static int getProject(char *project)
{
	int rc = 0;
	rc = getfileData(PRJ_VERSION_PATH, project);
	return rc;
}

/*add by hongbo.dai@camera 20190225, for fix current leak issue*/
static int RamWriteByte(struct camera_io_master *cci_master_info,
	uint32_t addr, uint32_t data, unsigned short mdelay)
{
	int32_t rc = 0;
	int retry = 1;
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
	if (cci_master_info == NULL) {
		CAM_ERR(CAM_SENSOR, "Invalid Args");
		return -EINVAL;
	}

	for( i = 0; i < retry; i++)
	{
		rc = camera_io_dev_write(cci_master_info, &i2c_write);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "write 0x%04x failed, retry:%d", addr, i+1);
		} else {
			return rc;
		}
	}
	return rc;
}


static int RamWriteWord(struct camera_io_master *cci_master_info,
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
		i2c_write .addr_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
		i2c_write .data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	}
	if (cci_master_info == NULL) {
		CAM_ERR(CAM_SENSOR, "Invalid Args");
		return -EINVAL;
	}

	for( i = 0; i < retry; i++)
	{
		rc = camera_io_dev_write(cci_master_info, &i2c_write);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "write 0x%04x failed, retry:%d", addr, i+1);
		} else {
			return rc;
		}
	}
	return rc;
}

#define SEM1815S_FW_VERSION_OFFSET           0x7FF4
#define SEM1815S_FW_UPDATE_VERSION           0x1240
#define SEM1815S_SEC_FW_UPDATE_VERSION       0x12d4
#define ABNORMAL_FW_UPDATE_VERSION           0x0000
#define SEM1815S_FW_UPDATE_NAME              "ois_sem1215s_19066_dvt.bin"

static int cam_sem1815s_ois_fw_download(struct cam_sensor_ctrl_t *o_ctrl)
{
	uint16_t                           total_bytes = 0;
	uint8_t                            *ptr = NULL;
	int32_t                            rc = 0, cnt;
	uint32_t                           fw_size, data;
	uint16_t                           check_sum = 0x0;
	const struct firmware              *fw = NULL;
	const char                         *fw_name_bin = SEM1815S_FW_UPDATE_NAME;

	struct device                      *dev = &(o_ctrl->pdev->dev);
	struct cam_sensor_i2c_reg_setting  i2c_reg_setting;
	struct page                        *page = NULL;
	int                                reg_double = 1;
	uint8_t                            fw_ver[4] = {0};
	uint32_t                           ois_fw_version = SEM1815S_FW_UPDATE_VERSION;
	uint16_t tmp_slave_addr = 0x00;
	uint16_t ois_prog_addr = 0x1100;
	int i = 0;


	struct cam_camera_slave_info *slave_info;

	if (!o_ctrl) {
		CAM_ERR(CAM_SENSOR, "Invalid Args");
		return -EINVAL;
	}

	CAM_INFO(CAM_SENSOR, "entry:%s ", __func__);

	slave_info = &(o_ctrl->sensordata->slave_info);
	if (!slave_info) {
		CAM_ERR(CAM_SENSOR, " failed: %pK",
			 slave_info);
		return -EINVAL;
	}

	/* Load FW first , if firmware not exist , return */
	rc = request_firmware(&fw, fw_name_bin, dev);
	if (rc) {
		CAM_ERR(CAM_SENSOR, "Failed to locate %s", fw_name_bin);
		o_ctrl->io_master_info.cci_client->sid = tmp_slave_addr;
		return 0;
	}

	tmp_slave_addr = o_ctrl->io_master_info.cci_client->sid;
	o_ctrl->io_master_info.cci_client->sid = (0x68 >> 1);

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x040b, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);

	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "read 0x040b fail");
		o_ctrl->io_master_info.cci_client->sid = tmp_slave_addr;
		return 0;
	}

	CAM_INFO(CAM_SENSOR, "read data 0x040b is 0x%x ",data);

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x1008, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "read 0x1008 fail");
		o_ctrl->io_master_info.cci_client->sid = tmp_slave_addr;
		return 0;
	}
	data = ((data >> 8) & 0x0FF) | ((data & 0xFF) << 8);

	if (data == ois_fw_version || data == SEM1815S_SEC_FW_UPDATE_VERSION || data == ABNORMAL_FW_UPDATE_VERSION) {
		CAM_INFO(CAM_SENSOR, "fw version:0x%0x need to update ois_fw_version:0x%0x !!!", data, ois_fw_version);
	} else {
		CAM_INFO(CAM_SENSOR, "fw version:0x%0x ois_fw_version:0x%0x no need to update !!!", data, ois_fw_version);
		o_ctrl->io_master_info.cci_client->sid = tmp_slave_addr;
		return 0;
	}

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x1020, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_INFO(CAM_SENSOR, "read 0x1020 fail");
		o_ctrl->io_master_info.cci_client->sid = tmp_slave_addr;
		return 0;
	}

	CAM_INFO(CAM_SENSOR, "read data 0x1020 is 0x%x ",data);

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x0001, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0) {
		CAM_INFO(CAM_SENSOR, "read 0x0001 fail");
	}
	if (data != 0x01) {
		RamWriteByte(&(o_ctrl->io_master_info), 0x0000, 0x0, 50);
	}

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x0201, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "read 0x0201 fail");
	}
	if (data != 0x01) {
		RamWriteByte(&(o_ctrl->io_master_info), 0x0200, 0x0, 10);
		rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x0201, &data,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0) {
			CAM_ERR(CAM_SENSOR, "read 0x0201 fail");
		}
	}

	RamWriteByte(&(o_ctrl->io_master_info), 0x1000, 0x05, 60);

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x0001, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "read 0x0001 fail");
	}
	if (data != 0x02) {
		o_ctrl->io_master_info.cci_client->sid = tmp_slave_addr;
		return 0;
	}

	total_bytes = fw->size;
	i2c_reg_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	i2c_reg_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	i2c_reg_setting.size = total_bytes;
	i2c_reg_setting.delay = 0;
	fw_size = PAGE_ALIGN(sizeof(struct cam_sensor_i2c_reg_array) *
		total_bytes) >> PAGE_SHIFT;
	page = cma_alloc(dev_get_cma_area((o_ctrl->soc_info.dev)),
		fw_size, 0, GFP_KERNEL);
	if (!page) {
		CAM_ERR(CAM_SENSOR, "Failed in allocating i2c_array");
		release_firmware(fw);
		o_ctrl->io_master_info.cci_client->sid = tmp_slave_addr;
		return -ENOMEM;
	}

	i2c_reg_setting.reg_setting = (struct cam_sensor_i2c_reg_array *) (
		page_address(page));

	CAM_INFO(CAM_SENSOR, "total_bytes:%d", total_bytes);

	for (cnt = 0, ptr = (uint8_t *)fw->data; cnt < total_bytes;) {
		i2c_reg_setting.size = 0;
		for (i = 0; (i < MAX_LENGTH && cnt < total_bytes); i++,ptr++) {
			if (cnt >= SEM1815S_FW_VERSION_OFFSET && cnt < (SEM1815S_FW_VERSION_OFFSET + 4)) {
				fw_ver[cnt-SEM1815S_FW_VERSION_OFFSET] = *ptr;
				CAM_ERR(CAM_SENSOR, "get fw version:0x%0x", fw_ver[cnt-SEM1815S_FW_VERSION_OFFSET]);
			}
			i2c_reg_setting.reg_setting[i].reg_addr = ois_prog_addr;
			i2c_reg_setting.reg_setting[i].reg_data = *ptr;
			i2c_reg_setting.reg_setting[i].delay = 0;
			i2c_reg_setting.reg_setting[i].data_mask = 0;
			i2c_reg_setting.size++;
			cnt++;
			if (reg_double == 0) {
				reg_double = 1;
			} else {
				check_sum += ((*(ptr+1) << 8) | *ptr) & 0xFFFF;
				reg_double = 0;
			}
		}
		i2c_reg_setting.delay = 0;

		if (i2c_reg_setting.size > 0) {
			rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info),
				&i2c_reg_setting, 1);
			msleep(1);
		}
	}
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "OIS FW download failed %d", rc);
		goto release_firmware;
	}
	CAM_INFO(CAM_SENSOR, "check sum:0x%0x", check_sum);

	RamWriteWord(&(o_ctrl->io_master_info), 0x1002, ((check_sum&0x0FF) << 8) | ((check_sum&0xFF00) >> 8));
	msleep(10);

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x1001, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "read 0x1001 fail");
	} else {
		CAM_INFO(CAM_SENSOR, "get 0x1001 = 0x%0x", data);
	}

	RamWriteByte(&(o_ctrl->io_master_info), 0x1000, 0x80, 200);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "write 0x1000 fail");
	}

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x1008, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "read 0x1008 fail");
	}
	CAM_INFO(CAM_SENSOR, "get 0x1008 = 0x%0x", data);

	rc = camera_io_dev_read(&(o_ctrl->io_master_info), 0x100A, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
	if (rc < 0) {
		CAM_ERR(CAM_SENSOR, "read 0x100A fail");
	}
	CAM_INFO(CAM_SENSOR, "get 0x100A = 0x%0x", data);

release_firmware:
	o_ctrl->io_master_info.cci_client->sid = tmp_slave_addr;
	cma_release(dev_get_cma_area((o_ctrl->soc_info.dev)),
		page, fw_size);
	release_firmware(fw);

	return rc;
}
#endif
#ifdef VENDOR_EDIT
/*add by hongbo.dai@camera 20190221, get DPC Data for IMX471*/
#define FD_DFCT_NUM_ADDR 0x7678
#define SG_DFCT_NUM_ADDR 0x767A
#define FD_DFCT_ADDR 0x8B00
#define SG_DFCT_ADDR 0x8B10

#define V_ADDR_SHIFT 12
#define H_DATA_MASK 0xFFF80000
#define V_DATA_MASK 0x0007FF80

struct sony_dfct_tbl_t imx471_dfct_tbl;

static int sensor_imx471_get_dpc_data(struct cam_sensor_ctrl_t *s_ctrl)
{
    int i = 0, j = 0;
    int rc = 0;
    int check_reg_val, dfct_data_h, dfct_data_l;
    int dfct_data = 0;
    int fd_dfct_num = 0, sg_dfct_num = 0;
    int retry_cnt = 5;
    int data_h = 0, data_v = 0;
    int fd_dfct_addr = FD_DFCT_ADDR;
    int sg_dfct_addr = SG_DFCT_ADDR;

    CAM_INFO(CAM_SENSOR, "sensor_imx471_get_dpc_data enter");
    if (s_ctrl == NULL) {
        CAM_ERR(CAM_SENSOR, "Invalid Args");
        return -EINVAL;
    }

    memset(&imx471_dfct_tbl, 0, sizeof(struct sony_dfct_tbl_t));

    for (i = 0; i < retry_cnt; i++) {
        check_reg_val = 0;
        rc = camera_io_dev_read(&(s_ctrl->io_master_info),
            FD_DFCT_NUM_ADDR, &check_reg_val,
            CAMERA_SENSOR_I2C_TYPE_WORD,
            CAMERA_SENSOR_I2C_TYPE_BYTE);

        if (0 == rc) {
            fd_dfct_num = check_reg_val & 0x07;
            if (fd_dfct_num > FD_DFCT_MAX_NUM)
                fd_dfct_num = FD_DFCT_MAX_NUM;
            break;
        }
    }

    for (i = 0; i < retry_cnt; i++) {
        check_reg_val = 0;
        rc = camera_io_dev_read(&(s_ctrl->io_master_info),
            SG_DFCT_NUM_ADDR, &check_reg_val,
            CAMERA_SENSOR_I2C_TYPE_WORD,
            CAMERA_SENSOR_I2C_TYPE_WORD);

        if (0 == rc) {
            sg_dfct_num = check_reg_val & 0x01FF;
            if (sg_dfct_num > SG_DFCT_MAX_NUM)
                sg_dfct_num = SG_DFCT_MAX_NUM;
            break;
        }
    }

    CAM_INFO(CAM_SENSOR, " fd_dfct_num = %d, sg_dfct_num = %d", fd_dfct_num, sg_dfct_num);
    imx471_dfct_tbl.fd_dfct_num = fd_dfct_num;
    imx471_dfct_tbl.sg_dfct_num = sg_dfct_num;

    if (fd_dfct_num > 0) {
        for (j = 0; j < fd_dfct_num; j++) {
            dfct_data = 0;
            for (i = 0; i < retry_cnt; i++) {
                dfct_data_h = 0;
                rc = camera_io_dev_read(&(s_ctrl->io_master_info),
                        fd_dfct_addr, &dfct_data_h,
                        CAMERA_SENSOR_I2C_TYPE_WORD,
                        CAMERA_SENSOR_I2C_TYPE_WORD);
                if (0 == rc) {
                    break;
                }
            }
            for (i = 0; i < retry_cnt; i++) {
                dfct_data_l = 0;
                rc = camera_io_dev_read(&(s_ctrl->io_master_info),
                        fd_dfct_addr+2, &dfct_data_l,
                        CAMERA_SENSOR_I2C_TYPE_WORD,
                        CAMERA_SENSOR_I2C_TYPE_WORD);
                if (0 == rc) {
                    break;
                }
            }
            CAM_DBG(CAM_SENSOR, " dfct_data_h = 0x%x, dfct_data_l = 0x%x", dfct_data_h, dfct_data_l);
            dfct_data = (dfct_data_h << 16) | dfct_data_l;
            data_h = 0;
            data_v = 0;
            data_h = (dfct_data & (H_DATA_MASK >> j%8)) >> (19 - j%8); //19 = 32 -13;
            data_v = (dfct_data & (V_DATA_MASK >> j%8)) >> (7 - j%8);  // 7 = 32 -13 -12;
            CAM_DBG(CAM_SENSOR, "j = %d, H = %d, V = %d", j, data_h, data_v);
            imx471_dfct_tbl.fd_dfct_addr[j] = ((data_h & 0x1FFF) << V_ADDR_SHIFT) | (data_v & 0x0FFF);
            CAM_DBG(CAM_SENSOR, "fd_dfct_data[%d] = 0x%08x", j, imx471_dfct_tbl.fd_dfct_addr[j]);
            fd_dfct_addr = fd_dfct_addr + 3 + ((j+1)%8 == 0);
        }
    }
    if (sg_dfct_num > 0) {
        for (j = 0; j < sg_dfct_num; j++) {
            dfct_data = 0;
            for (i = 0; i < retry_cnt; i++) {
                dfct_data_h = 0;
                rc = camera_io_dev_read(&(s_ctrl->io_master_info),
                        sg_dfct_addr, &dfct_data_h,
                        CAMERA_SENSOR_I2C_TYPE_WORD,
                        CAMERA_SENSOR_I2C_TYPE_WORD);
                if (0 == rc) {
                    break;
                }
            }
            for (i = 0; i < retry_cnt; i++) {
                dfct_data_l = 0;
                rc = camera_io_dev_read(&(s_ctrl->io_master_info),
                        sg_dfct_addr+2, &dfct_data_l,
                        CAMERA_SENSOR_I2C_TYPE_WORD,
                        CAMERA_SENSOR_I2C_TYPE_WORD);
                if (0 == rc) {
                    break;
                }
            }
            CAM_DBG(CAM_SENSOR, " dfct_data_h = 0x%x, dfct_data_l = 0x%x", dfct_data_h, dfct_data_l);
            dfct_data = (dfct_data_h << 16) | dfct_data_l;
            data_h = 0;
            data_v = 0;
            data_h = (dfct_data & (H_DATA_MASK >> j%8)) >> (19 - j%8); //19 = 32 -13;
            data_v = (dfct_data & (V_DATA_MASK >> j%8)) >> (7 - j%8);  // 7 = 32 -13 -12;
            CAM_DBG(CAM_SENSOR, "j = %d, H = %d, V = %d", j, data_h, data_v);
            imx471_dfct_tbl.sg_dfct_addr[j] = ((data_h & 0x1FFF) << V_ADDR_SHIFT) | (data_v & 0x0FFF);
            CAM_DBG(CAM_SENSOR, "sg_dfct_data[%d] = 0x%08x", j, imx471_dfct_tbl.sg_dfct_addr[j]);
            sg_dfct_addr = sg_dfct_addr + 3 + ((j+1)%8 == 0);
        }
    }

    CAM_INFO(CAM_SENSOR, "exit");
    return rc;
}
#endif

#ifdef VENDOR_EDIT
/*Guobao.Xiao@Camera.Drv, 20191226, Code transplant from 18115*/
/*add by hongbo.dai@camera, 20190419 fix for Laser current leak*/
#define LASER_ENABLE_PATH  "/sys/class/input/input6/enable_ps_sensor"
#define LASER_CROSSTALK_ENABLE "/sys/class/input/input6/crosstalk_enable"
#define LASER_CORRECTION_MODE  "/sys/class/input/input6/smudge_correction_mode"

static int writefileData(char *filename, int data)
{
	struct file *mfile = NULL;
	ssize_t size = 0;
	loff_t offsize = 0;
	mm_segment_t old_fs;
	char fdata[2] = {0};

	memset(fdata, 0, sizeof(fdata));
	mfile = filp_open(filename, O_RDWR, 0660);
	if (IS_ERR(mfile)) {
		CAM_ERR(CAM_SENSOR, "fopen file %s failed !", filename);
		return (-1);
	}
	else {
		CAM_INFO(CAM_SENSOR, "fopen file %s succeeded", filename);
	}
	snprintf(fdata, sizeof(fdata), "%d", data);
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	offsize = 0;
	size = vfs_write(mfile, fdata, sizeof(fdata), &offsize);
	if (size < 0) {
		CAM_ERR(CAM_SENSOR, "write file:%s data:%d error size:%d", filename, data, size);
		set_fs(old_fs);
		filp_close(mfile, NULL);
		return (-1);
	}
	else {
		CAM_INFO(CAM_SENSOR, "write file:%s data:%d correct size:%d", filename, data, size);
	}
	set_fs(old_fs);
	filp_close(mfile, NULL);

	return 0;
}
#endif

#ifdef VENDOR_EDIT
/*add by fangyan@camera, 20181222 for get sensor version*/
#define SONY_SENSOR_MP1 (0x02)  //imx708 cut1.0
#endif

uint32_t oplus_cam_sensor_addr_is_byte_type(struct cam_sensor_ctrl_t *s_ctrl, struct cam_camera_slave_info *slave_info)
{
    int rc = 0;
    uint32_t chipid = 0;

    //yufeng@camera, 20190711, add for sensor which has 1 byte regaddr
    uint32_t gc02m0_high = 0;
    uint32_t gc02m0_low = 0;
    uint32_t chipid_high = 0;
    uint32_t chipid_low = 0;
	uint32_t sensor_version = 0;
	uint16_t sensor_version_reg = 0x0018;
	const uint32_t IMX708_0_9_SOURCE_CHIPID = 0xF708;
	const uint32_t IMX708_1_0_SOURCE_CHIPID = 0xE708;

    if (slave_info->sensor_id == 0x02d0 || slave_info->sensor_id == 0x25 || slave_info->sensor_id == 0x02e0) {
    //yufeng@camera, 20190711, add for sensor which has 1 byte regaddr
    gc02m0_high = slave_info->sensor_id_reg_addr & 0xff00;
    gc02m0_high = gc02m0_high >> 8;
    gc02m0_low = slave_info->sensor_id_reg_addr & 0x00ff;
    rc = camera_io_dev_read(
        &(s_ctrl->io_master_info),
        gc02m0_high,
        &chipid_high, CAMERA_SENSOR_I2C_TYPE_BYTE,
        CAMERA_SENSOR_I2C_TYPE_BYTE);

    CAM_ERR(CAM_SENSOR, "gc02m0_high: 0x%x chipid_high id 0x%x:",
        gc02m0_high, chipid_high);

    rc = camera_io_dev_read(
        &(s_ctrl->io_master_info),
        gc02m0_low,
        &chipid_low, CAMERA_SENSOR_I2C_TYPE_BYTE,
        CAMERA_SENSOR_I2C_TYPE_BYTE);

    CAM_ERR(CAM_SENSOR, "gc02m0_low: 0x%x chipid_low id 0x%x:",
        gc02m0_low, chipid_low);

    chipid = ((chipid_high << 8) & 0xff00) | (chipid_low & 0x00ff);
        }
	/*add by fangyan@camera, 20181222 for multi sensor version*/
	CAM_WARN(CAM_SENSOR, "slave_info->sensor_id is %x",slave_info->sensor_id);
	if (slave_info->sensor_id == IMX708_0_9_SOURCE_CHIPID || slave_info->sensor_id == IMX708_1_0_SOURCE_CHIPID) {
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			sensor_version_reg,
			&sensor_version, CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_WORD);
		CAM_WARN(CAM_SENSOR, "imx708 sensor_version: 0x%x",
			sensor_version >> 8);
		if ((sensor_version >> 8) >= SONY_SENSOR_MP1) {
			chipid = IMX708_1_0_SOURCE_CHIPID;
			CAM_WARN(CAM_SENSOR, "set chipid : 0x%x",
				IMX708_1_0_SOURCE_CHIPID);
			if ((sensor_version >> 8) > SONY_SENSOR_MP1) {
				s_ctrl->sensordata->slave_info.sensor_version = 1;
			} else {
				s_ctrl->sensordata->slave_info.sensor_version = 0;
			}
			CAM_WARN(CAM_SENSOR, "imx708 slave_info.sensor_version: %d:",
				s_ctrl->sensordata->slave_info.sensor_version );
		} else {
			chipid = IMX708_0_9_SOURCE_CHIPID;
			CAM_WARN(CAM_SENSOR, "set chipid : 0x%x",
				IMX708_0_9_SOURCE_CHIPID);
		}
	}

	if (slave_info->sensor_id == 0x0689) {
		rc = camera_io_dev_read(
			&(s_ctrl->io_master_info),
			sensor_version_reg,
			&sensor_version, CAMERA_SENSOR_I2C_TYPE_WORD,
			CAMERA_SENSOR_I2C_TYPE_WORD);
		CAM_WARN(CAM_SENSOR, "imx689 sensor_version: 0x%x",
			sensor_version >> 8);
		if ((sensor_version >> 8) > SONY_SENSOR_MP1) {
			s_ctrl->sensordata->slave_info.sensor_version = 1;
			CAM_WARN(CAM_SENSOR, "imx689 slave_info.sensor_version: %d:",
				s_ctrl->sensordata->slave_info.sensor_version );
		} else {
			s_ctrl->sensordata->slave_info.sensor_version = 0;
			CAM_WARN(CAM_SENSOR, "imx689 slave_info.sensor_version: %d:",
				s_ctrl->sensordata->slave_info.sensor_version );
		}
              chipid = 0x689;
	}

	if (slave_info->sensor_id == 0x2b03) {
		rc = camera_io_dev_read(
			 &(s_ctrl->io_master_info),
			 slave_info->sensor_id_reg_addr,
			 &chipid, CAMERA_SENSOR_I2C_TYPE_WORD,
			 CAMERA_SENSOR_I2C_TYPE_WORD);

		CAM_WARN(CAM_SENSOR, "ov02b chipid: 0x%x",
			chipid);
	}

    return chipid;
}

void oplus_cam_sensor_for_laser_ois(struct cam_sensor_ctrl_t *s_ctrl, struct cam_camera_slave_info *slave_info)
{
    //Guobao.Xiao@camera, 20191226, add for project division
        char strPrjname[10];
        memset(strPrjname, 0, sizeof(strPrjname));
        if(0 == getProject(strPrjname)) {
            CAM_INFO(CAM_SENSOR, "Get project name is %s", strPrjname);
        }

    /*Zhixian.Mai@Cam 20191224 temp hard code here for laser and ois*/
        if ((0 == strcmp(strPrjname,"19067")) ||
            (0 == strcmp(strPrjname,"19362")) ||
            (0 == strcmp(strPrjname,"19066"))) {
             if (slave_info->sensor_id == 0x0689) {
                /*Guobao.Xiao@Camera.Drv, 20191226, close laser when rear main camera probe*/
                 writefileData(LASER_ENABLE_PATH, 0);
                 writefileData(LASER_CROSSTALK_ENABLE, 1);
                 writefileData(LASER_CORRECTION_MODE, 1);
             }
             if (slave_info->sensor_id == 0x30D5) {
                 cam_sem1815s_ois_fw_download(s_ctrl);
             }
        }
        if ((0 == strcmp(strPrjname,"19065")) ||
            (0 == strcmp(strPrjname,"19361")) ||
            (0 == strcmp(strPrjname,"19063"))) {
             if (slave_info->sensor_id == 0x586) {
                /*Guobao.Xiao@Camera.Drv, 20191226, close laser when rear main camera probe*/
                 writefileData(LASER_ENABLE_PATH, 0);
                 writefileData(LASER_CROSSTALK_ENABLE, 1);
                 writefileData(LASER_CORRECTION_MODE, 1);
             }
        }

        /*add by hongbo.dai@camera 20190221, get DPC Data for IMX471*/
        if (slave_info->sensor_id == 0x0471) {
            sensor_imx471_get_dpc_data(s_ctrl);
        }
}

int32_t oplus_cam_sensor_driver_cmd(struct cam_sensor_ctrl_t *s_ctrl,
	void *arg)
{
        int rc = 0;
        struct cam_control *cmd = (struct cam_control *)arg;
#ifdef VENDOR_EDIT
/*Zhixian.mai@Cam.Drv 20200329 add for oem ioctl for read /write register*/
	switch (cmd->op_code) {
	case CAM_OEM_IO_CMD:{
		struct cam_oem_rw_ctl oem_ctl;
		struct camera_io_master oem_io_master_info;
		struct cam_sensor_cci_client oem_cci_client;
              struct cam_oem_i2c_reg_array *cam_regs = NULL;
		if (copy_from_user(&oem_ctl, (void __user *)cmd->handle,
			sizeof(struct cam_oem_rw_ctl))) {
			CAM_ERR(CAM_SENSOR,
					"Fail in copy oem control infomation form user data");
                      rc = -ENOMEM;
                      return rc;
		}
		if (oem_ctl.num_bytes > 0) {
			cam_regs = (struct cam_oem_i2c_reg_array *)kzalloc(
				sizeof(struct cam_oem_i2c_reg_array)*oem_ctl.num_bytes, GFP_KERNEL);
			if (!cam_regs) {
				rc = -ENOMEM;
                             CAM_ERR(CAM_SENSOR,"failed alloc cam_regs");
				return rc;
			}

			if (copy_from_user(cam_regs, u64_to_user_ptr(oem_ctl.cam_regs_ptr),
				sizeof(struct cam_oem_i2c_reg_array)*oem_ctl.num_bytes)) {
				CAM_INFO(CAM_SENSOR, "copy_from_user error!!!", oem_ctl.num_bytes);
				rc = -EFAULT;
				goto free_cam_regs;
			}
		}
		memcpy(&oem_io_master_info, &(s_ctrl->io_master_info),sizeof(struct camera_io_master));
		memcpy(&oem_cci_client, s_ctrl->io_master_info.cci_client,sizeof(struct cam_sensor_cci_client));
		oem_io_master_info.cci_client = &oem_cci_client;
		if (oem_ctl.slave_addr != 0) {
			oem_io_master_info.cci_client->sid = (oem_ctl.slave_addr >> 1);
		}

		switch (oem_ctl.cmd_code) {
        	case CAM_OEM_CMD_READ_DEV: {
			int i = 0;
			for (; i < oem_ctl.num_bytes; i++)
			{
				rc |= cam_cci_i2c_read(
					 oem_io_master_info.cci_client,
					 cam_regs[i].reg_addr,
					 &(cam_regs[i].reg_data),
					 oem_ctl.reg_addr_type,
					 oem_ctl.reg_data_type);
				CAM_INFO(CAM_SENSOR,
					"read addr:0x%x  Data:0x%x ",
					cam_regs[i].reg_addr, cam_regs[i].reg_data);
			}

			if (rc < 0) {
				CAM_ERR(CAM_SENSOR,
					"Fail oem ctl data ,slave sensor id is 0x%x",s_ctrl->sensordata->slave_info.sensor_id);
				goto free_cam_regs;;
			}

			if (copy_to_user(u64_to_user_ptr(oem_ctl.cam_regs_ptr), cam_regs,
				sizeof(struct cam_oem_i2c_reg_array)*oem_ctl.num_bytes)) {
				CAM_ERR(CAM_SENSOR,
						"Fail oem ctl data ,slave sensor id is 0x%x",s_ctrl->sensordata->slave_info.sensor_id);
				goto free_cam_regs;

			}
			break;
		}
		case CAM_OEM_CMD_WRITE_DEV: {
			struct cam_sensor_i2c_reg_setting write_setting;
			int i = 0;
			for (;i < oem_ctl.num_bytes; i++)
			{
				CAM_DBG(CAM_SENSOR,"Get from OEM addr: 0x%x data: 0x%x ",
								cam_regs[i].reg_addr, cam_regs[i].reg_data);
			}

			write_setting.addr_type = oem_ctl.reg_addr_type;
			write_setting.data_type = oem_ctl.reg_data_type;
			write_setting.size = oem_ctl.num_bytes;
			write_setting.reg_setting = (struct cam_sensor_i2c_reg_array*)cam_regs;

			rc = cam_cci_i2c_write_table(&oem_io_master_info,&write_setting);

			if (rc < 0){
				CAM_ERR(CAM_SENSOR,
					"Fail oem write data ,slave sensor id is 0x%x",s_ctrl->sensordata->slave_info.sensor_id);
				goto free_cam_regs;
			}

			break;
		}

		case CAM_OEM_OIS_CALIB : {
			rc = cam_ois_sem1215s_calibration(&oem_io_master_info);
                      CAM_ERR(CAM_SENSOR, "ois calib failed rc:%d", rc);
			break;
		}

		default:
			CAM_ERR(CAM_SENSOR,
						"Unknow OEM cmd ,slave sensor id is 0x%x",s_ctrl->sensordata->slave_info.sensor_id);
			break ;
		}

free_cam_regs:
		if (cam_regs != NULL) {
			kfree(cam_regs);
			cam_regs = NULL;
		}
		mutex_unlock(&(s_ctrl->cam_sensor_mutex));
		return rc;
	}

	case CAM_OEM_GET_ID : {
		if (copy_to_user((void __user *)cmd->handle,&s_ctrl->soc_info.index,
						sizeof(uint32_t))) {
			CAM_ERR(CAM_SENSOR,
					"copy camera id to user fail ");
		}
		break;
	}
	/*add by hongbo.dai@camera 20190221, get DPC Data for IMX471*/
	case CAM_GET_DPC_DATA: {
		if (0x0471 != s_ctrl->sensordata->slave_info.sensor_id) {
			rc = -EFAULT;
                      return rc;
		}
		CAM_INFO(CAM_SENSOR, "imx471_dfct_tbl: fd_dfct_num=%d, sg_dfct_num=%d",
			imx471_dfct_tbl.fd_dfct_num, imx471_dfct_tbl.sg_dfct_num);
		if (copy_to_user((void __user *) cmd->handle, &imx471_dfct_tbl,
			sizeof(struct  sony_dfct_tbl_t))) {
			CAM_ERR(CAM_SENSOR, "Failed Copy to User");
			rc = -EFAULT;
                      return rc;
		}
	}
       break;
       default:
            	CAM_ERR(CAM_SENSOR, "Invalid Opcode: %d", cmd->op_code);
		rc = -EINVAL;
       break;
       }
#endif

	return rc;
}

int cam_ois_sem1215s_calibration(struct camera_io_master *ois_master_info)
{
	int32_t                            rc = 0;
	uint32_t                           data;
	uint32_t                           calib_data = 0x0;
	int                                calib_ret = 0;
	//uint32_t                           gyro_offset = 0;
	int                                i = 0;
	if (!ois_master_info) {
		CAM_ERR(CAM_OIS, "Invalid Args");
		return -EINVAL;
	}

	rc = camera_io_dev_read(ois_master_info, 0x0001, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0) {
		CAM_ERR(CAM_OIS, "read 0x0001 fail");
	}
	if (data != 0x01) {
		RamWriteByte(ois_master_info, 0x0000, 0x0, 50);
    }

	rc = camera_io_dev_read(ois_master_info, 0x0201, &data,
		CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (rc < 0) {
		CAM_ERR(CAM_OIS, "read 0x0201 fail");
	}
	if (data != 0x01) {
		RamWriteByte(ois_master_info, 0x0200, 0x0, 10);
	}

	RamWriteByte(ois_master_info, 0x0600, 0x1, 100);
	for (i = 0; i < 5; i++) {
		rc = camera_io_dev_read(ois_master_info, 0x0600, &data,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
		if (data == 0x00) {
        	break;
		}

		if((data != 0x00) && (i >= 5))
		{
			CAM_ERR(CAM_OIS, "Gyro Offset Cal FAIL ");

		}
	}

	//cam_ois_read_gyrodata(ois_master_info, 0x0604, 0x0606, &gyro_offset, 1);

	rc = camera_io_dev_read(ois_master_info, 0x0004, &calib_data,
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
		RamWriteByte(ois_master_info, 0x300, 0x1, 100);
		for (i = 0; i < 5; i++) {
			rc = camera_io_dev_read(ois_master_info, 0x0300, &data,
				CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE);
			if (data == 0x00) {
				break;
			} else if((data != 0x00) && (i >= 5)) {
				CAM_ERR(CAM_OIS, "Flash Save FAIL ");
			}
		}

		rc = camera_io_dev_read(ois_master_info, 0x0004, &calib_data,
			CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_WORD);
		if ((calib_data & 0x0040) != 0x00)
		{
			CAM_ERR(CAM_OIS, "Gyro Offset Cal ERROR ");
			calib_ret = (0x1 << 15);
		}
	}

	return calib_ret;
}

