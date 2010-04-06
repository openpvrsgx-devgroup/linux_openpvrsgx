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

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <asm/page.h>
#include <asm/shmparam.h>
#include <asm/pgtable.h>
#include <linux/sched.h>
#include <asm/current.h>
#include "img_defs.h"
#include "services.h"
#include "servicesint.h"
#include "pvrmmap.h"
#include "mutils.h"
#include "mmap.h"
#include "mm.h"
#include "pvr_debug.h"
#include "osfunc.h"
#include "proc.h"
#include "handle.h"
#include "perproc.h"
#include "env_perproc.h"
#include "bridged_support.h"

static struct mutex g_sMMapMutex;

static struct kmem_cache *g_psMemmapCache;
static LIST_HEAD(g_sMMapAreaList);
static LIST_HEAD(g_sMMapOffsetStructList);
#if defined(DEBUG_LINUX_MMAP_AREAS)
static u32 g_ui32RegisteredAreas;
static u32 g_ui32TotalByteSize;
#endif

#define	FIRST_PHYSICAL_PFN	0
#define	LAST_PHYSICAL_PFN	0x7ffffffful
#define	FIRST_SPECIAL_PFN	(LAST_PHYSICAL_PFN + 1)
#define	LAST_SPECIAL_PFN	0xfffffffful

#define	MAX_MMAP_HANDLE		0x7ffffffful

static inline IMG_BOOL PFNIsPhysical(u32 pfn)
{
	return pfn >= FIRST_PHYSICAL_PFN && pfn <= LAST_PHYSICAL_PFN;
}

static inline IMG_BOOL PFNIsSpecial(u32 pfn)
{
	return pfn >= FIRST_SPECIAL_PFN && pfn <= LAST_SPECIAL_PFN;
}

static inline void *MMapOffsetToHandle(u32 pfn)
{
	if (PFNIsPhysical(pfn)) {
		PVR_ASSERT(PFNIsPhysical(pfn));
		return NULL;
	}

	return (void *)(pfn - FIRST_SPECIAL_PFN);
}

static inline u32 HandleToMMapOffset(void *hHandle)
{
	u32 ulHandle = (u32) hHandle;

	if (PFNIsSpecial(ulHandle)) {
		PVR_ASSERT(PFNIsSpecial(ulHandle));
		return 0;
		}

	return ulHandle + FIRST_SPECIAL_PFN;
	}

static inline IMG_BOOL LinuxMemAreaUsesPhysicalMap(
					struct LinuxMemArea *psLinuxMemArea)
{
	return LinuxMemAreaPhysIsContig(psLinuxMemArea);
}

static inline u32 GetCurrentThreadID(void)
{

	return (u32) current->pid;
}

static struct KV_OFFSET_STRUCT *CreateOffsetStruct(struct LinuxMemArea
						   *psLinuxMemArea,
						   u32 ui32Offset,
						   u32 ui32RealByteSize)
{
	struct KV_OFFSET_STRUCT *psOffsetStruct;
#if defined(CONFIG_PVR_DEBUG) || defined(DEBUG_LINUX_MMAP_AREAS)
	const char *pszName =
	    LinuxMemAreaTypeToString(LinuxMemAreaRootType(psLinuxMemArea));
#endif

	PVR_DPF(PVR_DBG_MESSAGE,
		 "%s(%s, psLinuxMemArea: 0x%p, ui32AllocFlags: 0x%8lx)",
		 __func__, pszName, psLinuxMemArea,
		 psLinuxMemArea->ui32AreaFlags);

	PVR_ASSERT(psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC
		   || LinuxMemAreaRoot(psLinuxMemArea)->eAreaType !=
		   LINUX_MEM_AREA_SUB_ALLOC);

	PVR_ASSERT(psLinuxMemArea->bMMapRegistered);

	psOffsetStruct = KMemCacheAllocWrapper(g_psMemmapCache, GFP_KERNEL);
	if (psOffsetStruct == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "PVRMMapRegisterArea: "
			"Couldn't alloc another mapping record from cache");
		return NULL;
	}

	psOffsetStruct->ui32MMapOffset = ui32Offset;
	psOffsetStruct->psLinuxMemArea = psLinuxMemArea;
	psOffsetStruct->ui32Mapped = 0;
	psOffsetStruct->ui32RealByteSize = ui32RealByteSize;
	psOffsetStruct->ui32TID = GetCurrentThreadID();
	psOffsetStruct->ui32PID = OSGetCurrentProcessIDKM();
	psOffsetStruct->bOnMMapList = IMG_FALSE;
	psOffsetStruct->ui32RefCount = 0;
	psOffsetStruct->ui32UserVAddr = 0;
