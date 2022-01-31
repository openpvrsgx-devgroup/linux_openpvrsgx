########################################################################### ###
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Dual MIT/GPLv2
# 
# The contents of this file are subject to the MIT license as set out below.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# Alternatively, the contents of this file may be used under the terms of
# the GNU General Public License Version 2 ("GPL") in which case the provisions
# of GPL are applicable instead of those above.
# 
# If you wish to allow use of your version of this file only under the terms of
# GPL, and not to allow others to use your version of this file under the terms
# of the MIT license, indicate your decision by deleting the provisions above
# and replace them with the notice and other provisions required by GPL as set
# out in the file called "GPL-COPYING" included in this distribution. If you do
# not delete the provisions above, a recipient may use your version of this file
# under the terms of either the MIT license or GPL.
# 
# This License is also included in this distribution in the file called
# "MIT-COPYING".
# 
# EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
# PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
### ###########################################################################

$(call must-be-defined,$(SUPPORT_DRI_DRM))

DRM_SOURCE_DIR := drivers/gpu/drm

ccflags-y += \
	 -Iinclude/drm \
	 -I$(DRM_SOURCE_DIR)

drm-y += \
	services4/3rdparty/linux_drm/pvr_drm_stubs.o \
	external/$(DRM_SOURCE_DIR)/drm_auth.o \
	external/$(DRM_SOURCE_DIR)/drm_bufs.o \
	external/$(DRM_SOURCE_DIR)/drm_cache.o \
	external/$(DRM_SOURCE_DIR)/drm_context.o \
	external/$(DRM_SOURCE_DIR)/drm_dma.o \
	external/$(DRM_SOURCE_DIR)/drm_drawable.o \
	external/$(DRM_SOURCE_DIR)/drm_drv.o \
	external/$(DRM_SOURCE_DIR)/drm_fops.o \
	external/$(DRM_SOURCE_DIR)/drm_gem.o \
	external/$(DRM_SOURCE_DIR)/drm_ioctl.o \
	external/$(DRM_SOURCE_DIR)/drm_irq.o \
	external/$(DRM_SOURCE_DIR)/drm_lock.o \
	external/$(DRM_SOURCE_DIR)/drm_memory.o \
	external/$(DRM_SOURCE_DIR)/drm_proc.o \
	external/$(DRM_SOURCE_DIR)/drm_stub.o \
	external/$(DRM_SOURCE_DIR)/drm_vm.o \
	external/$(DRM_SOURCE_DIR)/drm_agpsupport.o \
	external/$(DRM_SOURCE_DIR)/drm_scatter.o \
	external/$(DRM_SOURCE_DIR)/ati_pcigart.o \
	external/$(DRM_SOURCE_DIR)/drm_pci.o \
	external/$(DRM_SOURCE_DIR)/drm_sysfs.o \
	external/$(DRM_SOURCE_DIR)/drm_hashtab.o \
	external/$(DRM_SOURCE_DIR)/drm_sman.o \
	external/$(DRM_SOURCE_DIR)/drm_mm.o \
	external/$(DRM_SOURCE_DIR)/drm_crtc.o \
	external/$(DRM_SOURCE_DIR)/drm_modes.o \
	external/$(DRM_SOURCE_DIR)/drm_edid.o \
	external/$(DRM_SOURCE_DIR)/drm_info.o \
	external/$(DRM_SOURCE_DIR)/drm_debugfs.o \
	external/$(DRM_SOURCE_DIR)/drm_encoder_slave.o

# extra flags for some files
CFLAGS_pvr_drm_stubs.o := -DCONFIG_PCI
CFLAGS_drm_drv.o := -DCONFIG_PCI
CFLAGS_drm_stub.o := -DCONFIG_PCI
CFLAGS_ati_pcigart.o := -DCONFIG_PCI
