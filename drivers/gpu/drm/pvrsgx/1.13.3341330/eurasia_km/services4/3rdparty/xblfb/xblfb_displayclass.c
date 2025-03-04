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
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>

/* IMG services headers */
#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "xblfb.h"

#define XBLFB_COMMAND_COUNT		1

#define	XBLFB_VSYNC_SETTLE_COUNT	5

#define	XBLFB_MAX_NUM_DEVICES		FB_MAX  //32
#if (XBLFB_MAX_NUM_DEVICES > FB_MAX)
#error "XBLFB_MAX_NUM_DEVICES must not be greater than FB_MAX"
#endif

// Storing the Infomation of every Screen
static XBLFB_DEVINFO *gapsDevInfo[XBLFB_MAX_NUM_DEVICES];

/* Top level 'hook ptr' */
static PFN_DC_GET_PVRJTABLE gpfnGetPVRJTable = NULL;

static inline unsigned long RoundUpToMultiple(unsigned long x, unsigned long y)
{
	unsigned long div = x / y;
	unsigned long rem = x % y;

	return (div + ((rem == 0) ? 0 : 1)) * y;
}

// GCD = Greatest Common Divisior
static unsigned long GCD(unsigned long x, unsigned long y)
{
	while (y != 0)
	{
		unsigned long r = x % y;
		x = y;
		y = r;
	}

	return x;
}

// LCM = Least Common Multiple
static unsigned long LCM(unsigned long x, unsigned long y)
{
	unsigned long gcd = GCD(x, y);

	return (gcd == 0) ? 0 : ((x / gcd) * y);
}

unsigned XBLFBMaxFBDevIDPlusOne(void)
{
	return XBLFB_MAX_NUM_DEVICES;
}

/* Returns DevInfo pointer for a given device */
XBLFB_DEVINFO *XBLFBGetDevInfoPtr(unsigned uiFBDevID)
{
	WARN_ON(uiFBDevID >= XBLFBMaxFBDevIDPlusOne());

	if (uiFBDevID >= XBLFB_MAX_NUM_DEVICES)
	{
		return NULL;
	}

	return gapsDevInfo[uiFBDevID];
}

/* Sets the DevInfo pointer for a given device */
static inline void XBLFBSetDevInfoPtr(unsigned uiFBDevID, XBLFB_DEVINFO *psDevInfo)
{
	WARN_ON(uiFBDevID >= XBLFB_MAX_NUM_DEVICES);

	if (uiFBDevID < XBLFB_MAX_NUM_DEVICES)
	{
		gapsDevInfo[uiFBDevID] = psDevInfo;
	}
}

static inline XBLFB_BOOL SwapChainHasChanged(XBLFB_DEVINFO *psDevInfo, XBLFB_SWAPCHAIN *psSwapChain)
{
	return (psDevInfo->psSwapChain != psSwapChain) ||
		(psDevInfo->uiSwapChainID != psSwapChain->uiSwapChainID);
}

/* Don't wait for vertical sync */
static inline XBLFB_BOOL DontWaitForVSync(XBLFB_DEVINFO *psDevInfo)
{
	XBLFB_BOOL bDontWait;

	bDontWait = XBLFBAtomicBoolRead(&psDevInfo->sBlanked) ||
			XBLFBAtomicBoolRead(&psDevInfo->sFlushCommands);

#if defined(CONFIG_HAS_EARLYSUSPEND)
	bDontWait = bDontWait || XBLFBAtomicBoolRead(&psDevInfo->sEarlySuspendFlag);
#endif

	return bDontWait;
}

/*
 * SetDCState
 * Called from services.
 */
static IMG_VOID SetDCState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	XBLFB_DEVINFO *psDevInfo = (XBLFB_DEVINFO *)hDevice;

	switch (ui32State)
	{
		case DC_STATE_FLUSH_COMMANDS:
			XBLFBAtomicBoolSet(&psDevInfo->sFlushCommands, XBLFB_TRUE);
			break;
		case DC_STATE_NO_FLUSH_COMMANDS:
			XBLFBAtomicBoolSet(&psDevInfo->sFlushCommands, XBLFB_FALSE);
			break;
#if 0
		/*
		 * This feature has been dropped in IMG DDK 1.11.
		 *
		 * This code was likely copied from OMAP implementation and isn't needed.
		 * I believe the OMAP used the GPU Hardware Composer and this facility
		 * slipped in a dummy frame if the GPU hardware got stuck.
		 *
		 * Ingenic uses it's own Hardware Composer and thus never needed
		 * this old GPU workaround.
		 */
		case DC_STATE_FORCE_SWAP_TO_SYSTEM:
			XBLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
			break;
#endif
		default:
			break;
	}
}

/*
 * OpenDCDevice
 * Called from services.
 */
static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 uiPVRDevID,
                                 IMG_HANDLE *phDevice,
                                 PVRSRV_SYNC_DATA* psSystemBufferSyncData)
{
	XBLFB_DEVINFO *psDevInfo;
	XBLFB_ERROR eError;
	unsigned uiMaxFBDevIDPlusOne = XBLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		psDevInfo = XBLFBGetDevInfoPtr(i);
		if (psDevInfo != NULL && psDevInfo->uiPVRDevID == uiPVRDevID)
		{
			break;
		}
	}
	if (i == uiMaxFBDevIDPlusOne)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: PVR Device %u not found\n", __FUNCTION__, uiPVRDevID));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}

	/* store the system surface sync data */
	psDevInfo->sSystemBuffer.psSyncData = psSystemBufferSyncData;
	
	eError = XBLFBUnblankDisplay(psDevInfo);
	if (eError != XBLFB_OK)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: XBLFBUnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError));
		return PVRSRV_ERROR_UNBLANK_DISPLAY_FAILED;
	}

	/* return handle to the devinfo */
	*phDevice = (IMG_HANDLE)psDevInfo;
	
	return PVRSRV_OK;
}

