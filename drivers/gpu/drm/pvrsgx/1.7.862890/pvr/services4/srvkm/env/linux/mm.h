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

#ifndef __IMG_LINUX_MM_H__
#define __IMG_LINUX_MM_H__

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/list.h>

#include <asm/io.h>

#define	PHYS_TO_PFN(phys) ((phys) >> PAGE_SHIFT)
#define PFN_TO_PHYS(pfn) ((pfn) << PAGE_SHIFT)

#define RANGE_TO_PAGES(range) (((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)

#define	ADDR_TO_PAGE_OFFSET(addr) (((unsigned long)(addr)) & (PAGE_SIZE - 1))

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10))
#define	REMAP_PFN_RANGE(vma, addr, pfn, size, prot) remap_pfn_range(vma, addr, pfn, size, prot)
#else
#define	REMAP_PFN_RANGE(vma, addr, pfn, size, prot) remap_page_range(vma, addr, PFN_TO_PHYS(pfn), size, prot)
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12))
#define	IO_REMAP_PFN_RANGE(vma, addr, pfn, size, prot) io_remap_pfn_range(vma, addr, pfn, size, prot)
#else
#define	IO_REMAP_PFN_RANGE(vma, addr, pfn, size, prot) io_remap_page_range(vma, addr, PFN_TO_PHYS(pfn), size, prot)
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15))
#define	VM_INSERT_PAGE(vma, addr, page) vm_insert_page(vma, addr, page)
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10))
#define VM_INSERT_PAGE(vma, addr, page) remap_pfn_range(vma, addr, page_to_pfn(page), PAGE_SIZE, vma->vm_page_prot);
#else
#define VM_INSERT_PAGE(vma, addr, page) remap_page_range(vma, addr, page_to_phys(page), PAGE_SIZE, vma->vm_page_prot);
#endif
#endif

static inline IMG_UINTPTR_T VMallocToPhys(IMG_VOID *pCpuVAddr)
{
	return (page_to_phys(vmalloc_to_page(pCpuVAddr)) + ADDR_TO_PAGE_OFFSET(pCpuVAddr));
		
}

typedef enum {
    LINUX_MEM_AREA_IOREMAP,
    LINUX_MEM_AREA_EXTERNAL_KV,
    LINUX_MEM_AREA_IO,
    LINUX_MEM_AREA_VMALLOC,
    LINUX_MEM_AREA_ALLOC_PAGES,
    LINUX_MEM_AREA_SUB_ALLOC,
#if defined(PVR_LINUX_MEM_AREA_USE_VMAP)
    LINUX_MEM_AREA_VMAP,
#endif
    LINUX_MEM_AREA_CMA,
    LINUX_MEM_AREA_TYPE_COUNT
}LINUX_MEM_AREA_TYPE;

typedef struct _LinuxMemArea LinuxMemArea;


/* FIXME - describe this structure. */
struct _LinuxMemArea {
    LINUX_MEM_AREA_TYPE eAreaType;
    union _uData
    {
        struct _sIORemap
        {
            /* Note: The memory this represents is _not_ implicitly
             * page aligned, neither is its size */
            IMG_CPU_PHYADDR CPUPhysAddr;
            IMG_VOID *pvIORemapCookie;
        }sIORemap;
        struct _sExternalKV
        {
            /* Note: The memory this represents is _not_ implicitly
             * page aligned, neither is its size */
	    IMG_BOOL bPhysContig;
	    union {
		    /*
		     * SYSPhysAddr is valid if bPhysContig is true, else
		     * pSysPhysAddr is valid
		     */
		    IMG_SYS_PHYADDR SysPhysAddr;
		    IMG_SYS_PHYADDR *pSysPhysAddr;
	    } uPhysAddr;
            IMG_VOID *pvExternalKV;
        }sExternalKV;
        struct _sIO
        {
            /* Note: The memory this represents is _not_ implicitly
             * page aligned, neither is its size */
            IMG_CPU_PHYADDR CPUPhysAddr;
        }sIO;
        struct _sVmalloc
        {
            /* Note the memory this represents _is_ implicitly
             * page aligned _and_ so is its size */
            IMG_VOID *pvVmallocAddress;
#if defined(PVR_LINUX_MEM_AREA_USE_VMAP)
            struct page **ppsPageList;
	    IMG_HANDLE hBlockPageList;
#endif
        }sVmalloc;
        struct _sPageList
        {
            /* Note the memory this represents _is_ implicitly
             * page aligned _and_ so is its size */
            struct page **pvPageList;
	    IMG_HANDLE hBlockPageList;
        }sPageList;
        struct _sSubAlloc
        {
            /* Note: The memory this represents is _not_ implicitly
             * page aligned, neither is its size */
            LinuxMemArea *psParentLinuxMemArea;
            IMG_UINTPTR_T uiByteOffset;
        }sSubAlloc;
        struct _sCmaRegion
        {
            IMG_VOID *hCookie;
            dma_addr_t dmaHandle;
        }sCmaRegion;
    }uData;

