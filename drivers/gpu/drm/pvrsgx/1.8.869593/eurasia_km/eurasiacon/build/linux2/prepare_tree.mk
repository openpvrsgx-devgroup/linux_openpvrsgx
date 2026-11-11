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

-include $(OUT)/config.mk.new

.PHONY: prepare_tree

-include eurasiacon/build/linux2/kbuild/external_tarball.mk

# If there's no external tarball, there's nothing to do
#
prepare_tree:

INTERNAL_INCLUDED_PREPARE_HEADERS :=
-include eurasiacon/build/linux2/prepare_headers.mk
ifneq ($(INTERNAL_INCLUDED_PREPARE_HEADERS),true)
missing_headers := $(strip $(shell test ! -e include4/pvrversion.h && echo true))
ifdef missing_headers
$(info )
$(info ** include4/pvrversion.h is missing, and cannot be rebuilt.)
$(info ** Cannot continue.)
$(info )
$(error Missing headers)
endif
endif
