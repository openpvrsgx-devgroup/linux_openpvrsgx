#
# this (ugly) Makefile builds multiple kernel modules
# representing different SGX variants and flavours
#
# We currently need this since there are different compiler
# options and -D definitions needed for e.g. omap3430,omap3630,am33xx etc.
# so that we have to build multiple kernel modules from the
# same source code tree.
#
# Limitations:
#   always rebuilds even if nothing has changed
#
# There are two ideas how to get rid of this ugly code
#
# 1. add something to copy the sources into different subdirectories
#    before calling make to keept the .cmd and .o separate.
#    Can we set $(obj) to something different from$(srctree) for each
#    sub-make?
#    Most likely not since Makefile.build says: src := $(obj) and
#    KBUILD_OUTPUT (O= parameter to make) has a different meaning.
#
# 2. reduce to a single .ko that handles all architectures and variants
#    This means making the SoC glue code of the pvr generic (i.e.
#    get rid of e.g. SGX_CORE_REV or -DSGX530) and replace all the
#    #ifdef SGX530 etc. in SGX errata and wherelse by dynamic decisions
#    based on DT information.
#

ifeq ($(CONFIG_PVRSGX_1_9_2253347),y)
SUBPATH=1.9.2253347
else ifeq ($(CONFIG_PVRSGX_1_10_2359475),y)
# this is partially working
SUBPATH=1.10.2359475
else ifeq ($(CONFIG_PVRSGX_1_13_3341330),y)
SUBPATH=1.13.3341330/eurasia_km
else ifeq ($(CONFIG_PVRSGX_1_14_3699939),y)
# only this is working
SUBPATH=1.14.3699939/eurasia_km
else ifeq ($(CONFIG_PVRSGX_1_14_3759903),y)
SUBPATH=1.14.3759903/eurasia_km
else ifeq ($(CONFIG_PVRSGX_1_14_3841212),y)
SUBPATH=1.14.3841212/eurasia_km
else ifeq ($(CONFIG_PVRSGX_1_15_4564147),y)
SUBPATH=1.15.4564147/eurasia_km
else ifeq ($(CONFIG_PVRSGX_1_17_4948957),y)
SUBPATH=1.17.4948957/eurasia_km
else
SUBPATH=unknown
endif

SOURCE=$(obj)/$(SUBPATH)

# FIXME: should somehow keep the compiled .o separate...

$(obj)/pvr:
ifneq ($(CONFIG_SGX_CEDARVIEW),)
	@$(kecho) "  MAKE    $(obj): cedarview"
# Fix versions
	$(Q)SOC_VENDOR=intel SOC_FAMILY=cedarview_linux SOC=gma3600 SGX=sgx545 SGX_FLAG=SGX545 SGX_REV=999 $(MAKE) $(build)=$(SOURCE)
endif # CONFIG_SGX_CEDARVIEW
ifneq ($(CONFIG_SGX_OMAP),)
ifneq ($(CONFIG_PVRSGX_1_14_3699939),)
ifneq ($(CONFIG_ARCH_OMAP3),)
	@$(kecho) "  MAKE    $(obj): omap3430"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=omap3 SGX=sgx530 SGX_FLAG=SGX530 SGX_REV=121 AM_VERSION=3 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
	@$(kecho) "  MAKE    $(obj): omap3630"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=omap3630 SGX=sgx530 SGX_FLAG=SGX530 SGX_REV=125 AM_VERSION=3 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
	@$(kecho) "  MAKE    $(obj): am3517"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=am3517 SGX=sgx530 SGX_FLAG=SGX530 SGX_REV=125 AM_VERSION=3 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
ifneq ($(CONFIG_SOC_AM33XX),)
	@$(kecho) "  MAKE    $(obj): am3352"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=am3352 SGX=sgx530 SGX_FLAG=SGX530 SGX_REV=125 AM_VERSION=3 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
endif # CONFIG_SOC_AM33XX
endif # CONFIG_ARCH_OMAP3
ifneq ($(CONFIG_ARCH_OMAP4),)
	@$(kecho) "  MAKE    $(obj): omap4430/60"
# ES1 would have REV=110 but that is unsupported by sgxerrata.h
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=omap4 SGX=sgx540 SGX_FLAG=SGX540 SGX_REV=120 AM_VERSION=4 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
	@$(kecho) "  MAKE    $(obj): omap4470"
