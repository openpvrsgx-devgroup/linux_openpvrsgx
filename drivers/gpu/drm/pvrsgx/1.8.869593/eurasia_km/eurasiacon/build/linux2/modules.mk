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

# Bits for processing $(modules) after reading in each Linux.mk

#$(info ---- $(modules) ----)
$(call must-be-nonempty,modules)

$(foreach _m,$(modules),$(if $(filter $(_m),$(ALL_MODULES)),$(error In makefile $(THIS_MAKEFILE): Duplicate module $(_m) (first seen in $(INTERNAL_MAKEFILE_FOR_MODULE_$(_m))) listed in $$(modules)),$(eval $(call register-module,$(_m)))))

ALL_MODULES += $(modules)
