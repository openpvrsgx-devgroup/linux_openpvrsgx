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

#include "sgxdefs.h"
#include "sgxmmu.h"
#include "services_headers.h"
#include "buffer_manager.h"
#include "hash.h"
#include "ra.h"
#include "pdump_km.h"
#include "sgxapi_km.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "mmu.h"
#include "sgxconfig.h"
#include "sgx_bridge_km.h"
#include "pdump_osfunc.h"

#define UINT32_MAX_VALUE	0xFFFFFFFFUL

#define SGX_MAX_PD_ENTRIES	(1<<(SGX_FEATURE_ADDRESS_SPACE_SIZE - SGX_MMU_PT_SHIFT - SGX_MMU_PAGE_SHIFT))

#if defined(FIX_HW_BRN_31620)
#define SGX_MMU_PDE_DUMMY_PAGE		(0)
#define SGX_MMU_PTE_DUMMY_PAGE		(0)

#define BRN31620_PT_ADDRESS_RANGE_SHIFT		22
#define BRN31620_PT_ADDRESS_RANGE_SIZE		(1 << BRN31620_PT_ADDRESS_RANGE_SHIFT)

#define BRN31620_PDE_CACHE_FILL_SHIFT		26
#define BRN31620_PDE_CACHE_FILL_SIZE		(1 << BRN31620_PDE_CACHE_FILL_SHIFT)
#define BRN31620_PDE_CACHE_FILL_MASK		(BRN31620_PDE_CACHE_FILL_SIZE - 1)

#define BRN31620_PDES_PER_CACHE_LINE_SHIFT	(BRN31620_PDE_CACHE_FILL_SHIFT - BRN31620_PT_ADDRESS_RANGE_SHIFT)
#define BRN31620_PDES_PER_CACHE_LINE_SIZE	(1 << BRN31620_PDES_PER_CACHE_LINE_SHIFT)
#define BRN31620_PDES_PER_CACHE_LINE_MASK	(BRN31620_PDES_PER_CACHE_LINE_SIZE - 1)

#define BRN31620_DUMMY_PAGE_OFFSET	(1 * SGX_MMU_PAGE_SIZE)
#define BRN31620_DUMMY_PDE_INDEX	(BRN31620_DUMMY_PAGE_OFFSET / BRN31620_PT_ADDRESS_RANGE_SIZE)
#define BRN31620_DUMMY_PTE_INDEX	((BRN31620_DUMMY_PAGE_OFFSET - (BRN31620_DUMMY_PDE_INDEX * BRN31620_PT_ADDRESS_RANGE_SIZE))/SGX_MMU_PAGE_SIZE)

#define BRN31620_CACHE_FLUSH_SHIFT		(32 - BRN31620_PDE_CACHE_FILL_SHIFT)
#define BRN31620_CACHE_FLUSH_SIZE		(1 << BRN31620_CACHE_FLUSH_SHIFT)

#define BRN31620_CACHE_FLUSH_BITS_SHIFT		5
#define BRN31620_CACHE_FLUSH_BITS_SIZE		(1 << BRN31620_CACHE_FLUSH_BITS_SHIFT)
#define BRN31620_CACHE_FLUSH_BITS_MASK		(BRN31620_CACHE_FLUSH_BITS_SIZE - 1)

#define BRN31620_CACHE_FLUSH_INDEX_BITS		(BRN31620_CACHE_FLUSH_SHIFT - BRN31620_CACHE_FLUSH_BITS_SHIFT)
#define BRN31620_CACHE_FLUSH_INDEX_SIZE		(1 << BRN31620_CACHE_FLUSH_INDEX_BITS)

#define BRN31620_DUMMY_PAGE_SIGNATURE	0xFEEBEE01
#endif

typedef struct _MMU_PT_INFO_
{
	
	IMG_VOID *hPTPageOSMemHandle;
	IMG_CPU_VIRTADDR PTPageCpuVAddr;
	
	
	IMG_UINT32 ui32ValidPTECount;
} MMU_PT_INFO;

struct _MMU_CONTEXT_
{
	
	PVRSRV_DEVICE_NODE *psDeviceNode;

	
	IMG_CPU_VIRTADDR pvPDCpuVAddr;
	IMG_DEV_PHYADDR sPDDevPAddr;

	IMG_VOID *hPDOSMemHandle;

	
	MMU_PT_INFO *apsPTInfoList[SGX_MAX_PD_ENTRIES];

	PVRSRV_SGXDEV_INFO *psDevInfo;

#if defined(PDUMP)
	IMG_UINT32 ui32PDumpMMUContextID;
#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
	IMG_BOOL bPDumpActive;
#endif
#endif

#if defined (FIX_HW_BRN_31620)
	IMG_UINT32 ui32PDChangeMask[BRN31620_CACHE_FLUSH_INDEX_SIZE];
	IMG_UINT32 ui32PDCacheRangeRefCount[BRN31620_CACHE_FLUSH_SIZE];
	MMU_PT_INFO *apsPTInfoListSave[SGX_MAX_PD_ENTRIES];
#endif
	struct _MMU_CONTEXT_ *psNext;
};

struct _MMU_HEAP_
{
	
	MMU_CONTEXT			*psMMUContext;

	

	
	IMG_UINT32			ui32PDBaseIndex;
	
	IMG_UINT32			ui32PageTableCount;
	
	IMG_UINT32			ui32PTETotalUsable;
	
	IMG_UINT32			ui32PDEPageSizeCtrl;

	

	
	IMG_UINT32			ui32DataPageSize;
	
	IMG_UINT32			ui32DataPageBitWidth;
	
	IMG_UINT32			ui32DataPageMask;

	

	
	IMG_UINT32			ui32PTShift;
	
	IMG_UINT32			ui32PTBitWidth;
	
	IMG_UINT32			ui32PTMask;
	
	IMG_UINT32			ui32PTSize;
	
	IMG_UINT32			ui32PTNumEntriesAllocated;
	
	IMG_UINT32			ui32PTNumEntriesUsable;

	

	
	IMG_UINT32			ui32PDShift;
	
	IMG_UINT32			ui32PDBitWidth;
	
	IMG_UINT32			ui32PDMask;

	

	RA_ARENA *psVMArena;
	DEV_ARENA_DESCRIPTOR *psDevArena;
#if defined(PDUMP)
	PDUMP_MMU_ATTRIB sMMUAttrib;
#endif
};



#if defined (SUPPORT_SGX_MMU_DUMMY_PAGE)
#define DUMMY_DATA_PAGE_SIGNATURE	0xDEADBEEF
#endif

static IMG_VOID
_DeferredFreePageTable (MMU_HEAP *pMMUHeap, IMG_UINT32 ui32PTIndex, IMG_BOOL bOSFreePT);

#if defined(PDUMP)
static IMG_VOID
MMU_PDumpPageTables	(MMU_HEAP *pMMUHeap,
					 IMG_DEV_VIRTADDR DevVAddr,
					 IMG_SIZE_T uSize,
					 IMG_BOOL bForUnmap,
					 IMG_HANDLE hUniqueTag);
#endif 

#define PAGE_TEST					0
#if PAGE_TEST
static IMG_VOID PageTest(IMG_VOID* pMem, IMG_DEV_PHYADDR sDevPAddr);
#endif

#define PT_DUMP 1

#define PT_DEBUG 0
#if (PT_DEBUG || PT_DUMP) && defined(PVRSRV_NEED_PVR_DPF)
static IMG_VOID DumpPT(MMU_PT_INFO *psPTInfoList)
{
	IMG_UINT32 *p = (IMG_UINT32*)psPTInfoList->PTPageCpuVAddr;
	IMG_UINT32 i;

	
	for(i = 0; i < 1024; i += 8)
	{
		PVR_DPF((PVR_DBG_ERROR,
				 "%08X %08X %08X %08X %08X %08X %08X %08X\n",
				 p[i + 0], p[i + 1], p[i + 2], p[i + 3],
				 p[i + 4], p[i + 5], p[i + 6], p[i + 7]));
	}
}
#else 
static INLINE IMG_VOID DumpPT(MMU_PT_INFO *psPTInfoList)
{
	PVR_UNREFERENCED_PARAMETER(psPTInfoList);
}
#endif 

#if PT_DEBUG
static IMG_VOID CheckPT(MMU_PT_INFO *psPTInfoList)
{
	IMG_UINT32 *p = (IMG_UINT32*) psPTInfoList->PTPageCpuVAddr;
	IMG_UINT32 i, ui32Count = 0;

	
	for(i = 0; i < 1024; i++)
		if(p[i] & SGX_MMU_PTE_VALID)
			ui32Count++;

	if(psPTInfoList->ui32ValidPTECount != ui32Count)
	{
		PVR_DPF((PVR_DBG_ERROR, "ui32ValidPTECount: %u ui32Count: %u\n",
				 psPTInfoList->ui32ValidPTECount, ui32Count));
		DumpPT(psPTInfoList);
		BUG();
	}
}
#else 
static INLINE IMG_VOID CheckPT(MMU_PT_INFO *psPTInfoList)
{
	PVR_UNREFERENCED_PARAMETER(psPTInfoList);
}
#endif 

#if defined(PVRSRV_MMU_MAKE_READWRITE_ON_DEMAND)

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#else
#include <generated/autoconf.h>
#endif

#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/highmem.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>

static IMG_VOID MakeKernelPageReadWrite(IMG_PVOID ulCPUVAddr)
{
    pgd_t *psPGD;
    pud_t *psPUD;
    pmd_t *psPMD;
    pte_t *psPTE;
    pte_t ptent;
    IMG_UINT32 ui32CPUVAddr = (IMG_UINT32) ulCPUVAddr;

    psPGD = pgd_offset_k(ui32CPUVAddr);
    if (pgd_none(*psPGD) || pgd_bad(*psPGD))
    {
        PVR_ASSERT(0);
    }

    psPUD = pud_offset(psPGD, ui32CPUVAddr);
    if (pud_none(*psPUD) || pud_bad(*psPUD))
    {
        PVR_ASSERT(0);
    }

    psPMD = pmd_offset(psPUD, ui32CPUVAddr);
    if (pmd_none(*psPMD) || pmd_bad(*psPMD))
    {
        PVR_ASSERT(0);
    }
	psPTE = (pte_t *)pte_offset_kernel(psPMD, ui32CPUVAddr);

	ptent = ptep_modify_prot_start(&init_mm, ui32CPUVAddr, psPTE);
	ptent = pte_mkwrite(ptent);
	ptep_modify_prot_commit(&init_mm, ui32CPUVAddr, psPTE, ptent);

	flush_tlb_all();
}

static IMG_VOID MakeKernelPageReadOnly(IMG_PVOID ulCPUVAddr)
{
    pgd_t *psPGD;
    pud_t *psPUD;
    pmd_t *psPMD;
    pte_t *psPTE;
    pte_t ptent;
    IMG_UINT32 ui32CPUVAddr = (IMG_UINT32) ulCPUVAddr;

	OSWriteMemoryBarrier();

    psPGD = pgd_offset_k(ui32CPUVAddr);
    if (pgd_none(*psPGD) || pgd_bad(*psPGD))
    {
        PVR_ASSERT(0);
    }

    psPUD = pud_offset(psPGD, ui32CPUVAddr);
    if (pud_none(*psPUD) || pud_bad(*psPUD))
    {
        PVR_ASSERT(0);
    }

    psPMD = pmd_offset(psPUD, ui32CPUVAddr);
    if (pmd_none(*psPMD) || pmd_bad(*psPMD))
    {
        PVR_ASSERT(0);
    }

	psPTE = (pte_t *)pte_offset_kernel(psPMD, ui32CPUVAddr);

	ptent = ptep_modify_prot_start(&init_mm, ui32CPUVAddr, psPTE);
	ptent = pte_wrprotect(ptent);
	ptep_modify_prot_commit(&init_mm, ui32CPUVAddr, psPTE, ptent);

	flush_tlb_all();

}

#else 

static INLINE IMG_VOID MakeKernelPageReadWrite(IMG_PVOID ulCPUVAddr)
{
	PVR_UNREFERENCED_PARAMETER(ulCPUVAddr);
}

static INLINE IMG_VOID MakeKernelPageReadOnly(IMG_PVOID ulCPUVAddr)
{
	PVR_UNREFERENCED_PARAMETER(ulCPUVAddr);
}

#endif 

IMG_BOOL MMU_IsHeapShared(MMU_HEAP* pMMUHeap)
{
	switch(pMMUHeap->psDevArena->DevMemHeapType)
	{
		case DEVICE_MEMORY_HEAP_SHARED :
		case DEVICE_MEMORY_HEAP_SHARED_EXPORTED :
			return IMG_TRUE;
		case DEVICE_MEMORY_HEAP_PERCONTEXT :
		case DEVICE_MEMORY_HEAP_KERNEL :
			return IMG_FALSE;
		default:
		{
			PVR_DPF((PVR_DBG_ERROR, "MMU_IsHeapShared: ERROR invalid heap type"));
			return IMG_FALSE;
		}
	}
}

#ifdef SUPPORT_SGX_MMU_BYPASS
IMG_VOID
EnableHostAccess (MMU_CONTEXT *psMMUContext)
{
	IMG_UINT32 ui32RegVal;
	IMG_VOID *pvRegsBaseKM = psMMUContext->psDevInfo->pvRegsBaseKM;

	


	ui32RegVal = OSReadHWReg(pvRegsBaseKM, EUR_CR_BIF_CTRL);

	OSWriteHWReg(pvRegsBaseKM,
				EUR_CR_BIF_CTRL,
				ui32RegVal | EUR_CR_BIF_CTRL_MMU_BYPASS_HOST_MASK);
	
	PDUMPREG(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, EUR_CR_BIF_CTRL_MMU_BYPASS_HOST_MASK);
}

IMG_VOID
DisableHostAccess (MMU_CONTEXT *psMMUContext)
{
	IMG_UINT32 ui32RegVal;
	IMG_VOID *pvRegsBaseKM = psMMUContext->psDevInfo->pvRegsBaseKM;

	



	OSWriteHWReg(pvRegsBaseKM,
				EUR_CR_BIF_CTRL,
				ui32RegVal & ~EUR_CR_BIF_CTRL_MMU_BYPASS_HOST_MASK);
	
	PDUMPREG(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, 0);
}
#endif


#if defined(SGX_FEATURE_SYSTEM_CACHE)
static IMG_VOID MMU_InvalidateSystemLevelCache(PVRSRV_SGXDEV_INFO *psDevInfo)
{
	#if defined(SGX_FEATURE_MP)
	psDevInfo->ui32CacheControl |= SGXMKIF_CC_INVAL_BIF_SL;
	#else
	
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	#endif 
}
#endif 

IMG_VOID MMU_InvalidateDirectoryCache(PVRSRV_SGXDEV_INFO *psDevInfo)
{
	psDevInfo->ui32CacheControl |= SGXMKIF_CC_INVAL_BIF_PD;
	#if defined(SGX_FEATURE_SYSTEM_CACHE)
	MMU_InvalidateSystemLevelCache(psDevInfo);
	#endif 
}


static IMG_VOID MMU_InvalidatePageTableCache(PVRSRV_SGXDEV_INFO *psDevInfo)
{
	psDevInfo->ui32CacheControl |= SGXMKIF_CC_INVAL_BIF_PT;
	#if defined(SGX_FEATURE_SYSTEM_CACHE)
	MMU_InvalidateSystemLevelCache(psDevInfo);
	#endif 
}

#if defined(FIX_HW_BRN_31620)
static IMG_VOID BRN31620InvalidatePageTableEntry(MMU_CONTEXT *psMMUContext, IMG_UINT32 ui32PDIndex, IMG_UINT32 ui32PTIndex, IMG_UINT32 *pui32PTE)
{
	PVRSRV_SGXDEV_INFO *psDevInfo = psMMUContext->psDevInfo;

	
	if (((ui32PDIndex % (BRN31620_PDE_CACHE_FILL_SIZE/BRN31620_PT_ADDRESS_RANGE_SIZE)) == BRN31620_DUMMY_PDE_INDEX)
		&& (ui32PTIndex == BRN31620_DUMMY_PTE_INDEX))
	{
		*pui32PTE = (psDevInfo->sBRN31620DummyPageDevPAddr.uiAddr>>SGX_MMU_PTE_ADDR_ALIGNSHIFT)
								| SGX_MMU_PTE_DUMMY_PAGE
								| SGX_MMU_PTE_READONLY
								| SGX_MMU_PTE_VALID;
	}
	else
	{
		*pui32PTE = 0;
	}
}

static IMG_BOOL BRN31620FreePageTable(MMU_HEAP *psMMUHeap, IMG_UINT32 ui32PDIndex)
{
	MMU_CONTEXT *psMMUContext = psMMUHeap->psMMUContext;
	PVRSRV_SGXDEV_INFO *psDevInfo = psMMUContext->psDevInfo;
	IMG_UINT32 ui32PDCacheLine = ui32PDIndex >> BRN31620_PDES_PER_CACHE_LINE_SHIFT;
	IMG_UINT32 bFreePTs = IMG_FALSE;
	IMG_UINT32 *pui32Tmp;

	PVR_ASSERT(psMMUHeap != IMG_NULL);

	
	PVR_ASSERT(psMMUContext->apsPTInfoListSave[ui32PDIndex] == IMG_NULL);

	psMMUContext->apsPTInfoListSave[ui32PDIndex] = psMMUContext->apsPTInfoList[ui32PDIndex];
	psMMUContext->apsPTInfoList[ui32PDIndex] = IMG_NULL;

	
	if (--psMMUContext->ui32PDCacheRangeRefCount[ui32PDCacheLine] == 0)
	{
		IMG_UINT32 i;
		IMG_UINT32 ui32PDIndexStart = ui32PDCacheLine * BRN31620_PDES_PER_CACHE_LINE_SIZE;
		IMG_UINT32 ui32PDIndexEnd = ui32PDIndexStart + BRN31620_PDES_PER_CACHE_LINE_SIZE;
		IMG_UINT32 ui32PDBitMaskIndex, ui32PDBitMaskShift;

		
		for (i=ui32PDIndexStart;i<ui32PDIndexEnd;i++)
		{
			
			psMMUContext->apsPTInfoList[i] = psMMUContext->apsPTInfoListSave[i];
			psMMUContext->apsPTInfoListSave[i] = IMG_NULL;
			_DeferredFreePageTable(psMMUHeap, i - psMMUHeap->ui32PDBaseIndex, IMG_TRUE);
		}

		ui32PDBitMaskIndex = ui32PDCacheLine >> BRN31620_CACHE_FLUSH_BITS_SHIFT;
		ui32PDBitMaskShift = ui32PDCacheLine & BRN31620_CACHE_FLUSH_BITS_MASK;

		
		if (MMU_IsHeapShared(psMMUHeap))
		{
			
			MMU_CONTEXT *psMMUContextWalker = (MMU_CONTEXT*) psMMUHeap->psMMUContext->psDevInfo->pvMMUContextList;

			while(psMMUContextWalker)
			{
				psMMUContextWalker->ui32PDChangeMask[ui32PDBitMaskIndex] |= 1 << ui32PDBitMaskShift;

				
				MakeKernelPageReadWrite(psMMUContextWalker->pvPDCpuVAddr);
				pui32Tmp = (IMG_UINT32 *) psMMUContextWalker->pvPDCpuVAddr;
				pui32Tmp[ui32PDIndexStart + BRN31620_DUMMY_PDE_INDEX] = (psDevInfo->sBRN31620DummyPTDevPAddr.uiAddr>>SGX_MMU_PDE_ADDR_ALIGNSHIFT)
												| SGX_MMU_PDE_PAGE_SIZE_4K
												| SGX_MMU_PDE_DUMMY_PAGE
												| SGX_MMU_PDE_VALID;
				MakeKernelPageReadOnly(psMMUContextWalker->pvPDCpuVAddr);

				PDUMPCOMMENT("BRN31620 Re-wire dummy PT due to releasing PT allocation block");
				PDUMPPDENTRIES(&psMMUHeap->sMMUAttrib, psMMUContextWalker->hPDOSMemHandle, (IMG_VOID*)&pui32Tmp[ui32PDIndexStart + BRN31620_DUMMY_PDE_INDEX], sizeof(IMG_UINT32), 0, IMG_FALSE, PDUMP_PT_UNIQUETAG, PDUMP_PT_UNIQUETAG);
				psMMUContextWalker = psMMUContextWalker->psNext;
			}
		}
		else
		{
			psMMUContext->ui32PDChangeMask[ui32PDBitMaskIndex] |= 1 << ui32PDBitMaskShift;

			
			MakeKernelPageReadWrite(psMMUContext->pvPDCpuVAddr);
			pui32Tmp = (IMG_UINT32 *) psMMUContext->pvPDCpuVAddr;
			pui32Tmp[ui32PDIndexStart + BRN31620_DUMMY_PDE_INDEX] = (psDevInfo->sBRN31620DummyPTDevPAddr.uiAddr>>SGX_MMU_PDE_ADDR_ALIGNSHIFT)
											| SGX_MMU_PDE_PAGE_SIZE_4K
											| SGX_MMU_PDE_DUMMY_PAGE
											| SGX_MMU_PDE_VALID;
			MakeKernelPageReadOnly(psMMUContext->pvPDCpuVAddr);

			PDUMPCOMMENT("BRN31620 Re-wire dummy PT due to releasing PT allocation block");
			PDUMPPDENTRIES(&psMMUHeap->sMMUAttrib, psMMUContext->hPDOSMemHandle, (IMG_VOID*)&pui32Tmp[ui32PDIndexStart + BRN31620_DUMMY_PDE_INDEX], sizeof(IMG_UINT32), 0, IMG_FALSE, PDUMP_PT_UNIQUETAG, PDUMP_PT_UNIQUETAG);
		}
		
		bFreePTs = IMG_TRUE;
	}

	return bFreePTs;
}
#endif

