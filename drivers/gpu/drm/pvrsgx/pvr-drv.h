/* SPDX-License-Identifier: (GPL-2.0 OR MIT) */

struct pvr_ioctl {
	u32 ui32Cmd;			/* ioctl command */
	u32 ui32Size;			/* needs to be correctly set */
	void *pInBuffer;		/* input data buffer */
	u32 ui32InBufferSize;		/* size of input data buffer */
	void *pOutBuffer;		/* output data buffer */
	u32 ui32OutBufferSize;		/* size of output data buffer */
};

struct pvr_dummy {
	char dummy[4];
};

struct pvr_unpriv {
	u32 cmd;
	u32 res;
};

#define DRM_PVR_SRVKM		0x0
#define DRM_PVR_DISP		0x1
#define DRM_PVR_BC		0x2
#define DRM_PVR_IS_MASTER	0x3
#define DRM_PVR_UNPRIV		0x4
#define DRM_PVR_DBGDRV		0x5

#define	DRM_IOCTL_PVR_SRVKM	DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_SRVKM, PVRSRV_BRIDGE_PACKAGE)
#define	DRM_IOCTL_PVR_IS_MASTER	DRM_IOW(DRM_COMMAND_BASE + DRM_PVR_IS_MASTER, struct pvr_dummy)
#define	DRM_IOCTL_PVR_UNPRIV	DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_UNPRIV, struct pvr_unpriv)
#define	DRM_IOCTL_PVR_DBGDRV	DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_DBGDRV, struct pvr_ioctl)
#define	DRM_IOCTL_PVR_DISP	DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_DISP, drm_pvr_display_cmd)

/* We are currently calling these from the Imagination SDK */
int PVRCore_Init(void);
void PVRCore_Cleanup(void);
int PVRSRVOpen(struct drm_device *dev, struct drm_file *pFile);
void PVRSRVRelease(void *pvPrivData);
int PVRSRV_BridgeDispatchKM(struct drm_device *dev, void *arg, struct drm_file *pFile);
int PVRSRVDriverSuspend(struct platform_device *pdev, pm_message_t state);
int PVRSRVDriverResume(struct platform_device *pdev);
void PVRSRVDriverShutdown(struct platform_device *pdev);
