/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
 */

#ifndef _OPLUS_CAM_SENSOR_CORE_H_
#define _OPLUS_CAM_SENSOR_CORE_H_
#include "cam_sensor_dev.h"

uint32_t oplus_cam_sensor_addr_is_byte_type(struct cam_sensor_ctrl_t *s_ctrl, struct cam_camera_slave_info *slave_info);
void oplus_cam_sensor_for_laser_ois(struct cam_sensor_ctrl_t *s_ctrl, struct cam_camera_slave_info *slave_info);
int32_t oplus_cam_sensor_driver_cmd(struct cam_sensor_ctrl_t *s_ctrl,
	void *arg);
int cam_ois_sem1215s_calibration(struct camera_io_master *ois_master_info);

#endif /* _OPLUS_CAM_SENSOR_CORE_H_ */
