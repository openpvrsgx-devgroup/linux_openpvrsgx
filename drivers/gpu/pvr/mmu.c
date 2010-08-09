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

#include "sgxdefs.h"
#include "sgxmmu.h"
#include "services_headers.h"
#include "buffer_manager.h"
#include "hash.h"
#include "ra.h"
#include "pdump_km.h"
#include "sgxapi_km.h"
#include "sgx_bridge_km.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "mmu.h"

#define UINT32_MAX_VALUE	0xFFFFFFFFUL

struct MMU_PT_INFO {
	void *hPTPageOSMemHandle;
	void *PTPageCpuVAddr;
	u32 ui32ValidPTECount;
};

struct MMU_CONTEXT {
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	void *pvPDCpuVAddr;
	struct IMG_DEV_PHYADDR sPDDevPAddr;
	void *hPDOSMemHandle;
	struct MMU_PT_INFO *apsPTInfoList[1024];
	struct PVRSRV_SGXDEV_INFO *psDevInfo;
	struct MMU_CONTEXT *psNext;
};

struct MMU_HEAP {
	struct MMU_CONTEXT *psMMUContext;

	u32 ui32PTBaseIndex;
	u32 ui32PTPageCount;
	u32 ui32PTEntryCount;

	struct RA_ARENA *psVMArena;

	struct DEV_ARENA_DESCRIPTOR *psDevArena;
};


#if defined(PDUMP)
static void MMU_PDumpPageTables(struct MMU_HEAP *pMMUHeap,
		    struct IMG_DEV_VIRTADDR DevVAddr, size_t uSize,
		    IMG_BOOL bForUnmap, void *hUniqueTag);
#endif

#define PAGE_TEST					0


void MMU_InvalidateDirectoryCache(struct PVRSRV_SGXDEV_INFO *psDevInfo)
{
	psDevInfo->ui32CacheControl |= SGX_BIF_INVALIDATE_PDCACHE;
}

static void MMU_InvalidatePageTableCache(struct PVRSRV_SGXDEV_INFO *psDevInfo)
{
	psDevInfo->ui32CacheControl |= SGX_BIF_INVALIDATE_PTCACHE;
}

static IMG_BOOL _AllocPageTables(struct MMU_HEAP *pMMUHeap)
{
	PVR_DPF(PVR_DBG_MESSAGE, "_AllocPageTables()");

	PVR_ASSERT(pMMUHeap != NULL);
	PVR_ASSERT(HOST_PAGESIZE() == SGX_MMU_PAGE_SIZE);

	if (pMMUHeap == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "_AllocPageTables: invalid parameter");
		return IMG_FALSE;
	}

	pMMUHeap->ui32PTEntryCount =
	    pMMUHeap->psDevArena->ui32Size >> SGX_MMU_PAGE_SHIFT;

	pMMUHeap->ui32PTBaseIndex =
	    (pMMUHeap->psDevArena->BaseDevVAddr.
	     uiAddr & (SGX_MMU_PD_MASK | SGX_MMU_PT_MASK)) >>
							SGX_MMU_PAGE_SHIFT;

	pMMUHeap->ui32PTPageCount =
	    (pMMUHeap->ui32PTEntryCount + SGX_MMU_PT_SIZE - 1) >>
							SGX_MMU_PT_SHIFT;

	return IMG_TRUE;
}

static void _DeferredFreePageTable(struct MMU_HEAP *pMMUHeap, u32 ui32PTIndex)
{
	u32 *pui32PDEntry;
	u32 i;
	u32 ui32PDIndex;
	struct SYS_DATA *psSysData;
	struct MMU_PT_INFO **ppsPTInfoList;

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "_DeferredFreePageTables: "
					"ERROR call to SysAcquireData failed");
		return;
	}

	ui32PDIndex =
	    pMMUHeap->psDevArena->BaseDevVAddr.uiAddr >> (SGX_MMU_PAGE_SHIFT +
							  SGX_MMU_PT_SHIFT);

	ppsPTInfoList = &pMMUHeap->psMMUContext->apsPTInfoList[ui32PDIndex];

	{
		PVR_ASSERT(ppsPTInfoList[ui32PTIndex] == NULL ||
			   ppsPTInfoList[ui32PTIndex]->ui32ValidPTECount ==
									 0);
	}

	PDUMPCOMMENT("Free page table (page count == %08X)",
		     pMMUHeap->ui32PTPageCount);
	if (ppsPTInfoList[ui32PTIndex]
	    && ppsPTInfoList[ui32PTIndex]->PTPageCpuVAddr)
		PDUMPFREEPAGETABLE(PVRSRV_DEVICE_TYPE_SGX,
				   ppsPTInfoList[ui32PTIndex]->PTPageCpuVAddr,
				   SGX_MMU_PAGE_SIZE, PDUMP_PT_UNIQUETAG);

	switch (pMMUHeap->psDevArena->DevMemHeapType) {
	case DEVICE_MEMORY_HEAP_SHARED:
	case DEVICE_MEMORY_HEAP_SHARED_EXPORTED:
		{
			struct MMU_CONTEXT *psMMUContext =
			  (struct MMU_CONTEXT *)
			    pMMUHeap->psMMUContext->psDevInfo->pvMMUContextList;

			while (psMMUContext) {
				pui32PDEntry =
				    (u32 *) psMMUContext->pvPDCpuVAddr;
				pui32PDEntry += ui32PDIndex;
				pui32PDEntry[ui32PTIndex] = 0;
				PDUMPMEM2(PVRSRV_DEVICE_TYPE_SGX,
					  (void *) &
					  pui32PDEntry[ui32PTIndex],
					  sizeof(u32), 0, IMG_FALSE,
					  PDUMP_PT_UNIQUETAG,
					  PDUMP_PT_UNIQUETAG);
				psMMUContext = psMMUContext->psNext;
			}
			break;
		}
	case DEVICE_MEMORY_HEAP_PERCONTEXT:
	case DEVICE_MEMORY_HEAP_KERNEL:
		{

			pui32PDEntry =
			    (u32 *) pMMUHeap->psMMUContext->pvPDCpuVAddr;
			pui32PDEntry += ui32PDIndex;
			pui32PDEntry[ui32PTIndex] = 0;
			PDUMPMEM2(PVRSRV_DEVICE_TYPE_SGX,
				  (void *) &pui32PDEntry[ui32PTIndex],
				  sizeof(u32), 0, IMG_FALSE,
				  PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
			break;
		}
	default:
		{
			PVR_DPF(PVR_DBG_ERROR,
			"_DeferredFreePagetable: ERROR invalid heap type");
			return;
		}
	}

	if (ppsPTInfoList[ui32PTIndex] != NULL) {
		if (ppsPTInfoList[ui32PTIndex]->PTPageCpuVAddr != NULL) {
			u32 *pui32Tmp;

			pui32Tmp =
			    (u32 *) ppsPTInfoList[ui32PTIndex]->
			    PTPageCpuVAddr;

			for (i = 0;
			     (i < pMMUHeap->ui32PTEntryCount) && (i < 1024);
			     i++)
				pui32Tmp[i] = 0;

			if (pMMUHeap->psDevArena->psDeviceMemoryHeapInfo->
			    psLocalDevMemArena == NULL) {
				OSFreePages(PVRSRV_HAP_WRITECOMBINE |
					    PVRSRV_HAP_KERNEL_ONLY,
					    SGX_MMU_PAGE_SIZE,
					    ppsPTInfoList[ui32PTIndex]->
						    PTPageCpuVAddr,
					    ppsPTInfoList[ui32PTIndex]->
						    hPTPageOSMemHandle);
			} else {
				struct IMG_SYS_PHYADDR sSysPAddr;
				struct IMG_CPU_PHYADDR sCpuPAddr;

				sCpuPAddr =
				    OSMapLinToCPUPhys(ppsPTInfoList
						      [ui32PTIndex]->
						      PTPageCpuVAddr);
				sSysPAddr = SysCpuPAddrToSysPAddr(sCpuPAddr);

				OSUnMapPhysToLin((void __force __iomem *)
						   ppsPTInfoList[ui32PTIndex]->
							PTPageCpuVAddr,
						 SGX_MMU_PAGE_SIZE,
						 PVRSRV_HAP_WRITECOMBINE |
							 PVRSRV_HAP_KERNEL_ONLY,
						 ppsPTInfoList[ui32PTIndex]->
							 hPTPageOSMemHandle);

				RA_Free(pMMUHeap->psDevArena->
						psDeviceMemoryHeapInfo->
							psLocalDevMemArena,
					sSysPAddr.uiAddr, IMG_FALSE);
			}

			pMMUHeap->ui32PTEntryCount -= i;
		} else {
			pMMUHeap->ui32PTEntryCount -= 1024;
		}

		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct MMU_PT_INFO),
			  ppsPTInfoList[ui32PTIndex], NULL);
		ppsPTInfoList[ui32PTIndex] = NULL;
	} else {
		pMMUHeap->ui32PTEntryCount -= 1024;
	}

	PDUMPCOMMENT("Finished free page table (page count == %08X)",
		     pMMUHeap->ui32PTPageCount);
}