static IMG_BOOL
_AllocPageTableMemory (MMU_HEAP *pMMUHeap,
						MMU_PT_INFO *psPTInfoList,
						IMG_DEV_PHYADDR	*psDevPAddr)
{
	IMG_DEV_PHYADDR	sDevPAddr;
	IMG_CPU_PHYADDR sCpuPAddr;

	


	if(pMMUHeap->psDevArena->psDeviceMemoryHeapInfo->psLocalDevMemArena == IMG_NULL)
	{
		
		if (OSAllocPages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
						 pMMUHeap->ui32PTSize,
						 SGX_MMU_PAGE_SIZE,
						 IMG_NULL,
						 0,
						 (IMG_VOID **)&psPTInfoList->PTPageCpuVAddr,
						 &psPTInfoList->hPTPageOSMemHandle) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "_AllocPageTableMemory: ERROR call to OSAllocPages failed"));
			return IMG_FALSE;
		}

		


		MakeKernelPageReadOnly(psPTInfoList->PTPageCpuVAddr);

		
		if(psPTInfoList->PTPageCpuVAddr)
		{
			sCpuPAddr = OSMapLinToCPUPhys(psPTInfoList->hPTPageOSMemHandle,
										  psPTInfoList->PTPageCpuVAddr);
		}
		else
		{
			
			sCpuPAddr = OSMemHandleToCpuPAddr(psPTInfoList->hPTPageOSMemHandle, 0);
		}

		sDevPAddr = SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);
	}
	else
	{
		IMG_SYS_PHYADDR sSysPAddr;

		


		
		if(RA_Alloc(pMMUHeap->psDevArena->psDeviceMemoryHeapInfo->psLocalDevMemArena,
					SGX_MMU_PAGE_SIZE,
					IMG_NULL,
					IMG_NULL,
					0,
					SGX_MMU_PAGE_SIZE,
					0,
					IMG_NULL,
					0,
					&(sSysPAddr.uiAddr))!= IMG_TRUE)
		{
			PVR_DPF((PVR_DBG_ERROR, "_AllocPageTableMemory: ERROR call to RA_Alloc failed"));
			return IMG_FALSE;
		}

		
		sCpuPAddr = SysSysPAddrToCpuPAddr(sSysPAddr);
		
		psPTInfoList->PTPageCpuVAddr = OSMapPhysToLin(sCpuPAddr,
													SGX_MMU_PAGE_SIZE,
													PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
													&psPTInfoList->hPTPageOSMemHandle);
		if(!psPTInfoList->PTPageCpuVAddr)
		{
			PVR_DPF((PVR_DBG_ERROR, "_AllocPageTableMemory: ERROR failed to map page tables"));
			return IMG_FALSE;
		}

		
		sDevPAddr = SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);

		#if PAGE_TEST
		PageTest(psPTInfoList->PTPageCpuVAddr, sDevPAddr);
		#endif
	}

	MakeKernelPageReadWrite(psPTInfoList->PTPageCpuVAddr);
#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
	{
		IMG_UINT32 *pui32Tmp;
		IMG_UINT32 i;

		pui32Tmp = (IMG_UINT32*)psPTInfoList->PTPageCpuVAddr;
		
		for(i=0; i<pMMUHeap->ui32PTNumEntriesUsable; i++)
		{
			pui32Tmp[i] = (pMMUHeap->psMMUContext->psDevInfo->sDummyDataDevPAddr.uiAddr>>SGX_MMU_PTE_ADDR_ALIGNSHIFT)
						| SGX_MMU_PTE_VALID;
		}
		
		for(; i<pMMUHeap->ui32PTNumEntriesAllocated; i++)
		{
			pui32Tmp[i] = 0;
		}
	}
#else
	
	OSMemSet(psPTInfoList->PTPageCpuVAddr, 0, pMMUHeap->ui32PTSize);
#endif
	MakeKernelPageReadOnly(psPTInfoList->PTPageCpuVAddr);

#if defined(PDUMP)
	{
		IMG_UINT32 ui32Flags = 0;
#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
		
		ui32Flags |= ( MMU_IsHeapShared(pMMUHeap) ) ? PDUMP_FLAGS_PERSISTENT : 0;
#endif
		
		PDUMPMALLOCPAGETABLE(&pMMUHeap->psMMUContext->psDeviceNode->sDevId, psPTInfoList->hPTPageOSMemHandle, 0, psPTInfoList->PTPageCpuVAddr, pMMUHeap->ui32PTSize, ui32Flags, PDUMP_PT_UNIQUETAG);
		
		PDUMPMEMPTENTRIES(&pMMUHeap->sMMUAttrib, psPTInfoList->hPTPageOSMemHandle, psPTInfoList->PTPageCpuVAddr, pMMUHeap->ui32PTSize, ui32Flags, IMG_TRUE, PDUMP_PT_UNIQUETAG, PDUMP_PT_UNIQUETAG);
	}
#endif
	
	
	*psDevPAddr = sDevPAddr;

	return IMG_TRUE;
}


static IMG_VOID
_FreePageTableMemory (MMU_HEAP *pMMUHeap, MMU_PT_INFO *psPTInfoList)
{
	



	if(pMMUHeap->psDevArena->psDeviceMemoryHeapInfo->psLocalDevMemArena == IMG_NULL)
	{
		
		MakeKernelPageReadWrite(psPTInfoList->PTPageCpuVAddr);

		
		OSFreePages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
					  pMMUHeap->ui32PTSize,
					  psPTInfoList->PTPageCpuVAddr,
					  psPTInfoList->hPTPageOSMemHandle);
	}
	else
	{
		IMG_SYS_PHYADDR sSysPAddr;
		IMG_CPU_PHYADDR sCpuPAddr;

		
		sCpuPAddr = OSMapLinToCPUPhys(psPTInfoList->hPTPageOSMemHandle, 
									  psPTInfoList->PTPageCpuVAddr);
		sSysPAddr = SysCpuPAddrToSysPAddr (sCpuPAddr);

		
		
		OSUnMapPhysToLin(psPTInfoList->PTPageCpuVAddr,
                         SGX_MMU_PAGE_SIZE,
                         PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
                         psPTInfoList->hPTPageOSMemHandle);

		


		RA_Free (pMMUHeap->psDevArena->psDeviceMemoryHeapInfo->psLocalDevMemArena, sSysPAddr.uiAddr, IMG_FALSE);
	}
}



static IMG_VOID
_DeferredFreePageTable (MMU_HEAP *pMMUHeap, IMG_UINT32 ui32PTIndex, IMG_BOOL bOSFreePT)
{
	IMG_UINT32 *pui32PDEntry;
	IMG_UINT32 i;
	IMG_UINT32 ui32PDIndex;
	SYS_DATA *psSysData;
	MMU_PT_INFO **ppsPTInfoList;

	SysAcquireData(&psSysData);

	
	ui32PDIndex = pMMUHeap->psDevArena->BaseDevVAddr.uiAddr >> pMMUHeap->ui32PDShift;

	
	ppsPTInfoList = &pMMUHeap->psMMUContext->apsPTInfoList[ui32PDIndex];

	{
#if PT_DEBUG
		if(ppsPTInfoList[ui32PTIndex] && ppsPTInfoList[ui32PTIndex]->ui32ValidPTECount > 0)
		{
			DumpPT(ppsPTInfoList[ui32PTIndex]);
			
		}
#endif

		
		PVR_ASSERT(ppsPTInfoList[ui32PTIndex] == IMG_NULL || ppsPTInfoList[ui32PTIndex]->ui32ValidPTECount == 0);
	}

#if defined(PDUMP)
	{
		IMG_UINT32 ui32Flags = 0;
#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
		ui32Flags |= ( MMU_IsHeapShared(pMMUHeap) ) ? PDUMP_FLAGS_PERSISTENT : 0;
#endif
		
		PDUMPCOMMENT("Free page table (page count == %08X)", pMMUHeap->ui32PageTableCount);
		if(ppsPTInfoList[ui32PTIndex] && ppsPTInfoList[ui32PTIndex]->PTPageCpuVAddr)
		{
			PDUMPFREEPAGETABLE(&pMMUHeap->psMMUContext->psDeviceNode->sDevId, ppsPTInfoList[ui32PTIndex]->hPTPageOSMemHandle, ppsPTInfoList[ui32PTIndex]->PTPageCpuVAddr, pMMUHeap->ui32PTSize, ui32Flags, PDUMP_PT_UNIQUETAG);
		}
	}
#endif

	switch(pMMUHeap->psDevArena->DevMemHeapType)
	{
		case DEVICE_MEMORY_HEAP_SHARED :
		case DEVICE_MEMORY_HEAP_SHARED_EXPORTED :
		{
			
			MMU_CONTEXT *psMMUContext = (MMU_CONTEXT*)pMMUHeap->psMMUContext->psDevInfo->pvMMUContextList;

			while(psMMUContext)
			{
				
				MakeKernelPageReadWrite(psMMUContext->pvPDCpuVAddr);
				pui32PDEntry = (IMG_UINT32*)psMMUContext->pvPDCpuVAddr;
				pui32PDEntry += ui32PDIndex;

#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
				
				pui32PDEntry[ui32PTIndex] = (psMMUContext->psDevInfo->sDummyPTDevPAddr.uiAddr
											>>SGX_MMU_PDE_ADDR_ALIGNSHIFT)
											| SGX_MMU_PDE_PAGE_SIZE_4K
											| SGX_MMU_PDE_VALID;
#else
				
				if(bOSFreePT)
				{
					pui32PDEntry[ui32PTIndex] = 0;
				}
#endif
				MakeKernelPageReadOnly(psMMUContext->pvPDCpuVAddr);
			#if defined(PDUMP)
				
			#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
				if(psMMUContext->bPDumpActive)
			#endif
				{
					PDUMPPDENTRIES(&pMMUHeap->sMMUAttrib, psMMUContext->hPDOSMemHandle, (IMG_VOID*)&pui32PDEntry[ui32PTIndex], sizeof(IMG_UINT32), 0, IMG_FALSE, PDUMP_PT_UNIQUETAG, PDUMP_PT_UNIQUETAG);
				}
			#endif
				
				psMMUContext = psMMUContext->psNext;
			}
			break;
		}
		case DEVICE_MEMORY_HEAP_PERCONTEXT :
		case DEVICE_MEMORY_HEAP_KERNEL :
		{
			MakeKernelPageReadWrite(pMMUHeap->psMMUContext->pvPDCpuVAddr);
			
			pui32PDEntry = (IMG_UINT32*)pMMUHeap->psMMUContext->pvPDCpuVAddr;
			pui32PDEntry += ui32PDIndex;

#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
			
			pui32PDEntry[ui32PTIndex] = (pMMUHeap->psMMUContext->psDevInfo->sDummyPTDevPAddr.uiAddr
										>>SGX_MMU_PDE_ADDR_ALIGNSHIFT)
										| SGX_MMU_PDE_PAGE_SIZE_4K
										| SGX_MMU_PDE_VALID;
#else
			
			if(bOSFreePT)
			{
				pui32PDEntry[ui32PTIndex] = 0;
			}
#endif
			MakeKernelPageReadOnly(pMMUHeap->psMMUContext->pvPDCpuVAddr);

			
			PDUMPPDENTRIES(&pMMUHeap->sMMUAttrib, pMMUHeap->psMMUContext->hPDOSMemHandle, (IMG_VOID*)&pui32PDEntry[ui32PTIndex], sizeof(IMG_UINT32), 0, IMG_FALSE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
			break;
		}
		default:
		{
			PVR_DPF((PVR_DBG_ERROR, "_DeferredFreePagetable: ERROR invalid heap type"));
			return;
		}
	}

	
	if(ppsPTInfoList[ui32PTIndex] != IMG_NULL)
	{
		if(ppsPTInfoList[ui32PTIndex]->PTPageCpuVAddr != IMG_NULL)
		{
			IMG_PUINT32 pui32Tmp;

			MakeKernelPageReadWrite(ppsPTInfoList[ui32PTIndex]->PTPageCpuVAddr);
			pui32Tmp = (IMG_UINT32*)ppsPTInfoList[ui32PTIndex]->PTPageCpuVAddr;

			
			for(i=0;
				(i<pMMUHeap->ui32PTETotalUsable) && (i<pMMUHeap->ui32PTNumEntriesUsable);
				 i++)
			{
				
				pui32Tmp[i] = 0;
			}
			MakeKernelPageReadOnly(ppsPTInfoList[ui32PTIndex]->PTPageCpuVAddr);

			

			if(bOSFreePT)
			{
				_FreePageTableMemory(pMMUHeap, ppsPTInfoList[ui32PTIndex]);
			}

			


			pMMUHeap->ui32PTETotalUsable -= i;
		}
		else
		{
			
			pMMUHeap->ui32PTETotalUsable -= pMMUHeap->ui32PTNumEntriesUsable;
		}

		if(bOSFreePT)
		{
			
			OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof(MMU_PT_INFO),
						ppsPTInfoList[ui32PTIndex],
						IMG_NULL);
			ppsPTInfoList[ui32PTIndex] = IMG_NULL;
		}
	}
	else
	{
		
		pMMUHeap->ui32PTETotalUsable -= pMMUHeap->ui32PTNumEntriesUsable;
	}

	PDUMPCOMMENT("Finished free page table (page count == %08X)", pMMUHeap->ui32PageTableCount);
}

static IMG_VOID
_DeferredFreePageTables (MMU_HEAP *pMMUHeap)
{
	IMG_UINT32 i;
#if defined(FIX_HW_BRN_31620)
	MMU_CONTEXT *psMMUContext = pMMUHeap->psMMUContext;
	IMG_BOOL bInvalidateDirectoryCache = IMG_FALSE;
	IMG_UINT32 ui32PDIndex;
	IMG_UINT32 *pui32Tmp;
	IMG_UINT32 j;
#endif
#if defined(PDUMP)
	PDUMPCOMMENT("Free PTs (MMU Context ID == %u, PDBaseIndex == %u, PT count == 0x%x)",
			pMMUHeap->psMMUContext->ui32PDumpMMUContextID,
			pMMUHeap->ui32PDBaseIndex,
			pMMUHeap->ui32PageTableCount);
#endif
#if defined(FIX_HW_BRN_31620)
	for(i=0; i<pMMUHeap->ui32PageTableCount; i++)
	{
		ui32PDIndex = (pMMUHeap->ui32PDBaseIndex + i);

		if (psMMUContext->apsPTInfoList[ui32PDIndex])
		{
			if (psMMUContext->apsPTInfoList[ui32PDIndex]->PTPageCpuVAddr)
			{
				
				for (j=0;j<SGX_MMU_PT_SIZE;j++)
				{
					pui32Tmp = (IMG_UINT32 *) psMMUContext->apsPTInfoList[ui32PDIndex]->PTPageCpuVAddr;
					BRN31620InvalidatePageTableEntry(psMMUContext, ui32PDIndex, j, &pui32Tmp[j]);
				}
			}
			
			if (BRN31620FreePageTable(pMMUHeap, ui32PDIndex) == IMG_TRUE)
			{
				bInvalidateDirectoryCache = IMG_TRUE;
			}
		}
	}

	
	if (bInvalidateDirectoryCache)
	{
		MMU_InvalidateDirectoryCache(pMMUHeap->psMMUContext->psDevInfo);
	}
	else
	{
		MMU_InvalidatePageTableCache(pMMUHeap->psMMUContext->psDevInfo);
	}
#else
	for(i=0; i<pMMUHeap->ui32PageTableCount; i++)
	{
		_DeferredFreePageTable(pMMUHeap, i, IMG_TRUE);
	}
	MMU_InvalidateDirectoryCache(pMMUHeap->psMMUContext->psDevInfo);
#endif
}


