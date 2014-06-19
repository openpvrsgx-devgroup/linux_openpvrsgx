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
#include <plat/sgx_omaplfb.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "omaplfb.h"

/*
 * Just use CONFIG_DSSCOMP to distinguish code which was previously had
 * additional mixes of CONFIG_TI_TILER and CONFIG_ION_OMAP. DSSCOMP makes
 * it a given that ION and the TILER will be used.
 */
#if defined(CONFIG_DSSCOMP)
#if !defined(CONFIG_TI_TILER) || !defined(CONFIG_ION_OMAP)
#error Expected CONFIG_TI_TILER and CONFIG_ION_OMAP to be defined
#endif
#include <linux/ion.h>
#include <linux/omap_ion.h>
#include <mach/tiler.h>
#include <video/dsscomp.h>
#include <plat/dsscomp.h>
#include <video/omap_hwc.h>
#include <services_headers.h>


extern struct ion_client *gpsIONClient;

#endif

#define OMAPLFB_COMMAND_COUNT		1

#define	OMAPLFB_VSYNC_SETTLE_COUNT	5

#define	OMAPLFB_MAX_NUM_DEVICES		FB_MAX
#if (OMAPLFB_MAX_NUM_DEVICES > FB_MAX)
#error "OMAPLFB_MAX_NUM_DEVICES must not be greater than FB_MAX"
#endif

static OMAPLFB_DEVINFO *gapsDevInfo[OMAPLFB_MAX_NUM_DEVICES];

static PFN_DC_GET_PVRJTABLE gpfnGetPVRJTable = NULL;

static IMG_BOOL gbBvInterfacePresent;
static IMG_BOOL gbBvReady = IMG_FALSE;
static IMG_BOOL bBltReady = IMG_FALSE;

static inline unsigned long RoundUpToMultiple(unsigned long x, unsigned long y)
{
	unsigned long div = x / y;
	unsigned long rem = x % y;

	return (div + ((rem == 0) ? 0 : 1)) * y;
}

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

static unsigned long LCM(unsigned long x, unsigned long y)
{
	unsigned long gcd = GCD(x, y);

	return (gcd == 0) ? 0 : ((x / gcd) * y);
}

unsigned OMAPLFBMaxFBDevIDPlusOne(void)
{
	return OMAPLFB_MAX_NUM_DEVICES;
}

OMAPLFB_DEVINFO *OMAPLFBGetDevInfoPtr(unsigned uiFBDevID)
{
	WARN_ON(uiFBDevID >= OMAPLFBMaxFBDevIDPlusOne());

	if (uiFBDevID >= OMAPLFB_MAX_NUM_DEVICES)
	{
		return NULL;
	}

	return gapsDevInfo[uiFBDevID];
}

static inline void OMAPLFBSetDevInfoPtr(unsigned uiFBDevID, OMAPLFB_DEVINFO *psDevInfo)
{
	WARN_ON(uiFBDevID >= OMAPLFB_MAX_NUM_DEVICES);

	if (uiFBDevID < OMAPLFB_MAX_NUM_DEVICES)
	{
		gapsDevInfo[uiFBDevID] = psDevInfo;
	}
}

static inline OMAPLFB_BOOL SwapChainHasChanged(OMAPLFB_DEVINFO *psDevInfo, OMAPLFB_SWAPCHAIN *psSwapChain)
{
	return (psDevInfo->psSwapChain != psSwapChain) ||
		(psDevInfo->uiSwapChainID != psSwapChain->uiSwapChainID);
}

static inline OMAPLFB_BOOL DontWaitForVSync(OMAPLFB_DEVINFO *psDevInfo)
{
	OMAPLFB_BOOL bDontWait;

	bDontWait = OMAPLFBAtomicBoolRead(&psDevInfo->sBlanked) ||
			OMAPLFBAtomicBoolRead(&psDevInfo->sFlushCommands);

#if defined(CONFIG_HAS_EARLYSUSPEND)
	bDontWait = bDontWait || OMAPLFBAtomicBoolRead(&psDevInfo->sEarlySuspendFlag);
#endif
#if defined(SUPPORT_DRI_DRM)
	bDontWait = bDontWait || OMAPLFBAtomicBoolRead(&psDevInfo->sLeaveVT);
#endif
	return bDontWait;
}

static IMG_VOID SetDCState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	OMAPLFB_DEVINFO *psDevInfo = (OMAPLFB_DEVINFO *)hDevice;

	switch (ui32State)
	{
		case DC_STATE_FLUSH_COMMANDS:
			OMAPLFBAtomicBoolSet(&psDevInfo->sFlushCommands, OMAPLFB_TRUE);
			break;
		case DC_STATE_NO_FLUSH_COMMANDS:
			OMAPLFBAtomicBoolSet(&psDevInfo->sFlushCommands, OMAPLFB_FALSE);
			break;
//SUPPORT_FORCE_FLIP_WORKAROUND
		case DC_STATE_FORCE_SWAP_TO_SYSTEM:
			OMAPLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
			break;
//SUPPORT_FORCE_FLIP_WORKAROUND
		default:
			break;
	}
}

static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 uiPVRDevID,
                                 IMG_HANDLE *phDevice,
                                 PVRSRV_SYNC_DATA* psSystemBufferSyncData)
{
	OMAPLFB_DEVINFO *psDevInfo;
	OMAPLFB_ERROR eError;
	unsigned uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		psDevInfo = OMAPLFBGetDevInfoPtr(i);
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

	
	psDevInfo->sSystemBuffer.psSyncData = psSystemBufferSyncData;
	
	eError = OMAPLFBUnblankDisplay(psDevInfo);
	if (eError != OMAPLFB_OK)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: OMAPLFBUnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError));
		return PVRSRV_ERROR_UNBLANK_DISPLAY_FAILED;
	}

	
	*phDevice = (IMG_HANDLE)psDevInfo;
	
	return PVRSRV_OK;
}

static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
#if defined(SUPPORT_DRI_DRM)
	OMAPLFB_DEVINFO *psDevInfo = (OMAPLFB_DEVINFO *)hDevice;

	OMAPLFBAtomicBoolSet(&psDevInfo->sLeaveVT, OMAPLFB_FALSE);
	(void) OMAPLFBUnblankDisplay(psDevInfo);
#else
	UNREFERENCED_PARAMETER(hDevice);
#endif
	return PVRSRV_OK;
}

static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE hDevice,
                                  IMG_UINT32 *pui32NumFormats,
                                  DISPLAY_FORMAT *psFormat)
{
	OMAPLFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !pui32NumFormats)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;
	
	*pui32NumFormats = 1;
	
	if(psFormat)
	{
		psFormat[0] = psDevInfo->sDisplayFormat;
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR EnumDCDims(IMG_HANDLE hDevice, 
                               DISPLAY_FORMAT *psFormat,
                               IMG_UINT32 *pui32NumDims,
                               DISPLAY_DIMS *psDim)
{
	OMAPLFB_DEVINFO	*psDevInfo;

	if(!hDevice || !psFormat || !pui32NumDims)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;

	*pui32NumDims = 1;

	
	if(psDim)
	{
		psDim[0] = psDevInfo->sDisplayDim;
	}
	
	return PVRSRV_OK;
}


static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice, IMG_HANDLE *phBuffer)
{
	OMAPLFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;

	*phBuffer = (IMG_HANDLE)&psDevInfo->sSystemBuffer;

	return PVRSRV_OK;
}


