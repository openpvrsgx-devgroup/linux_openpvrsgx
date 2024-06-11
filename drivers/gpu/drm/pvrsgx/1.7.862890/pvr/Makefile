# Makefile for the drm device driver.  This driver provides support for the
# Direct Rendering Infrastructure (DRI) in XFree86 4.1.0 and higher.

INCDIR=drivers/gpu/drm/pvrsgx/1.7.862890/pvr

include_dirs := \
	-I$(INCDIR)/include4 \
        -I$(INCDIR)/services4/include \
        -I$(INCDIR)/services4/include/env/linux \
        -I$(INCDIR)/services4/srvkm/env/linux \
        -I$(INCDIR)/services4/srvkm/include \
        -I$(INCDIR)/services4/srvkm/bridged \
	-I$(INCDIR)/services4/system/include \
        -I$(INCDIR)/services4/srvkm/hwdefs \
        -I$(INCDIR)/services4/srvkm/bridged/sgx \
        -I$(INCDIR)/services4/srvkm/devices/sgx \
        -I$(INCDIR)/ \
        -I$(INCDIR)/drv \
        -I$(INCDIR)/bc_video \
        -I$(INCDIR)/imgv \
	-Iinclude/linux \
	-Iinclude/drm

 
ccflags-y += $(include_dirs)

ccflags-$(CONFIG_DRM_PVR_PDUMP) += -I$(INCDIR)/tools/intern/debug/client \
			-I$(INCDIR)/tools/intern/debug/dbgdriv/common
ccflags-y += -I$(INCDIR)/services4/system/unified -DSGX545 -DSUPPORT_SGX545 -DSGX_CORE_REV=10131


ccflags-y += \
	-Wall \
	-Werror \
	-DLINUX \
	-DPVR_BUILD_DIR="\"pc_i686_cedarview_linux\"" \
	-DSUPPORT_DRI_DRM \
	-DSUPPORT_DRI_DRM_EXT \
	-DSERVICES4 \
	-D_XOPEN_SOURCE=600 \
	-DPVR2D_VALIDATE_INPUT_PARAMS \
	-DDISPLAY_CONTROLLER=mrstlfb \
	-UDEBUG_LOG_PATH_TRUNCATE \
	-DSUPPORT_SRVINIT \
	-DSUPPORT_SGX \
	-DSUPPORT_LINUX_X86_WRITECOMBINE \
	-DTRANSFER_QUEUE \
	-DSYS_USING_INTERRUPTS \
	-DSUPPORT_HW_RECOVERY \
	-DPVR_SECURE_HANDLES \
	-DUSE_PTHREADS \
	-DSUPPORT_SGX_EVENT_OBJECT \
	-DSUPPORT_SGX_HWPERF \
	-DSUPPORT_LINUX_X86_PAT \
	-DPVR_PROC_USE_SEQ_FILE \
	-DSUPPORT_CACHE_LINE_FLUSH \
	-DSUPPORT_CPU_CACHED_BUFFERS \
	-DDISABLE_PM \
	-DSUPPORT_SGX_NEW_STATUS_VALS \
	-DSUPPORT_PERCONTEXT_PB \
	-DDRM_PVR_USE_INTEL_FB

SUPPORT_EGL_EXTENSIONS ?=1
ifeq ($(SUPPORT_EGL_EXTENSIONS),1)
	PVRSRV_RESOURCE_PROFILING = y
	PERPROC_LIST = y
	RES_MAN_EXTEND = y
	PVRSRV_OS_MEM_INFO = y
else
	PVRSRV_RESOURCE_PROFILING =
        PERPROC_LIST =
        RES_MAN_EXTEND =
        PVRSRV_OS_MEM_INFO =
endif
ccflags-$(PVRSRV_RESOURCE_PROFILING) += -DPVRSRV_RESOURCE_PROFILING
ccflags-$(PERPROC_LIST) += -DPERPROC_LIST
ccflags-$(RES_MAN_EXTEND) += -DRES_MAN_EXTEND
ccflags-$(PVRSRV_OS_MEM_INFO) += -DPVRSRV_OS_MEM_INFO

ifeq ($(CONFIG_SGX_DEBUG),)
ccflags-y += -DBUILD="\"release\"" -DPVR_BUILD_TYPE="\"release\"" -DRELEASE
else
ccflags-$(CONFIG_DRM_CDV_DEBUG) += -DBUILD="\"debug\"" -DPVR_BUILD_TYPE="\"debug\"" -DDEBUG -DDEBUG_LINUX_MEM_AREAS -DDEBUG_LINUX_MEMORY_ALLOCATIONS -DDEBUG_LINUX_MMAP_AREAS -DDEBUG_BRIDGE_KM -DPVRSRV_NEED_PVR_TRACE -DDEBUG_MESA_OGL_TRACE -DPVRSRV_USSE_EDM_STATUS_DEBUG -DPVRSRV_DUMP_MK_TRACE
endif

ccflags-$(CONFIG_PCI_MSI) += -DCONFIG_PCI_MSI

ccflags-$(CONFIG_DRM_PVR_PDUMP) += -DPDUMP
ccflags-$(CONFIG_DRM_PVR_TRACE) += -DDEBUG_TRACE_BRIDGE_KM
	
ENVDIR = services4/srvkm/env/linux
COMMONDIR = services4/srvkm/common
BRIDGEDDIR = services4/srvkm/bridged
SGXDIR = services4/srvkm/devices/sgx
FBDEVDIR = services4/3rdparty/linux_framebuffer_mrst
DRMDRVDIR = drv
SYSCONFIGDIR = services4/system/unified
IMGVDIR = imgv
BUFFER_CLASS_DIR = bc_video

