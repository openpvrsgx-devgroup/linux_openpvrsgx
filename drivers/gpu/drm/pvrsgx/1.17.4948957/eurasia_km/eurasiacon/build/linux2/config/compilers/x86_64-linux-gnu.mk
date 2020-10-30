# 64-bit x86 compiler
TARGET_FORCE_32BIT := -m32
IS_KERNEL_32 := 0
ifneq ($(KERNELDIR),)
  IS_KERNEL_32 = ($(shell grep -q "CONFIG_X86_32=y" $(KERNELDIR)/.config && echo 1 || echo 0))
 ifneq ($(ARCH),i386)
  ifeq ($(IS_KERNEL_32),1)
   $(warning ******************************************************)
   $(warning Your kernel appears to be configured for 32-bit x86,)
   $(warning but CROSS_COMPILE (or KERNEL_CROSS_COMPILE) points)
   $(warning to a 64-bit compiler.)
   $(warning If you want a 32-bit build, either set CROSS_COMPILE)
   $(warning to point to a 32-bit compiler, or build with ARCH=i386)
   $(warning to force 32-bit mode with your existing compiler.)
   $(warning ******************************************************)
   $(error Invalid CROSS_COMPILE / kernel architecture combination)
  endif # CONFIG_X86_32
 endif # ARCH=i386
endif # KERNELDIR

# If ARCH=i386 is set, force a build for 32-bit only, even though we're
# using a 64-bit compiler.
ifeq ($(ARCH),i386)
 include $(compilers)/i386-linux-gnu.mk
 ifeq ($(IS_KERNEL_32),0)
	USE_64BIT_COMPAT := 1
 endif
else
 USE_64BIT_COMPAT := 1
 TARGET_PRIMARY_ARCH := target_x86_64
 ifeq ($(MULTIARCH),1)
  TARGET_SECONDARY_ARCH := target_i686
 endif # MULTIARCH
endif