static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	OMAPLFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !psDCInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;

	*psDCInfo = psDevInfo->sDisplayInfo;

	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE        hDevice,
                                    IMG_HANDLE        hBuffer, 
                                    IMG_SYS_PHYADDR   **ppsSysAddr,
                                    IMG_UINT32        *pui32ByteSize,
                                    IMG_VOID          **ppvCpuVAddr,
                                    IMG_HANDLE        *phOSMapInfo,
                                    IMG_BOOL          *pbIsContiguous,
                                    IMG_UINT32        *pui32TilingStride)
{
	OMAPLFB_DEVINFO	*psDevInfo;
	OMAPLFB_BUFFER *psSystemBuffer;

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

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;

	psSystemBuffer = (OMAPLFB_BUFFER *)hBuffer;

	*ppsSysAddr = &psSystemBuffer->sSysAddr;

	*pui32ByteSize = (IMG_UINT32)psDevInfo->sFBInfo.ulBufferSize;

	if (ppvCpuVAddr)
	{
		*ppvCpuVAddr = psDevInfo->sFBInfo.bIs2D ? NULL : psSystemBuffer->sCPUVAddr;
	}

	if (phOSMapInfo)
	{
		*phOSMapInfo = (IMG_HANDLE)0;
	}

	if (pbIsContiguous)
	{
		*pbIsContiguous = !psDevInfo->sFBInfo.bIs2D;
	}

#if defined(CONFIG_DSSCOMP)
	if (psDevInfo->sFBInfo.bIs2D) {
		int i = (psSystemBuffer->sSysAddr.uiAddr - psDevInfo->sFBInfo.psPageList->uiAddr) >> PAGE_SHIFT;
		*ppsSysAddr = psDevInfo->sFBInfo.psPageList + psDevInfo->sFBInfo.ulHeight * i;
	}
#endif

	return PVRSRV_OK;
}

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
	OMAPLFB_DEVINFO	*psDevInfo;
	OMAPLFB_SWAPCHAIN *psSwapChain;
	OMAPLFB_BUFFER *psBuffer;
	IMG_UINT32 i;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32BuffersToSkip;

	UNREFERENCED_PARAMETER(ui32OEMFlags);
	
	
	if(!hDevice
	|| !psDstSurfAttrib
	|| !psSrcSurfAttrib
	|| !ppsSyncData
	|| !phSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;
	
	
	if (psDevInfo->sDisplayInfo.ui32MaxSwapChains == 0)
	{
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	OMAPLFBCreateSwapChainLock(psDevInfo);

	
	if(psDevInfo->psSwapChain != NULL)
	{
		eError = PVRSRV_ERROR_FLIP_CHAIN_EXISTS;
		goto ExitUnLock;
	}
	
	
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

	
	ui32BuffersToSkip = psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers - ui32BufferCount;

	
	if(psDstSurfAttrib->pixelformat != psDevInfo->sDisplayFormat.pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psDevInfo->sDisplayDim.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psDevInfo->sDisplayDim.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psDevInfo->sDisplayDim.ui32Height)
	{
		
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}		

	if(psDstSurfAttrib->pixelformat != psSrcSurfAttrib->pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psSrcSurfAttrib->sDims.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psSrcSurfAttrib->sDims.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psSrcSurfAttrib->sDims.ui32Height)
	{
		
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}		

	UNREFERENCED_PARAMETER(ui32Flags);
	
#if defined(PVR_OMAPFB3_UPDATE_MODE)
	if (!OMAPLFBSetUpdateMode(psDevInfo, PVR_OMAPFB3_UPDATE_MODE))
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't set frame buffer update mode %d\n", __FUNCTION__, psDevInfo->uiFBDevID, PVR_OMAPFB3_UPDATE_MODE);
	}
#endif
	
	psSwapChain = (OMAPLFB_SWAPCHAIN*)OMAPLFBAllocKernelMem(sizeof(OMAPLFB_SWAPCHAIN));
	if(!psSwapChain)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ExitUnLock;
	}

	psBuffer = (OMAPLFB_BUFFER*)OMAPLFBAllocKernelMem(sizeof(OMAPLFB_BUFFER) * ui32BufferCount);
	if(!psBuffer)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorFreeSwapChain;
	}

	psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
	psSwapChain->psBuffer = psBuffer;
	psSwapChain->bNotVSynced = OMAPLFB_TRUE;
	psSwapChain->uiFBDevID = psDevInfo->uiFBDevID;

	
	for(i=0; i<ui32BufferCount-1; i++)
	{
		psBuffer[i].psNext = &psBuffer[i+1];
	}
	
	psBuffer[i].psNext = &psBuffer[0];

	for(i=0; i<ui32BufferCount; i++)
	{
		IMG_UINT32 ui32SwapBuffer = i + ui32BuffersToSkip;
		IMG_UINT32 ui32BufferOffset = ui32SwapBuffer * (IMG_UINT32)psDevInfo->sFBInfo.ulRoundedBufferSize;
		if (psDevInfo->sFBInfo.bIs2D)
		{
			ui32BufferOffset = 0;
		}

		psBuffer[i].psSyncData = ppsSyncData[i];
		psBuffer[i].sSysAddr.uiAddr = psDevInfo->sFBInfo.sSysAddr.uiAddr + ui32BufferOffset;
		psBuffer[i].sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr + ui32BufferOffset;
		psBuffer[i].ulYOffset = ui32BufferOffset / psDevInfo->sFBInfo.ulByteStride;
		if (psDevInfo->sFBInfo.bIs2D)
		{
			psBuffer[i].sSysAddr.uiAddr += ui32SwapBuffer *
				ALIGN((IMG_UINT32)psDevInfo->sFBInfo.ulWidth * psDevInfo->sFBInfo.uiBytesPerPixel, PAGE_SIZE);
		}
		psBuffer[i].psDevInfo = psDevInfo;
		OMAPLFBInitBufferForSwap(&psBuffer[i]);
		psBuffer[i].bvmap_handle = NULL;
	}


	if (OMAPLFBCreateSwapQueue(psSwapChain) != OMAPLFB_OK)
	{ 
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Failed to create workqueue\n", __FUNCTION__, psDevInfo->uiFBDevID);
		eError = PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
		goto ErrorFreeBuffers;
	}

	if (OMAPLFBEnableLFBEventNotification(psDevInfo)!= OMAPLFB_OK)
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
	OMAPLFBDestroySwapQueue(psSwapChain);
ErrorFreeBuffers:
	OMAPLFBFreeKernelMem(psBuffer);
ErrorFreeSwapChain:
	OMAPLFBFreeKernelMem(psSwapChain);
