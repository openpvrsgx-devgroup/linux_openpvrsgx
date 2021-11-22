.PHONY: all linstall clean

-include ../rules.make

INCS ?= -I$(GSDK_KM_DIR)/include4 \
        -I$(GSDK_KM_DIR)/services4/include

KBUILD_EXTRA_SYMBOLS ?= $(GSDK_KM_DIR)/Module.symvers


CFLAGS = -DLINUX $(INCS)
ifeq ($(OMAP3_ES2x),1)
	CFLAGS += -DOMAP3_ES2x
endif

export CROSS_COMPILE KBUILD_EXTRA_SYMBOLS

all:
	$(MAKE) -C $(KERNEL_DIR) M=`pwd` EXTRA_CFLAGS="$(CFLAGS)" modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=`pwd` clean

install:
	$(MAKE) -C $(KERNEL_DIR) M=`pwd` INSTALL_MOD_PATH=$(TGTFS_PATH) modules_install
