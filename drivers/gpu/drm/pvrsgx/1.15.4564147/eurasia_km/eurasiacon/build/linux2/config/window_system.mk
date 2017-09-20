########################################################################### ###
#@File
#@Title         Select a window system
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

-include ../config/default_window_system.mk
WINDOW_SYSTEM ?= ews

_window_systems := nullws nulldrmws ews wayland xorg surfaceless lws-generic

_unrecognised_window_system := $(strip $(filter-out $(_window_systems),$(WINDOW_SYSTEM)))
ifneq ($(_unrecognised_window_system),)
 $(warning *** Unrecognised WINDOW_SYSTEM: $(_unrecognised_window_system))
 $(warning *** WINDOW_SYSTEM was set via: $(origin WINDOW_SYSTEM))
 $(error Supported Window Systems are: $(_window_systems))
endif

# Set some defaults.
MESA_EGL :=
PVR_LWS_NODC :=

ifeq ($(WINDOW_SYSTEM),xorg)
 MESA_EGL := 1
 PVR_LWS_NODC := 1
 SUPPORT_DRI_DRM := 1
else ifeq ($(WINDOW_SYSTEM),wayland)
 MESA_EGL := 1
 SUPPORT_DRI_DRM := 1
 PVR_LWS_NODC := 1
else ifeq ($(WINDOW_SYSTEM),surfaceless)
 MESA_EGL := 1
 SUPPORT_DRI_DRM := 1
 PVR_LWS_NODC := 1
else ifeq ($(WINDOW_SYSTEM),lws-generic)
 MESA_EGL := 1
 SUPPORT_DRI_DRM := 1
 PVR_LWS_NODC := 1
else ifeq ($(WINDOW_SYSTEM),ews)
 OPK_DEFAULT  := libpvrEWS_WSEGL.so
 OPK_FALLBACK := libpvrPVR2D_FLIPWSEGL.so
else ifeq ($(WINDOW_SYSTEM),nullws)
 OPK_DEFAULT  := libpvrPVR2D_FLIPWSEGL.so
 OPK_FALLBACK := libpvrPVR2D_BLITWSEGL.so
else ifeq ($(WINDOW_SYSTEM),nulldrmws)
 OPK_DEFAULT  := libpvrDRMWSEGL.so
 OPK_FALLBACK := libpvrDRMWSEGL.so
 SUPPORT_DRI_DRM := 1
 PVR_LWS_NODC := 1
endif

ifeq ($(MESA_EGL),1)
 OGLES1_BASENAME := GLESv1_PVR_MESA
 OGLES2_BASENAME := GLESv2_PVR_MESA
endif

# Mesa supports external EGL images, which excludes IMG texture stream
# support.
PVR_LWS_NOBC := $(MESA_EGL)

ifeq ($(PVR_LWS_NODC),1)
 SUPPORT_DMABUF := $(SUPPORT_DRI_DRM)
endif
