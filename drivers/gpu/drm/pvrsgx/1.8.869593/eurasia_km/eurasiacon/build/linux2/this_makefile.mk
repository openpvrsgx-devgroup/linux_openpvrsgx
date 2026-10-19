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

# Find out the path of the Linux.mk makefile currently being processed, and
# set paths used by the build rules

# This magic is used so we can use this_makefile.mk twice: first when reading
# in each Linux.mk, and then again when generating rules. There we set
# $(THIS_MAKEFILE), and $(REMAINING_MAKEFILES) should be empty
ifneq ($(strip $(REMAINING_MAKEFILES)),)

# Absolute path to the Linux.mk being processed
THIS_MAKEFILE := $(firstword $(REMAINING_MAKEFILES))

# The list of makefiles left to process
REMAINING_MAKEFILES := $(wordlist 2,$(words $(REMAINING_MAKEFILES)),$(REMAINING_MAKEFILES))

else

# When generating rules, we should have read in every Linux.mk
$(if $(INTERNAL_INCLUDED_ALL_MAKEFILES),,$(error No makefiles left in $$(REMAINING_MAKEFILES), but $$(INTERNAL_INCLUDED_ALL_MAKEFILES) is not set))

endif

# Path to the directory containing Linux.mk
THIS_DIR := $(patsubst %/,%,$(dir $(THIS_MAKEFILE)))
ifeq ($(strip $(THIS_DIR)),)
$(error Empty $$(THIS_DIR) for makefile "$(THIS_MAKEFILE)")
endif

modules :=
