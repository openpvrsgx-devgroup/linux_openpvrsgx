/*************************************************************************/ /*!
@Title          MMU Management
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Implements basic low level control of MMU.
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

#ifndef _MMU_H_
#define _MMU_H_

#include "sgxinfokm.h"

/*
******************************************************************************
	FUNCTION:   MMU_Initialise

	PURPOSE:    Initialise the mmu module.
	                	
	PARAMETERS:	None
	RETURNS:	PVRSRV_ERROR
******************************************************************************/
PVRSRV_ERROR
MMU_Initialise (PVRSRV_DEVICE_NODE *psDeviceNode, MMU_CONTEXT **ppsMMUContext, IMG_DEV_PHYADDR *psPDDevPAddr);

/*
******************************************************************************
	FUNCTION:   MMU_Finalise

	PURPOSE:    Finalise the mmu module, deallocate all resources.
	                	
	PARAMETERS:	None.
	RETURNS:	None.
******************************************************************************/
IMG_VOID
MMU_Finalise (MMU_CONTEXT *psMMUContext);


/*
******************************************************************************
	FUNCTION:   MMU_InsertHeap

	PURPOSE:    Inserts shared heap into the specified context 
				from the kernel context
	                	
	PARAMETERS:	None.
	RETURNS:	None.
******************************************************************************/
IMG_VOID
MMU_InsertHeap(MMU_CONTEXT *psMMUContext, MMU_HEAP *psMMUHeap);

/*
******************************************************************************
    FUNCTION:   MMU_Create

    PURPOSE:    Create an mmu device.

    PARAMETERS: In: psMMUContext -
                In: psDevArena - 
				Out: ppsVMArena
    RETURNS:	MMU_HEAP
******************************************************************************/
MMU_HEAP *
MMU_Create (MMU_CONTEXT *psMMUContext,
			DEV_ARENA_DESCRIPTOR *psDevArena,
			RA_ARENA **ppsVMArena);

/*
******************************************************************************
	FUNCTION:   MMU_Delete

	PURPOSE:    Delete an mmu device.
	                	
	PARAMETERS:	In:  pMMUHeap - The mmu to delete.
	RETURNS:	
******************************************************************************/
IMG_VOID
MMU_Delete (MMU_HEAP *pMMU);

/*
******************************************************************************
    FUNCTION:   MMU_Alloc
    PURPOSE:    Allocate space in an mmu's virtual address space.
    PARAMETERS:	In:  pMMUHeap - MMU to allocate on.
                In:  uSize - Size in bytes to allocate.
                Out: pActualSize - If non null receives actual size allocated. 
                In:  uFlags - Allocation flags.
                In:  uDevVAddrAlignment - Required alignment.
                Out: pDevVAddr - Receives base address of allocation.
    RETURNS:	IMG_TRUE - Success
                IMG_FALSE - Failure
******************************************************************************/
IMG_BOOL
MMU_Alloc (MMU_HEAP *pMMU,
           IMG_SIZE_T uSize,
           IMG_SIZE_T *pActualSize,
           IMG_UINT32 uFlags,
		   IMG_UINT32 uDevVAddrAlignment,
           IMG_DEV_VIRTADDR *pDevVAddr);

/*
******************************************************************************
    FUNCTION:   MMU_Free
    PURPOSE:    Frees space in an mmu's virtual address space.
    PARAMETERS:	In: pMMUHeap - MMU to free on.
                In: DevVAddr - Base address of allocation.
    RETURNS:	IMG_TRUE - Success
                IMG_FALSE - Failure
******************************************************************************/
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

/*
******************************************************************************
	FUNCTION:   MMU_MapScatter

	PURPOSE:    Create a mapping for a list of pages to a specified device 
				virtual address.
	                	
	PARAMETERS:	In:  pMMUHeap - the mmu.
	            In:  DevVAddr - the device virtual address.
	            In:  psSysAddr - the list of physical addresses of the pages to
	                 map.
	RETURNS:	None
******************************************************************************/
IMG_VOID
MMU_MapScatter (MMU_HEAP *pMMU,
				IMG_DEV_VIRTADDR DevVAddr,
				IMG_SYS_PHYADDR *psSysAddr,
				IMG_SIZE_T uSize,
				IMG_UINT32 ui32MemFlags,
				IMG_HANDLE hUniqueTag);


