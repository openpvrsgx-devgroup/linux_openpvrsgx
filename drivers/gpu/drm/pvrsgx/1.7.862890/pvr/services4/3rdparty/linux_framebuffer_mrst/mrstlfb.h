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

#ifndef __MRSTLFB_H__
#define __MRSTLFB_H__

#include <drm/drmP.h>
#include "psb_intel_reg.h"

#define MRST_USING_INTERRUPTS

#define PSB_HWSTAM                0x2098
#define PSB_INSTPM                0x20C0
#define PSB_INT_IDENTITY_R        0x20A4
#define _PSB_VSYNC_PIPEB_FLAG     (1<<5)
#define _PSB_VSYNC_PIPEA_FLAG     (1<<7)
#define _PSB_IRQ_SGX_FLAG         (1<<18)
#define _PSB_IRQ_MSVDX_FLAG       (1<<19)
#define PSB_INT_MASK_R            0x20A8
#define PSB_INT_ENABLE_R          0x20A0

#define MAX_SWAPCHAINS			  1
#define MAX_FLIPBUFFERS			  9

/* IPC message and command defines used to enable/disable mipi panel voltages */
#define	IPC_MSG_PANEL_ON_OFF	0xE9
#define IPC_CMD_PANEL_ON	1
#define IPC_CMD_PANEL_OFF	0

typedef void *   MRST_HANDLE;

typedef enum tag_mrst_bool
{
	MRST_FALSE = 0,
	MRST_TRUE  = 1,
} MRST_BOOL, *MRST_PBOOL;

typedef int(* MRSTLFB_VSYNC_ISR_PFN)(struct drm_device* psDrmDevice, int iPipe);


typedef struct MRSTLFB_BUFFER_TAG
{
	
    IMG_UINT32		             	ui32BufferSize;
	union {
		
		IMG_SYS_PHYADDR             *psNonCont;
		
		IMG_SYS_PHYADDR				sCont;
	} uSysAddr;
	
	IMG_DEV_VIRTADDR             	sDevVAddr;
	
    IMG_CPU_VIRTADDR             	sCPUVAddr;    
	
	PVRSRV_SYNC_DATA             	*psSyncData;
	
	MRST_BOOL					 	bIsContiguous;
	
	MRST_BOOL					 	bIsAllocated;

	IMG_UINT32						ui32OwnerTaskID;

	MRST_BOOL						bFromPages;
	
	struct page			*buffer_pages[0];	
} MRSTLFB_BUFFER;

typedef struct MRSTLFB_VSYNC_FLIP_ITEM_TAG
{



	MRST_HANDLE      hCmdComplete;

	unsigned long    ulSwapInterval;

	MRST_BOOL        bValid;

	MRST_BOOL        bFlipped;

	MRST_BOOL        bCmdCompleted;





	MRSTLFB_BUFFER*	psBuffer;
} MRSTLFB_VSYNC_FLIP_ITEM;

typedef struct MRSTLFB_SWAPCHAIN_TAG
{
	
	unsigned long       ulBufferCount;

	IMG_UINT32			ui32SwapChainID;

	
	MRSTLFB_BUFFER     **ppsBuffer;

	
	unsigned long	    ulSwapChainLength;

	
	MRSTLFB_VSYNC_FLIP_ITEM	*psVSyncFlips;

	
	unsigned long       ulInsertIndex;
	
	
	unsigned long       ulRemoveIndex;

	
	PVRSRV_DC_DISP2SRV_KMJTABLE	*psPVRJTable;

	
	struct drm_driver         *psDrmDriver;

	
	struct drm_device         *psDrmDev;

	struct MRSTLFB_SWAPCHAIN_TAG *psNext;

	struct MRSTLFB_DEVINFO_TAG *psDevInfo;
	unsigned long 		ui32Height;
	unsigned long		ui32Width;
	unsigned long		ui32ByteStride;

} MRSTLFB_SWAPCHAIN;

typedef struct MRSTLFB_FBINFO_TAG
{
	unsigned long       ulFBSize;
	unsigned long       ulBufferSize;
	unsigned long       ulRoundedBufferSize;
	unsigned long       ulWidth;
	unsigned long       ulHeight;
	unsigned long       ulByteStride;



	IMG_SYS_PHYADDR     sSysAddr;
	IMG_CPU_VIRTADDR    sCPUVAddr;
        IMG_DEV_VIRTADDR    sDevVAddr;


	PVRSRV_PIXEL_FORMAT ePixelFormat;
}MRSTLFB_FBINFO;

/**
 * If DRI is enable then extemding drm_device
 */