PDUMP_MAIN_DIR = tools/intern/debug/dbgdriv/linux
PDUMP_COMMON_DIR = tools/intern/debug/dbgdriv/common

cedarview_gfx-y += $(ENVDIR)/osfunc.o \
	  $(ENVDIR)/mutils.o \
	  $(ENVDIR)/mmap.o \
	  $(ENVDIR)/module.o \
	  $(ENVDIR)/pdump.o \
	  $(ENVDIR)/proc.o \
	  $(ENVDIR)/pvr_bridge_k.o \
	  $(ENVDIR)/pvr_debug.o \
	  $(ENVDIR)/mm.o \
	  $(ENVDIR)/event.o \
	  $(ENVDIR)/osperproc.o \
	  $(ENVDIR)/pvr_drm.o

cedarview_gfx-y += $(COMMONDIR)/buffer_manager.o \
	  $(COMMONDIR)/devicemem.o \
	  $(COMMONDIR)/deviceclass.o \
	  $(COMMONDIR)/handle.o \
	  $(COMMONDIR)/hash.o \
	  $(COMMONDIR)/metrics.o \
	  $(COMMONDIR)/pvrsrv.o \
	  $(COMMONDIR)/queue.o \
	  $(COMMONDIR)/ra.o \
	  $(COMMONDIR)/resman.o \
	  $(COMMONDIR)/power.o \
	  $(COMMONDIR)/mem.o \
	  $(COMMONDIR)/pdump_common.o \
	  $(COMMONDIR)/perproc.o \
	  $(COMMONDIR)/lists.o \
	  $(COMMONDIR)/mem_debug.o \
	  $(COMMONDIR)/osfunc_common.o

cedarview_gfx-y += $(BRIDGEDDIR)/bridged_support.o \
	  $(BRIDGEDDIR)/bridged_pvr_bridge.o \
	  $(BRIDGEDDIR)/sgx/bridged_sgx_bridge.o

cedarview_gfx-y += $(SYSCONFIGDIR)/sysconfig.o \
	  $(SYSCONFIGDIR)/sysutils.o \
	  $(SYSCONFIGDIR)/sys_pvr_drm_export.o 

cedarview_gfx-y += $(SGXDIR)/sgxinit.o \
	  $(SGXDIR)/sgxpower.o \
	  $(SGXDIR)/sgxreset.o \
	  $(SGXDIR)/sgxutils.o \
	  $(SGXDIR)/sgxkick.o \
	  $(SGXDIR)/sgxtransfer.o \
	  $(SGXDIR)/mmu.o \
	  $(SGXDIR)/pb.o

cedarview_gfx-y += $(FBDEVDIR)/mrstlfb_displayclass.o \
	  $(FBDEVDIR)/mrstlfb_linux.o

cedarview_gfx-y += $(DRMDRVDIR)/psb_bl.o \
	  $(DRMDRVDIR)/psb_dpst.o \
	  $(DRMDRVDIR)/psb_drv.o \
	  $(DRMDRVDIR)/psb_fb.o \
	  $(DRMDRVDIR)/psb_gtt.o \
	  $(DRMDRVDIR)/psb_hotplug.o \
	  $(DRMDRVDIR)/psb_intel_bios.o \
	  $(DRMDRVDIR)/psb_intel_opregion.o \
	  $(DRMDRVDIR)/psb_intel_display.o \
	  $(DRMDRVDIR)/psb_intel_i2c.o \
	  $(DRMDRVDIR)/psb_intel_lvds.o \
	  $(DRMDRVDIR)/psb_intel_modes.o \
	  $(DRMDRVDIR)/psb_intel_hdmi.o \
	  $(DRMDRVDIR)/psb_reset.o \
	  $(DRMDRVDIR)/psb_schedule.o \
	  $(DRMDRVDIR)/psb_sgx.o \
	  $(DRMDRVDIR)/psb_socket.o \
	  $(DRMDRVDIR)/psb_pvr_glue.o \
	  $(DRMDRVDIR)/psb_umevents.o \
	  $(DRMDRVDIR)/psb_intel_crt.o \
	  $(DRMDRVDIR)/psb_intel_dpll_cdv.o \
	  $(DRMDRVDIR)/psb_intel_dp.o	\
	  $(DRMDRVDIR)/psb_intel_panel.o

cedarview_gfx-y += $(IMGVDIR)/psb_buffer.o \
	  $(IMGVDIR)/psb_fence.o \
	  $(IMGVDIR)/psb_mmu.o \
	  $(IMGVDIR)/psb_msvdx.o \
	  $(IMGVDIR)/msvdx_power.o \
	  $(IMGVDIR)/psb_msvdxinit.o \
	  $(IMGVDIR)/psb_ttm_glue.o \
	  $(IMGVDIR)/psb_ttm_fence.o \
	  $(IMGVDIR)/psb_ttm_fence_user.o \
	  $(IMGVDIR)/psb_ttm_placement_user.o 

cedarview_gfx-y += $(DRMDRVDIR)/psb_powermgmt.o $(DRMDRVDIR)/psb_irq.o

cedarview_gfx-y += $(BUFFER_CLASS_DIR)/bufferclass_video.o \
                        $(BUFFER_CLASS_DIR)/bufferclass_video_linux.o

cedarview_gfx-$(CONFIG_DRM_PVR_PDUMP) += $(PDUMP_MAIN_DIR)/main.o \
					 $(PDUMP_MAIN_DIR)/hostfunc.o \
					 $(PDUMP_COMMON_DIR)/dbgdriv.o \
					 $(PDUMP_COMMON_DIR)/handle.o \
					 $(PDUMP_COMMON_DIR)/hotkey.o \
					 $(PDUMP_COMMON_DIR)/ioctl.o
					

obj-$(CONFIG_SGX_CEDARVIEW) += cedarview_gfx.o

