override KERNEL_COMPONENTS := srvkm bufferclass_example
override EXTRA_PVRSRVKM_COMPONENTS := dc_sunxi dbgdrv
override PVR_SYSTEM := sunxi
override BUILD := release
override DEBUGLINK := 1
override OPTIM := -Os
override SUPPORT_PVRSRV_DEVICE_CLASS := 1
override SUPPORT_DRI_DRM := 1
