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

#ifndef __IMG_LINUX_MM_H__
#define __IMG_LINUX_MM_H__

#include <linux/version.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/list.h>

#include <linux/io.h>

#define	PHYS_TO_PFN(phys)			((phys) >> PAGE_SHIFT)
#define PFN_TO_PHYS(pfn)			((pfn) << PAGE_SHIFT)

#define RANGE_TO_PAGES(range)						\
	(((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)

#define	ADDR_TO_PAGE_OFFSET(addr) (((unsigned long)(addr)) & (PAGE_SIZE - 1))

#define	REMAP_PFN_RANGE(vma, addr, pfn, size, prot)			\
	remap_pfn_range(vma, addr, pfn, size, prot)

#define	IO_REMAP_PFN_RANGE(vma, addr, pfn, size, prot)			\
	io_remap_pfn_range(vma, addr, pfn, size, prot)

#define	VM_INSERT_PAGE(vma, addr, page) vm_insert_page(vma, addr, page)

static inline u32 VMallocToPhys(void *pCpuVAddr)
{
	return page_to_phys(vmalloc_to_page(pCpuVAddr)) +
		ADDR_TO_PAGE_OFFSET(pCpuVAddr);

}

enum LINUX_MEM_AREA_TYPE {
	LINUX_MEM_AREA_IOREMAP,
	LINUX_MEM_AREA_EXTERNAL_KV,
	LINUX_MEM_AREA_IO,
	LINUX_MEM_AREA_VMALLOC,
	LINUX_MEM_AREA_ALLOC_PAGES,
	LINUX_MEM_AREA_SUB_ALLOC,
	LINUX_MEM_AREA_TYPE_COUNT
};

struct LinuxMemArea;

struct LinuxMemArea {
	enum LINUX_MEM_AREA_TYPE eAreaType;
	union _uData {
		struct _sIORemap {
			struct IMG_CPU_PHYADDR CPUPhysAddr;
			void __iomem *pvIORemapCookie;
		} sIORemap;
		struct _sExternalKV {
			IMG_BOOL bPhysContig;
			union {
				struct IMG_SYS_PHYADDR SysPhysAddr;
				struct IMG_SYS_PHYADDR *pSysPhysAddr;
			} uPhysAddr;
			void *pvExternalKV;
		} sExternalKV;
		struct _sIO {
			struct IMG_CPU_PHYADDR CPUPhysAddr;
		} sIO;
		struct _sVmalloc {
			void *pvVmallocAddress;
		} sVmalloc;
		struct _sPageList {
			struct page **pvPageList;
			void *hBlockPageList;
		} sPageList;
		struct _sSubAlloc {
			struct LinuxMemArea *psParentLinuxMemArea;
			u32 ui32ByteOffset;
		} sSubAlloc;
	} uData;
	u32 ui32ByteSize;
	u32 ui32AreaFlags;
	IMG_BOOL bMMapRegistered;
	struct list_head sMMapItem;
	struct list_head sMMapOffsetStructList;
};

struct kmem_cache;

enum PVRSRV_ERROR LinuxMMInit(void);

void LinuxMMCleanup(void);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KMallocWrapper(ui32ByteSize)	\
		_KMallocWrapper(ui32ByteSize, __FILE__, __LINE__)
#else
#define KMallocWrapper(ui32ByteSize)	\
		_KMallocWrapper(ui32ByteSize, NULL, 0)
#endif
void *_KMallocWrapper(u32 ui32ByteSize, char *szFileName, u32 ui32Line);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KFreeWrapper(pvCpuVAddr) _KFreeWrapper(pvCpuVAddr, __FILE__, __LINE__)
#else
#define KFreeWrapper(pvCpuVAddr) _KFreeWrapper(pvCpuVAddr, NULL, 0)
#endif
void _KFreeWrapper(void *pvCpuVAddr, char *pszFileName, u32 ui32Line);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define VMallocWrapper(ui32Bytes, ui32AllocFlags)	\
		_VMallocWrapper(ui32Bytes, ui32AllocFlags, __FILE__, __LINE__)
#else
#define VMallocWrapper(ui32Bytes, ui32AllocFlags)	\
		_VMallocWrapper(ui32Bytes, ui32AllocFlags, NULL, 0)
#endif
void *_VMallocWrapper(u32 ui32Bytes, u32 ui32AllocFlags, char *pszFileName,
		      u32 ui32Line);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define VFreeWrapper(pvCpuVAddr) _VFreeWrapper(pvCpuVAddr, __FILE__, __LINE__)
#else
#define VFreeWrapper(pvCpuVAddr) _VFreeWrapper(pvCpuVAddr, NULL, 0)
#endif
void _VFreeWrapper(void *pvCpuVAddr, char *pszFileName, u32 ui32Line);

struct LinuxMemArea *NewVMallocLinuxMemArea(u32 ui32Bytes, u32 ui32AreaFlags);

void FreeVMallocLinuxMemArea(struct LinuxMemArea *psLinuxMemArea);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define IORemapWrapper(BasePAddr, ui32Bytes, ui32MappingFlags) \
    _IORemapWrapper(BasePAddr, ui32Bytes, ui32MappingFlags, __FILE__, __LINE__)
#else
#define IORemapWrapper(BasePAddr, ui32Bytes, ui32MappingFlags) \
    _IORemapWrapper(BasePAddr, ui32Bytes, ui32MappingFlags, NULL, 0)
#endif
void __iomem *_IORemapWrapper(struct IMG_CPU_PHYADDR BasePAddr, u32 ui32Bytes,
		u32 ui32MappingFlags, char *pszFileName,
		u32 ui32Line);

struct LinuxMemArea *NewIORemapLinuxMemArea(struct IMG_CPU_PHYADDR BasePAddr,
		u32 ui32Bytes, u32 ui32AreaFlags);

void FreeIORemapLinuxMemArea(struct LinuxMemArea *psLinuxMemArea);

struct LinuxMemArea *NewExternalKVLinuxMemArea(
		struct IMG_SYS_PHYADDR *pBasePAddr, void *pvCPUVAddr,
		u32 ui32Bytes, IMG_BOOL bPhysContig, u32 ui32AreaFlags);

void FreeExternalKVLinuxMemArea(struct LinuxMemArea *psLinuxMemArea);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define IOUnmapWrapper(pvIORemapCookie) \
    _IOUnmapWrapper(pvIORemapCookie, __FILE__, __LINE__)
#else
#define IOUnmapWrapper(pvIORemapCookie) \
    _IOUnmapWrapper(pvIORemapCookie, NULL, 0)
#endif
void _IOUnmapWrapper(void __iomem *pvIORemapCookie, char *pszFileName,
			 u32 ui32Line);

struct page *LinuxMemAreaOffsetToPage(struct LinuxMemArea *psLinuxMemArea,
				      u32 ui32ByteOffset);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KMemCacheAllocWrapper(psCache, Flags)	\
		_KMemCacheAllocWrapper(psCache, Flags, __FILE__, __LINE__)
#else
#define KMemCacheAllocWrapper(psCache, Flags)	\
		_KMemCacheAllocWrapper(psCache, Flags, NULL, 0)
#endif

void *_KMemCacheAllocWrapper(struct kmem_cache *psCache, gfp_t Flags,
				 char *pszFileName, u32 ui32Line);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
#define KMemCacheFreeWrapper(psCache, pvObject)	\
		_KMemCacheFreeWrapper(psCache, pvObject, __FILE__, __LINE__)
#else
#define KMemCacheFreeWrapper(psCache, pvObject)	\
		_KMemCacheFreeWrapper(psCache, pvObject, NULL, 0)
#endif
void _KMemCacheFreeWrapper(struct kmem_cache *psCache, void *pvObject,
				char *pszFileName, u32 ui32Line);

const char *KMemCacheNameWrapper(struct kmem_cache *psCache);

struct LinuxMemArea *NewIOLinuxMemArea(struct IMG_CPU_PHYADDR BasePAddr,
				u32 ui32Bytes, u32 ui32AreaFlags);

void FreeIOLinuxMemArea(struct LinuxMemArea *psLinuxMemArea);

struct LinuxMemArea *NewAllocPagesLinuxMemArea(u32 ui32Bytes,
				u32 ui32AreaFlags);

void FreeAllocPagesLinuxMemArea(struct LinuxMemArea *psLinuxMemArea);

struct LinuxMemArea *NewSubLinuxMemArea(
				struct LinuxMemArea *psParentLinuxMemArea,
				u32 ui32ByteOffset, u32 ui32Bytes);

void LinuxMemAreaDeepFree(struct LinuxMemArea *psLinuxMemArea);

#if defined(LINUX_MEM_AREAS_DEBUG)
void LinuxMemAreaRegister(struct LinuxMemArea *psLinuxMemArea);
#else
#define LinuxMemAreaRegister(X)
#endif

void *LinuxMemAreaToCpuVAddr(struct LinuxMemArea *psLinuxMemArea);

struct IMG_CPU_PHYADDR LinuxMemAreaToCpuPAddr(
				struct LinuxMemArea *psLinuxMemArea,
				u32 ui32ByteOffset);

#define	 LinuxMemAreaToCpuPFN(psLinuxMemArea, ui32ByteOffset)		\
	PHYS_TO_PFN(LinuxMemAreaToCpuPAddr(psLinuxMemArea,		\
					   ui32ByteOffset).uiAddr)

void inv_cache_mem_area(const struct LinuxMemArea *mem_area);

IMG_BOOL LinuxMemAreaPhysIsContig(struct LinuxMemArea *psLinuxMemArea);

static inline struct LinuxMemArea *LinuxMemAreaRoot(struct LinuxMemArea
						    *psLinuxMemArea)
{
	if (psLinuxMemArea->eAreaType == LINUX_MEM_AREA_SUB_ALLOC)
		return psLinuxMemArea->uData.sSubAlloc.psParentLinuxMemArea;
	else
		return psLinuxMemArea;
}

static inline enum LINUX_MEM_AREA_TYPE LinuxMemAreaRootType(struct LinuxMemArea
							    *psLinuxMemArea)
{
	return LinuxMemAreaRoot(psLinuxMemArea)->eAreaType;
}

const char *LinuxMemAreaTypeToString(enum LINUX_MEM_AREA_TYPE eMemAreaType);

#if defined(DEBUG) || defined(DEBUG_LINUX_MEM_AREAS)
const char *HAPFlagsToString(u32 ui32Flags);
#endif

#endif
