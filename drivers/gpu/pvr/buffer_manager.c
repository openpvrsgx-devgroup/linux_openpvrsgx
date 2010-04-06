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
#include "services_headers.h"

#include "sysconfig.h"
#include "hash.h"
#include "ra.h"
#include "pdump_km.h"

#define MIN(a, b)       (a > b ? b : a)

static IMG_BOOL ZeroBuf(struct BM_BUF *pBuf, struct BM_MAPPING *pMapping,
		u32 ui32Bytes, u32 ui32Flags);
static void BM_FreeMemory(void *pH, u32 base, struct BM_MAPPING *psMapping);
static IMG_BOOL BM_ImportMemory(void *pH, size_t uSize,
		size_t *pActualSize, struct BM_MAPPING **ppsMapping, u32 uFlags,
		u32 *pBase);

static IMG_BOOL DevMemoryAlloc(struct BM_CONTEXT *pBMContext,
		struct BM_MAPPING *pMapping, u32 uFlags,
		u32 dev_vaddr_alignment, struct IMG_DEV_VIRTADDR *pDevVAddr);
static void DevMemoryFree(struct BM_MAPPING *pMapping);

static IMG_BOOL AllocMemory(struct BM_CONTEXT *pBMContext,
		struct BM_HEAP *psBMHeap, struct IMG_DEV_VIRTADDR *psDevVAddr,
		size_t uSize, u32 uFlags, u32 uDevVAddrAlignment,
		struct BM_BUF *pBuf)
{
	struct BM_MAPPING *pMapping;
	u32 uOffset;
	struct RA_ARENA *pArena = NULL;

	PVR_DPF(PVR_DBG_MESSAGE, "AllocMemory "
			"(pBMContext=%08X, uSize=0x%x, uFlags=0x%x, "
			"align=0x%x, pBuf=%08X)",
		 pBMContext, uSize, uFlags, uDevVAddrAlignment, pBuf);

	if (uFlags & PVRSRV_MEM_RAM_BACKED_ALLOCATION) {
		if (uFlags & PVRSRV_MEM_USER_SUPPLIED_DEVVADDR) {
			PVR_DPF(PVR_DBG_ERROR, "AllocMemory: "
				"combination of DevVAddr management and "
				"RAM backing mode unsupported");
			return IMG_FALSE;
		}

		if (psBMHeap->ui32Attribs &
		    (PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG |
		     PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG)) {
			pArena = psBMHeap->pImportArena;
		} else {
			PVR_DPF(PVR_DBG_ERROR, "AllocMemory: "
				"backing store type doesn't match heap");
			return IMG_FALSE;
		}

		if (!RA_Alloc(pArena, uSize, (void *)&pMapping, uFlags,
			      uDevVAddrAlignment,
			      (u32 *)&(pBuf->DevVAddr.uiAddr))) {
			PVR_DPF(PVR_DBG_ERROR,
				 "AllocMemory: RA_Alloc(0x%x) FAILED", uSize);
			return IMG_FALSE;
		}

		uOffset = pBuf->DevVAddr.uiAddr - pMapping->DevVAddr.uiAddr;
		if (pMapping->CpuVAddr) {
			pBuf->CpuVAddr =
			    (void *)((u32) pMapping->CpuVAddr + uOffset);
		} else {
			pBuf->CpuVAddr = NULL;
		}

		if (uSize == pMapping->uSize) {
			pBuf->hOSMemHandle = pMapping->hOSMemHandle;
		} else {
			if (OSGetSubMemHandle(pMapping->hOSMemHandle, uOffset,
					uSize, psBMHeap->ui32Attribs,
					&pBuf->hOSMemHandle) != PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR, "AllocMemory: "
						"OSGetSubMemHandle FAILED");
				return IMG_FALSE;
			}
		}

		pBuf->CpuPAddr.uiAddr = pMapping->CpuPAddr.uiAddr + uOffset;

		if (uFlags & PVRSRV_MEM_ZERO)
			if (!ZeroBuf(pBuf, pMapping, uSize,
				     psBMHeap->ui32Attribs | uFlags))
				return IMG_FALSE;
	} else {
		if (uFlags & PVRSRV_MEM_USER_SUPPLIED_DEVVADDR) {
			PVR_ASSERT(psDevVAddr != NULL);

			if (psDevVAddr == NULL) {
				PVR_DPF(PVR_DBG_ERROR, "AllocMemory: "
					"invalid parameter - psDevVAddr");
				return IMG_FALSE;
			}

			pBMContext->psDeviceNode->pfnMMUAlloc(
					      psBMHeap->pMMUHeap, uSize,
					      PVRSRV_MEM_USER_SUPPLIED_DEVVADDR,
					      uDevVAddrAlignment, psDevVAddr);
			pBuf->DevVAddr = *psDevVAddr;
		} else {
			pBMContext->psDeviceNode->pfnMMUAlloc(psBMHeap->
							     pMMUHeap, uSize, 0,
							     uDevVAddrAlignment,
							     &pBuf->DevVAddr);
		}

		if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			       sizeof(struct BM_MAPPING),
			       (void **)&pMapping, NULL) != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				 "AllocMemory: OSAllocMem(0x%x) FAILED");
			return IMG_FALSE;
		}

		pBuf->CpuVAddr = NULL;
		pBuf->hOSMemHandle = NULL;
		pBuf->CpuPAddr.uiAddr = 0;

		pMapping->CpuVAddr = NULL;
		pMapping->CpuPAddr.uiAddr = 0;
		pMapping->DevVAddr = pBuf->DevVAddr;
		pMapping->psSysAddr = NULL;
		pMapping->uSize = uSize;
		pMapping->hOSMemHandle = NULL;
	}

	pMapping->pArena = pArena;

	pMapping->pBMHeap = psBMHeap;
	pBuf->pMapping = pMapping;

	PVR_DPF(PVR_DBG_MESSAGE, "AllocMemory: "
		"pMapping=%08X: DevV=%08X CpuV=%08X CpuP=%08X uSize=0x%x",
		 pMapping, pMapping->DevVAddr.uiAddr, pMapping->CpuVAddr,
		 pMapping->CpuPAddr.uiAddr, pMapping->uSize);

	PVR_DPF(PVR_DBG_MESSAGE, "AllocMemory: "
		 "pBuf=%08X: DevV=%08X CpuV=%08X CpuP=%08X uSize=0x%x",
		 pBuf, pBuf->DevVAddr.uiAddr, pBuf->CpuVAddr,
		 pBuf->CpuPAddr.uiAddr, uSize);

	PVR_ASSERT(((pBuf->DevVAddr.uiAddr) & (uDevVAddrAlignment - 1)) == 0);

	return IMG_TRUE;
}

static IMG_BOOL WrapMemory(struct BM_HEAP *psBMHeap,
	   size_t uSize, u32 ui32BaseOffset, IMG_BOOL bPhysContig,
	   struct IMG_SYS_PHYADDR *psAddr, void *pvCPUVAddr, u32 uFlags,
	   struct BM_BUF *pBuf)
{
	struct IMG_DEV_VIRTADDR DevVAddr = { 0 };
	struct BM_MAPPING *pMapping;
	IMG_BOOL bResult;
	u32 const ui32PageSize = HOST_PAGESIZE();

