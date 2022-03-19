
COMPATNUM=@LIBTOOL_VERSION_INFO@

AM_CFLAGS = \
	-I$(top_srcdir)/src -Wall -Werror \
	-I $(LINUX_DIR)/ \
	-I $(LINUX_DIR)/arch/arm/include \
	-I $(LINUX_DIR)/arch/arm/include/generated \
	-I $(LINUX_DIR)/include \
	-I $(LINUX_DIR)/arch/arm/include/uapi \
	-I $(LINUX_DIR)/arch/arm/include/generated/uapi \
	-I $(LINUX_DIR)/include/uapi \
	-I $(LINUX_DIR)/include/generated/uapi \
	-include $(LINUX_DIR)/include/linux/compiler-version.h \
	-include $(LINUX_DIR)/include/linux/kconfig.h \
	-include $(LINUX_DIR)/include/linux/compiler_types.h \
	-include $(LINUX_DIR)/include/linux/types.h \
	-D__KERNEL__ \
	-D__LINUX_ARM_ARCH__=7 \
	-D_SYS_STAT_H \
	-D_LINUX_WAIT_H \
	-D_LINUX_TIME_H \
	-D__ASM_ARM_BYTEORDER_H \
	-D__user=''

bin_PROGRAMS = socfw

socfw_SOURCES = socfw.c socfw.h

socfw_CFLAGS = $(AM_CFLAGS)
socfw_LDADD = libsocfw.la

include_HEADERS = \
	socfw.h

lib_LTLIBRARIES = libsocfw.la
libsocfw_la_SOURCES = \
	lib.c

libsocfw_la_LDFLAGS = \
	-version-info 0 -release $(VERSION) \
	-export-dynamic
