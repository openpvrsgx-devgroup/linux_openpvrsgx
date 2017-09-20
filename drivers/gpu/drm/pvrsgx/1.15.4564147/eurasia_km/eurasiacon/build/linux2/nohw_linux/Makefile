########################################################################### ###
#@Title         Root makefile for No HW Linux.  Builds everything else.
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

include ../config/window_system.mk

PVR_SYSTEM := sgx_nohw

KERNEL_COMPONENTS := srvkm

ifeq ($(PVR_LWS_NOBC),)
 BUFFERCLASS_MODULE ?= bufferclass_example
 KERNEL_COMPONENTS += $(BUFFERCLASS_MODULE)
endif

ifeq ($(PVR_LWS_NODC),)
 DISPLAY_CONTROLLER := dcnohw
 ifeq ($(SUPPORT_DRI_DRM),1)
  EXTRA_PVRSRVKM_COMPONENTS += dc_nohw
 else
  KERNEL_COMPONENTS += dc_nohw
 endif
 DC_NOHW_WIDTH ?= 640
 DC_NOHW_HEIGHT ?= 480
else
 DRM_DISPLAY_CONTROLLER := drm_nulldisp
 KERNEL_COMPONENTS += $(DRM_DISPLAY_CONTROLLER)

 PVR_DRM_MODESET_DRIVER_NAME := nulldisp
 PVR_DRM_MODESET_MODULE_NAME := dumb
endif

PDUMP ?= 1
NO_HARDWARE := 1

ifneq ($(SGXCORE),520)
 ifneq ($(SGXCORE),5300)
  SGX_FAST_DPM_INIT ?= 1
 endif
endif

SUPPORT_SGX_HWPERF ?= 0

XORG_EXPLICIT_PVR_SERVICES_LOAD := 1

ifneq ($(strip $(KERNELDIR)),)
include ../kernel_version.mk

ifeq ($(SUPPORT_DRI_DRM),1)
 # For the benefit of DRM, the SGX driver will create a dummy platform
 # device to represent SGX.
 LDM_PCI := 0
 LDM_PLATFORM := 1
 PVR_DRI_DRM_PLATFORM_DEV := 1
 PVR_DRI_DRM_STATIC_BUS_ID := 1
 ifeq ($(call kernel-version-at-least,3,1),true)
  ifeq ($(call kernel-version-at-least,3,4),true)
   PVR_DRI_DRM_DEV_BUS_ID := "\"platform:sgxnohw:00\""
  else
   PVR_DRI_DRM_DEV_BUS_ID := "\"platform:sgxnohw:-1\""
  endif
 endif
endif
endif # KERNELDIR

# Should be last
include ../config/core.mk
include ../common/xorg.mk
include ../common/dridrm.mk
include ../common/opencl.mk
ifeq ($(PVR_LWS_NODC),)
 $(eval $(call KernelConfigC,DC_NOHW_BUFFER_WIDTH,$(DC_NOHW_WIDTH)))
 $(eval $(call KernelConfigC,DC_NOHW_BUFFER_HEIGHT,$(DC_NOHW_HEIGHT)))
endif