/*
 * CloseDCDevice
 * Called from services.
 */
static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
	UNREFERENCED_PARAMETER(hDevice);

	return PVRSRV_OK;
}

/*
 * EnumDCFormats
 * Called from services.
 */
static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE hDevice,
								  IMG_UINT32 *pui32NumFormats,
								  DISPLAY_FORMAT *psFormat)
{
	XBLFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !pui32NumFormats)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (XBLFB_DEVINFO*)hDevice;
	
	*pui32NumFormats = 1;
	
	if(psFormat)
	{
		psFormat[0] = psDevInfo->sDisplayFormat;
	}

	return PVRSRV_OK;
}

/*
 * EnumDCDims
 * Called from services.
 */
static PVRSRV_ERROR EnumDCDims(IMG_HANDLE hDevice, 
							   DISPLAY_FORMAT *psFormat,
							   IMG_UINT32 *pui32NumDims,
							   DISPLAY_DIMS *psDim)
{
	XBLFB_DEVINFO	*psDevInfo;

	if(!hDevice || !psFormat || !pui32NumDims)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (XBLFB_DEVINFO*)hDevice;

	*pui32NumDims = 1;

	/* No need to look at psFormat; there is only one */
	if(psDim)
	{
		psDim[0] = psDevInfo->sDisplayDim;
	}
	
	return PVRSRV_OK;
}


/*
 * GetDCSystemBuffer
 * Called from services.
 */
