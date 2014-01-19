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

#ifndef _MMU_H_
#define _MMU_H_

#include "sgxinfokm.h"

PVRSRV_ERROR
MMU_Initialise (PVRSRV_DEVICE_NODE *psDeviceNode, MMU_CONTEXT **ppsMMUContext, IMG_DEV_PHYADDR *psPDDevPAddr);

IMG_VOID
MMU_Finalise (MMU_CONTEXT *psMMUContext);


IMG_VOID
MMU_InsertHeap(MMU_CONTEXT *psMMUContext, MMU_HEAP *psMMUHeap);

MMU_HEAP *
MMU_Create (MMU_CONTEXT *psMMUContext,
			DEV_ARENA_DESCRIPTOR *psDevArena,
			RA_ARENA **ppsVMArena);

IMG_VOID
MMU_Delete (MMU_HEAP *pMMU);

IMG_BOOL
MMU_Alloc (MMU_HEAP *pMMU,
           IMG_SIZE_T uSize,
           IMG_SIZE_T *pActualSize,
           IMG_UINT32 uFlags,
		   IMG_UINT32 uDevVAddrAlignment,
           IMG_DEV_VIRTADDR *pDevVAddr);

IMG_VOID
MMU_Free (MMU_HEAP *pMMU,
          IMG_DEV_VIRTADDR DevVAddr,
		  IMG_UINT32 ui32Size);

IMG_VOID
MMU_Enable (MMU_HEAP *pMMU);

IMG_VOID
MMU_Disable (MMU_HEAP *pMMU);

IMG_VOID
MMU_MapPages (MMU_HEAP *pMMU,
			  IMG_DEV_VIRTADDR devVAddr,
			  IMG_SYS_PHYADDR SysPAddr,
			  IMG_SIZE_T uSize,
			  IMG_UINT32 ui32MemFlags,
			  IMG_HANDLE hUniqueTag);

IMG_VOID
MMU_MapShadow (MMU_HEAP          * pMMU,
               IMG_DEV_VIRTADDR    MapBaseDevVAddr,
               IMG_SIZE_T          uSize,
               IMG_CPU_VIRTADDR    CpuVAddr,
               IMG_HANDLE          hOSMemHandle,
               IMG_DEV_VIRTADDR  * pDevVAddr,
               IMG_UINT32          ui32MemFlags,
               IMG_HANDLE          hUniqueTag);

IMG_VOID
MMU_UnmapPages (MMU_HEAP *pMMU,
             IMG_DEV_VIRTADDR dev_vaddr,
             IMG_UINT32 ui32PageCount,
             IMG_HANDLE hUniqueTag);

IMG_VOID
MMU_MapScatter (MMU_HEAP *pMMU,
				IMG_DEV_VIRTADDR DevVAddr,
				IMG_SYS_PHYADDR *psSysAddr,
				IMG_SIZE_T uSize,
				IMG_UINT32 ui32MemFlags,
				IMG_HANDLE hUniqueTag);


IMG_DEV_PHYADDR
MMU_GetPhysPageAddr(MMU_HEAP *pMMUHeap, IMG_DEV_VIRTADDR sDevVPageAddr);


IMG_DEV_PHYADDR
MMU_GetPDDevPAddr(MMU_CONTEXT *pMMUContext);


#ifdef SUPPORT_SGX_MMU_BYPASS
IMG_VOID
EnableHostAccess (MMU_CONTEXT *psMMUContext);


IMG_VOID
DisableHostAccess (MMU_CONTEXT *psMMUContext);
#endif

IMG_VOID MMU_InvalidateDirectoryCache(PVRSRV_SGXDEV_INFO *psDevInfo);

PVRSRV_ERROR MMU_BIFResetPDAlloc(PVRSRV_SGXDEV_INFO *psDevInfo);

IMG_VOID MMU_BIFResetPDFree(PVRSRV_SGXDEV_INFO *psDevInfo);

#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030) && defined(SGX_FEATURE_HOST_PORT)
PVRSRV_ERROR WorkaroundBRN22997Alloc(PVRSRV_SGXDEV_INFO *psDevInfo);

IMG_VOID WorkaroundBRN22997ReadHostPort(PVRSRV_SGXDEV_INFO *psDevInfo);

IMG_VOID WorkaroundBRN22997Free(PVRSRV_SGXDEV_INFO *psDevInfo);
#endif

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
PVRSRV_ERROR MMU_MapExtSystemCacheRegs(PVRSRV_DEVICE_NODE *psDeviceNode);

PVRSRV_ERROR MMU_UnmapExtSystemCacheRegs(PVRSRV_DEVICE_NODE *psDeviceNode);
#endif

#endif
