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

#ifndef __XBLFB_H__
#define __XBLFB_H__

#include <linux/version.h>

#include <asm/atomic.h>

#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/mutex.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38))
#define	XBLFB_CONSOLE_LOCK()		console_lock()
#define	XBLFB_CONSOLE_UNLOCK()	console_unlock()
#else
#define	XBLFB_CONSOLE_LOCK()		acquire_console_sem()
#define	XBLFB_CONSOLE_UNLOCK()	release_console_sem()
#endif

#define unref__ __attribute__ ((unused))

typedef void *       XBLFB_HANDLE;

typedef bool XBLFB_BOOL, *XBLFB_PBOOL;
#define	XBLFB_FALSE false
#define XBLFB_TRUE true

#define SWAP_CHAIN_LENGTH 3

typedef	atomic_t	XBLFB_ATOMIC_BOOL;

typedef atomic_t	XBLFB_ATOMIC_INT;

typedef struct XBLFB_BUFFER_TAG
{
	struct XBLFB_BUFFER_TAG	*psNext;
	struct XBLFB_DEVINFO_TAG	*psDevInfo;

	struct work_struct sWork;

	
	unsigned long		     	ulYOffset;

	
	
	IMG_SYS_PHYADDR              	sSysAddr;
	IMG_CPU_VIRTADDR             	sCPUVAddr;
	PVRSRV_SYNC_DATA            	*psSyncData;

	XBLFB_HANDLE      		hCmdComplete;
	unsigned long    		ulSwapInterval;
} XBLFB_BUFFER;

typedef struct XBLFB_SWAPCHAIN_TAG
{
	
	unsigned int			uiSwapChainID;

	
	unsigned long       		ulBufferCount;

	
	XBLFB_BUFFER     		*psBuffer;

	
	struct workqueue_struct   	*psWorkQueue;

	
	XBLFB_BOOL			bNotVSynced;

	
	int				iBlankEvents;

	
	unsigned int            	uiFBDevID;
} XBLFB_SWAPCHAIN;

typedef struct XBLFB_FBINFO_TAG
{
	unsigned long       ulFBSize;
	unsigned long       ulBufferSize;
	unsigned long       ulRoundedBufferSize;
	unsigned long       ulWidth;
	unsigned long       ulHeight;
	unsigned long       ulByteStride;
	unsigned long       ulPhysicalWidthmm;
	unsigned long       ulPhysicalHeightmm;

	
	
	IMG_SYS_PHYADDR     sSysAddr;
	IMG_CPU_VIRTADDR    sCPUVAddr;

	
	PVRSRV_PIXEL_FORMAT ePixelFormat;

} XBLFB_FBINFO;

typedef struct XBLFB_DEVINFO_TAG
{
	
	unsigned int            uiFBDevID;

	
	unsigned int            uiPVRDevID;

	
	struct mutex		sCreateSwapChainMutex;

	
	XBLFB_BUFFER          sSystemBuffer;

	
	PVRSRV_DC_DISP2SRV_KMJTABLE	sPVRJTable;
	
	
	PVRSRV_DC_SRV2DISP_KMJTABLE	sDCJTable;

	
	XBLFB_FBINFO          sFBInfo;

	
	XBLFB_SWAPCHAIN      *psSwapChain;

	
	unsigned int		uiSwapChainID;

	
	XBLFB_ATOMIC_BOOL     sFlushCommands;

	
	struct fb_info         *psLINFBInfo;

	
	struct notifier_block   sLINNotifBlock;

	
	

	
	IMG_DEV_VIRTADDR	sDisplayDevVAddr;

	DISPLAY_INFO            sDisplayInfo;

	
	DISPLAY_FORMAT          sDisplayFormat;
	
	
	DISPLAY_DIMS            sDisplayDim;

	
	XBLFB_ATOMIC_BOOL	sBlanked;

	
	XBLFB_ATOMIC_INT	sBlankEvents;

#ifdef CONFIG_HAS_EARLYSUSPEND
	
	XBLFB_ATOMIC_BOOL	sEarlySuspendFlag;

	struct early_suspend    sEarlySuspend;
#endif
/* #if defined(SUPPORT_DRI_DRM) */
/* 	XBLFB_ATOMIC_BOOL     sLeaveVT; */
/* #endif */

}  XBLFB_DEVINFO;

#define	XBLFB_PAGE_SIZE 4096

#ifdef	DEBUG
#define	DEBUG_PRINTK(x) printk x
#else
#define	DEBUG_PRINTK(x)
#endif

#define DISPLAY_DEVICE_NAME "PowerVR Xburst Linux Display Driver"
#define	DRVNAME	"xblfb"
#define	DEVNAME	DRVNAME
#define	DRIVER_PREFIX DRVNAME

