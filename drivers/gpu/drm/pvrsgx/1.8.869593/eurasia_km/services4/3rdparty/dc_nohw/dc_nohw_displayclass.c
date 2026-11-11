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

#if defined(__linux__)
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"

#include "dc_nohw.h"

#define DISPLAY_DEVICE_NAME "DC_NOHW"

#define DC_NOHW_COMMAND_COUNT		1

static void *gpvAnchor = 0;
static PFN_DC_GET_PVRJTABLE pfnGetPVRJTable = 0;




static DC_NOHW_DEVINFO * GetAnchorPtr(void)
{
	return (DC_NOHW_DEVINFO *)gpvAnchor;
}

static void SetAnchorPtr(DC_NOHW_DEVINFO *psDevInfo)
{
	gpvAnchor = (void *)psDevInfo;
}

#if !defined(DC_NOHW_DISCONTIG_BUFFERS) && !defined(USE_BASE_VIDEO_FRAMEBUFFER)
IMG_SYS_PHYADDR CpuPAddrToSysPAddr(IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;

	
	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}

IMG_CPU_PHYADDR SysPAddrToCpuPAddr(IMG_SYS_PHYADDR sys_paddr)
{
	IMG_CPU_PHYADDR cpu_paddr;

	
	cpu_paddr.uiAddr = sys_paddr.uiAddr;
	return cpu_paddr;
}
#endif


static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 ui32DeviceID,
                                 IMG_HANDLE *phDevice,
                                 PVRSRV_SYNC_DATA* psSystemBufferSyncData)
{
	DC_NOHW_DEVINFO *psDevInfo;
	PVR_UNREFERENCED_PARAMETER(ui32DeviceID);

	psDevInfo = GetAnchorPtr();

#if defined (ENABLE_DISPLAY_MODE_TRACKING)
	if (Shadow_Desktop_Resolution(psDevInfo) != DC_OK)
	{
		return (PVRSRV_ERROR_NOT_SUPPORTED);
	}
#endif

	
	psDevInfo->asBackBuffers[0].psSyncData = psSystemBufferSyncData;

	
	*phDevice = (IMG_HANDLE)psDevInfo;

#if defined(USE_BASE_VIDEO_FRAMEBUFFER)
	return (SetupDevInfo(psDevInfo));
#else
	return (PVRSRV_OK);
#endif
}


static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
	UNREFERENCED_PARAMETER(hDevice);

#if defined(USE_BASE_VIDEO_FRAMEBUFFER)
	FreeBackBuffers(GetAnchorPtr());
#endif

	return (PVRSRV_OK);
}


static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE		hDevice,
                                  IMG_UINT32		*pui32NumFormats,
                                  DISPLAY_FORMAT	*psFormat)
{
	DC_NOHW_DEVINFO	*psDevInfo;

	if(!hDevice || !pui32NumFormats)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO *)hDevice;

	*pui32NumFormats = (IMG_UINT32)psDevInfo->ulNumFormats;

	if(psFormat != IMG_NULL)
	{
		unsigned long i;

		for(i=0; i<psDevInfo->ulNumFormats; i++)
		{
			psFormat[i] = psDevInfo->asDisplayFormatList[i];
		}
	}

	return (PVRSRV_OK);
}


static PVRSRV_ERROR EnumDCDims(IMG_HANDLE		hDevice,
                               DISPLAY_FORMAT	*psFormat,
                               IMG_UINT32		*pui32NumDims,
                               DISPLAY_DIMS		*psDim)
{
	DC_NOHW_DEVINFO	*psDevInfo;

	if(!hDevice || !psFormat || !pui32NumDims)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO *)hDevice;

	*pui32NumDims = (IMG_UINT32)psDevInfo->ulNumDims;

	

	if(psDim != IMG_NULL)
	{
		unsigned long i;

		for(i=0; i<psDevInfo->ulNumDims; i++)
		{
			psDim[i] = psDevInfo->asDisplayDimList[i];
		}
	}

	return (PVRSRV_OK);
}


static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice, IMG_HANDLE *phBuffer)
{
	DC_NOHW_DEVINFO	*psDevInfo;

	if(!hDevice || !phBuffer)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO *)hDevice;

	*phBuffer = (IMG_HANDLE)&psDevInfo->asBackBuffers[0];

	return (PVRSRV_OK);
}