#if defined(DEBUG_LINUX_MMAP_AREAS)

	psOffsetStruct->pszName = pszName;
#endif

	list_add_tail(&psOffsetStruct->sAreaItem,
		      &psLinuxMemArea->sMMapOffsetStructList);

	return psOffsetStruct;
}

static void DestroyOffsetStruct(struct KV_OFFSET_STRUCT *psOffsetStruct)
{
	list_del(&psOffsetStruct->sAreaItem);

	if (psOffsetStruct->bOnMMapList)
		list_del(&psOffsetStruct->sMMapItem);

	PVR_DPF(PVR_DBG_MESSAGE, "%s: Table entry: "
		 "psLinuxMemArea=0x%08lX, CpuPAddr=0x%08lX", __func__,
		 psOffsetStruct->psLinuxMemArea,
		 LinuxMemAreaToCpuPAddr(psOffsetStruct->psLinuxMemArea, 0));

	KMemCacheFreeWrapper(g_psMemmapCache, psOffsetStruct);
}

static inline void DetermineUsersSizeAndByteOffset(struct LinuxMemArea
						   *psLinuxMemArea,
						   u32 *pui32RealByteSize,
						   u32 *pui32ByteOffset)
{
	u32 ui32PageAlignmentOffset;
	struct IMG_CPU_PHYADDR CpuPAddr;

	CpuPAddr = LinuxMemAreaToCpuPAddr(psLinuxMemArea, 0);
	ui32PageAlignmentOffset = ADDR_TO_PAGE_OFFSET(CpuPAddr.uiAddr);

	*pui32ByteOffset = ui32PageAlignmentOffset;

	*pui32RealByteSize =
	    PAGE_ALIGN(psLinuxMemArea->ui32ByteSize + ui32PageAlignmentOffset);
}

enum PVRSRV_ERROR PVRMMapOSMemHandleToMMapData(
			struct PVRSRV_PER_PROCESS_DATA *psPerProc,
			void *hMHandle, u32 *pui32MMapOffset,
			u32 *pui32ByteOffset, u32 *pui32RealByteSize,
			u32 *pui32UserVAddr)
{
	struct LinuxMemArea *psLinuxMemArea;
	struct KV_OFFSET_STRUCT *psOffsetStruct;
	void *hOSMemHandle;
	enum PVRSRV_ERROR eError = PVRSRV_ERROR_GENERIC;

	mutex_lock(&g_sMMapMutex);

	PVR_ASSERT(PVRSRVGetMaxHandle(psPerProc->psHandleBase) <=
		   MAX_MMAP_HANDLE);

	eError =
	    PVRSRVLookupOSMemHandle(psPerProc->psHandleBase, &hOSMemHandle,
				    hMHandle);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "%s: Lookup of handle 0x%lx failed",
			 __func__, hMHandle);

		goto exit_unlock;
	}

	psLinuxMemArea = (struct LinuxMemArea *)hOSMemHandle;

	DetermineUsersSizeAndByteOffset(psLinuxMemArea,
				       pui32RealByteSize, pui32ByteOffset);

	list_for_each_entry(psOffsetStruct,
			    &psLinuxMemArea->sMMapOffsetStructList, sAreaItem) {
		if (psPerProc->ui32PID == psOffsetStruct->ui32PID) {
			PVR_ASSERT(*pui32RealByteSize ==
				   psOffsetStruct->ui32RealByteSize);

			*pui32MMapOffset = psOffsetStruct->ui32MMapOffset;
			*pui32UserVAddr = psOffsetStruct->ui32UserVAddr;
			psOffsetStruct->ui32RefCount++;

			eError = PVRSRV_OK;
			goto exit_unlock;
		}
	}

	*pui32UserVAddr = 0;

	if (LinuxMemAreaUsesPhysicalMap(psLinuxMemArea)) {
		*pui32MMapOffset = LinuxMemAreaToCpuPFN(psLinuxMemArea, 0);
		PVR_ASSERT(PFNIsPhysical(*pui32MMapOffset));
	} else {
		*pui32MMapOffset = HandleToMMapOffset(hMHandle);
		PVR_ASSERT(PFNIsSpecial(*pui32MMapOffset));
	}

	psOffsetStruct = CreateOffsetStruct(psLinuxMemArea, *pui32MMapOffset,
					    *pui32RealByteSize);
	if (psOffsetStruct == NULL) {
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto exit_unlock;
	}

	list_add_tail(&psOffsetStruct->sMMapItem, &g_sMMapOffsetStructList);
	psOffsetStruct->bOnMMapList = IMG_TRUE;
	psOffsetStruct->ui32RefCount++;
	eError = PVRSRV_OK;

