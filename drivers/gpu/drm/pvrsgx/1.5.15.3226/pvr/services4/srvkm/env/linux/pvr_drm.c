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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/ioctl.h>
#include <drm/drmP.h>
#include <drm/drm.h>

#include "img_defs.h"
#include "services.h"
#include "kerneldisplay.h"
#include "kernelbuffer.h"
#include "sysconfig.h"
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
#include "pvrversion.h"
#include "linkage.h"
#include "pvr_drm_shared.h"
#include "pvr_drm.h"

#define	MAKENAME_HELPER(x, y) x ## y
#define	MAKENAME(x, y) MAKENAME_HELPER(x, y)

#define PVR_DRM_NAME	"pvrsrvkm"
#define PVR_DRM_DESC	"Imagination Technologies PVR DRM"

#define PVR_PCI_IDS \
	{SYS_SGX_DEV_VENDOR_ID, SYS_SGX_DEV_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0}, \
	{0, 0, 0}

struct pci_dev *gpsPVRLDMDev;
struct drm_device *gpsPVRDRMDev;

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,24))
#error "Linux kernel version 2.6.25 or later required for PVR DRM support"
#endif

#define PVR_DRM_FILE struct drm_file *

#if defined(DISPLAY_CONTROLLER)
extern int MAKENAME(DISPLAY_CONTROLLER, _Init)(struct drm_device *);
extern void MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(struct drm_device *);
#endif


#if !defined(SUPPORT_DRI_DRM_EXT)
static struct pci_device_id asPciIdList[] = {
	PVR_PCI_IDS
};
#endif

IMG_INT PVRSRVDrmLoad(struct drm_device *dev, unsigned long flags)
{
	IMG_INT iRes;

	PVR_TRACE(("PVRSRVDrmLoad"));

	gpsPVRDRMDev = dev;
	gpsPVRLDMDev = dev->pdev;

#if defined(PDUMP)
	iRes = dbgdrv_init();
	if (iRes != 0)
	{
		return iRes;
	}
#endif

	iRes = PVRCore_Init();
	if (iRes != 0)
	{
		goto exit_dbgdrv_cleanup;
	}

#if defined(DISPLAY_CONTROLLER)
	iRes = MAKENAME(DISPLAY_CONTROLLER, _Init)(dev);
	if (iRes != 0)
	{
		goto exit_pvrcore_cleanup;
	}
#endif
	return 0;

#if defined(DISPLAY_CONTROLLER)
exit_pvrcore_cleanup:
	PVRCore_Cleanup();
#endif
exit_dbgdrv_cleanup:
#if defined(PDUMP)
	dbgdrv_cleanup();
#endif
	return iRes;
}

IMG_INT PVRSRVDrmUnload(struct drm_device *dev)
{
	PVR_TRACE(("PVRSRVDrmUnload"));

#if defined(DISPLAY_CONTROLLER)
	MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(dev);
#endif

	PVRCore_Cleanup();

#if defined(PDUMP)
	dbgdrv_cleanup();
#endif

	return 0;
}

IMG_INT PVRSRVDrmOpen(struct drm_device *dev, struct drm_file *file)
{
	return PVRSRVOpen(dev, file);
}

IMG_VOID PVRSRVDrmPostClose(struct drm_device *dev, struct drm_file *file)
{
	PVRSRVRelease(dev, file);
}

DRI_DRM_STATIC IMG_INT
PVRDRMIsMaster(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile)
{
	return 0;
}

#if defined(SUPPORT_DRI_DRM_EXT)
IMG_INT
PVRDRM_Dummy_ioctl(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile)
{
	return 0;
}
#endif