ExitUnLock:
	OMAPLFBCreateSwapChainUnLock(psDevInfo);
	return eError;
}

static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,
                                       IMG_HANDLE hSwapChain)
{
	OMAPLFB_DEVINFO	*psDevInfo;
	OMAPLFB_SWAPCHAIN *psSwapChain;
	OMAPLFB_ERROR eError;
	
	if(!hDevice || !hSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;
	psSwapChain = (OMAPLFB_SWAPCHAIN*)hSwapChain;

	OMAPLFBCreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}

	
	OMAPLFBDestroySwapQueue(psSwapChain);

	eError = OMAPLFBDisableLFBEventNotification(psDevInfo);
	if (eError != OMAPLFB_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't disable framebuffer event notification\n", __FUNCTION__, psDevInfo->uiFBDevID);
	}

	OMAPLFBDeInitBltFBs(psDevInfo);
	gbBvReady = IMG_FALSE;
	bBltReady = IMG_FALSE;

	OMAPLFBFreeKernelMem(psSwapChain->psBuffer);
	OMAPLFBFreeKernelMem(psSwapChain);

	psDevInfo->psSwapChain = NULL;

	OMAPLFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
	(void) OMAPLFBCheckModeAndSync(psDevInfo);

	eError = PVRSRV_OK;

ExitUnLock:
	OMAPLFBCreateSwapChainUnLock(psDevInfo);

	return eError;
}

static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	
	
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE hDevice,
                                      IMG_HANDLE hSwapChain,
                                      IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE hDevice,
                                      IMG_HANDLE hSwapChain,
                                      IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	

	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_UINT32 *pui32BufferCount,
                                 IMG_HANDLE *phBuffer)
{
	OMAPLFB_DEVINFO   *psDevInfo;
	OMAPLFB_SWAPCHAIN *psSwapChain;
	PVRSRV_ERROR eError;
	unsigned i;
	
	
	if(!hDevice 
	|| !hSwapChain
	|| !pui32BufferCount
	|| !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (OMAPLFB_DEVINFO*)hDevice;
	psSwapChain = (OMAPLFB_SWAPCHAIN*)hSwapChain;

	OMAPLFBCreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto Exit;
	}
	
	
	*pui32BufferCount = (IMG_UINT32)psSwapChain->ulBufferCount;
	
	
	for(i=0; i<psSwapChain->ulBufferCount; i++)
	{
		phBuffer[i] = (IMG_HANDLE)&psSwapChain->psBuffer[i];
	}
	
	eError = PVRSRV_OK;

Exit:
	OMAPLFBCreateSwapChainUnLock(psDevInfo);

	return eError;
}

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
	
	

	return PVRSRV_OK;
}

static PVRSRV_ERROR SwapToDCSystem(IMG_HANDLE hDevice,
                                   IMG_HANDLE hSwapChain)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	
	
	return PVRSRV_OK;
}

#if !defined(CONFIG_GCBV)
IMG_BOOL OMAPLFBInitBlt(void)
{
	return IMG_FALSE;
}

OMAPLFB_ERROR OMAPLFBInitBltFBs(OMAPLFB_DEVINFO *psDevInfo)
{
	return OMAPLFB_ERROR_INIT_FAILURE;
}

void OMAPLFBDeInitBltFBs(OMAPLFB_DEVINFO *psDevInfo)
{
}

void OMAPLFBGetBltFBsBvHndl(OMAPLFB_FBINFO *psPVRFBInfo, IMG_UINTPTR_T *ppPhysAddr)
{
	*ppPhysAddr = 0;
}

void OMAPLFBDoBlits(OMAPLFB_DEVINFO *psDevInfo, PDC_MEM_INFO *ppsMemInfos, struct omap_hwc_blit_data *blit_data, IMG_UINT32 ui32NumMemInfos)
{
}
#endif /* CONFIG_GCBV */
static OMAPLFB_BOOL WaitForVSyncSettle(OMAPLFB_DEVINFO *psDevInfo)
{
		unsigned i;
		for(i = 0; i < OMAPLFB_VSYNC_SETTLE_COUNT; i++)
		{
			if (DontWaitForVSync(psDevInfo) || !OMAPLFBWaitForVSync(psDevInfo))
			{
				return OMAPLFB_FALSE;
			}
		}

		return OMAPLFB_TRUE;
}

void OMAPLFBSwapHandler(OMAPLFB_BUFFER *psBuffer)
{
	OMAPLFB_DEVINFO *psDevInfo = psBuffer->psDevInfo;
	OMAPLFB_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	OMAPLFB_BOOL bPreviouslyNotVSynced;

#if defined(SUPPORT_DRI_DRM)
	if (!OMAPLFBAtomicBoolRead(&psDevInfo->sLeaveVT))
#endif
	{
		OMAPLFBFlip(psDevInfo, psBuffer);
	}

	bPreviouslyNotVSynced = psSwapChain->bNotVSynced;
	psSwapChain->bNotVSynced = OMAPLFB_TRUE;


	if (!DontWaitForVSync(psDevInfo))
	{
		OMAPLFB_UPDATE_MODE eMode = OMAPLFBGetUpdateMode(psDevInfo);
		int iBlankEvents = OMAPLFBAtomicIntRead(&psDevInfo->sBlankEvents);

		switch(eMode)
		{
			case OMAPLFB_UPDATE_MODE_AUTO:
				psSwapChain->bNotVSynced = OMAPLFB_FALSE;

				if (bPreviouslyNotVSynced || psSwapChain->iBlankEvents != iBlankEvents)
				{
					psSwapChain->iBlankEvents = iBlankEvents;
					psSwapChain->bNotVSynced = !WaitForVSyncSettle(psDevInfo);
				} else if (psBuffer->ulSwapInterval != 0)
				{
					psSwapChain->bNotVSynced = !OMAPLFBWaitForVSync(psDevInfo);
				}
				break;
#if defined(PVR_OMAPFB3_MANUAL_UPDATE_SYNC_IN_SWAP)
			case OMAPLFB_UPDATE_MODE_MANUAL:
				if (psBuffer->ulSwapInterval != 0)
				{
					(void) OMAPLFBManualSync(psDevInfo);
				}
				break;
#endif
			default:
				break;
		}
	}

	psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psBuffer->hCmdComplete, IMG_TRUE);
}

#if defined(CONFIG_DSSCOMP)


static void dsscomp_proxy_cmdcomplete(void * cookie, int i)
{
	COMMAND_COMPLETE_DATA *psCmdCompleteData =
			(COMMAND_COMPLETE_DATA *)cookie;

	/* XXX: assumes that there is only one display */
	/* LK: Check if we are destroying sync objects.
	   In this case we are now in "late" callback,
	   so we can't allow callback to access sync objects
	   memory. To do this just set zero to count of source
	   and destination sync objects.
	*/
	if (NULL == gapsDevInfo[0]->psSwapChain) {
		psCmdCompleteData->ui32SrcSyncCount = 0;
		psCmdCompleteData->ui32DstSyncCount = 0;
	}
	gapsDevInfo[0]->sPVRJTable.pfnPVRSRVCmdComplete(cookie, i);
}
#endif

