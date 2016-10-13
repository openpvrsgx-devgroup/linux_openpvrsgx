########################################################################### ###
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Strictly Confidential.
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
 -DKERNEL_ID=$(KERNEL_ID) \
 -DPVRVERSION="$(PVRVERSION)" \
 -DSOLIB_VERSION=$(PVRVERSION_MAJ).$(PVRVERSION_MIN).$(PVRVERSION_BUILD) \
 -DSHLIB_DESTDIR=$(SHLIB_DESTDIR) \
 -DDEMO_DESTDIR=$(DEMO_DESTDIR) \
 -DHEADER_DESTDIR=$(HEADER_DESTDIR) \
 -DEGL_DESTDIR=$(EGL_DESTDIR) \
 -DSUPPORT_UNITTESTS=1

# Map COMPONENTS on to SUPPORT_ defs
#
$(eval $(call if-component,opengles1,\
 -DSUPPORT_OPENGLES1=1 -DOGLES1_MODULE=$(opengles1_target) \
 -DSUPPORT_OPENGLES1_V1_ONLY=0))
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
$(eval $(call if-component,opengl_mesa,\
 -DSUPPORT_OPENGL=1))
$(eval $(call if-component,opengl,\
 -DSUPPORT_OPENGL_NULLWS=1))
$(eval $(call if-component,null_pvr2d_flip,\
 -DSUPPORT_NULL_PVR2D_FLIP=1))
$(eval $(call if-component,null_pvr2d_blit,\
 -DSUPPORT_NULL_PVR2D_BLIT=1))
$(eval $(call if-component,null_pvr2d_front,\
 -DSUPPORT_NULL_PVR2D_FRONT=1))
$(eval $(call if-component,null_pvr2d_remote,\
 -DSUPPORT_NULL_PVR2D_REMOTE=1))
$(eval $(call if-component,null_pvr2d_linuxfb,\
 -DSUPPORT_NULL_PVR2D_LINUXFB=1))
$(eval $(call if-component,null_drm_ws,\
 -DSUPPORT_NULL_DRM_WS=1))
$(eval $(call if-component,ews_ws,\
 -DSUPPORT_EWS=1))
$(eval $(call if-component,ews_ws_remote,\
 -DSUPPORT_EWS_REMOTE=1))
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
 -DXORG_DEST= \
 -DXORG_DIR=$(LWS_PREFIX) \
 -DPVR_DDX_DESTDIR=$(LWS_PREFIX)/lib/xorg/modules/drivers \
 -DPVR_DRI_DESTDIR=$(LWS_PREFIX)/lib/dri \
 -DPVR_DDX_INPUT_DESTDIR=$(LWS_PREFIX)/lib/xorg/modules/input \
 -DXORG_EXPLICIT_PVR_SERVICES_LOAD=$(XORG_EXPLICIT_PVR_SERVICES_LOAD) \
 -DPVR_DRI_CLIENT_MODULE=$(PVR_DRI_CLIENT_MODULE)))

# These defs are common to all driver builds, and inherited from config.mk
#
M4DEFS += \
 -DBUILD=$(BUILD) \
 -DPVR_BUILD_DIR=$(PVR_BUILD_DIR) \
 -DPVRSRV_MODNAME=$(PVRSRV_MODNAME) \
 -DDISPLAY_KERNEL_MODULE=$(DISPLAY_CONTROLLER) \
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
 -DPVR_ANDROID_HAS_EGL_CFG=$(PVR_ANDROID_HAS_EGL_CFG) \
 -DGRALLOC_MODULE=gralloc.$(HAL_VARIANT).so \
 -DHWCOMPOSER_MODULE=hwcomposer.$(HAL_VARIANT).so \
 -DCAMERA_MODULE=camera.$(HAL_VARIANT).so \
 -DSENSORS_MODULE=sensors.$(HAL_VARIANT).so \
 -DMEMTRACK_MODULE=memtrack.$(HAL_VARIANT).so
endif

ifeq ($(PVR_REMOTE),1)
M4DEFS += -DPVR_REMOTE=1
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
$(TARGET_OUT)/install.sh: $(PVRVERSION_H) $(MAKE_TOP)/scripts/install.sh.tpl
	$(if $(V),,@echo "  GEN (from tpl)     " $(call relative-to-top,$@))
	@sed 's/\[PVRVERSION\]/$(PVRVERSION)/g' $(MAKE_TOP)/scripts/install.sh.tpl > $@
	$(CHMOD) +x $@
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