static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice, IMG_HANDLE *phBuffer)
{
	XBLFB_DEVINFO	*psDevInfo;

	if(!hDevice || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (XBLFB_DEVINFO*)hDevice;

	*phBuffer = (IMG_HANDLE)&psDevInfo->sSystemBuffer;

	return PVRSRV_OK;
}


/*
 * GetDCInfo
 * Called from services.
 */
static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	XBLFB_DEVINFO	*psDevInfo;

	if(!hDevice || !psDCInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (XBLFB_DEVINFO*)hDevice;

	*psDCInfo = psDevInfo->sDisplayInfo;

	return PVRSRV_OK;
}

/*
 * GetDCBufferAddr
 * Called from services.
 */
static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE        hDevice,
									IMG_HANDLE        hBuffer,
									IMG_SYS_PHYADDR   **ppsSysAddr,
									IMG_UINT32        *pui32ByteSize,
									IMG_VOID          **ppvCpuVAddr,
									IMG_HANDLE        *phOSMapInfo,
									IMG_BOOL          *pbIsContiguous,
									IMG_UINT32        *pui32TilingStride)
{
	XBLFB_DEVINFO	*psDevInfo;
	XBLFB_BUFFER *psSystemBuffer;

	UNREFERENCED_PARAMETER(pui32TilingStride);

	if(!hDevice)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(!hBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (!ppsSysAddr)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (!pui32ByteSize)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (XBLFB_DEVINFO*)hDevice;

	psSystemBuffer = (XBLFB_BUFFER *)hBuffer;

	*ppsSysAddr = &psSystemBuffer->sSysAddr;

	*pui32ByteSize = (IMG_UINT32)psDevInfo->sFBInfo.ulBufferSize;

	if (ppvCpuVAddr)
	{
		*ppvCpuVAddr = psSystemBuffer->sCPUVAddr;
	}

	if (phOSMapInfo)
	{
		*phOSMapInfo = (IMG_HANDLE)0;
	}

	if (pbIsContiguous)
	{
		*pbIsContiguous = IMG_TRUE;
	}

	return PVRSRV_OK;
}

/*
 * CreateDCSwapChain
 * Called from services.
 */
static PVRSRV_ERROR CreateDCSwapChain(IMG_HANDLE hDevice,
									  IMG_UINT32 ui32Flags,
									  DISPLAY_SURF_ATTRIBUTES *psDstSurfAttrib,
									  DISPLAY_SURF_ATTRIBUTES *psSrcSurfAttrib,
									  IMG_UINT32 ui32BufferCount,
									  PVRSRV_SYNC_DATA **ppsSyncData,
									  IMG_UINT32 ui32OEMFlags,
									  IMG_HANDLE *phSwapChain,
									  IMG_UINT32 *pui32SwapChainID)
{
	XBLFB_DEVINFO	*psDevInfo;
	XBLFB_SWAPCHAIN *psSwapChain;
	XBLFB_BUFFER *psBuffer;
	IMG_UINT32 i;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32BuffersToSkip;

	UNREFERENCED_PARAMETER(ui32OEMFlags);

	/* Check parameters */
	if(!hDevice
	|| !psDstSurfAttrib
	|| !psSrcSurfAttrib
	|| !ppsSyncData
	|| !phSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (XBLFB_DEVINFO*)hDevice;
	
	/* Do we support swap chains? */
	if (psDevInfo->sDisplayInfo.ui32MaxSwapChains == 0)
	{
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	XBLFBCreateSwapChainLock(psDevInfo);

	/* The driver only supports a single swapchain */
	if(psDevInfo->psSwapChain != NULL)
	{
		eError = PVRSRV_ERROR_FLIP_CHAIN_EXISTS;
		goto ExitUnLock;
	}
	
	/* Check the buffer count */
	if(ui32BufferCount > psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers)
	{
		eError = PVRSRV_ERROR_TOOMANYBUFFERS;
		goto ExitUnLock;
	}
	
	if ((psDevInfo->sFBInfo.ulRoundedBufferSize * (unsigned long)ui32BufferCount) > psDevInfo->sFBInfo.ulFBSize)
	{
		eError = PVRSRV_ERROR_TOOMANYBUFFERS;
		goto ExitUnLock;
	}

	/*
	 * We will allocate the swap chain buffers at the back of the frame
	 * buffer area.  This preserves the front portion, which may be being
	 * used by other Linux Framebuffer based applications.
	 */
	ui32BuffersToSkip = psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers - ui32BufferCount;

	/*
	 *	Verify the DST/SRC attributes,
	 *	SRC/DST must match the current display mode config
	*/
	if(psDstSurfAttrib->pixelformat != psDevInfo->sDisplayFormat.pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psDevInfo->sDisplayDim.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psDevInfo->sDisplayDim.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psDevInfo->sDisplayDim.ui32Height)
	{
		/* DST doesn't match the current mode */
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}		

	if(psDstSurfAttrib->pixelformat != psSrcSurfAttrib->pixelformat
			|| psDstSurfAttrib->sDims.ui32ByteStride != psSrcSurfAttrib->sDims.ui32ByteStride
			|| psDstSurfAttrib->sDims.ui32Width != psSrcSurfAttrib->sDims.ui32Width
			|| psDstSurfAttrib->sDims.ui32Height != psSrcSurfAttrib->sDims.ui32Height)
	{
		/* DST doesn't match the SRC */
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}

	/* check flags if implementation requires them */
	UNREFERENCED_PARAMETER(ui32Flags);
	
	/* create a swapchain structure */
	psSwapChain = (XBLFB_SWAPCHAIN*)XBLFBAllocKernelMem(sizeof(XBLFB_SWAPCHAIN));
	if(!psSwapChain)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ExitUnLock;
	}

	psBuffer = (XBLFB_BUFFER*)XBLFBAllocKernelMem(sizeof(XBLFB_BUFFER) * ui32BufferCount);
	if(!psBuffer)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorFreeSwapChain;
	}

	psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
	psSwapChain->psBuffer = psBuffer;
	psSwapChain->bNotVSynced = XBLFB_TRUE;
	psSwapChain->uiFBDevID = psDevInfo->uiFBDevID;

	/* Link the buffers */
	for(i=0; i<ui32BufferCount-1; i++)
	{
		psBuffer[i].psNext = &psBuffer[i+1];
	}
	/* and link last to first */
	psBuffer[i].psNext = &psBuffer[0];

	/* Configure the swapchain buffers */
	for(i=0; i<ui32BufferCount; i++)
	{
		IMG_UINT32 ui32SwapBuffer = i + ui32BuffersToSkip;
		IMG_UINT32 ui32BufferOffset = ui32SwapBuffer * (IMG_UINT32)psDevInfo->sFBInfo.ulRoundedBufferSize;

		psBuffer[i].psSyncData = ppsSyncData[i];

		psBuffer[i].sSysAddr.uiAddr = psDevInfo->sFBInfo.sSysAddr.uiAddr + ui32BufferOffset;
		psBuffer[i].sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr + ui32BufferOffset;
		psBuffer[i].ulYOffset = ui32BufferOffset / psDevInfo->sFBInfo.ulByteStride;
		psBuffer[i].psDevInfo = psDevInfo;

		XBLFBInitBufferForSwap(&psBuffer[i]);
	}

	if (XBLFBCreateSwapQueue(psSwapChain) != XBLFB_OK)
	{ 
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Failed to create workqueue\n", __FUNCTION__, psDevInfo->uiFBDevID);
		eError = PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
		goto ErrorFreeBuffers;
	}

	if (XBLFBEnableLFBEventNotification(psDevInfo)!= XBLFB_OK)
	{
		eError = PVRSRV_ERROR_UNABLE_TO_ENABLE_EVENT;
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't enable framebuffer event notification\n", __FUNCTION__, psDevInfo->uiFBDevID);
		goto ErrorDestroySwapQueue;
	}

	psDevInfo->uiSwapChainID++;
	if (psDevInfo->uiSwapChainID == 0)
	{
		psDevInfo->uiSwapChainID++;
	}

	psSwapChain->uiSwapChainID = psDevInfo->uiSwapChainID;

	psDevInfo->psSwapChain = psSwapChain;

	*pui32SwapChainID = psDevInfo->uiSwapChainID;

	*phSwapChain = (IMG_HANDLE)psSwapChain;

	eError = PVRSRV_OK;
	goto ExitUnLock;

ErrorDestroySwapQueue:
	XBLFBDestroySwapQueue(psSwapChain);
ErrorFreeBuffers:
	XBLFBFreeKernelMem(psBuffer);
ErrorFreeSwapChain:
	XBLFBFreeKernelMem(psSwapChain);
ExitUnLock:
	XBLFBCreateSwapChainUnLock(psDevInfo);
	return eError;
}

/*
 * DestroyDCSwapChain
 * Called from services.
 */
static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,
	IMG_HANDLE hSwapChain)
{
	XBLFB_DEVINFO	*psDevInfo;
	XBLFB_SWAPCHAIN *psSwapChain;
	XBLFB_ERROR eError;

	/* Check parameters */	
	if(!hDevice || !hSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (XBLFB_DEVINFO*)hDevice;
	psSwapChain = (XBLFB_SWAPCHAIN*)hSwapChain;

	XBLFBCreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}

	/* The swap queue is flushed before being destroyed */	
	XBLFBDestroySwapQueue(psSwapChain);

	eError = XBLFBDisableLFBEventNotification(psDevInfo);
	if (eError != XBLFB_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't disable framebuffer event notification\n", __FUNCTION__, psDevInfo->uiFBDevID);
	}

	/* Free resources */	
	XBLFBFreeKernelMem(psSwapChain->psBuffer);
	XBLFBFreeKernelMem(psSwapChain);

	psDevInfo->psSwapChain = NULL;

	XBLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
	/* (void) XBLFBCheckModeAndSync(psDevInfo); */

	eError = PVRSRV_OK;

ExitUnLock:
	XBLFBCreateSwapChainUnLock(psDevInfo);

	return eError;
}