static void _DeferredFreePageTables(struct MMU_HEAP *pMMUHeap)
{
	u32 i;

	for (i = 0; i < pMMUHeap->ui32PTPageCount; i++)
		_DeferredFreePageTable(pMMUHeap, i);
	MMU_InvalidateDirectoryCache(pMMUHeap->psMMUContext->psDevInfo);
}

static IMG_BOOL _DeferredAllocPagetables(struct MMU_HEAP *pMMUHeap,
				struct IMG_DEV_VIRTADDR DevVAddr, u32 ui32Size)
{
	u32 ui32PTPageCount;
	u32 ui32PDIndex;
	u32 i;
	u32 *pui32PDEntry;
	struct MMU_PT_INFO **ppsPTInfoList;
	struct SYS_DATA *psSysData;
	struct IMG_DEV_VIRTADDR sHighDevVAddr;

	PVR_ASSERT(DevVAddr.uiAddr < (1 << SGX_FEATURE_ADDRESS_SPACE_SIZE));

	if (SysAcquireData(&psSysData) != PVRSRV_OK)
		return IMG_FALSE;

	ui32PDIndex =
	    DevVAddr.uiAddr >> (SGX_MMU_PAGE_SHIFT + SGX_MMU_PT_SHIFT);

	if ((UINT32_MAX_VALUE - DevVAddr.uiAddr) <
	    (ui32Size + (1 << (SGX_MMU_PAGE_SHIFT + SGX_MMU_PT_SHIFT)) - 1)) {

		sHighDevVAddr.uiAddr = UINT32_MAX_VALUE;
	} else {
		sHighDevVAddr.uiAddr = DevVAddr.uiAddr + ui32Size +
					(1 << (SGX_MMU_PAGE_SHIFT +
					       SGX_MMU_PT_SHIFT)) - 1;
	}

	ui32PTPageCount =
	    sHighDevVAddr.uiAddr >> (SGX_MMU_PAGE_SHIFT + SGX_MMU_PT_SHIFT);

	ui32PTPageCount -= ui32PDIndex;

	pui32PDEntry = (u32 *) pMMUHeap->psMMUContext->pvPDCpuVAddr;
	pui32PDEntry += ui32PDIndex;

	ppsPTInfoList = &pMMUHeap->psMMUContext->apsPTInfoList[ui32PDIndex];

	PDUMPCOMMENT("Alloc page table (page count == %08X)", ui32PTPageCount);
	PDUMPCOMMENT("Page directory mods (page count == %08X)",
		     ui32PTPageCount);

	for (i = 0; i < ui32PTPageCount; i++) {
		if (ppsPTInfoList[i] == NULL) {
			OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
				   sizeof(struct MMU_PT_INFO),
				   (void **) &ppsPTInfoList[i], NULL);
			if (ppsPTInfoList[i] == NULL) {
				PVR_DPF(PVR_DBG_ERROR,
					"_DeferredAllocPagetables: "
					"ERROR call to OSAllocMem failed");
				return IMG_FALSE;
			}
			OSMemSet(ppsPTInfoList[i], 0,
				 sizeof(struct MMU_PT_INFO));
		}

