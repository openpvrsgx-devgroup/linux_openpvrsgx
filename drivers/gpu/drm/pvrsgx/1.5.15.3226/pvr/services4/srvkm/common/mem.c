/**********************************************************************
 Copyright (c) Imagination Technologies Ltd.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ******************************************************************************/

#include "services_headers.h"
#include "pvr_bridge_km.h"


static PVRSRV_ERROR
FreeSharedSysMemCallBack(IMG_PVOID	pvParam,
						 IMG_UINT32	ui32Param)
{
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo = pvParam;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	OSFreePages(psKernelMemInfo->ui32Flags,
				psKernelMemInfo->ui32AllocSize,
				psKernelMemInfo->pvLinAddrKM,
				psKernelMemInfo->sMemBlk.hOSMemHandle);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(PVRSRV_KERNEL_MEM_INFO),
			  psKernelMemInfo,
			  IMG_NULL);


	return PVRSRV_OK;
}


IMG_EXPORT PVRSRV_ERROR
PVRSRVAllocSharedSysMemoryKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
							 IMG_UINT32					ui32Flags,
							 IMG_SIZE_T 				ui32Size,
							 PVRSRV_KERNEL_MEM_INFO 	**ppsKernelMemInfo)
{
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;

	if(OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(PVRSRV_KERNEL_MEM_INFO),
				  (IMG_VOID **)&psKernelMemInfo, IMG_NULL,
				  "Kernel Memory Info") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVAllocSharedSysMemoryKM: Failed to alloc memory for meminfo"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	OSMemSet(psKernelMemInfo, 0, sizeof(*psKernelMemInfo));

	ui32Flags &= ~PVRSRV_HAP_MAPTYPE_MASK;
	ui32Flags |= PVRSRV_HAP_MULTI_PROCESS;
	psKernelMemInfo->ui32Flags = ui32Flags;
	psKernelMemInfo->ui32AllocSize = ui32Size;

	if(OSAllocPages(psKernelMemInfo->ui32Flags,
					psKernelMemInfo->ui32AllocSize,
					HOST_PAGESIZE(),
					&psKernelMemInfo->pvLinAddrKM,
					&psKernelMemInfo->sMemBlk.hOSMemHandle)
		!= PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocSharedSysMemoryKM: Failed to alloc memory for block"));
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(PVRSRV_KERNEL_MEM_INFO),
				  psKernelMemInfo,
				  0);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}


	psKernelMemInfo->sMemBlk.hResItem =
				ResManRegisterRes(psPerProc->hResManContext,
								  RESMAN_TYPE_SHARED_MEM_INFO,
								  psKernelMemInfo,
								  0,
								  FreeSharedSysMemCallBack);

	*ppsKernelMemInfo = psKernelMemInfo;

	return PVRSRV_OK;
}


IMG_EXPORT PVRSRV_ERROR
PVRSRVFreeSharedSysMemoryKM(PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo)
{
	PVRSRV_ERROR eError;

	if(psKernelMemInfo->sMemBlk.hResItem)
	{
		eError = ResManFreeResByPtr(psKernelMemInfo->sMemBlk.hResItem);
	}
	else
	{
		eError = FreeSharedSysMemCallBack(psKernelMemInfo, 0);
	}

	return eError;
}


IMG_EXPORT PVRSRV_ERROR
PVRSRVDissociateMemFromResmanKM(PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	if(!psKernelMemInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(psKernelMemInfo->sMemBlk.hResItem)
	{
		eError = ResManDissociateRes(psKernelMemInfo->sMemBlk.hResItem, IMG_NULL);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"PVRSRVDissociateMemFromResmanKM: ResManDissociateRes failed"));
			PVR_DBG_BREAK;
			return eError;
		}

		psKernelMemInfo->sMemBlk.hResItem = IMG_NULL;
	}

	return eError;
}