static IMG_BOOL ProcessFlipV1(IMG_HANDLE hCmdCookie,
							  OMAPLFB_DEVINFO *psDevInfo,
							  OMAPLFB_SWAPCHAIN *psSwapChain,
							  OMAPLFB_BUFFER *psBuffer,
							  unsigned long ulSwapInterval)
{
	OMAPLFBCreateSwapChainLock(psDevInfo);
	
	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u (PVR Device ID %u): The swap chain has been destroyed\n",
			__FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));
	}
	else
	{
		psBuffer->hCmdComplete = (OMAPLFB_HANDLE)hCmdCookie;
		psBuffer->ulSwapInterval = ulSwapInterval;
#if defined(CONFIG_DSSCOMP)
		if (is_tiler_addr(psBuffer->sSysAddr.uiAddr)) {
			IMG_UINT32 w = psBuffer->psDevInfo->sDisplayDim.ui32Width;
			IMG_UINT32 h = psBuffer->psDevInfo->sDisplayDim.ui32Height;
			struct dsscomp_setup_dispc_data comp = {
				.num_mgrs = 1,
				.mgrs[0].alpha_blending = 1,
				.num_ovls = 1,
				.ovls[0].cfg = {
					.width = w,
					.win.w = w,
					.crop.w = w,
					.height = h,
					.win.h = h,
					.crop.h = h,
					.stride = psBuffer->psDevInfo->sDisplayDim.ui32ByteStride,
					.color_mode = OMAP_DSS_COLOR_ARGB32,
					.enabled = 1,
					.global_alpha = 255,
				},
				.mode = DSSCOMP_SETUP_DISPLAY,
			};
			struct tiler_pa_info *pas[1] = { NULL };
			comp.ovls[0].ba = (u32) psBuffer->sSysAddr.uiAddr;
			dsscomp_gralloc_queue(&comp, pas, true,
					      dsscomp_proxy_cmdcomplete,
					      (void *) psBuffer->hCmdComplete);
		} else
#endif
		{
			OMAPLFBQueueBufferForSwap(psSwapChain, psBuffer);
		}
	}

	OMAPLFBCreateSwapChainUnLock(psDevInfo);

	return IMG_TRUE;
}

#if defined(CONFIG_DSSCOMP) && defined(CONFIG_TI_TILER)
int meminfo_idx_valid(unsigned int meminfo_ix, int num_meminfos)
{
	if (meminfo_ix < 0 || meminfo_ix >= num_meminfos) {
		WARN(1, "%s: Invalid meminfo index %d, max %d\n",
				__func__, meminfo_ix, num_meminfos);
		return 0;
	}
	return 1;
}

static IMG_BOOL ProcessFlipV2(IMG_HANDLE hCmdCookie,
                              OMAPLFB_DEVINFO *psDevInfo,
                              PDC_MEM_INFO *ppsMemInfos,
                              IMG_UINT32 ui32NumMemInfos,
                              struct omap_hwc_data *psHwcData,
                              IMG_UINT32 uiHwcDataSz)
{
	struct tiler_pa_info *apsTilerPAs[5];
	IMG_UINT32 i, k;
	struct {
		IMG_UINTPTR_T uiAddr;
		IMG_UINTPTR_T uiUVAddr;
		struct tiler_pa_info *psTilerInfo;
	} asMemInfo[5];

	/* Framebuffer info just used to get FB geometry, the address to
	 * use for blitting (dst buffer) is the first meminfo
	 */
	int rgz_items;
	int calcsz;
	struct dsscomp_setup_dispc_data *psDssData = &(psHwcData->dsscomp_data);
	int iMemIdx = 0;
	int iUseBltFB;

	if (uiHwcDataSz <= offsetof(struct omap_hwc_data, blit_data))
		rgz_items = 0;
	else
		rgz_items = psHwcData->blit_data.rgz_items;


	psDssData = &(psHwcData->dsscomp_data);
	calcsz = sizeof(*psHwcData) +
		(sizeof(struct rgz_blt_entry) * rgz_items);
	iUseBltFB = psHwcData->blit_data.rgz_flags & HWC_BLT_FLAG_USE_FB;

	if (!iUseBltFB && rgz_items > 0) {
		WARN(1, "Trying to blit without a pipe configured for the blit FB");
		return IMG_FALSE;
	}

	if (rgz_items > 0 && !gbBvInterfacePresent)
	{
		/* We cannot blit if BV GC2D is not present!, likely a bug */
		WARN(1, "Trying to blit when BV GC2D is not present");
		rgz_items = 0; /* Prevent blits */
	}

	if (iUseBltFB && !bBltReady)
	{
		/* Defer allocation and mapping of blit buffers */
		if (OMAPLFBInitBltFBs(psDevInfo) != OMAPLFB_OK)
		{
			WARN(1, "Could not initialize blit FBs");
			return IMG_FALSE;
		}

		bBltReady = IMG_TRUE;
	}

	memset(asMemInfo, 0, sizeof(asMemInfo));

	/* Check the size of private data along with the blit operations */
	if (uiHwcDataSz != calcsz)
	{
		WARN(1, "invalid size of private data (%d vs %d)",
		     uiHwcDataSz, calcsz);
	}

	if(psDssData->num_ovls == 0 || ui32NumMemInfos == 0)
	{
		WARN(1, "must have at least one layer");
		return IMG_FALSE;
	}

	if (iUseBltFB)
	{
		iMemIdx++;
		/* Increment the Blt framebuffer and get new address */
		OMAPLFBGetBltFBsBvHndl(&psDevInfo->sFBInfo, &asMemInfo[0].uiAddr);
	}

	for (i = 0, k = iMemIdx; i < ui32NumMemInfos && k < ARRAY_SIZE(apsTilerPAs) &&
		k < psDssData->num_ovls; i++, k++) {

		struct tiler_pa_info *psTilerInfo;
		IMG_CPU_VIRTADDR virtAddr;
		IMG_CPU_PHYADDR phyAddr;
		IMG_UINT32 ui32NumPages;
		IMG_SIZE_T uByteSize;
		int j;

		psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetByteSize(ppsMemInfos[i], &uByteSize);
		ui32NumPages = (uByteSize + PAGE_SIZE - 1) >> PAGE_SHIFT;

		psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], 0, &phyAddr);