		if (ppsPTInfoList[i]->hPTPageOSMemHandle == NULL &&
		    ppsPTInfoList[i]->PTPageCpuVAddr == NULL) {
			struct IMG_CPU_PHYADDR sCpuPAddr;
			struct IMG_DEV_PHYADDR sDevPAddr;

			PVR_ASSERT(pui32PDEntry[i] == 0);

			if (pMMUHeap->psDevArena->psDeviceMemoryHeapInfo->
			    psLocalDevMemArena == NULL) {
				if (OSAllocPages(PVRSRV_HAP_WRITECOMBINE |
						     PVRSRV_HAP_KERNEL_ONLY,
					     SGX_MMU_PAGE_SIZE,
					     SGX_MMU_PAGE_SIZE,
					     (void **)&ppsPTInfoList[i]->
						PTPageCpuVAddr,
					     &ppsPTInfoList[i]->
						hPTPageOSMemHandle) !=
				    PVRSRV_OK) {
					PVR_DPF(PVR_DBG_ERROR,
					   "_DeferredAllocPagetables: "
					   "ERROR call to OSAllocPages failed");
					return IMG_FALSE;
				}

				if (ppsPTInfoList[i]->PTPageCpuVAddr) {
					sCpuPAddr =
					    OSMapLinToCPUPhys(ppsPTInfoList[i]->
							      PTPageCpuVAddr);
				} else {
					sCpuPAddr =
					    OSMemHandleToCpuPAddr(
						ppsPTInfoList[i]->
							  hPTPageOSMemHandle,
						0);
				}
				sDevPAddr =
				    SysCpuPAddrToDevPAddr
					    (PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);
			} else {
				struct IMG_SYS_PHYADDR sSysPAddr;

				if (RA_Alloc(pMMUHeap->psDevArena->
				     psDeviceMemoryHeapInfo->psLocalDevMemArena,
				     SGX_MMU_PAGE_SIZE, NULL, 0,
				     SGX_MMU_PAGE_SIZE,
				     &(sSysPAddr.uiAddr)) != IMG_TRUE) {
					PVR_DPF(PVR_DBG_ERROR,
					       "_DeferredAllocPagetables: "
					       "ERROR call to RA_Alloc failed");
					return IMG_FALSE;
				}

				sCpuPAddr = SysSysPAddrToCpuPAddr(sSysPAddr);
				ppsPTInfoList[i]->PTPageCpuVAddr =
				    (void __force *)
				    OSMapPhysToLin(sCpuPAddr, SGX_MMU_PAGE_SIZE,
						   PVRSRV_HAP_WRITECOMBINE |
						   PVRSRV_HAP_KERNEL_ONLY,
						   &ppsPTInfoList[i]->
						   hPTPageOSMemHandle);
				if (!ppsPTInfoList[i]->PTPageCpuVAddr) {
					PVR_DPF(PVR_DBG_ERROR,
					     "_DeferredAllocPagetables: "
					     "ERROR failed to map page tables");
					return IMG_FALSE;
				}

				sDevPAddr = SysCpuPAddrToDevPAddr
					    (PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);

			}


			OSMemSet(ppsPTInfoList[i]->PTPageCpuVAddr, 0,
				 SGX_MMU_PAGE_SIZE);

			PDUMPMALLOCPAGETABLE(PVRSRV_DEVICE_TYPE_SGX,
					     ppsPTInfoList[i]->PTPageCpuVAddr,
					     SGX_MMU_PAGE_SIZE,
					     PDUMP_PT_UNIQUETAG);

			PDUMPMEM2(PVRSRV_DEVICE_TYPE_SGX,
				  ppsPTInfoList[i]->PTPageCpuVAddr,
				  SGX_MMU_PAGE_SIZE, 0, IMG_TRUE,
				  PDUMP_PT_UNIQUETAG, PDUMP_PT_UNIQUETAG);

			switch (pMMUHeap->psDevArena->DevMemHeapType) {
			case DEVICE_MEMORY_HEAP_SHARED:
			case DEVICE_MEMORY_HEAP_SHARED_EXPORTED:
				{
					struct MMU_CONTEXT *psMMUContext =
					    (struct MMU_CONTEXT *)pMMUHeap->
						    psMMUContext->psDevInfo->
							    pvMMUContextList;

					while (psMMUContext) {
						pui32PDEntry =
						    (u32 *)psMMUContext->
								pvPDCpuVAddr;
						pui32PDEntry += ui32PDIndex;

						pui32PDEntry[i] =
						    sDevPAddr.uiAddr |
							SGX_MMU_PDE_VALID;

						PDUMPMEM2
						    (PVRSRV_DEVICE_TYPE_SGX,
						     (void *)&pui32PDEntry[i],
						     sizeof(u32), 0,
						     IMG_FALSE,
						     PDUMP_PD_UNIQUETAG,
						     PDUMP_PT_UNIQUETAG);

						psMMUContext =
						    psMMUContext->psNext;
					}
					break;
				}
			case DEVICE_MEMORY_HEAP_PERCONTEXT:
			case DEVICE_MEMORY_HEAP_KERNEL:
				{
					pui32PDEntry[i] = sDevPAddr.uiAddr |
							     SGX_MMU_PDE_VALID;

					PDUMPMEM2(PVRSRV_DEVICE_TYPE_SGX,
						  (void *)&pui32PDEntry[i],
						  sizeof(u32), 0,
						  IMG_FALSE, PDUMP_PD_UNIQUETAG,
						  PDUMP_PT_UNIQUETAG);

					break;
				}
			default:
				{
					PVR_DPF(PVR_DBG_ERROR,
						"_DeferredAllocPagetables: "
						"ERROR invalid heap type");
					return IMG_FALSE;
				}
			}


			MMU_InvalidateDirectoryCache(pMMUHeap->psMMUContext->
						     psDevInfo);
		} else {

			PVR_ASSERT(pui32PDEntry[i] != 0);
		}
	}

	return IMG_TRUE;
}

enum PVRSRV_ERROR MMU_Initialise(struct PVRSRV_DEVICE_NODE *psDeviceNode,
			    struct MMU_CONTEXT **ppsMMUContext,
			    struct IMG_DEV_PHYADDR *psPDDevPAddr)
{
	u32 *pui32Tmp;
	u32 i;
	void *pvPDCpuVAddr;
	struct IMG_DEV_PHYADDR sPDDevPAddr;
	struct IMG_CPU_PHYADDR sCpuPAddr;
	struct MMU_CONTEXT *psMMUContext;
	void *hPDOSMemHandle;
	struct SYS_DATA *psSysData;
	struct PVRSRV_SGXDEV_INFO *psDevInfo;

	PVR_DPF(PVR_DBG_MESSAGE, "MMU_Initialise");

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "MMU_Initialise: ERROR call to SysAcquireData failed");
		return PVRSRV_ERROR_GENERIC;
	}

	OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		   sizeof(struct MMU_CONTEXT), (void **) &psMMUContext, NULL);
	if (psMMUContext == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "MMU_Initialise: ERROR call to OSAllocMem failed");
		return PVRSRV_ERROR_GENERIC;
	}
	OSMemSet(psMMUContext, 0, sizeof(struct MMU_CONTEXT));

	psDevInfo = (struct PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;
	psMMUContext->psDevInfo = psDevInfo;

	psMMUContext->psDeviceNode = psDeviceNode;

	if (psDeviceNode->psLocalDevMemArena == NULL) {
		if (OSAllocPages
		    (PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
		     SGX_MMU_PAGE_SIZE, SGX_MMU_PAGE_SIZE, &pvPDCpuVAddr,
		     &hPDOSMemHandle) != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "MMU_Initialise: "
					"ERROR call to OSAllocPages failed");
			return PVRSRV_ERROR_GENERIC;
		}

		if (pvPDCpuVAddr)
			sCpuPAddr = OSMapLinToCPUPhys(pvPDCpuVAddr);
		else
			sCpuPAddr = OSMemHandleToCpuPAddr(hPDOSMemHandle, 0);
		sPDDevPAddr =
		    SysCpuPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);
	} else {
		struct IMG_SYS_PHYADDR sSysPAddr;

		if (RA_Alloc(psDeviceNode->psLocalDevMemArena,
			     SGX_MMU_PAGE_SIZE, NULL, 0, SGX_MMU_PAGE_SIZE,
			     &(sSysPAddr.uiAddr)) != IMG_TRUE) {
			PVR_DPF(PVR_DBG_ERROR, "MMU_Initialise: "
					"ERROR call to RA_Alloc failed");
			return PVRSRV_ERROR_GENERIC;
		}

		sCpuPAddr = SysSysPAddrToCpuPAddr(sSysPAddr);
		sPDDevPAddr =
		    SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sSysPAddr);
		pvPDCpuVAddr = (void __force *)
		    OSMapPhysToLin(sCpuPAddr, SGX_MMU_PAGE_SIZE,
				   PVRSRV_HAP_WRITECOMBINE |
				   PVRSRV_HAP_KERNEL_ONLY, &hPDOSMemHandle);
		if (!pvPDCpuVAddr) {
			PVR_DPF(PVR_DBG_ERROR, "MMU_Initialise: "
					"ERROR failed to map page tables");
			return PVRSRV_ERROR_GENERIC;
		}
	}

	PDUMPCOMMENT("Alloc page directory");

	PDUMPMALLOCPAGETABLE(PVRSRV_DEVICE_TYPE_SGX, pvPDCpuVAddr,
			     SGX_MMU_PAGE_SIZE, PDUMP_PD_UNIQUETAG);

	if (pvPDCpuVAddr) {
		pui32Tmp = (u32 *) pvPDCpuVAddr;
	} else {
		PVR_DPF(PVR_DBG_ERROR,
			 "MMU_Initialise: pvPDCpuVAddr invalid");
		return PVRSRV_ERROR_GENERIC;
	}

	for (i = 0; i < SGX_MMU_PD_SIZE; i++)
		pui32Tmp[i] = 0;

	PDUMPCOMMENT("Page directory contents");
	PDUMPMEM2(PVRSRV_DEVICE_TYPE_SGX, pvPDCpuVAddr, SGX_MMU_PAGE_SIZE, 0,
		  IMG_TRUE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);

	psMMUContext->pvPDCpuVAddr = pvPDCpuVAddr;
	psMMUContext->sPDDevPAddr = sPDDevPAddr;
	psMMUContext->hPDOSMemHandle = hPDOSMemHandle;

	*ppsMMUContext = psMMUContext;

	*psPDDevPAddr = sPDDevPAddr;

	psMMUContext->psNext = (struct MMU_CONTEXT *)
						psDevInfo->pvMMUContextList;
	psDevInfo->pvMMUContextList = (void *) psMMUContext;


	return PVRSRV_OK;
}