/*
 * SetDCDstRect
 * Called from services.
 */
static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE hDevice,
	IMG_HANDLE hSwapChain,
	IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	/* Only full display swapchains on this device */
	
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/*
 * SetDCSrcRect
 * Called from services.
 */
static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE hDevice,
								 IMG_HANDLE hSwapChain,
								 IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	/* Only full display swapchains on this device */

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/*
 * SetDCDstColourKey
 * Called from services.
 */
static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE hDevice,
									  IMG_HANDLE hSwapChain,
									  IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	/* Don't support DST CK on this device */

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/*
 * SetDCSrcColourKey
 * Called from services.
 */
static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE hDevice,
									  IMG_HANDLE hSwapChain,
									  IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	/* Don't support SRC CK on this device */

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/*
 * GetDCBuffers
 * Called from services.
 */
static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE hDevice,
								 IMG_HANDLE hSwapChain,
								 IMG_UINT32 *pui32BufferCount,
								 IMG_HANDLE *phBuffer)
{
	XBLFB_DEVINFO   *psDevInfo;
	XBLFB_SWAPCHAIN *psSwapChain;
	PVRSRV_ERROR eError;
	unsigned i;

	/* Check parameters */
	if(!hDevice 
	|| !hSwapChain
	|| !pui32BufferCount
	|| !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (XBLFB_DEVINFO*)hDevice;
	psSwapChain = (XBLFB_SWAPCHAIN*)hSwapChain;

	XBLFBCreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto Exit;
	}
	
	/* Return the buffer count */
	*pui32BufferCount = (IMG_UINT32)psSwapChain->ulBufferCount;
	
	/* Return the buffers */
	for(i=0; i<psSwapChain->ulBufferCount; i++)
	{
		phBuffer[i] = (IMG_HANDLE)&psSwapChain->psBuffer[i];
	}
	
	eError = PVRSRV_OK;

Exit:
	XBLFBCreateSwapChainUnLock(psDevInfo);

	return eError;
}

/*
 * SwapToDCBuffer
 * Called from services.
 */
static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE hDevice,
								   IMG_HANDLE hBuffer,
								   IMG_UINT32 ui32SwapInterval,
								   IMG_HANDLE hPrivateTag,
								   IMG_UINT32 ui32ClipRectCount,
								   IMG_RECT *psClipRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hBuffer);
	UNREFERENCED_PARAMETER(ui32SwapInterval);
	UNREFERENCED_PARAMETER(hPrivateTag);
	UNREFERENCED_PARAMETER(ui32ClipRectCount);
	UNREFERENCED_PARAMETER(psClipRect);
	
	/* * Nothing to do since Services common code does the work */

	return PVRSRV_OK;
}

/*
 * Called after the screen has unblanked, or after any other occasion
 * when we didn't wait for vsync, but now need to. Not doing this after
 * unblank leads to screen jitter on some screens.
 * Returns true if the screen has been deemed to have settled.
 */
/* static XBLFB_BOOL WaitForVSyncSettle(XBLFB_DEVINFO *psDevInfo) */
/* { */
/* } */

/*
 * Swap handler.
 * Called from the swap chain work queue handler.
 * There is no need to take the swap chain creation lock in here, or use
 * some other method of stopping the swap chain from being destroyed.
 * This is because the swap chain creation lock is taken when queueing work,
 * and the work queue is flushed before the swap chain is destroyed.
 */
void XBLFBSwapHandler(XBLFB_BUFFER *psBuffer)
{
	XBLFB_DEVINFO *psDevInfo = psBuffer->psDevInfo;
	XBLFB_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	XBLFB_BOOL bPreviouslyNotVSynced;

	{
		XBLFBFlip(psDevInfo, psBuffer);
	}

	bPreviouslyNotVSynced = psSwapChain->bNotVSynced;
	psSwapChain->bNotVSynced = XBLFB_TRUE;

	//printk(KERN_WARNING DRIVER_PREFIX ": %s: Work [%p] pre flip complete.\n", __FUNCTION__, psBuffer->sCPUVAddr);

	psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psBuffer->hCmdComplete, IMG_TRUE);

	//printk(KERN_WARNING DRIVER_PREFIX ": %s: Work [%p] post flip complete.\n", __FUNCTION__, psBuffer->sCPUVAddr);
}

/* Triggered by PVRSRVSwapToDCBuffer
 *  
 * A swap chain is created of a specific size/number of buffers 
 * and ProcessFlipV1 is used to switch between those buffers in a specific order. 
 * The function is only handed buffers in the swap chain and always 
 * in the same order e.g. 1,2,3,1,2,3.. 
 * This is used by Xorg on Linux
 */
