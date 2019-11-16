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

#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/fb.h>
#include <linux/console.h>
//#include <linux/omapfb.h>
#include <linux/mutex.h>
//#include <video/xbdss.h>

#include "img_defs.h"

#if defined(SUPPORT_DRI_DRM)
#include <drm/drmP.h>
#include "pvr_drm.h"
#include "3rdparty_dc_drm_shared.h"
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#define PVR_OMAPFB3_NEEDS_PLAT_VRFB_H

#if defined(PVR_OMAPFB3_NEEDS_PLAT_VRFB_H)
//#include <plat/vrfb.h>
#else
#if defined(PVR_OMAPFB3_NEEDS_MACH_VRFB_H)
//#include <mach/vrfb.h>
#endif
#endif

#if defined(DEBUG)
#define	PVR_DEBUG DEBUG
#undef DEBUG
#endif
//#include <xbfb.h>
#if defined(DEBUG)
#undef DEBUG
#endif
#if defined(PVR_DEBUG)
#define	DEBUG PVR_DEBUG
#undef PVR_DEBUG
#endif
#endif	


#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "xblfb.h"
#include "pvrmodule.h"

#undef CONFIG_HAS_EARLYSUSPEND

#if !defined(PVR_LINUX_USING_WORKQUEUES)
#error "PVR_LINUX_USING_WORKQUEUES must be defined"
#endif

MODULE_SUPPORTED_DEVICE(DEVNAME);

#if defined(SUPPORT_DRI_DRM)
int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Suspend)(struct drm_device unref__ *dev)
{
	return 0;
}