exit_unlock:
	mutex_unlock(&g_sMMapMutex);

	return eError;
}

enum PVRSRV_ERROR PVRMMapReleaseMMapData(
			struct PVRSRV_PER_PROCESS_DATA *psPerProc,
			void *hMHandle, IMG_BOOL *pbMUnmap,
			u32 *pui32RealByteSize, u32 *pui32UserVAddr)
{
	struct LinuxMemArea *psLinuxMemArea;
	struct KV_OFFSET_STRUCT *psOffsetStruct;
	void *hOSMemHandle;
	enum PVRSRV_ERROR eError = PVRSRV_ERROR_GENERIC;
	u32 ui32PID = OSGetCurrentProcessIDKM();

	mutex_lock(&g_sMMapMutex);

	PVR_ASSERT(PVRSRVGetMaxHandle(psPerProc->psHandleBase) <=
		   MAX_MMAP_HANDLE);

	eError = PVRSRVLookupOSMemHandle(psPerProc->psHandleBase, &hOSMemHandle,
				    hMHandle);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "%s: Lookup of handle 0x%lx failed",
			 __func__, hMHandle);

		goto exit_unlock;
	}

	psLinuxMemArea = (struct LinuxMemArea *)hOSMemHandle;

	list_for_each_entry(psOffsetStruct,
			    &psLinuxMemArea->sMMapOffsetStructList, sAreaItem) {
		if (psOffsetStruct->ui32PID == ui32PID) {
			if (psOffsetStruct->ui32RefCount == 0) {
				PVR_DPF(PVR_DBG_ERROR, "%s: Attempt to "
					"release mmap data with zero reference "
					"count for offset struct 0x%p, "
					"memory area 0x%p",
					 __func__, psOffsetStruct,
					 psLinuxMemArea);
				eError = PVRSRV_ERROR_GENERIC;
				goto exit_unlock;
			}

			psOffsetStruct->ui32RefCount--;

			*pbMUnmap = (psOffsetStruct->ui32RefCount == 0)
			    && (psOffsetStruct->ui32UserVAddr != 0);

			*pui32UserVAddr =
			    (*pbMUnmap) ? psOffsetStruct->ui32UserVAddr : 0;
			*pui32RealByteSize =
			    (*pbMUnmap) ? psOffsetStruct->ui32RealByteSize : 0;

			eError = PVRSRV_OK;
			goto exit_unlock;
		}
	}

	PVR_DPF(PVR_DBG_ERROR, "%s: Mapping data not found for handle "
			"0x%lx (memory area 0x%p)",
			__func__, hMHandle, psLinuxMemArea);

	eError = PVRSRV_ERROR_GENERIC;

exit_unlock:
	mutex_unlock(&g_sMMapMutex);

	return eError;
}

static inline struct KV_OFFSET_STRUCT *FindOffsetStructByOffset(u32 ui32Offset,
							u32 ui32RealByteSize)
{
	struct KV_OFFSET_STRUCT *psOffsetStruct;
	u32 ui32TID = GetCurrentThreadID();
	u32 ui32PID = OSGetCurrentProcessIDKM();

	list_for_each_entry(psOffsetStruct, &g_sMMapOffsetStructList,
			    sMMapItem) {
		if (ui32Offset == psOffsetStruct->ui32MMapOffset &&
		    ui32RealByteSize == psOffsetStruct->ui32RealByteSize &&
		    psOffsetStruct->ui32PID == ui32PID)
			if (!PFNIsPhysical(ui32Offset) ||
			    psOffsetStruct->ui32TID == ui32TID)
				return psOffsetStruct;
	}

	return NULL;
}

static IMG_BOOL DoMapToUser(struct LinuxMemArea *psLinuxMemArea,
	    struct vm_area_struct *ps_vma, u32 ui32ByteOffset)
{
	u32 ui32ByteSize;