static IMG_BOOL ProcessFlipV1(IMG_HANDLE hCmdCookie,
							  XBLFB_DEVINFO *psDevInfo,
							  XBLFB_SWAPCHAIN *psSwapChain,
							  XBLFB_BUFFER *psBuffer,
							  unsigned long ulSwapInterval)
{
	XBLFBCreateSwapChainLock(psDevInfo);

	/* The swap chain has been destroyed */
	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u (PVR Device ID %u): The swap chain has been destroyed\n",
			__FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));
	}
	else
	{
		psBuffer->hCmdComplete = (XBLFB_HANDLE)hCmdCookie;
		psBuffer->ulSwapInterval = ulSwapInterval;
		{
			XBLFBQueueBufferForSwap(psSwapChain, psBuffer);
		}
	}

	XBLFBCreateSwapChainUnLock(psDevInfo);

	return IMG_TRUE;
}

/* ProcessFlipV2 uses same ProcessFlip entry point. 
 * The reason ProcessFlipV2 exists is that the hardware composer builds on top of the 
 * concept of a swap chain.
 * This signals that the command contains a  payload of private data. The private 
 * data is a vendor-specific structure passed from the hardware composer in userspace
 * which contains arbitrarily complex programming metadata. Instead of a swap chain 
 * buffer being passed to this function, an array of MEMINFOs is passed instead, as 
 * with the HWC path we can have arbitrary buffers rather than a swap chain.
 * 
 * The combination of MEMINFOs and private data should be enough for the customer to program 
 * any buffer to their display engine.
 * 
 */
static IMG_BOOL ProcessFlipV2(IMG_HANDLE hCmdCookie,
							  XBLFB_DEVINFO *psDevInfo,
							  PDC_MEM_INFO *ppsMemInfos,
							  IMG_UINT32 ui32NumMemInfos,
							  IMG_PVOID *psDispcData,
							  IMG_UINT32 ui32DispcDataLength)
{
	XBLFBCreateSwapChainLock(psDevInfo);

	{
		IMG_CPU_VIRTADDR pViAddress;
		gapsDevInfo[0]->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuVAddr(ppsMemInfos[0], &pViAddress);

		XBLFBQueueBufferForSwap2(psDevInfo->psSwapChain, pViAddress, (XBLFB_HANDLE)hCmdCookie);
	}

	XBLFBCreateSwapChainUnLock(psDevInfo);

	return IMG_TRUE;
}

/* Command processing flip handler function.  Called from services. */
static IMG_BOOL ProcessFlip(IMG_HANDLE  hCmdCookie,
							IMG_UINT32  ui32DataSize,
							IMG_VOID   *pvData)
{
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	XBLFB_DEVINFO *psDevInfo;

	/* Check parameters  */
	if(!hCmdCookie || !pvData)
	{
		return IMG_FALSE;
	}

	/* Validate data packet  */
	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND*)pvData;

	if (psFlipCmd == IMG_NULL)
	{
		return IMG_FALSE;
	}

	psDevInfo = (XBLFB_DEVINFO*)psFlipCmd->hExtDevice;

	if(psFlipCmd->hExtBuffer)
	{
		return ProcessFlipV1(hCmdCookie,
							 psDevInfo,
							 psFlipCmd->hExtSwapChain,
							 psFlipCmd->hExtBuffer,
							 psFlipCmd->ui32SwapInterval);
	}
	else
	{
		/* If hExtBuffer is NULL, this signals that the command contains 
		 * a payload of private data. Call ProcessFlipV2 to handle this. 
		 */
		DISPLAYCLASS_FLIP_COMMAND2 *psFlipCmd2;
		psFlipCmd2 = (DISPLAYCLASS_FLIP_COMMAND2 *)pvData;
		return ProcessFlipV2(hCmdCookie,
							 psDevInfo,
							 psFlipCmd2->ppsMemInfos,
							 psFlipCmd2->ui32NumMemInfos,
							 psFlipCmd2->pvPrivData,
							 psFlipCmd2->ui32PrivDataLength);
	}
}