static IMG_BOOL
_DeferredAllocPagetables(MMU_HEAP *pMMUHeap, IMG_DEV_VIRTADDR DevVAddr, IMG_UINT32 ui32Size)
{
	IMG_UINT32 ui32PageTableCount;
	IMG_UINT32 ui32PDIndex;
	IMG_UINT32 i;
	IMG_UINT32 *pui32PDEntry;
	MMU_PT_INFO **ppsPTInfoList;
	SYS_DATA *psSysData;
	IMG_DEV_VIRTADDR sHighDevVAddr;
#if defined(FIX_HW_BRN_31620)
	IMG_BOOL bFlushSystemCache = IMG_FALSE;
	IMG_BOOL bSharedPT = IMG_FALSE;
	IMG_DEV_VIRTADDR sDevVAddrRequestStart;
	IMG_DEV_VIRTADDR sDevVAddrRequestEnd;
	IMG_UINT32 ui32PDRequestStart;
	IMG_UINT32 ui32PDRequestEnd;
	IMG_UINT32 ui32ModifiedCachelines[BRN31620_CACHE_FLUSH_INDEX_SIZE];
#endif

	
#if SGX_FEATURE_ADDRESS_SPACE_SIZE < 32
	PVR_ASSERT(DevVAddr.uiAddr < (1<<SGX_FEATURE_ADDRESS_SPACE_SIZE));
#endif

	
	SysAcquireData(&psSysData);

	
	ui32PDIndex = DevVAddr.uiAddr >> pMMUHeap->ui32PDShift;

	
	
	if((UINT32_MAX_VALUE - DevVAddr.uiAddr)
		< (ui32Size + pMMUHeap->ui32DataPageMask + pMMUHeap->ui32PTMask))
	{
		
		sHighDevVAddr.uiAddr = UINT32_MAX_VALUE;
	}
	else
	{
		sHighDevVAddr.uiAddr = DevVAddr.uiAddr
								+ ui32Size
								+ pMMUHeap->ui32DataPageMask
								+ pMMUHeap->ui32PTMask;
	}

	ui32PageTableCount = sHighDevVAddr.uiAddr >> pMMUHeap->ui32PDShift;

	
	if (ui32PageTableCount == 0)
		ui32PageTableCount = 1024;

#if defined(FIX_HW_BRN_31620)
	for (i=0;i<BRN31620_CACHE_FLUSH_INDEX_SIZE;i++)
	{
		ui32ModifiedCachelines[i] = 0;
	}

	
	
	
	sDevVAddrRequestStart = DevVAddr;
	ui32PDRequestStart = ui32PDIndex;
	sDevVAddrRequestEnd = sHighDevVAddr;
	ui32PDRequestEnd = ui32PageTableCount - 1;

	
	DevVAddr.uiAddr = DevVAddr.uiAddr & (~BRN31620_PDE_CACHE_FILL_MASK);

	
	sHighDevVAddr.uiAddr = ((sHighDevVAddr.uiAddr + (BRN31620_PDE_CACHE_FILL_SIZE - 1)) & (~BRN31620_PDE_CACHE_FILL_MASK));

	ui32PDIndex = DevVAddr.uiAddr >> pMMUHeap->ui32PDShift;
	ui32PageTableCount = sHighDevVAddr.uiAddr >> pMMUHeap->ui32PDShift;

	
	if (ui32PageTableCount == 0)
		ui32PageTableCount = 1024;
#endif

	ui32PageTableCount -= ui32PDIndex;

	
	pui32PDEntry = (IMG_UINT32*)pMMUHeap->psMMUContext->pvPDCpuVAddr;
	pui32PDEntry += ui32PDIndex;

	
	ppsPTInfoList = &pMMUHeap->psMMUContext->apsPTInfoList[ui32PDIndex];

#if defined(PDUMP)
	{
		IMG_UINT32 ui32Flags = 0;
		
		
		if( MMU_IsHeapShared(pMMUHeap) )
		{
			ui32Flags |= PDUMP_FLAGS_CONTINUOUS;
		}
		PDUMPCOMMENTWITHFLAGS(ui32Flags, "Alloc PTs (MMU Context ID == %u, PDBaseIndex == %u, Size == 0x%x)",
				pMMUHeap->psMMUContext->ui32PDumpMMUContextID,
				pMMUHeap->ui32PDBaseIndex,
				ui32Size);
		PDUMPCOMMENTWITHFLAGS(ui32Flags, "Alloc page table (page count == %08X)", ui32PageTableCount);
		PDUMPCOMMENTWITHFLAGS(ui32Flags, "Page directory mods (page count == %08X)", ui32PageTableCount);
	}
#endif
	
	for(i=0; i<ui32PageTableCount; i++)
	{
		if(ppsPTInfoList[i] == IMG_NULL)
		{
#if defined(FIX_HW_BRN_31620)
			
			if (pMMUHeap->psMMUContext->apsPTInfoListSave[ui32PDIndex + i])
			{
				
				if (((ui32PDIndex + i) >= ui32PDRequestStart) && ((ui32PDIndex + i) <= ui32PDRequestEnd))
				{
					IMG_UINT32 ui32PDCacheLine = (ui32PDIndex + i) >> BRN31620_PDES_PER_CACHE_LINE_SHIFT;

					ppsPTInfoList[i] = pMMUHeap->psMMUContext->apsPTInfoListSave[ui32PDIndex + i];
					pMMUHeap->psMMUContext->apsPTInfoListSave[ui32PDIndex + i] = IMG_NULL;

					pMMUHeap->psMMUContext->ui32PDCacheRangeRefCount[ui32PDCacheLine]++;
				}
			}
			else
			{
#endif
			OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						 sizeof (MMU_PT_INFO),
						 (IMG_VOID **)&ppsPTInfoList[i], IMG_NULL,
						 "MMU Page Table Info");
			if (ppsPTInfoList[i] == IMG_NULL)
			{
				PVR_DPF((PVR_DBG_ERROR, "_DeferredAllocPagetables: ERROR call to OSAllocMem failed"));
				return IMG_FALSE;
			}
			OSMemSet (ppsPTInfoList[i], 0, sizeof(MMU_PT_INFO));
#if defined(FIX_HW_BRN_31620)
			}
#endif
		}
#if defined(FIX_HW_BRN_31620)
		
		if (ppsPTInfoList[i])
		{
#endif
		if(ppsPTInfoList[i]->hPTPageOSMemHandle == IMG_NULL
		&& ppsPTInfoList[i]->PTPageCpuVAddr == IMG_NULL)
		{
			IMG_DEV_PHYADDR	sDevPAddr;
#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
			IMG_UINT32 *pui32Tmp;
			IMG_UINT32 j;
#else
#if !defined(FIX_HW_BRN_31620)
			
			PVR_ASSERT(pui32PDEntry[i] == 0);
#endif
#endif
			if(_AllocPageTableMemory (pMMUHeap, ppsPTInfoList[i], &sDevPAddr) != IMG_TRUE)
			{
				PVR_DPF((PVR_DBG_ERROR, "_DeferredAllocPagetables: ERROR call to _AllocPageTableMemory failed"));
				return IMG_FALSE;
			}
#if defined(FIX_HW_BRN_31620)
			bFlushSystemCache = IMG_TRUE;
			
			{
				IMG_UINT32 ui32PD;
				IMG_UINT32 ui32PDCacheLine;
				IMG_UINT32 ui32PDBitMaskIndex;
				IMG_UINT32 ui32PDBitMaskShift;

				ui32PD = ui32PDIndex + i;
				ui32PDCacheLine = ui32PD >> BRN31620_PDES_PER_CACHE_LINE_SHIFT;
				ui32PDBitMaskIndex = ui32PDCacheLine >> BRN31620_CACHE_FLUSH_BITS_SHIFT;
				ui32PDBitMaskShift = ui32PDCacheLine & BRN31620_CACHE_FLUSH_BITS_MASK;
				ui32ModifiedCachelines[ui32PDBitMaskIndex] |= 1 << ui32PDBitMaskShift;

				
				if ((pMMUHeap->ui32PDBaseIndex + pMMUHeap->ui32PageTableCount) < (ui32PD + 1))
				{
					pMMUHeap->ui32PageTableCount = (ui32PD + 1) - pMMUHeap->ui32PDBaseIndex;
				}

				if (((ui32PDIndex + i) >= ui32PDRequestStart) && ((ui32PDIndex + i) <= ui32PDRequestEnd))
				{
					pMMUHeap->psMMUContext->ui32PDCacheRangeRefCount[ui32PDCacheLine]++;
				}
			}
#endif
			switch(pMMUHeap->psDevArena->DevMemHeapType)
			{
				case DEVICE_MEMORY_HEAP_SHARED :
				case DEVICE_MEMORY_HEAP_SHARED_EXPORTED :
				{
					
					MMU_CONTEXT *psMMUContext = (MMU_CONTEXT*)pMMUHeap->psMMUContext->psDevInfo->pvMMUContextList;

					while(psMMUContext)
					{
						MakeKernelPageReadWrite(psMMUContext->pvPDCpuVAddr);
						
						pui32PDEntry = (IMG_UINT32*)psMMUContext->pvPDCpuVAddr;
						pui32PDEntry += ui32PDIndex;

						
						pui32PDEntry[i] = (sDevPAddr.uiAddr>>SGX_MMU_PDE_ADDR_ALIGNSHIFT)
										| pMMUHeap->ui32PDEPageSizeCtrl
										| SGX_MMU_PDE_VALID;
						MakeKernelPageReadOnly(psMMUContext->pvPDCpuVAddr);
						#if defined(PDUMP)
						
						#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
						if(psMMUContext->bPDumpActive)
						#endif
						{
							
							PDUMPPDENTRIES(&pMMUHeap->sMMUAttrib, psMMUContext->hPDOSMemHandle, (IMG_VOID*)&pui32PDEntry[i], sizeof(IMG_UINT32), 0, IMG_FALSE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
						}
						#endif 
						
						psMMUContext = psMMUContext->psNext;
					}
#if defined(FIX_HW_BRN_31620)
					bSharedPT = IMG_TRUE;
#endif
					break;
				}
				case DEVICE_MEMORY_HEAP_PERCONTEXT :
				case DEVICE_MEMORY_HEAP_KERNEL :
				{
					MakeKernelPageReadWrite(pMMUHeap->psMMUContext->pvPDCpuVAddr);
					
					pui32PDEntry[i] = (sDevPAddr.uiAddr>>SGX_MMU_PDE_ADDR_ALIGNSHIFT)
									| pMMUHeap->ui32PDEPageSizeCtrl
									| SGX_MMU_PDE_VALID;
					MakeKernelPageReadOnly(pMMUHeap->psMMUContext->pvPDCpuVAddr);
					
					
					PDUMPPDENTRIES(&pMMUHeap->sMMUAttrib, pMMUHeap->psMMUContext->hPDOSMemHandle, (IMG_VOID*)&pui32PDEntry[i], sizeof(IMG_UINT32), 0, IMG_FALSE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
					break;
				}
				default:
				{
					PVR_DPF((PVR_DBG_ERROR, "_DeferredAllocPagetables: ERROR invalid heap type"));
					return IMG_FALSE;
				}
			}

#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
			



			MMU_InvalidateDirectoryCache(pMMUHeap->psMMUContext->psDevInfo);
#endif
#if defined(FIX_HW_BRN_31620)
			
			if (((ui32PDIndex + i) < ui32PDRequestStart) || ((ui32PDIndex + i) > ui32PDRequestEnd))
			{
					pMMUHeap->psMMUContext->apsPTInfoListSave[ui32PDIndex + i] = ppsPTInfoList[i];
					ppsPTInfoList[i] = IMG_NULL;
			}
#endif
		}
		else
		{
#if !defined(FIX_HW_BRN_31620)
			
			PVR_ASSERT(pui32PDEntry[i] != 0);
#endif
		}
#if defined(FIX_HW_BRN_31620)
		}
#endif
	}

	#if defined(SGX_FEATURE_SYSTEM_CACHE)
	#if defined(FIX_HW_BRN_31620)
	
	if (bFlushSystemCache)
	{
	#endif

	MMU_InvalidateSystemLevelCache(pMMUHeap->psMMUContext->psDevInfo);
	#endif 
	#if defined(FIX_HW_BRN_31620)
	}

	
	sHighDevVAddr.uiAddr = sHighDevVAddr.uiAddr - 1;

	
	if (bFlushSystemCache)
	{
		MMU_CONTEXT *psMMUContext;

		if (bSharedPT)
		{
			MMU_CONTEXT *psMMUContext = (MMU_CONTEXT*)pMMUHeap->psMMUContext->psDevInfo->pvMMUContextList;

			while(psMMUContext)
			{
				for (i=0;i<BRN31620_CACHE_FLUSH_INDEX_SIZE;i++)
				{
					psMMUContext->ui32PDChangeMask[i] |= ui32ModifiedCachelines[i];
				}

				
				psMMUContext = psMMUContext->psNext;
			}
		}
		else
		{
			for (i=0;i<BRN31620_CACHE_FLUSH_INDEX_SIZE;i++)
			{
				pMMUHeap->psMMUContext->ui32PDChangeMask[i] |= ui32ModifiedCachelines[i];
			}
		}

		
		psMMUContext = pMMUHeap->psMMUContext;
		for (i=0;i<BRN31620_CACHE_FLUSH_INDEX_SIZE;i++)
		{
			IMG_UINT32 j;

			for(j=0;j<BRN31620_CACHE_FLUSH_BITS_SIZE;j++)
			{
				if (ui32ModifiedCachelines[i] & (1 << j))
				{
					PVRSRV_SGXDEV_INFO *psDevInfo = psMMUContext->psDevInfo;
					MMU_PT_INFO *psTempPTInfo = IMG_NULL;
					IMG_UINT32 *pui32Tmp;

					ui32PDIndex = (((i * BRN31620_CACHE_FLUSH_BITS_SIZE) + j) * BRN31620_PDES_PER_CACHE_LINE_SIZE) + BRN31620_DUMMY_PDE_INDEX;

					
					if (psMMUContext->apsPTInfoList[ui32PDIndex])
					{
						psTempPTInfo = psMMUContext->apsPTInfoList[ui32PDIndex];
					}
					else
					{
						psTempPTInfo = psMMUContext->apsPTInfoListSave[ui32PDIndex];
					}

					PVR_ASSERT(psTempPTInfo != IMG_NULL);

					MakeKernelPageReadWrite(psTempPTInfo->PTPageCpuVAddr);
					pui32Tmp = (IMG_UINT32 *) psTempPTInfo->PTPageCpuVAddr;
					PVR_ASSERT(pui32Tmp != IMG_NULL);
					pui32Tmp[BRN31620_DUMMY_PTE_INDEX] = (psDevInfo->sBRN31620DummyPageDevPAddr.uiAddr>>SGX_MMU_PTE_ADDR_ALIGNSHIFT)
															| SGX_MMU_PTE_DUMMY_PAGE
															| SGX_MMU_PTE_READONLY
															| SGX_MMU_PTE_VALID;
					MakeKernelPageReadOnly(psTempPTInfo->PTPageCpuVAddr);
					PDUMPCOMMENT("BRN31620 Dump PTE for dummy page after wireing up new PT");
					PDUMPMEMPTENTRIES(&pMMUHeap->sMMUAttrib, psTempPTInfo->hPTPageOSMemHandle, (IMG_VOID *) &pui32Tmp[BRN31620_DUMMY_PTE_INDEX], sizeof(IMG_UINT32), 0, IMG_FALSE, PDUMP_PT_UNIQUETAG, PDUMP_PT_UNIQUETAG);
				}
			}
		}
	}
	#endif

	return IMG_TRUE;
}


#if defined(PDUMP)
IMG_UINT32 MMU_GetPDumpContextID(IMG_HANDLE hDevMemContext)
{
	BM_CONTEXT *pBMContext = hDevMemContext;
	PVR_ASSERT(pBMContext);
	 
	return pBMContext->psMMUContext->ui32PDumpMMUContextID;
}

static IMG_VOID MMU_SetPDumpAttribs(PDUMP_MMU_ATTRIB *psMMUAttrib,
	PVRSRV_DEVICE_NODE *psDeviceNode,
	IMG_UINT32 ui32DataPageMask,
	IMG_UINT32 ui32PTSize)
{
	
	psMMUAttrib->sDevId = psDeviceNode->sDevId;
	
	psMMUAttrib->pszPDRegRegion = IMG_NULL;
	psMMUAttrib->ui32DataPageMask = ui32DataPageMask;
	
	psMMUAttrib->ui32PTEValid = SGX_MMU_PTE_VALID;
	psMMUAttrib->ui32PTSize = ui32PTSize;
	psMMUAttrib->ui32PTEAlignShift = SGX_MMU_PTE_ADDR_ALIGNSHIFT;
	
	psMMUAttrib->ui32PDEMask = SGX_MMU_PDE_ADDR_MASK;
	psMMUAttrib->ui32PDEAlignShift = SGX_MMU_PDE_ADDR_ALIGNSHIFT;
}
#endif 

PVRSRV_ERROR
MMU_Initialise (PVRSRV_DEVICE_NODE *psDeviceNode, MMU_CONTEXT **ppsMMUContext, IMG_DEV_PHYADDR *psPDDevPAddr)
{
	IMG_UINT32 *pui32Tmp;
	IMG_UINT32 i;
	IMG_CPU_VIRTADDR pvPDCpuVAddr;
	IMG_DEV_PHYADDR sPDDevPAddr;
	IMG_CPU_PHYADDR sCpuPAddr;
	MMU_CONTEXT *psMMUContext;
	IMG_HANDLE hPDOSMemHandle;
	SYS_DATA *psSysData;
	PVRSRV_SGXDEV_INFO *psDevInfo;
#if defined(PDUMP)
	PDUMP_MMU_ATTRIB sMMUAttrib;
#endif
	PVR_DPF ((PVR_DBG_MESSAGE, "MMU_Initialise"));

	SysAcquireData(&psSysData);
#if defined(PDUMP)
	
	
	MMU_SetPDumpAttribs(&sMMUAttrib, psDeviceNode,
						SGX_MMU_PAGE_MASK,
						SGX_MMU_PT_SIZE * sizeof(IMG_UINT32));
#endif

	OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
				 sizeof (MMU_CONTEXT),
				 (IMG_VOID **)&psMMUContext, IMG_NULL,
				 "MMU Context");
	if (psMMUContext == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to OSAllocMem failed"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	OSMemSet (psMMUContext, 0, sizeof(MMU_CONTEXT));

	
	psDevInfo = (PVRSRV_SGXDEV_INFO*)psDeviceNode->pvDevice;
	psMMUContext->psDevInfo = psDevInfo;

	
	psMMUContext->psDeviceNode = psDeviceNode;

	
	if(psDeviceNode->psLocalDevMemArena == IMG_NULL)
	{
		if (OSAllocPages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
						 SGX_MMU_PAGE_SIZE,
						 SGX_MMU_PAGE_SIZE,
						 IMG_NULL,
						 0,
						 &pvPDCpuVAddr,
						 &hPDOSMemHandle) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to OSAllocPages failed"));
			return PVRSRV_ERROR_FAILED_TO_ALLOC_PAGES;
		}

		if(pvPDCpuVAddr)
		{
			sCpuPAddr = OSMapLinToCPUPhys(hPDOSMemHandle,
										  pvPDCpuVAddr);
		}
		else
		{
			
			sCpuPAddr = OSMemHandleToCpuPAddr(hPDOSMemHandle, 0);
		}
		sPDDevPAddr = SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);

		#if PAGE_TEST
		PageTest(pvPDCpuVAddr, sPDDevPAddr);
		#endif

#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
		
		if(!psDevInfo->pvMMUContextList)
		{
			
			if (OSAllocPages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
							 SGX_MMU_PAGE_SIZE,
							 SGX_MMU_PAGE_SIZE,
							 IMG_NULL,
							 0,
							 &psDevInfo->pvDummyPTPageCpuVAddr,
							 &psDevInfo->hDummyPTPageOSMemHandle) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to OSAllocPages failed"));
				return PVRSRV_ERROR_FAILED_TO_ALLOC_PAGES;
			}

			if(psDevInfo->pvDummyPTPageCpuVAddr)
			{
				sCpuPAddr = OSMapLinToCPUPhys(psDevInfo->hDummyPTPageOSMemHandle,
											  psDevInfo->pvDummyPTPageCpuVAddr);
			}
			else
			{
				
				sCpuPAddr = OSMemHandleToCpuPAddr(psDevInfo->hDummyPTPageOSMemHandle, 0);
			}
			psDevInfo->sDummyPTDevPAddr = SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);

			
			if (OSAllocPages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
							 SGX_MMU_PAGE_SIZE,
							 SGX_MMU_PAGE_SIZE,
							 IMG_NULL,
							 0,
							 &psDevInfo->pvDummyDataPageCpuVAddr,
							 &psDevInfo->hDummyDataPageOSMemHandle) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to OSAllocPages failed"));
				return PVRSRV_ERROR_FAILED_TO_ALLOC_PAGES;
			}

			if(psDevInfo->pvDummyDataPageCpuVAddr)
			{
				sCpuPAddr = OSMapLinToCPUPhys(psDevInfo->hDummyPTPageOSMemHandle,
											  psDevInfo->pvDummyDataPageCpuVAddr);
			}
			else
			{
				sCpuPAddr = OSMemHandleToCpuPAddr(psDevInfo->hDummyDataPageOSMemHandle, 0);
			}
			psDevInfo->sDummyDataDevPAddr = SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);
		}
