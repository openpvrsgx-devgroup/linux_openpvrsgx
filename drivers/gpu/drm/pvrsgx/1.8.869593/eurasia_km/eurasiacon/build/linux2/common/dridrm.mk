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

$(eval $(call TunableBothConfigC,SUPPORT_DRI_DRM,))
$(eval $(call TunableBothConfigC,SUPPORT_DRI_DRM_EXT,))
$(eval $(call TunableKernelConfigC,SUPPORT_DRI_DRM_PLUGIN,))


$(eval $(call TunableBothConfigMake,SUPPORT_DRI_DRM,))

ifeq ($(SUPPORT_DRI_DRM),1)
ifeq ($(SUPPORT_DRI_DRM_NO_LIBDRM),1)
endif
$(eval $(call TunableKernelConfigC,PVR_SECURE_DRM_AUTH_EXPORT,))
endif

$(eval $(call TunableKernelConfigC,PVR_DISPLAY_CONTROLLER_DRM_IOCTL,))

$(eval $(call TunableBothConfigC,PVR_DRI_DRM_NOT_PCI))
$(eval $(call TunableBothConfigMake,PVR_DRI_DRM_NOT_PCI))

$(eval $(call TunableKernelConfigC,PVR_DRI_DRM_PLATFORM_DEV,))


export EXTERNAL_3PDD_TARBALL