	PVR_DPF(PVR_DBG_MESSAGE,
		 "WrapMemory(psBMHeap=%08X, size=0x%x, offset=0x%x, "
		 "bPhysContig=0x%x, pvCPUVAddr = 0x%x, flags=0x%x, pBuf=%08X)",
		 psBMHeap, uSize, ui32BaseOffset, bPhysContig, pvCPUVAddr,
		 uFlags, pBuf);

	PVR_ASSERT((psAddr->uiAddr & (ui32PageSize - 1)) == 0);

	PVR_ASSERT(((u32) pvCPUVAddr & (ui32PageSize - 1)) == 0);

	uSize += ui32BaseOffset;
	uSize = HOST_PAGEALIGN(uSize);

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*pMapping),
		       (void **)&pMapping, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "WrapMemory: OSAllocMem(0x%x) FAILED",
			 sizeof(*pMapping));
		return IMG_FALSE;
	}

	OSMemSet(pMapping, 0, sizeof(*pMapping));

	pMapping->uSize = uSize;
	pMapping->pBMHeap = psBMHeap;

	if (pvCPUVAddr) {
		pMapping->CpuVAddr = pvCPUVAddr;

		if (bPhysContig) {
			pMapping->eCpuMemoryOrigin = hm_wrapped_virtaddr;
			pMapping->CpuPAddr = SysSysPAddrToCpuPAddr(psAddr[0]);

			if (OSRegisterMem(pMapping->CpuPAddr,
					pMapping->CpuVAddr, pMapping->uSize,
					  uFlags, &pMapping->hOSMemHandle) !=
					PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR, "WrapMemory: "
					"OSRegisterMem Phys=0x%08X, "
					"CpuVAddr = 0x%08X, Size=%d) failed",
					 pMapping->CpuPAddr, pMapping->CpuVAddr,
					 pMapping->uSize);
				goto fail_cleanup;
			}
		} else {
			pMapping->eCpuMemoryOrigin =
			    hm_wrapped_scatter_virtaddr;
			pMapping->psSysAddr = psAddr;

			if (OSRegisterDiscontigMem(pMapping->psSysAddr,
						   pMapping->CpuVAddr,
						   pMapping->uSize,
						   uFlags,
						   &pMapping->hOSMemHandle) !=
			    PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR, "WrapMemory: "
					"OSRegisterDiscontigMem CpuVAddr = "
					"0x%08X, Size=%d) failed",
					 pMapping->CpuVAddr, pMapping->uSize);
				goto fail_cleanup;
			}
		}
	} else {
		if (bPhysContig) {
			pMapping->eCpuMemoryOrigin = hm_wrapped;
			pMapping->CpuPAddr = SysSysPAddrToCpuPAddr(psAddr[0]);

			if (OSReservePhys(pMapping->CpuPAddr, pMapping->uSize,
					uFlags, &pMapping->CpuVAddr,
					&pMapping->hOSMemHandle) != PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR, "WrapMemory: "
					"OSReservePhys Phys=0x%08X, Size=%d) "
					"failed",
					 pMapping->CpuPAddr, pMapping->uSize);
				goto fail_cleanup;
			}
		} else {
			pMapping->eCpuMemoryOrigin = hm_wrapped_scatter;
			pMapping->psSysAddr = psAddr;

			if (OSReserveDiscontigPhys(pMapping->psSysAddr,
						   pMapping->uSize, uFlags,
						   &pMapping->CpuVAddr,
						   &pMapping->hOSMemHandle) !=
			    PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR, "WrapMemory: "
				       "OSReserveDiscontigPhys Size=%d) failed",
					 pMapping->uSize);
				goto fail_cleanup;
			}
		}
	}

	bResult = DevMemoryAlloc(psBMHeap->pBMContext, pMapping,
				 uFlags | PVRSRV_MEM_READ | PVRSRV_MEM_WRITE,
				 ui32PageSize, &DevVAddr);
	if (!bResult) {
		PVR_DPF(PVR_DBG_ERROR,
			 "WrapMemory: DevMemoryAlloc(0x%x) failed",
			 pMapping->uSize);
		goto fail_cleanup;
	}

	pBuf->CpuPAddr.uiAddr = pMapping->CpuPAddr.uiAddr + ui32BaseOffset;
	if (!ui32BaseOffset)
		pBuf->hOSMemHandle = pMapping->hOSMemHandle;
	else
		if (OSGetSubMemHandle(pMapping->hOSMemHandle,
				      ui32BaseOffset,
				      (pMapping->uSize - ui32BaseOffset),
				      uFlags,
				      &pBuf->hOSMemHandle) != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				 "WrapMemory: OSGetSubMemHandle failed");
			goto fail_cleanup;
		}
	if (pMapping->CpuVAddr)
		pBuf->CpuVAddr = (void *)((u32) pMapping->CpuVAddr +
							     ui32BaseOffset);
	pBuf->DevVAddr.uiAddr = pMapping->DevVAddr.uiAddr + ui32BaseOffset;

	if (uFlags & PVRSRV_MEM_ZERO)
		if (!ZeroBuf(pBuf, pMapping, uSize, uFlags))
			return IMG_FALSE;

	PVR_DPF(PVR_DBG_MESSAGE, "DevVaddr.uiAddr=%08X", DevVAddr.uiAddr);
	PVR_DPF(PVR_DBG_MESSAGE, "WrapMemory: pMapping=%08X: DevV=%08X "
			"CpuV=%08X CpuP=%08X uSize=0x%x",
		 pMapping, pMapping->DevVAddr.uiAddr, pMapping->CpuVAddr,
		 pMapping->CpuPAddr.uiAddr, pMapping->uSize);
	PVR_DPF(PVR_DBG_MESSAGE, "WrapMemory: pBuf=%08X: DevV=%08X "
				"CpuV=%08X CpuP=%08X uSize=0x%x",
		 pBuf, pBuf->DevVAddr.uiAddr, pBuf->CpuVAddr,
		 pBuf->CpuPAddr.uiAddr, uSize);

	pBuf->pMapping = pMapping;
	return IMG_TRUE;

fail_cleanup:
	if (ui32BaseOffset && pBuf->hOSMemHandle)
		OSReleaseSubMemHandle(pBuf->hOSMemHandle, uFlags);

	if (pMapping && (pMapping->CpuVAddr || pMapping->hOSMemHandle))
		switch (pMapping->eCpuMemoryOrigin) {
		case hm_wrapped:
			OSUnReservePhys(pMapping->CpuVAddr, pMapping->uSize,
					uFlags, pMapping->hOSMemHandle);
			break;
		case hm_wrapped_virtaddr:
			OSUnRegisterMem(pMapping->CpuVAddr, pMapping->uSize,
					uFlags, pMapping->hOSMemHandle);
			break;
		case hm_wrapped_scatter:
			OSUnReserveDiscontigPhys(pMapping->CpuVAddr,
						 pMapping->uSize, uFlags,
						 pMapping->hOSMemHandle);
			break;
		case hm_wrapped_scatter_virtaddr:
			OSUnRegisterDiscontigMem(pMapping->CpuVAddr,
						 pMapping->uSize, uFlags,
						 pMapping->hOSMemHandle);
			break;
		default:
			break;
		}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_MAPPING), pMapping,
		  NULL);

	return IMG_FALSE;
}

