/*************************************************************************/ /*!
@Title          PowerVR drm driver
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    linux module setup
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
#if defined(SUPPORT_DRI_DRM)

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <asm/ioctl.h>
#include <drm/drmP.h>
#include <drm/drm.h>

#if defined(SUPPORT_DRI_DRM_EXTERNAL)
#  include <omap_drm.h>
#  include <omap_drv.h>
#endif

#include "img_defs.h"
#include "services.h"
#include "kerneldisplay.h"
#include "kernelbuffer.h"
#include "syscommon.h"
#include "pvrmmap.h"
#include "mm.h"
#include "mmap.h"
#include "mutex.h"
#include "pvr_debug.h"
#include "srvkm.h"
#include "perproc.h"
#include "handle.h"
#include "pvr_bridge_km.h"
#include "pvr_bridge.h"
#include "proc.h"
#include "pvrmodule.h"
#include "pvrversion.h"
#include "lock.h"
#include "linkage.h"
#include "pvr_drm.h"
#include "private_data.h"

#if defined(PVR_DRI_DRM_NOT_PCI)
#include "pvr_drm_mod.h"
#endif

#define PVR_DRM_NAME	PVRSRV_MODNAME
#define PVR_DRM_DESC	"Imagination Technologies PVR DRM"

DECLARE_WAIT_QUEUE_HEAD(sWaitForInit);

IMG_BOOL bInitComplete;
IMG_BOOL bInitFailed;

#if !defined(PVR_DRI_DRM_NOT_PCI) && !defined(SUPPORT_DRI_DRM_EXTERNAL)
#if defined(PVR_DRI_DRM_PLATFORM_DEV)
struct platform_device *gpsPVRLDMDev;
#else
struct pci_dev *gpsPVRLDMDev;
#endif
#endif

struct drm_device *gpsPVRDRMDev;

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24))
#error "Linux kernel version 2.6.25 or later required for PVR DRM support"
#endif

#define PVR_DRM_FILE struct drm_file *

#if !defined(SUPPORT_DRI_DRM_EXT)
#if defined(PVR_DRI_DRM_PLATFORM_DEV)
#if defined(PVR_LDM_PLATFORM_PRE_REGISTERED) && !defined(SUPPORT_DRI_DRM_EXTERNAL)
static struct platform_device_id asPlatIdList[] = {
	{SYS_SGX_DEV_NAME, 0},
	{}
};
#endif
#else	/* defined(PVR_DRI_DRM_PLATFORM_DEV) */
static struct pci_device_id asPciIdList[] = {
#if defined(PVR_DRI_DRM_NOT_PCI)
	{1, 1, 1, 1, 0, 0, 0},
#else	/* defined(PVR_DRI_DRM_NOT_PCI) */
	{SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
#if defined(SYS_SGX_DEV1_DEVICE_ID)
	{SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV1_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
#endif	/* defined(SYS_SGX_DEV1_DEVICE_ID) */
#endif	/* defined(PVR_DRI_DRM_NOT_PCI) */
	{0}
};
#endif	/* defined(PVR_DRI_DRM_PLATFORM_DEV) */
#endif	/* !defined(SUPPORT_DRI_DRM_EXT) */

DRI_DRM_STATIC int
PVRSRVDrmLoad(struct drm_device *dev, unsigned long flags)
{
	int iRes = 0;

	PVR_TRACE(("PVRSRVDrmLoad"));

	gpsPVRDRMDev = dev;
#if !defined(PVR_DRI_DRM_NOT_PCI) && !defined(SUPPORT_DRI_DRM_EXTERNAL)
#if defined(PVR_DRI_DRM_PLATFORM_DEV)
	gpsPVRLDMDev = dev->platformdev;
#else
	gpsPVRLDMDev = dev->pdev;
#endif
#endif

#if defined(PDUMP)
	iRes = dbgdrv_init();
	if (iRes != 0)
	{
		goto exit;
	}
#endif
	/* Module initialisation */
	iRes = PVRCore_Init();
	if (iRes != 0)
	{
		goto exit_dbgdrv_cleanup;
	}

#if defined(DISPLAY_CONTROLLER)
	iRes = PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Init)(dev);
	if (iRes != 0)
	{
		goto exit_pvrcore_cleanup;
	}
#endif
	goto exit;

#if defined(DISPLAY_CONTROLLER)
exit_pvrcore_cleanup:
	PVRCore_Cleanup();
#endif
exit_dbgdrv_cleanup:
#if defined(PDUMP)
	dbgdrv_cleanup();
#endif
exit:
	if (iRes != 0)
	{
		bInitFailed = IMG_TRUE;
	}
	bInitComplete = IMG_TRUE;

	wake_up_interruptible(&sWaitForInit);

	return iRes;
}

DRI_DRM_STATIC int
PVRSRVDrmUnload(struct drm_device *dev)
{
	PVR_TRACE(("PVRSRVDrmUnload"));

#if defined(DISPLAY_CONTROLLER)
	PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(dev);
#endif

	PVRCore_Cleanup();

#if defined(PDUMP)
	dbgdrv_cleanup();
#endif

	return 0;
}

DRI_DRM_STATIC int
PVRSRVDrmOpen(struct drm_device *dev, struct drm_file *file)
{
	while (!bInitComplete)
	{
		DEFINE_WAIT(sWait);

		prepare_to_wait(&sWaitForInit, &sWait, TASK_INTERRUPTIBLE);

		if (!bInitComplete)
		{
			PVR_TRACE(("%s: Waiting for module initialisation to complete", __FUNCTION__));

			schedule();
		}

		finish_wait(&sWaitForInit, &sWait);

		if (signal_pending(current))
		{
			return -ERESTARTSYS;
		}
	}

	if (bInitFailed)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Module initialisation failed", __FUNCTION__));
		return -EINVAL;
	}

