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

define must-be-defined
$(if $(filter undefined,$(origin $(1))),$(error In makefile $(THIS_MAKEFILE): $$($(1)) must be defined),)
endef

define must-be-nonempty
$(if $(strip $($(1))),,$(error In makefile $(THIS_MAKEFILE): $$($(1)) must contain a value))
endef

define directory-must-exist
$(if $(wildcard $(abspath $(1)/)),,$(error Directory $(1) must exist))
endef

define one-word-only
$(if $(filter-out $(firstword $($(1))),$($(1))),$(error In makefile $(THIS_MAKEFILE): $$($(1)) must contain only one word),)
endef

define target-intermediates-of
$(addprefix $(TARGET_OUT)/intermediates/$(1)/,$(2))
endef

define host-intermediates-of
$(addprefix $(HOST_OUT)/intermediates/$(1)/,$(2))
endef

define module-library
$(patsubst lib%.so,%,$(if $($(1)_target),$($(1)_target),$(1).so))
endef

# This is done to allow module type makefiles to use $(THIS_MAKEFILE)
define register-module
INTERNAL_MAKEFILE_FOR_MODULE_$(1) := $(THIS_MAKEFILE)
endef

define process-module
THIS_MODULE := $(1)
THIS_MAKEFILE := $(INTERNAL_MAKEFILE_FOR_MODULE_$(1))
include $$(MAKE_TOP)/this_makefile.mk
$$(call must-be-nonempty,THIS_MAKEFILE)
$$(call must-be-nonempty,$(1)_type)
MODULE_HOST_BUILD := $$(if $(filter host_%,$($(1)_type)),true,)
include $$(MAKE_TOP)/moduledefs.mk
include $$(MAKE_TOP)/$$(patsubst host_%,%,$($(1)_type)).mk
INTERNAL_TARGETS_FOR_$(THIS_MODULE) := $(MODULE_TARGETS)
endef

# This can be used by module_type.mk files to indicate that they can't be
# built as host_module_type
define target-build-only
$(if $(filter true,$(MODULE_HOST_BUILD)),$(error In makefile $(THIS_MAKEFILE): Module $(THIS_MODULE) attempted to build a host $(1), which is not supported))
endef

define relative-to-top
$(patsubst $(TOP)/%,%,$(1))
endef

define cc-check
$(shell \
	CC_CHECK=$(patsubst @%,%,$(CC_CHECK)) && \
	$(patsubst @%,%,$(DOS2UNIX)) $$CC_CHECK && \
	$(patsubst @%,%,$(CHMOD)) +x $$CC_CHECK && \
	$$CC_CHECK --cc "$(1)" --out "$(2)" $(3))
endef

define cc-is-64bit
$(call cc-check,$(1),$(OUT),--64)
endef

define cc-option
$(call cc-check,$(patsubst @%,%,$(CC)),$(OUT),$(1))
endef

define cxx-option
$(call cc-check,$(patsubst @%,%,$(CXX)),$(OUT),$(1))
endef

define host-cc-option
$(call cc-check,$(patsubst @%,%,$(HOST_CC)),$(OUT),$(1))
endef

define kernel-cc-option
$(call cc-check,$(KERNEL_CROSS_COMPILE)gcc,$(OUT),$(1))
endef

# Turn a particular warning on, or explicitly turn it off, depending on
# the value of W. The "-W" or "-Wno-" part of the warning need not be
# specified.
define cc-optional-warning
$(call cc-option,-W$(if $(W),,no-)$(patsubst -W%,%,$(patsubst -Wno-%,%,$(1))))
endef

define host-cc-optional-warning
$(call host-cc-option,-W$(if $(W),,no-)$(patsubst -W%,%,$(patsubst -Wno-%,%,$(1))))
endef

define kernel-cc-optional-warning
$(call kernel-cc-option,-W$(if $(W),,no-)$(patsubst -W%,%,$(patsubst -Wno-%,%,$(1))))
endef

define module-info-line
$(if $(filter modules,$(D)),$(info $(1)),)
endef
