# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux Kernel SOC specific device drivers.
#

obj-$(CONFIG_ARCH_ACTIONS)	+= actions/
obj-$(CONFIG_ARCH_AT91)		+= atmel/
obj-y				+= bcm/
obj-$(CONFIG_ARCH_DOVE)		+= dove/
obj-$(CONFIG_MACH_DOVE)		+= dove/
obj-y				+= fsl/
obj-$(CONFIG_ARCH_GEMINI)	+= gemini/
obj-$(CONFIG_ARCH_MXC)		+= imx/
obj-$(CONFIG_SOC_XWAY)		+= lantiq/
obj-y				+= mediatek/
obj-y				+= amlogic/
obj-y				+= qcom/
obj-y				+= renesas/
obj-$(CONFIG_ARCH_ROCKCHIP)	+= rockchip/
obj-$(CONFIG_SOC_SAMSUNG)	+= samsung/
obj-$(CONFIG_ARCH_SUNXI)	+= sunxi/
obj-$(CONFIG_ARCH_TEGRA)	+= tegra/
obj-$(CONFIG_SOC_TI)		+= ti/
obj-$(CONFIG_ARCH_U8500)	+= ux500/
obj-$(CONFIG_PLAT_VERSATILE)	+= versatile/
obj-y				+= xilinx/
obj-$(CONFIG_ARCH_ZX)		+= zte/
obj-y                           += oplus/
#ifdef VENDOR_EDIT
#wen.luo@BSP.Kernel.Stability 2020/03/25 add for oplus kernel system
obj-$(CONFIG_OPLUS_SYSTEM_KERNEL)	+= oplus/system/
#endif /* VENDOR_EDIT */

#ifdef VENDOR_EDIT
#/* Hank.liu@TECH.PLAT.Storage, 2020-02-18, add oppo iomonitor driver*/
obj-$(CONFIG_OPPO_IOMONITOR)    +=  oplus/oppo_iomonitor/
#endif

#ifdef OPLUS_FEATURE_SENSOR_SMEM
#ChenYajie@BSP.SENSOR, 2020/06/16, Add for sensor info
obj-y						+= oplus/sensor/
#endif
#ifdef OPLUS_FEATURE_SENSOR_CRASH_MONITOR
#ChenYajie@BSP.SENSOR, 2020/06/16, Add for sensor crash monitor
obj-$(CONFIG_OPLUS_FEATURE_SENSOR_CRASH_MONITOR)	+= oplus/oppo_slpirst/
#endif


#ifdef OPLUS_FEATURE_HEALTHINFO
#//wenbin.liu@PSW.Kernel.MM, 2018/05/23, Add for Healthinfo monitor
obj-y     +=  oplus/oppo_healthinfo/
#endif /* OPLUS_FEATURE_HEALTHINFO */

#ifdef OPLUS_FEATURE_POWERINFO_STANDBY
#SunFaliang@BSP.Power.Basic, 2020/07/09, add for oppo_wakelock_profile.
obj-$(CONFIG_OPPO_WAKELOCK_PROFILER) += oplus/owakelock/
#endif /* OPLUS_FEATURE_POWERINFO_STANDBY */