	return PVRSRVOpen(dev, file);
}

#if defined(SUPPORT_DRI_DRM_EXT) && !defined(PVR_LINUX_USING_WORKQUEUES)
DRI_DRM_STATIC void
PVRSRVDrmPostClose(struct drm_device *dev, struct drm_file *file)
{
	PVRSRVRelease(get_private(file));
	set_private(file, IMG_NULL);
}
#elif defined(SUPPORT_DRI_DRM_EXTERNAL)
DRI_DRM_STATIC int
PVRSRVDrmRelease(struct drm_device *dev, struct drm_file *file)
{
	void *psDriverPriv = get_private(file);

	PVR_TRACE(("PVRSRVDrmRelease: psDriverPriv=%p", psDriverPriv));

	if (psDriverPriv)
	{
		PVRSRVRelease(psDriverPriv);
	}

	set_private(file, IMG_NULL);

	return 0;
}
#else
DRI_DRM_STATIC int
PVRSRVDrmRelease(struct inode *inode, struct file *filp)
{
	struct drm_file *file_priv = filp->private_data;
	void *psDriverPriv = get_private(file_priv);
	int ret;

	ret = drm_release(inode, filp);

	if (ret != 0)
	{
		/*
		 * An error means drm_release didn't call drm_lastclose,
		 * but it will have freed file_priv.
		 */
		PVR_DPF((PVR_DBG_ERROR, "%s : drm_release failed: %d",
			__FUNCTION__, ret));
	}

	PVRSRVRelease(psDriverPriv);

	return 0;
}
#endif

DRI_DRM_STATIC int
PVRDRMIsMaster(struct drm_device *dev, void *arg, struct drm_file *pFile)
{
	return 0;
}

#if defined(SUPPORT_DRI_DRM_EXT)
int
PVRDRM_Dummy_ioctl(struct drm_device *dev, void *arg, struct drm_file *pFile)
{
	return 0;
}
#endif

DRI_DRM_STATIC int
PVRDRMUnprivCmd(struct drm_device *dev, void *arg, struct drm_file *pFile)
{
	int ret = 0;

	LinuxLockMutexNested(&gPVRSRVLock, PVRSRV_LOCK_CLASS_BRIDGE);

	if (arg == NULL)
	{
		ret = -EFAULT;
	}
	else
	{
		IMG_UINT32 *pui32Args = (IMG_UINT32 *)arg;
		IMG_UINT32 ui32Cmd = pui32Args[0];
		IMG_UINT32 *pui32OutArg = (IMG_UINT32 *)arg;

		switch (ui32Cmd)
		{
			case PVR_DRM_UNPRIV_INIT_SUCCESFUL:
				*pui32OutArg = PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_SUCCESSFUL) ? 1 : 0;
				break;

			default:
				ret = -EFAULT;
		}

	}

	LinuxUnLockMutex(&gPVRSRVLock);

	return ret;
}

