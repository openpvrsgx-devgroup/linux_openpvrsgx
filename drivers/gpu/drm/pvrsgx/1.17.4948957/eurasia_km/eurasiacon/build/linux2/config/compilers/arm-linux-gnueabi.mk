# 32-bit ARM soft float compiler
ifeq ($(MULTIARCH),1)
 TARGET_SECONDARY_ARCH := target_armel
else
 TARGET_PRIMARY_ARCH   := target_armel
endif
