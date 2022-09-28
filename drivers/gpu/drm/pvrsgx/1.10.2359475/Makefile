# derived from some SDK4 Makefiles

# although e.g. both omap3630 and am335x have the same SGX530 version
# we need to compile different kernel modules because the interfaces
# are not the same and we don't have a generic driver (yet) that
# uses DT for all TI_PLATFORM variance.

REV := generic
SGX := sgx

ifeq ($(TI_PLATFORM),omap3)
REV := 121
SGX := sgx530
ccflags-y += -DSGX530
endif
ifeq ($(TI_PLATFORM),omap3630)
REV := 125
SGX := sgx530
ccflags-y += -DSGX530
endif
ifeq ($(TI_PLATFORM),ti335x)
REV := 125
SGX := sgx530
ccflags-y += -DSGX530
endif
ifeq ($(TI_PLATFORM),omap4430)
REV := 110
SGX := sgx540
ccflags-y += -DSGX540
endif
ifeq ($(TI_PLATFORM),omap4460)
REV := 120
SGX := sgx540
ccflags-y += -DSGX540
endif
ifeq ($(TI_PLATFORM),omap4470)
REV := 100
SGX := sgx544
ccflags-y += -DSGX540
endif
ifeq ($(TI_PLATFORM),omap5)
REV := 100
SGX := sgx544
ccflags-y += -DSGX544
endif

MODULE := pvr_$(TI_PLATFORM)_$(SGX)_$(REV)
TARGET := $(MODULE)-$(CONFIG_SGX_OMAP)


obj-$(CONFIG_SGX_OMAP) += $(MODULE).o

ccflags-y += -DSGX_CORE_REV=$(REV) -DSGX_COMPATIBLE="\"ti,omap-$(TI_PLATFORM)-$(SGX)-$(REV)\""
ccflags-y += -DPVR_BUILD_DATE="\"$(shell date "+%Y%m%d" )\""

ccflags-y += \
	-Iinclude \
	-Iarch/arm/mach-omap2 \
	-Idrivers/video/fbdev/omap2 \
	-I$(src)/ \
	-I$(src)/include4 \
	-I$(src)/services4/include \
	-I$(src)/services4/include/env/linux \
	-I$(src)/services4/system/include \
	-I$(src)/services4/system/$(TI_PLATFORM) \
	-I$(src)/services4/srvkm/bridged \
	-I$(src)/services4/srvkm/bridged/sgx \
	-I$(src)/services4/srvkm/devices/sgx \
	-I$(src)/services4/srvkm/env/linux \
	-I$(src)/services4/srvkm/hwdefs \
	-I$(src)/services4/srvkm/include \
	-I$(src)/services4/3rdparty/bufferclass_ti

#
# many of the following options must match the compiled binary pvrsrvinit or you will see e.g.:
#
# SGXInit: Mismatch in driver and microkernel build options; extra options present in driver: (0x1)
# SGXInit: Mismatch in driver and microkernel build options; extra options present in microkernel: (0x210008) [1297, drivers/gpu/pvr/omap3/services4/srvkm/devices/sgx/sgxinit.c]
#

ccflags-y += \
	-DLINUX -D__linux__ \
	-DPVR_BUILD_DIR="\"omap_sgx\"" \
	-DPVRSRV_MODNAME="\"pvrsrvkm"\" \
	-DPVR_USE_PRE_REGISTERED_PLATFORM_DEV \
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
	-DPVR_DRI_DRM_NOT_PCI \
	-DSYS_USING_INTERRUPTS \
	-DSUPPORT_HW_RECOVERY \
	-DSUPPORT_ACTIVE_POWER_MANAGEMENT \
	-DPVR_SECURE_HANDLES \
	-DPVR_SECURE_DRM_AUTH_EXPORT \
	-DUSE_PTHREADS \
	-DSUPPORT_SGX_EVENT_OBJECT \
	-DLDM_PLATFORM \
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
	-DSUPPORT_SGX_NEW_STATUS_VALS

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

$(TARGET) := \
	services4/srvkm/bridged/bridged_pvr_bridge.o \
	services4/srvkm/bridged/bridged_support.o \
	services4/srvkm/bridged/sgx/bridged_sgx_bridge.o \
	services4/srvkm/common/buffer_manager.o \
	services4/srvkm/common/devicemem.o \
	services4/srvkm/common/deviceclass.o \
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
	services4/srvkm/env/linux/pvr_drm.o \
	services4/system/$(TI_PLATFORM)/sysconfig.o \
	services4/system/$(TI_PLATFORM)/sysutils.o

ifneq ($(CONFIG_SGX_DRM),)

ccflags-y += -Iinclude/drm \
	-I$(src)/services4/3rdparty/linux_drm \
	-DSYS_USING_INTERRUPTS \
	-DPVR_LINUX_USING_WORKQUEUES \
	-DPVR_DISPLAY_CONTROLLER_DRM_IOCTL \
	-DSUPPORT_DRM_MODESET \
	-DSUPPORT_DRI_DRM_EXT \
	-DSUPPORT_DRI_DRM	# FIXME: leads to $(src)/services4/srvkm/env/linux/pvr_drm.c:448:1: error: size of array is too large

ccflags-n += \
	-DPVR_DRI_DRM_PLATFORM_DEV

$(TARGET) += \
	services4/3rdparty/linux_drm/pvr_drm_stubs.o

endif

ifneq ($(SUPPORT_OMAPLFB),)

ccflags-y += \
	-I$(src)/services4/3rdparty/dc_omapfb3_linux \
	-DFBDEV_PRESENT=1 \
	-DSUPPORT_OMAP3430_OMAPFB3 \
	-DDISPLAY_CONTROLLER=omaplfb \
	-DPVR_OMAPLFB_DRM_FB	# tries to #include <linux/omap_gpu.h>

# FIXME: this is also TI_PLATFORM specific (different FB address for OMAP3 and AM33xx)
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