void MMU_Finalise(struct MMU_CONTEXT *psMMUContext)
{
	u32 *pui32Tmp, i;
	struct SYS_DATA *psSysData;
	struct MMU_CONTEXT **ppsMMUContext;

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "MMU_Finalise: ERROR call to SysAcquireData failed");
		return;
	}

	PDUMPCOMMENT("Free page directory");
	PDUMPFREEPAGETABLE(PVRSRV_DEVICE_TYPE_SGX, psMMUContext->pvPDCpuVAddr,
			   SGX_MMU_PAGE_SIZE, PDUMP_PT_UNIQUETAG);

	pui32Tmp = (u32 *) psMMUContext->pvPDCpuVAddr;

	for (i = 0; i < SGX_MMU_PD_SIZE; i++)
		pui32Tmp[i] = 0;

	if (psMMUContext->psDeviceNode->psLocalDevMemArena == NULL) {
		OSFreePages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
			    SGX_MMU_PAGE_SIZE,
			    psMMUContext->pvPDCpuVAddr,
			    psMMUContext->hPDOSMemHandle);

	} else {
		struct IMG_SYS_PHYADDR sSysPAddr;
		struct IMG_CPU_PHYADDR sCpuPAddr;

		sCpuPAddr = OSMapLinToCPUPhys(psMMUContext->pvPDCpuVAddr);
		sSysPAddr = SysCpuPAddrToSysPAddr(sCpuPAddr);

		OSUnMapPhysToLin((void __iomem __force *)
					psMMUContext->pvPDCpuVAddr,
				 SGX_MMU_PAGE_SIZE,
				 PVRSRV_HAP_WRITECOMBINE |
						PVRSRV_HAP_KERNEL_ONLY,
				 psMMUContext->hPDOSMemHandle);

		RA_Free(psMMUContext->psDeviceNode->psLocalDevMemArena,
			sSysPAddr.uiAddr, IMG_FALSE);

	}

	PVR_DPF(PVR_DBG_MESSAGE, "MMU_Finalise");

	ppsMMUContext =
	    (struct MMU_CONTEXT **) &psMMUContext->psDevInfo->pvMMUContextList;
	while (*ppsMMUContext) {
		if (*ppsMMUContext == psMMUContext) {

			*ppsMMUContext = psMMUContext->psNext;
			break;
		}

		ppsMMUContext = &((*ppsMMUContext)->psNext);
	}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct MMU_CONTEXT),
		  psMMUContext, NULL);
}

void MMU_InsertHeap(struct MMU_CONTEXT *psMMUContext,
		    struct MMU_HEAP *psMMUHeap)
{
	u32 *pui32PDCpuVAddr = (u32 *)psMMUContext->pvPDCpuVAddr;
	u32 *pui32KernelPDCpuVAddr = (u32 *)
					psMMUHeap->psMMUContext->pvPDCpuVAddr;
	u32 ui32PDEntry;
	IMG_BOOL bInvalidateDirectoryCache = IMG_FALSE;

	pui32PDCpuVAddr +=
	    psMMUHeap->psDevArena->BaseDevVAddr.uiAddr >> (SGX_MMU_PAGE_SHIFT +
							   SGX_MMU_PT_SHIFT);
	pui32KernelPDCpuVAddr +=
	    psMMUHeap->psDevArena->BaseDevVAddr.uiAddr >> (SGX_MMU_PAGE_SHIFT +
							   SGX_MMU_PT_SHIFT);

	PDUMPCOMMENT("Page directory shared heap range copy");

	for (ui32PDEntry = 0; ui32PDEntry < psMMUHeap->ui32PTPageCount;
	     ui32PDEntry++) {

		PVR_ASSERT(pui32PDCpuVAddr[ui32PDEntry] == 0);

		pui32PDCpuVAddr[ui32PDEntry] =
		    pui32KernelPDCpuVAddr[ui32PDEntry];
		if (pui32PDCpuVAddr[ui32PDEntry]) {
			PDUMPMEM2(PVRSRV_DEVICE_TYPE_SGX,
				  (void *) &pui32PDCpuVAddr[ui32PDEntry],
				  sizeof(u32), 0, IMG_FALSE,
				  PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);

			bInvalidateDirectoryCache = IMG_TRUE;
		}
	}

	if (bInvalidateDirectoryCache)
		MMU_InvalidateDirectoryCache(psMMUContext->psDevInfo);
}

static void MMU_UnmapPagesAndFreePTs(struct MMU_HEAP *psMMUHeap,
			 struct IMG_DEV_VIRTADDR sDevVAddr,
			 u32 ui32PageCount, void *hUniqueTag)
{
	u32 uPageSize = HOST_PAGESIZE();
	struct IMG_DEV_VIRTADDR sTmpDevVAddr;
	u32 i;
	u32 ui32PDIndex;
	u32 ui32PTIndex;
	u32 *pui32Tmp;
	IMG_BOOL bInvalidateDirectoryCache = IMG_FALSE;

#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(hUniqueTag);
#endif

	sTmpDevVAddr = sDevVAddr;

	for (i = 0; i < ui32PageCount; i++) {
		struct MMU_PT_INFO **ppsPTInfoList;

		ui32PDIndex =
		    sTmpDevVAddr.uiAddr >> (SGX_MMU_PAGE_SHIFT +
					    SGX_MMU_PT_SHIFT);

		ppsPTInfoList =
		    &psMMUHeap->psMMUContext->apsPTInfoList[ui32PDIndex];

		{
			ui32PTIndex = (sTmpDevVAddr.uiAddr & SGX_MMU_PT_MASK)
						>> SGX_MMU_PAGE_SHIFT;

			if (!ppsPTInfoList[0]) {
				PVR_DPF(PVR_DBG_MESSAGE,
					"MMU_UnmapPagesAndFreePTs: "
					"Invalid PT for alloc at VAddr:0x%08lX "
					"(VaddrIni:0x%08lX AllocPage:%u) "
					"PDIdx:%u PTIdx:%u",
					 sTmpDevVAddr.uiAddr, sDevVAddr.uiAddr,
					 i, ui32PDIndex, ui32PTIndex);

				sTmpDevVAddr.uiAddr += uPageSize;

				continue;
			}

			pui32Tmp = (u32 *)ppsPTInfoList[0]->PTPageCpuVAddr;

			if (!pui32Tmp)
				continue;

			if (pui32Tmp[ui32PTIndex] & SGX_MMU_PTE_VALID) {
				ppsPTInfoList[0]->ui32ValidPTECount--;
			} else {
				PVR_DPF(PVR_DBG_MESSAGE,
					 "MMU_UnmapPagesAndFreePTs: "
					 "Page is already invalid for alloc at "
					 "VAddr:0x%08lX "
					 "(VAddrIni:0x%08lX AllocPage:%u) "
					 "PDIdx:%u PTIdx:%u",
					 sTmpDevVAddr.uiAddr, sDevVAddr.uiAddr,
					 i, ui32PDIndex, ui32PTIndex);
			}

			PVR_ASSERT((s32)ppsPTInfoList[0]->ui32ValidPTECount >=
									0);
			pui32Tmp[ui32PTIndex] = 0;
		}

		if (ppsPTInfoList[0]
		    && ppsPTInfoList[0]->ui32ValidPTECount == 0) {
			_DeferredFreePageTable(psMMUHeap,
					       ui32PDIndex - (psMMUHeap->
						   ui32PTBaseIndex >>
						       SGX_MMU_PT_SHIFT));
			bInvalidateDirectoryCache = IMG_TRUE;
		}

		sTmpDevVAddr.uiAddr += uPageSize;
	}

	if (bInvalidateDirectoryCache) {
		MMU_InvalidateDirectoryCache(psMMUHeap->psMMUContext->
							     psDevInfo);
	} else {
		MMU_InvalidatePageTableCache(psMMUHeap->psMMUContext->
							     psDevInfo);
	}

#if defined(PDUMP)
	MMU_PDumpPageTables(psMMUHeap, sDevVAddr, uPageSize * ui32PageCount,
			    IMG_TRUE, hUniqueTag);
#endif
}