#endif 
#if defined(FIX_HW_BRN_31620)
		
		if(!psDevInfo->pvMMUContextList)
		{
			IMG_UINT32 j;
			
			if (OSAllocPages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
							 SGX_MMU_PAGE_SIZE,
							 SGX_MMU_PAGE_SIZE,
							 IMG_NULL,
							 0,
							 &psDevInfo->pvBRN31620DummyPageCpuVAddr,
							 &psDevInfo->hBRN31620DummyPageOSMemHandle) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to OSAllocPages failed"));
				return PVRSRV_ERROR_FAILED_TO_ALLOC_PAGES;
			}				

			
			if(psDevInfo->pvBRN31620DummyPageCpuVAddr)
			{
				sCpuPAddr = OSMapLinToCPUPhys(psDevInfo->hBRN31620DummyPageOSMemHandle,
											  psDevInfo->pvBRN31620DummyPageCpuVAddr);
			}
			else
			{
				sCpuPAddr = OSMemHandleToCpuPAddr(psDevInfo->hBRN31620DummyPageOSMemHandle, 0);
			}

			pui32Tmp = (IMG_UINT32 *)psDevInfo->pvBRN31620DummyPageCpuVAddr;
			for(j=0; j<(SGX_MMU_PAGE_SIZE/4); j++)
			{
				pui32Tmp[j] = BRN31620_DUMMY_PAGE_SIGNATURE;
			}

			psDevInfo->sBRN31620DummyPageDevPAddr = SysCpuPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);
			PDUMPMALLOCPAGETABLE(&psDeviceNode->sDevId, psDevInfo->hBRN31620DummyPageOSMemHandle, 0, psDevInfo->pvBRN31620DummyPageCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);

			
			if (OSAllocPages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
							 SGX_MMU_PAGE_SIZE,
							 SGX_MMU_PAGE_SIZE,
							 IMG_NULL,
							 0,
							 &psDevInfo->pvBRN31620DummyPTCpuVAddr,
							 &psDevInfo->hBRN31620DummyPTOSMemHandle) != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to OSAllocPages failed"));
				return PVRSRV_ERROR_FAILED_TO_ALLOC_PAGES;
			}				

			
			if(psDevInfo->pvBRN31620DummyPTCpuVAddr)
			{
				sCpuPAddr = OSMapLinToCPUPhys(psDevInfo->hBRN31620DummyPTOSMemHandle,
											  psDevInfo->pvBRN31620DummyPTCpuVAddr);
			}
			else
			{
				sCpuPAddr = OSMemHandleToCpuPAddr(psDevInfo->hBRN31620DummyPTOSMemHandle, 0);
			}

			OSMemSet(psDevInfo->pvBRN31620DummyPTCpuVAddr,0,SGX_MMU_PAGE_SIZE);
			psDevInfo->sBRN31620DummyPTDevPAddr = SysCpuPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);
			PDUMPMALLOCPAGETABLE(&psDeviceNode->sDevId, psDevInfo->hBRN31620DummyPTOSMemHandle, 0, psDevInfo->pvBRN31620DummyPTCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);
		}
#endif
	}
	else
	{
		IMG_SYS_PHYADDR sSysPAddr;

		
		if(RA_Alloc(psDeviceNode->psLocalDevMemArena,
					SGX_MMU_PAGE_SIZE,
					IMG_NULL,
					IMG_NULL,
					0,
					SGX_MMU_PAGE_SIZE,
					0,
					IMG_NULL,
					0,
					&(sSysPAddr.uiAddr))!= IMG_TRUE)
		{
			PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to RA_Alloc failed"));
			return PVRSRV_ERROR_FAILED_TO_ALLOC_VIRT_MEMORY;
		}

		
		sCpuPAddr = SysSysPAddrToCpuPAddr(sSysPAddr);
		sPDDevPAddr = SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sSysPAddr);
		pvPDCpuVAddr = OSMapPhysToLin(sCpuPAddr,
										SGX_MMU_PAGE_SIZE,
										PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
										&hPDOSMemHandle);
		if(!pvPDCpuVAddr)
		{
			PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR failed to map page tables"));
			return PVRSRV_ERROR_FAILED_TO_MAP_PAGE_TABLE;
		}

		#if PAGE_TEST
		PageTest(pvPDCpuVAddr, sPDDevPAddr);
		#endif

#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
		
		if(!psDevInfo->pvMMUContextList)
		{
			
			if(RA_Alloc(psDeviceNode->psLocalDevMemArena,
						SGX_MMU_PAGE_SIZE,
						IMG_NULL,
						IMG_NULL,
						0,
						SGX_MMU_PAGE_SIZE,
						0,
						IMG_NULL,
						0,
						&(sSysPAddr.uiAddr))!= IMG_TRUE)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to RA_Alloc failed"));
				return PVRSRV_ERROR_FAILED_TO_ALLOC_VIRT_MEMORY;
			}

			
			sCpuPAddr = SysSysPAddrToCpuPAddr(sSysPAddr);
			psDevInfo->sDummyPTDevPAddr = SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sSysPAddr);
			psDevInfo->pvDummyPTPageCpuVAddr = OSMapPhysToLin(sCpuPAddr,
																SGX_MMU_PAGE_SIZE,
																PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
																&psDevInfo->hDummyPTPageOSMemHandle);
			if(!psDevInfo->pvDummyPTPageCpuVAddr)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR failed to map page tables"));
				return PVRSRV_ERROR_FAILED_TO_MAP_PAGE_TABLE;
			}

			
			if(RA_Alloc(psDeviceNode->psLocalDevMemArena,
						SGX_MMU_PAGE_SIZE,
						IMG_NULL,
						IMG_NULL,
						0,
						SGX_MMU_PAGE_SIZE,
						0,
						IMG_NULL,
						0,
						&(sSysPAddr.uiAddr))!= IMG_TRUE)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to RA_Alloc failed"));
				return PVRSRV_ERROR_FAILED_TO_ALLOC_VIRT_MEMORY;
			}

			
			sCpuPAddr = SysSysPAddrToCpuPAddr(sSysPAddr);
			psDevInfo->sDummyDataDevPAddr = SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sSysPAddr);
			psDevInfo->pvDummyDataPageCpuVAddr = OSMapPhysToLin(sCpuPAddr,
																SGX_MMU_PAGE_SIZE,
																PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
																&psDevInfo->hDummyDataPageOSMemHandle);
			if(!psDevInfo->pvDummyDataPageCpuVAddr)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR failed to map page tables"));
				return PVRSRV_ERROR_FAILED_TO_MAP_PAGE_TABLE;
			}
		}
#endif 
#if defined(FIX_HW_BRN_31620)
		
		if(!psDevInfo->pvMMUContextList)
		{
			IMG_UINT32 j;
			
			if(RA_Alloc(psDeviceNode->psLocalDevMemArena,
						SGX_MMU_PAGE_SIZE,
						IMG_NULL,
						IMG_NULL,
						0,
						SGX_MMU_PAGE_SIZE,
						0,
						IMG_NULL,
						0,
						&(sSysPAddr.uiAddr))!= IMG_TRUE)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to RA_Alloc failed"));
				return PVRSRV_ERROR_FAILED_TO_ALLOC_VIRT_MEMORY;
			}

			
			sCpuPAddr = SysSysPAddrToCpuPAddr(sSysPAddr);
			psDevInfo->sBRN31620DummyPageDevPAddr = SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sSysPAddr);
			psDevInfo->pvBRN31620DummyPageCpuVAddr = OSMapPhysToLin(sCpuPAddr,
																SGX_MMU_PAGE_SIZE,
																PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
																&psDevInfo->hBRN31620DummyPageOSMemHandle);
			if(!psDevInfo->pvBRN31620DummyPageCpuVAddr)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR failed to map page tables"));
				return PVRSRV_ERROR_FAILED_TO_MAP_PAGE_TABLE;
			}

			MakeKernelPageReadWrite(psDevInfo->pvBRN31620DummyPageCpuVAddr);
			pui32Tmp = (IMG_UINT32 *)psDevInfo->pvBRN31620DummyPageCpuVAddr;
			for(j=0; j<(SGX_MMU_PAGE_SIZE/4); j++)
			{
				pui32Tmp[j] = BRN31620_DUMMY_PAGE_SIGNATURE;
			}
			MakeKernelPageReadOnly(psDevInfo->pvBRN31620DummyPageCpuVAddr);
			PDUMPMALLOCPAGETABLE(&psDeviceNode->sDevId, psDevInfo->hBRN31620DummyPageOSMemHandle, 0, psDevInfo->pvBRN31620DummyPageCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);

			
			if(RA_Alloc(psDeviceNode->psLocalDevMemArena,
						SGX_MMU_PAGE_SIZE,
						IMG_NULL,
						IMG_NULL,
						0,
						SGX_MMU_PAGE_SIZE,
						0,
						IMG_NULL,
						0,
						&(sSysPAddr.uiAddr))!= IMG_TRUE)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to RA_Alloc failed"));
				return PVRSRV_ERROR_FAILED_TO_ALLOC_VIRT_MEMORY;
			}

			
			sCpuPAddr = SysSysPAddrToCpuPAddr(sSysPAddr);
			psDevInfo->sBRN31620DummyPTDevPAddr = SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sSysPAddr);
			psDevInfo->pvBRN31620DummyPTCpuVAddr = OSMapPhysToLin(sCpuPAddr,
																SGX_MMU_PAGE_SIZE,
																PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
																&psDevInfo->hBRN31620DummyPTOSMemHandle);

			if(!psDevInfo->pvBRN31620DummyPTCpuVAddr)
			{
				PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR failed to map page tables"));
				return PVRSRV_ERROR_FAILED_TO_MAP_PAGE_TABLE;
			}

			OSMemSet(psDevInfo->pvBRN31620DummyPTCpuVAddr,0,SGX_MMU_PAGE_SIZE);		
			PDUMPMALLOCPAGETABLE(&psDeviceNode->sDevId, psDevInfo->hBRN31620DummyPTOSMemHandle, 0, psDevInfo->pvBRN31620DummyPTCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);
		}
#endif 
	}

#if defined(FIX_HW_BRN_31620)
	if (!psDevInfo->pvMMUContextList)
	{
		
		psDevInfo->hKernelMMUContext = psMMUContext;
		PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: saving kernel mmu context: %p", psMMUContext));
	}
#endif

#if defined(PDUMP)
#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
	
	{
		PVRSRV_PER_PROCESS_DATA* psPerProc = PVRSRVFindPerProcessData();
		if(psPerProc == IMG_NULL)
		{
			
			psMMUContext->bPDumpActive = IMG_TRUE;
		}
		else
		{
			psMMUContext->bPDumpActive = psPerProc->bPDumpActive;
		}
	}
#endif 
	
#if IMG_ADDRSPACE_PHYSADDR_BITS == 32
	PDUMPCOMMENT("Alloc page directory for new MMU context (PDDevPAddr == 0x%08x)",
			sPDDevPAddr.uiAddr);
#else
	PDUMPCOMMENT("Alloc page directory for new MMU context, 64-bit arch detected (PDDevPAddr == 0x%08x%08x)",
			sPDDevPAddr.uiHighAddr, sPDDevPAddr.uiAddr);
#endif
	PDUMPMALLOCPAGETABLE(&psDeviceNode->sDevId, hPDOSMemHandle, 0, pvPDCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PD_UNIQUETAG);
#endif 

#ifdef SUPPORT_SGX_MMU_BYPASS
	EnableHostAccess(psMMUContext);
#endif

	if (pvPDCpuVAddr)
	{
		pui32Tmp = (IMG_UINT32 *)pvPDCpuVAddr;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: pvPDCpuVAddr invalid"));
		return PVRSRV_ERROR_INVALID_CPU_ADDR;
	}


#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
	MakeKernelPageReadWrite(pvPDCpuVAddr);
	
	for(i=0; i<SGX_MMU_PD_SIZE; i++)
	{
		pui32Tmp[i] = (psDevInfo->sDummyPTDevPAddr.uiAddr>>SGX_MMU_PDE_ADDR_ALIGNSHIFT)
					| SGX_MMU_PDE_PAGE_SIZE_4K
					| SGX_MMU_PDE_VALID;
	}
	MakeKernelPageReadOnly(pvPDCpuVAddr);

	if(!psDevInfo->pvMMUContextList)
	{
		


		MakeKernelPageReadWrite(psDevInfo->pvDummyPTPageCpuVAddr);
		pui32Tmp = (IMG_UINT32 *)psDevInfo->pvDummyPTPageCpuVAddr;
		for(i=0; i<SGX_MMU_PT_SIZE; i++)
		{
			pui32Tmp[i] = (psDevInfo->sDummyDataDevPAddr.uiAddr>>SGX_MMU_PTE_ADDR_ALIGNSHIFT)
						| SGX_MMU_PTE_VALID;
		}
		MakeKernelPageReadOnly(psDevInfo->pvDummyPTPageCpuVAddr);
		
		PDUMPCOMMENT("Dummy Page table contents");
		PDUMPMEMPTENTRIES(&sMMUAttrib, psDevInfo->hDummyPTOSMemHandle, psDevInfo->pvDummyPTPageCpuVAddr, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);

		

		MakeKernelPageReadWrite(psDevInfo->pvDummyDataPageCpuVAddr);
		pui32Tmp = (IMG_UINT32 *)psDevInfo->pvDummyDataPageCpuVAddr;
		for(i=0; i<(SGX_MMU_PAGE_SIZE/4); i++)
		{
			pui32Tmp[i] = DUMMY_DATA_PAGE_SIGNATURE;
		}
		MakeKernelPageReadOnly(psDevInfo->pvDummyDataPageCpuVAddr);
		
		PDUMPCOMMENT("Dummy Data Page contents");
		PDUMPMEMPTENTRIES(PVRSRV_DEVICE_TYPE_SGX, psDevInfo->hDummyDataPageOSMemHandle, psDevInfo->pvDummyDataPageCpuVAddr, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
	}
#else 
	
	MakeKernelPageReadWrite(pvPDCpuVAddr);
	for(i=0; i<SGX_MMU_PD_SIZE; i++)
	{
		
		pui32Tmp[i] = 0;
	}
	MakeKernelPageReadOnly(pvPDCpuVAddr);
#endif 

#if defined(PDUMP)
#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
	if(psMMUContext->bPDumpActive)
#endif 
	{
		
		PDUMPCOMMENT("Page directory contents");
		PDUMPPDENTRIES(&sMMUAttrib, hPDOSMemHandle, pvPDCpuVAddr, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
	}
#endif
#if defined(FIX_HW_BRN_31620)
	{
		IMG_UINT32 i;
		IMG_UINT32 ui32PDCount = 0;
		IMG_UINT32 *pui32PT;
		pui32Tmp = (IMG_UINT32 *)pvPDCpuVAddr;

		PDUMPCOMMENT("BRN31620 Set up dummy PT");

		MakeKernelPageReadWrite(psDevInfo->pvBRN31620DummyPTCpuVAddr);
		pui32PT = (IMG_UINT32 *) psDevInfo->pvBRN31620DummyPTCpuVAddr;
		pui32PT[BRN31620_DUMMY_PTE_INDEX] = (psDevInfo->sBRN31620DummyPageDevPAddr.uiAddr>>SGX_MMU_PTE_ADDR_ALIGNSHIFT)
								| SGX_MMU_PTE_DUMMY_PAGE
								| SGX_MMU_PTE_READONLY
								| SGX_MMU_PTE_VALID;
		MakeKernelPageReadOnly(psDevInfo->pvBRN31620DummyPTCpuVAddr);

#if defined(PDUMP)
		
		PDUMPCOMMENT("BRN31620 Dump dummy PT contents");
		PDUMPMEMPTENTRIES(&sMMUAttrib,  psDevInfo->hBRN31620DummyPTOSMemHandle, psDevInfo->pvBRN31620DummyPTCpuVAddr, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
		PDUMPCOMMENT("BRN31620 Dump dummy page contents");
		PDUMPMEMPTENTRIES(&sMMUAttrib,  psDevInfo->hBRN31620DummyPageOSMemHandle, psDevInfo->pvBRN31620DummyPageCpuVAddr, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);

				
		for(i=0;i<SGX_MMU_PT_SIZE;i++)
		{
			PDUMPMEMPTENTRIES(&sMMUAttrib, psDevInfo->hBRN31620DummyPTOSMemHandle, &pui32PT[i], sizeof(IMG_UINT32), 0, IMG_FALSE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
		}
#endif
		PDUMPCOMMENT("BRN31620 Dump PDE wire up");
		
		for(i=0;i<SGX_MMU_PD_SIZE;i++)
		{
			pui32Tmp[i] = 0;

			if (ui32PDCount == BRN31620_DUMMY_PDE_INDEX)
			{
				MakeKernelPageReadWrite(pvPDCpuVAddr);
				pui32Tmp[i] = (psDevInfo->sBRN31620DummyPTDevPAddr.uiAddr>>SGX_MMU_PDE_ADDR_ALIGNSHIFT)
						| SGX_MMU_PDE_PAGE_SIZE_4K
						| SGX_MMU_PDE_DUMMY_PAGE
						| SGX_MMU_PDE_VALID;
				MakeKernelPageReadOnly(pvPDCpuVAddr);
			}
				PDUMPMEMPTENTRIES(&sMMUAttrib, hPDOSMemHandle, (IMG_VOID *) &pui32Tmp[i], sizeof(IMG_UINT32), 0, IMG_FALSE, PDUMP_PT_UNIQUETAG, PDUMP_PT_UNIQUETAG);
			ui32PDCount++;
			if (ui32PDCount == BRN31620_PDES_PER_CACHE_LINE_SIZE)
			{
				
				ui32PDCount = 0;
			}
		}


		
		PDUMPCOMMENT("BRN31620 dummy Page table contents");
		PDUMPMEMPTENTRIES(&sMMUAttrib, psDevInfo->hBRN31620DummyPageOSMemHandle, psDevInfo->pvBRN31620DummyPageCpuVAddr, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
	}
#endif
#if defined(PDUMP)
	
	{
		PVRSRV_ERROR eError;
		
		IMG_UINT32 ui32MMUType = 1;

		#if defined(SGX_FEATURE_36BIT_MMU)
			ui32MMUType = 3;
		#else
			#if defined(SGX_FEATURE_VARIABLE_MMU_PAGE_SIZE)
				ui32MMUType = 2;
			#endif
		#endif

		eError = PDumpSetMMUContext(PVRSRV_DEVICE_TYPE_SGX,
									psDeviceNode->sDevId.pszPDumpDevName,
									&psMMUContext->ui32PDumpMMUContextID,
									ui32MMUType,
									PDUMP_PT_UNIQUETAG,
									hPDOSMemHandle,
									pvPDCpuVAddr);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "MMU_Initialise: ERROR call to PDumpSetMMUContext failed"));
			return eError;
		}
	}

	
	PDUMPCOMMENT("Set MMU context complete (MMU Context ID == %u)", psMMUContext->ui32PDumpMMUContextID);
#endif

#if defined(FIX_HW_BRN_31620)
	for(i=0;i<BRN31620_CACHE_FLUSH_INDEX_SIZE;i++)
	{
		psMMUContext->ui32PDChangeMask[i] = 0;
	}

	for(i=0;i<BRN31620_CACHE_FLUSH_SIZE;i++)
	{
		psMMUContext->ui32PDCacheRangeRefCount[i] = 0;
	}

	for(i=0;i<SGX_MAX_PD_ENTRIES;i++)
	{
		psMMUContext->apsPTInfoListSave[i] = IMG_NULL;
	}
#endif
	
	psMMUContext->pvPDCpuVAddr = pvPDCpuVAddr;
	psMMUContext->sPDDevPAddr = sPDDevPAddr;
	psMMUContext->hPDOSMemHandle = hPDOSMemHandle;

	
	*ppsMMUContext = psMMUContext;

	
	*psPDDevPAddr = sPDDevPAddr;

	
	psMMUContext->psNext = (MMU_CONTEXT*)psDevInfo->pvMMUContextList;
	psDevInfo->pvMMUContextList = (IMG_VOID*)psMMUContext;

#ifdef SUPPORT_SGX_MMU_BYPASS
	DisableHostAccess(psMMUContext);
#endif

	return PVRSRV_OK;
}

IMG_VOID
MMU_Finalise (MMU_CONTEXT *psMMUContext)
{
	IMG_UINT32 *pui32Tmp, i;
	SYS_DATA *psSysData;
	MMU_CONTEXT **ppsMMUContext;
#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE) || defined(FIX_HW_BRN_31620)
	PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO*)psMMUContext->psDevInfo;
	MMU_CONTEXT *psMMUContextList = (MMU_CONTEXT*)psDevInfo->pvMMUContextList;
#endif

	SysAcquireData(&psSysData);

#if defined(PDUMP)
	
	PDUMPCOMMENT("Clear MMU context (MMU Context ID == %u)", psMMUContext->ui32PDumpMMUContextID);
	PDUMPCLEARMMUCONTEXT(PVRSRV_DEVICE_TYPE_SGX, psMMUContext->psDeviceNode->sDevId.pszPDumpDevName, psMMUContext->ui32PDumpMMUContextID, 2);

	
#if IMG_ADDRSPACE_PHYSADDR_BITS == 32
	PDUMPCOMMENT("Free page directory (PDDevPAddr == 0x%08x)",
			psMMUContext->sPDDevPAddr.uiAddr);
#else
	PDUMPCOMMENT("Free page directory, 64-bit arch detected (PDDevPAddr == 0x%08x%08x)",
			psMMUContext->sPDDevPAddr.uiHighAddr, psMMUContext->sPDDevPAddr.uiAddr);
#endif
#endif 

	PDUMPFREEPAGETABLE(&psMMUContext->psDeviceNode->sDevId, psMMUContext->hPDOSMemHandle, psMMUContext->pvPDCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);
#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
	PDUMPFREEPAGETABLE(&psMMUContext->psDeviceNode->sDevId, psDevInfo->hDummyPTPageOSMemHandle, psDevInfo->pvDummyPTPageCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);
	PDUMPFREEPAGETABLE(&psMMUContext->psDeviceNode->sDevId, psDevInfo->hDummyDataPageOSMemHandle, psDevInfo->pvDummyDataPageCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);
#endif

	pui32Tmp = (IMG_UINT32 *)psMMUContext->pvPDCpuVAddr;

	MakeKernelPageReadWrite(psMMUContext->pvPDCpuVAddr);
	
	for(i=0; i<SGX_MMU_PD_SIZE; i++)
	{
		
		pui32Tmp[i] = 0;
	}
	MakeKernelPageReadOnly(psMMUContext->pvPDCpuVAddr);

	



	if(psMMUContext->psDeviceNode->psLocalDevMemArena == IMG_NULL)
	{
#if defined(FIX_HW_BRN_31620)
		PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO*)psMMUContext->psDevInfo;
#endif
		MakeKernelPageReadWrite(psMMUContext->pvPDCpuVAddr);
		OSFreePages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
						SGX_MMU_PAGE_SIZE,
						psMMUContext->pvPDCpuVAddr,
						psMMUContext->hPDOSMemHandle);

