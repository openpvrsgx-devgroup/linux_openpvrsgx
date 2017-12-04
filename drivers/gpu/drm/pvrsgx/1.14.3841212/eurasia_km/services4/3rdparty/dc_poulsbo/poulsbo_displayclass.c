/*************************************************************************/ /*!
@Title          Poulsbo driver display functions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

/**************************************************************************
 The 3rd party driver is a specification of an API to integrate the IMG POWERVR
 Services driver with 3rd Party display hardware.  It is NOT a specification for
 a display controller driver, rather a specification to extend the API for a
 pre-existing driver for the display hardware.
 
 The 3rd party driver interface provides IMG POWERVR client drivers (e.g. PVR2D)
 with an API abstraction of the system's underlying display hardware, allowing
 the client drivers to indirectly control the display hardware and access its
 associated memory.
 
 Functions of the API include
 - query primary surface attributes (width, height, stride, pixel format, CPU
	 physical and virtual address)
 - swap/flip chain creation and subsequent query of surface attributes
 - asynchronous display surface flipping, taking account of asynchronous read
 (flip) and write (render) operations to the display surface
 
 Note: having queried surface attributes the client drivers are able to map the
 display memory to any IMG POWERVR Services device by calling
 PVRSRVMapDeviceClassMemory with the display surface handle.
 
 This code is intended to be an example of how a pre-existing display driver may
 be extended to support the 3rd Party Display interface to POWERVR Services
 - IMG is not providing a display driver implementation.
 **************************************************************************/

#include "dc_poulsbo.h"

#include <linux/string.h>
#include <linux/kernel.h>

/* Kernel services is a kernel module and must be loaded first.
   The display controller driver is also a kernel module and must be loaded 
   after the PVR services module. The display controller driver should be able 
   to retrieve the address of the services PVRGetDisplayClassJTable from 
   (the already loaded) kernel services module. */

/* The number of command types to register with PowerVR Services (1, flip) */
#define PVRPSB_COMMAND_COUNT	(1)

/* Device PCI device IDs */
static const IMG_UINT16 aui16PciDeviceIdMap[][2] = 
{
	{0x8108, PSB_POULSBO},
	{0x0BE0, PSB_CEDARVIEW},
	{0x0BE1, PSB_CEDARVIEW},
	{0x0BE2, PSB_CEDARVIEW},
	{   0x0, PSB_UNKNOWN}
};

static IMG_VOID *gpvAnchor;


/*******************************************************************************
 * Helper functions
 ******************************************************************************/

static PVRPSB_DEVICE GetDevice(IMG_UINT16 ui16DevId)
{
	IMG_UINT32 ui32I;

	for (ui32I = 0; aui16PciDeviceIdMap[ui32I][0] != 0; ui32I++)
	{
		if (aui16PciDeviceIdMap[ui32I][0] == ui16DevId)
		{
			return (PVRPSB_DEVICE)aui16PciDeviceIdMap[ui32I][1];
		}
	}

	return PSB_UNKNOWN;
}

static IMG_UINT32 GetMemRegionSize(PVRPSB_DEVINFO *psDevInfo, IMG_UINT32 ui32Reg)
{
	IMG_UINT32 ui32SavedValue;
	IMG_UINT32 ui32AddressRange;

	ui32SavedValue = PVROSPciReadDWord(psDevInfo, ui32Reg);

	PVROSPciWriteDWord(psDevInfo, ui32Reg, 0xFFFFFFFF);

	ui32AddressRange = PVROSPciReadDWord(psDevInfo, ui32Reg);

	PVROSPciWriteDWord(psDevInfo, ui32Reg, ui32SavedValue);

	return ~(ui32AddressRange & 0xFFFFFFF0) + 1;
}

/* Advance an index into the Vsync flip array */
static void AdvanceFlipIndex(PVRPSB_SWAPCHAIN *psSwapChain, IMG_UINT32 *pui32Index)
{
	(*pui32Index)++;
	
	if (*pui32Index >= psSwapChain->ui32BufferCount)
	{
		*pui32Index = 0;
	}
}

/* Flip to buffer function */
PSB_ERROR PVRPSBFlip(PVRPSB_DEVINFO *psDevInfo, PVRPSB_BUFFER *psBuffer)
{
	if (psDevInfo != NULL)
	{
#if defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
		if (psDevInfo->bLeaveVT == PSB_FALSE)
#endif
		{
			IMG_UINT32 ui32Offset = psBuffer->sDevVAddr.uiAddr - psDevInfo->sGTTInfo.sGMemDevVAddr.uiAddr;

			if (psDevInfo->eActivePipe == PSB_PIPE_A)
			{
				PVROSWriteMMIOReg(psDevInfo, PVRPSB_DSPALINOFF, ui32Offset);
			}
			else
			{
				PVROSWriteMMIOReg(psDevInfo, PVRPSB_DSPBLINOFF, ui32Offset);
			}

			psDevInfo->psCurrentBuffer = psBuffer;
		}

		return PSB_OK;
	}
	else
	{
		return PSB_ERROR_INVALID_PARAMS;
	}
}

#if defined(SYS_USING_INTERRUPTS)
void PVRPSBEnableVSyncInterrupt(PVRPSB_DEVINFO *psDevInfo)
{
	IMG_UINT32 ui32InterruptEnable;
	IMG_UINT32 ui32InterruptMask;

	ui32InterruptEnable	= PVROSReadMMIOReg(psDevInfo, PVRPSB_IER);
	ui32InterruptMask	= PVROSReadMMIOReg(psDevInfo, PVRPSB_IMR);

	if (psDevInfo->eActivePipe == PSB_PIPE_A)
	{
		ui32InterruptEnable	= PVRPSB_IER_PIPEA_ENABLE_SET(ui32InterruptEnable, 1);
		ui32InterruptMask	= PVRPSB_IMR_PIPEA_MASK_SET(ui32InterruptMask, 0);
	}
	else
	{
		ui32InterruptEnable	= PVRPSB_IER_PIPEB_ENABLE_SET(ui32InterruptEnable, 1);
		ui32InterruptMask	= PVRPSB_IMR_PIPEB_MASK_SET(ui32InterruptMask, 0);
	}

	PVROSWriteMMIOReg(psDevInfo, PVRPSB_IER, ui32InterruptEnable);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_IMR, ui32InterruptMask);
}

void PVRPSBDisableVSyncInterrupt(PVRPSB_DEVINFO *psDevInfo)
{
	IMG_UINT32 ui32InterruptEnable;

	ui32InterruptEnable = PVROSReadMMIOReg(psDevInfo, PVRPSB_IER);

	if (psDevInfo->eActivePipe == PSB_PIPE_A)
	{
		ui32InterruptEnable = PVRPSB_IER_PIPEA_ENABLE_SET(ui32InterruptEnable, 0);
	}
	else
	{
		ui32InterruptEnable = PVRPSB_IER_PIPEB_ENABLE_SET(ui32InterruptEnable, 0);
	}

	PVROSWriteMMIOReg(psDevInfo, PVRPSB_IER, ui32InterruptEnable);
}
#endif /* SYS_USING_INTERRUPTS */

static void ResetVSyncFlipItems(PVRPSB_SWAPCHAIN *psSwapChain)
{
	IMG_UINT32 ui32I;
	
	psSwapChain->ui32InsertIndex = 0;
	psSwapChain->ui32RemoveIndex = 0;
	
	for (ui32I = 0; ui32I < psSwapChain->ui32BufferCount; ui32I++)
	{
		psSwapChain->psVSyncFlipItems[ui32I].bValid		= PSB_FALSE;
		psSwapChain->psVSyncFlipItems[ui32I].bFlipped		= PSB_FALSE;
		psSwapChain->psVSyncFlipItems[ui32I].bCmdCompleted	= PSB_FALSE;
	}
}

/* Function to flush all items out of the VSYNC queue.
   Apply pfFlipAction on each flip item. */
void PVRPSBFlushInternalVSyncQueue(PVRPSB_DEVINFO *psDevInfo)
{
	PVRPSB_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	PVRPSB_VSYNC_FLIP_ITEM *psFlipItem;

#if defined(SYS_USING_INTERRUPTS)
	/* Disable interrupts while we remove the internal vsync flip queue */
	PVRPSBDisableVSyncInterrupt(psDevInfo);
#endif
	
	/* Need to flush any flips now pending in Internal queue */
	psFlipItem = &psSwapChain->psVSyncFlipItems[psSwapChain->ui32RemoveIndex];
	
	while (psFlipItem->bValid)
	{
		if (psFlipItem->bFlipped == PSB_FALSE)
		{
			/* Flip to new surface - flip latches on next interrupt */
			PVRPSBFlip(psDevInfo, psFlipItem->psBuffer);
		}
		
		/* Command complete handler - allows dependencies for outstanding flips 
		   to be updated - doesn't matter that vsync interrupts have been disabled. */
		if (psFlipItem->bCmdCompleted == PSB_FALSE)
		{
			/* Don't schedule the MISR, by passing IMG_FALSE, as we're
			   just emptying the internal VsyncQueue */
			psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psFlipItem->hCmdComplete, IMG_FALSE);
		}
		
		AdvanceFlipIndex(psSwapChain, &psSwapChain->ui32RemoveIndex);
		
		/* Clear item state */
		psFlipItem->bFlipped		= PSB_FALSE;
		psFlipItem->bCmdCompleted	= PSB_FALSE;
		psFlipItem->bValid		= PSB_FALSE;
		
		/* Update to next flip item */
		psFlipItem = &psSwapChain->psVSyncFlipItems[psSwapChain->ui32RemoveIndex];
	}
	
	psSwapChain->ui32InsertIndex = 0;
	psSwapChain->ui32RemoveIndex = 0;

