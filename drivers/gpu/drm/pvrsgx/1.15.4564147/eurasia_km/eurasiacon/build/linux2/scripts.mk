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

define if-component
ifneq ($$(filter $(1),$$(COMPONENTS)),)
M4DEFS += $(2)
endif
endef

# common.m4 lives here
#
M4FLAGS := -I$(MAKE_TOP)/scripts

# The driver version is required to rename libraries
#
include $(MAKE_TOP)/pvrversion.mk

# These defs are either obsolete features, or not derived from
# user variables
#
M4DEFS := \
 -DKM_SUFFIX=ko \
 -DLIB_SUFFIX=so \
 -DSUPPORT_SRVINIT=1 \
 -DPVRVERSION="$(PVRVERSION)" \
 -DMOD_ROOTDIR=$(MOD_ROOTDIR) \
 -DMOD_DESTDIR=$(patsubst %/,%,$(PVRSRV_MODULE_BASEDIR)) \
 -DSOLIB_VERSION=$(PVRVERSION_MAJ).$(PVRVERSION_MIN).$(PVRVERSION_BUILD) \
 -DSHLIB_DESTDIR=$(SHLIB_DESTDIR) \
 -DDEMO_DESTDIR=$(DEMO_DESTDIR) \
 -DHEADER_DESTDIR=$(HEADER_DESTDIR) \
 -DEGL_DESTDIR=$(EGL_DESTDIR) \
 -DSUPPORT_UNITTESTS=1

ifeq ($(MESA_EGL),1)
 M4DEFS += -DSUPPORT_MESA=1
else
 ifneq ($(filter xorg,$(COMPONENTS)),)
  ifneq ($(filter opengl,$(COMPONENTS)),)
   M4DEFS += -DSUPPORT_MESA=1
  endif
 endif
endif

ifeq ($(SUPPORT_BUILD_LWS),1)
 ifneq ($(LWS_NATIVE),1)
  M4DEFS += -DLWS_INSTALL_TREE=1
 endif
endif

ifeq ($(PVR_LWS_NOBC),1)
 M4DEFS += -DNO_BUFFER_CLASS_MODULE=1
endif

ifneq ($(DRM_DISPLAY_CONTROLLER),)
 M4DEFS += \
	-DHAVE_DRM_DISPLAY_MODULE=1 \
	-DDISPLAY_KERNEL_MODULE=$(DRM_DISPLAY_CONTROLLER)
else
 M4DEFS += -DDISPLAY_KERNEL_MODULE=$(DISPLAY_CONTROLLER)
endif

# Map COMPONENTS on to SUPPORT_ defs
#
$(eval $(call if-component,opengles1,\
 -DSUPPORT_OPENGLES1=1 -DOGLES1_MODULE=$(opengles1_target) \
 -DSUPPORT_OPENGLES1_V1_ONLY=$(if $(SUPPORT_OPENGLES1_V1_ONLY),1,0)))
$(eval $(call if-component,opengles2,\
 -DSUPPORT_OPENGLES2=1 -DOGLES2_MODULE=$(opengles2_target)))
$(eval $(call if-component,egl,\
 -DSUPPORT_LIBEGL=1 -DEGL_MODULE=$(egl_target)))
$(eval $(call if-component,pvr2d,\
 -DSUPPORT_LIBPVR2D=1))
$(eval $(call if-component,glslcompiler,\
 -DSUPPORT_SOURCE_SHADER=1))
$(eval $(call if-component,opencl,\
 -DSUPPORT_OPENCL=1))
$(eval $(call if-component,rscompute,\
 -DSUPPORT_RSCOMPUTE=1))
$(eval $(call if-component,opengl,\
 -DSUPPORT_OPENGL=1))
$(eval $(call if-component,null_pvr2d_flip,\
 -DSUPPORT_NULL_PVR2D_FLIP=1))
$(eval $(call if-component,null_pvr2d_blit,\
 -DSUPPORT_NULL_PVR2D_BLIT=1))
$(eval $(call if-component,null_pvr2d_front,\
 -DSUPPORT_NULL_PVR2D_FRONT=1))
$(eval $(call if-component,null_pvr2d_linuxfb,\
 -DSUPPORT_NULL_PVR2D_LINUXFB=1))