static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	DC_NOHW_DEVINFO	*psDevInfo;

	if(!hDevice || !psDCInfo)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO *)hDevice;

	*psDCInfo = psDevInfo->sDisplayInfo;

	return (PVRSRV_OK);
}


static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE         hDevice,
                                    IMG_HANDLE         hBuffer,
                                    IMG_SYS_PHYADDR **ppsSysAddr,
                                    IMG_UINT32        *pui32ByteSize,
                                    IMG_VOID         **ppvCpuVAddr,
                                    IMG_HANDLE        *phOSMapInfo,
                                    IMG_BOOL          *pbIsContiguous,
                                    IMG_UINT32		  *pui32TilingStride)
{
	DC_NOHW_DEVINFO	*psDevInfo;
	DC_NOHW_BUFFER	*psBuffer;

	if(!hDevice || !hBuffer || !ppsSysAddr || !pui32ByteSize)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO *)hDevice;

	psBuffer = (DC_NOHW_BUFFER*)hBuffer;

	*ppvCpuVAddr = psBuffer->sCPUVAddr;

	*pui32ByteSize = (IMG_UINT32)(psDevInfo->asDisplayDimList[0].ui32Height * psDevInfo->asDisplayDimList[0].ui32ByteStride);
	*phOSMapInfo = IMG_NULL;

#if defined(DC_NOHW_DISCONTIG_BUFFERS)
	*ppsSysAddr = psBuffer->psSysAddr;
	*pbIsContiguous = IMG_FALSE;
#else
	*ppsSysAddr = &psBuffer->sSysAddr;
	*pbIsContiguous = IMG_TRUE;
#endif

#if defined(SUPPORT_MEMORY_TILING)
	{
		IMG_UINT32 ui32Stride = psDevInfo->asDisplayDimList[0].ui32ByteStride;
		IMG_UINT32 ui32NumBits = 0, ui32StrideTopBit, n;

		
		for(n = 0; n < 32; n++)
		{
			if(ui32Stride & (1<<n))
			{
				ui32NumBits = n+1;
			}
		}

		
		if(ui32NumBits < 10)
		{
			ui32NumBits = 10;
		}

		
		ui32StrideTopBit = ui32NumBits - 1;

		
		ui32StrideTopBit -= 9;

		*pui32TilingStride = ui32StrideTopBit;
	}
#else
	UNREFERENCED_PARAMETER(pui32TilingStride);