#if defined(SYS_USING_INTERRUPTS)
	/* Enable interrupts */
	PVRPSBEnableVSyncInterrupt(psDevInfo);
#endif
}


/*******************************************************************************
 * Functions called from services via the 3rd Party display class interface
 ******************************************************************************/

/* Open device function, called from services */
static PVRSRV_ERROR OpenDCDevice(IMG_UINT32		ui32DeviceID, 
				 IMG_HANDLE		*phDevice, 
				 PVRSRV_SYNC_DATA	*psSystemBufferSyncData)
{
	PVRPSB_DEVINFO *psDevInfo = PVRPSBGetDevInfo();

	if (psDevInfo->ui32ID == ui32DeviceID)
	{
		psDevInfo->psSystemBuffer->psSyncData = psSystemBufferSyncData;

		*phDevice = (IMG_HANDLE)psDevInfo;

		return PVRSRV_OK;
	}
	else
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
}

/* Close device function, called from services */
static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
	PVR_UNREFERENCED_PARAMETER(hDevice);

	return PVRSRV_OK;
}

/* Enumerate formats function, called from services */
static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE		hDevice,
				  IMG_UINT32		*pui32NumFormats,
				  DISPLAY_FORMAT	*psFormat)
{
	PVRPSB_DEVINFO *psDevInfo;

	if (!hDevice || !pui32NumFormats)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRPSB_DEVINFO *)hDevice;

	*pui32NumFormats = (IMG_UINT32)psDevInfo->ui32NumFormats;

	if (psFormat != IMG_NULL)
	{
		IMG_UINT32 ui32I;

		for (ui32I = 0; ui32I < psDevInfo->ui32NumFormats; ui32I++)
		{
			psFormat[ui32I] = psDevInfo->asDisplayFormatList[ui32I];
		}
	}

	return PVRSRV_OK;
}


/* Enumerate dims function, called from services */
static PVRSRV_ERROR EnumDCDims(IMG_HANDLE	hDevice,
			       DISPLAY_FORMAT	*psFormat,
			       IMG_UINT32	*pui32NumDims,
			       DISPLAY_DIMS	*psDim)
{
	PVRPSB_DEVINFO *psDevInfo;

	if (!hDevice || !psFormat || !pui32NumDims)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRPSB_DEVINFO *)hDevice;

	*pui32NumDims = (IMG_UINT32)psDevInfo->ui32NumDims;

	/* We have only one format so there's no need to check psFormat */
	if (psDim != IMG_NULL)
	{
		IMG_UINT32 ui32I;

		for (ui32I = 0; ui32I < psDevInfo->ui32NumDims; ui32I++)
		{
			psDim[ui32I] = psDevInfo->asDisplayDimList[ui32I];
		}
	}

	return PVRSRV_OK;
}

/* Get the system buffer function, called from services */
static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice, IMG_HANDLE *phBuffer)
{
	PVRPSB_DEVINFO *psDevInfo;

	if (!hDevice || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRPSB_DEVINFO *)hDevice;

	*phBuffer = (IMG_HANDLE)psDevInfo->psSystemBuffer;

	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	PVRPSB_DEVINFO *psDevInfo;

	if (!hDevice || !psDCInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRPSB_DEVINFO *)hDevice;

	*psDCInfo = psDevInfo->sDisplayInfo;

	return PVRSRV_OK;
}

/* get buffer address function, called from services */
static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE		hDevice,
				    IMG_HANDLE		hBuffer,
				    IMG_SYS_PHYADDR	**ppsSysAddr,
				    IMG_UINT32		*pui32ByteSize,
				    IMG_VOID		**ppvCpuVAddr,
				    IMG_HANDLE		*phOSMapInfo,
				    IMG_BOOL		*pbIsContiguous,
				    IMG_UINT32		*pui32TilingStride)
{
	PVRPSB_DEVINFO	*psDevInfo;
	PVRPSB_BUFFER	*psBuffer;

	PVR_UNREFERENCED_PARAMETER(pui32TilingStride);

	if (!hDevice || !hBuffer || !ppsSysAddr || !pui32ByteSize)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRPSB_DEVINFO *)hDevice;
	psBuffer = (PVRPSB_BUFFER *)hBuffer;

	*pui32ByteSize = psBuffer->ui32Size;

	if (psBuffer->bIsContiguous == PSB_TRUE)
	{
		*ppsSysAddr = &psBuffer->uSysAddr.sCont;
	}
	else
	{
		*ppsSysAddr = psBuffer->uSysAddr.psNonCont;
	}

	if (ppvCpuVAddr != IMG_NULL)
	{
		*ppvCpuVAddr = psBuffer->pvCPUVAddr;
	}

	if (phOSMapInfo != IMG_NULL)
	{
		*phOSMapInfo = (IMG_HANDLE)IMG_NULL;
	}

	if (pbIsContiguous != IMG_NULL)
	{
		*pbIsContiguous = (psBuffer->bIsContiguous == PSB_TRUE) ? IMG_TRUE : IMG_FALSE;
	}

	return PVRSRV_OK;
}

/** Create swapchain function, called from services
 *
 *  @Sets up a swapchain with @p ui32BufferCount number of buffers in the flip chain.
 *  If USE_PRIMARY_SURFACE_IN_FLIP_CHAIN is #define'd, the first buffer is the primary
 *  (aka system) surface and (@p ui32BufferCount -1) back buffers are setup.
 *  If USE_PRIMARY_SURFACE_IN_FLIP_CHAIN is not #define'd, @p ui32BufferCount back buffers
 *  are setup.
 */