	if (psLinuxMemArea->eAreaType == LINUX_MEM_AREA_SUB_ALLOC)
		return DoMapToUser(LinuxMemAreaRoot(psLinuxMemArea), ps_vma,
				   psLinuxMemArea->uData.sSubAlloc.
				   ui32ByteOffset + ui32ByteOffset);

	ui32ByteSize = ps_vma->vm_end - ps_vma->vm_start;
	PVR_ASSERT(ADDR_TO_PAGE_OFFSET(ui32ByteSize) == 0);

	if (PFNIsPhysical(ps_vma->vm_pgoff)) {
		int result;

		PVR_ASSERT(LinuxMemAreaPhysIsContig(psLinuxMemArea));
		PVR_ASSERT(LinuxMemAreaToCpuPFN(psLinuxMemArea, ui32ByteOffset)
			   == ps_vma->vm_pgoff);

		result =
		    IO_REMAP_PFN_RANGE(ps_vma, ps_vma->vm_start,
				       ps_vma->vm_pgoff, ui32ByteSize,
				       ps_vma->vm_page_prot);

		if (result == 0)
			return IMG_TRUE;

		PVR_DPF(PVR_DBG_MESSAGE,
			"%s: Failed to map contiguous physical address "
			"range (%d), trying non-contiguous path",
				 __func__, result);
	}

	{
		u32 ulVMAPos;
		u32 ui32ByteEnd = ui32ByteOffset + ui32ByteSize;
		u32 ui32PA;

		for (ui32PA = ui32ByteOffset; ui32PA < ui32ByteEnd;
		     ui32PA += PAGE_SIZE) {
			u32 pfn = LinuxMemAreaToCpuPFN(psLinuxMemArea, ui32PA);

			if (!pfn_valid(pfn)) {
				PVR_DPF(PVR_DBG_ERROR,
					 "%s: Error - PFN invalid: 0x%lx",
					 __func__, pfn);
				return IMG_FALSE;
			}
		}

		ulVMAPos = ps_vma->vm_start;
		for (ui32PA = ui32ByteOffset; ui32PA < ui32ByteEnd;
		     ui32PA += PAGE_SIZE) {
			u32 pfn;
			struct page *psPage;
			int result;

			pfn = LinuxMemAreaToCpuPFN(psLinuxMemArea, ui32PA);
			PVR_ASSERT(pfn_valid(pfn));

			psPage = pfn_to_page(pfn);

			result = VM_INSERT_PAGE(ps_vma, ulVMAPos, psPage);
			if (result != 0) {
				PVR_DPF(PVR_DBG_ERROR,
				"%s: Error - VM_INSERT_PAGE failed (%d)",
					 __func__, result);
				return IMG_FALSE;
			}
			ulVMAPos += PAGE_SIZE;
		}
	}

	return IMG_TRUE;
}

static IMG_BOOL CheckSize(struct LinuxMemArea *psLinuxMemArea, u32 ui32ByteSize)
{
	struct IMG_CPU_PHYADDR CpuPAddr;
	u32 ui32PageAlignmentOffset;
	u32 ui32RealByteSize;
	CpuPAddr = LinuxMemAreaToCpuPAddr(psLinuxMemArea, 0);
	ui32PageAlignmentOffset = ADDR_TO_PAGE_OFFSET(CpuPAddr.uiAddr);
	ui32RealByteSize =
	    PAGE_ALIGN(psLinuxMemArea->ui32ByteSize + ui32PageAlignmentOffset);
	if (ui32RealByteSize < ui32ByteSize) {
		PVR_DPF(PVR_DBG_ERROR, "Cannot mmap %ld bytes from: "
					"%-8p %-8p %08lx %-8ld %-24s\n",
			 ui32ByteSize, psLinuxMemArea,
			 LinuxMemAreaToCpuVAddr(psLinuxMemArea),
			 LinuxMemAreaToCpuPAddr(psLinuxMemArea, 0).uiAddr,
			 psLinuxMemArea->ui32ByteSize,
			 LinuxMemAreaTypeToString(psLinuxMemArea->eAreaType));
		return IMG_FALSE;
	}
	return IMG_TRUE;
}