static IMG_BOOL ZeroBuf(struct BM_BUF *pBuf, struct BM_MAPPING *pMapping,
			u32 ui32Bytes, u32 ui32Flags)
{
	void *pvCpuVAddr;

	if (pBuf->CpuVAddr) {
		OSMemSet(pBuf->CpuVAddr, 0, ui32Bytes);
	} else if (pMapping->eCpuMemoryOrigin == hm_contiguous ||
		   pMapping->eCpuMemoryOrigin == hm_wrapped) {
		pvCpuVAddr = (void __force *)OSMapPhysToLin(pBuf->CpuPAddr,
					    ui32Bytes,
					    PVRSRV_HAP_KERNEL_ONLY |
					    (ui32Flags &
					       PVRSRV_HAP_CACHETYPE_MASK),
					    NULL);
		if (!pvCpuVAddr) {
			PVR_DPF(PVR_DBG_ERROR, "ZeroBuf: "
				"OSMapPhysToLin for contiguous buffer failed");
			return IMG_FALSE;
		}
		OSMemSet(pvCpuVAddr, 0, ui32Bytes);
		OSUnMapPhysToLin((void __force __iomem *)pvCpuVAddr, ui32Bytes,
				 PVRSRV_HAP_KERNEL_ONLY |
				    (ui32Flags & PVRSRV_HAP_CACHETYPE_MASK),
				 NULL);
	} else {
		u32 ui32BytesRemaining = ui32Bytes;
		u32 ui32CurrentOffset = 0;
		struct IMG_CPU_PHYADDR CpuPAddr;

		PVR_ASSERT(pBuf->hOSMemHandle);

		while (ui32BytesRemaining > 0) {
			u32 ui32BlockBytes =
			    MIN(ui32BytesRemaining, HOST_PAGESIZE());
			CpuPAddr =
			    OSMemHandleToCpuPAddr(pBuf->hOSMemHandle,
						  ui32CurrentOffset);

			if (CpuPAddr.uiAddr & (HOST_PAGESIZE() - 1))
				ui32BlockBytes =
				    MIN(ui32BytesRemaining,
					HOST_PAGEALIGN(CpuPAddr.uiAddr) -
					CpuPAddr.uiAddr);

			pvCpuVAddr = (void __force *)OSMapPhysToLin(CpuPAddr,
						    ui32BlockBytes,
						    PVRSRV_HAP_KERNEL_ONLY |
						     (ui32Flags &
						     PVRSRV_HAP_CACHETYPE_MASK),
						    NULL);
			if (!pvCpuVAddr) {
				PVR_DPF(PVR_DBG_ERROR, "ZeroBuf: "
					"OSMapPhysToLin while "
				       "zeroing non-contiguous memory FAILED");
				return IMG_FALSE;
			}
			OSMemSet(pvCpuVAddr, 0, ui32BlockBytes);
			OSUnMapPhysToLin((void __force __iomem *)pvCpuVAddr,
					 ui32BlockBytes,
					 PVRSRV_HAP_KERNEL_ONLY |
					   (ui32Flags &
					    PVRSRV_HAP_CACHETYPE_MASK),
					 NULL);

			ui32BytesRemaining -= ui32BlockBytes;
			ui32CurrentOffset += ui32BlockBytes;
		}
	}

	return IMG_TRUE;
}

static void FreeBuf(struct BM_BUF *pBuf, u32 ui32Flags)
{
	struct BM_MAPPING *pMapping;

	PVR_DPF(PVR_DBG_MESSAGE,
		"FreeBuf: pBuf=%08X: DevVAddr=%08X CpuVAddr=%08X CpuPAddr=%08X",
		 pBuf, pBuf->DevVAddr.uiAddr, pBuf->CpuVAddr,
		 pBuf->CpuPAddr.uiAddr);

	pMapping = pBuf->pMapping;

	if (ui32Flags & PVRSRV_MEM_USER_SUPPLIED_DEVVADDR) {
		if (ui32Flags & PVRSRV_MEM_RAM_BACKED_ALLOCATION)
			PVR_DPF(PVR_DBG_ERROR, "FreeBuf: "
				"combination of DevVAddr management "
				"and RAM backing mode unsupported");
		else
			OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
					sizeof(struct BM_MAPPING),
				  pMapping, NULL);
	} else {
		if (pBuf->hOSMemHandle != pMapping->hOSMemHandle)
			OSReleaseSubMemHandle(pBuf->hOSMemHandle, ui32Flags);
		if (ui32Flags & PVRSRV_MEM_RAM_BACKED_ALLOCATION) {
			RA_Free(pBuf->pMapping->pArena, pBuf->DevVAddr.uiAddr,
				IMG_FALSE);
		} else {
			switch (pMapping->eCpuMemoryOrigin) {
			case hm_wrapped:
				OSUnReservePhys(pMapping->CpuVAddr,
						pMapping->uSize, ui32Flags,
						pMapping->hOSMemHandle);
				break;
			case hm_wrapped_virtaddr:
				OSUnRegisterMem(pMapping->CpuVAddr,
						pMapping->uSize, ui32Flags,
						pMapping->hOSMemHandle);
				break;
			case hm_wrapped_scatter:
				OSUnReserveDiscontigPhys(pMapping->CpuVAddr,
							 pMapping->uSize,
							 ui32Flags,
							 pMapping->
							 hOSMemHandle);
				break;
			case hm_wrapped_scatter_virtaddr:
				OSUnRegisterDiscontigMem(pMapping->CpuVAddr,
							 pMapping->uSize,
							 ui32Flags,
							 pMapping->
							 hOSMemHandle);
				break;
			default:
				break;
			}

			DevMemoryFree(pMapping);

			OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(struct BM_MAPPING), pMapping, NULL);
		}
	}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_BUF), pBuf, NULL);
}

void BM_DestroyContext(void *hBMContext)
{
	struct BM_CONTEXT *pBMContext = (struct BM_CONTEXT *)hBMContext;
	struct BM_HEAP *psBMHeap;

	PVR_DPF(PVR_DBG_MESSAGE, "BM_DestroyContext");

	for (psBMHeap = pBMContext->psBMHeap;
	     psBMHeap != NULL; psBMHeap = psBMHeap->psNext)
		if (psBMHeap->ui32Attribs &
		    (PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG |
		      PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG))
			if (psBMHeap->pImportArena) {
				IMG_BOOL bTestDelete =
				    RA_TestDelete(psBMHeap->pImportArena);
				BUG_ON(!bTestDelete);
			}

	ResManFreeResByPtr(pBMContext->hResItem);
}

static enum PVRSRV_ERROR BM_DestroyContextCallBack(void *pvParam, u32 ui32Param)
{
	struct BM_CONTEXT *pBMContext = pvParam;
	struct BM_CONTEXT **ppBMContext;
	struct BM_HEAP *psBMHeap, *psTmpBMHeap;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	psDeviceNode = pBMContext->psDeviceNode;