static PVRSRV_ERROR CreateDCSwapChain(IMG_HANDLE		hDevice,
				      IMG_UINT32		ui32Flags,
				      DISPLAY_SURF_ATTRIBUTES	*psDstSurfAttrib,
				      DISPLAY_SURF_ATTRIBUTES	*psSrcSurfAttrib,
				      IMG_UINT32		ui32BufferCount,
				      PVRSRV_SYNC_DATA		**ppsSyncData,
				      IMG_UINT32		ui32OEMFlags,
				      IMG_HANDLE		*phSwapChain,
				      IMG_UINT32		*pui32SwapChainID)
{
	PVRPSB_DEVINFO *psDevInfo;
	PVRPSB_SWAPCHAIN *psSwapChain;
	PVRPSB_BUFFER *psBuffer;
	IMG_UINT32 ui32BufferNum;
	IMG_UINT32 ui32BackBufferNum;

	PVR_UNREFERENCED_PARAMETER(ui32Flags);
	PVR_UNREFERENCED_PARAMETER(ui32OEMFlags);

	if (!hDevice ||
	    !psDstSurfAttrib ||
	    !psSrcSurfAttrib ||
	    !ppsSyncData ||
	    !phSwapChain ||
	    !ui32BufferCount)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (PVRPSB_DEVINFO *)hDevice;

	/* This Poulsbo driver only supports a single swapchain */
	if (psDevInfo->psSwapChain)
	{
		return PVRSRV_ERROR_FLIP_CHAIN_EXISTS;
	}

	if (ui32BufferCount > psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers)
	{
		return PVRSRV_ERROR_TOOMANYBUFFERS;
	}

	/* SRC/DST must match the current display mode config */
	if (psDstSurfAttrib->pixelformat != psDevInfo->sDisplayFormat.pixelformat ||
	    psDstSurfAttrib->sDims.ui32ByteStride != psDevInfo->sDisplayDims.ui32ByteStride ||
	    psDstSurfAttrib->sDims.ui32Width != psDevInfo->sDisplayDims.ui32Width ||
	    psDstSurfAttrib->sDims.ui32Height != psDevInfo->sDisplayDims.ui32Height)
	{
		/* DST doesn't match the current mode */
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (psDstSurfAttrib->pixelformat != psSrcSurfAttrib->pixelformat ||
		psDstSurfAttrib->sDims.ui32ByteStride != psSrcSurfAttrib->sDims.ui32ByteStride ||
		psDstSurfAttrib->sDims.ui32Width != psSrcSurfAttrib->sDims.ui32Width ||
		psDstSurfAttrib->sDims.ui32Height != psSrcSurfAttrib->sDims.ui32Height)
	{
		/* DST doesn't match the SRC */
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psSwapChain = (PVRPSB_SWAPCHAIN *)PVROSAllocKernelMem(sizeof(PVRPSB_SWAPCHAIN));
	if (psSwapChain == NULL)
	{
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psSwapChain->ui32ID		= 1;

	psBuffer = (PVRPSB_BUFFER *)PVROSAllocKernelMem(sizeof(PVRPSB_BUFFER) * ui32BufferCount);
	if (psBuffer == NULL)
	{
		PVROSFreeKernelMem(psSwapChain);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psSwapChain->ui32BufferCount	= ui32BufferCount;
	psSwapChain->psBuffer		= psBuffer;

#if defined(USE_PRIMARY_SURFACE_IN_FLIP_CHAIN)
	/* The primary surface becomes psBuffer[0] in the swapchain buffer array. */
	psBuffer[0].ui32Size		= psDevInfo->psSystemBuffer->ui32Size;
	psBuffer[0].bIsContiguous	= psDevInfo->psSystemBuffer->bIsContiguous;
	psBuffer[0].uSysAddr		= psDevInfo->psSystemBuffer->uSysAddr;
	psBuffer[0].sDevVAddr		= psDevInfo->psSystemBuffer->sDevVAddr;
	psBuffer[0].pvCPUVAddr		= psDevInfo->psSystemBuffer->pvCPUVAddr;
	psBuffer[0].psSyncData		= ppsSyncData[0];
	ui32BufferNum = 1;
#else
	ui32BufferNum = 0;
#endif

	/* Populate the buffers with the preallocated buffers */
	for (ui32BackBufferNum = 0; ui32BufferNum < ui32BufferCount; ui32BufferNum++, ui32BackBufferNum++)
	{
		psBuffer[ui32BufferNum].ui32Size		= psDevInfo->apsBackBuffers[ui32BackBufferNum]->ui32Size;
		psBuffer[ui32BufferNum].bIsContiguous		= psDevInfo->apsBackBuffers[ui32BackBufferNum]->bIsContiguous;
		psBuffer[ui32BufferNum].uSysAddr		= psDevInfo->apsBackBuffers[ui32BackBufferNum]->uSysAddr;
		psBuffer[ui32BufferNum].sDevVAddr		= psDevInfo->apsBackBuffers[ui32BackBufferNum]->sDevVAddr;
		psBuffer[ui32BufferNum].pvCPUVAddr		= psDevInfo->apsBackBuffers[ui32BackBufferNum]->pvCPUVAddr;
		psBuffer[ui32BufferNum].psSyncData 		= ppsSyncData[ui32BufferNum];
	}

	psSwapChain->psVSyncFlipItems = (PVRPSB_VSYNC_FLIP_ITEM *)PVROSAllocKernelMem(sizeof(PVRPSB_VSYNC_FLIP_ITEM) * ui32BufferCount);
	if (psSwapChain->psVSyncFlipItems == NULL)
	{
		PVROSFreeKernelMem(psSwapChain->psBuffer);
		PVROSFreeKernelMem(psSwapChain);

		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	ResetVSyncFlipItems(psSwapChain);

	/* Store the swapchain and return it */
	psDevInfo->psSwapChain	= psSwapChain;
	*phSwapChain		= (IMG_HANDLE)psSwapChain;
	*pui32SwapChainID	= psSwapChain->ui32ID;

#if defined(SYS_USING_INTERRUPTS)
	PVRPSBEnableVSyncInterrupt(psDevInfo);
#endif

	return PVRSRV_OK;
}

/* destroy swapchain function, called from services */
static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice, IMG_HANDLE hSwapChain)
{
	PVRPSB_DEVINFO		*psDevInfo;
	PVRPSB_SWAPCHAIN	*psSwapChain;
	PSB_ERROR		eError;

	if (!hDevice || !hSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo	= (PVRPSB_DEVINFO *)hDevice;
	psSwapChain	= (PVRPSB_SWAPCHAIN *)hSwapChain;

	/* Flush the vsync flip queue */
	PVRPSBFlushInternalVSyncQueue(psDevInfo);
	
	/* Swap to primary */
	eError = PVRPSBFlip(psDevInfo, psDevInfo->psSystemBuffer);
	if (eError != PSB_OK)
	{
		return PVRSRV_ERROR_FLIP_FAILED;
	}

	PVROSFreeKernelMem(psSwapChain->psVSyncFlipItems);
	PVROSFreeKernelMem(psSwapChain->psBuffer);
	PVROSFreeKernelMem(psSwapChain);

	psDevInfo->psSwapChain = IMG_NULL;

#if defined(SYS_USING_INTERRUPTS)
	/* This should be done at the start of the function but PVRPSBFlushInternalVSyncQueue 
	   will reenable the vsync interrupts so disable it here instead */
	PVRPSBDisableVSyncInterrupt(psDevInfo);
#endif

	return PVRSRV_OK;
}

/* Set DST rect function, called from services */
static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE	hDevice,
				 IMG_HANDLE	hSwapChain,
				 IMG_RECT	*psRect)
{
	PVR_UNREFERENCED_PARAMETER(hDevice);
	PVR_UNREFERENCED_PARAMETER(hSwapChain);
	PVR_UNREFERENCED_PARAMETER(psRect);

	/* Only full screen swapchains on this device */
	return PVRSRV_ERROR_NOT_SUPPORTED;
}


/* Set SRC rect function, called from services */
static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE	hDevice,
				 IMG_HANDLE	hSwapChain,
				 IMG_RECT	*psRect)
{
	PVR_UNREFERENCED_PARAMETER(hDevice);
	PVR_UNREFERENCED_PARAMETER(hSwapChain);
	PVR_UNREFERENCED_PARAMETER(psRect);

	/* Only full screen swapchains on this device */
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/* Set DST colourkey function, called from services */
static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE	hDevice,
				      IMG_HANDLE	hSwapChain,
				      IMG_UINT32	ui32CKColour)
{
	PVR_UNREFERENCED_PARAMETER(hDevice);
	PVR_UNREFERENCED_PARAMETER(hSwapChain);
	PVR_UNREFERENCED_PARAMETER(ui32CKColour);

	/* Don't support DST CK on this device */
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/* Set SRC colourkey function, called from services */
static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE	hDevice,
				      IMG_HANDLE	hSwapChain,
				      IMG_UINT32	ui32CKColour)
{
	PVR_UNREFERENCED_PARAMETER(hDevice);
	PVR_UNREFERENCED_PARAMETER(hSwapChain);
	PVR_UNREFERENCED_PARAMETER(ui32CKColour);

	/* Don't support SRC CK on this device */
	return PVRSRV_ERROR_NOT_SUPPORTED;
}

/* Get swapchain buffers function, called from services */
static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE hDevice,
				 IMG_HANDLE hSwapChain,
				 IMG_UINT32 *pui32BufferCount,
				 IMG_HANDLE *phBuffer)
{
	PVRPSB_SWAPCHAIN *psSwapChain;
	IMG_UINT32 ui32Buffer;

	if (!hDevice || !hSwapChain || !pui32BufferCount || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psSwapChain = (PVRPSB_SWAPCHAIN *)hSwapChain;

	for (ui32Buffer = 0; ui32Buffer < psSwapChain->ui32BufferCount; ui32Buffer++)
	{
		phBuffer[ui32Buffer] = (IMG_HANDLE)&psSwapChain->psBuffer[ui32Buffer];
	}
	*pui32BufferCount = psSwapChain->ui32BufferCount;

	return PVRSRV_OK;
}

/* Swap to buffer function, called from services */
static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE	hDevice,
				   IMG_HANDLE	hBuffer,
				   IMG_UINT32	ui32SwapInterval,
				   IMG_HANDLE	hPrivateTag,
				   IMG_UINT32	ui32ClipRectCount,
				   IMG_RECT	*psClipRect)
{
	PVR_UNREFERENCED_PARAMETER(ui32SwapInterval);
	PVR_UNREFERENCED_PARAMETER(hPrivateTag);
	PVR_UNREFERENCED_PARAMETER(psClipRect);

	if (!hDevice || !hBuffer || (ui32ClipRectCount != 0))
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* Nothing to do since services common code does the work in the general case */
	return PVRSRV_OK;
}

/* Set state function, called from services */
static void PSBSetState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	PVRPSB_DEVINFO *psDevInfo = (PVRPSB_DEVINFO *)hDevice;
	
	if (ui32State == DC_STATE_FLUSH_COMMANDS)
	{
		if (psDevInfo->psSwapChain != 0)
		{
			PVRPSBFlushInternalVSyncQueue(psDevInfo);
		}
		
		psDevInfo->bFlushCommands = PSB_TRUE;
	}
	else if (ui32State == DC_STATE_NO_FLUSH_COMMANDS)
	{
		psDevInfo->bFlushCommands = PSB_FALSE;
	}
}


/************************************************************
	Command processing and interrupt specific functions:
************************************************************/
#if defined(SYS_USING_INTERRUPTS)
static IMG_BOOL VSyncISR(IMG_VOID *pvDevInfo)
{
	PVRPSB_DEVINFO		*psDevInfo = (PVRPSB_DEVINFO *)pvDevInfo;
	PVRPSB_SWAPCHAIN	*psSwapChain = psDevInfo->psSwapChain;
	IMG_UINT32		ui32InterruptStatus;
	IMG_BOOL		bStatus;

#if defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
	if (psDevInfo->bLeaveVT == PSB_TRUE)
	{
		return PSB_TRUE;
	}
#endif

	PVRPSBDisableVSyncInterrupt(psDevInfo);

	/* Get the interrupt status */
	ui32InterruptStatus = PVROSReadMMIOReg(psDevInfo, PVRPSB_IIR);

	if (psDevInfo->eActivePipe == PSB_PIPE_A)
	{
		ui32InterruptStatus = PVRPSB_IMR_PIPEA_STATUS_GET(ui32InterruptStatus);
	}
	else
	{
		ui32InterruptStatus = PVRPSB_IMR_PIPEB_STATUS_GET(ui32InterruptStatus);
	}

	bStatus = (ui32InterruptStatus != 0) ? IMG_TRUE : IMG_FALSE;
	if (bStatus == IMG_TRUE)
	{
		const IMG_UINT32	ui32PipeStatReg = (psDevInfo->eActivePipe == PSB_PIPE_A) ? PVRPSB_PIPEASTAT : PVRPSB_PIPEBSTAT;
		PVRPSB_VSYNC_FLIP_ITEM	*psFlipItem;
		IMG_UINT32		ui32PipeStat;

		/* Clear interrupts. The manual says we must clear the corresponding 
		   bit in PIPESTAT first, before the bit in IIR. */
		ui32PipeStat = PVROSReadMMIOReg(psDevInfo, ui32PipeStatReg);
		ui32PipeStat = PVRPSB_PIPESTAT_VBLANK_EN_SET(ui32PipeStat, 1);
		PVROSWriteMMIOReg(psDevInfo, ui32PipeStatReg, ui32PipeStat);

		/* Now we're allowed to clear the bit in the IIR, too */
		if (psDevInfo->eActivePipe == PSB_PIPE_A)
		{
			PVROSWriteMMIOReg(psDevInfo, PVRPSB_IIR, PVRPSB_IMR_PIPEA_STATUS_SET(0, 1));
		}
		else
		{
			PVROSWriteMMIOReg(psDevInfo, PVRPSB_IIR, PVRPSB_IMR_PIPEB_STATUS_SET(0, 1));
		}

		/* Check if swapchain exists */
		if (!psDevInfo->psSwapChain)
		{
			/* Re-enable interrupts */
			PVRPSBEnableVSyncInterrupt(psDevInfo);
			return IMG_FALSE;
		}

		psFlipItem = &psSwapChain->psVSyncFlipItems[psSwapChain->ui32RemoveIndex];

		while (psFlipItem->bValid)
		{
			/* Have we already flipped BEFORE this interrupt */
			if (psFlipItem->bFlipped)
			{
				/* Have we already 'Cmd Completed'? */
				if (!psFlipItem->bCmdCompleted)
				{
					IMG_BOOL bScheduleMISR;
					/* Only schedule the MISR if the display vsync is on its own LISR */
#if defined(POULSBO_DEVICE_ISR)
					bScheduleMISR = IMG_TRUE;
#else
					bScheduleMISR = IMG_FALSE;
#endif
					/* Command complete the flip */
					psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psFlipItem->hCmdComplete, bScheduleMISR);

					/* Signal we've done the cmd complete */
					psFlipItem->bCmdCompleted = PSB_TRUE;
				}

				/* We've cmd completed so decrement the swap interval */
				psFlipItem->ui32SwapInterval--;

				/* Can we remove the flip item? */
				if (psFlipItem->ui32SwapInterval == 0)
				{
					/* Advance remove index */
					AdvanceFlipIndex(psSwapChain, &psSwapChain->ui32RemoveIndex);

					/* Clear item state */
					psFlipItem->bCmdCompleted = PSB_FALSE;
					psFlipItem->bFlipped = PSB_FALSE;

					/* Only mark as invalid once item data is finished with */
					psFlipItem->bValid = PSB_FALSE;
				}
				else
				{
					/* We're waiting for the last flip to finish displaying
					   so the remove index hasn't been updated to block any
					   new flips occuring. Nothing more to do on interrupt */
					break;
				}
			}
			else
			{
				/* Flip to new surface - flip latches on next interrupt */
				PVRPSBFlip(psDevInfo, psFlipItem->psBuffer);

				/* Signal we've issued the flip to the HW */
				psFlipItem->bFlipped = PSB_TRUE;

				/* Nothing more to do on interrupt */
				break;
			}

			/* Update to next flip item */
			psFlipItem = &psSwapChain->psVSyncFlipItems[psSwapChain->ui32RemoveIndex];
		}
	}

	PVRPSBEnableVSyncInterrupt(psDevInfo);

	return bStatus;
}