int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Resume)(struct drm_device unref__ *dev)
{
	return 0;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#define XB_DISPLAY_DRIVER(drv, dev) struct xb_display_driver *drv = (dev) != NULL ? (dev)->driver : NULL
#define XB_DISPLAY_MANAGER(man, dev) struct xb_overlay_manager *man = (dev) != NULL ? (dev)->manager : NULL
#define	WAIT_FOR_VSYNC(man)	((man)->wait_for_vsync)
#endif	

void *XBLFBAllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void XBLFBFreeKernelMem(void *pvMem)
{

	kfree(pvMem);
}

void XBLFBCreateSwapChainLockInit(XBLFB_DEVINFO *psDevInfo)
{
	mutex_init(&psDevInfo->sCreateSwapChainMutex);
}

void XBLFBCreateSwapChainLockDeInit(XBLFB_DEVINFO *psDevInfo)
{
	mutex_destroy(&psDevInfo->sCreateSwapChainMutex);
}

void XBLFBCreateSwapChainLock(XBLFB_DEVINFO *psDevInfo)
{
	mutex_lock(&psDevInfo->sCreateSwapChainMutex);
}

void XBLFBCreateSwapChainUnLock(XBLFB_DEVINFO *psDevInfo)
{
	mutex_unlock(&psDevInfo->sCreateSwapChainMutex);
}

void XBLFBAtomicBoolInit(XBLFB_ATOMIC_BOOL *psAtomic, XBLFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

void XBLFBAtomicBoolDeInit(XBLFB_ATOMIC_BOOL *psAtomic)
{
}

void XBLFBAtomicBoolSet(XBLFB_ATOMIC_BOOL *psAtomic, XBLFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

XBLFB_BOOL XBLFBAtomicBoolRead(XBLFB_ATOMIC_BOOL *psAtomic)
{
	return (XBLFB_BOOL)atomic_read(psAtomic);
}

void XBLFBAtomicIntInit(XBLFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

void XBLFBAtomicIntDeInit(XBLFB_ATOMIC_INT *psAtomic)
{
}

void XBLFBAtomicIntSet(XBLFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

int XBLFBAtomicIntRead(XBLFB_ATOMIC_INT *psAtomic)
{
	return atomic_read(psAtomic);
}

void XBLFBAtomicIntInc(XBLFB_ATOMIC_INT *psAtomic)
{
	atomic_inc(psAtomic);
}

XBLFB_ERROR XBLFBGetLibFuncAddr (char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
	{
		return (XBLFB_ERROR_INVALID_PARAMS);
	}

	/* Nothing to do - should be exported from pvrsrv.ko */
	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return (XBLFB_OK);
}

/* Insert a swap buffer into the swap chain work queue */
void XBLFBQueueBufferForSwap(XBLFB_SWAPCHAIN *psSwapChain, XBLFB_BUFFER *psBuffer)
{
	int res = queue_work(psSwapChain->psWorkQueue, &psBuffer->sWork);

	if (res == 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Buffer already on work queue\n", __FUNCTION__, psSwapChain->uiFBDevID);
	}
}

/* Insert a swap buffer into the swap chain work queue */
void XBLFBQueueBufferForSwap2(XBLFB_SWAPCHAIN *psSwapChain, IMG_CPU_VIRTADDR pViAddress, XBLFB_HANDLE hCmdCookie)
{
	int res = 0;
	int i = 0;
	XBLFB_BUFFER *psBuffer = IMG_NULL;
	XBLFB_BUFFER *psBufferI = psSwapChain->psBuffer;

	for (i = 0; i < SWAP_CHAIN_LENGTH; i++)
	{
		//printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: psBuffer search [i%d] [%p] pViAddress[%p].\n", __FUNCTION__, psSwapChain->uiFBDevID, i, psBufferI->sCPUVAddr, pViAddress);
		if (pViAddress == psBufferI->sCPUVAddr)
		{
			psBuffer = psBufferI;
			break;
		}
		psBufferI = psBufferI->psNext;
	}
	if (psBuffer == IMG_NULL) {
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Cannot find the XBLFB_BUFFER for the virtual address.\n", __FUNCTION__, psSwapChain->uiFBDevID);
		psSwapChain->psBuffer->psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)hCmdCookie, IMG_TRUE);
		return;
	}

	psBuffer->hCmdComplete = hCmdCookie;

	res = queue_work(psSwapChain->psWorkQueue, &psBuffer->sWork);
	//printk(KERN_WARNING DRIVER_PREFIX ": %s: Work [%p] queued.\n", __FUNCTION__, pViAddress);

	if (res == 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Buffer already on work queue\n", __FUNCTION__, psSwapChain->uiFBDevID);
	}
}

/* Process an item on a swap chain work queue */
static void WorkQueueHandler(struct work_struct *psWork)
{
	XBLFB_BUFFER *psBuffer = container_of(psWork, XBLFB_BUFFER, sWork);

	//printk(KERN_WARNING DRIVER_PREFIX ": %s: Work [%p] received.\n", __FUNCTION__, psBuffer->sCPUVAddr);

	XBLFBSwapHandler(psBuffer);
}

/* Create a swap chain work queue */
XBLFB_ERROR XBLFBCreateSwapQueue(XBLFB_SWAPCHAIN *psSwapChain)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
	/*
	 * Calling alloc_ordered_workqueue with the WQ_FREEZABLE and
	 * WQ_MEM_RECLAIM flags set, (currently) has the same effect as
	 * calling create_freezable_workqueue. None of the other WQ
	 * flags are valid. Setting WQ_MEM_RECLAIM should allow the
	 * workqueue to continue to service the swap chain in low memory
	 * conditions, preventing the driver from holding on to
	 * resources longer than it needs to.
	 */
	psSwapChain->psWorkQueue = alloc_ordered_workqueue(DEVNAME, WQ_FREEZABLE | WQ_MEM_RECLAIM);
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	psSwapChain->psWorkQueue = create_freezable_workqueue(DEVNAME);
#else
	/*
	 * Create a single-threaded, freezable, rt-prio workqueue.
	 * Such workqueues are frozen with user threads when a system
	 * suspends, before driver suspend entry points are called.
	 * This ensures this driver will not call into the Linux
	 * framebuffer driver after the latter is suspended.
	 */
	psSwapChain->psWorkQueue = __create_workqueue(DEVNAME, 1, 1, 1);
#endif
#endif
	if (psSwapChain->psWorkQueue == NULL)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: Couldn't create workqueue\n", __FUNCTION__, psSwapChain->uiFBDevID);

		return (XBLFB_ERROR_INIT_FAILURE);
	}

	return (XBLFB_OK);
}

struct work_struct sWork;

/* Prepare buffer for insertion into a swap chain work queue */
void XBLFBInitBufferForSwap(XBLFB_BUFFER *psBuffer)
{
	INIT_WORK(&psBuffer->sWork, WorkQueueHandler);
}

/* Destroy a swap chain work queue */
void XBLFBDestroySwapQueue(XBLFB_SWAPCHAIN *psSwapChain)
{
	destroy_workqueue(psSwapChain->psWorkQueue);
}

/* Flip display to given buffer */
void XBLFBFlip(XBLFB_DEVINFO *psDevInfo, XBLFB_BUFFER *psBuffer)
{
	struct fb_var_screeninfo sFBVar;
	int res;
	unsigned long ulYResVirtual;

	XBLFB_CONSOLE_LOCK();

	sFBVar = psDevInfo->psLINFBInfo->var;

	sFBVar.xoffset = 0;
	sFBVar.yoffset = psBuffer->ulYOffset;

	ulYResVirtual = psBuffer->ulYOffset + sFBVar.yres;

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

	XBLFB_CONSOLE_UNLOCK();
}

/*
 * If the screen is manual or auto update mode, wait for the screen to
 * update.
 */
/* XBLFB_BOOL XBLFBCheckModeAndSync(XBLFB_DEVINFO *psDevInfo) */
/* { */
/* 	XBLFB_UPDATE_MODE eMode = XBLFBGetUpdateMode(psDevInfo); */

/* 	switch(eMode) */
/* 	{ */
/* 		case XBLFB_UPDATE_MODE_AUTO: */
/* 		case XBLFB_UPDATE_MODE_MANUAL: */
/* 			return XBLFBManualSync(psDevInfo); */
/* 		default: */
/* 			break; */
/* 	} */

/* 	return XBLFB_TRUE; */
/* } */

/* Linux Framebuffer event notification handler */
static int XBLFBFrameBufferEvents(struct notifier_block *psNotif,
								  unsigned long event, void *data)
{
	XBLFB_DEVINFO *psDevInfo;
	struct fb_event *psFBEvent = (struct fb_event *)data;
	struct fb_info *psFBInfo = psFBEvent->info;
	XBLFB_BOOL bBlanked;

	/* Only interested in blanking events */
	if (event != FB_EVENT_BLANK)
	{
		return 0;
	}

	bBlanked = (*(IMG_INT *)psFBEvent->data != 0) ? XBLFB_TRUE: XBLFB_FALSE;

	psDevInfo = XBLFBGetDevInfoPtr(psFBInfo->node);

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
		XBLFBAtomicBoolSet(&psDevInfo->sBlanked, bBlanked);
		XBLFBAtomicIntInc(&psDevInfo->sBlankEvents);
	}

	return 0;
}