	psBMHeap = pBMContext->psBMHeap;
	while (psBMHeap) {
		if (psBMHeap->ui32Attribs &
		    (PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG |
		      PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG)) {
			if (psBMHeap->pImportArena)
				RA_Delete(psBMHeap->pImportArena);
		} else {
			PVR_DPF(PVR_DBG_ERROR, "BM_DestroyContext: "
					   "backing store type unsupported");
			return PVRSRV_ERROR_GENERIC;
		}

		psDeviceNode->pfnMMUDelete(psBMHeap->pMMUHeap);

		psTmpBMHeap = psBMHeap;

		psBMHeap = psBMHeap->psNext;

		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_HEAP),
			  psTmpBMHeap, NULL);
	}

	if (pBMContext->psMMUContext)
		psDeviceNode->pfnMMUFinalise(pBMContext->psMMUContext);

	if (pBMContext->pBufferHash)
		HASH_Delete(pBMContext->pBufferHash);

	if (pBMContext == psDeviceNode->sDevMemoryInfo.pBMKernelContext) {
		psDeviceNode->sDevMemoryInfo.pBMKernelContext = NULL;
	} else {
		for (ppBMContext = &psDeviceNode->sDevMemoryInfo.pBMContext;
		     *ppBMContext; ppBMContext = &((*ppBMContext)->psNext))
			if (*ppBMContext == pBMContext) {
				*ppBMContext = pBMContext->psNext;
				break;
			}
	}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_CONTEXT),
		  pBMContext, NULL);

	return PVRSRV_OK;
}

void *BM_CreateContext(struct PVRSRV_DEVICE_NODE *psDeviceNode,
		 struct IMG_DEV_PHYADDR *psPDDevPAddr,
		 struct PVRSRV_PER_PROCESS_DATA *psPerProc, IMG_BOOL *pbCreated)
{
	struct BM_CONTEXT *pBMContext;
	struct BM_HEAP *psBMHeap;
	struct DEVICE_MEMORY_INFO *psDevMemoryInfo;
	IMG_BOOL bKernelContext;
	struct RESMAN_CONTEXT *hResManContext;

	PVR_DPF(PVR_DBG_MESSAGE, "BM_CreateContext");

	if (psPerProc == NULL) {
		bKernelContext = IMG_TRUE;
		hResManContext = psDeviceNode->hResManContext;
	} else {
		bKernelContext = IMG_FALSE;
		hResManContext = psPerProc->hResManContext;
	}

	if (pbCreated != NULL)
		*pbCreated = IMG_FALSE;

	psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;

	if (bKernelContext == IMG_FALSE)
		for (pBMContext = psDevMemoryInfo->pBMContext;
		     pBMContext != NULL; pBMContext = pBMContext->psNext)
			if (ResManFindResourceByPtr(hResManContext,
						     pBMContext->hResItem) ==
			    PVRSRV_OK) {
				pBMContext->ui32RefCount++;
				return (void *)pBMContext;
			}
	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_CONTEXT),
		       (void **)&pBMContext, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "BM_CreateContext: Alloc failed");
		return NULL;
	}
	OSMemSet(pBMContext, 0, sizeof(struct BM_CONTEXT));

	pBMContext->psDeviceNode = psDeviceNode;

	pBMContext->pBufferHash = HASH_Create(32);
	if (pBMContext->pBufferHash == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "BM_CreateContext: HASH_Create failed");
		goto cleanup;
	}

	if (psDeviceNode->pfnMMUInitialise(psDeviceNode,
					   &pBMContext->psMMUContext,
					   psPDDevPAddr) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "BM_CreateContext: MMUInitialise failed");
		goto cleanup;
	}

	if (bKernelContext) {
		PVR_ASSERT(psDevMemoryInfo->pBMKernelContext == NULL);
		psDevMemoryInfo->pBMKernelContext = pBMContext;
	} else {

		PVR_ASSERT(psDevMemoryInfo->pBMKernelContext);

		if (psDevMemoryInfo->pBMKernelContext == NULL) {
			PVR_DPF(PVR_DBG_ERROR, "BM_CreateContext: "
				"psDevMemoryInfo->pBMKernelContext invalid");
			goto cleanup;
		}

		PVR_ASSERT(psDevMemoryInfo->pBMKernelContext->psBMHeap);

		pBMContext->psBMSharedHeap =
		    psDevMemoryInfo->pBMKernelContext->psBMHeap;

		psBMHeap = pBMContext->psBMSharedHeap;
		while (psBMHeap) {
			switch (psBMHeap->sDevArena.DevMemHeapType) {
			case DEVICE_MEMORY_HEAP_SHARED:
			case DEVICE_MEMORY_HEAP_SHARED_EXPORTED:
				{
					psDeviceNode->
					    pfnMMUInsertHeap(pBMContext->
								 psMMUContext,
							     psBMHeap->
								 pMMUHeap);
					break;
				}
			}
			psBMHeap = psBMHeap->psNext;
		}
		pBMContext->psNext = psDevMemoryInfo->pBMContext;
		psDevMemoryInfo->pBMContext = pBMContext;
	}
	pBMContext->ui32RefCount++;
	pBMContext->hResItem = ResManRegisterRes(hResManContext,
						 RESMAN_TYPE_DEVICEMEM_CONTEXT,
						 pBMContext,
						 0, BM_DestroyContextCallBack);
	if (pBMContext->hResItem == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "BM_CreateContext: ResManRegisterRes failed");
		goto cleanup;
	}

	if (pbCreated != NULL)
		*pbCreated = IMG_TRUE;
	return (void *)pBMContext;

cleanup:
	BM_DestroyContextCallBack(pBMContext, 0);

	return NULL;
}

void *BM_CreateHeap(void *hBMContext,
		    struct DEVICE_MEMORY_HEAP_INFO *psDevMemHeapInfo)
{
	struct BM_CONTEXT *pBMContext = (struct BM_CONTEXT *)hBMContext;
	struct PVRSRV_DEVICE_NODE *psDeviceNode = pBMContext->psDeviceNode;
	struct BM_HEAP *psBMHeap;

	PVR_DPF(PVR_DBG_MESSAGE, "BM_CreateHeap");

	if (!pBMContext)
		return NULL;

	if (pBMContext->ui32RefCount > 0) {
		psBMHeap = pBMContext->psBMHeap;

		while (psBMHeap) {
			if (psBMHeap->sDevArena.ui32HeapID ==
			    psDevMemHeapInfo->ui32HeapID)

				return psBMHeap;
			psBMHeap = psBMHeap->psNext;
		}
	}

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_HEAP),
		       (void **) &psBMHeap, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "BM_CreateHeap: Alloc failed");
		return NULL;
	}

	OSMemSet(psBMHeap, 0, sizeof(struct BM_HEAP));

	psBMHeap->sDevArena.ui32HeapID = psDevMemHeapInfo->ui32HeapID;
	psBMHeap->sDevArena.pszName = psDevMemHeapInfo->pszName;
	psBMHeap->sDevArena.BaseDevVAddr = psDevMemHeapInfo->sDevVAddrBase;
	psBMHeap->sDevArena.ui32Size = psDevMemHeapInfo->ui32HeapSize;
	psBMHeap->sDevArena.DevMemHeapType = psDevMemHeapInfo->DevMemHeapType;
	psBMHeap->sDevArena.ui32DataPageSize =
	    psDevMemHeapInfo->ui32DataPageSize;
	psBMHeap->sDevArena.psDeviceMemoryHeapInfo = psDevMemHeapInfo;
	psBMHeap->ui32Attribs = psDevMemHeapInfo->ui32Attribs;

	psBMHeap->pBMContext = pBMContext;

	psBMHeap->pMMUHeap =
	    psDeviceNode->pfnMMUCreate(pBMContext->psMMUContext,
				       &psBMHeap->sDevArena,
				       &psBMHeap->pVMArena);
	if (!psBMHeap->pMMUHeap) {
		PVR_DPF(PVR_DBG_ERROR, "BM_CreateHeap: MMUCreate failed");
		goto ErrorExit;
	}

	psBMHeap->pImportArena = RA_Create(psDevMemHeapInfo->pszBSName,
					   0, 0, NULL,
					   psBMHeap->sDevArena.ui32DataPageSize,
					   BM_ImportMemory,
					   BM_FreeMemory, NULL, psBMHeap);
	if (psBMHeap->pImportArena == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "BM_CreateHeap: RA_Create failed");
		goto ErrorExit;
	}

	if (psBMHeap->ui32Attribs & PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG) {

		psBMHeap->pLocalDevMemArena =
		    psDevMemHeapInfo->psLocalDevMemArena;
		if (psBMHeap->pLocalDevMemArena == NULL) {
			PVR_DPF(PVR_DBG_ERROR,
				 "BM_CreateHeap: LocalDevMemArena null");
			goto ErrorExit;
		}
	}

	psBMHeap->psNext = pBMContext->psBMHeap;
	pBMContext->psBMHeap = psBMHeap;

	return (void *)psBMHeap;