#if defined(FIX_HW_BRN_31620)
		
		if (!psMMUContextList->psNext)
		{
			PDUMPFREEPAGETABLE(&psMMUContext->psDeviceNode->sDevId, psDevInfo->hBRN31620DummyPageOSMemHandle, psDevInfo->pvBRN31620DummyPageCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);
			OSFreePages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
							SGX_MMU_PAGE_SIZE,
							psDevInfo->pvBRN31620DummyPageCpuVAddr,
							psDevInfo->hBRN31620DummyPageOSMemHandle);

			PDUMPFREEPAGETABLE(&psMMUContext->psDeviceNode->sDevId, psDevInfo->hBRN31620DummyPTOSMemHandle, psDevInfo->pvBRN31620DummyPTCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);
			OSFreePages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
							SGX_MMU_PAGE_SIZE,
							psDevInfo->pvBRN31620DummyPTCpuVAddr,
							psDevInfo->hBRN31620DummyPTOSMemHandle);
	
		}
#endif
#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
		
		if(!psMMUContextList->psNext)
		{
			OSFreePages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
							SGX_MMU_PAGE_SIZE,
							psDevInfo->pvDummyPTPageCpuVAddr,
							psDevInfo->hDummyPTPageOSMemHandle);
			OSFreePages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
							SGX_MMU_PAGE_SIZE,
							psDevInfo->pvDummyDataPageCpuVAddr,
							psDevInfo->hDummyDataPageOSMemHandle);
		}
#endif
	}
	else
	{
		IMG_SYS_PHYADDR sSysPAddr;
		IMG_CPU_PHYADDR sCpuPAddr;

		
		sCpuPAddr = OSMapLinToCPUPhys(psMMUContext->hPDOSMemHandle,
									  psMMUContext->pvPDCpuVAddr);
		sSysPAddr = SysCpuPAddrToSysPAddr(sCpuPAddr);

		
		OSUnMapPhysToLin(psMMUContext->pvPDCpuVAddr,
							SGX_MMU_PAGE_SIZE,
                            PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
							psMMUContext->hPDOSMemHandle);
		
		RA_Free (psMMUContext->psDeviceNode->psLocalDevMemArena, sSysPAddr.uiAddr, IMG_FALSE);

#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
		
		if(!psMMUContextList->psNext)
		{
			
			sCpuPAddr = OSMapLinToCPUPhys(psDevInfo->hDummyPTPageOSMemHandle,
										  psDevInfo->pvDummyPTPageCpuVAddr);
			sSysPAddr = SysCpuPAddrToSysPAddr(sCpuPAddr);

			
			OSUnMapPhysToLin(psDevInfo->pvDummyPTPageCpuVAddr,
								SGX_MMU_PAGE_SIZE,
                                PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
								psDevInfo->hDummyPTPageOSMemHandle);
			
			RA_Free (psMMUContext->psDeviceNode->psLocalDevMemArena, sSysPAddr.uiAddr, IMG_FALSE);

			
			sCpuPAddr = OSMapLinToCPUPhys(psDevInfo->hDummyDataPageOSMemHandle,
										  psDevInfo->pvDummyDataPageCpuVAddr);
			sSysPAddr = SysCpuPAddrToSysPAddr(sCpuPAddr);

			
			OSUnMapPhysToLin(psDevInfo->pvDummyDataPageCpuVAddr,
								SGX_MMU_PAGE_SIZE,
                                PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
								psDevInfo->hDummyDataPageOSMemHandle);
			
			RA_Free (psMMUContext->psDeviceNode->psLocalDevMemArena, sSysPAddr.uiAddr, IMG_FALSE);
		}
#endif
#if defined(FIX_HW_BRN_31620)
		
		if(!psMMUContextList->psNext)
		{
			
			PDUMPFREEPAGETABLE(&psMMUContext->psDeviceNode->sDevId, psDevInfo->hBRN31620DummyPageOSMemHandle, psDevInfo->pvBRN31620DummyPageCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);

			sCpuPAddr = OSMapLinToCPUPhys(psDevInfo->hBRN31620DummyPageOSMemHandle,
										  psDevInfo->pvBRN31620DummyPageCpuVAddr);
			sSysPAddr = SysCpuPAddrToSysPAddr(sCpuPAddr);

			
			OSUnMapPhysToLin(psDevInfo->pvBRN31620DummyPageCpuVAddr,
								SGX_MMU_PAGE_SIZE,
                                PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
								psDevInfo->hBRN31620DummyPageOSMemHandle);
			
			RA_Free (psMMUContext->psDeviceNode->psLocalDevMemArena, sSysPAddr.uiAddr, IMG_FALSE);

			
			PDUMPFREEPAGETABLE(&psMMUContext->psDeviceNode->sDevId, psDevInfo->hBRN31620DummyPTOSMemHandle, psDevInfo->pvBRN31620DummyPTCpuVAddr, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);

			sCpuPAddr = OSMapLinToCPUPhys(psDevInfo->hBRN31620DummyPTOSMemHandle,
										  psDevInfo->pvBRN31620DummyPTCpuVAddr);
			sSysPAddr = SysCpuPAddrToSysPAddr(sCpuPAddr);

			
			OSUnMapPhysToLin(psDevInfo->pvBRN31620DummyPTCpuVAddr,
								SGX_MMU_PAGE_SIZE,
                                PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
								psDevInfo->hBRN31620DummyPTOSMemHandle);
			
			RA_Free (psMMUContext->psDeviceNode->psLocalDevMemArena, sSysPAddr.uiAddr, IMG_FALSE);
		}
#endif
	}

	PVR_DPF ((PVR_DBG_MESSAGE, "MMU_Finalise"));

	
	ppsMMUContext = (MMU_CONTEXT**)&psMMUContext->psDevInfo->pvMMUContextList;
	while(*ppsMMUContext)
	{
		if(*ppsMMUContext == psMMUContext)
		{
			
			*ppsMMUContext = psMMUContext->psNext;
			break;
		}

		
		ppsMMUContext = &((*ppsMMUContext)->psNext);
	}

	
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(MMU_CONTEXT), psMMUContext, IMG_NULL);
	
}


IMG_VOID
MMU_InsertHeap(MMU_CONTEXT *psMMUContext, MMU_HEAP *psMMUHeap)
{
	IMG_UINT32 *pui32PDCpuVAddr = (IMG_UINT32 *) psMMUContext->pvPDCpuVAddr;
	IMG_UINT32 *pui32KernelPDCpuVAddr = (IMG_UINT32 *) psMMUHeap->psMMUContext->pvPDCpuVAddr;
	IMG_UINT32 ui32PDEntry;
#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	IMG_BOOL bInvalidateDirectoryCache = IMG_FALSE;
#endif

	
	pui32PDCpuVAddr += psMMUHeap->psDevArena->BaseDevVAddr.uiAddr >> psMMUHeap->ui32PDShift;
	pui32KernelPDCpuVAddr += psMMUHeap->psDevArena->BaseDevVAddr.uiAddr >> psMMUHeap->ui32PDShift;

	


#if defined(PDUMP)
	PDUMPCOMMENT("Page directory shared heap range copy");
	PDUMPCOMMENT("  (Source heap MMU Context ID == %u, PT count == 0x%x)",
			psMMUHeap->psMMUContext->ui32PDumpMMUContextID,
			psMMUHeap->ui32PageTableCount);
	PDUMPCOMMENT("  (Destination MMU Context ID == %u)", psMMUContext->ui32PDumpMMUContextID);
#endif 
#ifdef SUPPORT_SGX_MMU_BYPASS
	EnableHostAccess(psMMUContext);
#endif

	for (ui32PDEntry = 0; ui32PDEntry < psMMUHeap->ui32PageTableCount; ui32PDEntry++)
	{
#if (!defined(SUPPORT_SGX_MMU_DUMMY_PAGE)) && (!defined(FIX_HW_BRN_31620))
		
		PVR_ASSERT(pui32PDCpuVAddr[ui32PDEntry] == 0);
#endif
		MakeKernelPageReadWrite(psMMUContext->pvPDCpuVAddr);
		
		pui32PDCpuVAddr[ui32PDEntry] = pui32KernelPDCpuVAddr[ui32PDEntry];
		MakeKernelPageReadOnly(psMMUContext->pvPDCpuVAddr);
		if (pui32PDCpuVAddr[ui32PDEntry])
		{
			
		#if defined(PDUMP)
			
		#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
			if(psMMUContext->bPDumpActive)
		#endif 
			{
				PDUMPPDENTRIES(&psMMUHeap->sMMUAttrib, psMMUContext->hPDOSMemHandle, (IMG_VOID *) &pui32PDCpuVAddr[ui32PDEntry], sizeof(IMG_UINT32), 0, IMG_FALSE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
			}
		#endif
#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
			bInvalidateDirectoryCache = IMG_TRUE;
#endif
		}
	}

#ifdef SUPPORT_SGX_MMU_BYPASS
	DisableHostAccess(psMMUContext);
#endif

#if !defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS)
	if (bInvalidateDirectoryCache)
	{
		



		MMU_InvalidateDirectoryCache(psMMUContext->psDevInfo);
	}
#endif
}


static IMG_VOID
MMU_UnmapPagesAndFreePTs (MMU_HEAP *psMMUHeap,
						  IMG_DEV_VIRTADDR sDevVAddr,
						  IMG_UINT32 ui32PageCount,
						  IMG_HANDLE hUniqueTag)
{
	IMG_DEV_VIRTADDR	sTmpDevVAddr;
	IMG_UINT32			i;
	IMG_UINT32			ui32PDIndex;
	IMG_UINT32			ui32PTIndex;
	IMG_UINT32			*pui32Tmp;
	IMG_BOOL			bInvalidateDirectoryCache = IMG_FALSE;

#if !defined (PDUMP)
	PVR_UNREFERENCED_PARAMETER(hUniqueTag);
#endif
	
	sTmpDevVAddr = sDevVAddr;

	for(i=0; i<ui32PageCount; i++)
	{
		MMU_PT_INFO **ppsPTInfoList;

		
		ui32PDIndex = sTmpDevVAddr.uiAddr >> psMMUHeap->ui32PDShift;

		
		ppsPTInfoList = &psMMUHeap->psMMUContext->apsPTInfoList[ui32PDIndex];

		{
			
			ui32PTIndex = (sTmpDevVAddr.uiAddr & psMMUHeap->ui32PTMask) >> psMMUHeap->ui32PTShift;

			
			if (!ppsPTInfoList[0])
			{
				PVR_DPF((PVR_DBG_MESSAGE, "MMU_UnmapPagesAndFreePTs: Invalid PT for alloc at VAddr:0x%08X (VaddrIni:0x%08X AllocPage:%u) PDIdx:%u PTIdx:%u",sTmpDevVAddr.uiAddr, sDevVAddr.uiAddr,i, ui32PDIndex, ui32PTIndex ));

				
				sTmpDevVAddr.uiAddr += psMMUHeap->ui32DataPageSize;

				
				continue;
			}

			
			pui32Tmp = (IMG_UINT32*)ppsPTInfoList[0]->PTPageCpuVAddr;

			
			if (!pui32Tmp)
			{
				continue;
			}

			CheckPT(ppsPTInfoList[0]);

			
			if (pui32Tmp[ui32PTIndex] & SGX_MMU_PTE_VALID)
			{
				ppsPTInfoList[0]->ui32ValidPTECount--;
			}
			else
			{
				PVR_DPF((PVR_DBG_MESSAGE, "MMU_UnmapPagesAndFreePTs: Page is already invalid for alloc at VAddr:0x%08X (VAddrIni:0x%08X AllocPage:%u) PDIdx:%u PTIdx:%u",sTmpDevVAddr.uiAddr, sDevVAddr.uiAddr,i, ui32PDIndex, ui32PTIndex ));
			}

			
			PVR_ASSERT((IMG_INT32)ppsPTInfoList[0]->ui32ValidPTECount >= 0);
			MakeKernelPageReadWrite(ppsPTInfoList[0]->PTPageCpuVAddr);
#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
			
			pui32Tmp[ui32PTIndex] = (psMMUHeap->psMMUContext->psDevInfo->sDummyDataDevPAddr.uiAddr>>SGX_MMU_PTE_ADDR_ALIGNSHIFT)
									| SGX_MMU_PTE_VALID;
#else
			
#if defined(FIX_HW_BRN_31620)
			BRN31620InvalidatePageTableEntry(psMMUHeap->psMMUContext, ui32PDIndex, ui32PTIndex, &pui32Tmp[ui32PTIndex]);
#else
			pui32Tmp[ui32PTIndex] = 0;
#endif
#endif
			MakeKernelPageReadOnly(ppsPTInfoList[0]->PTPageCpuVAddr);
			CheckPT(ppsPTInfoList[0]);
		}

		

		if (ppsPTInfoList[0] && (ppsPTInfoList[0]->ui32ValidPTECount == 0)
			)
		{
#if defined(FIX_HW_BRN_31620)
			if (BRN31620FreePageTable(psMMUHeap, ui32PDIndex) == IMG_TRUE)
			{
				bInvalidateDirectoryCache = IMG_TRUE;
			}
#else
			_DeferredFreePageTable(psMMUHeap, ui32PDIndex - psMMUHeap->ui32PDBaseIndex, IMG_TRUE);
			bInvalidateDirectoryCache = IMG_TRUE;
#endif
		}

		
		sTmpDevVAddr.uiAddr += psMMUHeap->ui32DataPageSize;
	}

	if(bInvalidateDirectoryCache)
	{
		MMU_InvalidateDirectoryCache(psMMUHeap->psMMUContext->psDevInfo);
	}
	else
	{
		MMU_InvalidatePageTableCache(psMMUHeap->psMMUContext->psDevInfo);
	}

#if defined(PDUMP)
	MMU_PDumpPageTables(psMMUHeap,
						sDevVAddr,
						psMMUHeap->ui32DataPageSize * ui32PageCount,
						IMG_TRUE,
						hUniqueTag);
#endif 
}


static IMG_VOID MMU_FreePageTables(IMG_PVOID pvMMUHeap,
                                   IMG_SIZE_T ui32Start,
                                   IMG_SIZE_T ui32End,
                                   IMG_HANDLE hUniqueTag)
{
	MMU_HEAP *pMMUHeap = (MMU_HEAP*)pvMMUHeap;
	IMG_DEV_VIRTADDR Start;

	Start.uiAddr = (IMG_UINT32)ui32Start;

	MMU_UnmapPagesAndFreePTs(pMMUHeap, Start, (IMG_UINT32)((ui32End - ui32Start) >> pMMUHeap->ui32PTShift), hUniqueTag);
}

MMU_HEAP *
MMU_Create (MMU_CONTEXT *psMMUContext,
			DEV_ARENA_DESCRIPTOR *psDevArena,
			RA_ARENA **ppsVMArena,
			PDUMP_MMU_ATTRIB **ppsMMUAttrib)
{
	MMU_HEAP *pMMUHeap;
	IMG_UINT32 ui32ScaleSize;

	PVR_UNREFERENCED_PARAMETER(ppsMMUAttrib);

	PVR_ASSERT (psDevArena != IMG_NULL);

	if (psDevArena == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "MMU_Create: invalid parameter"));
		return IMG_NULL;
	}

	OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
				 sizeof (MMU_HEAP),
				 (IMG_VOID **)&pMMUHeap, IMG_NULL,
				 "MMU Heap");
	if (pMMUHeap == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "MMU_Create: ERROR call to OSAllocMem failed"));
		return IMG_NULL;
	}

	pMMUHeap->psMMUContext = psMMUContext;
	pMMUHeap->psDevArena = psDevArena;

	


	switch(pMMUHeap->psDevArena->ui32DataPageSize)
	{
		case 0x1000:
			ui32ScaleSize = 0;
			pMMUHeap->ui32PDEPageSizeCtrl = SGX_MMU_PDE_PAGE_SIZE_4K;
			break;
#if defined(SGX_FEATURE_VARIABLE_MMU_PAGE_SIZE)
		case 0x4000:
			ui32ScaleSize = 2;
			pMMUHeap->ui32PDEPageSizeCtrl = SGX_MMU_PDE_PAGE_SIZE_16K;
			break;
		case 0x10000:
			ui32ScaleSize = 4;
			pMMUHeap->ui32PDEPageSizeCtrl = SGX_MMU_PDE_PAGE_SIZE_64K;
			break;
		case 0x40000:
			ui32ScaleSize = 6;
			pMMUHeap->ui32PDEPageSizeCtrl = SGX_MMU_PDE_PAGE_SIZE_256K;
			break;
		case 0x100000:
			ui32ScaleSize = 8;
			pMMUHeap->ui32PDEPageSizeCtrl = SGX_MMU_PDE_PAGE_SIZE_1M;
			break;
		case 0x400000:
			ui32ScaleSize = 10;
			pMMUHeap->ui32PDEPageSizeCtrl = SGX_MMU_PDE_PAGE_SIZE_4M;
			break;
#endif 
		default:
			PVR_DPF((PVR_DBG_ERROR, "MMU_Create: invalid data page size"));
			goto ErrorFreeHeap;
	}

	
	pMMUHeap->ui32DataPageSize = psDevArena->ui32DataPageSize;
	pMMUHeap->ui32DataPageBitWidth = SGX_MMU_PAGE_SHIFT + ui32ScaleSize;
	pMMUHeap->ui32DataPageMask = pMMUHeap->ui32DataPageSize - 1;
	
	pMMUHeap->ui32PTShift = pMMUHeap->ui32DataPageBitWidth;
	pMMUHeap->ui32PTBitWidth = SGX_MMU_PT_SHIFT - ui32ScaleSize;
	pMMUHeap->ui32PTMask = SGX_MMU_PT_MASK & (SGX_MMU_PT_MASK<<ui32ScaleSize);
	pMMUHeap->ui32PTSize = (IMG_UINT32)(1UL<<pMMUHeap->ui32PTBitWidth) * sizeof(IMG_UINT32);

	
	if(pMMUHeap->ui32PTSize < 4 * sizeof(IMG_UINT32))
	{
		pMMUHeap->ui32PTSize = 4 * sizeof(IMG_UINT32);
	}
	pMMUHeap->ui32PTNumEntriesAllocated = pMMUHeap->ui32PTSize >> 2;

	
	pMMUHeap->ui32PTNumEntriesUsable = (IMG_UINT32)(1UL << pMMUHeap->ui32PTBitWidth);

	
	pMMUHeap->ui32PDShift = pMMUHeap->ui32PTBitWidth + pMMUHeap->ui32PTShift;
	pMMUHeap->ui32PDBitWidth = SGX_FEATURE_ADDRESS_SPACE_SIZE - pMMUHeap->ui32PTBitWidth - pMMUHeap->ui32DataPageBitWidth;
	pMMUHeap->ui32PDMask = SGX_MMU_PD_MASK & (SGX_MMU_PD_MASK>>(32-SGX_FEATURE_ADDRESS_SPACE_SIZE));

	
