/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
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

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <asm/atomic.h>

#if defined(SUPPORT_DRI_DRM)
#include <drm/drmP.h>
#else
#include <linux/module.h>
#endif

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/omapfb.h>
#include <linux/mutex.h>

#if defined(PVR_OMAPLFB_DRM_FB)
#include <plat/display.h>
#include <linux/omap_gpu.h>
#else	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#define PVR_OMAPFB3_NEEDS_PLAT_VRFB_H
#endif

#if defined(PVR_OMAPFB3_NEEDS_PLAT_VRFB_H)
#include <plat/vrfb.h>
#else
#if defined(PVR_OMAPFB3_NEEDS_MACH_VRFB_H)
#include <mach/vrfb.h>
#endif
#endif

#if defined(DEBUG)
#define	PVR_DEBUG DEBUG
#undef DEBUG
#endif
#include <drivers/video/omap2/omapfb/omapfb.h>
#if defined(DEBUG)
#undef DEBUG
#endif
#if defined(PVR_DEBUG)
#define	DEBUG PVR_DEBUG
#undef PVR_DEBUG
#endif
#endif	

#if defined(CONFIG_DSSCOMP)
#include <mach/tiler.h>
#include <video/dsscomp.h>
#include <plat/dsscomp.h>
#endif 

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "omaplfb.h"
#include "pvrmodule.h"
#if defined(SUPPORT_DRI_DRM)
#include "pvr_drm.h"
#include "3rdparty_dc_drm_shared.h"
#endif

#if !defined(PVR_LINUX_USING_WORKQUEUES)
#error "PVR_LINUX_USING_WORKQUEUES must be defined"
#endif

MODULE_SUPPORTED_DEVICE(DEVNAME);

#if !defined(PVR_OMAPLFB_DRM_FB)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#define OMAP_DSS_DRIVER(drv, dev) struct omap_dss_driver *drv = (dev) != NULL ? (dev)->driver : NULL
#define OMAP_DSS_MANAGER(man, dev) struct omap_overlay_manager *man = (dev) != NULL ? (dev)->manager : NULL
#define	WAIT_FOR_VSYNC(man)	((man)->wait_for_vsync)
#else
#define OMAP_DSS_DRIVER(drv, dev) struct omap_dss_device *drv = (dev)
#define OMAP_DSS_MANAGER(man, dev) struct omap_dss_device *man = (dev)
#define	WAIT_FOR_VSYNC(man)	((man)->wait_vsync)
#endif
#endif	

void *OMAPLFBAllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void OMAPLFBFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