ErrorExit:

	if (psBMHeap->pMMUHeap != NULL) {
		psDeviceNode->pfnMMUDelete(psBMHeap->pMMUHeap);
		psDeviceNode->pfnMMUFinalise(pBMContext->psMMUContext);
	}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_HEAP),
		  psBMHeap, NULL);

	return NULL;
}

void BM_DestroyHeap(void *hDevMemHeap)
{
	struct BM_HEAP *psBMHeap = (struct BM_HEAP *)hDevMemHeap;
	struct PVRSRV_DEVICE_NODE *psDeviceNode =
					psBMHeap->pBMContext->psDeviceNode;

	PVR_DPF(PVR_DBG_MESSAGE, "BM_DestroyHeap");

	if (psBMHeap) {
		struct BM_HEAP **ppsBMHeap;

		if (psBMHeap->ui32Attribs &
		    (PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG |
		      PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG)) {
			if (psBMHeap->pImportArena)
				RA_Delete(psBMHeap->pImportArena);
		} else {
			PVR_DPF(PVR_DBG_ERROR,
			"BM_DestroyHeap: backing store type unsupported");
			return;
		}

		psDeviceNode->pfnMMUDelete(psBMHeap->pMMUHeap);

		ppsBMHeap = &psBMHeap->pBMContext->psBMHeap;
		while (*ppsBMHeap) {
			if (*ppsBMHeap == psBMHeap) {
				*ppsBMHeap = psBMHeap->psNext;
				OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
					  sizeof(struct BM_HEAP), psBMHeap,
					  NULL);
				break;
			}
			ppsBMHeap = &((*ppsBMHeap)->psNext);
		}
	} else {
		PVR_DPF(PVR_DBG_ERROR, "BM_DestroyHeap: invalid heap handle");
	}
}

IMG_BOOL BM_Reinitialise(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_DPF(PVR_DBG_MESSAGE, "BM_Reinitialise");
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

	return IMG_TRUE;
}

IMG_BOOL BM_Alloc(void *hDevMemHeap, struct IMG_DEV_VIRTADDR *psDevVAddr,
		  size_t uSize, u32 *pui32Flags, u32 uDevVAddrAlignment,
		  void **phBuf)
{
	struct BM_BUF *pBuf;
	struct BM_CONTEXT *pBMContext;
	struct BM_HEAP *psBMHeap;
	struct SYS_DATA *psSysData;
	u32 uFlags;

	if (pui32Flags == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "BM_Alloc: invalid parameter");
		PVR_DBG_BREAK;
		return IMG_FALSE;
	}

	uFlags = *pui32Flags;

	PVR_DPF(PVR_DBG_MESSAGE,
		 "BM_Alloc (uSize=0x%x, uFlags=0x%x, uDevVAddrAlignment=0x%x)",
		 uSize, uFlags, uDevVAddrAlignment);

	if (SysAcquireData(&psSysData) != PVRSRV_OK)
		return IMG_FALSE;

	psBMHeap = (struct BM_HEAP *)hDevMemHeap;
	pBMContext = psBMHeap->pBMContext;

	if (uDevVAddrAlignment == 0)
		uDevVAddrAlignment = 1;

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_BUF),
		       (void **)&pBuf, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "BM_Alloc: BM_Buf alloc FAILED");
		return IMG_FALSE;
	}
	OSMemSet(pBuf, 0, sizeof(struct BM_BUF));

	if (AllocMemory(pBMContext, psBMHeap, psDevVAddr, uSize, uFlags,
			uDevVAddrAlignment, pBuf) != IMG_TRUE) {
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_BUF), pBuf,
			  NULL);
		PVR_DPF(PVR_DBG_ERROR, "BM_Alloc: AllocMemory FAILED");
		return IMG_FALSE;
	}

	PVR_DPF(PVR_DBG_MESSAGE, "BM_Alloc (uSize=0x%x, uFlags=0x%x)=%08X",
		 uSize, uFlags, pBuf);

	pBuf->ui32RefCount = 1;
	pvr_get_ctx(pBMContext);
	*phBuf = (void *) pBuf;
	*pui32Flags = uFlags | psBMHeap->ui32Attribs;

	return IMG_TRUE;
}

