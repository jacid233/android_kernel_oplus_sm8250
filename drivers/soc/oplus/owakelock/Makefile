#ifdef OPLUS_FEATURE_POWERINFO_STANDBY
#SunFaliang@BSP.Power.Basic, 2020/05/01, add for wakelock profiler
ifeq ($(CONFIG_OPPO_POWER_MTK),y)
	obj-$(CONFIG_OPPO_WAKELOCK_PROFILER)	+= oppo_wakelock_profiler_mtk.o
else ifeq ($(CONFIG_OPPO_POWER_QCOM),y)
	obj-$(CONFIG_OPPO_WAKELOCK_PROFILER)    += oppo_wakelock_profiler_qcom.o
else ifeq ($(CONFIG_OPPO_WAKELOCK_PROFILER),y)
	obj-y                                   += oppo_wakelock_profiler.o
endif
#endif /* OPLUS_FEATURE_POWERINFO_STANDBY */