void OMAPLFBCreateSwapChainLockInit(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_init(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBCreateSwapChainLockDeInit(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_destroy(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBCreateSwapChainLock(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_lock(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBCreateSwapChainUnLock(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_unlock(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBAtomicBoolInit(OMAPLFB_ATOMIC_BOOL *psAtomic, OMAPLFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

void OMAPLFBAtomicBoolDeInit(OMAPLFB_ATOMIC_BOOL *psAtomic)
{
}

void OMAPLFBAtomicBoolSet(OMAPLFB_ATOMIC_BOOL *psAtomic, OMAPLFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

OMAPLFB_BOOL OMAPLFBAtomicBoolRead(OMAPLFB_ATOMIC_BOOL *psAtomic)
{
	return (OMAPLFB_BOOL)atomic_read(psAtomic);
}

void OMAPLFBAtomicIntInit(OMAPLFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

void OMAPLFBAtomicIntDeInit(OMAPLFB_ATOMIC_INT *psAtomic)
{
}

void OMAPLFBAtomicIntSet(OMAPLFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

int OMAPLFBAtomicIntRead(OMAPLFB_ATOMIC_INT *psAtomic)
{
	return atomic_read(psAtomic);
}

void OMAPLFBAtomicIntInc(OMAPLFB_ATOMIC_INT *psAtomic)
{
	atomic_inc(psAtomic);
}

OMAPLFB_ERROR OMAPLFBGetLibFuncAddr (char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
	{
		return (OMAPLFB_ERROR_INVALID_PARAMS);
	}

	
	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return (OMAPLFB_OK);
}

void OMAPLFBQueueBufferForSwap(OMAPLFB_SWAPCHAIN *psSwapChain, OMAPLFB_BUFFER *psBuffer)
{
	int res = queue_work(psSwapChain->psWorkQueue, &psBuffer->sWork);

	if (res == 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Buffer already on work queue\n", __FUNCTION__, psSwapChain->uiFBDevID);
	}
}

static void WorkQueueHandler(struct work_struct *psWork)
{
	OMAPLFB_BUFFER *psBuffer = container_of(psWork, OMAPLFB_BUFFER, sWork);

	OMAPLFBSwapHandler(psBuffer);
}

OMAPLFB_ERROR OMAPLFBCreateSwapQueue(OMAPLFB_SWAPCHAIN *psSwapChain)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
	
	psSwapChain->psWorkQueue = alloc_ordered_workqueue(DEVNAME, WQ_FREEZABLE | WQ_MEM_RECLAIM);
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	psSwapChain->psWorkQueue = create_freezable_workqueue(DEVNAME);
#else
	
	psSwapChain->psWorkQueue = __create_workqueue(DEVNAME, 1, 1, 1);
#endif
#endif
	if (psSwapChain->psWorkQueue == NULL)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: Couldn't create workqueue\n", __FUNCTION__, psSwapChain->uiFBDevID);

		return (OMAPLFB_ERROR_INIT_FAILURE);
	}

	return (OMAPLFB_OK);
}

void OMAPLFBInitBufferForSwap(OMAPLFB_BUFFER *psBuffer)
{
	INIT_WORK(&psBuffer->sWork, WorkQueueHandler);
}

void OMAPLFBDestroySwapQueue(OMAPLFB_SWAPCHAIN *psSwapChain)
{
	destroy_workqueue(psSwapChain->psWorkQueue);
}

void OMAPLFBFlip(OMAPLFB_DEVINFO *psDevInfo, OMAPLFB_BUFFER *psBuffer)
{
	struct fb_var_screeninfo sFBVar;
	int res;
	unsigned long ulYResVirtual;

	OMAPLFB_CONSOLE_LOCK();

	sFBVar = psDevInfo->psLINFBInfo->var;

	sFBVar.xoffset = 0;
	sFBVar.yoffset = psBuffer->ulYOffset;

	ulYResVirtual = psBuffer->ulYOffset + sFBVar.yres;

#if defined(CONFIG_DSSCOMP)
	{
		
		struct fb_fix_screeninfo sFBFix = psDevInfo->psLINFBInfo->fix;
		struct dsscomp_setup_dispc_data d =
		{
			.num_ovls = 1,
			.num_mgrs = 1,
			.mgrs[0].alpha_blending = 1,
			.ovls[0] =
			{
				.cfg =
				{
					.win.w = sFBVar.xres,
					.win.h = sFBVar.yres,
					.crop.x = sFBVar.xoffset,
					.crop.y = sFBVar.yoffset,
					.crop.w = sFBVar.xres,
					.crop.h = sFBVar.yres,
					.width = sFBVar.xres_virtual,
					.height = sFBVar.yres_virtual,
					.stride = sFBFix.line_length,
					.enabled = 1,
					.global_alpha = 255,
				},
			},
		};

		
		struct tiler_pa_info *pas[] = { NULL };

		d.ovls[0].ba = sFBFix.smem_start;
		omapfb_mode_to_dss_mode(&sFBVar, &d.ovls[0].cfg.color_mode);

		res = dsscomp_gralloc_queue(&d, pas, true, NULL, NULL);
	}
#else 
	
#if !defined(PVR_OMAPLFB_DONT_USE_FB_PAN_DISPLAY)
	
	if (sFBVar.xres_virtual != sFBVar.xres || sFBVar.yres_virtual < ulYResVirtual)
#endif 
	{
		sFBVar.xres_virtual = sFBVar.xres;
		sFBVar.yres_virtual = ulYResVirtual;

		sFBVar.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;

		res = fb_set_var(psDevInfo->psLINFBInfo, &sFBVar);
		if (res != 0)
		{
			printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: fb_set_var failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
		}
	}
#if !defined(PVR_OMAPLFB_DONT_USE_FB_PAN_DISPLAY)
	else
	{
		res = fb_pan_display(psDevInfo->psLINFBInfo, &sFBVar);
		if (res != 0)
		{
			printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: fb_pan_display failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
		}
	}
#endif 
#endif 

	OMAPLFB_CONSOLE_UNLOCK();
}

#if !defined(PVR_OMAPLFB_DRM_FB) || defined(DEBUG)
static OMAPLFB_BOOL OMAPLFBValidateDSSUpdateMode(enum omap_dss_update_mode eMode)
{
	switch (eMode)
	{
		case OMAP_DSS_UPDATE_AUTO:
		case OMAP_DSS_UPDATE_MANUAL:
		case OMAP_DSS_UPDATE_DISABLED:
			return OMAPLFB_TRUE;
		default:
			break;
	}

	return OMAPLFB_FALSE;
}

static OMAPLFB_UPDATE_MODE OMAPLFBFromDSSUpdateMode(enum omap_dss_update_mode eMode)
{
	switch (eMode)
	{
		case OMAP_DSS_UPDATE_AUTO:
			return OMAPLFB_UPDATE_MODE_AUTO;
		case OMAP_DSS_UPDATE_MANUAL:
			return OMAPLFB_UPDATE_MODE_MANUAL;
		case OMAP_DSS_UPDATE_DISABLED:
			return OMAPLFB_UPDATE_MODE_DISABLED;
		default:
			break;
	}

	return OMAPLFB_UPDATE_MODE_UNDEFINED;
}
#endif

static OMAPLFB_BOOL OMAPLFBValidateUpdateMode(OMAPLFB_UPDATE_MODE eMode)
{
	switch(eMode)
	{
		case OMAPLFB_UPDATE_MODE_AUTO:
		case OMAPLFB_UPDATE_MODE_MANUAL:
		case OMAPLFB_UPDATE_MODE_DISABLED:
			return OMAPLFB_TRUE;
		default:
			break;
	}

	return OMAPLFB_FALSE;
}

static enum omap_dss_update_mode OMAPLFBToDSSUpdateMode(OMAPLFB_UPDATE_MODE eMode)
{
	switch(eMode)
	{
		case OMAPLFB_UPDATE_MODE_AUTO:
			return OMAP_DSS_UPDATE_AUTO;
		case OMAPLFB_UPDATE_MODE_MANUAL:
			return OMAP_DSS_UPDATE_MANUAL;
		case OMAPLFB_UPDATE_MODE_DISABLED:
			return OMAP_DSS_UPDATE_DISABLED;
		default:
			break;
	}

	return -1;
}

#if defined(DEBUG)
static const char *OMAPLFBUpdateModeToString(OMAPLFB_UPDATE_MODE eMode)
{
	switch(eMode)
	{
		case OMAPLFB_UPDATE_MODE_AUTO:
			return "Auto Update Mode";
		case OMAPLFB_UPDATE_MODE_MANUAL:
			return "Manual Update Mode";
		case OMAPLFB_UPDATE_MODE_DISABLED:
			return "Update Mode Disabled";
		case OMAPLFB_UPDATE_MODE_UNDEFINED:
			return "Update Mode Undefined";
		default:
			break;
	}

	return "Unknown Update Mode";
}

static const char *OMAPLFBDSSUpdateModeToString(enum omap_dss_update_mode eMode)
{
	if (!OMAPLFBValidateDSSUpdateMode(eMode))
	{
		return "Unknown Update Mode";
	}

	return OMAPLFBUpdateModeToString(OMAPLFBFromDSSUpdateMode(eMode));
}

void OMAPLFBPrintInfo(OMAPLFB_DEVINFO *psDevInfo)
{
#if defined(PVR_OMAPLFB_DRM_FB)
	struct drm_connector *psConnector;
	unsigned uConnectors;
	unsigned uConnector;

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: DRM framebuffer\n", psDevInfo->uiFBDevID));

	for (psConnector = NULL, uConnectors = 0;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		uConnectors++;
	}

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: Number of screens (DRM connectors): %u\n", psDevInfo->uiFBDevID, uConnectors));

	if (uConnectors == 0)
	{
		return;
	}

	for (psConnector = NULL, uConnector = 0;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL; uConnector++)
	{
		enum omap_dss_update_mode eMode = omap_connector_get_update_mode(psConnector);

		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: Screen %u: %s (%d)\n", psDevInfo->uiFBDevID, uConnector, OMAPLFBDSSUpdateModeToString(eMode), (int)eMode));

	}
#else	
	OMAPLFB_UPDATE_MODE eMode = OMAPLFBGetUpdateMode(psDevInfo);

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: non-DRM framebuffer\n", psDevInfo->uiFBDevID));

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: %s\n", psDevInfo->uiFBDevID, OMAPLFBUpdateModeToString(eMode)));
#endif	
}
#endif	

OMAPLFB_UPDATE_MODE OMAPLFBGetUpdateMode(OMAPLFB_DEVINFO *psDevInfo)
{
#if defined(PVR_OMAPLFB_DRM_FB)
	struct drm_connector *psConnector;
	OMAPLFB_UPDATE_MODE eMode = OMAPLFB_UPDATE_MODE_UNDEFINED;

	
	for (psConnector = NULL;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		switch(omap_connector_get_update_mode(psConnector))
		{
			case OMAP_DSS_UPDATE_MANUAL:
				eMode = OMAPLFB_UPDATE_MODE_MANUAL;
				break;
			case OMAP_DSS_UPDATE_DISABLED:
				if (eMode == OMAPLFB_UPDATE_MODE_UNDEFINED)
				{
					eMode = OMAPLFB_UPDATE_MODE_DISABLED;
				}
				break;
			case OMAP_DSS_UPDATE_AUTO:
				
			default:
				
				if (eMode != OMAPLFB_UPDATE_MODE_MANUAL)
				{
					eMode = OMAPLFB_UPDATE_MODE_AUTO;
				}
				break;
		}
	}

	return eMode;
#else	
	struct omap_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	OMAP_DSS_DRIVER(psDSSDrv, psDSSDev);

	enum omap_dss_update_mode eMode;

	if (psDSSDrv == NULL)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: No DSS device\n", __FUNCTION__, psDevInfo->uiFBDevID));
		return OMAPLFB_UPDATE_MODE_UNDEFINED;
	}

	if (psDSSDrv->get_update_mode == NULL)
	{
		if (strcmp(psDSSDev->name, "hdmi") == 0)
		{
			return OMAPLFB_UPDATE_MODE_AUTO;
		}
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: No get_update_mode function\n", __FUNCTION__, psDevInfo->uiFBDevID));
		return OMAPLFB_UPDATE_MODE_UNDEFINED;
	}

	eMode = psDSSDrv->get_update_mode(psDSSDev);
	if (!OMAPLFBValidateDSSUpdateMode(eMode))
	{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Unknown update mode (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, (int)eMode));
	}

	return OMAPLFBFromDSSUpdateMode(eMode);
#endif	
}

OMAPLFB_BOOL OMAPLFBSetUpdateMode(OMAPLFB_DEVINFO *psDevInfo, OMAPLFB_UPDATE_MODE eMode)
{
#if defined(PVR_OMAPLFB_DRM_FB)
	struct drm_connector *psConnector;
	enum omap_dss_update_mode eDSSMode;
	OMAPLFB_BOOL bSuccess = OMAPLFB_FALSE;
	OMAPLFB_BOOL bFailure = OMAPLFB_FALSE;

	if (!OMAPLFBValidateUpdateMode(eMode))
	{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Unknown update mode (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, (int)eMode));
			return OMAPLFB_FALSE;
	}
	eDSSMode = OMAPLFBToDSSUpdateMode(eMode);

	for (psConnector = NULL;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		int iRes = omap_connector_set_update_mode(psConnector, eDSSMode);
		OMAPLFB_BOOL bRes = (iRes == 0);


		bSuccess |= bRes;
		bFailure |= !bRes;
	}

	if (!bFailure)
	{
		if (!bSuccess)
		{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: No screens\n", __FUNCTION__, psDevInfo->uiFBDevID));
		}

		return OMAPLFB_TRUE;
	}

	if (!bSuccess)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't set %s for any screen\n", __FUNCTION__, psDevInfo->uiFBDevID, OMAPLFBUpdateModeToString(eMode)));
		return OMAPLFB_FALSE;
	}

	if (eMode == OMAPLFB_UPDATE_MODE_AUTO)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't set %s for all screens\n", __FUNCTION__, psDevInfo->uiFBDevID, OMAPLFBUpdateModeToString(eMode)));
		return OMAPLFB_FALSE;
	}

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: %s set for some screens\n", __FUNCTION__, psDevInfo->uiFBDevID, OMAPLFBUpdateModeToString(eMode)));

	return OMAPLFB_TRUE;
#else	
	struct omap_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	OMAP_DSS_DRIVER(psDSSDrv, psDSSDev);
	enum omap_dss_update_mode eDSSMode;
	int res;

	if (psDSSDrv == NULL || psDSSDrv->set_update_mode == NULL)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Can't set update mode\n", __FUNCTION__, psDevInfo->uiFBDevID));
		return OMAPLFB_FALSE;
	}

	if (!OMAPLFBValidateUpdateMode(eMode))
	{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Unknown update mode (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, (int)eMode));
			return OMAPLFB_FALSE;
	}
	eDSSMode = OMAPLFBToDSSUpdateMode(eMode);

	res = psDSSDrv->set_update_mode(psDSSDev, eDSSMode);
	if (res != 0)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: set_update_mode (%s) failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, OMAPLFBDSSUpdateModeToString(eDSSMode), res));
	}

	return (res == 0);
#endif	
}

