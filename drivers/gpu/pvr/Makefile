obj-$(CONFIG_PVR)	+= omaplfb.o pvrsrvkm.o

omaplfb-objs		:= omaplfb_displayclass.o omaplfb_linux.o

pvrsrvkm-objs		:= osfunc.o mmap.o module.o proc.o	   \
			   pvr_bridge_k.o mm.o event.o	   \
			   buffer_manager.o devicemem.o deviceclass.o	   \
			   handle.o hash.o pvrsrv.o queue.o ra.o	   \
			   resman.o power.o mem.o bridged_pvr_bridge.o	   \
			   sgxinit.o sgxreset.o sgxutils.o sgxkick.o	   \
			   sgxtransfer.o mmu.o pb.o perproc.o sysconfig.o  \
			   sysutils.o osperproc.o bridged_support.o	   \
			   bridged_sgx_bridge.o	sgxpower.o pvr_events.o

pvrsrvkm-objs-$(CONFIG_PVR_DEBUG) += pvr_debug.o
pvrsrvkm-objs-$(CONFIG_DEBUG_FS) += pvr_debugfs.o
pvrsrvkm-objs-$(CONFIG_PVR_TIMING) += pvr_debug.o
pvrsrvkm-objs-$(CONFIG_PVR_DEBUG_PDUMP) += pvr_pdump.o pvr_pdumpfs.o

pvrsrvkm-objs		+= $(pvrsrvkm-objs-y) $(pvrsrvkm-objs-m)

obj-$(CONFIG_PVR_EXAMPLES) += bc_example.o

bc_example-objs		:= bufferclass_example.o bufferclass_example_linux.o \
			   bufferclass_example_private.o


DATE			:= $(shell date "+%a %B %d %Z %Y" )
CBUILD			:= -O2						\
			   -DPVR_BUILD_DIR="\"$(PVR_BUILD_DIR)\""	\
			   -DPVR_BUILD_DATE="\"$(DATE)\""

ccflags-y		+= $(CBUILD) -include $(srctree)/$(src)/pvrconfig.h
ccflags-y		+= $(ccflags-m)