static IMG_INT
PVRDRMPCIBusIDField(struct drm_device *dev, IMG_UINT32 *pui32Field, IMG_UINT32 ui32FieldType)
{
	struct pci_dev *psPCIDev = (struct pci_dev *)dev->pdev;

	switch (ui32FieldType)
	{
		case PVR_DRM_PCI_DOMAIN:
			*pui32Field = pci_domain_nr(psPCIDev->bus);
			break;

		case PVR_DRM_PCI_BUS:
			*pui32Field = psPCIDev->bus->number;
			break;

		case PVR_DRM_PCI_DEV:
			*pui32Field = PCI_SLOT(psPCIDev->devfn);
			break;

		case PVR_DRM_PCI_FUNC:
			*pui32Field = PCI_FUNC(psPCIDev->devfn);
			break;

		default:
			return -EFAULT;
	}

	return 0;
}

DRI_DRM_STATIC IMG_INT
PVRDRMUnprivCmd(struct drm_device *dev, IMG_VOID *arg, struct drm_file *pFile)
{
	IMG_UINT32 *pui32Args = (IMG_UINT32 *)arg;
	IMG_UINT32 ui32Cmd = pui32Args[0];
	IMG_UINT32 ui32Arg1 = pui32Args[1];
	IMG_UINT32 *pui32OutArg = (IMG_UINT32 *)arg;

	switch (ui32Cmd)
	{
		case PVR_DRM_UNPRIV_INIT_SUCCESFUL:
			*pui32OutArg = PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_SUCCESSFUL) ? 1 : 0;
			break;

		case PVR_DRM_UNPRIV_BUSID_TYPE:
			*pui32OutArg = PVR_DRM_BUS_TYPE_PCI;
			break;

		case PVR_DRM_UNPRIV_BUSID_FIELD:
			return PVRDRMPCIBusIDField(dev, pui32OutArg, ui32Arg1);

		default:
			return -EFAULT;
	}

	return 0;
}

#if !defined(SUPPORT_DRI_DRM_EXT)
struct drm_ioctl_desc sPVRDrmIoctls[] = {
	DRM_IOCTL_DEF(PVR_DRM_SRVKM_IOCTL, PVRSRV_BridgeDispatchKM, 0),
	DRM_IOCTL_DEF(PVR_DRM_IS_MASTER_IOCTL, PVRDRMIsMaster, DRM_MASTER),
	DRM_IOCTL_DEF(PVR_DRM_UNPRIV_IOCTL, PVRDRMUnprivCmd, 0),
#if defined(PDUMP)
	DRM_IOCTL_DEF(PVR_DRM_DBGDRV_IOCTL, dbgdrv_ioctl, 0),
#endif
};

static IMG_INT pvr_max_ioctl = DRM_ARRAY_SIZE(sPVRDrmIoctls);

static struct drm_driver sPVRDrmDriver =
{
	.driver_features = 0,
	.dev_priv_size = 0,
	.load = PVRSRVDrmLoad,
	.unload = PVRSRVDrmUnload,
	.open = PVRSRVDrmOpen,
	.postclose = PVRSRVDrmPostClose,
	.suspend = PVRSRVDriverSuspend,
	.resume = PVRSRVDriverResume,
	.get_map_ofs = drm_core_get_map_ofs,
	.get_reg_ofs = drm_core_get_reg_ofs,
	.ioctls = sPVRDrmIoctls,
	.fops =
	{
		.owner = THIS_MODULE,
		.open = drm_open,
		.release = drm_release,
		.ioctl = drm_ioctl,
		.mmap = PVRMMap,
		.poll = drm_poll,
		.fasync = drm_fasync,
	},
	.pci_driver =
	{
		.name = PVR_DRM_NAME,
		.id_table = asPciIdList,
	},

	.name = PVR_DRM_NAME,
	.desc = PVR_DRM_DESC,
	.date = PVR_BUILD_DATE,
	.major = PVRVERSION_MAJ,
	.minor = PVRVERSION_MIN,
	.patchlevel = PVRVERSION_BUILD,
};

static IMG_INT __init PVRSRVDrmInit(IMG_VOID)
{
	IMG_INT iRes;
	sPVRDrmDriver.num_ioctls = pvr_max_ioctl;


	PVRDPFInit();

	iRes = drm_init(&sPVRDrmDriver);

	return iRes;
}

static IMG_VOID __exit PVRSRVDrmExit(IMG_VOID)
{
	drm_exit(&sPVRDrmDriver);
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