# see https://pyra-handheld.com/boards/threads/omap4470-sgx-drivers.78480/
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=omap4 SGX=sgx544 SGX_FLAG=SGX544 SGX_REV=112 AM_VERSION=4 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
ifneq ($(CONFIG_SOC_AM43XX),)
	@$(kecho) "  MAKE    $(obj): am4"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=am4 SGX=sgx530 SGX_FLAG=SGX530 SGX_REV=125 AM_VERSION=4 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
endif # CONFIG_SOC_AM43XX
endif # CONFIG_ARCH_OMAP4
ifneq ($(CONFIG_SOC_OMAP5),)
	@$(kecho) "  MAKE    $(obj): omap5"
# ES1 includes 105 and ES2 includes 116
# see https://e2e.ti.com/support/legacy_forums/omap/f/885/t/263519?OMAP5432-SGX544-Revisions
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=omap5 SGX=sgx544 SGX_FLAG=SGX544 SGX_REV=116 AM_VERSION=5 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
	@$(kecho) "  MAKE    $(obj): am57xx"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=am57xx SGX=sgx544 SGX_FLAG=SGX544 SGX_REV=116 AM_VERSION=5 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
endif	# CONFIG_SOC_OMAP5
ifneq ($(CONFIG_SOC_DRA7XX),)
	@$(kecho) "  MAKE    $(obj): dra7"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=dra7 SGX=sgx544 SGX_FLAG=SGX544 SGX_REV=116 AM_VERSION=5 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
endif	# CONFIG_SOC_DRA7XX
else # CONFIG_PVRSGX_1_14_3699939
# kept here for compatibility with omap-pvr tree
	@$(kecho) "  MAKE    $(obj): omap3"
	$(Q)TI_PLATFORM=omap3 $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
	@$(kecho) "  MAKE    $(obj): omap3630"
	$(Q)TI_PLATFORM=omap3630 $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
	@$(kecho) "  MAKE    $(obj): ti335x"
	$(Q)TI_PLATFORM=ti335x $(MAKE) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
# may add more SOC variants here, e.g.
# $(Q)CONFIG_SOC=generic $(MAKE) $(build)=$(SOURCE)
# $(Q)CONFIG_SOC=omap5 $(MAKE) $(build)=$(obj)/omap5
endif # CONFIG_PVRSGX_1_14_3699939
endif # CONFIG_SGX_OMAP
ifneq ($(CONFIG_SGX_POULSBO),)
	@$(kecho) "  MAKE    $(obj): poulsbo"
# Fix versions
	$(Q)SOC_VENDOR=intel SOC_FAMILY=poulsbo SOC=gma500 SGX=sgx530 SGX_FLAG=SGX535 SGX_REV=999 $(MAKE) TARGET_TYPE=$(CONFIG_SGX_POULSBO) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
endif # CONFIG_SGX_POULSBO
ifneq ($(CONFIG_SGX_JZ4780),)
	@$(kecho) "  MAKE    $(obj): jz4780"
# see https://elinux.org/CI20-SGX_kernel_module
# assume it is an sgx540-120
	$(Q)SOC_VENDOR=ingenic SOC_FAMILY=sgx_jz4780 SOC=jz4780 SGX=sgx540 SGX_FLAG=SGX540 SGX_REV=120 $(MAKE) TARGET_TYPE=$(CONFIG_SGX_JZ4780) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
endif # CONFIG_SGX_JZ4780
ifneq ($(CONFIG_SGX_SUNXI),)
	@$(kecho) "  MAKE    $(obj): sunxi"
# Fix versions
	$(Q)SOC_VENDOR=allwinner SOC_FAMILY=sunxi SOC=sun8i-a83t SGX=sgx544 SGX_FLAG=SGX544 SGX_REV=116 $(MAKE) TARGET_TYPE=$(CONFIG_SGX_SUNXI) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
endif # CONFIG_SGX_SUNXI
# there is also SOC_FAMILY=sgx_pc_i686_tc3 whatever this is
#
# only this should be remaining and be enabled automatically
# after we have finished work with reworking the driver...
#
ifneq ($(CONFIG_SGX_GENERIC),)
	@$(kecho) "  MAKE    $(obj): generic"
# Fix versions
	$(Q)SOC_VENDOR=generic SOC_FAMILY=any SGX=any SGX_FLAG=any SGX_REV=any $(MAKE) TARGET_TYPE=$(CONFIG_SGX_GENERIC) $(build)=$(SOURCE)
	$(Q)if [ -r $(SOURCE)/modules.order ]; then cat $(SOURCE)/modules.order >>$(obj)/modules.order; fi
endif # CONFIG_SGX_GENERIC

ifneq ($(CONFIG_SGX),)
always += pvr
clean-dirs := $(obj)/$(SUBPATH)
endif