#if !defined (SUPPORT_EXTERNAL_SYSTEM_CACHE)
	



	if(psDevArena->BaseDevVAddr.uiAddr > (pMMUHeap->ui32DataPageMask | pMMUHeap->ui32PTMask))
	{
		


		PVR_ASSERT ((psDevArena->BaseDevVAddr.uiAddr
						& (pMMUHeap->ui32DataPageMask
							| pMMUHeap->ui32PTMask)) == 0);
	}
#endif
	
	pMMUHeap->ui32PTETotalUsable = pMMUHeap->psDevArena->ui32Size >> pMMUHeap->ui32PTShift;

	
	pMMUHeap->ui32PDBaseIndex = (pMMUHeap->psDevArena->BaseDevVAddr.uiAddr & pMMUHeap->ui32PDMask) >> pMMUHeap->ui32PDShift;

	


	pMMUHeap->ui32PageTableCount = (pMMUHeap->ui32PTETotalUsable + pMMUHeap->ui32PTNumEntriesUsable - 1)
										>> pMMUHeap->ui32PTBitWidth;
	PVR_ASSERT(pMMUHeap->ui32PageTableCount > 0);

	
	pMMUHeap->psVMArena = RA_Create(psDevArena->pszName,
									psDevArena->BaseDevVAddr.uiAddr,
									psDevArena->ui32Size,
									IMG_NULL,
									MAX(HOST_PAGESIZE(), pMMUHeap->ui32DataPageSize),
									IMG_NULL,
									IMG_NULL,
									&MMU_FreePageTables,
									pMMUHeap);

	if (pMMUHeap->psVMArena == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "MMU_Create: ERROR call to RA_Create failed"));
		goto ErrorFreePagetables;
	}

#if defined(PDUMP)
	
	MMU_SetPDumpAttribs(&pMMUHeap->sMMUAttrib,
						psMMUContext->psDeviceNode,
						pMMUHeap->ui32DataPageMask,
						pMMUHeap->ui32PTSize);
	*ppsMMUAttrib = &pMMUHeap->sMMUAttrib;

	PDUMPCOMMENT("Create MMU device from arena %s (Size == 0x%x, DataPageSize == 0x%x, BaseDevVAddr == 0x%x)",
			psDevArena->pszName,
			psDevArena->ui32Size,
			pMMUHeap->ui32DataPageSize,
			psDevArena->BaseDevVAddr.uiAddr);
#endif 

#if 0 
	
	if(psDevArena->ui32HeapID == SGX_TILED_HEAP_ID)
	{
		IMG_UINT32 ui32RegVal;
		IMG_UINT32 ui32XTileStride;

		




		ui32XTileStride	= 2;

		ui32RegVal = (EUR_CR_BIF_TILE0_MIN_ADDRESS_MASK
						& ((psDevArena->BaseDevVAddr.uiAddr>>20)
						<< EUR_CR_BIF_TILE0_MIN_ADDRESS_SHIFT))
					|(EUR_CR_BIF_TILE0_MAX_ADDRESS_MASK
						& (((psDevArena->BaseDevVAddr.uiAddr+psDevArena->ui32Size)>>20)
						<< EUR_CR_BIF_TILE0_MAX_ADDRESS_SHIFT))
					|(EUR_CR_BIF_TILE0_CFG_MASK
						& (((ui32XTileStride<<1)|8) << EUR_CR_BIF_TILE0_CFG_SHIFT));
		PDUMPREG(SGX_PDUMPREG_NAME, EUR_CR_BIF_TILE0, ui32RegVal);
	}
#endif

	

	*ppsVMArena = pMMUHeap->psVMArena;

	return pMMUHeap;

	
ErrorFreePagetables:
	_DeferredFreePageTables (pMMUHeap);

ErrorFreeHeap:
	OSFreeMem (PVRSRV_OS_PAGEABLE_HEAP, sizeof(MMU_HEAP), pMMUHeap, IMG_NULL);
	

	return IMG_NULL;
}

IMG_VOID
MMU_Delete (MMU_HEAP *pMMUHeap)
{
	if (pMMUHeap != IMG_NULL)
	{
		PVR_DPF ((PVR_DBG_MESSAGE, "MMU_Delete"));

		if(pMMUHeap->psVMArena)
		{
			RA_Delete (pMMUHeap->psVMArena);
		}

#if defined(PDUMP)
		PDUMPCOMMENT("Delete MMU device from arena %s (BaseDevVAddr == 0x%x, PT count for deferred free == 0x%x)",
				pMMUHeap->psDevArena->pszName,
				pMMUHeap->psDevArena->BaseDevVAddr.uiAddr,
				pMMUHeap->ui32PageTableCount);
#endif 

#ifdef SUPPORT_SGX_MMU_BYPASS
		EnableHostAccess(pMMUHeap->psMMUContext);
#endif
		_DeferredFreePageTables (pMMUHeap);
#ifdef SUPPORT_SGX_MMU_BYPASS
		DisableHostAccess(pMMUHeap->psMMUContext);
#endif

		OSFreeMem (PVRSRV_OS_PAGEABLE_HEAP, sizeof(MMU_HEAP), pMMUHeap, IMG_NULL);
		
	}
}

IMG_BOOL
MMU_Alloc (MMU_HEAP *pMMUHeap,
		   IMG_SIZE_T uSize,
		   IMG_SIZE_T *pActualSize,
		   IMG_UINT32 uFlags,
		   IMG_UINT32 uDevVAddrAlignment,
		   IMG_DEV_VIRTADDR *psDevVAddr)
{
	IMG_BOOL bStatus;

	PVR_DPF ((PVR_DBG_MESSAGE,
		"MMU_Alloc: uSize=0x%x, flags=0x%x, align=0x%x",
		uSize, uFlags, uDevVAddrAlignment));

	

	if((uFlags & PVRSRV_MEM_USER_SUPPLIED_DEVVADDR) == 0)
	{
		IMG_UINTPTR_T uiAddr;

		bStatus = RA_Alloc (pMMUHeap->psVMArena,
							uSize,
							pActualSize,
							IMG_NULL,
							0,
							uDevVAddrAlignment,
							0,
							IMG_NULL,
							0,
							&uiAddr);
		if(!bStatus)
		{
			PVR_DPF((PVR_DBG_ERROR,"MMU_Alloc: RA_Alloc of VMArena failed"));
			PVR_DPF((PVR_DBG_ERROR,"MMU_Alloc: Alloc of DevVAddr failed from heap %s ID%d",
									pMMUHeap->psDevArena->pszName,
									pMMUHeap->psDevArena->ui32HeapID));
			return bStatus;
		}

		psDevVAddr->uiAddr = IMG_CAST_TO_DEVVADDR_UINT(uiAddr);
	}

	#ifdef SUPPORT_SGX_MMU_BYPASS
	EnableHostAccess(pMMUHeap->psMMUContext);
	#endif

	
	bStatus = _DeferredAllocPagetables(pMMUHeap, *psDevVAddr, (IMG_UINT32)uSize);

	#ifdef SUPPORT_SGX_MMU_BYPASS
	DisableHostAccess(pMMUHeap->psMMUContext);
	#endif

	if (!bStatus)
	{
		PVR_DPF((PVR_DBG_ERROR,"MMU_Alloc: _DeferredAllocPagetables failed"));
		PVR_DPF((PVR_DBG_ERROR,"MMU_Alloc: Failed to alloc pagetable(s) for DevVAddr 0x%8.8x from heap %s ID%d",
								psDevVAddr->uiAddr,
								pMMUHeap->psDevArena->pszName,
								pMMUHeap->psDevArena->ui32HeapID));
		if((uFlags & PVRSRV_MEM_USER_SUPPLIED_DEVVADDR) == 0)
		{
			
			RA_Free (pMMUHeap->psVMArena, psDevVAddr->uiAddr, IMG_FALSE);
		}
	}

	return bStatus;
}

IMG_VOID
MMU_Free (MMU_HEAP *pMMUHeap, IMG_DEV_VIRTADDR DevVAddr, IMG_UINT32 ui32Size)
{
	PVR_ASSERT (pMMUHeap != IMG_NULL);

	if (pMMUHeap == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "MMU_Free: invalid parameter"));
		return;
	}

	PVR_DPF((PVR_DBG_MESSAGE, "MMU_Free: Freeing DevVAddr 0x%08X from heap %s ID%d",
								DevVAddr.uiAddr,
								pMMUHeap->psDevArena->pszName,
								pMMUHeap->psDevArena->ui32HeapID));

	if((DevVAddr.uiAddr >= pMMUHeap->psDevArena->BaseDevVAddr.uiAddr) &&
		(DevVAddr.uiAddr + ui32Size <= pMMUHeap->psDevArena->BaseDevVAddr.uiAddr + pMMUHeap->psDevArena->ui32Size))
	{
		RA_Free (pMMUHeap->psVMArena, DevVAddr.uiAddr, IMG_TRUE);
		return;
	}

	PVR_DPF((PVR_DBG_ERROR,"MMU_Free: Couldn't free DevVAddr %08X from heap %s ID%d (not in range of heap))",
							DevVAddr.uiAddr,
							pMMUHeap->psDevArena->pszName,
							pMMUHeap->psDevArena->ui32HeapID));
}

IMG_VOID
MMU_Enable (MMU_HEAP *pMMUHeap)
{
	PVR_UNREFERENCED_PARAMETER(pMMUHeap);
	
}

IMG_VOID
MMU_Disable (MMU_HEAP *pMMUHeap)
{
	PVR_UNREFERENCED_PARAMETER(pMMUHeap);
	
}

#if defined(FIX_HW_BRN_31620)
IMG_VOID MMU_GetCacheFlushRange(MMU_CONTEXT *pMMUContext, IMG_UINT32 *pui32RangeMask)
{
	IMG_UINT32 i;

	for (i=0;i<BRN31620_CACHE_FLUSH_INDEX_SIZE;i++)
	{
		pui32RangeMask[i] = pMMUContext->ui32PDChangeMask[i];

		
		pMMUContext->ui32PDChangeMask[i] = 0;
	}
}

IMG_VOID MMU_GetPDPhysAddr(MMU_CONTEXT *pMMUContext, IMG_DEV_PHYADDR *psDevPAddr)
{
	*psDevPAddr = pMMUContext->sPDDevPAddr;
}

#endif
#if defined(PDUMP)
static IMG_VOID
MMU_PDumpPageTables	(MMU_HEAP *pMMUHeap,
					 IMG_DEV_VIRTADDR DevVAddr,
					 IMG_SIZE_T uSize,
					 IMG_BOOL bForUnmap,
					 IMG_HANDLE hUniqueTag)
{
	IMG_UINT32	ui32NumPTEntries;
	IMG_UINT32	ui32PTIndex;
	IMG_UINT32	*pui32PTEntry;

	MMU_PT_INFO **ppsPTInfoList;
	IMG_UINT32 ui32PDIndex;
	IMG_UINT32 ui32PTDumpCount;

	
	ui32NumPTEntries = (IMG_UINT32)((uSize + pMMUHeap->ui32DataPageMask) >> pMMUHeap->ui32PTShift);

	
	ui32PDIndex = DevVAddr.uiAddr >> pMMUHeap->ui32PDShift;

	
	ppsPTInfoList = &pMMUHeap->psMMUContext->apsPTInfoList[ui32PDIndex];

	
	ui32PTIndex = (DevVAddr.uiAddr & pMMUHeap->ui32PTMask) >> pMMUHeap->ui32PTShift;

	
	PDUMPCOMMENT("Page table mods (num entries == %08X) %s", ui32NumPTEntries, bForUnmap ? "(for unmap)" : "");

	
	while(ui32NumPTEntries > 0)
	{
		MMU_PT_INFO* psPTInfo = *ppsPTInfoList++;

		if(ui32NumPTEntries <= pMMUHeap->ui32PTNumEntriesUsable - ui32PTIndex)
		{
			ui32PTDumpCount = ui32NumPTEntries;
		}
		else
		{
			ui32PTDumpCount = pMMUHeap->ui32PTNumEntriesUsable - ui32PTIndex;
		}

		if (psPTInfo)
		{
			IMG_UINT32 ui32Flags = 0;
#if defined(SUPPORT_PDUMP_MULTI_PROCESS)
			ui32Flags |= ( MMU_IsHeapShared(pMMUHeap) ) ? PDUMP_FLAGS_PERSISTENT : 0;
#endif
			pui32PTEntry = (IMG_UINT32*)psPTInfo->PTPageCpuVAddr;
			PDUMPMEMPTENTRIES(&pMMUHeap->sMMUAttrib, psPTInfo->hPTPageOSMemHandle, (IMG_VOID *) &pui32PTEntry[ui32PTIndex], ui32PTDumpCount * sizeof(IMG_UINT32), ui32Flags, IMG_FALSE, PDUMP_PT_UNIQUETAG, hUniqueTag);
		}

		
		ui32NumPTEntries -= ui32PTDumpCount;

		
		ui32PTIndex = 0;
	}

	PDUMPCOMMENT("Finished page table mods %s", bForUnmap ? "(for unmap)" : "");
}
#endif 


static IMG_VOID
MMU_MapPage (MMU_HEAP *pMMUHeap,
			 IMG_DEV_VIRTADDR DevVAddr,
			 IMG_DEV_PHYADDR DevPAddr,
			 IMG_UINT32 ui32MemFlags)
{
	IMG_UINT32 ui32Index;
	IMG_UINT32 *pui32Tmp;
	IMG_UINT32 ui32MMUFlags = 0;
	MMU_PT_INFO **ppsPTInfoList;

	
	PVR_ASSERT((DevPAddr.uiAddr & pMMUHeap->ui32DataPageMask) == 0);

	

	if(((PVRSRV_MEM_READ|PVRSRV_MEM_WRITE) & ui32MemFlags) == (PVRSRV_MEM_READ|PVRSRV_MEM_WRITE))
	{
		
		ui32MMUFlags = 0;
	}
	else if(PVRSRV_MEM_READ & ui32MemFlags)
	{
		
		ui32MMUFlags |= SGX_MMU_PTE_READONLY;
	}
	else if(PVRSRV_MEM_WRITE & ui32MemFlags)
	{
		
		ui32MMUFlags |= SGX_MMU_PTE_WRITEONLY;
	}

	
	if(PVRSRV_MEM_CACHE_CONSISTENT & ui32MemFlags)
	{
		ui32MMUFlags |= SGX_MMU_PTE_CACHECONSISTENT;
	}

#if !defined(FIX_HW_BRN_25503)
	
	if(PVRSRV_MEM_EDM_PROTECT & ui32MemFlags)
	{
		ui32MMUFlags |= SGX_MMU_PTE_EDMPROTECT;
	}
#endif

	


	
	ui32Index = DevVAddr.uiAddr >> pMMUHeap->ui32PDShift;

	
	ppsPTInfoList = &pMMUHeap->psMMUContext->apsPTInfoList[ui32Index];

	CheckPT(ppsPTInfoList[0]);

	
	ui32Index = (DevVAddr.uiAddr & pMMUHeap->ui32PTMask) >> pMMUHeap->ui32PTShift;

	
	pui32Tmp = (IMG_UINT32*)ppsPTInfoList[0]->PTPageCpuVAddr;

#if !defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
	{
		IMG_UINT32 uTmp = pui32Tmp[ui32Index];
		
		
#if defined(FIX_HW_BRN_31620)
		if ((uTmp & SGX_MMU_PTE_VALID) && ((DevVAddr.uiAddr & BRN31620_PDE_CACHE_FILL_MASK) != BRN31620_DUMMY_PAGE_OFFSET))
#else
 		if ((uTmp & SGX_MMU_PTE_VALID) != 0)
#endif

		{
			PVR_DPF((PVR_DBG_ERROR, "MMU_MapPage: Page is already valid for alloc at VAddr:0x%08X PDIdx:%u PTIdx:%u",
									DevVAddr.uiAddr,
									DevVAddr.uiAddr >> pMMUHeap->ui32PDShift,
									ui32Index ));
			PVR_DPF((PVR_DBG_ERROR, "MMU_MapPage: Page table entry value: 0x%08X", uTmp));
			PVR_DPF((PVR_DBG_ERROR, "MMU_MapPage: Physical page to map: 0x%08X", DevPAddr.uiAddr));
#if PT_DUMP
			DumpPT(ppsPTInfoList[0]);
#endif
		}
#if !defined(FIX_HW_BRN_31620)
		PVR_ASSERT((uTmp & SGX_MMU_PTE_VALID) == 0);
#endif
	}
#endif

	
	ppsPTInfoList[0]->ui32ValidPTECount++;

	MakeKernelPageReadWrite(ppsPTInfoList[0]->PTPageCpuVAddr);
	
	pui32Tmp[ui32Index] = ((DevPAddr.uiAddr>>SGX_MMU_PTE_ADDR_ALIGNSHIFT)
						& ((~pMMUHeap->ui32DataPageMask)>>SGX_MMU_PTE_ADDR_ALIGNSHIFT))
						| SGX_MMU_PTE_VALID
						| ui32MMUFlags;
	MakeKernelPageReadOnly(ppsPTInfoList[0]->PTPageCpuVAddr);
	CheckPT(ppsPTInfoList[0]);
}


IMG_VOID
MMU_MapScatter (MMU_HEAP *pMMUHeap,
				IMG_DEV_VIRTADDR DevVAddr,
				IMG_SYS_PHYADDR *psSysAddr,
				IMG_SIZE_T uSize,
				IMG_UINT32 ui32MemFlags,
				IMG_HANDLE hUniqueTag)
{
#if defined(PDUMP)
	IMG_DEV_VIRTADDR MapBaseDevVAddr;
#endif 
	IMG_UINT32 uCount, i;
	IMG_DEV_PHYADDR DevPAddr;

	PVR_ASSERT (pMMUHeap != IMG_NULL);

#if defined(PDUMP)
	MapBaseDevVAddr = DevVAddr;
#else
	PVR_UNREFERENCED_PARAMETER(hUniqueTag);
#endif 

	for (i=0, uCount=0; uCount<uSize; i++, uCount+=pMMUHeap->ui32DataPageSize)
	{
		IMG_SYS_PHYADDR sSysAddr;

		sSysAddr = psSysAddr[i];


		
		PVR_ASSERT((sSysAddr.uiAddr & pMMUHeap->ui32DataPageMask) == 0);

		DevPAddr = SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sSysAddr);

		MMU_MapPage (pMMUHeap, DevVAddr, DevPAddr, ui32MemFlags);
		DevVAddr.uiAddr += pMMUHeap->ui32DataPageSize;

		PVR_DPF ((PVR_DBG_MESSAGE,
				 "MMU_MapScatter: devVAddr=%08X, SysAddr=%08X, size=0x%x/0x%x",
				  DevVAddr.uiAddr, sSysAddr.uiAddr, uCount, uSize));
	}

#if defined(PDUMP)
	MMU_PDumpPageTables (pMMUHeap, MapBaseDevVAddr, uSize, IMG_FALSE, hUniqueTag);
#endif 
}