static void MMU_FreePageTables(void *pvMMUHeap, u32 ui32Start, u32 ui32End,
			       void *hUniqueTag)
{
	struct MMU_HEAP *pMMUHeap = (struct MMU_HEAP *)pvMMUHeap;
	struct IMG_DEV_VIRTADDR Start;

	Start.uiAddr = ui32Start;

	MMU_UnmapPagesAndFreePTs(pMMUHeap, Start,
				 (ui32End - ui32Start) / SGX_MMU_PAGE_SIZE,
				 hUniqueTag);
}

struct MMU_HEAP *MMU_Create(struct MMU_CONTEXT *psMMUContext,
			    struct DEV_ARENA_DESCRIPTOR *psDevArena,
			    struct RA_ARENA **ppsVMArena)
{
	struct MMU_HEAP *pMMUHeap;
	IMG_BOOL bRes;

	PVR_ASSERT(psDevArena != NULL);

	if (psDevArena == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "MMU_Create: invalid parameter");
		return NULL;
	}

	OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		   sizeof(struct MMU_HEAP), (void **)&pMMUHeap, NULL);
	if (pMMUHeap == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "MMU_Create: ERROR call to OSAllocMem failed");
		return NULL;
	}

	pMMUHeap->psMMUContext = psMMUContext;
	pMMUHeap->psDevArena = psDevArena;

	bRes = _AllocPageTables(pMMUHeap);
	if (!bRes) {
		PVR_DPF(PVR_DBG_ERROR,
			 "MMU_Create: ERROR call to _AllocPageTables failed");
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct MMU_HEAP),
			  pMMUHeap, NULL);
		return NULL;
	}

	pMMUHeap->psVMArena = RA_Create(psDevArena->pszName,
					psDevArena->BaseDevVAddr.uiAddr,
					psDevArena->ui32Size, NULL,
					SGX_MMU_PAGE_SIZE, NULL, NULL,
					MMU_FreePageTables, pMMUHeap);

	if (pMMUHeap->psVMArena == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "MMU_Create: ERROR call to RA_Create failed");
		_DeferredFreePageTables(pMMUHeap);
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct MMU_HEAP),
			  pMMUHeap, NULL);
		return NULL;
	}

	*ppsVMArena = pMMUHeap->psVMArena;

	return pMMUHeap;
}

void MMU_Delete(struct MMU_HEAP *pMMUHeap)
{
	if (pMMUHeap != NULL) {
		PVR_DPF(PVR_DBG_MESSAGE, "MMU_Delete");

		if (pMMUHeap->psVMArena)
			RA_Delete(pMMUHeap->psVMArena);
		_DeferredFreePageTables(pMMUHeap);

		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct MMU_HEAP),
			  pMMUHeap, NULL);
	}
}

IMG_BOOL MMU_Alloc(struct MMU_HEAP *pMMUHeap, size_t uSize, u32 uFlags,
		   u32 uDevVAddrAlignment, struct IMG_DEV_VIRTADDR *psDevVAddr)
{
	IMG_BOOL bStatus;

	PVR_DPF(PVR_DBG_MESSAGE,
		 "MMU_Alloc: uSize=0x%x, flags=0x%x, align=0x%x",
		 uSize, uFlags, uDevVAddrAlignment);

	if ((uFlags & PVRSRV_MEM_USER_SUPPLIED_DEVVADDR) == 0) {
		bStatus = RA_Alloc(pMMUHeap->psVMArena, uSize, NULL, 0,
				   uDevVAddrAlignment, &(psDevVAddr->uiAddr));
		if (!bStatus) {
			PVR_DPF(PVR_DBG_ERROR,
				 "MMU_Alloc: RA_Alloc of VMArena failed");
			return bStatus;
		}
	}

	bStatus = _DeferredAllocPagetables(pMMUHeap, *psDevVAddr, uSize);


	if (!bStatus) {
		PVR_DPF(PVR_DBG_ERROR,
			 "MMU_Alloc: _DeferredAllocPagetables failed");
		if ((uFlags & PVRSRV_MEM_USER_SUPPLIED_DEVVADDR) == 0)
			RA_Free(pMMUHeap->psVMArena, psDevVAddr->uiAddr,
				IMG_FALSE);
	}

	return bStatus;
}

void MMU_Free(struct MMU_HEAP *pMMUHeap, struct IMG_DEV_VIRTADDR DevVAddr,
	      u32 ui32Size)
{
	PVR_ASSERT(pMMUHeap != NULL);

	if (pMMUHeap == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "MMU_Free: invalid parameter");
		return;
	}

	PVR_DPF(PVR_DBG_MESSAGE,
		 "MMU_Free: mmu=%08X, dev_vaddr=%08X", pMMUHeap,
		 DevVAddr.uiAddr);

	if ((DevVAddr.uiAddr >= pMMUHeap->psDevArena->BaseDevVAddr.uiAddr) &&
	    (DevVAddr.uiAddr + ui32Size <=
	     pMMUHeap->psDevArena->BaseDevVAddr.uiAddr +
	     pMMUHeap->psDevArena->ui32Size)) {
		RA_Free(pMMUHeap->psVMArena, DevVAddr.uiAddr, IMG_TRUE);
		return;
	}

	BUG();

	PVR_DPF(PVR_DBG_ERROR,
		 "MMU_Free: Couldn't find DevVAddr %08X in a DevArena",
		 DevVAddr.uiAddr);
}

void MMU_Enable(struct MMU_HEAP *pMMUHeap)
{
	PVR_UNREFERENCED_PARAMETER(pMMUHeap);

}

void MMU_Disable(struct MMU_HEAP *pMMUHeap)
{
	PVR_UNREFERENCED_PARAMETER(pMMUHeap);

}

