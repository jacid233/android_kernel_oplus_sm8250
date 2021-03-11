obj-$(CONFIG_OPPO_COMMON_SOFT)  +=  device_info/

#ifdef OPLUS_FEATURE_LOWMEM_DBG
#Hailong.Liu@BSP.Kernel.MM, 2020/06/17, Add for dump mem when shrink
obj-$(CONFIG_OPLUS_FEATURE_LOWMEM_DBG)	+=  lowmem_dbg/
#endif /* OPLUS_FEATURE_LOWMEM_DBG */

#ifdef OPLUS_FEATURE_MEMLEAK_DETECT
#Hailong.Liu@BSP.Kernel.MM, 2020/06/17, Add for svelte logging
obj-$(CONFIG_OPPO_SVELTE) +=  oppo_svelte/
#endif /* OPLUS_FEATURE_MEMLEAK_DETECT */

#ifdef OPLUS_FEATURE_THERMAL_HORAE
#shuze.ma@BSP.Power.Thermal 2020/08/17, Add for thermal
obj-$(CONFIG_HORAE_THERMAL_SHELL) += thermal/
#endif /* OPLUS_FEATURE_THERMAL_HORAE */

#ifdef OPLUS_FEATURE_GPIO_NC
#/*Jack@BSP.kernel, 2019/09/10, Add for NC gpio*/
obj-$(CONFIG_OPLUS_GPIO_NC) +=  oplus_gpio/
#endif

