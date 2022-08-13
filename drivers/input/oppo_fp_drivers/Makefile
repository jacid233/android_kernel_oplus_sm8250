obj-$(CONFIG_OPPO_FINGERPRINT) += oppo_fp_common/
obj-$(CONFIG_OPPO_FINGERPRINT) += fpc/

ifdef CONFIG_OPPO_FINGERPRINT_GOODIX_FP
    obj-$(CONFIG_OPPO_FINGERPRINT) += goodix_fp/
else
    obj-$(CONFIG_OPPO_FINGERPRINT) += goodix_optical_fp/
endif

ifeq ($(CONFIG_OPPO_FINGERPRINT_MTK),y)
    obj-$(CONFIG_OPPO_FINGERPRINT) += silead_optical_fp/
endif

ifeq ($(CONFIG_OPPO_FINGERPRINT_QCOM),y)
    ifndef CONFIG_OPPO_FINGERPRINT_GOODIX_FP
        obj-$(CONFIG_OPPO_FINGERPRINT) += silead/
    endif
endif