$(eval $(call if-component,null_drm_ws,\
 -DSUPPORT_NULL_DRM_WS=1))
$(eval $(call if-component,ews_ws,\
 -DSUPPORT_EWS=1))
$(eval $(call if-component,pdump,\
 -DPDUMP=1))
$(eval $(call if-component,imgtcl,\
 -DSUPPORT_IMGTCL=1))
$(eval $(call if-component,ews_wm,\
 -DSUPPORT_LUA=1))
$(eval $(call if-component,xmultiegltest,\
 -DSUPPORT_XUNITTESTS=1))
$(eval $(call if-component,pvr_conf,\
 -DSUPPORT_XORG_CONF=1))
$(eval $(call if-component,graphicshal,\
 -DSUPPORT_GRAPHICS_HAL=1))
$(eval $(call if-component,xorg,\
 -DSUPPORT_XORG=1 \
 -DXORG_DIR=$(LWS_PREFIX) \
 -DPVR_DDX_DESTDIR=$(LWS_PREFIX)/lib/xorg/modules/drivers \
 -DPVR_DRI_DESTDIR=$(LWS_PREFIX)/lib/dri \
 -DPVR_DDX_INPUT_DESTDIR=$(LWS_PREFIX)/lib/xorg/modules/input \
 -DXORG_EXPLICIT_PVR_SERVICES_LOAD=$(XORG_EXPLICIT_PVR_SERVICES_LOAD)))
$(eval $(call if-component,surfaceless,\
 -DSUPPORT_SURFACELESS=1 \
 -DPVR_DRI_DESTDIR=$(LWS_PREFIX)/lib/dri))
$(eval $(call if-component,wl,\
 -DSUPPORT_WAYLAND=1 \
 -DPVR_DRI_DESTDIR=$(LWS_PREFIX)/lib/dri))

# These defs are common to all driver builds, and inherited from config.mk
#
M4DEFS += \
 -DBUILD=$(BUILD) \
 -DPVR_BUILD_DIR=$(PVR_BUILD_DIR) \
 -DPVRSRV_MODNAME=$(PVRSRV_MODNAME) \
 -DPROFILE_COMMON=1 \
 -DFFGEN_UNIFLEX=1 \
 -DSUPPORT_SGX_HWPERF=$(SUPPORT_SGX_HWPERF) \

# These are common to some builds, and inherited from config.mk
#
ifeq ($(SUPPORT_DRI_DRM),1)
M4DEFS += -DSUPPORT_DRI_DRM=1 -DSUPPORT_DRI_DRM_NOT_PCI=$(PVR_DRI_DRM_NOT_PCI)
ifeq ($(PVR_DRI_DRM_NOT_PCI),1)
M4DEFS += -DDRM_MODNAME=drm
endif
endif

ifeq ($(SUPPORT_ANDROID_PLATFORM),1)
M4DEFS += \
 -DSUPPORT_ANDROID_PLATFORM=$(SUPPORT_ANDROID_PLATFORM) \
 -DGRALLOC_MODULE=gralloc.$(HAL_VARIANT).so \
 -DHWCOMPOSER_MODULE=hwcomposer.$(HAL_VARIANT).so \
 -DCAMERA_MODULE=camera.$(HAL_VARIANT).so \
 -DSENSORS_MODULE=sensors.$(HAL_VARIANT).so \
 -DMEMTRACK_MODULE=memtrack.$(HAL_VARIANT).so
endif

ifeq ($(PVR_REMVIEW),1)
M4DEFS += -DPVR_REMVIEW=1
endif

ifneq ($(wildcard $(MAKE_TOP)/$(PVR_BUILD_DIR)/install.sh.m4),)
# Build using old scheme where M4 built entire install.sh

$(TARGET_OUT)/install.sh: \
 $(PVRVERSION_H) $(CONFIG_MK) $(CONFIG_KERNEL_MK) \
 $(MAKE_TOP)/scripts/install.sh.m4 \
 $(MAKE_TOP)/$(PVR_BUILD_DIR)/install.sh.m4 \
 | $(TARGET_OUT)
	$(if $(V),,@echo "  GEN     " $(call relative-to-top,$@))
	$(M4) $(M4FLAGS) $(M4DEFS) \
		$(MAKE_TOP)/scripts/install.sh.m4 \
		$(MAKE_TOP)/$(PVR_BUILD_DIR)/install.sh.m4 > $@
	$(CHMOD) +x $@