static XBLFB_ERROR XBLFBInitFBDev(XBLFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo;
	struct module *psLINFBOwner;
	XBLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	XBLFB_ERROR eError = XBLFB_ERROR_GENERIC;
	unsigned long FBSize;
	unsigned long ulLCM;
	unsigned uiFBDevID = psDevInfo->uiFBDevID;

	XBLFB_CONSOLE_LOCK();

	psLINFBInfo = registered_fb[uiFBDevID];
	if (psLINFBInfo == NULL)
	{
		eError = XBLFB_ERROR_INVALID_DEVICE;
		goto ErrorRelSem;
	}

	FBSize = (psLINFBInfo->screen_size) != 0 ?
					psLINFBInfo->screen_size :
					psLINFBInfo->fix.smem_len;

	
	if (FBSize == 0 || psLINFBInfo->fix.line_length == 0)
	{
		eError = XBLFB_ERROR_INVALID_DEVICE;
		goto ErrorRelSem;
	}

	psLINFBOwner = psLINFBInfo->fbops->owner;
	if (!try_module_get(psLINFBOwner))
	{
		printk(KERN_INFO DRIVER_PREFIX
			": %s: Device %u: Couldn't get framebuffer module\n", __FUNCTION__, uiFBDevID);

		goto ErrorRelSem;
	}

	if (psLINFBInfo->fbops->fb_open != NULL)
	{
		int res;

		res = psLINFBInfo->fbops->fb_open(psLINFBInfo, 0);
		if (res != 0)
		{
			printk(KERN_INFO DRIVER_PREFIX
				" %s: Device %u: Couldn't open framebuffer(%d)\n", __FUNCTION__, uiFBDevID, res);

			goto ErrorModPut;
		}
	}

	psDevInfo->psLINFBInfo = psLINFBInfo;

	ulLCM = LCM(psLINFBInfo->fix.line_length, XBLFB_PAGE_SIZE); //4096

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer physical address: 0x%lx\n",
			psDevInfo->uiFBDevID, psLINFBInfo->fix.smem_start));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual address: 0x%lx\n",
			psDevInfo->uiFBDevID, (unsigned long)psLINFBInfo->screen_base));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer size: %lu\n",
			psDevInfo->uiFBDevID, FBSize));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual width: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.xres_virtual));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual height: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.yres_virtual));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer width: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.xres));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer height: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.yres));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer stride: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->fix.line_length));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: LCM of stride and page size: %lu\n",
			psDevInfo->uiFBDevID, ulLCM));

	/* Additional implementation specific information */	
        /* XBLFBPrintInfo(psDevInfo); */

	/* System Surface */
	psPVRFBInfo->sSysAddr.uiAddr = psLINFBInfo->fix.smem_start;
	psPVRFBInfo->sCPUVAddr = psLINFBInfo->screen_base;

	psPVRFBInfo->ulWidth = psLINFBInfo->var.xres;
	psPVRFBInfo->ulHeight = psLINFBInfo->var.yres;
	psPVRFBInfo->ulByteStride =  psLINFBInfo->fix.line_length;
	psPVRFBInfo->ulFBSize = FBSize;

	psPVRFBInfo->ulBufferSize = psPVRFBInfo->ulHeight * psPVRFBInfo->ulByteStride;

	/* Round the buffer size up to a multiple of the number of pages
	 * and the byte stride.
	 * This is used internally, to ensure buffers start on page
	 * boundaries, for the benefit of PVR Services.
	 */
	psPVRFBInfo->ulRoundedBufferSize = RoundUpToMultiple(psPVRFBInfo->ulBufferSize, ulLCM);

	if(psLINFBInfo->var.bits_per_pixel == 16)
	{
		if((psLINFBInfo->var.red.length == 5) &&
			(psLINFBInfo->var.green.length == 6) && 
			(psLINFBInfo->var.blue.length == 5) && 
			(psLINFBInfo->var.red.offset == 11) &&
			(psLINFBInfo->var.green.offset == 5) && 
			(psLINFBInfo->var.blue.offset == 0) && 
			(psLINFBInfo->var.red.msb_right == 0))
		{
			psPVRFBInfo->ePixelFormat = PVRSRV_PIXEL_FORMAT_RGB565;
		}
		else
		{
                    printk(KERN_INFO DRIVER_PREFIX ": %s: %d Device %u: Unknown FB format\n", __FUNCTION__, __LINE__, uiFBDevID);
		}
	}
	else if(psLINFBInfo->var.bits_per_pixel == 32)
	{
		/* if((psLINFBInfo->var.red.length == 8) && */
		/* 	(psLINFBInfo->var.green.length == 8) &&  */
		/* 	(psLINFBInfo->var.blue.length == 8) &&  */
		/* 	(psLINFBInfo->var.red.offset == 0) && */
		/* 	(psLINFBInfo->var.green.offset == 8) &&  */
		/* 	(psLINFBInfo->var.blue.offset == 16) &&  */
		/* 	(psLINFBInfo->var.red.msb_right == 0)) */
		if((psLINFBInfo->var.red.length == 8) &&
			(psLINFBInfo->var.green.length == 8) && 
			(psLINFBInfo->var.blue.length == 8) && 
			(psLINFBInfo->var.red.offset == 16) &&
			(psLINFBInfo->var.green.offset == 8) && 
			(psLINFBInfo->var.blue.offset == 0))
		{
			psPVRFBInfo->ePixelFormat = PVRSRV_PIXEL_FORMAT_ARGB8888;
		}
		else
		{
			printk(KERN_INFO DRIVER_PREFIX ": %s: %d Device %u: Unknown FB format\n", __FUNCTION__, __LINE__, uiFBDevID);
		}
	}	
	else
	{
		printk(KERN_INFO DRIVER_PREFIX ": %s: %d Device %u: Unknown FB format\n", __FUNCTION__, __LINE__, uiFBDevID);
	}

	psDevInfo->sFBInfo.ulPhysicalWidthmm =
		((int)psLINFBInfo->var.width  > 0) ? psLINFBInfo->var.width  : 90;

	psDevInfo->sFBInfo.ulPhysicalHeightmm =
		((int)psLINFBInfo->var.height > 0) ? psLINFBInfo->var.height : 54;

	/* System Surface */
	psDevInfo->sFBInfo.sSysAddr.uiAddr = psPVRFBInfo->sSysAddr.uiAddr;
	psDevInfo->sFBInfo.sCPUVAddr = psPVRFBInfo->sCPUVAddr;

	eError = XBLFB_OK;
	goto ErrorRelSem;

ErrorModPut:
	module_put(psLINFBOwner);
ErrorRelSem:
	XBLFB_CONSOLE_UNLOCK();

	return eError;
}

