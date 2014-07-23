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

#if defined(SUPPORT_DRI_DRM)
IMG_INT PVRCore_Init(IMG_VOID);
IMG_VOID PVRCore_Cleanup(IMG_VOID);
IMG_INT PVRSRVOpen(struct drm_device *dev, struct drm_file *pFile);
IMG_INT PVRSRVRelease(struct drm_device *dev, struct drm_file *pFile);
IMG_INT PVRSRVDriverSuspend(struct drm_device *pDevice, pm_message_t state);
IMG_INT PVRSRVDriverResume(struct drm_device *pDevice);

IMG_INT PVRSRV_BridgeDispatchKM(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);

#if defined(SUPPORT_DRI_DRM_EXT)
#define	DRI_DRM_STATIC
IMG_INT PVRSRVDrmLoad(struct drm_device *dev, unsigned long flags);
IMG_INT PVRSRVDrmUnload(struct drm_device *dev);
IMG_VOID PVRSRVDrmPostClose(struct drm_device *dev, struct drm_file *file);
IMG_INT PVRDRMIsMaster(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
IMG_INT PVRDRMUnprivCmd(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
IMG_INT PVRDRM_Dummy_ioctl(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
#else
#define	DRI_DRM_STATIC	static
#endif

#if defined(PDUMP)
int dbgdrv_init(void);
void dbgdrv_cleanup(void);
IMG_INT dbgdrv_ioctl(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile);
#endif

#if !defined(SUPPORT_DRI_DRM_EXT)
#define	PVR_DRM_SRVKM_IOCTL	_IO(0, PVR_DRM_SRVKM_CMD)
#define	PVR_DRM_IS_MASTER_IOCTL _IO(0, PVR_DRM_IS_MASTER_CMD)
#define	PVR_DRM_UNPRIV_IOCTL	_IO(0, PVR_DRM_UNPRIV_CMD)
#define	PVR_DRM_DBGDRV_IOCTL	_IO(0, PVR_DRM_DBGDRV_CMD)
#endif

#endif

#endif


