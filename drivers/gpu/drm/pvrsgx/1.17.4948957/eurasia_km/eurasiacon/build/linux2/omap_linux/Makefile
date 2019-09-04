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

include ../config/window_system.mk

# Customize this build as per the TARGET_PRODUCT setting
ifneq ($(filter ti335x,$(TARGET_PRODUCT)),)
 SGXCORE                        := 530
 SGX_CORE_REV                   := 125

 # SoC Version
 AM_VERSION                     := 3
else ifneq ($(filter ti437x,$(TARGET_PRODUCT)),)
 SGXCORE                        := 530
 SGX_CORE_REV                   := 125

 # SoC Version
 AM_VERSION                     := 4
else ifneq ($(filter jacinto6evm omap5uevm,$(TARGET_PRODUCT)),)
 SGXCORE                        := 544
 SGX_CORE_REV                   := 116
 SGX_FEATURE_MP                 := 1
 SGX_FEATURE_SYSTEM_CACHE       := 1
 SGX_FEATURE_MP_CORE_COUNT      := 2

 # SoC Version
 AM_VERSION                     := 5
else ifneq ($(filter ti654x,$(TARGET_PRODUCT)),)
 SGXCORE                        := 544
 SGX_CORE_REV                   := 117
 SGX_FEATURE_MP                 := 1
 SGX_FEATURE_SYSTEM_CACHE       := 1
 SGX_FEATURE_MP_CORE_COUNT      := 1

 # SoC Version
 AM_VERSION                     := 6
else
 # If a product wasn't specified, error out to prevent building for the wrong platform
 $(warning *** Unrecognised or empty TARGET_PRODUCT: $(TARGET_PRODUCT))
 $(error *** Supported Target Systems are: ti335x ti437x jacinto6evm omap5uevm ti654x)
endif

PVRSRV_NEED_PVR_DPF := 1
PVRSRV_NEW_PVR_DPF := 1
#PVRSRV_NEED_PVR_TRACE := 1
PVR_LWS_NOBC := 1

# Disable active power management
SUPPORT_ACTIVE_POWER_MANAGEMENT 	:= 0

# Disable Device Class
SUPPORT_PVRSRV_DEVICE_CLASS := 0

PVR_NO_OMAP_TIMER := 1

PVR_SYSTEM		:= omap

KERNEL_COMPONENTS := srvkm
ifeq ($(PVR_LWS_NOBC),)
BUFFERCLASS_MODULE := bufferclass_example
KERNEL_COMPONENTS += $(BUFFERCLASS_MODULE)
endif

SUPPORT_OLD_ION_API := 1

# Only enable active power management if passive power management is
# enabled, as indicated by LDM_PLATFORM being set to 1.  On OMAP,
# the system can suspend in the case where active power management is
# enabled in the SGX driver, but passive power management isn't. As
# passive power management isn't enabled, the driver won't see the
# system suspend/resume events, and so won't take appropriate action.
LDM_PLATFORM ?= 1

ifeq ($(LDM_PLATFORM),1)
SUPPORT_LINUX_USING_WORKQUEUES := 1
#With DRM EGL and display, omaplfb is not needed
#DISPLAY_CONTROLLER_COMPONENT += dc_omapfb3_linux
#DISPLAY_CONTROLLER := omaplfb
else
SUPPORT_LINUX_USING_SHARED_WORKQUEUES := 1
OMAP_NON_FLIP_DISPLAY := 1
DISPLAY_CONTROLLER_COMPONENT += linux_framebuffer
DISPLAY_CONTROLLER := pvrlfb
endif

ifeq ($(ARCH), arm64)
#find the right number
  SYS_CFLAGS := -march=armv8-a
else
  SYS_CFLAGS := -march=armv7-a
endif

UBUNTU_TOOLCHAIN :=
ifeq ($(CROSS_COMPILE),)
UBUNTU_TOOLCHAIN = 1
endif
ifeq ($(CROSS_COMPILE),arm-linux-gnueabi-)
UBUNTU_TOOLCHAIN = 1
endif
ifeq ($(CROSS_COMPILE),arm-linux-gnueabihf-)
UBUNTU_TOOLCHAIN = 1
endif

ifeq ($(UBUNTU_TOOLCHAIN),)
OPTIM := -Os

ifneq ($(CROSS_COMPILE),)
#check whether it should be trad
ifeq ($(ARCH), arm64)
SYS_CFLAGS += -mtls-dialect=desc
SUPPORT_OMAP4430_NEON ?= 0
else
SYS_CFLAGS += -mtls-dialect=arm
SUPPORT_OMAP4430_NEON ?= 1
endif
endif


ifeq ($(SUPPORT_OMAP4430_NEON),1)
SYS_CFLAGS += -ftree-vectorize -mfpu=neon -mfloat-abi=softfp
endif
endif	# UBUNTU_TOOLCHAIN

LIBGCC := $(shell $(CROSS_COMPILE)gcc -print-libgcc-file-name)

SGX_DYNAMIC_TIMING_INFO := 1
SYS_CUSTOM_POWERLOCK_WRAP := 1

ifeq ($(OMAP_NON_FLIP_DISPLAY),1)
OPK_DEFAULT := libpvrPVR2D_BLITWSEGL.so
else
OPK_DEFAULT := libpvrPVR2D_FLIPWSEGL.so
endif

include ../common/xorg_test.mk

ifneq ($(strip $(KERNELDIR)),)
include ../kernel_version.mk

XORG_EXPLICIT_PVR_SERVICES_LOAD := 1
SUPPORT_DRI_DRM := 1
LDM_PLATFORM := 1
PVR_LDM_DEVICE_TREE := 1
PVR_DRI_DRM_PLATFORM_DEV := 1
SUPPORT_DMABUF := 1
# No display class driver.
DISPLAY_CONTROLLER_COMPONENT :=
DISPLAY_CONTROLLER :=
ifeq ($(PVR_LWS_NODC),1)
# No display class driver.
DISPLAY_CONTROLLER :=
else
PVR_OMAPLFB_DRM_FB := 0
PVR_OMAPLFB_DONT_USE_FB_PAN_DISPLAY := 1
endif

PVR_DRI_DRM_PLATFORM_DEV := 1

ifeq ($(PVR_LWS_NODC),)
ifeq ($(SUPPORT_BUILD_XORG),1)
# A client DRI authorisation failure, whilst switched away from the X Server
# VT, prevents all other attempts at DRI authorisation, even after
# switching back to the X server VT, so don't perform a DRM drop master
# call.
PVR_XORG_DONT_DROP_MASTER_IN_LEAVE_VT := 1
XORG_PVR_VIDEO ?= omap4

ifneq ($(OMAP_NON_FLIP_DISPLAY),1)
PVR_DISPLAY_CONTROLLER_DRM_IOCTL := 1
endif
endif # xorg isn't excluded

ifeq ($(PVR_DRI_DRM_NOT_PCI),1)
KERNEL_COMPONENTS += linux_drm
EXTRA_KBUILD_SOURCE := $(KERNELDIR)
endif
KERNEL_COMPONENTS += $(DISPLAY_CONTROLLER)
endif	# PVR_LWS_NODC
endif	# KERNELDIR

include ../config/core.mk
include ../common/xorg.mk
include ../common/dridrm.mk
include ../common/opencl.mk
include ../common/omap4.mk

$(eval $(call TunableKernelConfigC,SYS_OMAP_HAS_DVFS_FRAMEWORK,))

$(eval $(call TunableUserConfigC,PVR_MUTEXES_COND_USING_PTHREAD_CONDVARS,1))