#ifdef CONFIG_DSSCOMP_ADAPT
		{
			OMAPLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
			if ( i==k		//no Blit
				&& psPVRFBInfo->sSysAddr.uiAddr<=phyAddr.uiAddr
				&& phyAddr.uiAddr<(psPVRFBInfo->sSysAddr.uiAddr + psPVRFBInfo->ulFBSize) )
			{
//[LGE_UPDATE_S] OSP fb0 read issue Fixed jeonghoon.cho@lge.com 0415
				struct fb_info *fb = psDevInfo->psLINFBInfo;
//[LGE_UPDATE_E] OSP fb0 read issue Fixed jeonghoon.cho@lge.com
				psDssData->ovls[i].addressing = OMAP_DSS_BUFADDR_FB;
				psDssData->ovls[i].ba = phyAddr.uiAddr - psPVRFBInfo->sSysAddr.uiAddr;
//[LGE_UPDATE_S] OSP fb0 read issue Fixed jeonghoon.cho@lge.com 0415
				if (fb!=NULL )
				{
					unsigned offset = (unsigned)psDssData->ovls[i].ba;
					unsigned line_lenght = fb->fix.line_length;
					unsigned bpp = fb->var.bits_per_pixel / 8;

					//change offset to give information to fb capturing apps.
					fb->var.yoffset = offset / line_lenght;
					fb->var.xoffset = (offset % line_lenght) / bpp;
				}
//[LGE_UPDATE_E] OSP fb0 read issue Fixed jeonghoon.cho@lge.com
				continue;
			}

		}
#endif

		/* TILER buffers do not need meminfos */
		if(is_tiler_addr((u32)phyAddr.uiAddr))
		{
			asMemInfo[k].uiAddr = phyAddr.uiAddr + ppsMemInfos[i]->planeOffsets[0];
			if (tiler_fmt((u32)phyAddr.uiAddr) == TILFMT_8BIT)
			{
				IMG_UINT32 uvOffset = ppsMemInfos[i]->planeOffsets[1] ?
					ppsMemInfos[i]->planeOffsets[1] : ((uByteSize * 2) / 3);
				psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i],
						uvOffset, &phyAddr);
				asMemInfo[k].uiUVAddr = phyAddr.uiAddr;
			}
			continue;
		}

		if (phyAddr.uiAddr >= psDevInfo->psLINFBInfo->fix.smem_start &&
		    phyAddr.uiAddr < (psDevInfo->psLINFBInfo->fix.smem_start + psDevInfo->psLINFBInfo->fix.smem_len))
		{
			asMemInfo[k].uiAddr = phyAddr.uiAddr;
			continue;
		}
		/* normal gralloc layer */
		psTilerInfo = kzalloc(sizeof(*psTilerInfo), GFP_KERNEL);
		if(!psTilerInfo)
		{
			continue;
		}

		psTilerInfo->mem = kzalloc(sizeof(*psTilerInfo->mem) * ui32NumPages, GFP_KERNEL);
		if(!psTilerInfo->mem)
		{
			kfree(psTilerInfo);
			continue;
		}

		psTilerInfo->num_pg = ui32NumPages;
		psTilerInfo->memtype = TILER_MEM_USING;
		for(j = 0; j < ui32NumPages; j++)
		{
			psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], j << PAGE_SHIFT, &phyAddr);
			psTilerInfo->mem[j] = (u32)phyAddr.uiAddr;
		}

		/* need base address for in-page offset */
		psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuVAddr(ppsMemInfos[i], &virtAddr);
		asMemInfo[k].uiAddr = (IMG_UINTPTR_T) virtAddr;
		asMemInfo[k].psTilerInfo = psTilerInfo;
	}

	for(i = 0; i < psDssData->num_ovls; i++)
	{
		unsigned int ix;
		apsTilerPAs[i] = NULL;

		/* only supporting Post2, cloned and fbmem layers */
		if (psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_LAYER_IX &&
		    psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_OVL_IX &&
		    psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_FB)
			psDssData->ovls[i].cfg.enabled = false;

		if (psDssData->ovls[i].addressing != OMAP_DSS_BUFADDR_LAYER_IX)
			continue;

		/* Post2 layers */
		ix = psDssData->ovls[i].ba;
		if (ix >= k)
		{
			WARN(1, "Invalid Post2 layer (%u)", ix);
			psDssData->ovls[i].cfg.enabled = false;
			continue;
		}

		psDssData->ovls[i].addressing = OMAP_DSS_BUFADDR_DIRECT;
		psDssData->ovls[i].ba = (u32) asMemInfo[ix].uiAddr;
		psDssData->ovls[i].uv = (u32) asMemInfo[ix].uiUVAddr;
		apsTilerPAs[i] = asMemInfo[ix].psTilerInfo;
	}

	if (rgz_items > 0)
	{
		OMAPLFBDoBlits(psDevInfo, ppsMemInfos, &psHwcData->blit_data, ui32NumMemInfos);
	}

	dsscomp_gralloc_queue(psDssData, apsTilerPAs, false,
						  dsscomp_proxy_cmdcomplete,
						  (void *)hCmdCookie);

	for(i = 0; i < k; i++)
	{
		tiler_pa_free(apsTilerPAs[i]);
	}

	return IMG_TRUE;
}

#endif

static IMG_BOOL ProcessFlip(IMG_HANDLE  hCmdCookie,
                            IMG_UINT32  ui32DataSize,
                            IMG_VOID   *pvData)
{
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	OMAPLFB_DEVINFO *psDevInfo;

	if(!hCmdCookie || !pvData)
	{
		return IMG_FALSE;
	}

	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND*)pvData;

	if (psFlipCmd == IMG_NULL)
	{
		return IMG_FALSE;
	}

	psDevInfo = (OMAPLFB_DEVINFO*)psFlipCmd->hExtDevice;

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
#if defined(CONFIG_DSSCOMP) && defined(CONFIG_TI_TILER)
		DISPLAYCLASS_FLIP_COMMAND2 *psFlipCmd2;
		psFlipCmd2 = (DISPLAYCLASS_FLIP_COMMAND2 *)pvData;
		return ProcessFlipV2(hCmdCookie,
							 psDevInfo,
							 psFlipCmd2->ppsMemInfos,
							 psFlipCmd2->ui32NumMemInfos,
							 psFlipCmd2->pvPrivData,
							 psFlipCmd2->ui32PrivDataLength);
#else
		BUG();
#endif
	}
}

#if defined(CONFIG_DSSCOMP)