IMG_BOOL BM_Wrap(void *hDevMemHeap, u32 ui32Size, u32 ui32Offset,
		 IMG_BOOL bPhysContig, struct IMG_SYS_PHYADDR *psSysAddr,
		 void *pvCPUVAddr, u32 *pui32Flags, void **phBuf)
{
	struct BM_BUF *pBuf;
	struct BM_CONTEXT *psBMContext;
	struct BM_HEAP *psBMHeap;
	struct SYS_DATA *psSysData;
	struct IMG_SYS_PHYADDR sHashAddress;
	u32 uFlags;

	psBMHeap = (struct BM_HEAP *)hDevMemHeap;
	psBMContext = psBMHeap->pBMContext;

	uFlags = psBMHeap->ui32Attribs &
		 (PVRSRV_HAP_CACHETYPE_MASK | PVRSRV_HAP_MAPTYPE_MASK);

	if (pui32Flags)
		uFlags |= *pui32Flags;

	PVR_DPF(PVR_DBG_MESSAGE, "BM_Wrap (uSize=0x%x, uOffset=0x%x, "
			"bPhysContig=0x%x, pvCPUVAddr=0x%x, uFlags=0x%x)",
			ui32Size, ui32Offset, bPhysContig, pvCPUVAddr, uFlags);

	if (SysAcquireData(&psSysData) != PVRSRV_OK)
		return IMG_FALSE;

	sHashAddress = psSysAddr[0];

	sHashAddress.uiAddr += ui32Offset;

	pBuf = (struct BM_BUF *)HASH_Retrieve(psBMContext->pBufferHash,
				     (u32) sHashAddress.uiAddr);

	if (pBuf) {
		u32 ui32MappingSize =
		    HOST_PAGEALIGN(ui32Size + ui32Offset);

		if (pBuf->pMapping->uSize == ui32MappingSize &&
		    (pBuf->pMapping->eCpuMemoryOrigin == hm_wrapped ||
		      pBuf->pMapping->eCpuMemoryOrigin ==
			hm_wrapped_virtaddr)) {
			PVR_DPF(PVR_DBG_MESSAGE, "BM_Wrap "
				"(Matched previous Wrap! uSize=0x%x, "
				"uOffset=0x%x, SysAddr=%08X)",
				 ui32Size, ui32Offset, sHashAddress.uiAddr);

			pBuf->ui32RefCount++;
			*phBuf = (void *)pBuf;
			if (pui32Flags)
				*pui32Flags = uFlags;

			return IMG_TRUE;
		}
	}

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_BUF),
		       (void **)&pBuf, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "BM_Wrap: BM_Buf alloc FAILED");
		return IMG_FALSE;
	}
	OSMemSet(pBuf, 0, sizeof(struct BM_BUF));

	if (WrapMemory(psBMHeap, ui32Size, ui32Offset, bPhysContig, psSysAddr,
			pvCPUVAddr, uFlags, pBuf) != IMG_TRUE) {
		PVR_DPF(PVR_DBG_ERROR, "BM_Wrap: WrapMemory FAILED");
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_BUF), pBuf,
			  NULL);
		return IMG_FALSE;
	}

	if (pBuf->pMapping->eCpuMemoryOrigin == hm_wrapped ||
	    pBuf->pMapping->eCpuMemoryOrigin == hm_wrapped_virtaddr) {

		PVR_ASSERT(SysSysPAddrToCpuPAddr(sHashAddress).uiAddr ==
			   pBuf->CpuPAddr.uiAddr);

		if (!HASH_Insert(psBMContext->pBufferHash,
				 (u32)sHashAddress.uiAddr, (u32) pBuf)) {
			FreeBuf(pBuf, uFlags);
			PVR_DPF(PVR_DBG_ERROR, "BM_Wrap: HASH_Insert FAILED");
			return IMG_FALSE;
		}
	}

	PVR_DPF(PVR_DBG_MESSAGE,
		 "BM_Wrap (uSize=0x%x, uFlags=0x%x)=%08X(devVAddr=%08X)",
		 ui32Size, uFlags, pBuf, pBuf->DevVAddr.uiAddr);

	pBuf->ui32RefCount = 1;
	pvr_get_ctx(psBMContext);
	*phBuf = (void *) pBuf;
	if (pui32Flags)
		*pui32Flags = (uFlags & ~PVRSRV_HAP_MAPTYPE_MASK) |
			      PVRSRV_HAP_MULTI_PROCESS;

	return IMG_TRUE;
}

void BM_Free(void *hBuf, u32 ui32Flags)
{
	struct BM_BUF *pBuf = (struct BM_BUF *)hBuf;
	struct SYS_DATA *psSysData;
	struct IMG_SYS_PHYADDR sHashAddr;

	PVR_DPF(PVR_DBG_MESSAGE, "BM_Free (h=%08X)", hBuf);
	PVR_ASSERT(pBuf != NULL);

	if (pBuf == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "BM_Free: invalid parameter");
		return;
	}

	if (SysAcquireData(&psSysData) != PVRSRV_OK)
		return;

	pBuf->ui32RefCount--;

	if (pBuf->ui32RefCount == 0) {
		struct BM_MAPPING *map = pBuf->pMapping;
		struct BM_CONTEXT *ctx = map->pBMHeap->pBMContext;

		if (map->eCpuMemoryOrigin == hm_wrapped ||
		    map->eCpuMemoryOrigin == hm_wrapped_virtaddr) {
			sHashAddr = SysCpuPAddrToSysPAddr(pBuf->CpuPAddr);

			HASH_Remove(ctx->pBufferHash, (u32)sHashAddr.uiAddr);
		}
		FreeBuf(pBuf, ui32Flags);
		pvr_put_ctx(ctx);
	}
}

void *BM_HandleToCpuVaddr(void *hBuf)
{
	struct BM_BUF *pBuf = (struct BM_BUF *)hBuf;

	PVR_ASSERT(pBuf != NULL);
	if (pBuf == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "BM_HandleToCpuVaddr: invalid parameter");
		return NULL;
	}

	PVR_DPF(PVR_DBG_MESSAGE,
		 "BM_HandleToCpuVaddr(h=%08X)=%08X", hBuf, pBuf->CpuVAddr);
	return pBuf->CpuVAddr;
}

struct IMG_DEV_VIRTADDR BM_HandleToDevVaddr(void *hBuf)
{
	struct BM_BUF *pBuf = (struct BM_BUF *)hBuf;

	PVR_ASSERT(pBuf != NULL);
	if (pBuf == NULL) {
		struct IMG_DEV_VIRTADDR DevVAddr = { 0 };
		PVR_DPF(PVR_DBG_ERROR,
			 "BM_HandleToDevVaddr: invalid parameter");
		return DevVAddr;
	}

	PVR_DPF(PVR_DBG_MESSAGE, "BM_HandleToDevVaddr(h=%08X)=%08X", hBuf,
		 pBuf->DevVAddr);
	return pBuf->DevVAddr;
}

struct IMG_SYS_PHYADDR BM_HandleToSysPaddr(void *hBuf)
{
	struct BM_BUF *pBuf = (struct BM_BUF *)hBuf;

	PVR_ASSERT(pBuf != NULL);

	if (pBuf == NULL) {
		struct IMG_SYS_PHYADDR PhysAddr = { 0 };
		PVR_DPF(PVR_DBG_ERROR,
			 "BM_HandleToSysPaddr: invalid parameter");
		return PhysAddr;
	}

	PVR_DPF(PVR_DBG_MESSAGE, "BM_HandleToSysPaddr(h=%08X)=%08X", hBuf,
		 pBuf->CpuPAddr.uiAddr);
	return SysCpuPAddrToSysPAddr(pBuf->CpuPAddr);
}

void *BM_HandleToOSMemHandle(void *hBuf)
{
	struct BM_BUF *pBuf = (struct BM_BUF *)hBuf;

	PVR_ASSERT(pBuf != NULL);

	if (pBuf == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "BM_HandleToOSMemHandle: invalid parameter");
		return NULL;
	}

	PVR_DPF(PVR_DBG_MESSAGE,
		 "BM_HandleToOSMemHandle(h=%08X)=%08X",
		 hBuf, pBuf->hOSMemHandle);
	return pBuf->hOSMemHandle;
}

IMG_BOOL BM_ContiguousStatistics(u32 uFlags, u32 *pTotalBytes,
				 u32 *pAvailableBytes)
{
	if (pAvailableBytes || pTotalBytes || uFlags)
		;
	return IMG_FALSE;
}

static IMG_BOOL DevMemoryAlloc(struct BM_CONTEXT *pBMContext,
	       struct BM_MAPPING *pMapping, u32 uFlags, u32 dev_vaddr_alignment,
	       struct IMG_DEV_VIRTADDR *pDevVAddr)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
#ifdef PDUMP
	u32 ui32PDumpSize = pMapping->uSize;
#endif

	psDeviceNode = pBMContext->psDeviceNode;

	if (uFlags & PVRSRV_MEM_INTERLEAVED)

		pMapping->uSize *= 2;
#ifdef PDUMP
	if (uFlags & PVRSRV_MEM_DUMMY)

		ui32PDumpSize = pMapping->pBMHeap->sDevArena.ui32DataPageSize;
