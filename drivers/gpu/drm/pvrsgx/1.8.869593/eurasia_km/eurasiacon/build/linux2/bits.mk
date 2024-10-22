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

# Useful special targets which don't build anything

ifneq ($(filter dumpvar-%,$(MAKECMDGOALS)),)
dumpvar-%: ;
$(foreach _var_to_dump,$(patsubst dumpvar-%,%,$(filter dumpvar-%,$(MAKECMDGOALS))),$(info $(if $(filter undefined,$(origin $(_var_to_dump))),# $$($(_var_to_dump)) is not set,$(_var_to_dump) := $($(_var_to_dump)))))
endif

ifneq ($(filter whereis-%,$(MAKECMDGOALS)),)
whereis-%: ;
$(foreach _module_to_find,$(patsubst whereis-%,%,$(filter whereis-%,$(MAKECMDGOALS))),$(info $(if $(INTERNAL_MAKEFILE_FOR_MODULE_$(_module_to_find)),$(INTERNAL_MAKEFILE_FOR_MODULE_$(_module_to_find)),# No module $(_module_to_find))))
endif

ifneq ($(filter whatis-%,$(MAKECMDGOALS)),)
whatis-$(RELATIVE_OUT)/target/%: ;
whatis-$(RELATIVE_OUT)/host/%: ;
$(foreach _file_to_find,$(patsubst whatis-%,%,$(filter whatis-%,$(MAKECMDGOALS))),$(info $(strip $(foreach _m,$(ALL_MODULES),$(if $(filter $(_file_to_find),$(INTERNAL_TARGETS_FOR_$(_m))),$(_file_to_find) is in $(_m) which is defined in $(INTERNAL_MAKEFILE_FOR_MODULE_$(_m)),)))))
endif

.PHONY: ls-modules ls-modules-v
ls-modules:
	@: $(foreach _m,$(ALL_MODULES),$(info $(_m)))
ls-modules-v:
	@: $(foreach _m,$(ALL_MODULES),$(info $($(_m)_type) $(_m) $(patsubst $(TOP)/%,%,$(INTERNAL_MAKEFILE_FOR_MODULE_$(_m)))))

ifeq ($(strip $(MAKECMDGOALS)),visualise)
FORMAT ?= xlib
GRAPHVIZ ?= neato
visualise: $(OUT)/MAKE_RULES.dot
	$(GRAPHVIZ) -T$(FORMAT) -o $(OUT)/MAKE_RULES.$(FORMAT) $<
$(OUT)/MAKE_RULES.dot: $(OUT)/MAKE_RULES
	perl $(MAKE_TOP)/tools/depgraph.pl -t $(TOP) -g $(firstword $(GRAPHVIZ)) $(OUT)/MAKE_RULES >$(OUT)/MAKE_RULES.dot
$(OUT)/MAKE_RULES: $(ALL_MAKEFILES)
	-$(MAKE) -C $(TOP) -f $(MAKE_TOP)/toplevel.mk TOP=$(TOP) OUT=$(OUT) ls-modules -qp >$(OUT)/MAKE_RULES 2>&1
else
visualise:
	@: $(error visualise specified along with other goals. This is not supported)
endif

.PHONY: help
help:
	@echo 'Build targets'
	@echo '  make, make build       Build the UM/KM components of the build and scripts'
	@echo '  make components        Build only the UM components'
	@echo '  make kbuild            Build only the KM components'
	@echo '  make scripts           Build only the scripts (rc.pvr and install.sh)'
	@echo '  make MODULE            Build the module MODULE and all its dependencies'
	@echo '  make eurasiacon/binary2_.../target/libsomething.so'
	@echo '                         Build a particular file (including intermediates)'
	@echo 'Variables'
	@echo '  make V=1 ...           Print the commands that are executed'
	@echo '  make OUT=dir ...       Place output+intermediates in specified directory'
	@echo '  make SOMEOPTION=1 ...  Set configuration options (see Makefile.config)'
	@echo ''
	@echo 'Clean targets'
	@echo '  make clean             Remove only intermediates for the current build'
	@echo '  make clobber           As "make clean", but remove output files too'
	@echo '  make clean-MODULE      Clean (or clobber) only files for MODULE'
	@echo ''
	@echo 'Special targets'
	@echo '  make whereis-MODULE    Show the path to the Linux.mk defining MODULE'
	@echo '  make whatis-FILE       Show which module builds an output FILE'
	@echo '  make ls-modules[-v]    List all modules [with type+makefile]'
