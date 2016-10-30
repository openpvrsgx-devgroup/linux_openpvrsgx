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
#    Can we set $(obj) to something different from$(src) for each
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

ifneq ($(CONFIG_SGX_114),)
SUBPATH=1.14.3699939/eurasia_km
else
SUBPATH=omap3
endif

SOURCE=$(obj)/$(SUBPATH)

# FIXME: should somehow keep the compiled .o separate...

$(obj)/pvr:
ifneq ($(CONFIG_SGX_CEDARVIEW),)
	@$(kecho) "  MAKE    $(obj): cedarview"
# Fix versions
	$(Q)SOC_VENDOR=intel SOC_FAMILY=cedarview_linux SGX=sgx530 SGX_FLAG=SGX530 SGX_REV=121 $(MAKE) $(build)=$(SOURCE)
endif
ifneq ($(CONFIG_SGX_OMAP),)
ifneq ($(CONFIG_SGX_114),)
	@$(kecho) "  MAKE    $(obj): omap3"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=omap3 SGX=sgx530 SGX_FLAG=SGX530 SGX_REV=121 AM_VERSION=3 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	@$(kecho) "  MAKE    $(obj): omap3630"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=omap3630 SGX=sgx530 SGX_FLAG=SGX530 SGX_REV=125 AM_VERSION=3 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	@$(kecho) "  MAKE    $(obj): am335x"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=am335x SGX=sgx530 SGX_FLAG=SGX530 SGX_REV=125 AM_VERSION=3 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	@$(kecho) "  MAKE    $(obj): omap4430"
# ES1 would have REV=110 but that is unsupported by sgxerrata.h
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=omap4430 SGX=sgx540 SGX_FLAG=SGX540 SGX_REV=120 AM_VERSION=4 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	@$(kecho) "  MAKE    $(obj): omap4460"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=omap4460 SGX=sgx540 SGX_FLAG=SGX540 SGX_REV=120 AM_VERSION=4 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	@$(kecho) "  MAKE    $(obj): omap4470"
# see https://pyra-handheld.com/boards/threads/omap4470-sgx-drivers.78480/
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=omap4470 SGX=sgx544 SGX_FLAG=SGX544 SGX_REV=112 AM_VERSION=4 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	@$(kecho) "  MAKE    $(obj): am43xx"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=am43xx SGX=sgx544 SGX_FLAG=SGX544 SGX_REV=112 AM_VERSION=4 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	@$(kecho) "  MAKE    $(obj): omap5"
# ES1 includes 105 and ES2 includes 116
# see https://e2e.ti.com/support/legacy_forums/omap/f/885/t/263519?OMAP5432-SGX544-Revisions
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=omap5 SGX=sgx544 SGX_FLAG=SGX544 SGX_REV=116 AM_VERSION=5 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	@$(kecho) "  MAKE    $(obj): am57xx"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=am57xx SGX=sgx544 SGX_FLAG=SGX544 SGX_REV=116 AM_VERSION=5 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
	@$(kecho) "  MAKE    $(obj): dra7xx"
	$(Q)SOC_VENDOR=ti SOC_FAMILY=omap SOC=dra7xx SGX=sgx544 SGX_FLAG=SGX544 SGX_REV=116 AM_VERSION=5 TARGET_TYPE=$(CONFIG_SGX_OMAP) $(MAKE) $(build)=$(SOURCE)
else
# kept here for compatibility with omap-pvr tree
	@$(kecho) "  MAKE    $(obj): omap3"
	$(Q)TI_PLATFORM=omap3 $(MAKE) $(build)=$(SOURCE)
	@$(kecho) "  MAKE    $(obj): omap3630"
	$(Q)TI_PLATFORM=omap3630 $(MAKE) $(build)=$(SOURCE)
	@$(kecho) "  MAKE    $(obj): ti335x"
	$(Q)TI_PLATFORM=ti335x $(MAKE) $(build)=$(SOURCE)
# may add more SOC variants here, e.g.
# $(Q)CONFIG_SOC=generic $(MAKE) $(build)=$(SOURCE)
# $(Q)CONFIG_SOC=omap5 $(MAKE) $(build)=$(obj)/omap5
endif
endif
ifneq ($(CONFIG_SGX_POULSBO),)
	@$(kecho) "  MAKE    $(obj): poulsbo"
# Fix versions
	$(Q)SOC_VENDOR=intel SOC_FAMILY=poulsbo SGX=sgx530 SGX_FLAG=SGX530 SGX_REV=121 $(MAKE) TARGET_TYPE=$(CONFIG_SGX_POULSBO) $(build)=$(SOURCE)
endif
ifneq ($(CONFIG_SGX_JZ4780),)
	@$(kecho) "  MAKE    $(obj): jz4780"
# see https://elinux.org/CI20-SGX_kernel_module
	$(Q)SOC_VENDOR=ingenic SOC_FAMILY=sgx_jz4780 SOC=jz4780 SGX=sgx540 SGX_FLAG=SGX540 SGX_REV=120 $(MAKE) TARGET_TYPE=$(CONFIG_SGX_JZ4780) $(build)=$(SOURCE)
endif
ifneq ($(CONFIG_SGX_SUNXI),)
	@$(kecho) "  MAKE    $(obj): sunxi"
# Fix versions
	$(Q)SOC_VENDOR=allwinner SOC_FAMILY=sunxi SGX=sgx544 SGX_FLAG=SGX544 SGX_REV=116 $(MAKE) TARGET_TYPE=$(CONFIG_SGX_SUNXI) $(build)=$(SOURCE)
endif
ifneq ($(CONFIG_SGX_GENERIC),)
	@$(kecho) "  MAKE    $(obj): generic"
# Fix versions
	$(Q)SOC_VENDOR=generic SOC_FAMILY=sgx_nohw SGX=sgx530 SGX_FLAG=SGX530 SGX_REV=121 $(MAKE) TARGET_TYPE=$(CONFIG_SGX_GENERIC) $(build)=$(SOURCE)
endif

ifneq ($(CONFIG_SGX),)
always += pvr
clean-dirs := $(objtree)/$(SUBPATH)
endif