static void MMapVOpenNoLock(struct vm_area_struct *ps_vma)
{
	struct KV_OFFSET_STRUCT *psOffsetStruct =
	    (struct KV_OFFSET_STRUCT *)ps_vma->vm_private_data;

	PVR_ASSERT(psOffsetStruct != NULL);
	psOffsetStruct->ui32Mapped++;
	PVR_ASSERT(!psOffsetStruct->bOnMMapList);

	if (psOffsetStruct->ui32Mapped > 1) {
		PVR_DPF(PVR_DBG_WARNING,
			"%s: Offset structure 0x%p is being shared "
			"across processes (psOffsetStruct->ui32Mapped: %lu)",
			 __func__, psOffsetStruct, psOffsetStruct->ui32Mapped);
		PVR_ASSERT((ps_vma->vm_flags & VM_DONTCOPY) == 0);
	}
#if defined(DEBUG_LINUX_MMAP_AREAS)

	PVR_DPF(PVR_DBG_MESSAGE,
	"%s: psLinuxMemArea 0x%p, KVAddress 0x%p MMapOffset %ld, ui32Mapped %d",
		 __func__,
		 psOffsetStruct->psLinuxMemArea,
		 LinuxMemAreaToCpuVAddr(psOffsetStruct->psLinuxMemArea),
		 psOffsetStruct->ui32MMapOffset, psOffsetStruct->ui32Mapped);
#endif

}

static void MMapVOpen(struct vm_area_struct *ps_vma)
{
	mutex_lock(&g_sMMapMutex);
	MMapVOpenNoLock(ps_vma);
	mutex_unlock(&g_sMMapMutex);
}

static void MMapVCloseNoLock(struct vm_area_struct *ps_vma)
{
	struct KV_OFFSET_STRUCT *psOffsetStruct =
	    (struct KV_OFFSET_STRUCT *)ps_vma->vm_private_data;

	PVR_ASSERT(psOffsetStruct != NULL);
#if defined(DEBUG_LINUX_MMAP_AREAS)
	PVR_DPF(PVR_DBG_MESSAGE, "%s: psLinuxMemArea "
			"0x%p, CpuVAddr 0x%p ui32MMapOffset %ld, ui32Mapped %d",
		 __func__,
		 psOffsetStruct->psLinuxMemArea,
		 LinuxMemAreaToCpuVAddr(psOffsetStruct->psLinuxMemArea),
		     psOffsetStruct->ui32MMapOffset,
		     psOffsetStruct->ui32Mapped);
#endif

	PVR_ASSERT(!psOffsetStruct->bOnMMapList);
	psOffsetStruct->ui32Mapped--;
	if (psOffsetStruct->ui32Mapped == 0) {
		if (psOffsetStruct->ui32RefCount != 0)
			PVR_DPF(PVR_DBG_MESSAGE,
			"%s: psOffsetStruct 0x%p has non-zero "
			"reference count (ui32RefCount = %lu). "
			"User mode address of start of mapping: 0x%lx",
				 __func__, psOffsetStruct,
				 psOffsetStruct->ui32RefCount,
				 psOffsetStruct->ui32UserVAddr);

		DestroyOffsetStruct(psOffsetStruct);
	}
	ps_vma->vm_private_data = NULL;
}

static void MMapVClose(struct vm_area_struct *ps_vma)
{
	mutex_lock(&g_sMMapMutex);
	MMapVCloseNoLock(ps_vma);
	mutex_unlock(&g_sMMapMutex);
}

static struct vm_operations_struct MMapIOOps = {
	.open	= MMapVOpen,
	.close	= MMapVClose
};

