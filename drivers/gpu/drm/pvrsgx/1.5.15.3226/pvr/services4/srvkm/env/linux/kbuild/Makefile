#
# Copyright (c) Imagination Technologies Ltd.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE. 
# 
#

MODULE		?= pvrsrvkm

MODULE_CFLAGS = 

ifeq ($(PVR_KBUILD_IN_KERNEL),1)
# PVRKROOT is the relative path to the PVR sources.
# DRVKROOT should point to the non-PVR sources, if any.
PVRKROOT ?= .
KBUILDROOT 	= $(PVRKROOT)/services4
else
KBUILDROOT 	= ../../../..
endif

clean-dirs :=

INCLUDES = 	-I$(EURASIAROOT)/include4 \
			-I$(EURASIAROOT)/services4/include \
			-I$(EURASIAROOT)/services4/srvkm/env/linux \
			-I$(EURASIAROOT)/services4/srvkm/include \
			-I$(EURASIAROOT)/services4/srvkm/bridged \
			-I$(EURASIAROOT)/services4/system/include \
			-I$(EURASIAROOT)/services4/system/plb \
			-I$(EURASIAROOT)/services4/system/tnc 


SOURCES	      = $(KBUILDROOT)/srvkm/env/linux/osfunc.c \
				$(KBUILDROOT)/srvkm/env/linux/mutils.c \
				$(KBUILDROOT)/srvkm/env/linux/mmap.c \
				$(KBUILDROOT)/srvkm/env/linux/module.c \
				$(KBUILDROOT)/srvkm/env/linux/pdump.c \
				$(KBUILDROOT)/srvkm/env/linux/proc.c \
				$(KBUILDROOT)/srvkm/env/linux/pvr_bridge_k.c \
				$(KBUILDROOT)/srvkm/env/linux/pvr_debug.c \
				$(KBUILDROOT)/srvkm/env/linux/mm.c \
				$(KBUILDROOT)/srvkm/env/linux/mutex.c \
				$(KBUILDROOT)/srvkm/env/linux/event.c \
				$(KBUILDROOT)/srvkm/env/linux/osperproc.c

SOURCES	     += $(KBUILDROOT)/srvkm/common/buffer_manager.c \
				$(KBUILDROOT)/srvkm/common/devicemem.c \
				$(KBUILDROOT)/srvkm/common/deviceclass.c \
				$(KBUILDROOT)/srvkm/common/handle.c \
				$(KBUILDROOT)/srvkm/common/hash.c \
				$(KBUILDROOT)/srvkm/common/metrics.c \
				$(KBUILDROOT)/srvkm/common/pvrsrv.c \
				$(KBUILDROOT)/srvkm/common/queue.c \
				$(KBUILDROOT)/srvkm/common/ra.c \
				$(KBUILDROOT)/srvkm/common/resman.c \
				$(KBUILDROOT)/srvkm/common/power.c \
				$(KBUILDROOT)/srvkm/common/mem.c \
				$(KBUILDROOT)/srvkm/common/pdump_common.c \
				$(KBUILDROOT)/srvkm/bridged/bridged_support.c \
				$(KBUILDROOT)/srvkm/bridged/bridged_pvr_bridge.c \
				$(KBUILDROOT)/srvkm/common/perproc.c \
				$(KBUILDROOT)/../services4/system/plb/sysconfig.c \
				$(KBUILDROOT)/../services4/system/tnc/sysconfig.c \
				$(KBUILDROOT)/../services4/system/common/sysconfig.c \
				$(KBUILDROOT)/../services4/system/common/sysutils.c \
				$(KBUILDROOT)/srvkm/common/lists.c \
				$(KBUILDROOT)/srvkm/common/mem_debug.c

INCLUDES += -I$(EURASIAROOT)/services4/srvkm/hwdefs 

SYM_VERS_DEPS = $(EURASIAROOT)/tools/intern/debug/dbgdriv/linux


#
# Device dependent
#

ifeq ($(SUPPORT_SGX), 1)

INCLUDES 	+=	-I$(EURASIAROOT)/services4/srvkm/bridged/sgx \
				-I$(EURASIAROOT)/services4/srvkm/devices/sgx

SOURCES		+= 	$(KBUILDROOT)/srvkm/bridged/sgx/bridged_sgx_bridge.c \
				$(KBUILDROOT)/srvkm/devices/sgx/sgxinit.c \
				$(KBUILDROOT)/srvkm/devices/sgx/sgxpower.c \
				$(KBUILDROOT)/srvkm/devices/sgx/sgxreset.c \
				$(KBUILDROOT)/srvkm/devices/sgx/sgxutils.c \
				$(KBUILDROOT)/srvkm/devices/sgx/sgxkick.c \
				$(KBUILDROOT)/srvkm/devices/sgx/sgxtransfer.c \
				$(KBUILDROOT)/srvkm/devices/sgx/mmu.c \
				$(KBUILDROOT)/srvkm/devices/sgx/pb.c

else  

ifeq ($(SUPPORT_VGX), 1)

INCLUDES 	+=	-I$(EURASIAROOT)/services4/srvkm/bridged/vgx \
				-I$(EURASIAROOT)/services4/srvkm/devices/vgx

#FIXME: We need to cerate a subset of vgxdefs.h from $(EURASIAROOT)/hwdefs 
# and put it into srvkm/hwdefs for GPL compliance
INCLUDES 	+=	-I$(EURASIAROOT)/hwdefs

SOURCES		+= 	$(KBUILDROOT)/srvkm/bridged/vgx/bridged_vgx_bridge.c \
				$(KBUILDROOT)/srvkm/devices/vgx/vgxinit.c \
				$(KBUILDROOT)/srvkm/devices/vgx/vgxreset.c \
				$(KBUILDROOT)/srvkm/devices/vgx/mmu.c \
				$(KBUILDROOT)/srvkm/devices/vgx/pb.c \
				$(KBUILDROOT)/srvkm/env/linux/pdump_vgx.c
endif

endif # SUPPORT_SGX

ifeq ($(SUPPORT_DRI_DRM),1)
	SOURCES += $(KBUILDROOT)/srvkm/env/linux/pvr_drm.c
	INCLUDES += -I$(KERNELDIR)/include/drm \
		-I$(EURASIAROOT)/services4/include/env/linux

ifneq ("$(DISPLAY_CONTROLLER_DIR)","")
include $(EURASIAROOT)/services4/$(DISPLAY_CONTROLLER_DIR)/makefile.linux.common
endif

ifeq ($(PDUMP),1)
include $(EURASIAROOT)/tools/intern/debug/dbgdriv/linux/makefile.linux.common
endif

endif

ifeq ($(PVR_MODULE_TEST),1)
INCLUDES += -I$(EURASIAROOT)/moduletests/include
SOURCES	 += $(KBUILDROOT)/../moduletests/services_test/kern_test.c
SYS_CFLAGS += -DMODULE_TEST
endif

include $(EURASIAROOT)/eurasiacon/build/linux/kbuild/Makefile.kbuild_subdir_common