#if defined(DISPLAY_CONTROLLER) && defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
static int
PVRDRM_Display_ioctl(struct drm_device *dev, void *arg, struct drm_file *pFile)
{
	int res;

	LinuxLockMutexNested(&gPVRSRVLock, PVRSRV_LOCK_CLASS_BRIDGE);

	res = PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Ioctl)(dev, arg, pFile);

	LinuxUnLockMutex(&gPVRSRVLock);

	return res;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33))
#define	PVR_DRM_FOPS_IOCTL	.unlocked_ioctl
#define	PVR_DRM_UNLOCKED	DRM_UNLOCKED
#else
#define	PVR_DRM_FOPS_IOCTL	.ioctl
#define	PVR_DRM_UNLOCKED	0
#endif

#if !defined(SUPPORT_DRI_DRM_EXT)
struct drm_ioctl_desc sPVRDrmIoctls[] = {
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,35))
	DRM_IOCTL_DEF(PVR_DRM_SRVKM_IOCTL, PVRSRV_BridgeDispatchKM, PVR_DRM_UNLOCKED),
	DRM_IOCTL_DEF(PVR_DRM_IS_MASTER_IOCTL, PVRDRMIsMaster, DRM_MASTER | PVR_DRM_UNLOCKED),
	DRM_IOCTL_DEF(PVR_DRM_UNPRIV_IOCTL, PVRDRMUnprivCmd, PVR_DRM_UNLOCKED),
#if defined(PDUMP)
	DRM_IOCTL_DEF(PVR_DRM_DBGDRV_IOCTL, dbgdrv_ioctl, PVR_DRM_UNLOCKED),
#endif
#if defined(DISPLAY_CONTROLLER) && defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
	DRM_IOCTL_DEF(PVR_DRM_DISP_IOCTL, PVRDRM_Display_ioctl, DRM_MASTER | PVR_DRM_UNLOCKED)
#endif
#else
	DRM_IOCTL_DEF_DRV(PVR_SRVKM, PVRSRV_BridgeDispatchKM, PVR_DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PVR_IS_MASTER, PVRDRMIsMaster, DRM_MASTER | PVR_DRM_UNLOCKED),
	DRM_IOCTL_DEF_DRV(PVR_UNPRIV, PVRDRMUnprivCmd, PVR_DRM_UNLOCKED),
#if defined(PDUMP)
	DRM_IOCTL_DEF_DRV(PVR_DBGDRV, dbgdrv_ioctl, PVR_DRM_UNLOCKED),
#endif
#if defined(DISPLAY_CONTROLLER) && defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
	DRM_IOCTL_DEF_DRV(PVR_DISP, PVRDRM_Display_ioctl, DRM_MASTER | PVR_DRM_UNLOCKED)
#endif
#endif
};

#if !defined(SUPPORT_DRI_DRM_EXTERNAL)
static int pvr_max_ioctl = DRM_ARRAY_SIZE(sPVRDrmIoctls);
#endif

#if defined(SUPPORT_DRI_DRM_EXTERNAL)
int pvr_ioctl_base;
int pvr_mapper_id;
static struct omap_drm_plugin plugin = {
		.name = PVR_DRM_NAME,

		.open = PVRSRVDrmOpen,
		.load = PVRSRVDrmLoad,
		.unload = PVRSRVDrmUnload,

		.release = PVRSRVDrmRelease,