/* Command processing flip handler function, called from services */
static IMG_BOOL ProcessFlip(IMG_HANDLE	hCmdCookie,
			    IMG_UINT32	ui32DataSize,
			    IMG_VOID	*pvData)
{
	DISPLAYCLASS_FLIP_COMMAND	*psFlipCmd;
	PVRPSB_VSYNC_FLIP_ITEM		*psFlipItem;
	PVRPSB_DEVINFO			*psDevInfo;
	PVRPSB_BUFFER			*psBuffer;
	PVRPSB_SWAPCHAIN		*psSwapChain;
	PSB_ERROR			eError;

	if (!hCmdCookie || !pvData)
	{
		return IMG_FALSE;
	}

	/* Validate data packet */
	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND *)pvData;
	if (sizeof(DISPLAYCLASS_FLIP_COMMAND) != ui32DataSize)
	{
		return IMG_FALSE;
	}

	/* Setup some useful pointers */
	psDevInfo	= (PVRPSB_DEVINFO *)psFlipCmd->hExtDevice;
	psBuffer	= (PVRPSB_BUFFER *)psFlipCmd->hExtBuffer; /* This is the buffer we are flipping to */
	psSwapChain	= psDevInfo->psSwapChain;

	if (psDevInfo->bFlushCommands)
	{
		/* PVR is flushing its queues so no need to flip.
		   Also don't schedule the MISR as we're already in the MISR */
		psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_FALSE);
		return IMG_TRUE;
	}

	/* Support for vsync "unlocked" flipping - not real support as this is a latched display,
	   we just complete immediately. */
	if (psFlipCmd->ui32SwapInterval == 0)
	{
		/* The 'baseaddr' register can be updated outside the vertical blanking region.
		   The 'baseaddr' update only takes effect on the vfetch event and the baseaddr register
		   update is double-buffered. Hence page flipping is 'latched'. */

		eError = PVRPSBFlip(psDevInfo, psBuffer);
		if (eError != PSB_OK)
		{
			return IMG_FALSE;
		}

		/* Call command complete callback. Don't schedule the MISR as we're already in the MISR */
		psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_FALSE);

		return IMG_TRUE;
	}

	psFlipItem = &psSwapChain->psVSyncFlipItems[psSwapChain->ui32InsertIndex];

	/* Try to insert command into list */
	if (!psFlipItem->bValid)
	{
		if (psSwapChain->ui32InsertIndex == psSwapChain->ui32RemoveIndex)
		{
			eError = PVRPSBFlip(psDevInfo, psBuffer);
			if (eError != PSB_OK)
			{
				return IMG_FALSE;
			}

			psFlipItem->bFlipped = PSB_TRUE;
		}
		else
		{
			psFlipItem->bFlipped = PSB_FALSE;
		}

		psFlipItem->hCmdComplete	= hCmdCookie;
		psFlipItem->psBuffer		= psBuffer;
		psFlipItem->ui32SwapInterval	= psFlipCmd->ui32SwapInterval;
		psFlipItem->bValid		= PSB_TRUE;

		AdvanceFlipIndex(psSwapChain, &psSwapChain->ui32InsertIndex);

		return IMG_TRUE;
	}

	return IMG_FALSE;
}

#else /* #if defined(SYS_USING_INTERRUPTS) */

/* Command processing flip handler function, called from services 
   Note: in the case of no interrupts just flip and complete */
static IMG_BOOL ProcessFlip(IMG_HANDLE	hCmdCookie,
			    IMG_UINT32	ui32DataSize,
			    IMG_VOID	*pvData)
{
	DISPLAYCLASS_FLIP_COMMAND	*psFlipCmd;
	PVRPSB_DEVINFO			*psDevInfo;
	PVRPSB_BUFFER			*psBuffer;
	PSB_ERROR			eError;
	
	if (!hCmdCookie || !pvData)
	{
		return IMG_FALSE;
	}
	
	/* Validate data packet */
	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND *)pvData;
	if (sizeof(DISPLAYCLASS_FLIP_COMMAND) != ui32DataSize)
	{
		return IMG_FALSE;
	}
	
	/* Setup some useful pointers */
	psDevInfo	= (PVRPSB_DEVINFO *)psFlipCmd->hExtDevice;
	psBuffer	= (PVRPSB_BUFFER *)psFlipCmd->hExtBuffer;
	
	eError = PVRPSBFlip(psDevInfo, psBuffer);
	if (eError != PSB_OK)
	{
		return IMG_FALSE;
	}
	
	/* Call command complete Callback. Don't schedule the MISR as we're already in the MISR */
	psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_FALSE);
	
	return IMG_TRUE;
}
#endif /* #if defined(SYS_USING_INTERRUPTS) */

/************************************************************
	services controlled power callbacks:
************************************************************/

