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

#
# This file is read once at the start of the build, after reading in
# config.mk. It should define the non-MODULE_* variables used in commands,
# like ALL_CFLAGS
#

ifeq ($(BUILD),debug)
COMMON_USER_FLAGS := -O0
else
OPTIM ?= -O2
COMMON_USER_FLAGS := $(OPTIM)
endif

# FIXME: We should probably audit the driver for aliasing
#
COMMON_USER_FLAGS += -fno-strict-aliasing

# We always enable debugging. Either the release binaries are stripped
# and the symbols put in the symbolpackage, or we're building debug.
#
COMMON_USER_FLAGS += -g

# These flags are used for kernel, User C and User C++
#
COMMON_FLAGS = -W -Wall

# Some GCC warnings are C only, so we must mask them from C++
#
COMMON_CFLAGS := $(COMMON_FLAGS) \
 -Wdeclaration-after-statement -Wno-format-zero-length \
 -Wmissing-prototypes -Wstrict-prototypes

# Additional warnings, and optional warnings.
#
WARNING_CFLAGS := \
 -Wpointer-arith -Wunused-parameter \
 -Wmissing-format-attribute \
 $(call cc-option,-Wno-missing-field-initializers) \
 $(call cc-option,-fdiagnostics-show-option)

ifeq ($(W),1)
WARNING_CFLAGS += \
 $(call cc-option,-Wbad-function-cast) \
 $(call cc-option,-Wcast-qual) \
 $(call cc-option,-Wcast-align) \
 $(call cc-option,-Wconversion) \
 $(call cc-option,-Wdisabled-optimization) \
 $(call cc-option,-Wlogical-op) \
 $(call cc-option,-Wmissing-declarations) \
 $(call cc-option,-Wmissing-include-dirs) \
 $(call cc-option,-Wnested-externs) \
 $(call cc-option,-Wold-style-definition) \
 $(call cc-option,-Woverlength-strings) \
 $(call cc-option,-Wpacked) \
 $(call cc-option,-Wpacked-bitfield-compat) \
 $(call cc-option,-Wpadded) \
 $(call cc-option,-Wredundant-decls) \
 $(call cc-option,-Wshadow) \
 $(call cc-option,-Wswitch-default) \
 $(call cc-option,-Wvla) \
 $(call cc-option,-Wwrite-strings)
endif

WARNING_CFLAGS += \
 $(call cc-optional-warning,-Wunused-but-set-variable)

HOST_WARNING_CFLAGS := \
 -Wpointer-arith -Wunused-parameter \
 -Wmissing-format-attribute \
 $(call host-cc-option,-Wno-missing-field-initializers) \
 $(call host-cc-option,-fdiagnostics-show-option)

ifeq ($(W),1)
HOST_WARNING_CFLAGS += \
 $(call host-cc-option,-Wbad-function-cast) \
 $(call host-cc-option,-Wcast-qual) \
 $(call host-cc-option,-Wcast-align) \
 $(call host-cc-option,-Wconversion) \
 $(call host-cc-option,-Wdisabled-optimization) \
 $(call host-cc-option,-Wlogical-op) \
 $(call host-cc-option,-Wmissing-declarations) \
 $(call host-cc-option,-Wmissing-include-dirs) \
 $(call host-cc-option,-Wnested-externs) \
 $(call host-cc-option,-Wold-style-definition) \
 $(call host-cc-option,-Woverlength-strings) \
 $(call host-cc-option,-Wpacked) \
 $(call host-cc-option,-Wpacked-bitfield-compat) \
 $(call host-cc-option,-Wpadded) \
 $(call host-cc-option,-Wredundant-decls) \
 $(call host-cc-option,-Wshadow) \
 $(call host-cc-option,-Wswitch-default) \
 $(call host-cc-option,-Wvla) \
 $(call host-cc-option,-Wwrite-strings)
endif

HOST_WARNING_CFLAGS += \
 $(call host-cc-optional-warning,-Wunused-but-set-variable)