static void XBLFBDeInitFBDev(XBLFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo = psDevInfo->psLINFBInfo;
	struct module *psLINFBOwner;

	XBLFB_CONSOLE_LOCK();

	psLINFBOwner = psLINFBInfo->fbops->owner;

	if (psLINFBInfo->fbops->fb_release != NULL) 
	{
		(void) psLINFBInfo->fbops->fb_release(psLINFBInfo, 0);
	}

	module_put(psLINFBOwner);

	XBLFB_CONSOLE_UNLOCK();
}

static XBLFB_DEVINFO *XBLFBInitDev(unsigned uiFBDevID)
{
	PFN_CMD_PROC	 	pfnCmdProcList[XBLFB_COMMAND_COUNT];
	IMG_UINT32		aui32SyncCountList[XBLFB_COMMAND_COUNT][2];
	XBLFB_DEVINFO		*psDevInfo = NULL;

	/* Allocate device info. structure */	
	psDevInfo = (XBLFB_DEVINFO *)XBLFBAllocKernelMem(sizeof(XBLFB_DEVINFO));

	if(psDevInfo == NULL)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: Couldn't allocate device information structure\n", __FUNCTION__, uiFBDevID);

		goto ErrorExit;
	}

	/* Any fields not set will be zero */
	memset(psDevInfo, 0, sizeof(XBLFB_DEVINFO));

	psDevInfo->uiFBDevID = uiFBDevID;

	/* Get the kernel services function table */
	if(!(*gpfnGetPVRJTable)(&psDevInfo->sPVRJTable))
	{
		goto ErrorFreeDevInfo;
	}

	/* Save private fbdev information structure in the dev. info. */
	if(XBLFBInitFBDev(psDevInfo) != XBLFB_OK)
	{
		/*
		 * Leave it to XBLFBInitFBDev to print an error message, if
		 * required.  The function may have failed because
		 * there is no Linux framebuffer device corresponding
		 * to the device ID.
		 */
		goto ErrorFreeDevInfo;
	}

	psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers = (IMG_UINT32)(psDevInfo->sFBInfo.ulFBSize / psDevInfo->sFBInfo.ulRoundedBufferSize);
	if (psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers != 0)
	{
		psDevInfo->sDisplayInfo.ui32MaxSwapChains = 1;
		psDevInfo->sDisplayInfo.ui32MaxSwapInterval = 1;
	}

	psDevInfo->sDisplayInfo.ui32PhysicalWidthmm = psDevInfo->sFBInfo.ulPhysicalWidthmm;
	psDevInfo->sDisplayInfo.ui32PhysicalHeightmm = psDevInfo->sFBInfo.ulPhysicalHeightmm;

	strncpy(psDevInfo->sDisplayInfo.szDisplayName, DISPLAY_DEVICE_NAME, MAX_DISPLAY_NAME_SIZE);

	psDevInfo->sDisplayFormat.pixelformat = psDevInfo->sFBInfo.ePixelFormat;
	psDevInfo->sDisplayDim.ui32Width      = (IMG_UINT32)psDevInfo->sFBInfo.ulWidth;
	psDevInfo->sDisplayDim.ui32Height     = (IMG_UINT32)psDevInfo->sFBInfo.ulHeight;
	psDevInfo->sDisplayDim.ui32ByteStride = (IMG_UINT32)psDevInfo->sFBInfo.ulByteStride;

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		": Device %u: Maximum number of swap chain buffers: %u\n",
		psDevInfo->uiFBDevID, psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers));

	/* Setup system buffer */
	psDevInfo->sSystemBuffer.sSysAddr = psDevInfo->sFBInfo.sSysAddr;
	psDevInfo->sSystemBuffer.sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr;
	psDevInfo->sSystemBuffer.psDevInfo = psDevInfo;

	XBLFBInitBufferForSwap(&psDevInfo->sSystemBuffer);

	/*
		Setup the DC Jtable so SRVKM can call into this driver
	*/
	psDevInfo->sDCJTable.ui32TableSize = sizeof(PVRSRV_DC_SRV2DISP_KMJTABLE);
	psDevInfo->sDCJTable.pfnOpenDCDevice = OpenDCDevice;
	psDevInfo->sDCJTable.pfnCloseDCDevice = CloseDCDevice;
	psDevInfo->sDCJTable.pfnEnumDCFormats = EnumDCFormats;
	psDevInfo->sDCJTable.pfnEnumDCDims = EnumDCDims;
	psDevInfo->sDCJTable.pfnGetDCSystemBuffer = GetDCSystemBuffer;
	psDevInfo->sDCJTable.pfnGetDCInfo = GetDCInfo;
	psDevInfo->sDCJTable.pfnGetBufferAddr = GetDCBufferAddr;
	psDevInfo->sDCJTable.pfnCreateDCSwapChain = CreateDCSwapChain;
	psDevInfo->sDCJTable.pfnDestroyDCSwapChain = DestroyDCSwapChain;
	psDevInfo->sDCJTable.pfnSetDCDstRect = SetDCDstRect;
	psDevInfo->sDCJTable.pfnSetDCSrcRect = SetDCSrcRect;
	psDevInfo->sDCJTable.pfnSetDCDstColourKey = SetDCDstColourKey;
	psDevInfo->sDCJTable.pfnSetDCSrcColourKey = SetDCSrcColourKey;
	psDevInfo->sDCJTable.pfnGetDCBuffers = GetDCBuffers;
	psDevInfo->sDCJTable.pfnSwapToDCBuffer = SwapToDCBuffer;
	psDevInfo->sDCJTable.pfnSetDCState = SetDCState;

	/* Register device with services and retrieve device index */
	if(psDevInfo->sPVRJTable.pfnPVRSRVRegisterDCDevice(
		&psDevInfo->sDCJTable,
		&psDevInfo->uiPVRDevID) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Services device registration failed\n", __FUNCTION__, uiFBDevID);

		goto ErrorDeInitFBDev;
	}
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		": Device %u: PVR Device ID: %u\n",
		psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));
	
	/* Setup private command processing function table ... */
	pfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;

	/* ... and associated sync count(s) */
	aui32SyncCountList[DC_FLIP_COMMAND][0] = 0; /* writes */
	aui32SyncCountList[DC_FLIP_COMMAND][1] = 10; /* reads */

	/*
		Register private command processing functions with
		the Command Queue Manager and setup the general
		command complete function in the devinfo.
	*/
	if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList(psDevInfo->uiPVRDevID,
							       &pfnCmdProcList[0],
							       aui32SyncCountList,
							       XBLFB_COMMAND_COUNT) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: Couldn't register command processing functions with PVR Services\n", __FUNCTION__, uiFBDevID);
		goto ErrorUnregisterDevice;
	}

	XBLFBCreateSwapChainLockInit(psDevInfo);

	XBLFBAtomicBoolInit(&psDevInfo->sBlanked, XBLFB_FALSE);
	XBLFBAtomicIntInit(&psDevInfo->sBlankEvents, 0);
	XBLFBAtomicBoolInit(&psDevInfo->sFlushCommands, XBLFB_FALSE);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	XBLFBAtomicBoolInit(&psDevInfo->sEarlySuspendFlag, XBLFB_FALSE);
