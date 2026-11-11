#
# Copyright (C) Imagination Technologies Ltd. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
# 
# This program is distributed in the hope it will be useful but, except 
# as otherwise stated in writing, without any warranty; without even the 
# implied warranty of merchantability or fitness for a particular purpose. 
# See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
# 
# The full GNU General Public License is included in this distribution in
# the file called "COPYING".
#
# Contact Information:
# Imagination Technologies Ltd. <gpl-support@imgtec.com>
# Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
# 

pvrsrvkm_sgx$(SGXCORE)_$(SGX_CORE_REV)-y += \
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
	services4/srvkm/env/linux/sysfs.o \
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
	services4/srvkm/common/refcount.o \
	services4/srvkm/common/resman.o \
	services4/srvkm/bridged/bridged_support.o \
	services4/srvkm/bridged/bridged_pvr_bridge.o \
	services4/system/$(PVR_SYSTEM)/sysconfig.o \
	services4/system/$(PVR_SYSTEM)/sysutils.o

pvrsrvkm_sgx$(SGXCORE)_$(SGX_CORE_REV)-$(CONFIG_ION_OMAP) += \
	services4/srvkm/env/linux/ion.o
pvrsrvkm_sgx$(SGXCORE)_$(SGX_CORE_REV)-$(CONFIG_GCBV) += \
	services4/srvkm/env/linux/gc_bvmapping.o

ifeq ($(TTRACE),1)
pvrsrvkm_sgx$(SGXCORE)_$(SGX_CORE_REV)-y += \
	services4/srvkm/common/ttrace.o
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

pvrsrvkm_sgx$(SGXCORE)_$(SGX_CORE_REV)-y += \
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

ifeq ($(SUPPORT_DRI_DRM),1)

pvrsrvkm_sgx$(SGXCORE)_$(SGX_CORE_REV)-y += \
 services4/srvkm/env/linux/pvr_drm.o

ccflags-y += \
 -I$(KERNELDIR)/include/drm \
 -I$(TOP)/services4/include/env/linux \

ifeq ($(PVR_DRI_DRM_NOT_PCI),1)
ccflags-y += -I$(TOP)/services4/3rdparty/linux_drm
endif

endif # SUPPORT_DRI_DRM
