########################################################################### ###
#@Title         Root makefile for OMAP4430 Linux.  Builds everything else.
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

PVR_SYSTEM		:= omap4

KERNEL_COMPONENTS := srvkm

# Confirm kernel version with KERNELDIR if supplied, otherwise assume
# a 2.6.35+ kernel.
ifneq ($(KERNELDIR),)
include ../kernel_version.mk
OMAP_KERNEL_AT_LEAST_2_6_35 := $(shell test $(KERNEL_VERSION) -ge 3 || \
			test $(KERNEL_VERSION) -ge 2 -a \
			$(KERNEL_PATCHLEVEL) -ge 6 -a \
			$(KERNEL_SUBLEVEL) -ge 35 && echo 1 || echo 0)
else
OMAP_KERNEL_AT_LEAST_2_6_35 := 1
endif

include ../common/xorg_test.mk

ifeq ($(want_xorg),1)
SUPPORT_DRI_DRM := 1
SUPPORT_DRI_DRM_EXTERNAL := 1
SUPPORT_BC_EXAMPLE ?= 0
else
SUPPORT_BC_EXAMPLE ?= 1
endif

# Only enable active power management if passive power management is
# enabled, as indicated by LDM_PLATFORM being set to 1.  On OMAP,
# the system can suspend in the case where active power management is
# enabled in the SGX driver, but passive power management isn't. As
# passive power management isn't enabled, the driver won't see the
# system suspend/resume events, and so won't take appropriate action.
LDM_PLATFORM ?= 1

ifeq ($(LDM_PLATFORM),1)
SUPPORT_LINUX_USING_WORKQUEUES := 1
#DISPLAY_CONTROLLER_COMPONENT += dc_omapdrm_linux
#DISPLAY_CONTROLLER := omaplfb
else
SUPPORT_LINUX_USING_SHARED_WORKQUEUES := 1
OMAP_NON_FLIP_DISPLAY := 1
DISPLAY_CONTROLLER_COMPONENT += linux_framebuffer
DISPLAY_CONTROLLER := pvrlfb
endif

# When flipping is enabled, the Framebuffer API will be used to
# present the frames to the display in the 3PDD
FLIP_TECHNIQUE_FRAMEBUFFER := 1

OPTIM := -Os

SYS_CFLAGS := -march=armv7-a

ifneq ($(CROSS_COMPILE),)
SYS_CFLAGS += -mtls-dialect=arm
endif

SUPPORT_OMAP4430_NEON ?= 1

ifeq ($(SUPPORT_OMAP4430_NEON),1)
SYS_CFLAGS += -ftree-vectorize -mfpu=neon -mfloat-abi=hard
endif

PVR_NO_FULL_CACHE_OPS := 1

LIBGCC := $(shell $(CROSS_COMPILE)gcc -print-libgcc-file-name)

SGXCORE := 540
SGX_CORE_REV := 120

SGX_DYNAMIC_TIMING_INFO := 1
SYS_CUSTOM_POWERLOCK_WRAP := 1

ifeq ($(OMAP_KERNEL_AT_LEAST_2_6_35),1)
ifeq ($(LDM_PLATFORM),1)
PVR_LDM_PLATFORM_PRE_REGISTERED := 1
ifeq ($(call kernel-version-at-least,2,6,35,7),true)
# Not all variants of the OMAP4 kernel have a DRM based framebuffer.
# Note that a non-X.Org version of the driver does not have to be built
# with DRM support if the kernel has a DRM based framebuffer.
PVR_OMAPLFB_DRM_FB ?= 1
ifeq ($(PVR_OMAPLFB_DRM_FB),1)
PVR_LDM_PLATFORM_PRE_REGISTERED_DEV := "\"pvrsrvkm_pvr\""
# There is already a "pvrsrvkm" driver (part of the DRM framebuffer code),
# so use the pre-registered device name instead.
PVR_LDM_DRIVER_REGISTRATION_NAME := "\"pvrsrvkm_pvr"\"
# The DRM library will not load the Services module on behalf of the X Server,
# as a DRM module has already been loaded (the DRM based framebuffer), so
# load the Services module before starting the X Server.
XORG_EXPLICIT_PVR_SERVICES_LOAD := 1
else
PVR_LDM_PLATFORM_PRE_REGISTERED_DEV := "\"pvrsrvkm\""
endif
endif
endif
endif

BUILD_OPENCL ?= 0
ifneq ($(BUILD_OPENCL),1)
EXCLUDED_APIS += opencl
endif

ifeq ($(want_xorg),1)

ifeq ($(call kernel-version-at-least,2,6,35),true)
PVR_DRI_DRM_PLATFORM_DEV := 1
else
PVR_DRI_DRM_NOT_PCI := 1
endif

XORG_PREFIX := /usr
XORG_TOOLCHAIN := extern
XORG_PVR_CONF := omap4
XORG_PVR_VIDEO := omap4
XF86_VIDEO ?= ti-pvr_video

OPK_DEFAULT := libpvrPVR2D_DRIWSEGL.so
OPK_FALLBACK := libpvrPVR2D_DRIWSEGL.so

ifneq ($(OMAP_NON_FLIP_DISPLAY),1)
XORG_PVR_VIDEO ?= $(PVR_SYSTEM)
PVR_DISPLAY_CONTROLLER_DRM_IOCTL := 1
endif

else  # xorg isn't excluded

ifeq ($(OMAP_NON_FLIP_DISPLAY),1)
OPK_DEFAULT := libpvrPVR2D_BLITWSEGL.so
else
OPK_DEFAULT := libpvrPVR2D_FLIPWSEGL.so
endif
OPK_FALLBACK := libpvrPVR2D_BLITWSEGL.so

endif # xorg isn't excluded

ifeq ($(SUPPORT_DRI_DRM),1)
ifeq ($(PVR_DRI_DRM_NOT_PCI),1)
KERNEL_COMPONENTS += linux_drm
EXTRA_KBUILD_SOURCE := $(KERNELDIR)
endif
EXTRA_PVRSRVKM_COMPONENTS += $(DISPLAY_CONTROLLER_COMPONENT)
else
KERNEL_COMPONENTS += $(DISPLAY_CONTROLLER_COMPONENT)
endif

SUPPORT_V4L2_GFX ?= 0

SUPPORT_MEMINFO_IDS ?= 1

SUPPORT_MESA_SONAME ?= 1

SUPPORT_MEMINFO_IDS ?= 1

ifeq ($(SUPPORT_V4L2_GFX),1)
KERNEL_COMPONENTS += v4l2-gfx
endif

ifeq ($(SUPPORT_BC_EXAMPLE),1)
KERNEL_COMPONENTS += bufferclass_example
endif

PVRSRV_MODNAME := omapdrm_pvr

PVRSRV_BUSID := platform:omapdrm:00

include ../config/core.mk
include ../common/xorg.mk
include ../common/dridrm.mk
include ../common/opencl.mk
include ../common/omap4.mk

# We only need this for pvr_video's includes, which should
# really be done differently, as DISPLAY_CONTROLLER_DIR is
# now obsolete..
#
$(eval $(call UserConfigMake,DISPLAY_CONTROLLER_DIR,3rdparty/$(DISPLAY_CONTROLLER_COMPONENT)))