#if defined(PDUMP)
static void MMU_PDumpPageTables(struct MMU_HEAP *pMMUHeap,
		    struct IMG_DEV_VIRTADDR DevVAddr,
		    size_t uSize, IMG_BOOL bForUnmap, void *hUniqueTag)
{
	u32 ui32NumPTEntries;
	u32 ui32PTIndex;
	u32 *pui32PTEntry;

	struct MMU_PT_INFO **ppsPTInfoList;
	u32 ui32PDIndex;
	u32 ui32PTDumpCount;

	ui32NumPTEntries =
	    (uSize + SGX_MMU_PAGE_SIZE - 1) >> SGX_MMU_PAGE_SHIFT;

	ui32PDIndex =
	    DevVAddr.uiAddr >> (SGX_MMU_PAGE_SHIFT + SGX_MMU_PT_SHIFT);

	ppsPTInfoList = &pMMUHeap->psMMUContext->apsPTInfoList[ui32PDIndex];

	ui32PTIndex = (DevVAddr.uiAddr & SGX_MMU_PT_MASK) >> SGX_MMU_PAGE_SHIFT;

	PDUMPCOMMENT("Page table mods (num entries == %08X) %s",
		     ui32NumPTEntries, bForUnmap ? "(for unmap)" : "");

	while (ui32NumPTEntries > 0) {
		struct MMU_PT_INFO *psPTInfo = *ppsPTInfoList++;

		if (ui32NumPTEntries <= 1024 - ui32PTIndex)
			ui32PTDumpCount = ui32NumPTEntries;
		else
			ui32PTDumpCount = 1024 - ui32PTIndex;

		if (psPTInfo) {
			pui32PTEntry = (u32 *)psPTInfo->PTPageCpuVAddr;
			PDUMPMEM2(PVRSRV_DEVICE_TYPE_SGX,
				  (void *)&pui32PTEntry[ui32PTIndex],
				  ui32PTDumpCount * sizeof(u32), 0,
				  IMG_FALSE, PDUMP_PT_UNIQUETAG, hUniqueTag);
		}

		ui32NumPTEntries -= ui32PTDumpCount;

		ui32PTIndex = 0;
	}

	PDUMPCOMMENT("Finished page table mods %s",
		     bForUnmap ? "(for unmap)" : "");
}
#endif

static void MMU_MapPage(struct MMU_HEAP *pMMUHeap,
	    struct IMG_DEV_VIRTADDR DevVAddr,
	    struct IMG_DEV_PHYADDR DevPAddr, u32 ui32MemFlags)
{
	u32 ui32Index;
	u32 *pui32Tmp;
	u32 ui32MMUFlags = 0;
	struct MMU_PT_INFO **ppsPTInfoList;

	if (((PVRSRV_MEM_READ | PVRSRV_MEM_WRITE) & ui32MemFlags) ==
	    (PVRSRV_MEM_READ | PVRSRV_MEM_WRITE))
		ui32MMUFlags = 0;
	else if (PVRSRV_MEM_READ & ui32MemFlags)
		ui32MMUFlags |= SGX_MMU_PTE_READONLY;
	else if (PVRSRV_MEM_WRITE & ui32MemFlags)
		ui32MMUFlags |= SGX_MMU_PTE_WRITEONLY;

	if (PVRSRV_MEM_CACHE_CONSISTENT & ui32MemFlags)
		ui32MMUFlags |= SGX_MMU_PTE_CACHECONSISTENT;

	if (PVRSRV_MEM_EDM_PROTECT & ui32MemFlags)
		ui32MMUFlags |= SGX_MMU_PTE_EDMPROTECT;

	ui32Index = DevVAddr.uiAddr >> (SGX_MMU_PAGE_SHIFT + SGX_MMU_PT_SHIFT);

	ppsPTInfoList = &pMMUHeap->psMMUContext->apsPTInfoList[ui32Index];

	ui32Index = (DevVAddr.uiAddr & SGX_MMU_PT_MASK) >> SGX_MMU_PAGE_SHIFT;

	pui32Tmp = (u32 *) ppsPTInfoList[0]->PTPageCpuVAddr;


	if (pui32Tmp[ui32Index] & SGX_MMU_PTE_VALID)
		PVR_DPF(PVR_DBG_ERROR,
				"MMU_MapPage: "
				"Page is already valid for alloc at "
				"VAddr:0x%08lX PDIdx:%u PTIdx:%u",
			 DevVAddr.uiAddr,
			 DevVAddr.uiAddr >> (SGX_MMU_PAGE_SHIFT +
					     SGX_MMU_PT_SHIFT), ui32Index);

	PVR_ASSERT((pui32Tmp[ui32Index] & SGX_MMU_PTE_VALID) == 0);

	ppsPTInfoList[0]->ui32ValidPTECount++;

	pui32Tmp[ui32Index] = (DevPAddr.uiAddr & SGX_MMU_PTE_ADDR_MASK)
	    | SGX_MMU_PTE_VALID | ui32MMUFlags;
}

void MMU_MapScatter(struct MMU_HEAP *pMMUHeap, struct IMG_DEV_VIRTADDR DevVAddr,
		    struct IMG_SYS_PHYADDR *psSysAddr, size_t uSize,
		    u32 ui32MemFlags, void *hUniqueTag)
{
#if defined(PDUMP)
	struct IMG_DEV_VIRTADDR MapBaseDevVAddr;
#endif
	u32 uCount, i;
	struct IMG_DEV_PHYADDR DevPAddr;

	PVR_ASSERT(pMMUHeap != NULL);

#if defined(PDUMP)
	MapBaseDevVAddr = DevVAddr;
#else
	PVR_UNREFERENCED_PARAMETER(hUniqueTag);
#endif

	for (i = 0, uCount = 0; uCount < uSize;
	     i++, uCount += SGX_MMU_PAGE_SIZE) {
		struct IMG_SYS_PHYADDR sSysAddr;

		sSysAddr = psSysAddr[i];

		DevPAddr =
		    SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sSysAddr);

		MMU_MapPage(pMMUHeap, DevVAddr, DevPAddr, ui32MemFlags);
		DevVAddr.uiAddr += SGX_MMU_PAGE_SIZE;

		PVR_DPF(PVR_DBG_MESSAGE, "MMU_MapScatter: "
				"devVAddr=%08X, SysAddr=%08X, size=0x%x/0x%x",
			 DevVAddr.uiAddr, sSysAddr.uiAddr, uCount, uSize);
	}

#if defined(PDUMP)
	MMU_PDumpPageTables(pMMUHeap, MapBaseDevVAddr, uSize, IMG_FALSE,
			    hUniqueTag);
#endif
}

void MMU_MapPages(struct MMU_HEAP *pMMUHeap, struct IMG_DEV_VIRTADDR DevVAddr,
		  struct IMG_SYS_PHYADDR SysPAddr, size_t uSize,
		  u32 ui32MemFlags, void *hUniqueTag)
{
	struct IMG_DEV_PHYADDR DevPAddr;
#if defined(PDUMP)
	struct IMG_DEV_VIRTADDR MapBaseDevVAddr;
#endif
	u32 uCount;
	u32 ui32VAdvance = SGX_MMU_PAGE_SIZE;
	u32 ui32PAdvance = SGX_MMU_PAGE_SIZE;

	PVR_ASSERT(pMMUHeap != NULL);

	PVR_DPF(PVR_DBG_MESSAGE, "MMU_MapPages: "
		 "mmu=%08X, devVAddr=%08X, SysPAddr=%08X, size=0x%x",
		 pMMUHeap, DevVAddr.uiAddr, SysPAddr.uiAddr, uSize);

#if defined(PDUMP)
	MapBaseDevVAddr = DevVAddr;
#else
	PVR_UNREFERENCED_PARAMETER(hUniqueTag);
#endif

	DevPAddr = SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, SysPAddr);

	if (ui32MemFlags & PVRSRV_MEM_DUMMY)
		ui32PAdvance = 0;

	for (uCount = 0; uCount < uSize; uCount += ui32VAdvance) {
		MMU_MapPage(pMMUHeap, DevVAddr, DevPAddr, ui32MemFlags);
		DevVAddr.uiAddr += ui32VAdvance;
		DevPAddr.uiAddr += ui32PAdvance;
	}