int PVRMMap(struct file *pFile, struct vm_area_struct *ps_vma)
{
	u32 ui32ByteSize;
	struct KV_OFFSET_STRUCT *psOffsetStruct = NULL;
	int iRetVal = 0;

	PVR_UNREFERENCED_PARAMETER(pFile);

	mutex_lock(&g_sMMapMutex);

	ui32ByteSize = ps_vma->vm_end - ps_vma->vm_start;

	PVR_DPF(PVR_DBG_MESSAGE,
		 "%s: Received mmap(2) request with ui32MMapOffset 0x%08lx,"
		 " and ui32ByteSize %ld(0x%08lx)", __func__, ps_vma->vm_pgoff,
		 ui32ByteSize, ui32ByteSize);

	if ((ps_vma->vm_flags & VM_WRITE) && !(ps_vma->vm_flags & VM_SHARED)) {
		PVR_DPF(PVR_DBG_ERROR,
			 "%s: Cannot mmap non-shareable writable areas",
			 __func__);
		iRetVal = -EINVAL;
		goto unlock_and_return;
	}

	psOffsetStruct =
	    FindOffsetStructByOffset(ps_vma->vm_pgoff, ui32ByteSize);
	if (psOffsetStruct == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
		"%s: Attempted to mmap unregistered area at vm_pgoff %ld",
			 __func__, ps_vma->vm_pgoff);
		iRetVal = -EINVAL;
		goto unlock_and_return;
	}
	list_del(&psOffsetStruct->sMMapItem);
	psOffsetStruct->bOnMMapList = IMG_FALSE;

	PVR_DPF(PVR_DBG_MESSAGE, "%s: Mapped psLinuxMemArea 0x%p\n",
		 __func__, psOffsetStruct->psLinuxMemArea);

	if (!CheckSize(psOffsetStruct->psLinuxMemArea, ui32ByteSize)) {
		iRetVal = -EINVAL;
		goto unlock_and_return;
	}

	ps_vma->vm_flags |= VM_RESERVED;
	ps_vma->vm_flags |= VM_IO;

	ps_vma->vm_flags |= VM_DONTEXPAND;

	ps_vma->vm_flags |= VM_DONTCOPY;

	ps_vma->vm_private_data = (void *)psOffsetStruct;

	switch (psOffsetStruct->psLinuxMemArea->
		ui32AreaFlags & PVRSRV_HAP_CACHETYPE_MASK) {
	case PVRSRV_HAP_CACHED:

		break;
	case PVRSRV_HAP_WRITECOMBINE:
		ps_vma->vm_page_prot = PGPROT_WC(ps_vma->vm_page_prot);
		break;
	case PVRSRV_HAP_UNCACHED:
		ps_vma->vm_page_prot = PGPROT_UC(ps_vma->vm_page_prot);
		break;
	default:
		PVR_DPF(PVR_DBG_ERROR, "%s: unknown cache type", __func__);
		iRetVal = -EINVAL;
		goto unlock_and_return;
	}

	ps_vma->vm_ops = &MMapIOOps;

	if (!DoMapToUser(psOffsetStruct->psLinuxMemArea, ps_vma, 0)) {
		iRetVal = -EAGAIN;
		goto unlock_and_return;
	}

	PVR_ASSERT(psOffsetStruct->ui32UserVAddr == 0);

	psOffsetStruct->ui32UserVAddr = ps_vma->vm_start;

	MMapVOpenNoLock(ps_vma);

	PVR_DPF(PVR_DBG_MESSAGE, "%s: Mapped area at offset 0x%08lx\n",
		 __func__, ps_vma->vm_pgoff);

unlock_and_return:
	if (iRetVal != 0 && psOffsetStruct != NULL)
		DestroyOffsetStruct(psOffsetStruct);

	mutex_unlock(&g_sMMapMutex);

	return iRetVal;
}

#if defined(DEBUG_LINUX_MMAP_AREAS)
static off_t PrintMMapReg_helper(char *buffer, size_t size,
				 const struct KV_OFFSET_STRUCT *psOffsetStruct,
				 struct LinuxMemArea *psLinuxMemArea)
{
	off_t Ret;
	u32 ui32RealByteSize;
	u32 ui32ByteOffset;

	PVR_ASSERT(psOffsetStruct->psLinuxMemArea == psLinuxMemArea);

	DetermineUsersSizeAndByteOffset(psLinuxMemArea,
					&ui32RealByteSize,
					&ui32ByteOffset);

	Ret = printAppend(buffer, size, 0,
			  "%-8p       %08x %-8p %08x %08x   "
			  "%-8d   %-24s %-5u %-8s %08x(%s)\n",
			  psLinuxMemArea,
			  psOffsetStruct->ui32UserVAddr + ui32ByteOffset,
			  LinuxMemAreaToCpuVAddr(psLinuxMemArea),
			  LinuxMemAreaToCpuPAddr(psLinuxMemArea, 0).uiAddr,
			  psOffsetStruct->ui32MMapOffset,
			  psLinuxMemArea->ui32ByteSize,
			  LinuxMemAreaTypeToString(psLinuxMemArea->eAreaType),
			  psOffsetStruct->ui32PID,
			  psOffsetStruct->pszName,
			  psLinuxMemArea->ui32AreaFlags,
			  HAPFlagsToString(psLinuxMemArea->ui32AreaFlags));
	return Ret;

}