#if !defined(POULSBO_DEVICE_POWER)
static PVRSRV_ERROR PrePower(IMG_HANDLE			hDevHandle,
			     PVRSRV_DEV_POWER_STATE	eNewPowerState,
			     PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if ((eNewPowerState != eCurrentPowerState) &&
	    (eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF))
	{
		PVRPSB_DEVINFO *psDevInfo = (PVRPSB_DEVINFO *)hDevHandle;

#if !defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL) && defined(SYS_USING_INTERRUPTS)
		if (psDevInfo->psSwapChain != 0)
		{
			PVRPSBDisableVSyncInterrupt(psDevInfo);
		}
#endif /* #if !defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL) && defined(SYS_USING_INTERRUPTS) */

		PVROSSaveState(psDevInfo);
	}
	
	return PVRSRV_OK;
}

static PVRSRV_ERROR PostPower(IMG_HANDLE 		hDevHandle,
			      PVRSRV_DEV_POWER_STATE	eNewPowerState,
			      PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if ((eNewPowerState != eCurrentPowerState) &&
	    (eCurrentPowerState == PVRSRV_DEV_POWER_STATE_OFF))
	{
		PVRPSB_DEVINFO *psDevInfo = (PVRPSB_DEVINFO *)hDevHandle;

		PVROSRestoreState(psDevInfo);

#if !defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL) && defined(SYS_USING_INTERRUPTS)
		if (psDevInfo->psSwapChain != 0)
		{
			PVRPSBEnableVSyncInterrupt(psDevInfo);
		}
#endif /* #if !defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL) && defined(SYS_USING_INTERRUPTS) */
	}
	
	return PVRSRV_OK;
}
#endif /* #if !defined(POULSBO_DEVICE_POWER) */


/*******************************************************************************
 * GTT functions
 ******************************************************************************/

static PSB_ERROR GTTInit(PVRPSB_DEVINFO *psDevInfo, PVRPSB_GTT_INFO *psGTTInfo)
{
	IMG_UINT16 ui16GCValue;
	IMG_UINT32 ui32PGTBLValue;

	/* Set enable bits so we can use the GTT */
	ui16GCValue = PVROSPciReadWord(psDevInfo, PVRPSB_PCIREG_GC);
	PVROSPciWriteWord(psDevInfo, PVRPSB_PCIREG_GC, ui16GCValue | 0x4);

	ui32PGTBLValue = PVROSReadMMIOReg(psDevInfo, PVRPSB_PGTBL_CTL);
	ui32PGTBLValue = PVRPSB_PGTBL_CTL_ENABLE_SET(ui32PGTBLValue, 1);
	PVROSWriteMMIOReg(psDevInfo, PVRPSB_PGTBL_CTL, ui32PGTBLValue);

	/* Read it back to make sure it sticks */
	ui32PGTBLValue = PVROSReadMMIOReg(psDevInfo, PVRPSB_PGTBL_CTL);

	/* Map the GTT so we can insert the stolen and system memory into it */
	psGTTInfo->sGTTSysAddr.uiAddr = PVRPSB_PGTBL_CTL_BASEADDR_GET(ui32PGTBLValue);
	
	psGTTInfo->ui32GTTSize = GetMemRegionSize(psDevInfo, PVRPSB_PCIREG_GTT_BASE);
	if (psGTTInfo->ui32GTTSize == 0)
	{
		/* The BAR is uninitialised on Cedarview so set the GTT size manually */
		psGTTInfo->ui32GTTSize = 64 << PVRPSB_PAGE_SHIFT;
	}

	psGTTInfo->pvGTTCPUVAddr = PVROSMapPhysAddr(psGTTInfo->sGTTSysAddr, psGTTInfo->ui32GTTSize);
	psGTTInfo->ui32GTTOffset = 0;

	/* Get graphics memory information */
	psGTTInfo->sGMemDevVAddr.uiAddr = PVROSPciReadDWord(psDevInfo, PVRPSB_PCIREG_GMEM_BASE);
	if (psGTTInfo->sGMemDevVAddr.uiAddr == 0)
	{
		/* The BAR is uninitialised on Cedarview so set the GMem address manually */
		psGTTInfo->sGMemDevVAddr.uiAddr = 0x40000000;
	}

	psGTTInfo->ui32GMemSizeInPages = GetMemRegionSize(psDevInfo, PVRPSB_PCIREG_GMEM_BASE) >> PVRPSB_PAGE_SHIFT;
	if (psGTTInfo->ui32GMemSizeInPages == 0)
	{
		/* The BAR is uninitialised on Cedarview so set the GMem size manually */
		psGTTInfo->ui32GMemSizeInPages = (128 * 1024 * 1024)  >> PVRPSB_PAGE_SHIFT;
	}

	/* Get the address of the stolen memory so we can use it for our buffers */
	psGTTInfo->sStolenSysAddr.uiAddr	= PVROSPciReadDWord(psDevInfo, PVRPSB_PCIREG_BSM);
	psGTTInfo->ui32StolenSizeInPages	= (psGTTInfo->sGTTSysAddr.uiAddr - psGTTInfo->sStolenSysAddr.uiAddr - PVRPSB_PAGE_SIZE) >> PVRPSB_PAGE_SHIFT;
	psGTTInfo->ui32StolenPageOffset		= 0;

#if defined(SUPPORT_DRI_DRM)
	/* Alloc memory to save the GTT contents when saving and restoring the chip state */
	psDevInfo->sSuspendState.sDevRegisters.pui32GTTContents = (IMG_UINT32 *)PVROSAllocKernelMem(psGTTInfo->ui32GTTSize);
	if (psDevInfo->sSuspendState.sDevRegisters.pui32GTTContents == NULL)
	{
		printk(KERN_WARNING DRVNAME ": %s - Failed to allocate memory to store GTT contents\n", __FUNCTION__);

		PVROSUnMapPhysAddr(psGTTInfo->pvGTTCPUVAddr);
		return PSB_ERROR_OUT_OF_MEMORY;
	}
#endif /* #if defined(SUPPORT_DRI_DRM) */

	return PSB_OK;
}

static void GTTDeinit(PVRPSB_DEVINFO *psDevInfo, PVRPSB_GTT_INFO *psGTTInfo)
{
#if defined(SUPPORT_DRI_DRM)
	PVROSFreeKernelMem(psDevInfo->sSuspendState.sDevRegisters.pui32GTTContents);
	psDevInfo->sSuspendState.sDevRegisters.pui32GTTContents = NULL;
#endif /* #if defined(SUPPORT_DRI_DRM) */

	/* Reset the GTT information */
	psGTTInfo->ui32GMemSizeInPages = 0;
	psGTTInfo->sGMemDevVAddr.uiAddr = IMG_NULL;

	psGTTInfo->ui32StolenPageOffset = 0;
	psGTTInfo->ui32StolenSizeInPages = 0;
	psGTTInfo->sStolenSysAddr.uiAddr = IMG_NULL;

	PVROSUnMapPhysAddr(psGTTInfo->pvGTTCPUVAddr);
	psGTTInfo->ui32GTTSize = 0;
	psGTTInfo->sGTTSysAddr.uiAddr = IMG_NULL;
	psGTTInfo->ui32GTTOffset = 0;
}

static IMG_VOID GTTAllocStolenPages(PVRPSB_GTT_INFO *psGTTInfo, IMG_UINT32 ui32SizeInPages, IMG_SYS_PHYADDR *psSysPhysAddr)
{
	psSysPhysAddr->uiAddr = 0;

	if (psGTTInfo->ui32StolenPageOffset + ui32SizeInPages <= psGTTInfo->ui32StolenSizeInPages)
	{
		psSysPhysAddr->uiAddr = psGTTInfo->sStolenSysAddr.uiAddr + (psGTTInfo->ui32StolenPageOffset << PVRPSB_PAGE_SHIFT);
		psGTTInfo->ui32StolenPageOffset += ui32SizeInPages;
	}
}

static IMG_BOOL GTTInsertPages(PVRPSB_DEVINFO *psDevInfo, IMG_SYS_PHYADDR *psSysAddr, IMG_UINT32 ui32NumPages, IMG_BOOL bContiguous, IMG_DEV_VIRTADDR *psDevVAddr)
{
	PVRPSB_GTT_INFO *psGTTInfo = &(psDevInfo->sGTTInfo);
	IMG_UINT32 ui32PageAddr;
	IMG_UINT32 ui32PageNum;

	/* Check we have enough space in the GTT to insert all the pages */
	if (psGTTInfo->ui32GTTSize < psGTTInfo->ui32GTTOffset + (ui32NumPages << 2))
	{
		return IMG_FALSE;
	}

	psDevVAddr->uiAddr = psGTTInfo->sGMemDevVAddr.uiAddr + ((psGTTInfo->ui32GTTOffset >> 2) << PVRPSB_PAGE_SHIFT);

	for (ui32PageNum = 0; ui32PageNum < ui32NumPages; ui32PageNum++, psGTTInfo->ui32GTTOffset += 4)
	{
		if (bContiguous == IMG_TRUE)
		{
			ui32PageAddr = psSysAddr->uiAddr + (ui32PageNum << PVRPSB_PAGE_SHIFT);
		}
		else
		{
			ui32PageAddr = psSysAddr[ui32PageNum].uiAddr;
		}

		PVROSWriteIOMem(psGTTInfo->pvGTTCPUVAddr + psGTTInfo->ui32GTTOffset, ui32PageAddr | 0x1);
	}

	return IMG_TRUE;
}


