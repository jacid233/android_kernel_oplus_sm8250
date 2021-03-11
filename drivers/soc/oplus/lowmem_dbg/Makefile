ifneq ($(OPLUS_FEATURE_LOWMEM_DBG),)
#Hailong.Liu@BSP.Kernel.MM, 2020/06/19, Add for dump memory usage when lowmmem occurs.
obj-$(CONFIG_OPLUS_FEATURE_LOWMEM_DBG)   += lowmem_dbg.o
ccflags-$(CONFIG_MTK_ION)  += -I$(srctree)/drivers/staging/android/mtk_ion/
endif
