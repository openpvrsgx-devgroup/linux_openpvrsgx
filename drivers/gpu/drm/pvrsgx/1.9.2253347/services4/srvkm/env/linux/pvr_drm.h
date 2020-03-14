/*************************************************************************/ /*!
@Title          PowerVR drm driver
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    drm module 
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/
#if !defined(__PVR_DRM_H__)
#define __PVR_DRM_H__

#include <drm/drm_device.h>
#include <linux/platform_device.h>

#include "pvr_drm_shared.h"

#if defined(SUPPORT_DRI_DRM)
#define	PVR_DRM_MAKENAME_HELPER(x, y) x ## y
#define	PVR_DRM_MAKENAME(x, y) PVR_DRM_MAKENAME_HELPER(x, y)

#if defined(PVR_DRI_DRM_PLATFORM_DEV)
struct platform_device;
#define	LDM_DEV	struct platform_device
#endif

int PVRCore_Init(void);
void PVRCore_Cleanup(void);
int PVRSRVOpen(struct drm_device *dev, struct drm_file *pFile);
void PVRSRVRelease(void *pvPrivData);

#if defined(SUPPORT_DRI_DRM_EXTERNAL) || defined(PVR_DRI_DRM_PLATFORM_DEV)
int PVRSRVDriverProbe(LDM_DEV *pDevice);
int PVRSRVDriverRemove(LDM_DEV *pDevice);
int PVRSRVDriverSuspend(LDM_DEV *pDevice, pm_message_t state);
int PVRSRVDriverResume(LDM_DEV *pDevice);
void PVRSRVDriverShutdown(LDM_DEV *pDevice);
#else
int PVRSRVDriverSuspend(struct drm_device *pDevice, pm_message_t state);
int PVRSRVDriverResume(struct drm_device *pDevice);
#endif

int PVRSRV_BridgeDispatchKM(struct drm_device *dev, void *arg, struct drm_file *pFile);

#if defined(SUPPORT_DRI_DRM_EXT)
#define	DRI_DRM_STATIC
/*Exported functions to common drm layer*/
int PVRSRVDrmLoad(struct drm_device *dev, unsigned long flags);
int PVRSRVDrmUnload(struct drm_device *dev);
int PVRSRVDrmOpen(struct drm_device *dev, struct drm_file *file);
#if defined(PVR_LINUX_USING_WORKQUEUES)
DRI_DRM_STATIC int PVRSRVDrmRelease(struct inode *inode, struct file *filp);
#else
void PVRSRVDrmPostClose(struct drm_device *dev, struct drm_file *file);
#endif
int PVRDRMIsMaster(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
int PVRDRMUnprivCmd(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
int PVRDRM_Dummy_ioctl(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
#else
#define	DRI_DRM_STATIC	static
#endif	/* defined(SUPPORT_DRI_DRM_EXT) */

#if defined(DISPLAY_CONTROLLER)
extern int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Init)(struct drm_device *);
extern void PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(struct drm_device *);
extern int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Suspend)(struct drm_device *);
extern int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Resume)(struct drm_device *);
#if defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
extern int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Ioctl)(struct drm_device *dev, void *arg, struct drm_file *pFile);
#endif
#endif

#if defined(PDUMP)
int dbgdrv_init(void);
void dbgdrv_cleanup(void);
IMG_INT dbgdrv_ioctl(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
#endif

#if !defined(SUPPORT_DRI_DRM_EXT)
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,35))
#define        PVR_DRM_SRVKM_IOCTL     _IOWR(0, DRM_PVR_SRVKM, PVRSRV_BRIDGE_PACKAGE)
#define        PVR_DRM_DISP_IOCTL      _IOWR(0, DRM_PVR_DISP, uint32_t[PVR_DRM_DISP_NUM_ARGS])
#define        PVR_DRM_IS_MASTER_IOCTL _IO(0, DRM_PVR_IS_MASTER)
#define        PVR_DRM_UNPRIV_IOCTL    _IOWR(0, DRM_PVR_UNPRIV, IMG_UINT32)
#define        PVR_DRM_DBGDRV_IOCTL    _IOWR(0, DRM_PVR_DBGDRV, IOCTL_PACKAGE)
#else
#define        DRM_IOCTL_PVR_SRVKM     DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_SRVKM, PVRSRV_BRIDGE_PACKAGE)
#define        DRM_IOCTL_PVR_DISP      DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_DISP, uint32_t[PVR_DRM_DISP_NUM_ARGS])
#define        DRM_IOCTL_PVR_IS_MASTER DRM_IO(DRM_COMMAND_BASE + DRM_PVR_IS_MASTER)
#define        DRM_IOCTL_PVR_UNPRIV    DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_UNPRIV, IMG_UINT32)
#define        DRM_IOCTL_PVR_DBGDRV    DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_DBGDRV, IOCTL_PACKAGE)
#endif

#endif	

#endif	

#endif 