#endif

	if (!psDeviceNode->pfnMMUAlloc(pMapping->pBMHeap->pMMUHeap,
				       pMapping->uSize, 0, dev_vaddr_alignment,
				       &(pMapping->DevVAddr))) {
		PVR_DPF(PVR_DBG_ERROR, "DevMemoryAlloc ERROR MMU_Alloc");
		return IMG_FALSE;
	}

	PDUMPMALLOCPAGES(psDeviceNode->sDevId.eDeviceType,
			 pMapping->DevVAddr.uiAddr, pMapping->CpuVAddr,
			 pMapping->hOSMemHandle, ui32PDumpSize,
			 pMapping->pBMHeap->sDevArena.ui32DataPageSize,
			 (void *)pMapping);

	switch (pMapping->eCpuMemoryOrigin) {
	case hm_wrapped:
	case hm_wrapped_virtaddr:
	case hm_contiguous:
		{
			psDeviceNode->pfnMMUMapPages(pMapping->pBMHeap->
							   pMMUHeap,
						     pMapping->DevVAddr,
						     SysCpuPAddrToSysPAddr
							   (pMapping->CpuPAddr),
						     pMapping->uSize, uFlags,
						     (void *)pMapping);

			*pDevVAddr = pMapping->DevVAddr;
			break;
		}
	case hm_env:
		{
			psDeviceNode->pfnMMUMapShadow(pMapping->pBMHeap->
							    pMMUHeap,
						      pMapping->DevVAddr,
						      pMapping->uSize,
						      pMapping->CpuVAddr,
						      pMapping->hOSMemHandle,
						      pDevVAddr, uFlags,
						      (void *)pMapping);
			break;
		}
	case hm_wrapped_scatter:
	case hm_wrapped_scatter_virtaddr:
		{
			psDeviceNode->pfnMMUMapScatter(pMapping->pBMHeap->
							     pMMUHeap,
						       pMapping->DevVAddr,
						       pMapping->psSysAddr,
						       pMapping->uSize, uFlags,
						       (void *)pMapping);

			*pDevVAddr = pMapping->DevVAddr;
			break;
		}
	default:
		PVR_DPF(PVR_DBG_ERROR,
			 "Illegal value %d for pMapping->eCpuMemoryOrigin",
			 pMapping->eCpuMemoryOrigin);
		return IMG_FALSE;
	}


	return IMG_TRUE;
}

static void DevMemoryFree(struct BM_MAPPING *pMapping)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
#ifdef PDUMP
	u32 ui32PSize;
#endif

#ifdef PDUMP

	if (pMapping->ui32Flags & PVRSRV_MEM_DUMMY)
		ui32PSize = pMapping->pBMHeap->sDevArena.ui32DataPageSize;
	else
		ui32PSize = pMapping->uSize;

	PDUMPFREEPAGES(pMapping->pBMHeap, pMapping->DevVAddr, ui32PSize,
		       pMapping->pBMHeap->sDevArena.ui32DataPageSize,
		       (void *)pMapping, (IMG_BOOL)(pMapping->
				   ui32Flags & PVRSRV_MEM_INTERLEAVED));
#endif

	psDeviceNode = pMapping->pBMHeap->pBMContext->psDeviceNode;

	psDeviceNode->pfnMMUFree(pMapping->pBMHeap->pMMUHeap,
				 pMapping->DevVAddr, pMapping->uSize);
}

static IMG_BOOL BM_ImportMemory(void *pH, size_t uRequestSize,
		size_t *pActualSize, struct BM_MAPPING **ppsMapping,
		u32 uFlags, u32 *pBase)
{
	struct BM_MAPPING *pMapping;
	struct BM_HEAP *pBMHeap = pH;
	struct BM_CONTEXT *pBMContext = pBMHeap->pBMContext;
	IMG_BOOL bResult;
	size_t uSize;
	size_t uPSize;
	u32 uDevVAddrAlignment = 0;

	PVR_DPF(PVR_DBG_MESSAGE,
		"BM_ImportMemory (pBMContext=%08X, uRequestSize=0x%x, "
		"uFlags=0x%x, uAlign=0x%x)",
		 pBMContext, uRequestSize, uFlags, uDevVAddrAlignment);

	PVR_ASSERT(ppsMapping != NULL);
	PVR_ASSERT(pBMContext != NULL);

	if (ppsMapping == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "BM_ImportMemory: invalid parameter");
		goto fail_exit;
	}

	uSize = HOST_PAGEALIGN(uRequestSize);
	PVR_ASSERT(uSize >= uRequestSize);

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_MAPPING),
		       (void **)&pMapping, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "BM_ImportMemory: failed struct BM_MAPPING alloc");
		goto fail_exit;
	}

	pMapping->hOSMemHandle = NULL;
	pMapping->CpuVAddr = NULL;
	pMapping->DevVAddr.uiAddr = 0;
	pMapping->CpuPAddr.uiAddr = 0;
	pMapping->uSize = uSize;
	pMapping->pBMHeap = pBMHeap;
	pMapping->ui32Flags = uFlags;

	if (pActualSize)
		*pActualSize = uSize;

	if (pMapping->ui32Flags & PVRSRV_MEM_DUMMY)
		uPSize = pBMHeap->sDevArena.ui32DataPageSize;
	else
		uPSize = pMapping->uSize;

	if (pBMHeap->ui32Attribs & PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG) {
		if (OSAllocPages(pBMHeap->ui32Attribs, uPSize,
				 pBMHeap->sDevArena.ui32DataPageSize,
				 (void **)&pMapping->CpuVAddr,
				 &pMapping->hOSMemHandle) != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				 "BM_ImportMemory: OSAllocPages(0x%x) failed",
				 uPSize);
			goto fail_mapping_alloc;
		}

		pMapping->eCpuMemoryOrigin = hm_env;
	} else if (pBMHeap->ui32Attribs & PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG) {
		struct IMG_SYS_PHYADDR sSysPAddr;

		PVR_ASSERT(pBMHeap->pLocalDevMemArena != NULL);

		if (!RA_Alloc(pBMHeap->pLocalDevMemArena, uPSize, NULL, 0,
			      pBMHeap->sDevArena.ui32DataPageSize,
			      (u32 *)&sSysPAddr.uiAddr)) {
			PVR_DPF(PVR_DBG_ERROR,
				 "BM_ImportMemory: RA_Alloc(0x%x) FAILED",
				 uPSize);
			goto fail_mapping_alloc;
		}

		pMapping->CpuPAddr = SysSysPAddrToCpuPAddr(sSysPAddr);
		if (OSReservePhys(pMapping->CpuPAddr, uPSize,
				  pBMHeap->ui32Attribs, &pMapping->CpuVAddr,
				  &pMapping->hOSMemHandle) != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				 "BM_ImportMemory: OSReservePhys failed");
			goto fail_dev_mem_alloc;
		}

		pMapping->eCpuMemoryOrigin = hm_contiguous;
	} else {
		PVR_DPF(PVR_DBG_ERROR,
			 "BM_ImportMemory: Invalid backing store type");
		goto fail_mapping_alloc;
	}

	bResult = DevMemoryAlloc(pBMContext, pMapping, uFlags,
				 uDevVAddrAlignment, &pMapping->DevVAddr);
	if (!bResult) {
		PVR_DPF(PVR_DBG_ERROR,
			 "BM_ImportMemory: DevMemoryAlloc(0x%x) failed",
			 pMapping->uSize);
		goto fail_dev_mem_alloc;
	}

	PVR_ASSERT(uDevVAddrAlignment > 1 ?
		   (pMapping->DevVAddr.uiAddr % uDevVAddrAlignment) == 0 : 1);

	*pBase = pMapping->DevVAddr.uiAddr;
	*ppsMapping = pMapping;

	PVR_DPF(PVR_DBG_MESSAGE, "BM_ImportMemory: IMG_TRUE");
	return IMG_TRUE;

