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

# Figure out the version of Android we're building against.
#
PLATFORM_VERSION := $(shell \
	if [ -f $(TARGET_ROOT)/product/$(TARGET_PRODUCT)/system/build.prop ]; then \
		cat $(TARGET_ROOT)/product/$(TARGET_PRODUCT)/system/build.prop | \
			grep ^ro.build.version.release | cut -f2 -d'=' | cut -f1 -d'-'; \
	else \
		echo 2.0; \
	fi)

define version-starts-with
$(shell echo $(PLATFORM_VERSION) | grep -q ^$(1); \
	[ "$$?" = "0" ] && echo 1 || echo 0)
endef

# ro.build.version.release contains the version number for release builds, or
# the version codename otherwise. In this case we need to assume that the
# version of Android we're building against has the features that are in the
# final release of that version, so we set PLATFORM_VERSION to the
# corresponding release number.
#
ifeq ($(call version-starts-with,Eclair),1)
PLATFORM_VERSION := 2.0
else ifeq ($(call version-starts-with,Froyo),1)
PLATFORM_VERSION := 2.2
else ifeq ($(call version-starts-with,Gingerbread),1)
PLATFORM_VERSION := 2.3
else ifeq ($(call version-starts-with,Honeycomb),1)
PLATFORM_VERSION := 3.0
else ifeq ($(call version-starts-with,IceCreamSandwichMR),1)
PLATFORM_VERSION := 4.1
else ifeq ($(call version-starts-with,IceCreamSandwich),1)
PLATFORM_VERSION := 4.0
else ifeq ($(shell echo $(PLATFORM_VERSION) | grep -qE "[A-Za-z]+"; echo $$?),0)
PLATFORM_VERSION := 4.5
endif

PLATFORM_VERSION_MAJ := $(shell echo $(PLATFORM_VERSION) | cut -f1 -d'.')
PLATFORM_VERSION_MIN := $(shell echo $(PLATFORM_VERSION) | cut -f2 -d'.')

# Macros to help categorize support for features and API_LEVEL for tests.
#
is_at_least_eclair := \
	$(shell ( test $(PLATFORM_VERSION_MAJ) -gt 2 || \
				( test $(PLATFORM_VERSION_MAJ) -eq 2 && \
				  test $(PLATFORM_VERSION_MIN) -ge 0 ) ) && echo 1 || echo 0)
is_at_least_froyo := \
	$(shell ( test $(PLATFORM_VERSION_MAJ) -gt 2 || \
				( test $(PLATFORM_VERSION_MAJ) -eq 2 && \
				  test $(PLATFORM_VERSION_MIN) -ge 2 ) ) && echo 1 || echo 0)
is_at_least_gingerbread := \
	$(shell ( test $(PLATFORM_VERSION_MAJ) -gt 2 || \
				( test $(PLATFORM_VERSION_MAJ) -eq 2 && \
				  test $(PLATFORM_VERSION_MIN) -ge 3 ) ) && echo 1 || echo 0)
is_at_least_honeycomb := \
	$(shell	test $(PLATFORM_VERSION_MAJ) -ge 3 && echo 1 || echo 0)
is_at_least_icecream_sandwich := \
	$(shell test $(PLATFORM_VERSION_MAJ) -ge 4 && echo 1 || echo 0)
is_at_least_icecream_sandwich_mr1 := \
	$(shell ( test $(PLATFORM_VERSION_MAJ) -gt 4 || \
				( test $(PLATFORM_VERSION_MAJ) -eq 4 && \
				  test $(PLATFORM_VERSION_MIN) -ge 1 ) ) && echo 1 || echo 0)

# FIXME: Assume "future versions" are >=4.5, but we don't really know
is_future_version := \
	$(shell ( test $(PLATFORM_VERSION_MAJ) -gt 4 || \
				( test $(PLATFORM_VERSION_MAJ) -eq 4 && \
				  test $(PLATFORM_VERSION_MIN) -ge 5 ) ) && echo 1 || echo 0)

# Picking an exact match of API_LEVEL for the platform we're building
# against can avoid compatibility theming and affords better integration.
#
ifeq ($(is_future_version),1)
API_LEVEL := 15
else ifeq ($(is_at_least_icecream_sandwich),1)
# MR2        15
# MR1        15
API_LEVEL := 14
else ifeq ($(is_at_least_honeycomb),1)
# MR2        13
# MR1        12
API_LEVEL := 11
else ifeq ($(is_at_least_gingerbread),1)
# MR1        10
API_LEVEL := 9
else ifeq ($(is_at_least_froyo),1)
API_LEVEL := 8
else ifeq ($(is_at_least_eclair),1)
# MR1        7
# 2.0.1      6
API_LEVEL := 5
else
$(error Must build against Android >= 2.0)
endif

# Each DDK is tested against only a single version of the platform.
# Warn if a different platform version is used.
#
ifeq ($(is_future_version),1)
$(info WARNING: Android version is newer than this DDK supports)
else ifneq ($(is_at_least_icecream_sandwich),1)
$(info WARNING: Android version is older than this DDK supports)
endif
