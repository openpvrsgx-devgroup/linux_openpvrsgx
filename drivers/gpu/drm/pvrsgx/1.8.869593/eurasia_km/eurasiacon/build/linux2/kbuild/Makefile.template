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

# This top-level kbuild makefile builds all the Linux kernel modules in the
# DDK. To run kbuild, this makefile is copied to $(TARGET_OUT)/kbuild/Makefile
# and make is invoked in $(TARGET_OUT)/kbuild.

# This makefile doesn't define any kbuild special variables apart from
# ccflags-y and obj-m. The variables for objects are picked up by including
# the kbuild makefile fragments named in $(INTERNAL_KBUILD_MAKEFILES). The
# list of objects that these fragments make is collected in
# $(INTERNAL_KBUILD_OBJECTS) and $(INTERNAL_EXTRA_KBUILD_OBJECTS). These
# variables are set according to the build's $(KERNEL_COMPONENTS) and
# $(EXTRA_PVRSRVKM_COMPONENTS). To add a new kernel module to the build, edit
# these variables in the per-build Makefile.

include $(OUT)/config_kernel.mk

.SECONDARY:

$(OUT)/target/kbuild/external/%.c: $(EXTRA_KBUILD_SOURCE)/%.c
	@if [ ! -e $(dir $@) ]; then mkdir -p $(dir $@); fi
	@if [ ! -h $@ ]; then ln -sf $< $@; fi

$(OUT)/target/kbuild/%.c: $(TOP)/%.c
	@if [ ! -e $(dir $@) ]; then mkdir -p $(dir $@); fi
	@if [ ! -h $@ ]; then ln -sf $< $@; fi

ccflags-y += -D__linux__ -include $(OUT)/config_kernel.h \
 -DDEBUG_LOG_PATH_TRUNCATE="\"$(OUT)/target/kbuild\"" \
 -I$(OUT)/include \
 -I$(TOP)/include4 \
 -I$(TOP)/services4/include \
 -I$(TOP)/services4/system/$(PVR_SYSTEM) \
 -I$(TOP)/services4/system/include \
 -I$(TOP)/services4/srvkm/hwdefs \
 -I$(TOP)/services4/srvkm/bridged \
 -I$(TOP)/services4/srvkm/bridged/sgx \
 -I$(TOP)/services4/srvkm/common \
 -I$(TOP)/services4/srvkm/devices/sgx \
 -I$(TOP)/services4/srvkm/env/linux \
 -I$(TOP)/services4/srvkm/include

include $(INTERNAL_KBUILD_MAKEFILES)

$(if $(pvrsrvkm_sgx$(SGXCORE)_$(SGX_CORE_REV)-y),,$(error pvrsrvkm_sgx$(SGXCORE)_$(SGX_CORE_REV)-y was empty, which could mean that srvkm is missing from $$(KERNEL_COMPONENTS)))
pvrsrvkm-y += $(foreach _m,$(INTERNAL_EXTRA_KBUILD_OBJECTS:.o=),$($(_m)-y))

obj-m += $(INTERNAL_KBUILD_OBJECTS)