/*******************************************************************************
 * Buffer functions
 ******************************************************************************/
PVRPSB_BUFFER *PVRPSBCreateBuffer(PVRPSB_DEVINFO *psDevInfo, IMG_UINT32 ui32BufferSize)
{
	PVRPSB_BUFFER *psBuffer;

	psBuffer = (PVRPSB_BUFFER *)PVROSAllocKernelMem(sizeof(PVRPSB_BUFFER));
	if (psBuffer != NULL)
	{
		PVRPSB_GTT_INFO *psGTTInfo = &(psDevInfo->sGTTInfo);
		IMG_UINT32 ui32BufferSizeInPages;

		ui32BufferSize		= PVRPSB_PAGE_ALIGN(ui32BufferSize);
		ui32BufferSizeInPages	= ui32BufferSize >> PVRPSB_PAGE_SHIFT;

		/* Try to use stolen memory and if this fails fall back to using system memory */
		GTTAllocStolenPages(psGTTInfo, ui32BufferSizeInPages, &(psBuffer->uSysAddr.sCont));

		if (psBuffer->uSysAddr.sCont.uiAddr != 0)
		{
			if (GTTInsertPages(psDevInfo, &(psBuffer->uSysAddr.sCont), ui32BufferSizeInPages, IMG_TRUE, &(psBuffer->sDevVAddr)) == IMG_FALSE)
			{
				goto ErrorFreeBuffer;
			}

			psBuffer->pvCPUVAddr	= PVROSMapPhysAddrWC(psBuffer->uSysAddr.sCont, ui32BufferSize);	
			psBuffer->bIsContiguous	= PSB_TRUE;
		}
		else
		{
			psBuffer->uSysAddr.psNonCont = (IMG_SYS_PHYADDR *)PVROSAllocKernelMem(sizeof(IMG_SYS_PHYADDR) * ui32BufferSizeInPages);
			if (psBuffer->uSysAddr.psNonCont == NULL)
			{
				goto ErrorFreeBuffer;
			}
			
			psBuffer->pvCPUVAddr = PVROSAllocKernelMemForBuffer(ui32BufferSize, psBuffer->uSysAddr.psNonCont);
			if (psBuffer->pvCPUVAddr == NULL)
			{
				goto ErrorFreePageBuffer;
			}

			if (GTTInsertPages(psDevInfo, psBuffer->uSysAddr.psNonCont, ui32BufferSizeInPages, IMG_FALSE, &(psBuffer->sDevVAddr)) == IMG_FALSE)
			{
				goto ErrorFreeBufferMem;
			}
			
			psBuffer->bIsContiguous	= PSB_FALSE;
		}
		psBuffer->ui32Size = ui32BufferSize;
	}

	return psBuffer;

ErrorFreeBufferMem:
	PVROSFreeKernelMemForBuffer(psBuffer->pvCPUVAddr);

ErrorFreePageBuffer:
	PVROSFreeKernelMem(psBuffer->uSysAddr.psNonCont);

ErrorFreeBuffer:
	PVROSFreeKernelMem(psBuffer);

	return NULL;
}

void PVRPSBDestroyBuffer(PVRPSB_BUFFER *psBuffer)
{
	if (psBuffer->bIsContiguous == PSB_TRUE)
	{
		PVROSUnMapPhysAddr(psBuffer->pvCPUVAddr);
	}
	else
	{
		PVROSFreeKernelMem(psBuffer->uSysAddr.psNonCont);
		PVROSFreeKernelMemForBuffer(psBuffer->pvCPUVAddr);
	}

	PVROSFreeKernelMem(psBuffer);
}

static PSB_ERROR BuffersInit(PVRPSB_DEVINFO *psDevInfo)
{
	IMG_UINT32 ui32BufferSize = psDevInfo->sDisplayDims.ui32ByteStride * psDevInfo->sDisplayDims.ui32Height;
	IMG_UINT32 ui32BufferNum;

#if !defined(SUPPORT_DRI_DRM)
	psDevInfo->psSystemBuffer = PVRPSBCreateBuffer(psDevInfo, ui32BufferSize);
	if (psDevInfo->psSystemBuffer == NULL)
	{
		return PSB_ERROR_OUT_OF_MEMORY;
	}

	if (psDevInfo->sDisplayFormat.pixelformat == PVRSRV_PIXEL_FORMAT_ARGB8888)
	{
		IMG_UINT32 *pui32Buffer = (IMG_UINT32 *)psDevInfo->psSystemBuffer->pvCPUVAddr;
		IMG_UINT32 ui32NumPixels = psDevInfo->psSystemBuffer->ui32Size >> 2;
		int i;

		/* Clear the system buffer to green */
		for (i = 0; i < ui32NumPixels; i++)
		{
			pui32Buffer[i] = 0xFF00FF00;
		}
	}
	else
	{
		/* Clear the system buffer to black */
		PVROSSetIOMem(psDevInfo->psSystemBuffer->pvCPUVAddr, 0, psDevInfo->psSystemBuffer->ui32Size);
	}
#endif /* #if !defined(SUPPORT_DRI_DRM) */	

	for (ui32BufferNum = 0; ui32BufferNum < PVRPSB_MAX_BACKBUFFERS; ui32BufferNum++)
	{
		psDevInfo->apsBackBuffers[ui32BufferNum] = PVRPSBCreateBuffer(psDevInfo, ui32BufferSize);
		if (psDevInfo->apsBackBuffers[ui32BufferNum] == IMG_NULL)
		{
			break;
		}
	}
	psDevInfo->ui32TotalBackBuffers = ui32BufferNum;

	return PSB_OK;
}

static void BufferDeinit(PVRPSB_DEVINFO *psDevInfo)
{
	IMG_UINT32 ui32BufferNum;

	for (ui32BufferNum = 0; ui32BufferNum < psDevInfo->ui32TotalBackBuffers; ui32BufferNum++)
	{
		PVRPSBDestroyBuffer(psDevInfo->apsBackBuffers[ui32BufferNum]);
		psDevInfo->apsBackBuffers[ui32BufferNum] = IMG_NULL;
	}
	psDevInfo->ui32TotalBackBuffers = 0;

#if !defined(SUPPORT_DRI_DRM)
	/* Clear the system buffer to black so we know the driver has unloaded */
	PVROSSetIOMem(psDevInfo->psSystemBuffer->pvCPUVAddr, 0, psDevInfo->psSystemBuffer->ui32Size);

	PVRPSBDestroyBuffer(psDevInfo->psSystemBuffer);
	psDevInfo->psSystemBuffer = IMG_NULL;
#endif
}


/*******************************************************************************
 * HW Cursor functions
 ******************************************************************************/
static PSB_ERROR CursorInit(PVRPSB_DEVINFO *psDevInfo)
{
#if defined(SUPPORT_DRI_DRM)
	IMG_UINT32 ui32MaxBufferSize = PVRPSB_HW_CURSOR_STRIDE * PVRPSB_HW_CURSOR_HEIGHT;

	psDevInfo->sCursorInfo.psBuffer = PVRPSBCreateBuffer(psDevInfo, ui32MaxBufferSize);
	if (psDevInfo->sCursorInfo.psBuffer == IMG_NULL)
	{
		return PSB_ERROR_OUT_OF_MEMORY;
	}

	/* For some reason using system memory results in a corrupt cursor. If  
	   we get a non-contiguous buffer we know it's not stolen memory. So,  
	   in this case fail to avoid showing a corrupt cursor. */
	if (psDevInfo->sCursorInfo.psBuffer->bIsContiguous == PSB_FALSE)
	{
		PVRPSBDestroyBuffer(psDevInfo->sCursorInfo.psBuffer);
		psDevInfo->sCursorInfo.psBuffer = IMG_NULL;

		return PSB_ERROR_GENERIC;
	}

	psDevInfo->sCursorInfo.ui32Width	= PVRPSB_HW_CURSOR_WIDTH;
	psDevInfo->sCursorInfo.ui32Height	= PVRPSB_HW_CURSOR_HEIGHT;

	psDevInfo->sCursorInfo.i32X		= 0;
	psDevInfo->sCursorInfo.i32Y		= 0;

	return PSB_OK;
#else
	return PSB_OK;
#endif
}

static void CursorDeinit(PVRPSB_DEVINFO *psDevInfo)
{
#if defined(SUPPORT_DRI_DRM)
	psDevInfo->sCursorInfo.ui32Width	= 0;
	psDevInfo->sCursorInfo.ui32Height	= 0;

	psDevInfo->sCursorInfo.i32X		= 0;
	psDevInfo->sCursorInfo.i32Y		= 0;

	PVRPSBDestroyBuffer(psDevInfo->sCursorInfo.psBuffer);
	psDevInfo->sCursorInfo.psBuffer = IMG_NULL;
#endif
}


/*******************************************************************************
 * OS independent functions
 ******************************************************************************/

PVRPSB_DEVINFO *PVRPSBGetDevInfo(void)
{
	return (PVRPSB_DEVINFO *)gpvAnchor;
}

void PVRPSBSetDevInfo(PVRPSB_DEVINFO *psDevInfo)
{
	gpvAnchor = (IMG_VOID *)psDevInfo;
}

