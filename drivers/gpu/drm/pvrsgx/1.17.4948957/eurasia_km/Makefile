# derived from some SDK4 Makefiles

# although e.g. both omap3630 and am335x have the same SGX530 version
# we need to compile different kernel modules because the interfaces
# are not the same and we don't have a generic driver (yet) that
# uses DT for all $(SOC_FAMILY) and $(SOC) variance.
# and use DT for $(SGX) and $(SGX_REV) variance.

# FIXME: all this should be coverd by dynamic DT code incl. sgxerrata.h
# i.e. the board .compatible entry should uniquely specify
# e.g. .compatible = "soc", "img,sgx544-116"

ccflags-y += -D$(SGX_FLAG)
ccflags-y += -DAM_VERSION=$(AM_VERSION)

# FIXME: if we use dynamic errata and flags and a generic backend
# we only need to build a single module without differentiating
# SoC, SGX version and revision
#
# for the moment we must separate them by module name built from
#
# - SoC family (omap, sunxi, ...)
# - SoC version (omap3, omap4, omap5, ...)
# - SGX version (530, 540, 544)
# - SGX revision

MODULE := pvrsrvkm_$(SOC)_$(SGX)_$(SGX_REV)

# make either $(MODULE)-y or $(MODULE)-m as defined by TARGET_TYPE
TARGET := $(MODULE)-$(TARGET_TYPE)
obj-$(TARGET_TYPE) += $(MODULE).o
ccflags-y += -DPVR_BUILD_DATE="\"$(shell date "+%Y%m%d" )\""

# this defines the compatible string which must be present in the DTB of the board
ccflags-y += -DSYS_SGX_DEV_NAME="\"$(SOC_VENDOR),$(SOC)-$(SGX)-$(SGX_REV)\""

# vendor_soc_sgx_rev for macro use
ccflags-y += -D$(SOC_VENDOR)_$(SOC)_$(SGX)_$(SGX_REV)

ccflags-y += -DSGX_CORE_REV=$(SGX_REV)

ccflags-y += \
	-I$(srctree)/include \
	-I$(srctree)/arch/arm/mach-omap2 \
	-I$(srctree)/drivers/video/fbdev/omap2 \
	-I$(srctree)/$(src)/ \
	-I$(srctree)/$(src)/include4 \
	-I$(srctree)/$(src)/services4/include \
	-I$(srctree)/$(src)/services4/include/env/linux \
	-I$(srctree)/$(src)/services4/system/include \
	-I$(srctree)/$(src)/services4/srvkm/bridged \
	-I$(srctree)/$(src)/services4/srvkm/bridged/sgx \
	-I$(srctree)/$(src)/services4/srvkm/devices/sgx \
	-I$(srctree)/$(src)/services4/srvkm/env/linux \
	-I$(srctree)/$(src)/services4/srvkm/hwdefs \
	-I$(srctree)/$(src)/services4/srvkm/include \
	-I$(srctree)/$(src)/services4/3rdparty/bufferclass_ti \
	-I$(srctree)/include/linux

# choose SoC integration variant to choose the right source subtree

ccflags-y += \
	-I$(srctree)/$(src)/services4/system/$(SOC_FAMILY)

ifeq ($(SOC_FAMILY),omap)
$(TARGET) += \
	services4/system/$(SOC_FAMILY)/sysconfig.o \
	services4/system/$(SOC_FAMILY)/sysutils_linux.o
ifneq ($(CONFIG_OMAP_SGX_FREQ),)
# not in Kconfig... some API for do_gettimeofday() etc. missing/changed
$(TARGET) += \
	services4/system/$(SOC_FAMILY)/sgxfreq_activeidle.o \
	services4/system/$(SOC_FAMILY)/sgxfreq_cool.o \
	services4/system/$(SOC_FAMILY)/sgxfreq_on3demand.o \
	services4/system/$(SOC_FAMILY)/sgxfreq_onoff.o \
	services4/system/$(SOC_FAMILY)/sgxfreq_userspace.o \
	services4/system/$(SOC_FAMILY)/sgxfreq.o
