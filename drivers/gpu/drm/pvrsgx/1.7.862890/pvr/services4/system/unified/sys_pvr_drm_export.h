/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope it will be useful but, except 
 * as otherwise stated in writing, without any warranty; without even the 
 * implied warranty of merchantability or fitness for a particular purpose. 
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK 
 *
 ******************************************************************************/

#if !defined(__SYS_PVR_DRM_EXPORT_H__)
#define __SYS_PVR_DRM_EXPORT_H__

#include "pvr_drm_shared.h"

#if defined(__KERNEL__)

#include "services_headers.h"
#include "private_data.h"
#include "pvr_drm.h"

#include "pvr_bridge.h"

#if defined(PDUMP)
#include "linuxsrv.h"
#endif

#define PVR_DRM_SRVKM_IOCTL \
	DRM_IOW(DRM_COMMAND_BASE + PVR_DRM_SRVKM_CMD, PVRSRV_BRIDGE_PACKAGE)

#define PVR_DRM_DISP_IOCTL \
	DRM_IO(DRM_COMMAND_BASE + PVR_DRM_DISP_CMD)

#define PVR_DRM_BC_IOCTL \
	DRM_IO(DRM_COMMAND_BASE + PVR_DRM_BC_CMD)

#define	PVR_DRM_IS_MASTER_IOCTL \
	DRM_IO(DRM_COMMAND_BASE + PVR_DRM_IS_MASTER_CMD)

#define	PVR_DRM_UNPRIV_IOCTL \
	DRM_IOWR(DRM_COMMAND_BASE + PVR_DRM_UNPRIV_CMD, IMG_UINT32)

#if defined(PDUMP)
#define	PVR_DRM_DBGDRV_IOCTL \
	DRM_IOW(DRM_COMMAND_BASE + PVR_DRM_DBGDRV_CMD, IOCTL_PACKAGE)
#else
#define	PVR_DRM_DBGDRV_IOCTL \
	DRM_IO(DRM_COMMAND_BASE + PVR_DRM_DBGDRV_CMD)
#endif

int SYSPVRInit(void);
int SYSPVRLoad(struct drm_device *dev, unsigned long flags);
int SYSPVROpen(struct drm_device *dev, struct drm_file *pFile);
int SYSPVRUnload(struct drm_device *dev);
void SYSPVRPostClose(struct drm_device *dev, struct drm_file *file);
int SYSPVRBridgeDispatch(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
int SYSPVRDCDriverIoctl(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
int SYSPVRBCDriverIoctl(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
int SYSPVRIsMaster(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
int SYSPVRUnprivCmd(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);

int SYSPVRMMap(struct file* pFile, struct vm_area_struct* ps_vma);

int SYSPVRDBGDrivIoctl(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);

int SYSPVRServiceSGXInterrupt(struct drm_device *dev);

void SYSPVRSuspendLock(struct drm_device *dev);
void SYSPVRSuspendUnlock(struct drm_device *dev);
int SYSPVRPreSuspend(struct drm_device *dev);
int SYSPVRPostSuspend(struct drm_device *dev);
int SYSPVRResume(struct drm_device *dev);

int BC_Video_ModInit(void);
int BC_Video_ModCleanup(void);
int BC_Video_Bridge(struct drm_device *dev, IMG_VOID *arg, struct drm_file *file_priv);

#endif	

#endif	