IMG_VOID
MMU_MapPages (MMU_HEAP *pMMUHeap,
			  IMG_DEV_VIRTADDR DevVAddr,
			  IMG_SYS_PHYADDR SysPAddr,
			  IMG_SIZE_T uSize,
			  IMG_UINT32 ui32MemFlags,
			  IMG_HANDLE hUniqueTag)
{
	IMG_DEV_PHYADDR DevPAddr;
#if defined(PDUMP)
	IMG_DEV_VIRTADDR MapBaseDevVAddr;
#endif 
	IMG_UINT32 uCount;
	IMG_UINT32 ui32VAdvance;
	IMG_UINT32 ui32PAdvance;

	PVR_ASSERT (pMMUHeap != IMG_NULL);

	PVR_DPF ((PVR_DBG_MESSAGE, "MMU_MapPages: heap:%s, heap_id:%d devVAddr=%08X, SysPAddr=%08X, size=0x%x",
								pMMUHeap->psDevArena->pszName,
								pMMUHeap->psDevArena->ui32HeapID,
								DevVAddr.uiAddr, 
								SysPAddr.uiAddr,
								uSize));

	
	ui32VAdvance = pMMUHeap->ui32DataPageSize;
	ui32PAdvance = pMMUHeap->ui32DataPageSize;

#if defined(PDUMP)
	MapBaseDevVAddr = DevVAddr;
#else
	PVR_UNREFERENCED_PARAMETER(hUniqueTag);
#endif 

	DevPAddr = SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, SysPAddr);

	
	PVR_ASSERT((DevPAddr.uiAddr & pMMUHeap->ui32DataPageMask) == 0);

#if defined(FIX_HW_BRN_23281)
	if(ui32MemFlags & PVRSRV_MEM_INTERLEAVED)
	{
		ui32VAdvance *= 2;
	}
#endif

	


	if(ui32MemFlags & PVRSRV_MEM_DUMMY)
	{
		ui32PAdvance = 0;
	}

	for (uCount=0; uCount<uSize; uCount+=ui32VAdvance)
	{
		MMU_MapPage (pMMUHeap, DevVAddr, DevPAddr, ui32MemFlags);
		DevVAddr.uiAddr += ui32VAdvance;
		DevPAddr.uiAddr += ui32PAdvance;
	}

#if defined(PDUMP)
	MMU_PDumpPageTables (pMMUHeap, MapBaseDevVAddr, uSize, IMG_FALSE, hUniqueTag);
#endif 
}

IMG_VOID
MMU_MapShadow (MMU_HEAP          *pMMUHeap,
			   IMG_DEV_VIRTADDR   MapBaseDevVAddr,
			   IMG_SIZE_T         uByteSize,
			   IMG_CPU_VIRTADDR   CpuVAddr,
			   IMG_HANDLE         hOSMemHandle,
			   IMG_DEV_VIRTADDR  *pDevVAddr,
			   IMG_UINT32         ui32MemFlags,
			   IMG_HANDLE         hUniqueTag)
{
	IMG_UINT32			i;
	IMG_UINT32			uOffset = 0;
	IMG_DEV_VIRTADDR	MapDevVAddr;
	IMG_UINT32			ui32VAdvance;
	IMG_UINT32			ui32PAdvance;

#if !defined (PDUMP)
	PVR_UNREFERENCED_PARAMETER(hUniqueTag);
#endif

	PVR_DPF ((PVR_DBG_MESSAGE,
			"MMU_MapShadow: DevVAddr:%08X, Bytes:0x%x, CPUVAddr:%08X",
			MapBaseDevVAddr.uiAddr,
			uByteSize,
			(IMG_UINTPTR_T)CpuVAddr));

	
	ui32VAdvance = pMMUHeap->ui32DataPageSize;
	ui32PAdvance = pMMUHeap->ui32DataPageSize;

	
	PVR_ASSERT(((IMG_UINTPTR_T)CpuVAddr & (SGX_MMU_PAGE_SIZE - 1)) == 0);
	PVR_ASSERT(((IMG_UINT32)uByteSize & pMMUHeap->ui32DataPageMask) == 0);
	pDevVAddr->uiAddr = MapBaseDevVAddr.uiAddr;

#if defined(FIX_HW_BRN_23281)
	if(ui32MemFlags & PVRSRV_MEM_INTERLEAVED)
	{
		ui32VAdvance *= 2;
	}
#endif

	


	if(ui32MemFlags & PVRSRV_MEM_DUMMY)
	{
		ui32PAdvance = 0;
	}

	
	MapDevVAddr = MapBaseDevVAddr;
	for (i=0; i<uByteSize; i+=ui32VAdvance)
	{
		IMG_CPU_PHYADDR CpuPAddr;
		IMG_DEV_PHYADDR DevPAddr;

		if(CpuVAddr)
		{
			CpuPAddr = OSMapLinToCPUPhys (hOSMemHandle,
										  (IMG_VOID *)((IMG_UINTPTR_T)CpuVAddr + uOffset));
		}
		else
		{
			CpuPAddr = OSMemHandleToCpuPAddr(hOSMemHandle, uOffset);
		}
		DevPAddr = SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE_SGX, CpuPAddr);

		
		PVR_ASSERT((DevPAddr.uiAddr & pMMUHeap->ui32DataPageMask) == 0);

		PVR_DPF ((PVR_DBG_MESSAGE,
				"Offset=0x%x: CpuVAddr=%08X, CpuPAddr=%08X, DevVAddr=%08X, DevPAddr=%08X",
				uOffset,
				(IMG_UINTPTR_T)CpuVAddr + uOffset,
				CpuPAddr.uiAddr,
				MapDevVAddr.uiAddr,
				DevPAddr.uiAddr));

		MMU_MapPage (pMMUHeap, MapDevVAddr, DevPAddr, ui32MemFlags);

		
		MapDevVAddr.uiAddr += ui32VAdvance;
		uOffset += ui32PAdvance;
	}

#if defined(PDUMP)
	MMU_PDumpPageTables (pMMUHeap, MapBaseDevVAddr, uByteSize, IMG_FALSE, hUniqueTag);
#endif 
}


IMG_VOID
MMU_UnmapPages (MMU_HEAP *psMMUHeap,
				IMG_DEV_VIRTADDR sDevVAddr,
				IMG_UINT32 ui32PageCount,
				IMG_HANDLE hUniqueTag)
{
	IMG_UINT32			uPageSize = psMMUHeap->ui32DataPageSize;
	IMG_DEV_VIRTADDR	sTmpDevVAddr;
	IMG_UINT32			i;
	IMG_UINT32			ui32PDIndex;
	IMG_UINT32			ui32PTIndex;
	IMG_UINT32			*pui32Tmp;

#if !defined (PDUMP)
	PVR_UNREFERENCED_PARAMETER(hUniqueTag);
#endif

	
	sTmpDevVAddr = sDevVAddr;

	for(i=0; i<ui32PageCount; i++)
	{
		MMU_PT_INFO **ppsPTInfoList;

		
		ui32PDIndex = sTmpDevVAddr.uiAddr >> psMMUHeap->ui32PDShift;

		
		ppsPTInfoList = &psMMUHeap->psMMUContext->apsPTInfoList[ui32PDIndex];

		
		ui32PTIndex = (sTmpDevVAddr.uiAddr & psMMUHeap->ui32PTMask) >> psMMUHeap->ui32PTShift;

		
		if (!ppsPTInfoList[0])
		{
			PVR_DPF((PVR_DBG_ERROR, "MMU_UnmapPages: ERROR Invalid PT for alloc at VAddr:0x%08X (VaddrIni:0x%08X AllocPage:%u) PDIdx:%u PTIdx:%u",
									sTmpDevVAddr.uiAddr,
									sDevVAddr.uiAddr,
									i,
									ui32PDIndex,
									ui32PTIndex));

			
			sTmpDevVAddr.uiAddr += uPageSize;

			
			continue;
		}

		CheckPT(ppsPTInfoList[0]);

		
		pui32Tmp = (IMG_UINT32*)ppsPTInfoList[0]->PTPageCpuVAddr;

		
		if (pui32Tmp[ui32PTIndex] & SGX_MMU_PTE_VALID)
		{
			ppsPTInfoList[0]->ui32ValidPTECount--;
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "MMU_UnmapPages: Page is already invalid for alloc at VAddr:0x%08X (VAddrIni:0x%08X AllocPage:%u) PDIdx:%u PTIdx:%u",
									sTmpDevVAddr.uiAddr,
									sDevVAddr.uiAddr,
									i,
									ui32PDIndex,
									ui32PTIndex));
			PVR_DPF((PVR_DBG_ERROR, "MMU_UnmapPages: Page table entry value: 0x%08X", pui32Tmp[ui32PTIndex]));
		}

		
		PVR_ASSERT((IMG_INT32)ppsPTInfoList[0]->ui32ValidPTECount >= 0);

		MakeKernelPageReadWrite(ppsPTInfoList[0]->PTPageCpuVAddr);
#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
		
		pui32Tmp[ui32PTIndex] = (psMMUHeap->psMMUContext->psDevInfo->sDummyDataDevPAddr.uiAddr>>SGX_MMU_PTE_ADDR_ALIGNSHIFT)
								| SGX_MMU_PTE_VALID;
#else
		
#if defined(FIX_HW_BRN_31620)
		BRN31620InvalidatePageTableEntry(psMMUHeap->psMMUContext, ui32PDIndex, ui32PTIndex, &pui32Tmp[ui32PTIndex]);
#else
		pui32Tmp[ui32PTIndex] = 0;
#endif
#endif
		MakeKernelPageReadOnly(ppsPTInfoList[0]->PTPageCpuVAddr);

		CheckPT(ppsPTInfoList[0]);

		
		sTmpDevVAddr.uiAddr += uPageSize;
	}

	MMU_InvalidatePageTableCache(psMMUHeap->psMMUContext->psDevInfo);

#if defined(PDUMP)
	MMU_PDumpPageTables (psMMUHeap, sDevVAddr, uPageSize*ui32PageCount, IMG_TRUE, hUniqueTag);
#endif 
}


IMG_DEV_PHYADDR
MMU_GetPhysPageAddr(MMU_HEAP *pMMUHeap, IMG_DEV_VIRTADDR sDevVPageAddr)
{
	IMG_UINT32 *pui32PageTable;
	IMG_UINT32 ui32Index;
	IMG_DEV_PHYADDR sDevPAddr;
	MMU_PT_INFO **ppsPTInfoList;

	
	ui32Index = sDevVPageAddr.uiAddr >> pMMUHeap->ui32PDShift;

	
	ppsPTInfoList = &pMMUHeap->psMMUContext->apsPTInfoList[ui32Index];
	if (!ppsPTInfoList[0])
	{
		PVR_DPF((PVR_DBG_ERROR,"MMU_GetPhysPageAddr: Not mapped in at 0x%08x", sDevVPageAddr.uiAddr));
		sDevPAddr.uiAddr = 0;
		return sDevPAddr;
	}

	
	ui32Index = (sDevVPageAddr.uiAddr & pMMUHeap->ui32PTMask) >> pMMUHeap->ui32PTShift;

	
	pui32PageTable = (IMG_UINT32*)ppsPTInfoList[0]->PTPageCpuVAddr;

	
	sDevPAddr.uiAddr = pui32PageTable[ui32Index];

	
	sDevPAddr.uiAddr &= ~(pMMUHeap->ui32DataPageMask>>SGX_MMU_PTE_ADDR_ALIGNSHIFT);

	
	sDevPAddr.uiAddr <<= SGX_MMU_PTE_ADDR_ALIGNSHIFT;

	return sDevPAddr;
}


IMG_DEV_PHYADDR MMU_GetPDDevPAddr(MMU_CONTEXT *pMMUContext)
{
	return (pMMUContext->sPDDevPAddr);
}


IMG_EXPORT
PVRSRV_ERROR SGXGetPhysPageAddrKM (IMG_HANDLE hDevMemHeap,
								   IMG_DEV_VIRTADDR sDevVAddr,
								   IMG_DEV_PHYADDR *pDevPAddr,
								   IMG_CPU_PHYADDR *pCpuPAddr)
{
	MMU_HEAP *pMMUHeap;
	IMG_DEV_PHYADDR DevPAddr;

	

	pMMUHeap = (MMU_HEAP*)BM_GetMMUHeap(hDevMemHeap);

	DevPAddr = MMU_GetPhysPageAddr(pMMUHeap, sDevVAddr);
	pCpuPAddr->uiAddr = DevPAddr.uiAddr; 
	pDevPAddr->uiAddr = DevPAddr.uiAddr;

	return (pDevPAddr->uiAddr != 0) ? PVRSRV_OK : PVRSRV_ERROR_INVALID_PARAMS;
}


PVRSRV_ERROR SGXGetMMUPDAddrKM(IMG_HANDLE		hDevCookie,
								IMG_HANDLE 		hDevMemContext,
								IMG_DEV_PHYADDR *psPDDevPAddr)
{
	if (!hDevCookie || !hDevMemContext || !psPDDevPAddr)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	
	*psPDDevPAddr = ((BM_CONTEXT*)hDevMemContext)->psMMUContext->sPDDevPAddr;

	return PVRSRV_OK;
}

PVRSRV_ERROR MMU_BIFResetPDAlloc(PVRSRV_SGXDEV_INFO *psDevInfo)
{
	PVRSRV_ERROR eError;
	SYS_DATA *psSysData;
	RA_ARENA *psLocalDevMemArena;
	IMG_HANDLE hOSMemHandle = IMG_NULL;
	IMG_BYTE *pui8MemBlock = IMG_NULL;
	IMG_SYS_PHYADDR sMemBlockSysPAddr;
	IMG_CPU_PHYADDR sMemBlockCpuPAddr;

	SysAcquireData(&psSysData);

	psLocalDevMemArena = psSysData->apsLocalDevMemArena[0];

	
	if(psLocalDevMemArena == IMG_NULL)
	{
		
		eError = OSAllocPages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
						      3 * SGX_MMU_PAGE_SIZE,
						      SGX_MMU_PAGE_SIZE,
							  IMG_NULL,
							  0,
						      (IMG_VOID **)&pui8MemBlock,
						      &hOSMemHandle);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "MMU_BIFResetPDAlloc: ERROR call to OSAllocPages failed"));
			return eError;
		}

		
		if(pui8MemBlock)
		{
			sMemBlockCpuPAddr = OSMapLinToCPUPhys(hOSMemHandle,
												  pui8MemBlock);
		}
		else
		{
			
			sMemBlockCpuPAddr = OSMemHandleToCpuPAddr(hOSMemHandle, 0);
		}
	}
	else
	{
		

		if(RA_Alloc(psLocalDevMemArena,
					3 * SGX_MMU_PAGE_SIZE,
					IMG_NULL,
					IMG_NULL,
					0,
					SGX_MMU_PAGE_SIZE,
					0,
					IMG_NULL,
					0,
					&(sMemBlockSysPAddr.uiAddr)) != IMG_TRUE)
		{
			PVR_DPF((PVR_DBG_ERROR, "MMU_BIFResetPDAlloc: ERROR call to RA_Alloc failed"));
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}

		
		sMemBlockCpuPAddr = SysSysPAddrToCpuPAddr(sMemBlockSysPAddr);
		pui8MemBlock = OSMapPhysToLin(sMemBlockCpuPAddr,
									  SGX_MMU_PAGE_SIZE * 3,
									  PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
									  &hOSMemHandle);
		if(!pui8MemBlock)
		{
			PVR_DPF((PVR_DBG_ERROR, "MMU_BIFResetPDAlloc: ERROR failed to map page tables"));
			return PVRSRV_ERROR_BAD_MAPPING;
		}
	}

	psDevInfo->hBIFResetPDOSMemHandle = hOSMemHandle;
	psDevInfo->sBIFResetPDDevPAddr = SysCpuPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sMemBlockCpuPAddr);
	psDevInfo->sBIFResetPTDevPAddr.uiAddr = psDevInfo->sBIFResetPDDevPAddr.uiAddr + SGX_MMU_PAGE_SIZE;
	psDevInfo->sBIFResetPageDevPAddr.uiAddr = psDevInfo->sBIFResetPTDevPAddr.uiAddr + SGX_MMU_PAGE_SIZE;
	
	
	psDevInfo->pui32BIFResetPD = (IMG_UINT32 *)pui8MemBlock;
	psDevInfo->pui32BIFResetPT = (IMG_UINT32 *)(pui8MemBlock + SGX_MMU_PAGE_SIZE);

	
	OSMemSet(psDevInfo->pui32BIFResetPD, 0, SGX_MMU_PAGE_SIZE);
	OSMemSet(psDevInfo->pui32BIFResetPT, 0, SGX_MMU_PAGE_SIZE);
	
	OSMemSet(pui8MemBlock + (2 * SGX_MMU_PAGE_SIZE), 0xDB, SGX_MMU_PAGE_SIZE);

	return PVRSRV_OK;
}

IMG_VOID MMU_BIFResetPDFree(PVRSRV_SGXDEV_INFO *psDevInfo)
{
	SYS_DATA *psSysData;
	RA_ARENA *psLocalDevMemArena;
	IMG_SYS_PHYADDR sPDSysPAddr;

	SysAcquireData(&psSysData);

	psLocalDevMemArena = psSysData->apsLocalDevMemArena[0];

	
	if(psLocalDevMemArena == IMG_NULL)
	{
		OSFreePages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
					3 * SGX_MMU_PAGE_SIZE,
					psDevInfo->pui32BIFResetPD,
					psDevInfo->hBIFResetPDOSMemHandle);
	}
	else
	{
		OSUnMapPhysToLin(psDevInfo->pui32BIFResetPD,
                         3 * SGX_MMU_PAGE_SIZE,
                         PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
                         psDevInfo->hBIFResetPDOSMemHandle);

		sPDSysPAddr = SysDevPAddrToSysPAddr(PVRSRV_DEVICE_TYPE_SGX, psDevInfo->sBIFResetPDDevPAddr);
		RA_Free(psLocalDevMemArena, sPDSysPAddr.uiAddr, IMG_FALSE);
	}
}

IMG_VOID MMU_CheckFaultAddr(PVRSRV_SGXDEV_INFO *psDevInfo, IMG_UINT32 ui32PDDevPAddr, IMG_UINT32 ui32FaultAddr)
{
	MMU_CONTEXT *psMMUContext = psDevInfo->pvMMUContextList;

	while (psMMUContext && (psMMUContext->sPDDevPAddr.uiAddr != ui32PDDevPAddr))
	{
		psMMUContext = psMMUContext->psNext;
	}

	if (psMMUContext)
	{
		IMG_UINT32 ui32PTIndex;
		IMG_UINT32 ui32PDIndex;

		PVR_LOG(("Found MMU context for page fault 0x%08x", ui32FaultAddr));

		ui32PTIndex = (ui32FaultAddr & SGX_MMU_PT_MASK) >> SGX_MMU_PAGE_SHIFT;
		ui32PDIndex = (ui32FaultAddr & SGX_MMU_PD_MASK) >> (SGX_MMU_PT_SHIFT + SGX_MMU_PAGE_SHIFT);

		if (psMMUContext->apsPTInfoList[ui32PDIndex])
		{
			if (psMMUContext->apsPTInfoList[ui32PDIndex]->PTPageCpuVAddr)
			{
				IMG_UINT32 *pui32Ptr = psMMUContext->apsPTInfoList[ui32PDIndex]->PTPageCpuVAddr;
				IMG_UINT32 ui32PTE = pui32Ptr[ui32PTIndex];

				PVR_LOG(("PDE valid: PTE = 0x%08x (PhysAddr = 0x%08x, %s)",
						  ui32PTE,
						  ui32PTE & SGX_MMU_PTE_ADDR_MASK,
						  ui32PTE & SGX_MMU_PTE_VALID?"valid":"Invalid"));
			}
			else
			{
				PVR_LOG(("Found PT info but no CPU address"));
			}
		}
		else
		{
			PVR_LOG(("No PDE found"));
		}
	}
}

