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

$(if $(strip $(KERNELDIR)),,$(error KERNELDIR must be set))
$(call directory-must-exist,$(KERNELDIR))

$(TARGET_OUT)/kbuild/Makefile: $(MAKE_TOP)/kbuild/Makefile.template
	@[ ! -e $(dir $@) ] && mkdir -p $(dir $@) || true
	$(CP) -f $< $@

# We need to make INTERNAL_KBUILD_MAKEFILES absolute because the files will be
# read while chdir'd into $(KERNELDIR)
INTERNAL_KBUILD_MAKEFILES := $(abspath $(foreach _m,$(KERNEL_COMPONENTS) $(EXTRA_PVRSRVKM_COMPONENTS),$(if $(INTERNAL_KBUILD_MAKEFILE_FOR_$(_m)),$(INTERNAL_KBUILD_MAKEFILE_FOR_$(_m)),$(error Unknown kbuild module "$(_m)"))))
INTERNAL_KBUILD_OBJECTS := $(foreach _m,$(KERNEL_COMPONENTS),$(if $(INTERNAL_KBUILD_OBJECTS_FOR_$(_m)),$(INTERNAL_KBUILD_OBJECTS_FOR_$(_m)),$(error BUG: Unknown kbuild module "$(_m)" should have been caught earlier)))
INTERNAL_EXTRA_KBUILD_OBJECTS := $(foreach _m,$(EXTRA_PVRSRVKM_COMPONENTS),$(if $(INTERNAL_KBUILD_OBJECTS_FOR_$(_m)),$(INTERNAL_KBUILD_OBJECTS_FOR_$(_m)),$(error BUG: Unknown kbuild module "$(_m)" should have been caught earlier)))
.PHONY: kbuild kbuild_clean

kbuild: $(TARGET_OUT)/kbuild/Makefile
	@$(MAKE) -Rr --no-print-directory -C $(KERNELDIR) M=$(abspath $(TARGET_OUT)/kbuild) \
		INTERNAL_KBUILD_MAKEFILES="$(INTERNAL_KBUILD_MAKEFILES)" \
		INTERNAL_KBUILD_OBJECTS="$(INTERNAL_KBUILD_OBJECTS)" \
		INTERNAL_EXTRA_KBUILD_OBJECTS="$(INTERNAL_EXTRA_KBUILD_OBJECTS)" \
		EXTRA_KBUILD_SOURCE="$(EXTRA_KBUILD_SOURCE)" \
		CROSS_COMPILE="$(KERNEL_CROSS_COMPILE)" \
		EXTRA_CFLAGS="$(ALL_KBUILD_CFLAGS)" \
		V=$(V) W=$(W) \
		TOP=$(TOP)
ifeq ($(DEBUGLINK),1)
	@for kernel_module in $(addprefix $(TARGET_OUT)/kbuild/,$(INTERNAL_KBUILD_OBJECTS:.o=.ko)); do \
		$(patsubst @%,%,$(STRIP)) --strip-unneeded $$kernel_module; \
	done
endif
	@for kernel_module in $(addprefix $(TARGET_OUT)/kbuild/,$(INTERNAL_KBUILD_OBJECTS:.o=.ko)); do \
		cp $$kernel_module $(TARGET_OUT); \
	done

kbuild_clean: $(TARGET_OUT)/kbuild/Makefile
	@$(MAKE) -Rr --no-print-directory -C $(KERNELDIR) M=$(abspath $(TARGET_OUT)/kbuild) \
		INTERNAL_KBUILD_MAKEFILES="$(INTERNAL_KBUILD_MAKEFILES)" \
		INTERNAL_KBUILD_OBJECTS="$(INTERNAL_KBUILD_OBJECTS)" \
		INTERNAL_EXTRA_KBUILD_OBJECTS="$(INTERNAL_EXTRA_KBUILD_OBJECTS)" \
		EXTRA_KBUILD_SOURCE="$(EXTRA_KBUILD_SOURCE)" \
		CROSS_COMPILE="$(KERNEL_CROSS_COMPILE)" \
		EXTRA_CFLAGS="$(ALL_KBUILD_CFLAGS)" \
		V=$(V) W=$(W) \
		TOP=$(TOP) clean

kbuild_install: $(TARGET_OUT)/kbuild/Makefile
	@: $(if $(strip $(DISCIMAGE)),,$(error $$(DISCIMAGE) was empty or unset while trying to use it to set INSTALL_MOD_PATH for modules_install))
	@$(MAKE) -Rr --no-print-directory -C $(KERNELDIR) M=$(abspath $(TARGET_OUT)/kbuild) \
		INTERNAL_KBUILD_MAKEFILES="$(INTERNAL_KBUILD_MAKEFILES)" \
		INTERNAL_KBUILD_OBJECTS="$(INTERNAL_KBUILD_OBJECTS)" \
		INTERNAL_EXTRA_KBUILD_OBJECTS="$(INTERNAL_EXTRA_KBUILD_OBJECTS)" \
		EXTRA_KBUILD_SOURCE="$(EXTRA_KBUILD_SOURCE)" \
		CROSS_COMPILE="$(KERNEL_CROSS_COMPILE)" \
		EXTRA_CFLAGS="$(ALL_KBUILD_CFLAGS)" \
		INSTALL_MOD_PATH="$(DISCIMAGE)" \
		V=$(V) W=$(W) \
		TOP=$(TOP) modules_install