		.ioctls = sPVRDrmIoctls,
		.num_ioctls = ARRAY_SIZE(sPVRDrmIoctls),
		.ioctl_base = 0,  /* initialized when plugin is registered */
};
#else	/* defined(SUPPORT_DRI_DRM_EXTERNAL) */
static struct drm_driver sPVRDrmDriver = 
{
#if defined(PVR_DRI_DRM_PLATFORM_DEV)
	.driver_features = DRIVER_USE_PLATFORM_DEVICE,
#else
	.driver_features = 0,
#endif
	.dev_priv_size = 0,
	.load = PVRSRVDrmLoad,
	.unload = PVRSRVDrmUnload,
	.open = PVRSRVDrmOpen,
#if !defined(PVR_DRI_DRM_PLATFORM_DEV)
	.suspend = PVRSRVDriverSuspend,
	.resume = PVRSRVDriverResume,
#endif
	.get_map_ofs = drm_core_get_map_ofs,
	.get_reg_ofs = drm_core_get_reg_ofs,
	.ioctls = sPVRDrmIoctls,
	.fops = 
	{
		.owner = THIS_MODULE,
		.open = drm_open,
		.release = PVRSRVDrmRelease,
		PVR_DRM_FOPS_IOCTL = drm_ioctl,
		.mmap = PVRMMap,
		.poll = drm_poll,
		.fasync = drm_fasync,
	},
#if defined(PVR_DRI_DRM_PLATFORM_DEV)
	.platform_driver =
	{
		.id_table = asPlatIdList,
		.driver =
		{
			.name = PVR_DRM_NAME,
		},
		.probe = PVRSRVDrmProbe,
		.remove = PVRSRVDrmRemove,
		.suspend = PVRSRVDriverSuspend,
		.resume = PVRSRVDriverResume,
		.shutdown = PVRSRVDriverShutdown,
	},
#else
	.pci_driver = 
	{
		.name = PVR_DRM_NAME,
		.id_table = asPciIdList,
	},
#endif
	.name = PVR_DRM_NAME,
	.desc = PVR_DRM_DESC,
	.date = PVR_BUILD_DATE,
	.major = PVRVERSION_MAJ,
	.minor = PVRVERSION_MIN,
	.patchlevel = PVRVERSION_BUILD,
};
#endif

#if defined(PVR_DRI_DRM_PLATFORM_DEV) && !defined(SUPPORT_DRI_DRM_EXT) && !defined(SUPPORT_DRI_DRM_EXTERNAL)
static int
PVRSRVDrmProbe(struct platform_device *pDevice)
{
	PVR_TRACE(("PVRSRVDrmProbe"));

	return drm_get_platform_dev(pDevice, &sPVRDrmDriver);
}

static int
PVRSRVDrmRemove(struct platform_device *pDevice)
{
	PVR_TRACE(("PVRSRVDrmRemove"));

	drm_put_dev(gpsPVRDRMDev);

	return 0;
}
#endif

static int __init PVRSRVDrmInit(void)
{
	int iRes;

#if !defined(SUPPORT_DRI_DRM_EXTERNAL)
	sPVRDrmDriver.num_ioctls = pvr_max_ioctl;
#endif

	PVRDPFInit();

#if defined(PVR_DRI_DRM_NOT_PCI)
	iRes = drm_pvr_dev_add();
	if (iRes != 0)
	{
		return iRes;
	}
#endif

#if defined(SUPPORT_DRI_DRM_EXTERNAL)
	iRes = omap_drm_register_plugin(&plugin);
	pvr_ioctl_base = plugin.ioctl_base;
	pvr_mapper_id = omap_drm_register_mapper();
#else
	iRes = drm_init(&sPVRDrmDriver);
#endif

#if defined(PVR_DRI_DRM_NOT_PCI)
	if (iRes != 0)
	{
		drm_pvr_dev_remove();
	}
#endif
	return iRes;
}
	
static void __exit PVRSRVDrmExit(void)
{
#if defined(SUPPORT_DRI_DRM_EXTERNAL)
	omap_drm_unregister_mapper(pvr_mapper_id);
	omap_drm_unregister_plugin(&plugin);
#else
	drm_exit(&sPVRDrmDriver);

#if defined(PVR_DRI_DRM_NOT_PCI)
	drm_pvr_dev_remove();
#endif
#endif
}

/*
 * These macro calls define the initialisation and removal functions of the
 * driver.  Although they are prefixed `module_', they apply when compiling
 * statically as well; in both cases they define the function the kernel will
 * run to start/stop the driver.
*/
module_init(PVRSRVDrmInit);
module_exit(PVRSRVDrmExit);
#endif
#endif