endif
else ifeq ($(SOC_FAMILY),sunxi)
$(TARGET) += \
	services4/system/$(SOC_FAMILY)/sysconfig.o \
	services4/system/$(SOC_FAMILY)/sysutils_linux.o
else
$(TARGET) += \
	services4/system/$(SOC_FAMILY)/sysconfig.o \
	services4/system/$(SOC_FAMILY)/sysutils.o
endif

#
# many of the following options must match the compiled binary pvrsrvinit or you will see e.g.:
#
# SGXInit: Mismatch in driver and microkernel build options; extra options present in driver: (0x1)
# SGXInit: Mismatch in driver and microkernel build options; extra options present in microkernel: (0x210008) [1297, drivers/gpu/pvr/omap3/services4/srvkm/devices/sgx/sgxinit.c]
#

ccflags-y += \
	-DLINUX -D__linux__ \
	-DPVR_BUILD_DIR="\"omap_sgx\"" \
	-DPVR_LDM_DRIVER_REGISTRATION_NAME="\"pvrsrvkm"\" \
	-DPVRSRV_MODNAME="\"pvrsrvkm"\" \
	-DPM_RUNTIME_SUPPORT \
	-DSERVICES4 \
	-D_XOPEN_SOURCE=600 \
	-D_POSIX_C_SOURCE=199309 \
	-DPVR2D_VALIDATE_INPUT_PARAMS \
	-DDEBUG_LOG_PATH_TRUNCATE="\"\"" \
	-DSUPPORT_SRVINIT \
	-DSUPPORT_SGX \
	-DSUPPORT_PERCONTEXT_PB \
	-DDISABLE_SGX_PB_GROW_SHRINK \
	-DSUPPORT_PVRSRV_GET_DC_SYSTEM_BUFFER \
	-DSUPPORT_LINUX_X86_WRITECOMBINE \
	-DTRANSFER_QUEUE \
	-DSUPPORT_HW_RECOVERY \
	-DPVR_SECURE_HANDLES \
	-DPVR_SECURE_DRM_AUTH_EXPORT \
	-DUSE_PTHREADS \
	-DLDM_PLATFORM \
	-DSUPPORT_SGX_EVENT_OBJECT \
	-DPVR2D_ALT_2DHW \
	-DSUPPORT_SGX_HWPERF \
	-DSUPPORT_SGX_LOW_LATENCY_SCHEDULING \
	-DSUPPORT_LINUX_X86_PAT \
	-DPVR_LINUX_USING_WORKQUEUES \
	-DPVR_LINUX_MISR_USING_PRIVATE_WORKQUEUE \
	-DPVR_LINUX_TIMERS_USING_WORKQUEUES \
	-DSYS_CUSTOM_POWERLOCK_WRAP \
	-DPVR_NO_FULL_CACHE_OPS \
	-DSGX_CLK_CORE_DIV5 \
	-DSUPPORT_SGX_NEW_STATUS_VALS \
	-DSYS_USING_INTERRUPTS \
	-DSUPPORT_ACTIVE_POWER_MANAGEMENT

# new for ddk 1.14 tree
ccflags-y += \
	-DPVR_LDM_DEVICE_TREE

ifeq ($(CONFIG_SGX_DEBUG),)
ccflags-y += \
	-DPVR_BUILD_TYPE="\"release\"" \
	-DRELEASE \
	-g -Os \
	-fno-strict-aliasing -Wno-pointer-arith -Wno-sign-conversion
else
ccflags-y += \
	-O0 \
	-DPVR_BUILD_TYPE="\"debug\"" \
	-DDEBUG \
	-DDEBUG_LINUX_MEMORY_ALLOCATIONS \
	-DDEBUG_LINUX_MEM_AREAS \
	-DDEBUG_LINUX_MMAP_AREAS \
	-DDEBUG_BRIDGE_KM \
	-g -O0 \
	-DDLL_METRIC=1 \
	-fno-strict-aliasing -Wno-pointer-arith -Wno-sign-conversion