#endif 

	return (PVRSRV_OK);
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
	DC_NOHW_DEVINFO	*psDevInfo;
	DC_NOHW_SWAPCHAIN *psSwapChain;
	DC_NOHW_BUFFER *psBuffer;
	IMG_UINT32 i;

	UNREFERENCED_PARAMETER(ui32OEMFlags);
	UNREFERENCED_PARAMETER(pui32SwapChainID);

	
	if(!hDevice
	|| !psDstSurfAttrib
	|| !psSrcSurfAttrib
	|| !ppsSyncData
	|| !phSwapChain)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO*)hDevice;

	
	if(psDevInfo->psSwapChain)
	{
		return (PVRSRV_ERROR_FLIP_CHAIN_EXISTS);
	}

	
	if(ui32BufferCount > DC_NOHW_MAX_BACKBUFFERS)
	{
		return (PVRSRV_ERROR_TOOMANYBUFFERS);
	}

	


	if(psDstSurfAttrib->pixelformat != psDevInfo->sSysFormat.pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psDevInfo->sSysDims.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psDevInfo->sSysDims.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psDevInfo->sSysDims.ui32Height)
	{
		
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	if(psDstSurfAttrib->pixelformat != psSrcSurfAttrib->pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psSrcSurfAttrib->sDims.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psSrcSurfAttrib->sDims.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psSrcSurfAttrib->sDims.ui32Height)
	{
		
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	
	UNREFERENCED_PARAMETER(ui32Flags);

	
	psSwapChain = (DC_NOHW_SWAPCHAIN*)AllocKernelMem(sizeof(DC_NOHW_SWAPCHAIN));
	if(!psSwapChain)
	{
		return (PVRSRV_ERROR_OUT_OF_MEMORY);
	}

	psBuffer = (DC_NOHW_BUFFER*)AllocKernelMem(sizeof(DC_NOHW_BUFFER) * ui32BufferCount);
	if(!psBuffer)
	{
		FreeKernelMem(psSwapChain);
		return (PVRSRV_ERROR_OUT_OF_MEMORY);
	}

	
	memset(psSwapChain, 0, sizeof(DC_NOHW_SWAPCHAIN));
	memset(psBuffer, 0, sizeof(DC_NOHW_BUFFER) * ui32BufferCount);

	psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
	psSwapChain->psBuffer = psBuffer;

	
	for(i=0; i<ui32BufferCount-1; i++)
	{
		psBuffer[i].psNext = &psBuffer[i+1];
	}
	
	psBuffer[i].psNext = &psBuffer[0];

	
	for(i=0; i<ui32BufferCount; i++)
	{
		psBuffer[i].psSyncData = ppsSyncData[i];
#if defined(DC_NOHW_DISCONTIG_BUFFERS)
		psBuffer[i].psSysAddr = psDevInfo->asBackBuffers[i].psSysAddr;
#else
		psBuffer[i].sSysAddr = psDevInfo->asBackBuffers[i].sSysAddr;
#endif
		psBuffer[i].sDevVAddr = psDevInfo->asBackBuffers[i].sDevVAddr;
		psBuffer[i].sCPUVAddr = psDevInfo->asBackBuffers[i].sCPUVAddr;
		psBuffer[i].hSwapChain = (DC_HANDLE)psSwapChain;
	}

	
	psDevInfo->psSwapChain = psSwapChain;

	
	*phSwapChain = (IMG_HANDLE)psSwapChain;

	

	return (PVRSRV_OK);
}


static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,
                                       IMG_HANDLE hSwapChain)
{
	DC_NOHW_DEVINFO	*psDevInfo;
	DC_NOHW_SWAPCHAIN *psSwapChain;

	
	if(!hDevice
	|| !hSwapChain)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (DC_NOHW_DEVINFO*)hDevice;
	psSwapChain = (DC_NOHW_SWAPCHAIN*)hSwapChain;

	
	FreeKernelMem(psSwapChain->psBuffer);
	FreeKernelMem(psSwapChain);

	
	psDevInfo->psSwapChain = 0;

	

	return (PVRSRV_OK);
}


static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE	hDevice,
                                 IMG_HANDLE	hSwapChain,
                                 IMG_RECT	*psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	return (PVRSRV_ERROR_NOT_SUPPORTED);
}


static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE	hDevice,
                                 IMG_HANDLE	hSwapChain,
                                 IMG_RECT	*psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);

	return (PVRSRV_ERROR_NOT_SUPPORTED);
}


static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE	hDevice,
                                      IMG_HANDLE	hSwapChain,
                                      IMG_UINT32	ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	return (PVRSRV_ERROR_NOT_SUPPORTED);
}


static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE	hDevice,
                                      IMG_HANDLE	hSwapChain,
                                      IMG_UINT32	ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);

	return (PVRSRV_ERROR_NOT_SUPPORTED);
}


static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_UINT32 *pui32BufferCount,
                                 IMG_HANDLE *phBuffer)
{
	DC_NOHW_SWAPCHAIN *psSwapChain;
	unsigned long i;

	
	if(!hDevice
	|| !hSwapChain
	|| !pui32BufferCount
	|| !phBuffer)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psSwapChain = (DC_NOHW_SWAPCHAIN*)hSwapChain;

	
	*pui32BufferCount = (IMG_UINT32)psSwapChain->ulBufferCount;

	
	for(i=0; i<psSwapChain->ulBufferCount; i++)
	{
		phBuffer[i] = (IMG_HANDLE)&psSwapChain->psBuffer[i];
	}

	return (PVRSRV_OK);
}


static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE	hDevice,
                                   IMG_HANDLE	hBuffer,
                                   IMG_UINT32	ui32SwapInterval,
                                   IMG_HANDLE	hPrivateTag,
                                   IMG_UINT32	ui32ClipRectCount,
                                   IMG_RECT		*psClipRect)
{
	UNREFERENCED_PARAMETER(ui32SwapInterval);
	UNREFERENCED_PARAMETER(hPrivateTag);
	UNREFERENCED_PARAMETER(psClipRect);

	if(!hDevice
	|| !hBuffer
	|| (ui32ClipRectCount != 0))
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	
	return (PVRSRV_OK);
}


static PVRSRV_ERROR SwapToDCSystem(IMG_HANDLE hDevice,
                                   IMG_HANDLE hSwapChain)
{
	if(!hDevice
	|| !hSwapChain)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	
	return (PVRSRV_OK);
}