typedef enum _XBLFB_ERROR_
{
	XBLFB_OK                             =  0,
	XBLFB_ERROR_GENERIC                  =  1,
	XBLFB_ERROR_OUT_OF_MEMORY            =  2,
	XBLFB_ERROR_TOO_FEW_BUFFERS          =  3,
	XBLFB_ERROR_INVALID_PARAMS           =  4,
	XBLFB_ERROR_INIT_FAILURE             =  5,
	XBLFB_ERROR_CANT_REGISTER_CALLBACK   =  6,
	XBLFB_ERROR_INVALID_DEVICE           =  7,
	XBLFB_ERROR_DEVICE_REGISTER_FAILED   =  8,
	XBLFB_ERROR_SET_UPDATE_MODE_FAILED   =  9
} XBLFB_ERROR;

typedef enum _XBLFB_UPDATE_MODE_
{
	XBLFB_UPDATE_MODE_UNDEFINED			= 0,
	XBLFB_UPDATE_MODE_MANUAL			= 1,
	XBLFB_UPDATE_MODE_AUTO			        = 2,
	XBLFB_UPDATE_MODE_DISABLED			= 3
} XBLFB_UPDATE_MODE;

#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

XBLFB_ERROR XBLFBInit(void);
XBLFB_ERROR XBLFBDeInit(void);

XBLFB_DEVINFO *XBLFBGetDevInfoPtr(unsigned uiFBDevID);
unsigned XBLFBMaxFBDevIDPlusOne(void);
void *XBLFBAllocKernelMem(unsigned long ulSize);
void XBLFBFreeKernelMem(void *pvMem);
XBLFB_ERROR XBLFBGetLibFuncAddr(char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable);
XBLFB_ERROR XBLFBCreateSwapQueue (XBLFB_SWAPCHAIN *psSwapChain);
void XBLFBDestroySwapQueue(XBLFB_SWAPCHAIN *psSwapChain);
void XBLFBInitBufferForSwap(XBLFB_BUFFER *psBuffer);
void XBLFBSwapHandler(XBLFB_BUFFER *psBuffer);
void XBLFBQueueBufferForSwap(XBLFB_SWAPCHAIN *psSwapChain,XBLFB_BUFFER *psBuffer);
void XBLFBQueueBufferForSwap2(XBLFB_SWAPCHAIN *psSwapChain, IMG_CPU_VIRTADDR pViAddress, XBLFB_HANDLE hCmdCookie);
void XBLFBFlip(XBLFB_DEVINFO *psDevInfo, XBLFB_BUFFER *psBuffer);
XBLFB_UPDATE_MODE XBLFBGetUpdateMode(XBLFB_DEVINFO *psDevInfo);
XBLFB_BOOL XBLFBSetUpdateMode(XBLFB_DEVINFO *psDevInfo, XBLFB_UPDATE_MODE eMode);
XBLFB_BOOL XBLFBWaitForVSync(XBLFB_DEVINFO *psDevInfo);
XBLFB_BOOL XBLFBManualSync(XBLFB_DEVINFO *psDevInfo);
XBLFB_BOOL XBLFBCheckModeAndSync(XBLFB_DEVINFO *psDevInfo);
XBLFB_ERROR XBLFBUnblankDisplay(XBLFB_DEVINFO *psDevInfo);
XBLFB_ERROR XBLFBEnableLFBEventNotification(XBLFB_DEVINFO *psDevInfo);
XBLFB_ERROR XBLFBDisableLFBEventNotification(XBLFB_DEVINFO *psDevInfo);
void XBLFBCreateSwapChainLockInit(XBLFB_DEVINFO *psDevInfo);
void XBLFBCreateSwapChainLockDeInit(XBLFB_DEVINFO *psDevInfo);
void XBLFBCreateSwapChainLock(XBLFB_DEVINFO *psDevInfo);
void XBLFBCreateSwapChainUnLock(XBLFB_DEVINFO *psDevInfo);
void XBLFBAtomicBoolInit(XBLFB_ATOMIC_BOOL *psAtomic, XBLFB_BOOL bVal);
void XBLFBAtomicBoolDeInit(XBLFB_ATOMIC_BOOL *psAtomic);
void XBLFBAtomicBoolSet(XBLFB_ATOMIC_BOOL *psAtomic, XBLFB_BOOL bVal);
XBLFB_BOOL XBLFBAtomicBoolRead(XBLFB_ATOMIC_BOOL *psAtomic);
void XBLFBAtomicIntInit(XBLFB_ATOMIC_INT *psAtomic, int iVal);
void XBLFBAtomicIntDeInit(XBLFB_ATOMIC_INT *psAtomic);
void XBLFBAtomicIntSet(XBLFB_ATOMIC_INT *psAtomic, int iVal);
int XBLFBAtomicIntRead(XBLFB_ATOMIC_INT *psAtomic);
void XBLFBAtomicIntInc(XBLFB_ATOMIC_INT *psAtomic);

#if defined(DEBUG)
void XBLFBPrintInfo(XBLFB_DEVINFO *psDevInfo);
#else
#define	XBLFBPrintInfo(psDevInfo)
#endif

#endif 

