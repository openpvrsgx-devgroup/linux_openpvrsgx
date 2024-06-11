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


# Define the default goal. This masks a previous definition of the default
# goal in Makefile.config, which must match this one
.PHONY: build
build: components kbuild scripts

ifeq ($(OUT),)
$(error "Must specify output directory with OUT=")
endif

ifeq ($(TOP),)
$(error "Must specify root of source tree with TOP=")
endif
$(call directory-must-exist,$(TOP))

# Output directory for configuration, object code,
# final programs/libraries, and install/rc scripts.
#

# RELATIVE_OUT is relative only if it's under $(TOP)
RELATIVE_OUT		:= $(patsubst $(TOP)/%,%,$(OUT))
HOST_OUT			:= $(RELATIVE_OUT)/host
TARGET_OUT			:= $(RELATIVE_OUT)/target
CONFIG_MK			:= $(RELATIVE_OUT)/config.mk
CONFIG_H			:= $(RELATIVE_OUT)/config.h
CONFIG_KERNEL_MK	:= $(RELATIVE_OUT)/config_kernel.mk
CONFIG_KERNEL_H		:= $(RELATIVE_OUT)/config_kernel.h
MAKE_TOP			:= eurasiacon/build/linux2
THIS_MAKEFILE		:= (top-level makefiles)

include $(MAKE_TOP)/defs.mk

ifneq ($(INTERNAL_CLOBBER_ONLY),true)
# Create the out directory
#
$(shell mkdir -p $(OUT))

# Provide rules to create $(HOST_OUT) and $(TARGET_OUT)
.SECONDARY: $(HOST_OUT) $(TARGET_OUT)
$(HOST_OUT) $(TARGET_OUT):
	$(make-directory)

# If these generated files differ from any pre-existing ones,
# replace them, causing affected parts of the driver to rebuild.
#
$(shell \
	for file in $(CONFIG_MK) $(CONFIG_H) \
				$(CONFIG_KERNEL_MK) $(CONFIG_KERNEL_H); do \
		diff -q $$file $$file.new >/dev/null 2>/dev/null \
		&& rm -f $$file.new \
		|| mv -f $$file.new $$file >/dev/null 2>/dev/null; \
	done)
endif

MAKEFLAGS := -Rr --no-print-directory

ifneq ($(INTERNAL_CLOBBER_ONLY),true)

# This is so you can say "find $(TOP) -name Linux.mk > /tmp/something; export
# ALL_MAKEFILES=/tmp/something; make" and avoid having to run find. This is
# handy if your source tree is mounted over NFS or something
override ALL_MAKEFILES := $(call relative-to-top,$(if $(strip $(ALL_MAKEFILES)),$(shell cat $(ALL_MAKEFILES)),$(shell find $(TOP) -type f -name Linux.mk)))
ifeq ($(strip $(ALL_MAKEFILES)),)
$(info ** Unable to find any Linux.mk files under $$(TOP). This could mean that)
$(info ** there are no makefiles, or that ALL_MAKEFILES is set in the environment)
$(info ** and points to a nonexistent or empty file.)
$(error No makefiles)
endif

else # clobber-only
ALL_MAKEFILES :=
endif

REMAINING_MAKEFILES := $(ALL_MAKEFILES)
ALL_MODULES :=
INTERNAL_INCLUDED_ALL_MAKEFILES :=

ifneq ($(INTERNAL_CLOBBER_ONLY),true)
# Please do not change the format of the following lines
-include $(CONFIG_KERNEL_MK)
# These files may not exist in GPL km source packages
-include $(MAKE_TOP)/xorgconf.mk
-include $(MAKE_TOP)/llvm.mk
endif

include $(MAKE_TOP)/commands.mk
include $(MAKE_TOP)/buildvars.mk

HOST_INTERMEDIATES := $(HOST_OUT)/intermediates
TARGET_INTERMEDIATES := $(TARGET_OUT)/intermediates

# Include each Linux.mk, then include modules.mk to save some information
# about each module
include $(foreach _Linux.mk,$(ALL_MAKEFILES),$(MAKE_TOP)/this_makefile.mk $(_Linux.mk) $(MAKE_TOP)/modules.mk)