    IMG_SIZE_T uiByteSize;		/* Size of memory area */

    IMG_UINT32 ui32AreaFlags;		/* Flags passed at creation time */

    IMG_BOOL bMMapRegistered;		/* Registered with mmap code */

    IMG_BOOL bNeedsCacheInvalidate;	/* Cache should be invalidated on first map? */

    IMG_HANDLE hBMHandle;		/* Handle back to BM for this allocation */

    /* List entry for global list of areas registered for mmap */
    struct list_head	sMMapItem;

    /*
     * Head of list of all mmap offset structures associated with this
     * memory area.
     */
    struct list_head	sMMapOffsetStructList;
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17))
typedef kmem_cache_t LinuxKMemCache;
#else
typedef struct kmem_cache LinuxKMemCache;
#endif


PVRSRV_ERROR LinuxMMInit(IMG_VOID);


IMG_VOID LinuxMMCleanup(IMG_VOID);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KMallocWrapper(uByteSize, uFlags) _KMallocWrapper(uByteSize, uFlags, __FILE__, __LINE__, IMG_FALSE)
#else
#define KMallocWrapper(uByteSize, uFlags) _KMallocWrapper(uByteSize, uFlags, NULL, 0, IMG_FALSE)
#endif
IMG_VOID *_KMallocWrapper(IMG_SIZE_T uiByteSize, gfp_t uFlags, IMG_CHAR *szFileName, IMG_UINT32 ui32Line, IMG_BOOL bSwapAlloc);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KFreeWrapper(pvCpuVAddr) _KFreeWrapper(pvCpuVAddr, __FILE__, __LINE__, IMG_FALSE)
#else
#define KFreeWrapper(pvCpuVAddr) _KFreeWrapper(pvCpuVAddr, NULL, 0, IMG_FALSE)
#endif
IMG_VOID _KFreeWrapper(IMG_VOID *pvCpuVAddr, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line, IMG_BOOL bSwapAlloc);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define VMallocWrapper(uiBytes, ui32AllocFlags) _VMallocWrapper(uiBytes, ui32AllocFlags, __FILE__, __LINE__)
#else
#define VMallocWrapper(uiBytes, ui32AllocFlags) _VMallocWrapper(uiBytes, ui32AllocFlags, NULL, 0)
#endif
IMG_VOID *_VMallocWrapper(IMG_SIZE_T uiBytes, IMG_UINT32 ui32AllocFlags, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define VFreeWrapper(pvCpuVAddr) _VFreeWrapper(pvCpuVAddr, __FILE__, __LINE__)
#else
#define VFreeWrapper(pvCpuVAddr) _VFreeWrapper(pvCpuVAddr, NULL, 0)
#endif
IMG_VOID _VFreeWrapper(IMG_VOID *pvCpuVAddr, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);


LinuxMemArea *NewVMallocLinuxMemArea(IMG_SIZE_T uBytes, IMG_UINT32 ui32AreaFlags);


IMG_VOID FreeVMallocLinuxMemArea(LinuxMemArea *psLinuxMemArea);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define IORemapWrapper(BasePAddr, uiBytes, ui32MappingFlags) \
    _IORemapWrapper(BasePAddr, uiBytes, ui32MappingFlags, __FILE__, __LINE__)
#else
#define IORemapWrapper(BasePAddr, uiBytes, ui32MappingFlags) \
    _IORemapWrapper(BasePAddr, uiBytes, ui32MappingFlags, NULL, 0)
#endif
IMG_VOID *_IORemapWrapper(IMG_CPU_PHYADDR BasePAddr,
                          IMG_SIZE_T uiBytes,
                          IMG_UINT32 ui32MappingFlags,
                          IMG_CHAR *pszFileName,
                          IMG_UINT32 ui32Line);


LinuxMemArea *NewIORemapLinuxMemArea(IMG_CPU_PHYADDR BasePAddr, IMG_SIZE_T uiBytes, IMG_UINT32 ui32AreaFlags);


IMG_VOID FreeIORemapLinuxMemArea(LinuxMemArea *psLinuxMemArea);

LinuxMemArea *NewExternalKVLinuxMemArea(IMG_SYS_PHYADDR *pBasePAddr, IMG_VOID *pvCPUVAddr, IMG_SIZE_T uBytes, IMG_BOOL bPhysContig, IMG_UINT32 ui32AreaFlags);


IMG_VOID FreeExternalKVLinuxMemArea(LinuxMemArea *psLinuxMemArea);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define IOUnmapWrapper(pvIORemapCookie) \
    _IOUnmapWrapper(pvIORemapCookie, __FILE__, __LINE__)
#else
#define IOUnmapWrapper(pvIORemapCookie) \
    _IOUnmapWrapper(pvIORemapCookie, NULL, 0)
#endif
IMG_VOID _IOUnmapWrapper(IMG_VOID *pvIORemapCookie, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);


struct page *LinuxMemAreaOffsetToPage(LinuxMemArea *psLinuxMemArea, IMG_UINT32 ui32ByteOffset);


LinuxKMemCache *KMemCacheCreateWrapper(IMG_CHAR *pszName, size_t Size, size_t Align, IMG_UINT32 ui32Flags);


IMG_VOID KMemCacheDestroyWrapper(LinuxKMemCache *psCache);


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KMemCacheAllocWrapper(psCache, Flags) _KMemCacheAllocWrapper(psCache, Flags, __FILE__, __LINE__)
#else
#define KMemCacheAllocWrapper(psCache, Flags) _KMemCacheAllocWrapper(psCache, Flags, NULL, 0)
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14))
IMG_VOID *_KMemCacheAllocWrapper(LinuxKMemCache *psCache, gfp_t Flags, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);
#else
IMG_VOID *_KMemCacheAllocWrapper(LinuxKMemCache *psCache, int Flags, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);
#endif

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KMemCacheFreeWrapper(psCache, pvObject) _KMemCacheFreeWrapper(psCache, pvObject, __FILE__, __LINE__)
#else
#define KMemCacheFreeWrapper(psCache, pvObject) _KMemCacheFreeWrapper(psCache, pvObject, NULL, 0)
#endif
IMG_VOID _KMemCacheFreeWrapper(LinuxKMemCache *psCache, IMG_VOID *pvObject, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);