IMG_UINT32 PVRPSBGetBpp(PVRSRV_PIXEL_FORMAT ePixelFormat)
{
	switch (ePixelFormat)
	{
		case PVRSRV_PIXEL_FORMAT_ARGB8888:
		{
			return 4;
		}
		case PVRSRV_PIXEL_FORMAT_RGB565:
		{
			return 2;
		}
		case PVRSRV_PIXEL_FORMAT_PAL8:
		{
			return 1;
		}
		default:
		{
			return 0;
		}
	}
}

/*****************************************************************************
 Function Name	: PVRPSBSelectPLLFreq
 Inputs		: ui32TargetClock - desired dot clock.
 Outputs	: psPllFreq
 Returns	: PSB_TRUE - success; PSB_FALSE - failure.
 Description	: Get the best set of values of M1, M2, N, P1, P2 to satify 
		  the requested dot clock and reference frequency.

		  PLL frequency calculation formula (see page 49 of 
		  VOL_3_display_registers_updated.pdf):
 				
			dot_clock = ref_freq * (5 * (M1 + 2) + (M2 + 2))
			dot_clock = dot_clock / (N + 2) / (P1 * P2)

		  N, M1 and M2 are used to setup FPA0/FPB0 registers;
		  P1, P2 are used to setup PLLA_CTRL/PLLB_CTRL registers.
*****************************************************************************/
PSB_BOOL PVRPSBSelectPLLFreq(IMG_UINT32 ui32TargetClock, const PVRPSB_PLL_RANGE *psPllRange, PLL_FREQ *psPllFreq)
{
	IMG_UINT32 ui32ClockDiffBest = 0xFFFFFFFF;
	IMG_UINT32 ui32Clock;
	IMG_UINT32 ui32P1;
	IMG_UINT32 ui32P2;
	IMG_UINT32 ui32M1;
	IMG_UINT32 ui32M2;
	IMG_UINT32 ui32N;

	ui32P2 = (ui32TargetClock <= psPllRange->ui32P2Divide) ? psPllRange->ui32P2Hi : psPllRange->ui32P2Lo;

	for (ui32P1 = psPllRange->ui32P1Min; ui32P1 <= psPllRange->ui32P1Max; ui32P1++)
	{
		for (ui32N = psPllRange->ui32NMin; ui32N <= psPllRange->ui32NMax; ui32N++)
		{
			for (ui32M1 = psPllRange->ui32M1Min; ui32M1 <= psPllRange->ui32M1Max; ui32M1++)
			{
				for (ui32M2 = psPllRange->ui32M2Min; (ui32M2 <= psPllRange->ui32M2Max); ui32M2++)
				{
					IMG_UINT32 ui32Vco = 0;
					IMG_UINT32 ui32M = 0;
					IMG_UINT32 ui32ClockDiff;

					switch (psPllRange->eDevice)
					{
						case PSB_CEDARVIEW:
							ui32M = (ui32M2 + 2);
							break;
						case PSB_POULSBO:
							ui32M = (5 * (ui32M1 + 2)) + (ui32M2 + 2);
							break;
						case PSB_UNKNOWN:
							return PSB_FALSE;
					}

					/* Check the value is in range */
					if ((ui32M < psPllRange->ui32MMin) || (ui32M > psPllRange->ui32MMax))
					{
						continue;
					}

					switch (psPllRange->eDevice)
					{
						case PSB_CEDARVIEW:
							ui32Vco = (psPllRange->ui32RefFreq * ui32M) / ui32N;
							break;
						case PSB_POULSBO:
							ui32Vco = (psPllRange->ui32RefFreq * ui32M) / (ui32N + 2);
							break;
						case PSB_UNKNOWN:
							return PSB_FALSE;
					}

					/* Check the value is in range */
					if ((ui32Vco < psPllRange->ui32VcoMin) || (ui32Vco > psPllRange->ui32VcoMax))
					{
						continue;
					}

					ui32Clock = (ui32Vco / (ui32P1 * ui32P2));

					ui32ClockDiff = MAX(ui32TargetClock, ui32Clock) - MIN(ui32TargetClock, ui32Clock);
					if (ui32ClockDiff < ui32ClockDiffBest)
					{
						ui32ClockDiffBest	= ui32ClockDiff;
						psPllFreq->ui32Clock	= ui32Clock;
						psPllFreq->ui32Vco	= ui32Vco;
						psPllFreq->ui32M1	= ui32M1;
						psPllFreq->ui32M2	= ui32M2;
						psPllFreq->ui32N	= ui32N;
						psPllFreq->ui32P1	= ui32P1;
						psPllFreq->ui32P2	= ui32P2;

						/* We have already got the best values, quit now. */
						if (ui32ClockDiff == 0)
						{
							return PSB_TRUE;
						}
					}
				}
			}
		}
	}

	return (ui32ClockDiffBest == 0xFFFFFFFF) ? PSB_FALSE : PSB_TRUE;
}

/* The common Poulsbo driver initialisation function. Here we do the following:
   - connect to services
   - register with services
   - allocate and setup private data structure */
PSB_ERROR PVRPSBInit(PVRPSB_DEVINFO *psDevInfo)
{
	PFN_CMD_PROC apfnCmdProcList[PVRPSB_COMMAND_COUNT];
	IMG_UINT32 aui32SyncCountList[PVRPSB_COMMAND_COUNT][2];
	IMG_UINT32 ui32RegBaseAddr;
	IMG_UINT32 ui32RegSize;
	struct pci_dev *psPciDev;
	PSB_ERROR eReturn;

#if defined(SUPPORT_DRI_DRM)
	psPciDev = psDevInfo->psDrmDev->pdev;
#else
	psPciDev = psDevInfo->psPciDev;
#endif

	psDevInfo->eDevice = GetDevice(psPciDev->device);
	if (psDevInfo->eDevice == PSB_UNKNOWN)
	{
		printk(KERN_ERR DRVNAME ": %s - Device has unsupported PCI id 0x%04X\n", __FUNCTION__, psPciDev->device);
		return PSB_ERROR_INVALID_DEVICE;
	}

	strncpy(psDevInfo->sDisplayInfo.szDisplayName, DISPLAY_DEVICE_NAME, MAX_DISPLAY_NAME_SIZE);
	psDevInfo->bFlushCommands	= PSB_FALSE;
	psDevInfo->psSwapChain		= IMG_NULL;

	/* We need to map in the device registers before we can proceed */
	ui32RegBaseAddr			= PVROSPciReadDWord(psDevInfo, PVRPSB_PCIREG_MEM_BASE);
	ui32RegSize			= GetMemRegionSize(psDevInfo, PVRPSB_PCIREG_MEM_BASE);

	psDevInfo->sRegSysAddr.uiAddr	= ui32RegBaseAddr + PVRPSB_PCIREG_OFFSET;
	psDevInfo->pvRegCPUVAddr	= PVROSMapPhysAddr(psDevInfo->sRegSysAddr, ui32RegSize);

	/* This must be done before we can allocate any buffers */
	eReturn = GTTInit(psDevInfo, &psDevInfo->sGTTInfo);
	if (eReturn != PSB_OK)
	{
		goto ErrorUnmapRegisters;
	}

	/* If the cursor is allocated from system memory we end up with a corrupt cursor 
	   (maybe we're doing something wrong?). To work around this make sure the cursor 
	   buffer is allocated first so we can use stolen memory (if there is some). */
	if (CursorInit(psDevInfo) != PSB_OK)
	{
		/* This is non-fatal so just print a warning */
		printk(KERN_WARNING DRVNAME ": %s - Hardware cursor initialisation failed\n", __FUNCTION__);
	}

	eReturn = PVROSModeSetInit(psDevInfo);
	if (eReturn != PSB_OK)
	{
		goto ErrorCursorDeinit;
	}

	eReturn = BuffersInit(psDevInfo);
	if (eReturn != PSB_OK)
	{
		goto ErrorModeSetDeinit;
	}

#if !defined(SUPPORT_DRI_DRM)
	PVRPSBFlip(psDevInfo, psDevInfo->psSystemBuffer);
#endif

	/* Now we've done most of the important setup fill in some device information */
	psDevInfo->sDisplayInfo.ui32MinSwapInterval	= PVRPSB_MIN_SWAP_INTERVAL;
	psDevInfo->sDisplayInfo.ui32MaxSwapInterval	= PVRPSB_MAX_SWAP_INTERVAL;
	psDevInfo->sDisplayInfo.ui32MaxSwapChains	= PVRPSB_MAX_SWAPCHAINS;

#if defined(USE_PRIMARY_SURFACE_IN_FLIP_CHAIN)
	psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers	= psDevInfo->ui32TotalBackBuffers + 1;
#else
	psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers	= psDevInfo->ui32TotalBackBuffers;
#endif

	psDevInfo->ui32NumFormats			= PVRPSB_MAX_FORMATS;
	psDevInfo->asDisplayFormatList[0].pixelformat	= psDevInfo->sDisplayFormat.pixelformat;

	psDevInfo->ui32NumDims				= PVRPSB_MAX_DIMS;
	psDevInfo->asDisplayDimList[0].ui32ByteStride	= psDevInfo->sDisplayDims.ui32ByteStride;
	psDevInfo->asDisplayDimList[0].ui32Width	= psDevInfo->sDisplayDims.ui32Width;
	psDevInfo->asDisplayDimList[0].ui32Height	= psDevInfo->sDisplayDims.ui32Height;

#if defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
	psDevInfo->bLeaveVT 				= PSB_FALSE;
#endif

	if (PVRGetDisplayClassJTable(&psDevInfo->sPVRJTable) == IMG_FALSE)
	{
		eReturn = PSB_ERROR_INIT_FAILURE;
		goto ErrorBuffersDeinit;
	}

	/* Setup the DC Jtable so SRVKM can call into this driver */
	psDevInfo->sDCJTable.ui32TableSize		= sizeof(PVRSRV_DC_SRV2DISP_KMJTABLE);
	psDevInfo->sDCJTable.pfnOpenDCDevice		= OpenDCDevice;
	psDevInfo->sDCJTable.pfnCloseDCDevice		= CloseDCDevice;
	psDevInfo->sDCJTable.pfnEnumDCFormats		= EnumDCFormats;
	psDevInfo->sDCJTable.pfnEnumDCDims		= EnumDCDims;
	psDevInfo->sDCJTable.pfnGetDCSystemBuffer	= GetDCSystemBuffer;
	psDevInfo->sDCJTable.pfnGetDCInfo		= GetDCInfo;
	psDevInfo->sDCJTable.pfnGetBufferAddr		= GetDCBufferAddr;
	psDevInfo->sDCJTable.pfnCreateDCSwapChain	= CreateDCSwapChain;
	psDevInfo->sDCJTable.pfnDestroyDCSwapChain	= DestroyDCSwapChain;
	psDevInfo->sDCJTable.pfnSetDCDstRect		= SetDCDstRect;
	psDevInfo->sDCJTable.pfnSetDCSrcRect		= SetDCSrcRect;
	psDevInfo->sDCJTable.pfnSetDCDstColourKey	= SetDCDstColourKey;
	psDevInfo->sDCJTable.pfnSetDCSrcColourKey	= SetDCSrcColourKey;
	psDevInfo->sDCJTable.pfnGetDCBuffers		= GetDCBuffers;
	psDevInfo->sDCJTable.pfnSwapToDCBuffer		= SwapToDCBuffer;
	psDevInfo->sDCJTable.pfnSetDCState		= PSBSetState;

	/* Register device with services and retrieve device index */
	if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterDCDevice(&psDevInfo->sDCJTable,
							    &psDevInfo->ui32ID) != PVRSRV_OK)
	{
		eReturn = PSB_ERROR_DEVICE_REGISTER_FAILED;
		goto ErrorBuffersDeinit;
	}

	/* Setup private command processing function table */
	apfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;

	/* Associated sync count(s)	*/
	aui32SyncCountList[DC_FLIP_COMMAND][0] = 0; /* No writes */
	aui32SyncCountList[DC_FLIP_COMMAND][1] = 2; /* 2 reads: To / From */

	/* Register private command processing functions with the Command Queue 
	   Manager and setup the general command complete function in the devinfo. */
	if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList(psDevInfo->ui32ID, 
							       &apfnCmdProcList[0], 
							       aui32SyncCountList, 
							       PVRPSB_COMMAND_COUNT) != PVRSRV_OK)
	{
		eReturn = PSB_ERROR_CANT_REGISTER_CALLBACK;
		goto ErrorRemoveDCDevice;
	}

