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
#

ifeq ($(TARGET_PRODUCT),crespo)
$(warning *** Building crespo from omap4430_android is deprecated.)
$(error Use s5pc110_android instead)
endif

ifneq ($(filter blaze blaze_tablet tuna maguro toro mysid yakju p2 u2,$(TARGET_PRODUCT)),)
ifeq ($(SUPPORT_ANDROID_COMPOSITION_BYPASS),1)
$(error SUPPORT_ANDROID_COMPOSITION_BYPASS=1 is obsolete for this product)
endif
#ifeq ($(SUPPORT_ANDROID_OMAP_NV12),1)
#SUPPORT_NV12_FROM_2_HWADDRS := 1
#endif
# These default on in tuna_defconfig
PVRSRV_USSE_EDM_STATUS_DEBUG := 1
PVRSRV_DUMP_MK_TRACE := 1
SUPPORT_SGX_HWPERF := 1
PVRSRV_NEED_PVR_DPF := 1
PVRSRV_NEED_PVR_TRACE := 1
PVRSRV_NEED_PVR_ASSERT := 1
$(info "USSE_EDM_STATUS")
# Go back to the old compiler for tuna kernel modules

KERNEL_CROSS_COMPILE := arm-eabi-
endif
