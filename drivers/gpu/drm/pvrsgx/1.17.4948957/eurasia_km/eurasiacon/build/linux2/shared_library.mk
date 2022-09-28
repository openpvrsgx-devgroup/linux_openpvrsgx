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

ifeq ($($(THIS_MODULE)_target),)
$(THIS_MODULE)_target := $(THIS_MODULE).so
endif

MODULE_TARGETS := $(addprefix $(MODULE_OUT)/,$(if $($(THIS_MODULE)_target),$($(THIS_MODULE)_target),$(THIS_MODULE).so))
$(call module-info-line,shared library: $(MODULE_TARGETS))

include eurasiacon/build/linux2/_objects.mk

# Objects built by other modules
MODULE_EXTERNAL_OBJECTS := $($(THIS_MODULE)_obj)

# Determine whether the C++ compiler is needed for linking.
MODULE_NEEDS_CXX_LINKER := $(if $($(THIS_MODULE)_force_cxx_linker),true,)
ifeq ($($(THIS_MODULE)_force_cxx_linker),)
# If this module contains any C++ source files, MODULE_NEEDS_CXX_LINKER is
# set, because we have to use the C++ compiler to link it
MODULE_NEEDS_CXX_LINKER := $(if $(strip $(MODULE_CXX_SOURCES)),true,)
endif
ifeq ($(MODULE_NEEDS_CXX_LINKER),true)
ALL_CXX_MODULES += $(THIS_MODULE)
endif

MODULE_ALL_OBJECTS := \
 $(MODULE_C_OBJECTS) $(MODULE_CXX_OBJECTS) \
 $(MODULE_EXTERNAL_OBJECTS)

# Libraries that can be made, which this module links with
MODULE_BUILT_LIBRARIES := $(patsubst %,$(MODULE_OUT)/lib%.so,$($(THIS_MODULE)_libs))
MODULE_BUILT_STATIC_LIBRARIES := $(patsubst %,$(MODULE_OUT)/lib%.a,$($(THIS_MODULE)_staticlibs))

# Disallow undefined symbols, except for X.org video drivers
MODULE_LDFLAGS := \
 $(if $($(THIS_MODULE)_allow_undefined),,-Wl,--no-undefined) $(MODULE_LDFLAGS)
MODULE_HOST_LDFLAGS := \
 $(if $($(THIS_MODULE)_allow_undefined),,-Wl,--no-undefined) $(MODULE_HOST_LDFLAGS)

# Android may need to have DT_SONAME turned on
ifeq ($(PVR_ANDROID_NEEDS_SONAME),1)
 MODULE_LDFLAGS += -Wl,--soname=$($(THIS_MODULE)_target)
endif

MODULE_SONAME :=
ifneq ($(DONT_USE_SONAMES),1)
 ifneq ($($(THIS_MODULE)_soname),)
  MODULE_SONAME := $($(THIS_MODULE)_soname)
  MODULE_LDFLAGS += -Wl,--soname=$(MODULE_SONAME)
 endif
endif

ifeq ($(SUPPORT_BUILD_LWS),1)
 ifneq ($(MODULE_ARCH),$(TARGET_PRIMARY_ARCH))
  lws_libdir := $(LWS_PREFIX)/lib32
 else
  lws_libdir := $(LWS_PREFIX)/lib
 endif
 MODULE_LIBRARY_DIR_FLAGS += -L $(MODULE_OUT)$(lws_libdir)
 MODULE_LIBRARY_DIR_FLAGS += -Wl,-rpath-link=$(MODULE_OUT)$(lws_libdir)
 ifeq ($(filter $(lws_libdir),/usr/lib /usr/lib32),)
  MODULE_LIBRARY_DIR_FLAGS += -Wl,-rpath=$(lws_libdir)
 endif
endif

ifneq ($($(THIS_MODULE)_libpaths_relative),)
MODULE_LIBRARY_DIR_FLAGS += $(addprefix -L $(MODULE_OUT)/,$($(THIS_MODULE)_libpaths_relative))
endif

ifneq ($($(THIS_MODULE)_whole_extlibs),)
MODULE_LDFLAGS += -Wl,--whole-archive $(addprefix -l,$($(THIS_MODULE)_whole_extlibs)) -Wl,--no-whole-archive
endif

ifneq ($(MODULE_HOST_BUILD),true)
# Unusually, we define $(THIS_MODULE)_install_path if the user didn't, as we
# can't use MODULE_INSTALL_PATH in the scripts.mk logic.
ifeq ($($(THIS_MODULE)_install_path),)
$(THIS_MODULE)_install_path := \
 $${SHLIB_DESTDIR}/$(patsubst $(MODULE_OUT)/%,%,$(MODULE_TARGETS))
endif
MODULE_INSTALL_PATH := $($(THIS_MODULE)_install_path)
endif

MODULE_EXPORTS=$($(THIS_MODULE)_exports)

.PHONY: $(THIS_MODULE)
$(THIS_MODULE): $(MODULE_TARGETS)