endif

# Add rewritten driver components here for now
$(TARGET) += \
	../../pvr-drv.o \

# Keep Imagination SDK components here
$(TARGET) += \
	services4/srvkm/bridged/bridged_pvr_bridge.o \
	services4/srvkm/bridged/bridged_support.o \
	services4/srvkm/bridged/sgx/bridged_sgx_bridge.o \
	services4/srvkm/common/buffer_manager.o \
	services4/srvkm/common/devicemem.o \
	services4/srvkm/common/handle.o \
	services4/srvkm/common/hash.o \
	services4/srvkm/common/lists.o \
	services4/srvkm/common/mem.o \
	services4/srvkm/common/mem_debug.o \
	services4/srvkm/common/metrics.o \
	services4/srvkm/common/osfunc_common.o \
	services4/srvkm/common/pdump_common.o \
	services4/srvkm/common/perproc.o \
	services4/srvkm/common/power.o \
	services4/srvkm/common/pvrsrv.o \
	services4/srvkm/common/queue.o \
	services4/srvkm/common/ra.o \
	services4/srvkm/common/resman.o \
	services4/srvkm/common/refcount.o \
	services4/srvkm/devices/sgx/mmu.o \
	services4/srvkm/devices/sgx/pb.o \
	services4/srvkm/devices/sgx/sgxinit.o \
	services4/srvkm/devices/sgx/sgxkick.o \
	services4/srvkm/devices/sgx/sgxpower.o \
	services4/srvkm/devices/sgx/sgxreset.o \
	services4/srvkm/devices/sgx/sgxutils.o \
	services4/srvkm/devices/sgx/sgxtransfer.o \
	services4/srvkm/env/linux/dmabuf.o \
	services4/srvkm/env/linux/event.o \
	services4/srvkm/env/linux/mm.o \
	services4/srvkm/env/linux/mmap.o \
	services4/srvkm/env/linux/module.o \
	services4/srvkm/env/linux/mutex.o \
	services4/srvkm/env/linux/mutils.o \
	services4/srvkm/env/linux/osfunc.o \
	services4/srvkm/env/linux/osperproc.o \
	services4/srvkm/env/linux/pdump.o \
	services4/srvkm/env/linux/proc.o \
	services4/srvkm/env/linux/pvr_bridge_k.o \
	services4/srvkm/env/linux/pvr_debug.o \
	services4/srvkm/env/linux/pvr_linux_fence.o

ifneq ($(CONFIG_SGX_DRM),)

# special for OMAP5
ifneq ($(CONFIG_SOC_OMAP5),)
ccflags-y += -DCONFIG_ARCH_OMAP5	# checked for in mmap.c
endif

# special for omap5/6 according to eurasiacon/build/linux2/omap_linux/Makefile
ifeq ($(SGX),sgx544)
ccflags-y += -DSGX_FEATURE_MP
ccflags-y += -DSGX_FEATURE_MP_CORE_COUNT=2
ccflags-y += -DSGX_FEATURE_SYSTEM_CACHE	# assumed by client libs
ccflags-n += -DSYS_OMAP_HAS_DVFS_FRAMEWORK	# sgxfreq extension
# may be broken...
ccflags-y += -DAMVERSION=5
# PVR_OMAPLFB_DRM_FB			:= 0
endif

ifeq ($(SOC_FAMILY),sgx_jz4780)
ccflags-y += -DSGX_FAST_DPM_INIT
ccflags-y += -DSGX_FEATURE_SYSTEM_CACHE
ccflags-y += -DSUPPORT_ACTIVE_POWER_MANAGEMENT
endif

# gives duplicate export of drm_pvr_dev_add etc.
# $(TARGET) += services4/3rdparty/linux_drm/pvr_drm_stubs.o