OMAPLFB_BOOL OMAPLFBWaitForVSync(OMAPLFB_DEVINFO *psDevInfo)
{
#if defined(PVR_OMAPLFB_DRM_FB)
	struct drm_connector *psConnector;

	for (psConnector = NULL;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		(void) omap_encoder_wait_for_vsync(psConnector->encoder);
	}

	return OMAPLFB_TRUE;
#else	
	struct omap_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	OMAP_DSS_MANAGER(psDSSMan, psDSSDev);

	if (psDSSMan != NULL && WAIT_FOR_VSYNC(psDSSMan) != NULL)
	{
		int res = WAIT_FOR_VSYNC(psDSSMan)(psDSSMan);
		if (res != 0)
		{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Wait for vsync failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res));
			return OMAPLFB_FALSE;
		}
	}

	return OMAPLFB_TRUE;
#endif	
}

OMAPLFB_BOOL OMAPLFBManualSync(OMAPLFB_DEVINFO *psDevInfo)
{
#if defined(PVR_OMAPLFB_DRM_FB)
	struct drm_connector *psConnector;

	for (psConnector = NULL;
		(psConnector = omap_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL; )
	{
		
		if (omap_connector_sync(psConnector) != 0)
		{
			(void) omap_encoder_wait_for_vsync(psConnector->encoder);
		}
	}

	return OMAPLFB_TRUE;
#else	
	struct omap_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	OMAP_DSS_DRIVER(psDSSDrv, psDSSDev);

	if (psDSSDrv != NULL && psDSSDrv->sync != NULL)
	{
		int res = psDSSDrv->sync(psDSSDev);
		if (res != 0)
		{
			printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: Sync failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
			return OMAPLFB_FALSE;
		}
	}

	return OMAPLFB_TRUE;
#endif	
}

OMAPLFB_BOOL OMAPLFBCheckModeAndSync(OMAPLFB_DEVINFO *psDevInfo)
{
	OMAPLFB_UPDATE_MODE eMode = OMAPLFBGetUpdateMode(psDevInfo);

	switch(eMode)
	{
		case OMAPLFB_UPDATE_MODE_AUTO:
		case OMAPLFB_UPDATE_MODE_MANUAL:
			return OMAPLFBManualSync(psDevInfo);
		default:
			break;
	}

	return OMAPLFB_TRUE;
}

static int OMAPLFBFrameBufferEvents(struct notifier_block *psNotif,
                             unsigned long event, void *data)
{
	OMAPLFB_DEVINFO *psDevInfo;
	struct fb_event *psFBEvent = (struct fb_event *)data;
	struct fb_info *psFBInfo = psFBEvent->info;
	OMAPLFB_BOOL bBlanked;

	
	if (event != FB_EVENT_BLANK)
	{
		return 0;
	}

	bBlanked = (*(IMG_INT *)psFBEvent->data != 0) ? OMAPLFB_TRUE: OMAPLFB_FALSE;

	psDevInfo = OMAPLFBGetDevInfoPtr(psFBInfo->node);

#if 0
	if (psDevInfo != NULL)
	{
		if (bBlanked)
		{
			DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Blank event received\n", __FUNCTION__, psDevInfo->uiFBDevID));
		}
		else
		{
			DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unblank event received\n", __FUNCTION__, psDevInfo->uiFBDevID));
		}
	}
	else
	{
		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Blank/Unblank event for unknown framebuffer\n", __FUNCTION__, psFBInfo->node));
	}
#endif

	if (psDevInfo != NULL)
	{
		OMAPLFBAtomicBoolSet(&psDevInfo->sBlanked, bBlanked);
		OMAPLFBAtomicIntInc(&psDevInfo->sBlankEvents);
	}

	return 0;
}

OMAPLFB_ERROR OMAPLFBUnblankDisplay(OMAPLFB_DEVINFO *psDevInfo)
{
	int res;

	OMAPLFB_CONSOLE_LOCK();
	res = fb_blank(psDevInfo->psLINFBInfo, 0);
	OMAPLFB_CONSOLE_UNLOCK();
	if (res != 0 && res != -EINVAL)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_blank failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (OMAPLFB_ERROR_GENERIC);
	}

	return (OMAPLFB_OK);
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static void OMAPLFBBlankDisplay(OMAPLFB_DEVINFO *psDevInfo)
{
	OMAPLFB_CONSOLE_LOCK();
	fb_blank(psDevInfo->psLINFBInfo, 1);
	OMAPLFB_CONSOLE_UNLOCK();
}

static void OMAPLFBEarlySuspendHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		OMAPLFB_DEVINFO *psDevInfo = OMAPLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			OMAPLFBAtomicBoolSet(&psDevInfo->sEarlySuspendFlag, OMAPLFB_TRUE);
			OMAPLFBBlankDisplay(psDevInfo);
		}
	}
}

static void OMAPLFBEarlyResumeHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		OMAPLFB_DEVINFO *psDevInfo = OMAPLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			OMAPLFBUnblankDisplay(psDevInfo);
			OMAPLFBAtomicBoolSet(&psDevInfo->sEarlySuspendFlag, OMAPLFB_FALSE);
		}
	}
}

#endif 

OMAPLFB_ERROR OMAPLFBEnableLFBEventNotification(OMAPLFB_DEVINFO *psDevInfo)
{
	int                res;
	OMAPLFB_ERROR         eError;

	
	memset(&psDevInfo->sLINNotifBlock, 0, sizeof(psDevInfo->sLINNotifBlock));

	psDevInfo->sLINNotifBlock.notifier_call = OMAPLFBFrameBufferEvents;

	OMAPLFBAtomicBoolSet(&psDevInfo->sBlanked, OMAPLFB_FALSE);
	OMAPLFBAtomicIntSet(&psDevInfo->sBlankEvents, 0);

	res = fb_register_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_register_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);

		return (OMAPLFB_ERROR_GENERIC);
	}

	eError = OMAPLFBUnblankDisplay(psDevInfo);
	if (eError != OMAPLFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: UnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError);
		return eError;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	psDevInfo->sEarlySuspend.suspend = OMAPLFBEarlySuspendHandler;
	psDevInfo->sEarlySuspend.resume = OMAPLFBEarlyResumeHandler;
	psDevInfo->sEarlySuspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	register_early_suspend(&psDevInfo->sEarlySuspend);