const IMG_CHAR *KMemCacheNameWrapper(LinuxKMemCache *psCache);


LinuxMemArea *NewIOLinuxMemArea(IMG_CPU_PHYADDR BasePAddr, IMG_SIZE_T uiBytes, IMG_UINT32 ui32AreaFlags);


IMG_VOID FreeIOLinuxMemArea(LinuxMemArea *psLinuxMemArea);


LinuxMemArea *NewAllocPagesLinuxMemArea(IMG_SIZE_T uiBytes, IMG_UINT32 ui32AreaFlags);


IMG_VOID FreeAllocPagesLinuxMemArea(LinuxMemArea *psLinuxMemArea);


LinuxMemArea *NewSubLinuxMemArea(LinuxMemArea *psParentLinuxMemArea,
                                 IMG_UINTPTR_T uByteOffset,
                                 IMG_SIZE_T uBytes);


IMG_VOID LinuxMemAreaDeepFree(LinuxMemArea *psLinuxMemArea);


#if defined(LINUX_MEM_AREAS_DEBUG)
IMG_VOID LinuxMemAreaRegister(LinuxMemArea *psLinuxMemArea);
#else
#define LinuxMemAreaRegister(X)
#endif


IMG_VOID *LinuxMemAreaToCpuVAddr(LinuxMemArea *psLinuxMemArea);


IMG_CPU_PHYADDR LinuxMemAreaToCpuPAddr(LinuxMemArea *psLinuxMemArea, IMG_UINTPTR_T uByteOffset);


#define	 LinuxMemAreaToCpuPFN(psLinuxMemArea, uByteOffset) PHYS_TO_PFN(LinuxMemAreaToCpuPAddr(psLinuxMemArea, uByteOffset).uiAddr)

IMG_BOOL LinuxMemAreaPhysIsContig(LinuxMemArea *psLinuxMemArea);

static inline LinuxMemArea *
LinuxMemAreaRoot(LinuxMemArea *psLinuxMemArea)
{
    if(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_SUB_ALLOC)
    {
        return psLinuxMemArea->uData.sSubAlloc.psParentLinuxMemArea;
    }
    else
    {
        return psLinuxMemArea;
    }
}


static inline LINUX_MEM_AREA_TYPE
LinuxMemAreaRootType(LinuxMemArea *psLinuxMemArea)
{
    return LinuxMemAreaRoot(psLinuxMemArea)->eAreaType;
}


const IMG_CHAR *LinuxMemAreaTypeToString(LINUX_MEM_AREA_TYPE eMemAreaType);


#if defined(DEBUG) || defined(DEBUG_LINUX_MEM_AREAS)
const IMG_CHAR *HAPFlagsToString(IMG_UINT32 ui32Flags);
#endif

#endif 