#if defined(SYS_USING_INTERRUPTS)
#if defined(POULSBO_DEVICE_ISR)
	/* Install a device specific ISR handler for PSB */
	if (InstallVsyncISR(psDevInfo) != PSB_OK)
	{
		eReturn = PSB_ERROR_INIT_FAILURE;
		goto ErrorRemoveCmdProcList;
	}
#else
	/* Register external ISR to be called off the back of the services system ISR */
	if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterSystemISRHandler(VSyncISR, 
								    psDevInfo, 
								    0, 
								    psDevInfo->ui32ID) != PVRSRV_OK)
	{
		eReturn = PSB_ERROR_INIT_FAILURE;
		goto ErrorRemoveCmdProcList;
	}
#endif /* defined(POULSBO_DEVICE_ISR) */
#endif /* defined(SYS_USING_INTERRUPTS) */

#if defined(POULSBO_DEVICE_POWER)
	/* Note: In this case the Device should be registered with the OS power manager directly */
#else
	/* Register PSB with services to get power events from services.
	   Note: Generally this is not the recommended method for 
	   	 3rd party display drivers power management. */
	if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterPowerDevice(psDevInfo->ui32ID, 
							       PrePower, PostPower, 
							       IMG_NULL, IMG_NULL, 
							       psDevInfo, 
							       PVRSRV_DEV_POWER_STATE_ON, 
							       PVRSRV_DEV_POWER_STATE_ON) != PVRSRV_OK)
	{
		eReturn = PSB_ERROR_INIT_FAILURE;
		goto ErrorUninstallVSyncISR;
	}
#endif /* defined(POULSBO_DEVICE_POWER) */

	return PSB_OK;

ErrorUninstallVSyncISR:
#if defined(SYS_USING_INTERRUPTS)
#if defined(POULSBO_DEVICE_ISR)
	UninstallVsyncISR(psDevInfo);
#else
	psDevInfo->sPVRJTable.pfnPVRSRVRegisterSystemISRHandler(IMG_NULL, IMG_NULL, 0, psDevInfo->ui32ID);
#endif /* defined(PSB_DEVICE_ISR) */
#endif /* defined(SYS_USING_INTERRUPTS) */

ErrorRemoveCmdProcList:
	psDevInfo->sPVRJTable.pfnPVRSRVRemoveCmdProcList(psDevInfo->ui32ID, PVRPSB_COMMAND_COUNT);

ErrorRemoveDCDevice:
	psDevInfo->sPVRJTable.pfnPVRSRVRemoveDCDevice(psDevInfo->ui32ID);

ErrorBuffersDeinit:
	BufferDeinit(psDevInfo);

ErrorModeSetDeinit:
	PVROSModeSetDeinit(psDevInfo);

ErrorCursorDeinit:
	CursorDeinit(psDevInfo);
	GTTDeinit(psDevInfo, &psDevInfo->sGTTInfo);

ErrorUnmapRegisters:
	PVROSUnMapPhysAddr(psDevInfo->pvRegCPUVAddr);

	return eReturn;
}

/* The common PSB driver de-initialisation function */
PSB_ERROR PVRPSBDeinit(PVRPSB_DEVINFO *psDevInfo)
{
	PVRSRV_DC_DISP2SRV_KMJTABLE *psPVRJTable = &psDevInfo->sPVRJTable;
	PSB_ERROR eReturn = PSB_OK;

#if !defined(POULSBO_DEVICE_POWER)
	/* Unregister with services power manager */
	if (psPVRJTable->pfnPVRSRVRegisterPowerDevice(psDevInfo->ui32ID, 
						      IMG_NULL, IMG_NULL, 
						      IMG_NULL, IMG_NULL, IMG_NULL, 
						      PVRSRV_DEV_POWER_STATE_ON, 
						      PVRSRV_DEV_POWER_STATE_ON) != PVRSRV_OK)
	{
		eReturn = PSB_ERROR_GENERIC;
	}
#endif /* defined(POULSBO_DEVICE_POWER) */

#if defined(SYS_USING_INTERRUPTS)
#if defined(POULSBO_DEVICE_ISR)
	/* Uninstall device specific ISR handler for PSB */
	if (UninstallVsyncISR(psDevInfo) != PSB_OK)
	{
		eReturn = PSB_ERROR_GENERIC;
	}
#else
	/* Remove registration with external system ISR */
	if (psPVRJTable->pfnPVRSRVRegisterSystemISRHandler(IMG_NULL, IMG_NULL, 0, 
							   psDevInfo->ui32ID) != PVRSRV_OK)
	{
		eReturn = PSB_ERROR_GENERIC;
	}
#endif /* defined(PSB_DEVICE_ISR) */
#endif /* defined(SYS_USING_INTERRUPTS) */

	/* Remove cmd handler registration */
	if (psPVRJTable->pfnPVRSRVRemoveCmdProcList(psDevInfo->ui32ID, 
						    PVRPSB_COMMAND_COUNT) != PVRSRV_OK)
	{
		eReturn = PSB_ERROR_GENERIC;
	}
	
	/* Remove display class device from kernel services device register */
	if (psPVRJTable->pfnPVRSRVRemoveDCDevice(psDevInfo->ui32ID) != PVRSRV_OK)
	{
		eReturn = PSB_ERROR_GENERIC;
	}

	BufferDeinit(psDevInfo);
	PVROSModeSetDeinit(psDevInfo);
	CursorDeinit(psDevInfo);
	GTTDeinit(psDevInfo, &psDevInfo->sGTTInfo);

	PVROSUnMapPhysAddr(psDevInfo->pvRegCPUVAddr);
	psDevInfo->pvRegCPUVAddr = NULL;

	if (psDevInfo->psSwapChain != NULL)
	{
		eReturn = PSB_ERROR_GENERIC;
	}

	return eReturn;
}


/******************************************************************************
 End of file (poulsbo_displayclass.c)
******************************************************************************/