static OMAPLFB_ERROR OMAPLFBInitIonOmap(OMAPLFB_DEVINFO *psDevInfo,
                                        struct fb_info *psLINFBInfo,
                                        OMAPLFB_FBINFO *psPVRFBInfo)
{
	int n;
	int iMaxSwapChainBuffs;
	int res;
	int i, x, y, w;
	ion_phys_addr_t phys;
	size_t size;
	struct tiler_view_t view;

	struct omap_ion_tiler_alloc_data sAllocData = {
		.w = ALIGN(psLINFBInfo->var.xres, PAGE_SIZE / (psLINFBInfo->var.bits_per_pixel / 8)),
		.h = psLINFBInfo->var.yres,
		.fmt = psLINFBInfo->var.bits_per_pixel == 16 ? TILER_PIXEL_FMT_16BIT : TILER_PIXEL_FMT_32BIT,
		.flags = 0,
		.token = 0,
		.out_align = PAGE_SIZE
	};
	unsigned uiFBDevID = psDevInfo->uiFBDevID;
	struct sgx_omaplfb_config *psFBPlatConfig = GetFBPlatConfig(uiFBDevID);

	if (!psFBPlatConfig->swap_chain_length)
	{
		/* Set a default swap chain length if it's not present in the platform data */
		iMaxSwapChainBuffs = 2;
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Swap chain length missing in "
			"platform data, defaulting to %d\n", __FUNCTION__, psDevInfo->uiFBDevID,
			iMaxSwapChainBuffs);
	}
	else
	{
		iMaxSwapChainBuffs = psFBPlatConfig->swap_chain_length;
	}

	if (psFBPlatConfig->tiler2d_buffers < iMaxSwapChainBuffs)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Trying to use %d tiler "
			"buffers which is less than the swap chain length of %d, maximum "
			"swap chain length will be set to %d\n", __FUNCTION__, psDevInfo->uiFBDevID,
			psFBPlatConfig->tiler2d_buffers, iMaxSwapChainBuffs, psFBPlatConfig->tiler2d_buffers);
		iMaxSwapChainBuffs = psFBPlatConfig->tiler2d_buffers;
	}

	psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers = iMaxSwapChainBuffs;
	n = psFBPlatConfig->tiler2d_buffers;

	printk(KERN_DEBUG DRIVER_PREFIX
		": %s: Device %u: Requesting %d TILER 2D framebuffers\n", __FUNCTION__, uiFBDevID, n);
	sAllocData.w *= n;

	psPVRFBInfo->uiBytesPerPixel = psLINFBInfo->var.bits_per_pixel >> 3;
	psPVRFBInfo->bIs2D = OMAPLFB_TRUE;
	res = omap_ion_nonsecure_tiler_alloc(gpsIONClient, &sAllocData);
	if (res < 0)
	{
		printk(KERN_ERR DRIVER_PREFIX
			" %s: Device %u: Could not allocate 2D framebuffer(%d)\n", __FUNCTION__, uiFBDevID, res);
		return OMAPLFB_ERROR_INIT_FAILURE;
	}

	ion_phys(gpsIONClient, sAllocData.handle, &phys, &size);
	psPVRFBInfo->sSysAddr.uiAddr = phys;
	psPVRFBInfo->sCPUVAddr = 0;

	psPVRFBInfo->ulWidth = psLINFBInfo->var.xres;
	psPVRFBInfo->ulHeight = psLINFBInfo->var.yres;
	psPVRFBInfo->ulByteStride = PAGE_ALIGN(psPVRFBInfo->ulWidth * psPVRFBInfo->uiBytesPerPixel);
	psPVRFBInfo->ulBufferSize = psPVRFBInfo->ulHeight * psPVRFBInfo->ulByteStride;
	psPVRFBInfo->ulRoundedBufferSize = psPVRFBInfo->ulBufferSize;
	w = psPVRFBInfo->ulByteStride >> PAGE_SHIFT;

	/* this is an "effective" FB size to get correct number of buffers */
	psPVRFBInfo->ulFBSize = sAllocData.h * n * psPVRFBInfo->ulByteStride;
	psPVRFBInfo->psPageList = kzalloc(w * n * psPVRFBInfo->ulHeight * sizeof(*psPVRFBInfo->psPageList), GFP_KERNEL);
	if (!psPVRFBInfo->psPageList)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Could not allocate page list\n", __FUNCTION__, psDevInfo->uiFBDevID);
		ion_free(gpsIONClient, sAllocData.handle);
		return OMAPLFB_ERROR_INIT_FAILURE;
	}
	psPVRFBInfo->psIONHandle = sAllocData.handle;

	tilview_create(&view, phys, psDevInfo->sFBInfo.ulWidth, psDevInfo->sFBInfo.ulHeight);
	for(i=0; i<n; i++)
	{
		for(y=0; y<psDevInfo->sFBInfo.ulHeight; y++)
		{
			for(x=0; x<w; x++)
			{
				psPVRFBInfo->psPageList[i * psDevInfo->sFBInfo.ulHeight * w + y * w + x].uiAddr =
					phys + view.v_inc * y + ((x + i * w) << PAGE_SHIFT);
			}
		}
	}
	return OMAPLFB_OK;
}
#endif

static OMAPLFB_ERROR OMAPLFBInitFBVRAM(OMAPLFB_DEVINFO *psDevInfo,
                                        struct fb_info *psLINFBInfo,
                                        OMAPLFB_FBINFO *psPVRFBInfo)
{
	struct sgx_omaplfb_config *psFBPlatConfig = GetFBPlatConfig(psDevInfo->uiFBDevID);
	unsigned long FBSize = psLINFBInfo->fix.smem_len;
	unsigned long ulLCM;
	int iMaxSwapChainBuffs;
	IMG_UINT32 ui32FBAvailableBuffs;

	/* Check if there is VRAM reserved for this FB */
	if (FBSize == 0 || psLINFBInfo->fix.line_length == 0)
	{
		return OMAPLFB_ERROR_INVALID_DEVICE;
	}

	/* Fail to init this DC device if vram buffers are not set */
	if (!psFBPlatConfig->vram_buffers)
	{
		return OMAPLFB_ERROR_INVALID_PARAMS;
	}

	if (!psFBPlatConfig->swap_chain_length)
	{
		/* Set a default swap chain length if it's not present in the platform data */
		iMaxSwapChainBuffs = 2;
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Swap chain length missing in "
			"platform data, defaulting to %d\n", __FUNCTION__, psDevInfo->uiFBDevID,
			iMaxSwapChainBuffs);
	}
	else
	{
		iMaxSwapChainBuffs = psFBPlatConfig->swap_chain_length;
	}

	if (psFBPlatConfig->vram_buffers < iMaxSwapChainBuffs)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Trying to use %d vram "
			"buffers which is less than the swap chain length of %d, maximum "
			"swap chain length will be set to %d\n", __FUNCTION__, psDevInfo->uiFBDevID,
			psFBPlatConfig->vram_buffers, iMaxSwapChainBuffs, psFBPlatConfig->vram_buffers);
		iMaxSwapChainBuffs = psFBPlatConfig->vram_buffers;
	}

	ulLCM = LCM(psLINFBInfo->fix.line_length, OMAPLFB_PAGE_SIZE);
	psPVRFBInfo->sSysAddr.uiAddr = psLINFBInfo->fix.smem_start;
	psPVRFBInfo->sCPUVAddr = psLINFBInfo->screen_base;
	psPVRFBInfo->ulWidth = psLINFBInfo->var.xres;
	psPVRFBInfo->ulHeight = psLINFBInfo->var.yres;
	psPVRFBInfo->ulByteStride =  psLINFBInfo->fix.line_length;
	psPVRFBInfo->ulFBSize = FBSize;
	psPVRFBInfo->bIs2D = OMAPLFB_FALSE;
	psPVRFBInfo->psPageList = IMG_NULL;
	psPVRFBInfo->ulBufferSize = psPVRFBInfo->ulHeight * psPVRFBInfo->ulByteStride;
	psPVRFBInfo->ulRoundedBufferSize = RoundUpToMultiple(psPVRFBInfo->ulBufferSize, ulLCM);
	ui32FBAvailableBuffs = (IMG_UINT32)(psDevInfo->sFBInfo.ulFBSize / psDevInfo->sFBInfo.ulRoundedBufferSize);

	if (!ui32FBAvailableBuffs)
	{
		printk(KERN_ERR DRIVER_PREFIX " %s: Device %u: Not enough vram to init swap "
			"chain buffers\n", __FUNCTION__, psDevInfo->uiFBDevID);
		return OMAPLFB_ERROR_INIT_FAILURE;
	}
	else if (ui32FBAvailableBuffs < psFBPlatConfig->vram_buffers)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Not enough vram to hold "
			"%d buffers (available %d), swap chain length will be set to %d\n",
			__FUNCTION__, psDevInfo->uiFBDevID, iMaxSwapChainBuffs, ui32FBAvailableBuffs,
			ui32FBAvailableBuffs);
		iMaxSwapChainBuffs = ui32FBAvailableBuffs;
	}
	else
	{
		iMaxSwapChainBuffs = psFBPlatConfig->vram_buffers;
	}

	psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers = iMaxSwapChainBuffs;

	printk(KERN_DEBUG DRIVER_PREFIX ": %s: Device %u: Using %d VRAM framebuffers\n", __FUNCTION__,
		psDevInfo->uiFBDevID, iMaxSwapChainBuffs);

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual width: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.xres_virtual));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual height: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.yres_virtual));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: LCM of stride and page size: %lu\n",
			psDevInfo->uiFBDevID, ulLCM));

	return OMAPLFB_OK;
}