# This is the rule used to link the final shared library
.DELETE_ON_ERROR: $(MODULE_TARGETS)
$(MODULE_TARGETS): MODULE_CC := $(MODULE_CC)
$(MODULE_TARGETS): MODULE_CXX := $(MODULE_CXX)
$(MODULE_TARGETS): MODULE_LDFLAGS := $(MODULE_LDFLAGS)
$(MODULE_TARGETS): MODULE_HOST_LDFLAGS := $(MODULE_HOST_LDFLAGS)
$(MODULE_TARGETS): MODULE_CFLAGS := -fPIC $(MODULE_CFLAGS)
$(MODULE_TARGETS): MODULE_HOST_CFLAGS := -fPIC $(MODULE_HOST_CFLAGS)
$(MODULE_TARGETS): CHECKEXPORTS_DIR := $(MODULE_INTERMEDIATES_DIR)
$(MODULE_TARGETS): MODULE_LIBRARY_DIR_FLAGS := $(MODULE_LIBRARY_DIR_FLAGS)
$(MODULE_TARGETS): MODULE_LIBRARY_FLAGS := $(MODULE_LIBRARY_FLAGS)
$(MODULE_TARGETS): MODULE_ALL_OBJECTS := $(MODULE_ALL_OBJECTS)
$(MODULE_TARGETS): MODULE_SONAME := $(MODULE_SONAME)
$(MODULE_TARGETS): MODULE_LIB_LDFLAGS := $(MODULE_LIB_LDFLAGS)
$(MODULE_TARGETS): MODULE_LIB_CRTBEGIN := $(MODULE_LIB_CRTBEGIN)
$(MODULE_TARGETS): MODULE_LIB_CRTEND := $(MODULE_LIB_CRTEND)
$(MODULE_TARGETS): MODULE_LIBGCC := $(MODULE_LIBGCC)
$(MODULE_TARGETS): MODULE_EXPORTS := $(MODULE_EXPORTS)
ifeq ($(MODULE_HOST_BUILD),)
 ifneq ($(PKG_CONFIG_ENV_VAR),)
  $(MODULE_TARGETS): export PKG_CONFIG_TOP_BUILD_DIR := $(abspath $(MODULE_OUT))
  $(MODULE_TARGETS): export $(PKG_CONFIG_ENV_VAR) := $(abspath $(MODULE_OUT)/pkgconfig)
 else ifneq ($(PKG_CONFIG_LIBDIR),)
  $(MODULE_TARGETS): export PKG_CONFIG_LIBDIR := $(PKG_CONFIG_LIBDIR)
  $(MODULE_TARGETS): export PKG_CONFIG_PATH := $(PKG_CONFIG_PATH)
  $(MODULE_TARGETS): export PKG_CONFIG_ALLOW_SYSTEM_LIBS := 1
  $(MODULE_TARGETS): export PKG_CONFIG_SYSROOT_DIR := $(PKG_CONFIG_SYSROOT_DIR)
 endif
endif
$(MODULE_TARGETS): export PKG_CONFIG_SYSROOT_DIR := $(PKG_CONFIG_SYSROOT_DIR)
$(MODULE_TARGETS): $(MODULE_BUILT_LIBRARIES) $(MODULE_BUILT_STATIC_LIBRARIES)
$(MODULE_TARGETS): $(MODULE_ALL_OBJECTS) $(THIS_MAKEFILE)

ifeq ($(MODULE_HOST_BUILD),true)
ifeq ($(MODULE_NEEDS_CXX_LINKER),true)
	$(host-shared-library-cxx-from-o)
else
	$(host-shared-library-from-o)
endif
ifeq ($(DEBUGLINK),1)
	$(host-copy-debug-information)
	$(host-strip-debug-information)
	$(host-add-debuglink)
endif
else # MODULE_HOST_BUILD
ifeq ($(MODULE_NEEDS_CXX_LINKER),true)
	$(target-shared-library-cxx-from-o)
else
	$(target-shared-library-from-o)
endif
ifneq ($(MODULE_SONAME),)
	$(LN) $@ $@.1
endif
ifeq ($(DEBUGLINK),1)
	$(target-copy-debug-information)
	$(target-strip-debug-information)
	$(target-add-debuglink)
endif
	$(call check-exports,$(MODULE_EXPORTS))
endif # MODULE_HOST_BUILD

ifneq ($(MODULE_HOST_BUILD),true)
$(MODULE_INTERMEDIATES_DIR)/.install: MODULE_TYPE := $($(THIS_MODULE)_type)
$(MODULE_INTERMEDIATES_DIR)/.install: MODULE_INSTALL_PATH := $(MODULE_INSTALL_PATH)
$(MODULE_INTERMEDIATES_DIR)/.install: MODULE_TARGETS := $(patsubst $(MODULE_OUT)/%,%,$(MODULE_TARGETS))
$(MODULE_INTERMEDIATES_DIR)/.install: MODULE_SONAME := $(MODULE_SONAME)
$(MODULE_INTERMEDIATES_DIR)/.install: $(THIS_MAKEFILE) | $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR)/.install: $(PVRVERSION_H)
ifeq ($(MODULE_SONAME),)
	@echo 'install_file $(MODULE_TARGETS) $(MODULE_INSTALL_PATH) "$(MODULE_TYPE)" 0644 0:0' >$@
else
	@echo 'install_file $(MODULE_TARGETS) $(MODULE_INSTALL_PATH).$(PVRVERSION_NUM) "$(MODULE_TYPE)" 0644 0:0' >$@
	@echo 'link_library $(MODULE_INSTALL_PATH).$(PVRVERSION_NUM)' >>$@
endif
endif


MODULE_CFLAGS += -fPIC -pie
MODULE_CXXFLAGS += -fPIC -pie
MODULE_HOST_CFLAGS += -fPIC -pie
MODULE_HOST_CXXFLAGS += -fPIC -pie

$(foreach _src_file,$(MODULE_C_SOURCES),$(eval $(call rule-for-objects-o-from-one-c,$(MODULE_INTERMEDIATES_DIR)/$(notdir $(_src_file:.c=.o)),$(_src_file))))
$(foreach _src_file,$(MODULE_CXX_SOURCES),$(eval $(call rule-for-objects-o-from-one-cxx,$(MODULE_INTERMEDIATES_DIR)/$(notdir $(_src_file:.cpp=.o)),$(_src_file))))
