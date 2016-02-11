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

#if defined (PDUMP)
#include "linuxsrv.h"
#endif

#include "pvr_drm_shared.h"

#if defined(SUPPORT_DRI_DRM)

#if defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
#include "3rdparty_dc_drm_shared.h"
#endif

#define	PVR_DRM_MAKENAME_HELPER(x, y) x ## y
#define	PVR_DRM_MAKENAME(x, y) PVR_DRM_MAKENAME_HELPER(x, y)

#if defined(PVR_DRI_DRM_PLATFORM_DEV)
#define	LDM_DEV	struct platform_device
#endif

int PVRCore_Init(void);
void PVRCore_Cleanup(void);
int PVRSRVOpen(struct drm_device *dev, struct drm_file *pFile);
void PVRSRVRelease(void *pvPrivData);

#if !defined(SUPPORT_DRI_DRM_PLUGIN)
#if defined(PVR_DRI_DRM_PLATFORM_DEV)
void PVRSRVDriverShutdown(LDM_DEV *pDevice);
int PVRSRVDriverSuspend(LDM_DEV *pDevice, pm_message_t state);
int PVRSRVDriverResume(LDM_DEV *pDevice);
#else
#if defined(SUPPORT_DRM_MODESET)
int PVRSRVDriverSuspend(struct pci_dev *pDevice, pm_message_t state);
int PVRSRVDriverResume(struct pci_dev *pDevice);
#else
int PVRSRVDriverSuspend(struct drm_device *pDevice, pm_message_t state);
int PVRSRVDriverResume(struct drm_device *pDevice);
#endif /* defined(SUPPORT_DRM_MODESET) */
#endif /* defined(PVR_DRI_DRM_PLATFORM_DEV) */
#endif /* !defined(SUPPORT_DRI_DRM_PLUGIN) */

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
/*
 * We need the command number names to begin with "DRM_" for newer versions
 * of the macro used to fill out the DRM ioctl table.  Similarly, the
 * ioctl number names must begin with "DRM_IOCTL_". The suffixes for the
 * two sets of strings must match (e.g. end with "PVR_SRVKM" in both
 * cases).
 */

#define	DRM_PVR_SRVKM		PVR_DRM_SRVKM_CMD
#define	DRM_PVR_IS_MASTER	PVR_DRM_IS_MASTER_CMD
#define	DRM_PVR_UNPRIV		PVR_DRM_UNPRIV_CMD
#define	DRM_PVR_DBGDRV		PVR_DRM_DBGDRV_CMD
#define	DRM_PVR_DISP		PVR_DRM_DISP_CMD

/*
 * Some versions of the kernel will dereference a NULL pointer if data is
 * is passed to an ioctl that doesn't expect any, so avoid using the _IO
 * macro, and use _IOW instead, specifying a dummy argument.
*/
typedef struct {
	char dummy[4];
} drm_pvr_dummy_arg;

/* IOCTL numbers */
#define	DRM_IOCTL_PVR_SRVKM	DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_SRVKM, PVRSRV_BRIDGE_PACKAGE)
#define	DRM_IOCTL_PVR_IS_MASTER DRM_IOW(DRM_COMMAND_BASE + DRM_PVR_IS_MASTER, drm_pvr_dummy_arg)
#define	DRM_IOCTL_PVR_UNPRIV	DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_UNPRIV, drm_pvr_unpriv_cmd)

#if defined(PDUMP)
#define	DRM_IOCTL_PVR_DBGDRV	DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_DBGDRV, IOCTL_PACKAGE)
#endif

#if defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
#define	DRM_IOCTL_PVR_DISP	DRM_IOWR(DRM_COMMAND_BASE + DRM_PVR_DISP, drm_pvr_display_cmd)
#endif
#endif	/* !defined(SUPPORT_DRI_DRM_EXT) */

#if defined(SUPPORT_DRI_DRM_PLUGIN)
typedef	struct {
	char *name;

	int (*load)(struct drm_device *dev, unsigned long flags);
	int (*unload)(struct drm_device *dev);

	int (*open)(struct drm_device *dev, struct drm_file *file);
	int (*release)(struct drm_device *dev, struct drm_file *file);

	int (*mmap)(struct file* pFile, struct vm_area_struct* ps_vma);

	struct drm_ioctl_desc *ioctls;
	int num_ioctls;
	int ioctl_start;
} PVRSRV_DRM_PLUGIN;

int SysDRMRegisterPlugin(PVRSRV_DRM_PLUGIN *psDRMPlugin);
void SysDRMUnregisterPlugin(PVRSRV_DRM_PLUGIN *psDRMPlugin);
#endif	/* defined(SUPPORT_DRI_DRM_PLUGIN) */

#endif	/* defined(SUPPORT_DRI_DRM) */

#endif /* defined(__PVR_DRM_H__) */