static OMAPLFB_ERROR OMAPLFBInitFBDev(OMAPLFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo;
	struct module *psLINFBOwner;
	OMAPLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	OMAPLFB_ERROR eError = OMAPLFB_ERROR_GENERIC;
	unsigned long FBSize;
	unsigned long ulLCM;
	unsigned uiFBDevID = psDevInfo->uiFBDevID;
	struct sgx_omaplfb_config *psFBPlatConfig;

	OMAPLFB_CONSOLE_LOCK();

	psLINFBInfo = registered_fb[uiFBDevID];
	if (psLINFBInfo == NULL)
	{
		eError = OMAPLFB_ERROR_INVALID_DEVICE;
		goto ErrorRelSem;
	}

	FBSize = psLINFBInfo->fix.smem_len;

	
	if (FBSize == 0 || psLINFBInfo->fix.line_length == 0)
	{
		eError = OMAPLFB_ERROR_INVALID_DEVICE;
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

	ulLCM = LCM(psLINFBInfo->fix.line_length, OMAPLFB_PAGE_SIZE);
	/*
	 * Abort registering this DC device if no platform data found. This
	 * shouldn't happen since the FB index must be valid at this point.
	 */
	psFBPlatConfig = GetFBPlatConfig(uiFBDevID);
	if (!psFBPlatConfig)
	{
		eError = OMAPLFB_ERROR_INVALID_PARAMS;
		goto ErrorModPut;
	}

#if defined(CONFIG_DSSCOMP)
	if (psFBPlatConfig->tiler2d_buffers)
	{
		/* Use ION to create the flip chain buffers with TILER */
		eError = OMAPLFBInitIonOmap(psDevInfo, psLINFBInfo, psPVRFBInfo);
		if (eError != OMAPLFB_OK)
		{
			goto ErrorModPut;
		}
	}
	else
#endif
	{
		/* Fall back to allocate flip chain buffers with VRAM */
		eError = OMAPLFBInitFBVRAM(psDevInfo, psLINFBInfo, psPVRFBInfo);
		if (eError != OMAPLFB_OK)
		{
			goto ErrorModPut;
		}
	}

	OMAPLFBPrintInfo(psDevInfo);
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer physical address: %p\n",
			psDevInfo->uiFBDevID, (void *)psPVRFBInfo->sSysAddr.uiAddr));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual address: %p\n",
			psDevInfo->uiFBDevID, (void *)psPVRFBInfo->sCPUVAddr));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer size: %lu\n",
			psDevInfo->uiFBDevID, psPVRFBInfo->ulFBSize));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer width: %lu\n",
			psDevInfo->uiFBDevID, psPVRFBInfo->ulWidth));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer height: %lu\n",
			psDevInfo->uiFBDevID, psPVRFBInfo->ulHeight));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer stride: %lu\n",
			psDevInfo->uiFBDevID, psPVRFBInfo->ulByteStride));

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
			printk(KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
		}
	}
	else if(psLINFBInfo->var.bits_per_pixel == 32)
	{
		if((psLINFBInfo->var.red.length == 8) &&
			(psLINFBInfo->var.green.length == 8) && 
			(psLINFBInfo->var.blue.length == 8) && 
			(psLINFBInfo->var.red.offset == 16) &&
			(psLINFBInfo->var.green.offset == 8) && 
			(psLINFBInfo->var.blue.offset == 0) && 
			(psLINFBInfo->var.red.msb_right == 0))
		{
			psPVRFBInfo->ePixelFormat = PVRSRV_PIXEL_FORMAT_ARGB8888;
		}
		else
		{
			printk(KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
		}
	}	
	else
	{
		printk(KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
	}

	psDevInfo->sFBInfo.ulPhysicalWidthmm =
		((int)psLINFBInfo->var.width  > 0) ? psLINFBInfo->var.width  : 90;

	psDevInfo->sFBInfo.ulPhysicalHeightmm =
		((int)psLINFBInfo->var.height > 0) ? psLINFBInfo->var.height : 54;

	
	psDevInfo->sFBInfo.sSysAddr.uiAddr = psPVRFBInfo->sSysAddr.uiAddr;
	psDevInfo->sFBInfo.sCPUVAddr = psPVRFBInfo->sCPUVAddr;

	eError = OMAPLFB_OK;
	goto ErrorRelSem;

ErrorModPut:
	module_put(psLINFBOwner);
ErrorRelSem:
	OMAPLFB_CONSOLE_UNLOCK();

	return eError;
}

static void OMAPLFBDeInitFBDev(OMAPLFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo = psDevInfo->psLINFBInfo;
	OMAPLFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	struct module *psLINFBOwner;

	OMAPLFB_CONSOLE_LOCK();

	psLINFBOwner = psLINFBInfo->fbops->owner;

#if defined(CONFIG_DSSCOMP)
	kfree(psPVRFBInfo->psPageList);
	if (psPVRFBInfo->psIONHandle)
	{
		ion_free(gpsIONClient, psPVRFBInfo->psIONHandle);
	}
	if (psPVRFBInfo->psBltFBsIonHndl)
	{
		ion_free(gpsIONClient, psPVRFBInfo->psBltFBsIonHndl);
	}
	if (psPVRFBInfo->psBltFBsBvHndl)
	{
		int i;
		for (i = 0; i < psPVRFBInfo->psBltFBsNo; i++)
		{
			if (psPVRFBInfo->psBltFBsBvHndl[i])
			{
				kfree(psPVRFBInfo->psBltFBsBvHndl[i]);
			}
		}
		kfree(psPVRFBInfo->psBltFBsBvHndl);
		kfree(psPVRFBInfo->psBltFBsBvPhys);
	}
#endif
	if (psLINFBInfo->fbops->fb_release != NULL) 
	{
		(void) psLINFBInfo->fbops->fb_release(psLINFBInfo, 0);
	}

	module_put(psLINFBOwner);

	OMAPLFB_CONSOLE_UNLOCK();
}

static OMAPLFB_DEVINFO *OMAPLFBInitDev(unsigned uiFBDevID)
{
	PFN_CMD_PROC	 	pfnCmdProcList[OMAPLFB_COMMAND_COUNT];
	IMG_UINT32		aui32SyncCountList[OMAPLFB_COMMAND_COUNT][2];
	OMAPLFB_DEVINFO		*psDevInfo = NULL;

	
	psDevInfo = (OMAPLFB_DEVINFO *)OMAPLFBAllocKernelMem(sizeof(OMAPLFB_DEVINFO));

	if(psDevInfo == NULL)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: Couldn't allocate device information structure\n", __FUNCTION__, uiFBDevID);

		goto ErrorExit;
	}

	
	memset(psDevInfo, 0, sizeof(OMAPLFB_DEVINFO));

	psDevInfo->uiFBDevID = uiFBDevID;

	
	if(!(*gpfnGetPVRJTable)(&psDevInfo->sPVRJTable))
	{
		goto ErrorFreeDevInfo;
	}

	
	if(OMAPLFBInitFBDev(psDevInfo) != OMAPLFB_OK)
	{
		
		goto ErrorFreeDevInfo;
	}

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

	
	psDevInfo->sSystemBuffer.sSysAddr = psDevInfo->sFBInfo.sSysAddr;
	psDevInfo->sSystemBuffer.sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr;
	psDevInfo->sSystemBuffer.psDevInfo = psDevInfo;

	OMAPLFBInitBufferForSwap(&psDevInfo->sSystemBuffer);

	

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
	psDevInfo->sDCJTable.pfnSwapToDCSystem = SwapToDCSystem;
	psDevInfo->sDCJTable.pfnSetDCState = SetDCState;

	
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
	
	
	pfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;

	
	aui32SyncCountList[DC_FLIP_COMMAND][0] = 0; 
	if (gbBvInterfacePresent)
	{
		aui32SyncCountList[DC_FLIP_COMMAND][1] = 32;
	}
	else
	{
		aui32SyncCountList[DC_FLIP_COMMAND][1] = 10;
	}

	



	if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList(psDevInfo->uiPVRDevID,
			&pfnCmdProcList[0],
			aui32SyncCountList,
			OMAPLFB_COMMAND_COUNT) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: Couldn't register command processing functions with PVR Services\n", __FUNCTION__, uiFBDevID);
		goto ErrorUnregisterDevice;
	}

	OMAPLFBCreateSwapChainLockInit(psDevInfo);

	OMAPLFBAtomicBoolInit(&psDevInfo->sBlanked, OMAPLFB_FALSE);
	OMAPLFBAtomicIntInit(&psDevInfo->sBlankEvents, 0);
	OMAPLFBAtomicBoolInit(&psDevInfo->sFlushCommands, OMAPLFB_FALSE);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	OMAPLFBAtomicBoolInit(&psDevInfo->sEarlySuspendFlag, OMAPLFB_FALSE);