#if defined(PDUMP)
	MMU_PDumpPageTables(pMMUHeap, MapBaseDevVAddr, uSize, IMG_FALSE,
			    hUniqueTag);
#endif
}

void MMU_MapShadow(struct MMU_HEAP *pMMUHeap,
	      struct IMG_DEV_VIRTADDR MapBaseDevVAddr,
	      size_t uByteSize, void *CpuVAddr, void *hOSMemHandle,
	      struct IMG_DEV_VIRTADDR *pDevVAddr, u32 ui32MemFlags,
	      void *hUniqueTag)
{
	u32 i;
	u32 uOffset = 0;
	struct IMG_DEV_VIRTADDR MapDevVAddr;
	u32 ui32VAdvance = SGX_MMU_PAGE_SIZE;
	u32 ui32PAdvance = SGX_MMU_PAGE_SIZE;

#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(hUniqueTag);
#endif

	PVR_DPF(PVR_DBG_MESSAGE,
		 "MMU_MapShadow: %08X, 0x%x, %08X",
		 MapBaseDevVAddr.uiAddr, uByteSize, CpuVAddr);

	PVR_ASSERT(((u32) CpuVAddr & (SGX_MMU_PAGE_SIZE - 1)) == 0);
	PVR_ASSERT(((u32) uByteSize & (SGX_MMU_PAGE_SIZE - 1)) == 0);
	pDevVAddr->uiAddr = MapBaseDevVAddr.uiAddr;

	if (ui32MemFlags & PVRSRV_MEM_DUMMY)
		ui32PAdvance = 0;

	MapDevVAddr = MapBaseDevVAddr;
	for (i = 0; i < uByteSize; i += ui32VAdvance) {
		struct IMG_CPU_PHYADDR CpuPAddr;
		struct IMG_DEV_PHYADDR DevPAddr;

		if (CpuVAddr)
			CpuPAddr =
			    OSMapLinToCPUPhys((void *)((u32)CpuVAddr +
								    uOffset));
		else
			CpuPAddr = OSMemHandleToCpuPAddr(hOSMemHandle, uOffset);
		DevPAddr =
		    SysCpuPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, CpuPAddr);

		PVR_DPF(PVR_DBG_MESSAGE, "0x%x: CpuVAddr=%08X, "
				"CpuPAddr=%08X, DevVAddr=%08X, DevPAddr=%08X",
			 uOffset, (u32)CpuVAddr + uOffset, CpuPAddr.uiAddr,
			 MapDevVAddr.uiAddr, DevPAddr.uiAddr);

		MMU_MapPage(pMMUHeap, MapDevVAddr, DevPAddr, ui32MemFlags);

		MapDevVAddr.uiAddr += ui32VAdvance;
		uOffset += ui32PAdvance;
	}

#if defined(PDUMP)
	MMU_PDumpPageTables(pMMUHeap, MapBaseDevVAddr, uByteSize, IMG_FALSE,
			    hUniqueTag);
#endif
}

void MMU_UnmapPages(struct MMU_HEAP *psMMUHeap,
		   struct IMG_DEV_VIRTADDR sDevVAddr, u32 ui32PageCount,
		   void *hUniqueTag)
{
	u32 uPageSize = HOST_PAGESIZE();
	struct IMG_DEV_VIRTADDR sTmpDevVAddr;
	u32 i;
	u32 ui32PDIndex;
	u32 ui32PTIndex;
	u32 *pui32Tmp;

#if !defined(PDUMP)
	PVR_UNREFERENCED_PARAMETER(hUniqueTag);
#endif

	sTmpDevVAddr = sDevVAddr;

	for (i = 0; i < ui32PageCount; i++) {
		struct MMU_PT_INFO **ppsPTInfoList;

		ui32PDIndex = sTmpDevVAddr.uiAddr >> (SGX_MMU_PAGE_SHIFT +
						      SGX_MMU_PT_SHIFT);

		ppsPTInfoList = &psMMUHeap->psMMUContext->
						apsPTInfoList[ui32PDIndex];

		ui32PTIndex = (sTmpDevVAddr.uiAddr & SGX_MMU_PT_MASK) >>
							SGX_MMU_PAGE_SHIFT;

		if (!ppsPTInfoList[0]) {
			PVR_DPF(PVR_DBG_ERROR,
				"MMU_UnmapPages: "
				"ERROR Invalid PT for alloc at VAddr:0x%08lX "
				"(VaddrIni:0x%08lX AllocPage:%u) PDIdx:%u "
				"PTIdx:%u",
				 sTmpDevVAddr.uiAddr, sDevVAddr.uiAddr, i,
				 ui32PDIndex, ui32PTIndex);

			sTmpDevVAddr.uiAddr += uPageSize;

			continue;
		}

		pui32Tmp = (u32 *)ppsPTInfoList[0]->PTPageCpuVAddr;

		if (pui32Tmp[ui32PTIndex] & SGX_MMU_PTE_VALID)
			ppsPTInfoList[0]->ui32ValidPTECount--;
		else
			PVR_DPF(PVR_DBG_ERROR,
				"MMU_UnmapPages: Page is already invalid "
				"for alloc at VAddr:0x%08lX "
				"(VAddrIni:0x%08lX AllocPage:%u) "
				"PDIdx:%u PTIdx:%u",
				 sTmpDevVAddr.uiAddr, sDevVAddr.uiAddr, i,
				 ui32PDIndex, ui32PTIndex);

		PVR_ASSERT((s32) ppsPTInfoList[0]->ui32ValidPTECount >= 0);

		pui32Tmp[ui32PTIndex] = 0;

		sTmpDevVAddr.uiAddr += uPageSize;
	}

	MMU_InvalidatePageTableCache(psMMUHeap->psMMUContext->psDevInfo);

#if defined(PDUMP)
	MMU_PDumpPageTables(psMMUHeap, sDevVAddr, uPageSize * ui32PageCount,
			    IMG_TRUE, hUniqueTag);
#endif
}

struct IMG_DEV_PHYADDR MMU_GetPhysPageAddr(struct MMU_HEAP *pMMUHeap,
					struct IMG_DEV_VIRTADDR sDevVPageAddr)
{
	u32 *pui32PageTable;
	u32 ui32Index;
	struct IMG_DEV_PHYADDR sDevPAddr;
	struct MMU_PT_INFO **ppsPTInfoList;

	ui32Index = sDevVPageAddr.uiAddr >> (SGX_MMU_PAGE_SHIFT +
					     SGX_MMU_PT_SHIFT);

	ppsPTInfoList = &pMMUHeap->psMMUContext->apsPTInfoList[ui32Index];
	if (!ppsPTInfoList[0]) {
		PVR_DPF(PVR_DBG_ERROR,
			 "MMU_GetPhysPageAddr: Not mapped in at 0x%08x",
			 sDevVPageAddr.uiAddr);
		sDevPAddr.uiAddr = 0;
		return sDevPAddr;
	}