ifeq ($(strip $(REMAINING_MAKEFILES)),)
INTERNAL_INCLUDED_ALL_MAKEFILES := true
else
$(error Impossible: $(words $(REMAINING_MAKEFILES)) makefiles were mysteriously ignored when reading $$(ALL_MAKEFILES))
endif

# At this point, all Linux.mks have been included. Now generate rules to build
# each module: for each module in $(ALL_MODULES), set per-makefile variables
$(foreach _m,$(ALL_MODULES),$(eval $(call process-module,$(_m))))

.PHONY: kbuild scripts install install_script
kbuild scripts install install_script:

ifneq ($(INTERNAL_CLOBBER_ONLY),true)
-include $(MAKE_TOP)/scripts.mk
-include $(MAKE_TOP)/kbuild/kbuild.mk
else
# We won't depend on 'build' here so that people can build subsets of
# components and still have the install script attempt to install the
# subset.
install:
	@if [ ! -d "$(DISCIMAGE)" ]; then \
		echo; \
		echo "** DISCIMAGE was not set or does not point to a valid directory."; \
		echo "** Cannot continue with install."; \
		echo; \
		exit 1; \
	fi
	@if [ ! -f $(TARGET_OUT)/install.sh ]; then \
		echo; \
		echo "** install.sh not found in $(TARGET_OUT)."; \
		echo "** Cannot continue with install."; \
		echo; \
		exit 1; \
	fi
	@cd $(TARGET_OUT) && ./install.sh
endif

# You can say 'make all_modules' to attempt to make everything, or 'make
# components' to only make the things which are listed (in the per-build
# makefiles) as components of the build. 'make scripts' generates the
# install.sh and rc.pvr scripts.
.PHONY: all_modules components scripts
all_modules: $(ALL_MODULES)
components: $(COMPONENTS)

# 'make opk' builds the OEM Porting Kit. The build should set OPK_COMPONENTS
# in components.mk if it should be possible to build the OPK for it
.PHONY: opk
ifneq ($(strip $(OPK_COMPONENTS)),)
opk: $(OPK_COMPONENTS)

opk_clobber: MODULE_DIRS_TO_REMOVE := $(addprefix $(OUT)/target/intermediates/,$(OPK_COMPONENTS))
opk_clobber: OPK_OUTFILES := $(addprefix $(RELATIVE_OUT)/target/,$(foreach _c,$(OPK_COMPONENTS),$(if $($(_c)_target),$($(_c)_target),$(error Module $(_c) must be a shared library which sets $$($(_c)_target) for OPK clobbering))))
opk_clobber:
	$(clean-dirs)
	$(if $(V),,@echo "  RM      " $(call relative-to-top,$(OPK_OUTFILES)))
	$(RM) -f $(OPK_OUTFILES)
else
# OPK_COMPONENTS is empty or unset
opk:
	@echo
	@echo "** This build ($(PVR_BUILD_DIR)) is unable to build the OPK, because"
	@echo "** OPK_COMPONENTS is empty or unset. Cannot continue."
	@echo
	@false
endif

# Cleaning
.PHONY: clean clobber
clean: MODULE_DIRS_TO_REMOVE := $(OUT)/host/intermediates $(OUT)/target/intermediates $(OUT)/target/kbuild
clean:
	$(clean-dirs)
clobber: MODULE_DIRS_TO_REMOVE := $(OUT)
clobber:
	$(clean-dirs)

# Saying 'make clean-MODULE' removes the intermediates for MODULE.
# clobber-MODULE deletes the output files as well
clean-%:
	$(if $(V),,@echo "  RM      " $(call relative-to-top,$(OUT)/host/intermediates/$* $(OUT)/target/intermediates/$*))
	$(RM) -rf $(OUT)/host/intermediates/$*/* $(OUT)/target/intermediates/$*/*
clobber-%:
	$(if $(V),,@echo "  RM      " $(call relative-to-top,$(OUT)/host/intermediates/$* $(OUT)/target/intermediates/$* $(INTERNAL_TARGETS_FOR_$*)))
	$(RM) -rf $(OUT)/host/intermediates/$* $(OUT)/target/intermediates/$* $(INTERNAL_TARGETS_FOR_$*)

include $(MAKE_TOP)/bits.mk