install_script: $(TARGET_OUT)/install.sh
install_script_km:
else
# Build using new scheme where M4 just builds KM/UM specific portions of the script.
ifneq ($(SUPPORT_ANDROID_PLATFORM),)
$(TARGET_OUT)/install.sh: $(PVRVERSION_H) $(MAKE_TOP)/common/android/install.sh.tpl | $(TARGET_OUT)
	$(if $(V),,@echo "  GEN     " $(call relative-to-top,$@))
	@sed 's/\[PVRVERSION\]/$(subst /,\/,$(PVRVERSION))/g' $(MAKE_TOP)/common/android/install.sh.tpl > $@
	$(CHMOD) +x $@
else
$(TARGET_OUT)/install.sh: $(PVRVERSION_H) $(MAKE_TOP)/scripts/install.sh.tpl | $(TARGET_OUT)
	$(if $(V),,@echo "  GEN     " $(call relative-to-top,$@))
	@sed 's/\[PVRVERSION\]/$(subst /,\/,$(PVRVERSION))/g' $(MAKE_TOP)/scripts/install.sh.tpl > $@
	$(CHMOD) +x $@
endif #ifeq ($(SUPPORT_ANDROID_PLATFORM),1)

install_script: $(TARGET_OUT)/install.sh
install_script_km: $(TARGET_OUT)/install.sh

ifneq ($(wildcard $(MAKE_TOP)/$(PVR_BUILD_DIR)/install_km.sh.m4),)
# Need to install KM files
$(TARGET_OUT)/install_km.sh: $(PVRVERSION_H) $(CONFIG_KERNEL_MK) \
 $(MAKE_TOP)/scripts/common.m4 \
 $(MAKE_TOP)/$(PVR_BUILD_DIR)/install_km.sh.m4 \
 | $(TARGET_OUT)
	$(if $(V),,@echo "  GEN     " $(call relative-to-top,$@))
	$(M4) $(M4FLAGS) $(M4DEFS) \
	  $(MAKE_TOP)/scripts/common.m4 \
	  $(MAKE_TOP)/$(PVR_BUILD_DIR)/install_km.sh.m4 > $@

install_script_km: $(TARGET_OUT)/install_km.sh
endif

ifneq ($(wildcard $(MAKE_TOP)/$(PVR_BUILD_DIR)/install_um.sh.m4),)
# Need to install UM files
$(TARGET_OUT)/install_um.sh: $(PVRVERSION_H) $(CONFIG_MK)\
 $(MAKE_TOP)/scripts/common.m4 \
 $(MAKE_TOP)/$(PVR_BUILD_DIR)/install_um.sh.m4 \
 | $(TARGET_OUT)
	$(if $(V),,@echo "  GEN     " $(call relative-to-top,$@))
	$(M4) $(M4FLAGS) $(M4DEFS) \
	  $(MAKE_TOP)/scripts/common.m4 \
	  $(MAKE_TOP)/$(PVR_BUILD_DIR)/install_um.sh.m4 > $@

install_script: $(TARGET_OUT)/install_um.sh
endif

endif

$(TARGET_OUT)/rc.pvr: \
 $(PVRVERSION_H) $(CONFIG_MK) $(CONFIG_KERNEL_MK) \
 $(MAKE_TOP)/scripts/rc.pvr.m4 $(MAKE_TOP)/$(PVR_BUILD_DIR)/rc.pvr.m4 \
 | $(TARGET_OUT)
	$(if $(V),,@echo "  GEN     " $(call relative-to-top,$@))
	$(M4) $(M4FLAGS) $(M4DEFS) $(MAKE_TOP)/scripts/rc.pvr.m4 \
		$(MAKE_TOP)/$(PVR_BUILD_DIR)/rc.pvr.m4 > $@
	$(CHMOD) +x $@

init_script: $(TARGET_OUT)/rc.pvr
