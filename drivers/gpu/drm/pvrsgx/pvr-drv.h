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

#define PVR_UNPRIV_INIT_SUCCESFUL 0

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

/* OCP registers from OCP base, at least omaps have these */
#define SGX_OCP_REVISION		0x000
#define SGX_OCP_HWINFO			0x004
#define SGX_OCP_SYSCONFIG		0x010
#define SGX_OCP_IRQSTATUS_RAW_0		0x024
#define SGX_OCP_IRQSTATUS_RAW_1		0x028
#define SGX_OCP_IRQSTATUS_RAW_2		0x02c
#define SGX_OCP_IRQSTATUS_0		0x030
#define SGX_OCP_IRQSTATUS_1		0x034
#define SGX_OCP_IRQSTATUS_2		0x038
#define SGX_OCP_IRQENABLE_SET_0		0x03c
#define SGX_OCP_IRQENABLE_SET_1		0x040
#define SGX_OCP_IRQENABLE_SET_2		0x044
#define SGX_OCP_IRQENABLE_CLR_0		0x048
#define SGX_OCP_IRQENABLE_CLR_1		0x04c
#define SGX_OCP_IRQENABLE_CLR_2		0x050
#define SGX_OCP_PAGE_CONFIG		0x100
#define SGX_OCP_INTERRUPT_EVENT		0x104
#define SGX_OCP_DEBUG_CONFIG		0x108
#define SGX_OCP_DEBUG_STATUS_0		0x10c
#define SGX_OCP_DEBUG_STATUS_1		0x110	/* Only on omap4470 and later */

typedef enum _PVRSRV_INIT_SERVER_STATE_
{
	PVRSRV_INIT_SERVER_Unspecified		= -1,
	PVRSRV_INIT_SERVER_RUNNING			= 0,
	PVRSRV_INIT_SERVER_RAN				= 1,
	PVRSRV_INIT_SERVER_SUCCESSFUL		= 2,
	PVRSRV_INIT_SERVER_NUM				= 3,
	PVRSRV_INIT_SERVER_FORCE_I32 = 0x7fffffff

} PVRSRV_INIT_SERVER_STATE, *PPVRSRV_INIT_SERVER_STATE;

struct pvr;

u32 pvr_sgx_readl(struct pvr *ddata, unsigned short offset);
void pvr_sgx_writel(struct pvr *ddata, u32 val, unsigned short offset);
u32 pvr_ocp_readl(struct pvr *ddata, unsigned short offset);
void pvr_ocp_writel(struct pvr *ddata, u32 val, unsigned short offset);

/* We are currently calling these from the Imagination SDK */
int PVRCore_Init(void);
void PVRCore_Cleanup(void);
int PVRSRVOpen(struct drm_device *dev, struct drm_file *pFile);
void PVRSRVRelease(void *pvPrivData);
int PVRSRV_BridgeDispatchKM(struct drm_device *dev, void *arg, struct drm_file *pFile);
int PVRSRVDriverSuspend(struct platform_device *pdev, pm_message_t state);
int PVRSRVDriverResume(struct platform_device *pdev);
void PVRSRVDriverShutdown(struct platform_device *pdev);
int PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_STATE eInitServerState);