static off_t PrintMMapRegistrations(char *buffer, size_t size, off_t off)
{
	struct LinuxMemArea *psLinuxMemArea;
	off_t Ret;

	mutex_lock(&g_sMMapMutex);

	if (!off) {
		Ret = printAppend(buffer, size, 0,
				"Allocations registered for mmap: %u\n"
				"In total these areas correspond to %u bytes\n"
				"psLinuxMemArea UserVAddr KernelVAddr "
				"CpuPAddr MMapOffset ByteLength "
				"LinuxMemType             "
				"Pid   Name     Flags\n",
				g_ui32RegisteredAreas, g_ui32TotalByteSize);

		goto unlock_and_return;
	}

	if (size < 135) {
		Ret = 0;
		goto unlock_and_return;
	}

	PVR_ASSERT(off != 0);
	list_for_each_entry(psLinuxMemArea, &g_sMMapAreaList, sMMapItem) {
		struct KV_OFFSET_STRUCT *psOffsetStruct;

		list_for_each_entry(psOffsetStruct,
				    &psLinuxMemArea->sMMapOffsetStructList,
				    sAreaItem) {
			off--;
			if (off == 0) {
				Ret = PrintMMapReg_helper(buffer, size,
						psOffsetStruct, psLinuxMemArea);
				goto unlock_and_return;
			}
		}
	}
	Ret = END_OF_FILE;

unlock_and_return:
	mutex_unlock(&g_sMMapMutex);
	return Ret;
}
#endif

enum PVRSRV_ERROR PVRMMapRegisterArea(struct LinuxMemArea *psLinuxMemArea)
{
	enum PVRSRV_ERROR eError = PVRSRV_ERROR_GENERIC;
#if defined(CONFIG_PVR_DEBUG) || defined(DEBUG_LINUX_MMAP_AREAS)
	const char *pszName =
	    LinuxMemAreaTypeToString(LinuxMemAreaRootType(psLinuxMemArea));
#endif

	mutex_lock(&g_sMMapMutex);

	PVR_DPF(PVR_DBG_MESSAGE,
		 "%s(%s, psLinuxMemArea 0x%p, ui32AllocFlags 0x%8lx)",
		 __func__, pszName, psLinuxMemArea,
		 psLinuxMemArea->ui32AreaFlags);

	PVR_ASSERT(psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC
		   || LinuxMemAreaRoot(psLinuxMemArea)->eAreaType !=
		   LINUX_MEM_AREA_SUB_ALLOC);

	if (psLinuxMemArea->bMMapRegistered) {
		PVR_DPF(PVR_DBG_ERROR,
			 "%s: psLinuxMemArea 0x%p is already registered",
			 __func__, psLinuxMemArea);
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto exit_unlock;
	}

	list_add_tail(&psLinuxMemArea->sMMapItem, &g_sMMapAreaList);

	psLinuxMemArea->bMMapRegistered = IMG_TRUE;

#if defined(DEBUG_LINUX_MMAP_AREAS)
	g_ui32RegisteredAreas++;

	if (psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC)
		g_ui32TotalByteSize += psLinuxMemArea->ui32ByteSize;
#endif

	eError = PVRSRV_OK;

exit_unlock:
	mutex_unlock(&g_sMMapMutex);

	return eError;
}

enum PVRSRV_ERROR PVRMMapRemoveRegisteredArea(
					struct LinuxMemArea *psLinuxMemArea)
{
	enum PVRSRV_ERROR eError = PVRSRV_ERROR_GENERIC;
	struct KV_OFFSET_STRUCT *psOffsetStruct, *psTmpOffsetStruct;

	mutex_lock(&g_sMMapMutex);

	PVR_ASSERT(psLinuxMemArea->bMMapRegistered);

	list_for_each_entry_safe(psOffsetStruct, psTmpOffsetStruct,
				 &psLinuxMemArea->sMMapOffsetStructList,
				 sAreaItem) {
		if (psOffsetStruct->ui32Mapped != 0) {
			PVR_DPF(PVR_DBG_ERROR, "%s: psOffsetStruct "
					"0x%p for memory area "
					"0x0x%p is still mapped; "
					"psOffsetStruct->ui32Mapped %lu",
				 __func__, psOffsetStruct, psLinuxMemArea,
				 psOffsetStruct->ui32Mapped);
			eError = PVRSRV_ERROR_GENERIC;
			goto exit_unlock;
		} else {

			PVR_DPF(PVR_DBG_WARNING,
				 "%s: psOffsetStruct 0x%p was never mapped",
				 __func__, psOffsetStruct);
		}

		PVR_ASSERT((psOffsetStruct->ui32Mapped == 0)
			   && psOffsetStruct->bOnMMapList);

		DestroyOffsetStruct(psOffsetStruct);
	}