#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030) && defined(SGX_FEATURE_HOST_PORT)
PVRSRV_ERROR WorkaroundBRN22997Alloc(PVRSRV_DEVICE_NODE	*psDeviceNode)
{
	PVRSRV_ERROR eError;
	SYS_DATA *psSysData;
	RA_ARENA *psLocalDevMemArena;
	IMG_HANDLE hPTPageOSMemHandle = IMG_NULL;
	IMG_HANDLE hPDPageOSMemHandle = IMG_NULL;
	IMG_UINT32 *pui32PD = IMG_NULL;
	IMG_UINT32 *pui32PT = IMG_NULL;
	IMG_CPU_PHYADDR sCpuPAddr;
	IMG_DEV_PHYADDR sPTDevPAddr;
	IMG_DEV_PHYADDR sPDDevPAddr;
	PVRSRV_SGXDEV_INFO *psDevInfo;
	IMG_UINT32 ui32PDOffset;
	IMG_UINT32 ui32PTOffset;

	psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;

	SysAcquireData(&psSysData);

	psLocalDevMemArena = psSysData->apsLocalDevMemArena[0];

	
	if(psLocalDevMemArena == IMG_NULL)
	{
		
		eError = OSAllocPages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
							  SGX_MMU_PAGE_SIZE,
							  SGX_MMU_PAGE_SIZE,
							  IMG_NULL,
							  0,
							  (IMG_VOID **)&pui32PT,
							  &hPTPageOSMemHandle);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "WorkaroundBRN22997: ERROR call to OSAllocPages failed"));
			return eError;
		}
		ui32PTOffset = 0;

		eError = OSAllocPages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
							  SGX_MMU_PAGE_SIZE,
							  SGX_MMU_PAGE_SIZE,
							  IMG_NULL,
							  0,
							  (IMG_VOID **)&pui32PD,
							  &hPDPageOSMemHandle);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "WorkaroundBRN22997: ERROR call to OSAllocPages failed"));
			return eError;
		}
		ui32PDOffset = 0;

		
		if(pui32PT)
        {
            sCpuPAddr = OSMapLinToCPUPhys(hPTPageOSMemHandle,
										  pui32PT);
        }
        else
        {
            
            sCpuPAddr = OSMemHandleToCpuPAddr(hPTPageOSMemHandle, 0);
        }
		sPTDevPAddr = SysCpuPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);

		if(pui32PD)
        {
            sCpuPAddr = OSMapLinToCPUPhys(hPDPageOSMemHandle,
										  pui32PD);
        }
        else
        {
            
            sCpuPAddr = OSMemHandleToCpuPAddr(hPDPageOSMemHandle, 0);
        }
		sPDDevPAddr = SysCpuPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);

	}
	else
	{
		

		if(RA_Alloc(psLocalDevMemArena,
					SGX_MMU_PAGE_SIZE * 2,
					IMG_NULL,
					IMG_NULL,
					0,
					SGX_MMU_PAGE_SIZE,
					0,
					IMG_NULL,
					0,
					&(psDevInfo->sBRN22997SysPAddr.uiAddr))!= IMG_TRUE)
		{
			PVR_DPF((PVR_DBG_ERROR, "WorkaroundBRN22997: ERROR call to RA_Alloc failed"));
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}

		
		sCpuPAddr = SysSysPAddrToCpuPAddr(psDevInfo->sBRN22997SysPAddr);
		pui32PT = OSMapPhysToLin(sCpuPAddr,
								SGX_MMU_PAGE_SIZE * 2,
                                PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
								&hPTPageOSMemHandle);
		if(!pui32PT)
		{
			PVR_DPF((PVR_DBG_ERROR, "WorkaroundBRN22997: ERROR failed to map page tables"));
			return PVRSRV_ERROR_BAD_MAPPING;
		}
		ui32PTOffset = 0;

		
		sPTDevPAddr = SysCpuPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, sCpuPAddr);
		
		pui32PD = pui32PT + SGX_MMU_PAGE_SIZE/sizeof(IMG_UINT32);
		ui32PDOffset = SGX_MMU_PAGE_SIZE;
		hPDPageOSMemHandle = hPTPageOSMemHandle;
		sPDDevPAddr.uiAddr = sPTDevPAddr.uiAddr + SGX_MMU_PAGE_SIZE;
	}

	OSMemSet(pui32PD, 0, SGX_MMU_PAGE_SIZE);
	OSMemSet(pui32PT, 0, SGX_MMU_PAGE_SIZE);

	
	PDUMPMALLOCPAGETABLE(&psDeviceNode->sDevId, hPDPageOSMemHandle, ui32PDOffset, pui32PD, SGX_MMU_PAGE_SIZE, 0, PDUMP_PD_UNIQUETAG);
	PDUMPMALLOCPAGETABLE(&psDeviceNode->sDevId, hPTPageOSMemHandle, ui32PTOffset, pui32PT, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);
	PDUMPMEMPTENTRIES(&psDevInfo->sMMUAttrib, hPDPageOSMemHandle, pui32PD, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
	PDUMPMEMPTENTRIES(&psDevInfo->sMMUAttrib, hPTPageOSMemHandle, pui32PT, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PT_UNIQUETAG, PDUMP_PD_UNIQUETAG);

	psDevInfo->hBRN22997PTPageOSMemHandle = hPTPageOSMemHandle;
	psDevInfo->hBRN22997PDPageOSMemHandle = hPDPageOSMemHandle;
	psDevInfo->sBRN22997PTDevPAddr = sPTDevPAddr;
	psDevInfo->sBRN22997PDDevPAddr = sPDDevPAddr;
	psDevInfo->pui32BRN22997PD = pui32PD;
	psDevInfo->pui32BRN22997PT = pui32PT;

	return PVRSRV_OK;
}


IMG_VOID WorkaroundBRN22997ReadHostPort(PVRSRV_SGXDEV_INFO *psDevInfo)
{
	IMG_UINT32 *pui32PD = psDevInfo->pui32BRN22997PD;
	IMG_UINT32 *pui32PT = psDevInfo->pui32BRN22997PT;
	IMG_UINT32 ui32PDIndex;
	IMG_UINT32 ui32PTIndex;
	IMG_DEV_VIRTADDR sDevVAddr;
	volatile IMG_UINT32 *pui32HostPort;
	IMG_UINT32 ui32BIFCtrl;

	
	
	
	pui32HostPort = (volatile IMG_UINT32*)(((IMG_UINT8*)psDevInfo->pvHostPortBaseKM) + SYS_SGX_HOSTPORT_BRN23030_OFFSET);

	
	sDevVAddr.uiAddr = SYS_SGX_HOSTPORT_BASE_DEVVADDR + SYS_SGX_HOSTPORT_BRN23030_OFFSET;

	ui32PDIndex = (sDevVAddr.uiAddr & SGX_MMU_PD_MASK) >> (SGX_MMU_PAGE_SHIFT + SGX_MMU_PT_SHIFT);
	ui32PTIndex = (sDevVAddr.uiAddr & SGX_MMU_PT_MASK) >> SGX_MMU_PAGE_SHIFT;

	
	pui32PD[ui32PDIndex] = (psDevInfo->sBRN22997PTDevPAddr.uiAddr>>SGX_MMU_PDE_ADDR_ALIGNSHIFT)
							| SGX_MMU_PDE_VALID;
	
	pui32PT[ui32PTIndex] = (psDevInfo->sBRN22997PTDevPAddr.uiAddr>>SGX_MMU_PTE_ADDR_ALIGNSHIFT)
							| SGX_MMU_PTE_VALID;

	PDUMPMEMPTENTRIES(&psDevInfo->sMMUAttrib, psDevInfo->hBRN22997PDPageOSMemHandle, pui32PD, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
	PDUMPMEMPTENTRIES(&psDevInfo->sMMUAttrib, psDevInfo->hBRN22997PTPageOSMemHandle, pui32PT, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PT_UNIQUETAG, PDUMP_PD_UNIQUETAG);

	
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_DIR_LIST_BASE0,
				 psDevInfo->sBRN22997PDDevPAddr.uiAddr);
	PDUMPPDREG(&psDevInfo->sMMUAttrib, EUR_CR_BIF_DIR_LIST_BASE0, psDevInfo->sBRN22997PDDevPAddr.uiAddr, PDUMP_PD_UNIQUETAG);

	
	ui32BIFCtrl = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL);
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32BIFCtrl | EUR_CR_BIF_CTRL_INVALDC_MASK);
	PDUMPREG(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, ui32BIFCtrl | EUR_CR_BIF_CTRL_INVALDC_MASK);
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32BIFCtrl);
	PDUMPREG(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, ui32BIFCtrl);

	
	if (pui32HostPort)
	{
		 
		IMG_UINT32 ui32Tmp;
		ui32Tmp = *pui32HostPort;
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR,"Host Port not present for BRN22997 workaround"));
	}

	




	
	PDUMPCOMMENT("RDW :SGXMEM:v4:%08X\r\n", sDevVAddr.uiAddr);
	
    PDUMPCOMMENT("SAB :SGXMEM:v4:%08X 4 0 hostport.bin", sDevVAddr.uiAddr);

	
	pui32PD[ui32PDIndex] = 0;
	pui32PT[ui32PTIndex] = 0;

	
	PDUMPMEMPTENTRIES(&psDevInfo->sMMUAttrib, psDevInfo->hBRN22997PDPageOSMemHandle, pui32PD, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);
	PDUMPMEMPTENTRIES(&psDevInfo->sMMUAttrib, psDevInfo->hBRN22997PTPageOSMemHandle, pui32PT, SGX_MMU_PAGE_SIZE, 0, IMG_TRUE, PDUMP_PT_UNIQUETAG, PDUMP_PD_UNIQUETAG);

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32BIFCtrl | EUR_CR_BIF_CTRL_INVALDC_MASK);
	PDUMPREG(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, ui32BIFCtrl | EUR_CR_BIF_CTRL_INVALDC_MASK);
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL, ui32BIFCtrl);
	PDUMPREG(SGX_PDUMPREG_NAME, EUR_CR_BIF_CTRL, ui32BIFCtrl);
}


IMG_VOID WorkaroundBRN22997Free(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	SYS_DATA *psSysData;
	RA_ARENA *psLocalDevMemArena;
	PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO*)psDeviceNode->pvDevice;


	SysAcquireData(&psSysData);

	psLocalDevMemArena = psSysData->apsLocalDevMemArena[0];

	PDUMPFREEPAGETABLE(&psDeviceNode->sDevId, psDevInfo->hBRN22997PDPageOSMemHandle, psDevInfo->pui32BRN22997PD, SGX_MMU_PAGE_SIZE, 0, PDUMP_PD_UNIQUETAG);
	PDUMPFREEPAGETABLE(&psDeviceNode->sDevId, psDevInfo->hBRN22997PTPageOSMemHandle, psDevInfo->pui32BRN22997PT, SGX_MMU_PAGE_SIZE, 0, PDUMP_PT_UNIQUETAG);

	
	if(psLocalDevMemArena == IMG_NULL)
	{
		if (psDevInfo->pui32BRN22997PD != IMG_NULL)
		{
			OSFreePages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
						  SGX_MMU_PAGE_SIZE,
						  psDevInfo->pui32BRN22997PD,
						  psDevInfo->hBRN22997PDPageOSMemHandle);
		}

		if (psDevInfo->pui32BRN22997PT != IMG_NULL)
		{
			OSFreePages(PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_KERNEL_ONLY,
						  SGX_MMU_PAGE_SIZE,
						  psDevInfo->pui32BRN22997PT,
						  psDevInfo->hBRN22997PTPageOSMemHandle);
		}
	}
	else
	{
		if (psDevInfo->pui32BRN22997PT != IMG_NULL)
		{
			OSUnMapPhysToLin(psDevInfo->pui32BRN22997PT,
				 SGX_MMU_PAGE_SIZE * 2,
				 PVRSRV_HAP_WRITECOMBINE|PVRSRV_HAP_KERNEL_ONLY,
				 psDevInfo->hBRN22997PTPageOSMemHandle);


			RA_Free(psLocalDevMemArena, psDevInfo->sBRN22997SysPAddr.uiAddr, IMG_FALSE);
		}
	}
}
#endif 


#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
PVRSRV_ERROR MMU_MapExtSystemCacheRegs(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	IMG_UINT32 *pui32PT;
	PVRSRV_SGXDEV_INFO *psDevInfo;
	IMG_UINT32 ui32PDIndex;
	IMG_UINT32 ui32PTIndex;
	PDUMP_MMU_ATTRIB sMMUAttrib;

	psDevInfo = (PVRSRV_SGXDEV_INFO*)psDeviceNode->pvDevice;

	sMMUAttrib = psDevInfo->sMMUAttrib;
#if defined(PDUMP)
	MMU_SetPDumpAttribs(&sMMUAttrib, psDeviceNode,
						SGX_MMU_PAGE_MASK,
						SGX_MMU_PT_SIZE * sizeof(IMG_UINT32));
#endif

#if defined(PDUMP)
	{
		IMG_CHAR		szScript[128];

		sprintf(szScript, "MALLOC :EXTSYSCACHE:PA_%08X%08X %u %u 0x%08X\r\n", 0, psDevInfo->sExtSysCacheRegsDevPBase.uiAddr, SGX_MMU_PAGE_SIZE, SGX_MMU_PAGE_SIZE, psDevInfo->sExtSysCacheRegsDevPBase.uiAddr);
		PDumpOSWriteString2(szScript, PDUMP_FLAGS_CONTINUOUS);
	}
#endif

	ui32PDIndex = (SGX_EXT_SYSTEM_CACHE_REGS_DEVVADDR_BASE & SGX_MMU_PD_MASK) >> (SGX_MMU_PAGE_SHIFT + SGX_MMU_PT_SHIFT);
	ui32PTIndex = (SGX_EXT_SYSTEM_CACHE_REGS_DEVVADDR_BASE & SGX_MMU_PT_MASK) >> SGX_MMU_PAGE_SHIFT;

	pui32PT = (IMG_UINT32 *) psDeviceNode->sDevMemoryInfo.pBMKernelContext->psMMUContext->apsPTInfoList[ui32PDIndex]->PTPageCpuVAddr;

	MakeKernelPageReadWrite(pui32PT);
	
	pui32PT[ui32PTIndex] = (psDevInfo->sExtSysCacheRegsDevPBase.uiAddr>>SGX_MMU_PTE_ADDR_ALIGNSHIFT)
							| SGX_MMU_PTE_VALID;
	MakeKernelPageReadOnly(pui32PT);
#if defined(PDUMP)
	
	{
		IMG_DEV_PHYADDR sDevPAddr;
		IMG_CPU_PHYADDR sCpuPAddr;
		IMG_UINT32 ui32PageMask;
		IMG_UINT32 ui32PTE;
		PVRSRV_ERROR eErr;

		PDUMP_GET_SCRIPT_AND_FILE_STRING();

		ui32PageMask = sMMUAttrib.ui32PTSize - 1;
		sCpuPAddr = OSMapLinToCPUPhys(psDeviceNode->sDevMemoryInfo.pBMKernelContext->psMMUContext->apsPTInfoList[ui32PDIndex]->hPTPageOSMemHandle, &pui32PT[ui32PTIndex]);
		sDevPAddr = SysCpuPAddrToDevPAddr(sMMUAttrib.sDevId.eDeviceType, sCpuPAddr);
		ui32PTE = *((IMG_UINT32 *) (&pui32PT[ui32PTIndex]));

		eErr = PDumpOSBufprintf(hScript,
								ui32MaxLenScript,
								"WRW :%s:PA_%08X%08X:0x%08X :%s:PA_%08X%08X:0x%08X\r\n",
								sMMUAttrib.sDevId.pszPDumpDevName,
								(IMG_UINT32)(IMG_UINTPTR_T)PDUMP_PT_UNIQUETAG,
								(sDevPAddr.uiAddr) & ~ui32PageMask,
								(sDevPAddr.uiAddr) & ui32PageMask,
								"EXTSYSCACHE",
								(IMG_UINT32)(IMG_UINTPTR_T)PDUMP_PD_UNIQUETAG,
								(ui32PTE & sMMUAttrib.ui32PDEMask) << sMMUAttrib.ui32PTEAlignShift,
								ui32PTE & ~sMMUAttrib.ui32PDEMask);
					if(eErr != PVRSRV_OK)
					{
						return eErr;
					}
					PDumpOSWriteString2(hScript, PDUMP_FLAGS_CONTINUOUS);
	}
#endif

	return PVRSRV_OK;
}


PVRSRV_ERROR MMU_UnmapExtSystemCacheRegs(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	SYS_DATA *psSysData;
	RA_ARENA *psLocalDevMemArena;
	PVRSRV_SGXDEV_INFO *psDevInfo;
	IMG_UINT32 ui32PDIndex;
	IMG_UINT32 ui32PTIndex;
	IMG_UINT32 *pui32PT;
	PDUMP_MMU_ATTRIB sMMUAttrib;

	psDevInfo = (PVRSRV_SGXDEV_INFO*)psDeviceNode->pvDevice;

	sMMUAttrib = psDevInfo->sMMUAttrib;

#if defined(PDUMP)
	MMU_SetPDumpAttribs(&sMMUAttrib, psDeviceNode,
						SGX_MMU_PAGE_MASK,
						SGX_MMU_PT_SIZE * sizeof(IMG_UINT32));
#endif
	SysAcquireData(&psSysData);

	psLocalDevMemArena = psSysData->apsLocalDevMemArena[0];

	
	ui32PDIndex = (SGX_EXT_SYSTEM_CACHE_REGS_DEVVADDR_BASE & SGX_MMU_PD_MASK) >> (SGX_MMU_PAGE_SHIFT + SGX_MMU_PT_SHIFT);
	ui32PTIndex = (SGX_EXT_SYSTEM_CACHE_REGS_DEVVADDR_BASE & SGX_MMU_PT_MASK) >> SGX_MMU_PAGE_SHIFT;

	
	if (psDeviceNode->sDevMemoryInfo.pBMKernelContext->psMMUContext->apsPTInfoList[ui32PDIndex])
	{
		if (psDeviceNode->sDevMemoryInfo.pBMKernelContext->psMMUContext->apsPTInfoList[ui32PDIndex]->PTPageCpuVAddr)
		{
			pui32PT = (IMG_UINT32 *) psDeviceNode->sDevMemoryInfo.pBMKernelContext->psMMUContext->apsPTInfoList[ui32PDIndex]->PTPageCpuVAddr;
		}
	}

	MakeKernelPageReadWrite(pui32PT);
	pui32PT[ui32PTIndex] = 0;
	MakeKernelPageReadOnly(pui32PT);

	PDUMPMEMPTENTRIES(&sMMUAttrib, psDeviceNode->sDevMemoryInfo.pBMKernelContext->psMMUContext->hPDOSMemHandle, &pui32PT[ui32PTIndex], sizeof(IMG_UINT32), 0, IMG_FALSE, PDUMP_PD_UNIQUETAG, PDUMP_PT_UNIQUETAG);

	return PVRSRV_OK;
}
#endif


#if PAGE_TEST
static IMG_VOID PageTest(IMG_VOID* pMem, IMG_DEV_PHYADDR sDevPAddr)
{
	volatile IMG_UINT32 ui32WriteData;
	volatile IMG_UINT32 ui32ReadData;
	volatile IMG_UINT32 *pMem32 = (volatile IMG_UINT32 *)pMem;
	IMG_INT n;
	IMG_BOOL bOK=IMG_TRUE;

	ui32WriteData = 0xffffffff;

	for (n=0; n<1024; n++)
	{
		pMem32[n] = ui32WriteData;
		ui32ReadData = pMem32[n];

		if (ui32WriteData != ui32ReadData)
		{
			
			PVR_DPF ((PVR_DBG_ERROR, "Error - memory page test failed at device phys address 0x%08X", sDevPAddr.uiAddr + (n<<2) ));
			PVR_DBG_BREAK;
			bOK = IMG_FALSE;
		}
 	}

	ui32WriteData = 0;

	for (n=0; n<1024; n++)
	{
		pMem32[n] = ui32WriteData;
		ui32ReadData = pMem32[n];

		if (ui32WriteData != ui32ReadData)
		{
			
			PVR_DPF ((PVR_DBG_ERROR, "Error - memory page test failed at device phys address 0x%08X", sDevPAddr.uiAddr + (n<<2) ));
			PVR_DBG_BREAK;
			bOK = IMG_FALSE;
		}
 	}

	if (bOK)
	{
		PVR_DPF ((PVR_DBG_VERBOSE, "MMU Page 0x%08X is OK", sDevPAddr.uiAddr));
	}
	else
	{
		PVR_DPF ((PVR_DBG_VERBOSE, "MMU Page 0x%08X *** FAILED ***", sDevPAddr.uiAddr));
	}
}
#endif

