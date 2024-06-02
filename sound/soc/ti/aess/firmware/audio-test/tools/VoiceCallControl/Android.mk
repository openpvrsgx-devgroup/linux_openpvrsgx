#
# Copyright (C) 2010-2011  Texas Instruments, Inc.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

LOCAL_PATH:= $(call my-dir)

#
# Build voicecallcontrol
#

# for GingerBread
ifneq (,$(findstring GINGERBREAD, $(BUILD_ID)))
    include $(CLEAR_VARS)

    LOCAL_CFLAGS:= -D_POSIX_SOURCE

    LOCAL_C_INCLUDES += external/alsa-lib/include

    LOCAL_SRC_FILES:= voicecallcontrol.c

    LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
    LOCAL_MODULE_TAGS:= optional
    LOCAL_MODULE:=voicecallcontrol

    LOCAL_SHARED_LIBRARIES := \
        libasound \
        libcutils \
        libutils
endif
# for IceCreamSandwitch
ifneq (,$(findstring ICS, $(BUILD_ID)))
    include $(CLEAR_VARS)

    LOCAL_CFLAGS:= -D_POSIX_SOURCE -DTINYALSA

    LOCAL_C_INCLUDES += external/tinyalsa/include

    LOCAL_SRC_FILES:= voicecallcontrol.c

    LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
    LOCAL_MODULE_TAGS:= optional
    LOCAL_MODULE:=voicecallcontrol

    LOCAL_SHARED_LIBRARIES := \
        libtinyalsa \
        libcutils \
        libutils
endif

include $(BUILD_EXECUTABLE)