#endif
/* #if defined(SUPPORT_DRI_DRM) */
/* 	XBLFBAtomicBoolDeInit(&psDevInfo->sLeaveVT); */
/* #endif */

	return psDevInfo;

ErrorUnregisterDevice:
	(void)psDevInfo->sPVRJTable.pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID);
ErrorDeInitFBDev:
	XBLFBDeInitFBDev(psDevInfo);
ErrorFreeDevInfo:
	XBLFBFreeKernelMem(psDevInfo);
ErrorExit:
	return NULL;
}

XBLFB_ERROR XBLFBInit(void)
{
	unsigned uiMaxFBDevIDPlusOne = XBLFBMaxFBDevIDPlusOne();
	unsigned i;
	unsigned uiDevicesFound = 0;

	if(XBLFBGetLibFuncAddr ("PVRGetDisplayClassJTable", &gpfnGetPVRJTable) != XBLFB_OK)
	{
		return XBLFB_ERROR_INIT_FAILURE;
	}

	/*
	 * We search for frame buffer devices backwards, as the last device
	 * registered with PVR Services will be the first device enumerated
	 * by PVR Services.
	 */
	for(i = uiMaxFBDevIDPlusOne; i-- != 0;)
	{
		XBLFB_DEVINFO *psDevInfo = XBLFBInitDev(i);

		if (psDevInfo != NULL)
		{
			/* Set the top-level anchor */			
			XBLFBSetDevInfoPtr(psDevInfo->uiFBDevID, psDevInfo);
			uiDevicesFound++;
		}
	}

	return (uiDevicesFound != 0) ? XBLFB_OK : XBLFB_ERROR_INIT_FAILURE;
}

/*
 *	XBLFBDeInitDev
 *	DeInitialises one device
 */
static XBLFB_BOOL XBLFBDeInitDev(XBLFB_DEVINFO *psDevInfo)
{
	PVRSRV_DC_DISP2SRV_KMJTABLE *psPVRJTable = &psDevInfo->sPVRJTable;

	XBLFBCreateSwapChainLockDeInit(psDevInfo);

	XBLFBAtomicBoolDeInit(&psDevInfo->sBlanked);
	XBLFBAtomicIntDeInit(&psDevInfo->sBlankEvents);
	XBLFBAtomicBoolDeInit(&psDevInfo->sFlushCommands);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	XBLFBAtomicBoolDeInit(&psDevInfo->sEarlySuspendFlag);
#endif
	psPVRJTable = &psDevInfo->sPVRJTable;

	if (psPVRJTable->pfnPVRSRVRemoveCmdProcList (psDevInfo->uiPVRDevID, XBLFB_COMMAND_COUNT) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't unregister command processing functions\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return XBLFB_FALSE;
	}

	/*
	 * Remove display class device from kernel services device
	 * register.
	 */
	if (psPVRJTable->pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't remove device from PVR Services\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return XBLFB_FALSE;
	}
	
	XBLFBDeInitFBDev(psDevInfo);

	XBLFBSetDevInfoPtr(psDevInfo->uiFBDevID, NULL);

	/* De-allocate data structure */	
	XBLFBFreeKernelMem(psDevInfo);

	return XBLFB_TRUE;
}

/*
 *	XBLFBDeInit
 *	Deinitialises the display class device component of the FBDev
 */
XBLFB_ERROR XBLFBDeInit(void)
{
	unsigned uiMaxFBDevIDPlusOne = XBLFBMaxFBDevIDPlusOne();
	unsigned i;
	XBLFB_BOOL bError = XBLFB_FALSE;

	for(i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		XBLFB_DEVINFO *psDevInfo = XBLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			bError |= !XBLFBDeInitDev(psDevInfo);
		}
	}

	return (bError) ? XBLFB_ERROR_INIT_FAILURE : XBLFB_OK;
}