/*
******************************************************************************
    FUNCTION:   MMU_GetPhysPageAddr

    PURPOSE:    extracts physical address from MMU page tables

    PARAMETERS: In:  pMMUHeap - the mmu
	PARAMETERS: In:  sDevVPageAddr - the virtual address to extract physical 
					page mapping from
    RETURNS:    IMG_DEV_PHYADDR
******************************************************************************/
IMG_DEV_PHYADDR
MMU_GetPhysPageAddr(MMU_HEAP *pMMUHeap, IMG_DEV_VIRTADDR sDevVPageAddr);


/*
******************************************************************************
    FUNCTION:   MMU_GetPDDevPAddr

    PURPOSE:    returns PD given the MMU context (SGX to MMU API)

    PARAMETERS: In:  pMMUContext - the mmu
    RETURNS:    IMG_DEV_PHYADDR
******************************************************************************/
IMG_DEV_PHYADDR
MMU_GetPDDevPAddr(MMU_CONTEXT *pMMUContext);


#ifdef SUPPORT_SGX_MMU_BYPASS
/*
******************************************************************************
    FUNCTION:   EnableHostAccess

    PURPOSE:    Enables Host accesses to device memory, by passing the device 
    			MMU address translation

    PARAMETERS: In: psMMUContext
    RETURNS:    None
******************************************************************************/
IMG_VOID
EnableHostAccess (MMU_CONTEXT *psMMUContext);


/*
******************************************************************************
    FUNCTION:   DisableHostAccess

    PURPOSE:    Disables Host accesses to device memory, by passing the device 
    			MMU address translation

    PARAMETERS: In: psMMUContext
    RETURNS:    None
******************************************************************************/
IMG_VOID
DisableHostAccess (MMU_CONTEXT *psMMUContext);
#endif

/*
******************************************************************************
    FUNCTION:   MMU_InvalidateDirectoryCache

    PURPOSE:    Invalidates the page directory cache

    PARAMETERS: In: psDevInfo
    RETURNS:    None
******************************************************************************/
IMG_VOID MMU_InvalidateDirectoryCache(PVRSRV_SGXDEV_INFO *psDevInfo);

/*
******************************************************************************
	FUNCTION:   MMU_BIFResetPDAlloc

	PURPOSE:    Allocate a dummy Page Directory which causes all virtual
				addresses to page fault.

	PARAMETERS: In:  psDevInfo - device info
	RETURNS:    PVRSRV_OK or error
******************************************************************************/
PVRSRV_ERROR MMU_BIFResetPDAlloc(PVRSRV_SGXDEV_INFO *psDevInfo);

/*
******************************************************************************
	FUNCTION:   MMU_BIFResetPDFree

	PURPOSE:    Free resources allocated in MMU_BIFResetPDAlloc.

	PARAMETERS: In:  psDevInfo - device info
	RETURNS:
******************************************************************************/
IMG_VOID MMU_BIFResetPDFree(PVRSRV_SGXDEV_INFO *psDevInfo);

#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030) && defined(SGX_FEATURE_HOST_PORT)
PVRSRV_ERROR WorkaroundBRN22997Alloc(PVRSRV_SGXDEV_INFO *psDevInfo);

IMG_VOID WorkaroundBRN22997ReadHostPort(PVRSRV_SGXDEV_INFO *psDevInfo);

IMG_VOID WorkaroundBRN22997Free(PVRSRV_SGXDEV_INFO *psDevInfo);
#endif

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
PVRSRV_ERROR MMU_MapExtSystemCacheRegs(PVRSRV_DEVICE_NODE *psDeviceNode);

/*
******************************************************************************
	FUNCTION:   MMU_UnmapExtSystemCacheRegs

	PURPOSE:    unmaps external system cache control registers

	PARAMETERS: In:  psDeviceNode - device node
	RETURNS:
******************************************************************************/
PVRSRV_ERROR MMU_UnmapExtSystemCacheRegs(PVRSRV_DEVICE_NODE *psDeviceNode);
#endif

#endif