KBUILD_WARNING_CFLAGS := \
 -Wno-unused-parameter -Wno-sign-compare
KBUILD_WARNING_CFLAGS += \
 $(call kernel-cc-optional-warning,-Wbad-function-cast) \
 $(call kernel-cc-optional-warning,-Wcast-qual) \
 $(call kernel-cc-optional-warning,-Wcast-align) \
 $(call kernel-cc-optional-warning,-Wconversion) \
 $(call kernel-cc-optional-warning,-Wdisabled-optimization) \
 $(call kernel-cc-optional-warning,-Wlogical-op) \
 $(call kernel-cc-optional-warning,-Wmissing-declarations) \
 $(call kernel-cc-optional-warning,-Wmissing-include-dirs) \
 $(call kernel-cc-optional-warning,-Wnested-externs) \
 $(call kernel-cc-optional-warning,-Wno-missing-field-initializers) \
 $(call kernel-cc-optional-warning,-Wold-style-definition) \
 $(call kernel-cc-optional-warning,-Woverlength-strings) \
 $(call kernel-cc-optional-warning,-Wpacked) \
 $(call kernel-cc-optional-warning,-Wpacked-bitfield-compat) \
 $(call kernel-cc-optional-warning,-Wpadded) \
 $(call kernel-cc-optional-warning,-Wredundant-decls) \
 $(call kernel-cc-optional-warning,-Wshadow) \
 $(call kernel-cc-optional-warning,-Wswitch-default) \
 $(call kernel-cc-optional-warning,-Wvla) \
 $(call kernel-cc-optional-warning,-Wwrite-strings)

# User C only
#
ALL_CFLAGS := \
 $(COMMON_USER_FLAGS) $(COMMON_CFLAGS) $(WARNING_CFLAGS) \
 $(SYS_CFLAGS)

ALL_HOST_CFLAGS := \
 $(COMMON_USER_FLAGS) $(COMMON_CFLAGS) $(HOST_WARNING_CFLAGS)

# User C++ only
#
ALL_CXXFLAGS := \
 $(COMMON_USER_FLAGS) $(COMMON_FLAGS) \
 -fno-rtti -fno-exceptions \
 -Wpointer-arith -Wunused-parameter \
 $(SYS_CXXFLAGS)

# User C and C++
#
# NOTE: ALL_HOST_LDFLAGS should probably be using -rpath-link too, and if we
# ever need to support building host shared libraries, it's required.
#
# We can't use it right now because we want to support non-GNU-compatible
# linkers like the Darwin 'ld' which doesn't support -rpath-link.
#
ALL_HOST_LDFLAGS := -L$(HOST_OUT)
ALL_LDFLAGS := -L$(TARGET_OUT) -Xlinker -rpath-link=$(TARGET_OUT)

ifneq ($(strip $(TOOLCHAIN)),)
ALL_LDFLAGS += -L$(TOOLCHAIN)/lib -Xlinker -rpath-link=$(TOOLCHAIN)/lib
endif

ifneq ($(strip $(LINKER_RPATH)),)
ALL_LDFLAGS += $(addprefix -Xlinker -rpath=,$(LINKER_RPATH))
endif

ALL_LDFLAGS += $(SYS_LDFLAGS)

# Kernel C only
#
ALL_KBUILD_CFLAGS := $(COMMON_CFLAGS) $(KBUILD_WARNING_CFLAGS) \
 $(call kernel-cc-option,-Wno-type-limits) \
 $(call kernel-cc-option,-Wno-pointer-arith) \
 $(call kernel-cc-option,-Wno-aggregate-return) \
 $(call kernel-cc-option,-Wno-unused-but-set-variable)

# This variable contains a list of all modules built by kbuild
ALL_KBUILD_MODULES :=

# This variable contains a list of all modules which contain C++ source files
ALL_CXX_MODULES :=

# Toolchain triple for cross environment
CROSS_TRIPLE := $(patsubst %-,%,$(CROSS_COMPILE))