static DC_ERROR Flip(DC_NOHW_DEVINFO	*psDevInfo,
                     DC_NOHW_BUFFER		*psBuffer)
{
	
	if(!psDevInfo || !psBuffer)
	{
		return (DC_ERROR_INVALID_PARAMS);
	}
	

	return (DC_OK);
}


static IMG_BOOL ProcessFlip(IMG_HANDLE	hCmdCookie,
                            IMG_UINT32	ui32DataSize,
                            IMG_VOID	*pvData)
{
	DC_ERROR eError;
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	DC_NOHW_DEVINFO	*psDevInfo;
	DC_NOHW_BUFFER	*psBuffer;

	
	if(!hCmdCookie)
	{
		return (IMG_FALSE);
	}

	
	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND*)pvData;
	if (psFlipCmd == IMG_NULL || sizeof(DISPLAYCLASS_FLIP_COMMAND) != ui32DataSize)
	{
		return (IMG_FALSE);
	}

	
	psDevInfo = (DC_NOHW_DEVINFO*)psFlipCmd->hExtDevice;

	psBuffer = (DC_NOHW_BUFFER*)psFlipCmd->hExtBuffer;

	
	eError = Flip(psDevInfo, psBuffer);
	if(eError != DC_OK)
	{
		return (IMG_FALSE);
	}

	
	psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete(hCmdCookie, IMG_FALSE);

	return (IMG_TRUE);
}