#endif
#if defined(SUPPORT_DRI_DRM)
	OMAPLFBAtomicBoolInit(&psDevInfo->sLeaveVT, OMAPLFB_FALSE);
#endif
	return psDevInfo;

ErrorUnregisterDevice:
	(void)psDevInfo->sPVRJTable.pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID);
ErrorDeInitFBDev:
	OMAPLFBDeInitFBDev(psDevInfo);
ErrorFreeDevInfo:
	OMAPLFBFreeKernelMem(psDevInfo);
ErrorExit:
	return NULL;
}

OMAPLFB_ERROR OMAPLFBInit(void)
{
	unsigned uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();
	unsigned i;
	unsigned uiDevicesFound = 0;

	if(OMAPLFBGetLibFuncAddr ("PVRGetDisplayClassJTable", &gpfnGetPVRJTable) != OMAPLFB_OK)
	{
		return OMAPLFB_ERROR_INIT_FAILURE;
	}

	gbBvInterfacePresent = OMAPLFBInitBlt();

	if (!gbBvInterfacePresent)
	{
		printk(KERN_INFO DRIVER_PREFIX "%s: Blitsville gc2d "
			"not present, blits disabled\n", __func__);
	}
	for(i = uiMaxFBDevIDPlusOne; i-- != 0;)
	{
		OMAPLFB_DEVINFO *psDevInfo = OMAPLFBInitDev(i);

		if (psDevInfo != NULL)
		{
			
			OMAPLFBSetDevInfoPtr(psDevInfo->uiFBDevID, psDevInfo);
			uiDevicesFound++;
		}
	}

	return (uiDevicesFound != 0) ? OMAPLFB_OK : OMAPLFB_ERROR_INIT_FAILURE;
}

static OMAPLFB_BOOL OMAPLFBDeInitDev(OMAPLFB_DEVINFO *psDevInfo)
{
	PVRSRV_DC_DISP2SRV_KMJTABLE *psPVRJTable = &psDevInfo->sPVRJTable;

	OMAPLFBCreateSwapChainLockDeInit(psDevInfo);

	OMAPLFBAtomicBoolDeInit(&psDevInfo->sBlanked);
	OMAPLFBAtomicIntDeInit(&psDevInfo->sBlankEvents);
	OMAPLFBAtomicBoolDeInit(&psDevInfo->sFlushCommands);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	OMAPLFBAtomicBoolDeInit(&psDevInfo->sEarlySuspendFlag);
#endif
#if defined(SUPPORT_DRI_DRM)
	OMAPLFBAtomicBoolDeInit(&psDevInfo->sLeaveVT);
#endif
	psPVRJTable = &psDevInfo->sPVRJTable;

	if (psPVRJTable->pfnPVRSRVRemoveCmdProcList (psDevInfo->uiPVRDevID, OMAPLFB_COMMAND_COUNT) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't unregister command processing functions\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return OMAPLFB_FALSE;
	}

	
	if (psPVRJTable->pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't remove device from PVR Services\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return OMAPLFB_FALSE;
	}
	
	OMAPLFBDeInitFBDev(psDevInfo);

	OMAPLFBSetDevInfoPtr(psDevInfo->uiFBDevID, NULL);

	
	OMAPLFBFreeKernelMem(psDevInfo);

	return OMAPLFB_TRUE;
}

OMAPLFB_ERROR OMAPLFBDeInit(void)
{
	unsigned uiMaxFBDevIDPlusOne = OMAPLFBMaxFBDevIDPlusOne();
	unsigned i;
	OMAPLFB_BOOL bError = OMAPLFB_FALSE;

	for(i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		OMAPLFB_DEVINFO *psDevInfo = OMAPLFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			bError |= !OMAPLFBDeInitDev(psDevInfo);
		}
	}

	return (bError) ? OMAPLFB_ERROR_INIT_FAILURE : OMAPLFB_OK;
}

