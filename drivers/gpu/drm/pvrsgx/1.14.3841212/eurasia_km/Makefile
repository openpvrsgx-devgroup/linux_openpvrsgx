
TOP = $(src)

include $(TOP)/config_kernel.mk

ccflags-y += -D__linux__ -I$(TOP) -include config_kernel.h

ccflags-y += -I$(TOP)/services4/srvkm/hwdefs \
 -I$(TOP)/include4 \
 -I$(TOP)/services4/include \
 -I$(TOP)/services4/system/$(PVR_SYSTEM) \
 -I$(TOP)/services4/system/include \
 -I$(TOP)/services4/srvkm/bridged \
 -I$(TOP)/services4/srvkm/bridged/sgx \
 -I$(TOP)/services4/srvkm/common \
 -I$(TOP)/services4/srvkm/devices/sgx \
 -I$(TOP)/services4/srvkm/env/linux \
 -I$(TOP)/services4/srvkm/include \
 -I$(TOP)/tools/intern/debug/client


OBJS += \
	services4/srvkm/env/linux/osfunc.o \
	services4/srvkm/env/linux/mutils.o \
	services4/srvkm/env/linux/mmap.o \
	services4/srvkm/env/linux/module.o \
	services4/srvkm/env/linux/pdump.o \
	services4/srvkm/env/linux/proc.o \
	services4/srvkm/env/linux/pvr_bridge_k.o \
	services4/srvkm/env/linux/pvr_debug.o \
	services4/srvkm/env/linux/mm.o \
	services4/srvkm/env/linux/mutex.o \
	services4/srvkm/env/linux/event.o \
	services4/srvkm/env/linux/osperproc.o \
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
	services4/srvkm/common/ra.o \
	services4/srvkm/common/refcount.o \
	services4/srvkm/common/resman.o \
	services4/srvkm/bridged/bridged_support.o \
	services4/srvkm/bridged/bridged_pvr_bridge.o \
	services4/system/sunxi/sysconfig.o \
	services4/system/sunxi/sysutils.o

ifeq ($(SUPPORT_PVRSRV_DEVICE_CLASS),1)
OBJS += \
	services4/srvkm/common/deviceclass.o \
	services4/srvkm/common/queue.o
endif

ifeq ($(SUPPORT_DMABUF),1)
OBJS += \
	services4/srvkm/env/linux/dmabuf.o
endif

ifeq ($(SUPPORT_ION),1)
OBJS += \
	services4/srvkm/env/linux/ion.o
endif

ifeq ($(PVR_ANDROID_NATIVE_WINDOW_HAS_SYNC),1)
OBJS += \
	services4/srvkm/env/linux/pvr_sync.o
endif

ifeq ($(TTRACE),1)
OBJS += \
	services4/srvkm/common/ttrace.o
endif

ifeq ($(SUPPORT_PVRSRV_ANDROID_SYSTRACE),1)
OBJS += \
	services4/srvkm/env/linux/systrace.o
endif

ifneq ($(W),1)
CFLAGS_osfunc.o := -Werror
CFLAGS_mutils.o := -Werror
CFLAGS_mmap.o := -Werror
CFLAGS_module.o := -Werror
CFLAGS_pdump.o := -Werror
CFLAGS_proc.o := -Werror
CFLAGS_pvr_bridge_k.o := -Werror
CFLAGS_pvr_debug.o := -Werror
CFLAGS_mm.o := -Werror
CFLAGS_mutex.o := -Werror
CFLAGS_event.o := -Werror
CFLAGS_osperproc.o := -Werror
CFLAGS_buffer_manager.o := -Werror
CFLAGS_devicemem.o := -Werror
CFLAGS_deviceclass.o := -Werror
CFLAGS_handle.o := -Werror
CFLAGS_hash.o := -Werror
CFLAGS_metrics.o := -Werror
CFLAGS_pvrsrv.o := -Werror
CFLAGS_queue.o := -Werror
CFLAGS_ra.o := -Werror
CFLAGS_resman.o := -Werror
CFLAGS_power.o := -Werror
CFLAGS_mem.o := -Werror
CFLAGS_pdump_common.o := -Werror
CFLAGS_bridged_support.o := -Werror
CFLAGS_bridged_pvr_bridge.o := -Werror
CFLAGS_perproc.o := -Werror
CFLAGS_lists.o := -Werror
CFLAGS_mem_debug.o := -Werror
CFLAGS_osfunc_common.o := -Werror
CFLAGS_refcount.o := -Werror
endif

# SUPPORT_SGX==1 only

OBJS += \
	services4/srvkm/bridged/sgx/bridged_sgx_bridge.o \
	services4/srvkm/devices/sgx/sgxinit.o \
	services4/srvkm/devices/sgx/sgxpower.o \
	services4/srvkm/devices/sgx/sgxreset.o \
	services4/srvkm/devices/sgx/sgxutils.o \
	services4/srvkm/devices/sgx/sgxkick.o \
	services4/srvkm/devices/sgx/sgxtransfer.o \
	services4/srvkm/devices/sgx/mmu.o \
	services4/srvkm/devices/sgx/pb.o

ifneq ($(W),1)
CFLAGS_bridged_sgx_bridge.o := -Werror
CFLAGS_sgxinit.o := -Werror
CFLAGS_sgxpower.o := -Werror
CFLAGS_sgxreset.o := -Werror
CFLAGS_sgxutils.o := -Werror
CFLAGS_sgxkick.o := -Werror
CFLAGS_sgxtransfer.o := -Werror
CFLAGS_mmu.o := -Werror
CFLAGS_pb.o := -Werror
endif

ccflags-y += \
        -I$(TOP)/tools/intern/debug/dbgdriv/common \
        -I$(TOP)/tools/intern/debug/client

OBJS += \
        tools/intern/debug/dbgdriv/common/dbgdriv.o \
        tools/intern/debug/dbgdriv/common/ioctl.o \
        tools/intern/debug/dbgdriv/common/handle.o \
        tools/intern/debug/dbgdriv/common/hotkey.o \
        tools/intern/debug/dbgdriv/linux/main.o \
        tools/intern/debug/dbgdriv/linux/hostfunc.o


ifeq ($(SUPPORT_DRI_DRM),1)

OBJS += \
 services4/srvkm/env/linux/pvr_drm.o

ccflags-y += \
 -Iinclude/drm \
 -I$(TOP)/services4/include/env/linux \

ifeq ($(PVR_DRI_DRM_NOT_PCI),1)
ccflags-y += -I$(TOP)/services4/3rdparty/linux_drm
endif

endif # SUPPORT_DRI_DRM

ccflags-y += \
	-I$(TOP)/services4/3rdparty/dc_sunxi \
	-Idrivers/video

OBJS += \
	services4/3rdparty/dc_sunxi/dc_sunxi_displayclass.o \

pvrsrvkm-objs := $(OBJS)

obj-m += pvrsrvkm.o