DC_ERROR Init(void)
{
	DC_NOHW_DEVINFO *psDevInfo;
	DC_ERROR         eError;
	unsigned long    ulBBuf;
	unsigned long    ulNBBuf;
	



	



	

	psDevInfo = GetAnchorPtr();

	if (psDevInfo == 0)
	{
		PFN_CMD_PROC  pfnCmdProcList[DC_NOHW_COMMAND_COUNT];
		IMG_UINT32    aui32SyncCountList[DC_NOHW_COMMAND_COUNT][2];

		
		psDevInfo = (DC_NOHW_DEVINFO *)AllocKernelMem(sizeof(*psDevInfo));

		if(!psDevInfo)
		{
			eError = DC_ERROR_OUT_OF_MEMORY;
			goto ExitError;
		}

		
		memset(psDevInfo, 0, sizeof(*psDevInfo));

		
		SetAnchorPtr((void*)psDevInfo);

		
		psDevInfo->ulRefCount = 0UL;

		if(OpenPVRServices(&psDevInfo->hPVRServices) != DC_OK)
		{
			eError = DC_ERROR_INIT_FAILURE;
			goto ExitFreeDevInfo;
		}
		if(GetLibFuncAddr (psDevInfo->hPVRServices, "PVRGetDisplayClassJTable", &pfnGetPVRJTable) != DC_OK)
		{
			eError = DC_ERROR_INIT_FAILURE;
			goto ExitCloseServices;
		}

		
		if((*pfnGetPVRJTable)(&psDevInfo->sPVRJTable) == IMG_FALSE)
		{
			eError = DC_ERROR_INIT_FAILURE;
			goto ExitCloseServices;
		}

		

		psDevInfo->psSwapChain = 0;
		psDevInfo->sDisplayInfo.ui32MinSwapInterval = 0UL;
		psDevInfo->sDisplayInfo.ui32MaxSwapInterval = 1UL;
		psDevInfo->sDisplayInfo.ui32MaxSwapChains = 1UL;
		psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers = DC_NOHW_MAX_BACKBUFFERS;
		strncpy(psDevInfo->sDisplayInfo.szDisplayName, DISPLAY_DEVICE_NAME, MAX_DISPLAY_NAME_SIZE);

		psDevInfo->ulNumFormats = 1UL;

		psDevInfo->ulNumDims = 1UL;

#if defined(DC_NOHW_GET_BUFFER_DIMENSIONS)
		if (!GetBufferDimensions(&psDevInfo->asDisplayDimList[0].ui32Width,
			&psDevInfo->asDisplayDimList[0].ui32Height,
			&psDevInfo->asDisplayFormatList[0].pixelformat,
			&psDevInfo->asDisplayDimList[0].ui32ByteStride))
		{
			eError = DC_ERROR_INIT_FAILURE;
			goto ExitCloseServices;
		}
#else	
	#if defined (ENABLE_DISPLAY_MODE_TRACKING)
		
		psDevInfo->asDisplayFormatList[0].pixelformat = DC_NOHW_BUFFER_PIXEL_FORMAT;
		psDevInfo->asDisplayDimList[0].ui32Width =  0;
		psDevInfo->asDisplayDimList[0].ui32Height =  0;
		psDevInfo->asDisplayDimList[0].ui32ByteStride = 0;
	#else
		psDevInfo->asDisplayFormatList[0].pixelformat = DC_NOHW_BUFFER_PIXEL_FORMAT;
		psDevInfo->asDisplayDimList[0].ui32Width =  DC_NOHW_BUFFER_WIDTH;
		psDevInfo->asDisplayDimList[0].ui32Height =  DC_NOHW_BUFFER_HEIGHT;
		psDevInfo->asDisplayDimList[0].ui32ByteStride = DC_NOHW_BUFFER_BYTE_STRIDE;
	#endif
#endif	

		psDevInfo->sSysFormat = psDevInfo->asDisplayFormatList[0];
		psDevInfo->sSysDims.ui32Width = psDevInfo->asDisplayDimList[0].ui32Width;
		psDevInfo->sSysDims.ui32Height = psDevInfo->asDisplayDimList[0].ui32Height;
		psDevInfo->sSysDims.ui32ByteStride = psDevInfo->asDisplayDimList[0].ui32ByteStride;
		psDevInfo->ui32BufferSize = psDevInfo->sSysDims.ui32Height * psDevInfo->sSysDims.ui32ByteStride;


		
		for(ulBBuf=0; ulBBuf<DC_NOHW_MAX_BACKBUFFERS; ulBBuf++)
		{
#if defined(USE_BASE_VIDEO_FRAMEBUFFER) || defined (ENABLE_DISPLAY_MODE_TRACKING)
			psDevInfo->asBackBuffers[ulBBuf].sSysAddr.uiAddr = IMG_NULL;
			psDevInfo->asBackBuffers[ulBBuf].sCPUVAddr = IMG_NULL;
#else
#if defined(DC_NOHW_DISCONTIG_BUFFERS)
			if (AllocDiscontigMemory(psDevInfo->ui32BufferSize,
								  &psDevInfo->asBackBuffers[ulBBuf].hMemChunk,
								  &psDevInfo->asBackBuffers[ulBBuf].sCPUVAddr,
								  &psDevInfo->asBackBuffers[ulBBuf].psSysAddr) != DC_OK)
			{
				eError = DC_ERROR_INIT_FAILURE;
				goto ExitFreeMem;
			}
#else
			IMG_CPU_PHYADDR		sBufferCPUPAddr;

			if (AllocContigMemory(psDevInfo->ui32BufferSize,
								  &psDevInfo->asBackBuffers[ulBBuf].hMemChunk,
								  &psDevInfo->asBackBuffers[ulBBuf].sCPUVAddr,
								  &sBufferCPUPAddr) != DC_OK)
			{
				eError = DC_ERROR_INIT_FAILURE;
				goto ExitFreeMem;
			}

			psDevInfo->asBackBuffers[ulBBuf].sSysAddr =  CpuPAddrToSysPAddr(sBufferCPUPAddr);
#endif
#endif 
			
			psDevInfo->asBackBuffers[ulBBuf].sDevVAddr.uiAddr = 0UL;
			psDevInfo->asBackBuffers[ulBBuf].hSwapChain = 0;
			psDevInfo->asBackBuffers[ulBBuf].psSyncData = 0;
			psDevInfo->asBackBuffers[ulBBuf].psNext = 0;
		}

		

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
		psDevInfo->sDCJTable.pfnSetDCState = IMG_NULL;

		
		if(psDevInfo->sPVRJTable.pfnPVRSRVRegisterDCDevice (&psDevInfo->sDCJTable,
															&psDevInfo->uiDeviceID ) != PVRSRV_OK)
		{
			eError = DC_ERROR_DEVICE_REGISTER_FAILED;
			goto ExitFreeMem;
		}

		

		pfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;

		

		aui32SyncCountList[DC_FLIP_COMMAND][0] = 0UL;
		aui32SyncCountList[DC_FLIP_COMMAND][1] = 2UL;

		



		if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList(psDevInfo->uiDeviceID,
															   &pfnCmdProcList[0],
															   aui32SyncCountList,
															   DC_NOHW_COMMAND_COUNT) != PVRSRV_OK)
		{
			eError = DC_ERROR_CANT_REGISTER_CALLBACK;
			goto ExitRemoveDevice;
		}
	}

	
	psDevInfo->ulRefCount++;

	
	return (DC_OK);