/* Unblank the screen */
XBLFB_ERROR XBLFBUnblankDisplay(XBLFB_DEVINFO *psDevInfo)
{
	int res;

	XBLFB_CONSOLE_LOCK();
	res = fb_blank(psDevInfo->psLINFBInfo, 0);
	XBLFB_CONSOLE_UNLOCK();
	if (res != 0 && res != -EINVAL)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_blank failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (XBLFB_ERROR_GENERIC);
	}

	return (XBLFB_OK);
}

#ifdef CONFIG_HAS_EARLYSUSPEND

/* Blank the screen */
static void XBLFBBlankDisplay(XBLFB_DEVINFO *psDevInfo)
{
	XBLFB_CONSOLE_LOCK();
	fb_blank(psDevInfo->psLINFBInfo, 1);
	XBLFB_CONSOLE_UNLOCK();
}

static void XBLFBEarlySuspendHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = XBLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		XBLFB_DEVINFO *psDevInfo = XBLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			XBLFBAtomicBoolSet(&psDevInfo->sEarlySuspendFlag, XBLFB_TRUE);
			XBLFBBlankDisplay(psDevInfo);
		}
	}
}

static void XBLFBEarlyResumeHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = XBLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		XBLFB_DEVINFO *psDevInfo = XBLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			XBLFBUnblankDisplay(psDevInfo);
			XBLFBAtomicBoolSet(&psDevInfo->sEarlySuspendFlag, XBLFB_FALSE);
		}
	}
}

#endif 

/* Set up Linux Framebuffer event notification */
XBLFB_ERROR XBLFBEnableLFBEventNotification(XBLFB_DEVINFO *psDevInfo)
{
	int                res;
	XBLFB_ERROR        eError;

	/* Set up Linux Framebuffer event notification */
	memset(&psDevInfo->sLINNotifBlock, 0, sizeof(psDevInfo->sLINNotifBlock));

	psDevInfo->sLINNotifBlock.notifier_call = XBLFBFrameBufferEvents;

	XBLFBAtomicBoolSet(&psDevInfo->sBlanked, XBLFB_FALSE);
	XBLFBAtomicIntSet(&psDevInfo->sBlankEvents, 0);

	res = fb_register_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_register_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);

		return (XBLFB_ERROR_GENERIC);
	}

	eError = XBLFBUnblankDisplay(psDevInfo);
	if (eError != XBLFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: UnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError);
		return eError;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	psDevInfo->sEarlySuspend.suspend = XBLFBEarlySuspendHandler;
	psDevInfo->sEarlySuspend.resume = XBLFBEarlyResumeHandler;
	psDevInfo->sEarlySuspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	register_early_suspend(&psDevInfo->sEarlySuspend);
#endif

	return (XBLFB_OK);
}

/* Disable Linux Framebuffer event notification */
XBLFB_ERROR XBLFBDisableLFBEventNotification(XBLFB_DEVINFO *psDevInfo)
{
	int res;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&psDevInfo->sEarlySuspend);
#endif

	/* Unregister for Framebuffer events */
	res = fb_unregister_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_unregister_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (XBLFB_ERROR_GENERIC);
	}

	XBLFBAtomicBoolSet(&psDevInfo->sBlanked, XBLFB_FALSE);

	return (XBLFB_OK);
}

#if defined(SUPPORT_DRI_DRM)
int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Init)(struct drm_device unref__ *dev)
#else
static int __init XBLFB_Init(void)
#endif
{
	if(XBLFBInit() != XBLFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: XBLFBInit failed\n", __FUNCTION__);
		return -ENODEV;
	}

	return 0;

}

#if defined(SUPPORT_DRI_DRM)
void PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(struct drm_device unref__ *dev)
#else
static void __exit XBLFB_Cleanup(void)
#endif
{
	if(XBLFBDeInit() != XBLFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: XBLFBDeInit failed\n", __FUNCTION__);
	}
}

/*
 These macro calls define the initialisation and removal functions of the
 driver.  Although they are prefixed `module_', they apply when compiling
 statically as well; in both cases they define the function the kernel will
 run to start/stop the driver.
*/
#if !defined(SUPPORT_DRI_DRM)
late_initcall(XBLFB_Init);
module_exit(XBLFB_Cleanup);
#endif
