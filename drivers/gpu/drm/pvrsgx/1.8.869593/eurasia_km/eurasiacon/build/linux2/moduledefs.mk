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

MODULE_TARGETS :=
MODULE_CFLAGS := $(ALL_CFLAGS) $($(THIS_MODULE)_cflags)
MODULE_CXXFLAGS := $(ALL_CXXFLAGS) $($(THIS_MODULE)_cxxflags)
MODULE_HOST_CFLAGS := $(ALL_HOST_CFLAGS) $($(THIS_MODULE)_cflags)
MODULE_HOST_CXXFLAGS := $(ALL_HOST_CXXFLAGS) $($(THIS_MODULE)_cxxflags)
MODULE_LDFLAGS := $(ALL_LDFLAGS) $($(THIS_MODULE)_ldflags)
MODULE_HOST_LDFLAGS := $(ALL_HOST_LDFLAGS) $($(THIS_MODULE)_ldflags)
MODULE_BISON_FLAGS := $(ALL_BISON_FLAGS) $($(THIS_MODULE)_bisonflags)
MODULE_FLEX_FLAGS := $(ALL_FLEX_FLAGS) $($(THIS_MODULE)_flexflags)

# -L flags for library search dirs
MODULE_LIBRARY_DIR_FLAGS := $(foreach _path,$($(THIS_MODULE)_libpaths),$(if $(filter /%,$(_path)),-L$(call relative-to-top,$(_path)),-L$(_path)))
# -I flags for header search dirs
MODULE_INCLUDE_FLAGS := $(foreach _path,$($(THIS_MODULE)_includes),$(if $(filter /%,$(_path)),-I$(call relative-to-top,$(_path)),-I$(_path)))

# Variables used to differentiate between host/target builds
MODULE_OUT := $(if $(MODULE_HOST_BUILD),$(HOST_OUT),$(TARGET_OUT))
MODULE_INTERMEDIATES_DIR := $(if $(MODULE_HOST_BUILD),$(HOST_INTERMEDIATES)/$(THIS_MODULE),$(TARGET_INTERMEDIATES)/$(THIS_MODULE))

.SECONDARY: $(MODULE_INTERMEDIATES_DIR)
$(MODULE_INTERMEDIATES_DIR):
	$(make-directory)

Host_or_target := $(if $(MODULE_HOST_BUILD),Host,Target)

# These define the rules for finding source files.
# - If a name begins with a slash, we strip $(TOP) off the front if it begins
#   with $(TOP). This is so that we don't get really long error messages from
#   the compiler if the source tree is in a deeply nested directory, but we
#   still do get absolute paths if you say "make OUT=/tmp/somewhere"
# - Otherwise, if a name contains a slash and begins with $(OUT), we leave it
#   as it is. This is so you can say "module_src :=
#   $(TARGET_INTERMEDIATES)/something/generated.c"
# - Otherwise, we assume it's a path referring to somewhere under the
#   directory containing Linux.mk, and add $(THIS_DIR) to it
_SOURCES_WITHOUT_SLASH := $(strip $(foreach _s,$($(THIS_MODULE)_src),$(if $(findstring /,$(_s)),,$(_s))))
_SOURCES_WITH_SLASH := $(strip $(foreach _s,$($(THIS_MODULE)_src),$(if $(findstring /,$(_s)),$(_s),)))
MODULE_SOURCES := $(addprefix $(THIS_DIR)/,$(_SOURCES_WITHOUT_SLASH))
MODULE_SOURCES += $(call relative-to-top,$(filter /%,$(_SOURCES_WITH_SLASH)))
_RELATIVE_SOURCES_WITH_SLASH := $(filter-out /%,$(_SOURCES_WITH_SLASH))
_OUTDIR_RELATIVE_SOURCES_WITH_SLASH := $(filter $(RELATIVE_OUT)/%,$(_RELATIVE_SOURCES_WITH_SLASH))
_THISDIR_RELATIVE_SOURCES_WITH_SLASH := $(filter-out $(RELATIVE_OUT)/%,$(_RELATIVE_SOURCES_WITH_SLASH))
MODULE_SOURCES += $(_OUTDIR_RELATIVE_SOURCES_WITH_SLASH)
MODULE_SOURCES += $(addprefix $(THIS_DIR)/,$(_THISDIR_RELATIVE_SOURCES_WITH_SLASH))
MODULE_SOURCES += $(addprefix $(MODULE_OUT)/intermediates/,$($(THIS_MODULE)_gensrc))
MODULE_GENERATED_HEADERS := $(addprefix $(MODULE_OUT)/intermediates/,$($(THIS_MODULE)_genheaders))

# -l flags for each library
MODULE_LIBRARY_FLAGS := $(addprefix -l, $($(THIS_MODULE)_staticlibs)) $(addprefix -l,$($(THIS_MODULE)_libs)) $(foreach _lib,$($(THIS_MODULE)_extlibs),$(if $(filter undefined,$(origin lib$(_lib)_ldflags)),-l$(_lib),$(lib$(_lib)_ldflags)))

# pkg-config integration; primarily used by X.org
# FIXME: We don't support arbitrary CFLAGS yet (just includes)
$(foreach _package,$($(THIS_MODULE)_packages),\
 $(eval MODULE_INCLUDE_FLAGS     += `pkg-config --cflags-only-I $(_package)`)\
 $(eval MODULE_LIBRARY_FLAGS     += `pkg-config --libs-only-l $(_package)`)\
 $(eval MODULE_LIBRARY_DIR_FLAGS += `pkg-config --libs-only-L $(_package)`))