fail_dev_mem_alloc:
	if (pMapping && (pMapping->CpuVAddr || pMapping->hOSMemHandle)) {
		if (pMapping->ui32Flags & PVRSRV_MEM_INTERLEAVED)
			pMapping->uSize /= 2;

		if (pMapping->ui32Flags & PVRSRV_MEM_DUMMY)
			uPSize = pBMHeap->sDevArena.ui32DataPageSize;
		else
			uPSize = pMapping->uSize;

		if (pBMHeap->ui32Attribs &
			PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG) {
			OSFreePages(pBMHeap->ui32Attribs, uPSize,
				    (void *)pMapping->CpuVAddr,
				    pMapping->hOSMemHandle);
		} else {
			struct IMG_SYS_PHYADDR sSysPAddr;

			if (pMapping->CpuVAddr)
				OSUnReservePhys(pMapping->CpuVAddr, uPSize,
						pBMHeap->ui32Attribs,
						pMapping->hOSMemHandle);
			sSysPAddr = SysCpuPAddrToSysPAddr(pMapping->CpuPAddr);
			RA_Free(pBMHeap->pLocalDevMemArena, sSysPAddr.uiAddr,
				IMG_FALSE);
		}
	}
fail_mapping_alloc:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_MAPPING), pMapping,
		  NULL);
fail_exit:
	return IMG_FALSE;
}

static void BM_FreeMemory(void *h, u32 _base, struct BM_MAPPING *psMapping)
{
	struct BM_HEAP *pBMHeap = h;
	size_t uPSize;

	PVR_UNREFERENCED_PARAMETER(_base);

	PVR_DPF(PVR_DBG_MESSAGE,
		 "BM_FreeMemory (h=%08X, base=0x%x, psMapping=0x%x)", h, _base,
		 psMapping);

	PVR_ASSERT(psMapping != NULL);

	if (psMapping == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "BM_FreeMemory: invalid parameter");
		return;
	}

	DevMemoryFree(psMapping);

	if ((psMapping->ui32Flags & PVRSRV_MEM_INTERLEAVED) != 0)
		psMapping->uSize /= 2;

	if (psMapping->ui32Flags & PVRSRV_MEM_DUMMY)
		uPSize = psMapping->pBMHeap->sDevArena.ui32DataPageSize;
	else
		uPSize = psMapping->uSize;

	if (pBMHeap->ui32Attribs & PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG) {
		OSFreePages(pBMHeap->ui32Attribs, uPSize,
			    (void *)psMapping->CpuVAddr,
			    psMapping->hOSMemHandle);
	} else if (pBMHeap->ui32Attribs & PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG) {
		struct IMG_SYS_PHYADDR sSysPAddr;

		OSUnReservePhys(psMapping->CpuVAddr, uPSize,
				pBMHeap->ui32Attribs, psMapping->hOSMemHandle);

		sSysPAddr = SysCpuPAddrToSysPAddr(psMapping->CpuPAddr);

		RA_Free(pBMHeap->pLocalDevMemArena, sSysPAddr.uiAddr,
			IMG_FALSE);
	} else {
		PVR_DPF(PVR_DBG_ERROR,
			 "BM_FreeMemory: Invalid backing store type");
	}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BM_MAPPING), psMapping,
		  NULL);

	PVR_DPF(PVR_DBG_MESSAGE,
		 "..BM_FreeMemory (h=%08X, base=0x%x, psMapping=0x%x)",
		 h, _base, psMapping);
}

enum PVRSRV_ERROR BM_GetPhysPageAddr(struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
				struct IMG_DEV_VIRTADDR sDevVPageAddr,
				struct IMG_DEV_PHYADDR *psDevPAddr)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode;

	PVR_DPF(PVR_DBG_MESSAGE, "BM_GetPhysPageAddr");

	if (!psMemInfo || !psDevPAddr) {
		PVR_DPF(PVR_DBG_ERROR, "BM_GetPhysPageAddr: Invalid params");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	PVR_ASSERT((sDevVPageAddr.uiAddr & 0xFFF) == 0);

	psDeviceNode =
	    ((struct BM_BUF *)psMemInfo->sMemBlk.hBuffer)->pMapping->pBMHeap->
	    pBMContext->psDeviceNode;

	*psDevPAddr = psDeviceNode->pfnMMUGetPhysPageAddr(((struct BM_BUF *)
				psMemInfo->sMemBlk.hBuffer)->
				pMapping->pBMHeap->pMMUHeap, sDevVPageAddr);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR BM_GetHeapInfo(void *hDevMemHeap,
			    struct PVRSRV_HEAP_INFO *psHeapInfo)
{
	struct BM_HEAP *psBMHeap = (struct BM_HEAP *)hDevMemHeap;

	PVR_DPF(PVR_DBG_VERBOSE, "BM_GetHeapInfo");

	psHeapInfo->hDevMemHeap = hDevMemHeap;
	psHeapInfo->sDevVAddrBase = psBMHeap->sDevArena.BaseDevVAddr;
	psHeapInfo->ui32HeapByteSize = psBMHeap->sDevArena.ui32Size;
	psHeapInfo->ui32Attribs = psBMHeap->ui32Attribs;

	return PVRSRV_OK;
}

struct MMU_CONTEXT *BM_GetMMUContext(void *hDevMemHeap)
{
	struct BM_HEAP *pBMHeap = (struct BM_HEAP *)hDevMemHeap;

	PVR_DPF(PVR_DBG_VERBOSE, "BM_GetMMUContext");

	return pBMHeap->pBMContext->psMMUContext;
}

struct MMU_CONTEXT *BM_GetMMUContextFromMemContext(void *hDevMemContext)
{
	struct BM_CONTEXT *pBMContext = (struct BM_CONTEXT *)hDevMemContext;

	PVR_DPF(PVR_DBG_VERBOSE, "BM_GetMMUContextFromMemContext");

	return pBMContext->psMMUContext;
}

void *BM_GetMMUHeap(void *hDevMemHeap)
{
	PVR_DPF(PVR_DBG_VERBOSE, "BM_GetMMUHeap");

	return (void *)((struct BM_HEAP *)hDevMemHeap)->pMMUHeap;
}

struct PVRSRV_DEVICE_NODE *BM_GetDeviceNode(void *hDevMemContext)
{
	PVR_DPF(PVR_DBG_VERBOSE, "BM_GetDeviceNode");

	return ((struct BM_CONTEXT *)hDevMemContext)->psDeviceNode;
}

void *BM_GetMappingHandle(struct PVRSRV_KERNEL_MEM_INFO *psMemInfo)
{
	PVR_DPF(PVR_DBG_VERBOSE, "BM_GetMappingHandle");

	return ((struct BM_BUF *)
			psMemInfo->sMemBlk.hBuffer)->pMapping->hOSMemHandle;
}