ExitRemoveDevice:
	(IMG_VOID) psDevInfo->sPVRJTable.pfnPVRSRVRemoveDCDevice(psDevInfo->uiDeviceID);

ExitFreeMem:
	ulNBBuf = ulBBuf;
	for(ulBBuf=0; ulBBuf<ulNBBuf; ulBBuf++)
	{
#if defined(DC_NOHW_DISCONTIG_BUFFERS)
		FreeDiscontigMemory(psDevInfo->ui32BufferSize,
			 psDevInfo->asBackBuffers[ulBBuf].hMemChunk,
			 psDevInfo->asBackBuffers[ulBBuf].sCPUVAddr,
			 psDevInfo->asBackBuffers[ulBBuf].psSysAddr);
#else
#if !defined(USE_BASE_VIDEO_FRAMEBUFFER)

		FreeContigMemory(psDevInfo->ui32BufferSize,
			 psDevInfo->asBackBuffers[ulBBuf].hMemChunk,
			 psDevInfo->asBackBuffers[ulBBuf].sCPUVAddr,
			 SysPAddrToCpuPAddr(psDevInfo->asBackBuffers[ulBBuf].sSysAddr));


#endif 
#endif 
	}

ExitCloseServices:
	(void)ClosePVRServices(psDevInfo->hPVRServices);

ExitFreeDevInfo:
	FreeKernelMem(psDevInfo);
	SetAnchorPtr(0);

ExitError:
	return eError;
}



DC_ERROR Deinit(void)
{
	DC_NOHW_DEVINFO *psDevInfo, *psDevFirst;
#if !defined(USE_BASE_VIDEO_FRAMEBUFFER)
	unsigned long i;
#endif

	psDevFirst = GetAnchorPtr();
	psDevInfo = psDevFirst;

	
	if (psDevInfo == 0)
	{
		return (DC_ERROR_GENERIC);
	}

	
	psDevInfo->ulRefCount--;

	if (psDevInfo->ulRefCount == 0UL)
	{
		
		PVRSRV_DC_DISP2SRV_KMJTABLE	*psJTable = &psDevInfo->sPVRJTable;

		
		if (psJTable->pfnPVRSRVRemoveDCDevice((IMG_UINT32)psDevInfo->uiDeviceID) != PVRSRV_OK)
		{
			return (DC_ERROR_GENERIC);
		}

		if (psDevInfo->sPVRJTable.pfnPVRSRVRemoveCmdProcList(psDevInfo->uiDeviceID,
															 DC_NOHW_COMMAND_COUNT) != PVRSRV_OK)
		{
			return (DC_ERROR_GENERIC);
		}

		if (ClosePVRServices(psDevInfo->hPVRServices) != DC_OK)
		{
			psDevInfo->hPVRServices = 0;
			return (DC_ERROR_GENERIC);
		}

#if !defined(USE_BASE_VIDEO_FRAMEBUFFER)
		for(i=0; i<DC_NOHW_MAX_BACKBUFFERS; i++)
		{
			if (psDevInfo->asBackBuffers[i].sCPUVAddr)
			{
				#if defined(DC_NOHW_DISCONTIG_BUFFERS)
				FreeDiscontigMemory(psDevInfo->ui32BufferSize,
							 psDevInfo->asBackBuffers[i].hMemChunk,
							 psDevInfo->asBackBuffers[i].sCPUVAddr,
							 psDevInfo->asBackBuffers[i].psSysAddr);
				#else

				FreeContigMemory(psDevInfo->ui32BufferSize,
							 psDevInfo->asBackBuffers[i].hMemChunk,
							 psDevInfo->asBackBuffers[i].sCPUVAddr,
							 SysPAddrToCpuPAddr(psDevInfo->asBackBuffers[i].sSysAddr));
				#endif
			}
		}
#endif 

		
		FreeKernelMem(psDevInfo);
	}

#if defined (ENABLE_DISPLAY_MODE_TRACKING)
	CloseMiniport();
#endif
	
	SetAnchorPtr(0);

	
	return (DC_OK);
}