#endif

	return (OMAPLFB_OK);
}

OMAPLFB_ERROR OMAPLFBDisableLFBEventNotification(OMAPLFB_DEVINFO *psDevInfo)
{
	int res;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&psDevInfo->sEarlySuspend);
#endif

	
	res = fb_unregister_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_unregister_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (OMAPLFB_ERROR_GENERIC);
	}

	OMAPLFBAtomicBoolSet(&psDevInfo->sBlanked, OMAPLFB_FALSE);

	return (OMAPLFB_OK);
}

#if defined(SUPPORT_DRI_DRM) && defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
static OMAPLFB_DEVINFO *OMAPLFBPVRDevIDToDevInfo(unsigned uiPVRDevID)
{
	unsigned uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		OMAPLFB_DEVINFO *psDevInfo = OMAPLFBGetDevInfoPtr(i);

		if (psDevInfo->uiPVRDevID == uiPVRDevID)
		{
			return psDevInfo;
		}
	}

	printk(KERN_ERR DRIVER_PREFIX
		": %s: PVR Device %u: Couldn't find device\n", __FUNCTION__, uiPVRDevID);

	return NULL;
}

int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Ioctl)(struct drm_device unref__ *dev, void *arg, struct drm_file unref__ *pFile)
{
	uint32_t *puiArgs;
	uint32_t uiCmd;
	unsigned uiPVRDevID;
	int ret = 0;
	OMAPLFB_DEVINFO *psDevInfo;

	if (arg == NULL)
	{
		return -EFAULT;
	}

	puiArgs = (uint32_t *)arg;
	uiCmd = puiArgs[PVR_DRM_DISP_ARG_CMD];
	uiPVRDevID = puiArgs[PVR_DRM_DISP_ARG_DEV];

	psDevInfo = OMAPLFBPVRDevIDToDevInfo(uiPVRDevID);
	if (psDevInfo == NULL)
	{
		return -EINVAL;
	}


	switch (uiCmd)
	{
		case PVR_DRM_DISP_CMD_LEAVE_VT:
		case PVR_DRM_DISP_CMD_ENTER_VT:
		{
			OMAPLFB_BOOL bLeaveVT = (uiCmd == PVR_DRM_DISP_CMD_LEAVE_VT);
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: PVR Device %u: %s\n",
				__FUNCTION__, uiPVRDevID,
				bLeaveVT ? "Leave VT" : "Enter VT"));

			OMAPLFBCreateSwapChainLock(psDevInfo);
			
			OMAPLFBAtomicBoolSet(&psDevInfo->sLeaveVT, bLeaveVT);
			if (psDevInfo->psSwapChain != NULL)
			{
				flush_workqueue(psDevInfo->psSwapChain->psWorkQueue);

				if (bLeaveVT)
				{
					OMAPLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
					(void) OMAPLFBCheckModeAndSync(psDevInfo);
				}
			}

			OMAPLFBCreateSwapChainUnLock(psDevInfo);
			(void) OMAPLFBUnblankDisplay(psDevInfo);
			break;
		}
		case PVR_DRM_DISP_CMD_ON:
		case PVR_DRM_DISP_CMD_STANDBY:
		case PVR_DRM_DISP_CMD_SUSPEND:
		case PVR_DRM_DISP_CMD_OFF:
		{
			int iFBMode;
#if defined(DEBUG)
			{
				const char *pszMode;
				switch(uiCmd)
				{
					case PVR_DRM_DISP_CMD_ON:
						pszMode = "On";
						break;
					case PVR_DRM_DISP_CMD_STANDBY:
						pszMode = "Standby";
						break;
					case PVR_DRM_DISP_CMD_SUSPEND:
						pszMode = "Suspend";
						break;
					case PVR_DRM_DISP_CMD_OFF:
						pszMode = "Off";
						break;
					default:
						pszMode = "(Unknown Mode)";
						break;
				}
				printk(KERN_WARNING DRIVER_PREFIX ": %s: PVR Device %u: Display %s\n",
				__FUNCTION__, uiPVRDevID, pszMode);
			}
#endif
			switch(uiCmd)
			{
				case PVR_DRM_DISP_CMD_ON:
					iFBMode = FB_BLANK_UNBLANK;
					break;
				case PVR_DRM_DISP_CMD_STANDBY:
					iFBMode = FB_BLANK_HSYNC_SUSPEND;
					break;
				case PVR_DRM_DISP_CMD_SUSPEND:
					iFBMode = FB_BLANK_VSYNC_SUSPEND;
					break;
				case PVR_DRM_DISP_CMD_OFF:
					iFBMode = FB_BLANK_POWERDOWN;
					break;
				default:
					return -EINVAL;
			}

			OMAPLFBCreateSwapChainLock(psDevInfo);

			if (psDevInfo->psSwapChain != NULL)
			{
				flush_workqueue(psDevInfo->psSwapChain->psWorkQueue);
			}

			OMAPLFB_CONSOLE_LOCK();
			ret = fb_blank(psDevInfo->psLINFBInfo, iFBMode);
			OMAPLFB_CONSOLE_UNLOCK();

			OMAPLFBCreateSwapChainUnLock(psDevInfo);

			break;
		}
		default:
		{
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}
#endif

#if defined(SUPPORT_DRI_DRM)
int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Init)(struct drm_device unref__ *dev)
#else
static int __init OMAPLFB_Init(void)
#endif
{

	if(OMAPLFBInit() != OMAPLFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: OMAPLFBInit failed\n", __FUNCTION__);
		return -ENODEV;
	}

	return 0;

}

#if defined(SUPPORT_DRI_DRM)
void PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(struct drm_device unref__ *dev)
#else
static void __exit OMAPLFB_Cleanup(void)
#endif
{    
	if(OMAPLFBDeInit() != OMAPLFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: OMAPLFBDeInit failed\n", __FUNCTION__);
	}
}

#if !defined(SUPPORT_DRI_DRM)
late_initcall(OMAPLFB_Init);
module_exit(OMAPLFB_Cleanup);
#endif