	ui32Index =
	    (sDevVPageAddr.uiAddr & SGX_MMU_PT_MASK) >> SGX_MMU_PAGE_SHIFT;

	pui32PageTable = (u32 *) ppsPTInfoList[0]->PTPageCpuVAddr;

	sDevPAddr.uiAddr = pui32PageTable[ui32Index];

	sDevPAddr.uiAddr &= SGX_MMU_PTE_ADDR_MASK;

	return sDevPAddr;
}

struct IMG_DEV_PHYADDR MMU_GetPDDevPAddr(struct MMU_CONTEXT *pMMUContext)
{
	return pMMUContext->sPDDevPAddr;
}

enum PVRSRV_ERROR SGXGetPhysPageAddrKM(void *hDevMemHeap,
				      struct IMG_DEV_VIRTADDR sDevVAddr,
				      struct IMG_DEV_PHYADDR *pDevPAddr,
				      struct IMG_CPU_PHYADDR *pCpuPAddr)
{
	struct MMU_HEAP *pMMUHeap;
	struct IMG_DEV_PHYADDR DevPAddr;

	pMMUHeap = (struct MMU_HEAP *)BM_GetMMUHeap(hDevMemHeap);

	DevPAddr = MMU_GetPhysPageAddr(pMMUHeap, sDevVAddr);
	pCpuPAddr->uiAddr = DevPAddr.uiAddr;
	pDevPAddr->uiAddr = DevPAddr.uiAddr;

	return (pDevPAddr->uiAddr != 0) ?
		PVRSRV_OK : PVRSRV_ERROR_INVALID_PARAMS;
}

enum PVRSRV_ERROR SGXGetMMUPDAddrKM(void *hDevCookie,
			       void *hDevMemContext,
			       struct IMG_DEV_PHYADDR *psPDDevPAddr)
{
	if (!hDevCookie || !hDevMemContext || !psPDDevPAddr)
		return PVRSRV_ERROR_INVALID_PARAMS;

	*psPDDevPAddr =
	    ((struct BM_CONTEXT *)hDevMemContext)->psMMUContext->sPDDevPAddr;

	return PVRSRV_OK;
}

enum PVRSRV_ERROR MMU_BIFResetPDAlloc(struct PVRSRV_SGXDEV_INFO *psDevInfo)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;
	struct RA_ARENA *psLocalDevMemArena;
	void *hOSMemHandle = NULL;
	u8 *pui8MemBlock = NULL;
	struct IMG_SYS_PHYADDR sMemBlockSysPAddr;
	struct IMG_CPU_PHYADDR sMemBlockCpuPAddr;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
		   "MMU_BIFResetPDAlloc: ERROR call to SysAcquireData failed");
		return eError;
	}

	psLocalDevMemArena = psSysData->apsLocalDevMemArena[0];

	if (psLocalDevMemArena == NULL) {

		eError =
		    OSAllocPages(PVRSRV_HAP_WRITECOMBINE |
				 PVRSRV_HAP_KERNEL_ONLY, 3 * SGX_MMU_PAGE_SIZE,
				 SGX_MMU_PAGE_SIZE, (void **)&pui8MemBlock,
				 &hOSMemHandle);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "MMU_BIFResetPDAlloc: "
					"ERROR call to OSAllocPages failed");
			return eError;
		}
		sMemBlockCpuPAddr = OSMapLinToCPUPhys(pui8MemBlock);
	} else {
		if (RA_Alloc(psLocalDevMemArena, 3 * SGX_MMU_PAGE_SIZE,
			     NULL, 0, SGX_MMU_PAGE_SIZE,
			     &(sMemBlockSysPAddr.uiAddr)) != IMG_TRUE) {
			PVR_DPF(PVR_DBG_ERROR, "MMU_BIFResetPDAlloc: "
					"ERROR call to RA_Alloc failed");
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}

		sMemBlockCpuPAddr = SysSysPAddrToCpuPAddr(sMemBlockSysPAddr);
		pui8MemBlock = (void __force *)OSMapPhysToLin(sMemBlockCpuPAddr,
					      SGX_MMU_PAGE_SIZE * 3,
					      PVRSRV_HAP_WRITECOMBINE |
					      PVRSRV_HAP_KERNEL_ONLY,
					      &hOSMemHandle);
		if (!pui8MemBlock) {
			PVR_DPF(PVR_DBG_ERROR, "MMU_BIFResetPDAlloc: "
					"ERROR failed to map page tables");
			return PVRSRV_ERROR_BAD_MAPPING;
		}
	}

	psDevInfo->hBIFResetPDOSMemHandle = hOSMemHandle;
	psDevInfo->sBIFResetPDDevPAddr =
	    SysCpuPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sMemBlockCpuPAddr);
	psDevInfo->sBIFResetPTDevPAddr.uiAddr =
	    psDevInfo->sBIFResetPDDevPAddr.uiAddr + SGX_MMU_PAGE_SIZE;
	psDevInfo->sBIFResetPageDevPAddr.uiAddr =
	    psDevInfo->sBIFResetPTDevPAddr.uiAddr + SGX_MMU_PAGE_SIZE;
	psDevInfo->pui32BIFResetPD = (u32 *) pui8MemBlock;
	psDevInfo->pui32BIFResetPT =
	    (u32 *) (pui8MemBlock + SGX_MMU_PAGE_SIZE);

	OSMemSet(psDevInfo->pui32BIFResetPD, 0, SGX_MMU_PAGE_SIZE);
	OSMemSet(psDevInfo->pui32BIFResetPT, 0, SGX_MMU_PAGE_SIZE);

	OSMemSet(pui8MemBlock + (2 * SGX_MMU_PAGE_SIZE), 0xDB,
		 SGX_MMU_PAGE_SIZE);

	return PVRSRV_OK;
}

void MMU_BIFResetPDFree(struct PVRSRV_SGXDEV_INFO *psDevInfo)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;
	struct RA_ARENA *psLocalDevMemArena;
	struct IMG_SYS_PHYADDR sPDSysPAddr;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "MMU_BIFResetPDFree: "
				"ERROR call to SysAcquireData failed");
		return;
	}

	psLocalDevMemArena = psSysData->apsLocalDevMemArena[0];

	if (psLocalDevMemArena == NULL) {
		OSFreePages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
			    3 * SGX_MMU_PAGE_SIZE,
			    psDevInfo->pui32BIFResetPD,
			    psDevInfo->hBIFResetPDOSMemHandle);
	} else {
		OSUnMapPhysToLin((void __force __iomem *)
					psDevInfo->pui32BIFResetPD,
				 3 * SGX_MMU_PAGE_SIZE,
				 PVRSRV_HAP_WRITECOMBINE |
					PVRSRV_HAP_KERNEL_ONLY,
				 psDevInfo->hBIFResetPDOSMemHandle);

		sPDSysPAddr =
		    SysDevPAddrToSysPAddr(PVRSRV_DEVICE_TYPE_SGX,
					  psDevInfo->sBIFResetPDDevPAddr);
		RA_Free(psLocalDevMemArena, sPDSysPAddr.uiAddr, IMG_FALSE);
	}
}

u32 mmu_get_page_dir(struct MMU_CONTEXT *psMMUContext)
{
	return psMMUContext->sPDDevPAddr.uiAddr;
}