typedef struct MRSTLFB_DEVINFO_TAG
{
	unsigned int           uiDeviceID;

	struct drm_device 	*psDrmDevice;

	

	MRSTLFB_BUFFER          sSystemBuffer;

	
	PVRSRV_DC_DISP2SRV_KMJTABLE	sPVRJTable;
	
	
	PVRSRV_DC_SRV2DISP_KMJTABLE	sDCJTable;

	
	unsigned long           ulRefCount;

	MRSTLFB_SWAPCHAIN      *psCurrentSwapChain;

	MRSTLFB_SWAPCHAIN      *apsSwapChains[MAX_SWAPCHAINS];

	IMG_UINT32	   	ui32SwapChainNum;

	
	void *pvRegs;

	
	unsigned long ulSetFlushStateRefCount;

	
	MRST_BOOL           bFlushCommands;

	
	MRST_BOOL           bBlanked;

	
	struct fb_info         *psLINFBInfo;

	
	struct notifier_block   sLINNotifBlock;

	
	spinlock_t             sSwapChainLock;

	

	
	IMG_DEV_VIRTADDR	sDisplayDevVAddr;

	DISPLAY_INFO            sDisplayInfo;

	
	DISPLAY_FORMAT          sDisplayFormat;
	
	
	DISPLAY_DIMS            sDisplayDim;

	IMG_UINT32		ui32MainPipe;

	
	MRST_BOOL bSuspended;

	
	MRST_BOOL bLeaveVT;

	
	unsigned long ulLastFlipAddr;

	
	MRST_BOOL bLastFlipAddrValid;
}  MRSTLFB_DEVINFO;

#if 0
#define	MRSTLFB_PAGE_SIZE 4096
#define	MRSTLFB_PAGE_MASK (MRSTLFB_PAGE_SIZE - 1)
#define	MRSTLFB_PAGE_TRUNC (~MRSTLFB_PAGE_MASK)

#define	MRSTLFB_PAGE_ROUNDUP(x) (((x) + MRSTLFB_PAGE_MASK) & MRSTLFB_PAGE_TRUNC)
#endif

#ifdef	DEBUG
#define	DEBUG_PRINTK(x) printk x
#else
#define	DEBUG_PRINTK(x)
#endif

#define DISPLAY_DEVICE_NAME "PowerVR Cedartrail Linux Display Driver"
#define	DRVNAME	"cdvlfb"
#define	DEVNAME	DRVNAME
#define	DRIVER_PREFIX DRVNAME

typedef enum _MRST_ERROR_
{
	MRST_OK                             =  0,
	MRST_ERROR_GENERIC                  =  1,
	MRST_ERROR_OUT_OF_MEMORY            =  2,
	MRST_ERROR_TOO_FEW_BUFFERS          =  3,
	MRST_ERROR_INVALID_PARAMS           =  4,
	MRST_ERROR_INIT_FAILURE             =  5,
	MRST_ERROR_CANT_REGISTER_CALLBACK   =  6,
	MRST_ERROR_INVALID_DEVICE           =  7,
	MRST_ERROR_DEVICE_REGISTER_FAILED   =  8
} MRST_ERROR;


#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

MRST_ERROR MRSTLFBInit(struct drm_device * dev);
MRST_ERROR MRSTLFBDeinit(void);

int MRSTLFBAllocBuffer(struct drm_device *dev, IMG_UINT32 ui32Size, MRSTLFB_BUFFER **ppBuffer);
int MRSTLFBFreeBuffer(struct drm_device *dev, MRSTLFB_BUFFER **ppBuffer);

void *MRSTLFBAllocKernelMem(unsigned long ulSize);
void MRSTLFBFreeKernelMem(void *pvMem);
MRST_ERROR MRSTLFBGetLibFuncAddr(char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable);
MRST_ERROR MRSTLFBInstallVSyncISR (MRSTLFB_DEVINFO *psDevInfo, MRSTLFB_VSYNC_ISR_PFN pVsyncHandler);
MRST_ERROR MRSTLFBUninstallVSyncISR(MRSTLFB_DEVINFO *psDevInfo);
void MRSTLFBEnableVSyncInterrupt(MRSTLFB_DEVINFO *psDevInfo);
void MRSTLFBDisableVSyncInterrupt(MRSTLFB_DEVINFO *psDevInfo);

void MRSTLFBFlipToSurface(MRSTLFB_DEVINFO *psDevInfo,  unsigned long uiAddr);

void MRSTLFBSuspend(void);
void MRSTLFBResume(void);
int MRSTLFBAllocBufferPages(struct drm_device *dev, IMG_UINT32 ui32Size, MRSTLFB_BUFFER **ppBuffer);
int MRSTLFBFreeBufferPages(struct drm_device *dev, MRSTLFB_BUFFER **ppBuffer);

#endif 