	list_del(&psLinuxMemArea->sMMapItem);

	psLinuxMemArea->bMMapRegistered = IMG_FALSE;

#if defined(DEBUG_LINUX_MMAP_AREAS)
	g_ui32RegisteredAreas--;
	if (psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC)
		g_ui32TotalByteSize -= psLinuxMemArea->ui32ByteSize;
#endif

	eError = PVRSRV_OK;

exit_unlock:
	mutex_unlock(&g_sMMapMutex);
	return eError;
}

enum PVRSRV_ERROR LinuxMMapPerProcessConnect(struct PVRSRV_ENV_PER_PROCESS_DATA
					     *psEnvPerProc)
{
	PVR_UNREFERENCED_PARAMETER(psEnvPerProc);

	return PVRSRV_OK;
}

void LinuxMMapPerProcessDisconnect(struct PVRSRV_ENV_PER_PROCESS_DATA
				   *psEnvPerProc)
{
	struct KV_OFFSET_STRUCT *psOffsetStruct, *psTmpOffsetStruct;
	IMG_BOOL bWarn = IMG_FALSE;
	u32 ui32PID = OSGetCurrentProcessIDKM();

	PVR_UNREFERENCED_PARAMETER(psEnvPerProc);

	mutex_lock(&g_sMMapMutex);

	list_for_each_entry_safe(psOffsetStruct, psTmpOffsetStruct,
				 &g_sMMapOffsetStructList, sMMapItem) {
		if (psOffsetStruct->ui32PID == ui32PID) {
			if (!bWarn) {
				PVR_DPF(PVR_DBG_WARNING, "%s: process has "
						"unmapped offset structures. "
						"Removing them",
					 __func__);
				bWarn = IMG_TRUE;
			}
			PVR_ASSERT(psOffsetStruct->ui32Mapped == 0);
			PVR_ASSERT(psOffsetStruct->bOnMMapList);

			DestroyOffsetStruct(psOffsetStruct);
		}
	}

	mutex_unlock(&g_sMMapMutex);
}

enum PVRSRV_ERROR LinuxMMapPerProcessHandleOptions(struct PVRSRV_HANDLE_BASE
						   *psHandleBase)
{
	enum PVRSRV_ERROR eError = PVRSRV_OK;

	eError = PVRSRVSetMaxHandle(psHandleBase, MAX_MMAP_HANDLE);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "%s: failed to set handle limit (%d)",
			 __func__, eError);
		return eError;
	}

	return eError;
}

void PVRMMapInit(void)
{
	mutex_init(&g_sMMapMutex);

	g_psMemmapCache =
	    kmem_cache_create("img-mmap", sizeof(struct KV_OFFSET_STRUCT),
				   0, 0, NULL);
	if (!g_psMemmapCache) {
		PVR_DPF(PVR_DBG_ERROR, "%s: failed to allocate kmem_cache",
			 __func__);
		goto error;
	}
#if defined(DEBUG_LINUX_MMAP_AREAS)
	CreateProcReadEntry("mmap", PrintMMapRegistrations);
#endif

	return;

error:
	PVRMMapCleanup();
	return;
}

void PVRMMapCleanup(void)
{
	enum PVRSRV_ERROR eError;

	if (!list_empty(&g_sMMapAreaList)) {
		struct LinuxMemArea *psLinuxMemArea, *psTmpMemArea;

		PVR_DPF(PVR_DBG_ERROR,
			 "%s: Memory areas are still registered with MMap",
			 __func__);

		PVR_TRACE("%s: Unregistering memory areas", __func__);
		list_for_each_entry_safe(psLinuxMemArea, psTmpMemArea,
					 &g_sMMapAreaList, sMMapItem) {
			eError = PVRMMapRemoveRegisteredArea(psLinuxMemArea);
			if (eError != PVRSRV_OK)
				PVR_DPF(PVR_DBG_ERROR,
				"%s: PVRMMapRemoveRegisteredArea failed (%d)",
					 __func__, eError);
			PVR_ASSERT(eError == PVRSRV_OK);

			LinuxMemAreaDeepFree(psLinuxMemArea);
		}
	}
	PVR_ASSERT(list_empty((&g_sMMapAreaList)));

	RemoveProcEntry("mmap");

	if (g_psMemmapCache) {
		kmem_cache_destroy(g_psMemmapCache);
		g_psMemmapCache = NULL;
	}
}
