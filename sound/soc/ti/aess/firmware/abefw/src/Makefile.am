
AM_CFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/tasks

bin_PROGRAMS = abegen

abegen_SOURCES = abegen.c abegen.h

#todo move abe_asrc to versioned directory
abegen_CFLAGS = $(AM_CFLAGS) -Werror -Wall \
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
	-D__LINUX_ARM_ARCH__=7