ccflags-y += -I$(srctree)/include/drm \
	-I$(srctree)/$(src)/services4/3rdparty/linux_drm

ccflags-y += -DSUPPORT_HW_RECOVERY
ccflags-y += -DPVR_LINUX_USING_WORKQUEUES
ccflags-y += -DSUPPORT_DRI_DRM
ccflags-y += -DPVR_DRI_DRM_PLATFORM_DEV
ccflags-y += -DPVRSRV_NEED_PVR_DPF
ccflags-y += -DMODULE	# this disables some code in AcquireGPTimer in sysutils_linux.c
ccflags-y += -DSUPPORT_DMABUF

# these must not be set or we can't compile successfully for omap
ccflags-n += -DSUPPORT_DRI_DRM_EXT
ccflags-n += -DSUPPORT_DRI_DRM_PLUGIN
ccflags-n += -DPVR_DISPLAY_CONTROLLER_DRM_IOCTL
ccflags-n += -DPVR_DRI_DRM_NOT_PCI
ccflags-n += -DPVR_NO_OMAP_TIMER	# we have omap timers
ccflags-n += -DDISPLAY_CONTROLLER=drm	# would call drm_Init()
ccflags-n += -DSUPPORT_DRM_MODESET	# compiles, but fails with NULL deref in drm_modeset_register_all()

# unsure flags...

ccflags-n += -DODD	# there is code that seems to export PVRGetBufferClassJTable twice
ccflags-y += -DSUPPORT_PVRSRV_DEVICE_CLASS
$(TARGET) += services4/srvkm/common/deviceclass.o

else

ccflags-y += -DSYS_USING_INTERRUPTS
ccflags-y += -DPVR_DRI_DRM_NOT_PCI
ccflags-y += -DSUPPORT_PVRSRV_DEVICE_CLASS
$(TARGET) += services4/srvkm/common/deviceclass.o

endif

# this is broken and we do not have it in Kconfig
ifneq ($(SUPPORT_OMAPLFB),)

ccflags-y += \
	-I$(srctree)/$(src)/services4/3rdparty/dc_omapfb3_linux \
	-DFBDEV_PRESENT=1 \
	-DSUPPORT_OMAP3430_OMAPFB3 \
	-DDISPLAY_CONTROLLER=omaplfb \
	-DPVR_OMAPLFB_DRM_FB	# tries to #include <linux/omap_gpu.h>

# FIXME: this is also VERSION specific (different FB address for OMAP3 and AM33xx)
$(TARGET) += \
	services4/3rdparty/dc_omapfb3_linux/omaplfb_displayclass.o \
	services4/3rdparty/dc_omapfb3_linux/omaplfb_linux.o

endif

# old stuff (compile options which may no longer be available/relevant)
# CHECKME: should grep the source code if they exist and are still useful

ccflags-n += \
	-DSGX_DYNAMIC_TIMING_INFO \
	-DSUPPORT_XWS \
	-DPVR_SECURE_FD_EXPORT \
	-DSYS_CUSTOM_POWERLOCK_WRAP \
	-DSUPPORT_MEMINFO_IDS \
	-DCLIENT_DRIVER_DEFAULT_WAIT_RETRIES=50 \
	-DFLIP_TECHNIQUE_OVERLAY \
	-DSUPPORT_SGX_NEW_STATUS_VALS \
	-DSGX_EARLYSUSPEND

 # we are NOT compiling on/for Android
ccflags-n += \
	-DANDROID \
	-DSUPPORT_ANDROID_PLATFORM \
	-DSYS_SGX_ACTIVE_POWER_LATENCY_MS=100

ccflags-n += \
	-DDEBUG_BRIDGE_KM_DISPATCH_TABLE \
	-DPVRSRV_USSE_EDM_STATUS_DEBUG \
	-DPVRSRV_DUMP_MK_TRACE
