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

#include <linux/version.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/highmem.h>
#include <linux/sched.h>
#include <linux/dma-mapping.h>

#include "img_defs.h"
#include "services.h"
#include "servicesint.h"
#include "syscommon.h"
#include "mutils.h"
#include "mm.h"
#include "pvrmmap.h"
#include "mmap.h"
#include "osfunc.h"
#include "pvr_debug.h"
#include "proc.h"
#include "mutex.h"
#include "lock.h"

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
enum DEBUG_MEM_ALLOC_TYPE {
	DEBUG_MEM_ALLOC_TYPE_KMALLOC,
	DEBUG_MEM_ALLOC_TYPE_VMALLOC,
	DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES,
	DEBUG_MEM_ALLOC_TYPE_IOREMAP,
	DEBUG_MEM_ALLOC_TYPE_IO,
	DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE,
	DEBUG_MEM_ALLOC_TYPE_COUNT
};

struct DEBUG_MEM_ALLOC_REC {
	enum DEBUG_MEM_ALLOC_TYPE eAllocType;
	void *pvKey;
	void *pvCpuVAddr;
	u32 ulCpuPAddr;
	void *pvPrivateData;
	u32 ui32Bytes;
	pid_t pid;
	char *pszFileName;
	u32 ui32Line;

	struct DEBUG_MEM_ALLOC_REC *psNext;
};

static struct DEBUG_MEM_ALLOC_REC *g_MemoryRecords;

static u32 g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_COUNT];
static u32 g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_COUNT];

static u32 g_SysRAMWaterMark;
static u32 g_SysRAMHighWaterMark;

static u32 g_IOMemWaterMark;
static u32 g_IOMemHighWaterMark;

static void DebugMemAllocRecordAdd(enum DEBUG_MEM_ALLOC_TYPE eAllocType,
				       void *pvKey, void *pvCpuVAddr,
				       u32 ulCpuPAddr, void *pvPrivateData,
				       u32 ui32Bytes, char *pszFileName,
				       u32 ui32Line);

static void DebugMemAllocRecordRemove(enum DEBUG_MEM_ALLOC_TYPE eAllocType,
					  void *pvKey, char *pszFileName,
					  u32 ui32Line);

static char *DebugMemAllocRecordTypeToString(
					enum DEBUG_MEM_ALLOC_TYPE eAllocType);

static off_t printMemoryRecords(char *buffer, size_t size, off_t off);
#endif

#if defined(DEBUG_LINUX_MEM_AREAS)
struct DEBUG_LINUX_MEM_AREA_REC {
	struct LinuxMemArea *psLinuxMemArea;
	u32 ui32Flags;
	pid_t pid;

	struct DEBUG_LINUX_MEM_AREA_REC *psNext;
};

#if defined(DEBUG_LINUX_MEM_AREAS) || defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
static struct mutex g_sDebugMutex;
#endif

static struct DEBUG_LINUX_MEM_AREA_REC *g_LinuxMemAreaRecords;
static u32 g_LinuxMemAreaCount;
static u32 g_LinuxMemAreaWaterMark;
static u32 g_LinuxMemAreaHighWaterMark;

static off_t printLinuxMemAreaRecords(char *buffer, size_t size, off_t off);
#endif

static struct kmem_cache *psLinuxMemAreaCache;


static struct LinuxMemArea *LinuxMemAreaStructAlloc(void);
static void LinuxMemAreaStructFree(struct LinuxMemArea *psLinuxMemArea);
#if defined(DEBUG_LINUX_MEM_AREAS)
static void DebugLinuxMemAreaRecordAdd(struct LinuxMemArea *psLinuxMemArea,
					   u32 ui32Flags);
static struct DEBUG_LINUX_MEM_AREA_REC *DebugLinuxMemAreaRecordFind(
					struct LinuxMemArea *psLinuxMemArea);
static void DebugLinuxMemAreaRecordRemove(struct LinuxMemArea *psLinuxMemArea);
#endif

enum PVRSRV_ERROR LinuxMMInit(void)
{
#if defined(DEBUG_LINUX_MEM_AREAS) || defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	mutex_init(&g_sDebugMutex);
#endif

#if defined(DEBUG_LINUX_MEM_AREAS)
	{
		int iStatus;
		iStatus =
		    CreateProcReadEntry("mem_areas", printLinuxMemAreaRecords);
		if (iStatus != 0)
			return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
#endif

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	{
		int iStatus;
		iStatus = CreateProcReadEntry("meminfo", printMemoryRecords);
		if (iStatus != 0)
			return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
#endif
	psLinuxMemAreaCache =
	    kmem_cache_create("img-mm", sizeof(struct LinuxMemArea), 0, 0,
				NULL);
	if (!psLinuxMemAreaCache) {
		PVR_DPF(PVR_DBG_ERROR, "%s: failed to allocate kmem_cache",
			 __func__);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	return PVRSRV_OK;
}

void LinuxMMCleanup(void)
{
#if defined(DEBUG_LINUX_MEM_AREAS)
	{
		struct DEBUG_LINUX_MEM_AREA_REC *psCurrentRecord =
		    g_LinuxMemAreaRecords, *psNextRecord;

		if (g_LinuxMemAreaCount)
			PVR_DPF(PVR_DBG_ERROR, "%s: BUG!: "
				"There are %d struct LinuxMemArea "
				"allocation unfreed (%ld bytes)",
				 __func__, g_LinuxMemAreaCount,
				 g_LinuxMemAreaWaterMark);

		while (psCurrentRecord) {
			struct LinuxMemArea *psLinuxMemArea;

			psNextRecord = psCurrentRecord->psNext;
			psLinuxMemArea = psCurrentRecord->psLinuxMemArea;
			PVR_DPF(PVR_DBG_ERROR, "%s: BUG!: "
				"Cleaning up Linux memory area (%p), "
				"type=%s, size=%ld bytes",
				 __func__, psCurrentRecord->psLinuxMemArea,
				 LinuxMemAreaTypeToString(psCurrentRecord->
							  psLinuxMemArea->
							  eAreaType),
				 psCurrentRecord->psLinuxMemArea->
				 ui32ByteSize);

			LinuxMemAreaDeepFree(psLinuxMemArea);

			psCurrentRecord = psNextRecord;
		}
		RemoveProcEntry("mem_areas");
	}
#endif

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	{
		struct DEBUG_MEM_ALLOC_REC *psCurrentRecord =
		    g_MemoryRecords, *psNextRecord;

		while (psCurrentRecord) {
			psNextRecord = psCurrentRecord->psNext;
			PVR_DPF(PVR_DBG_ERROR, "%s: BUG!: Cleaning up memory: "
				 "type=%s CpuVAddr=%p CpuPAddr=0x%08lx, "
				 "allocated @ file=%s,line=%d",
				 __func__,
				 DebugMemAllocRecordTypeToString
				 (psCurrentRecord->eAllocType),
				 psCurrentRecord->pvCpuVAddr,
				 psCurrentRecord->ulCpuPAddr,
				 psCurrentRecord->pszFileName,
				 psCurrentRecord->ui32Line);
			switch (psCurrentRecord->eAllocType) {
			case DEBUG_MEM_ALLOC_TYPE_KMALLOC:
				KFreeWrapper(psCurrentRecord->pvCpuVAddr);
				break;
			case DEBUG_MEM_ALLOC_TYPE_IOREMAP:
				IOUnmapWrapper((__force __iomem void *)
						psCurrentRecord->pvCpuVAddr);
				break;
			case DEBUG_MEM_ALLOC_TYPE_IO:

				DebugMemAllocRecordRemove
				    (DEBUG_MEM_ALLOC_TYPE_IO,
				     psCurrentRecord->pvKey, __FILE__,
				     __LINE__);
				break;
			case DEBUG_MEM_ALLOC_TYPE_VMALLOC:
				VFreeWrapper(psCurrentRecord->pvCpuVAddr);
				break;
			case DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES:

				DebugMemAllocRecordRemove
				    (DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES,
				     psCurrentRecord->pvKey, __FILE__,
				     __LINE__);
				break;
			case DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE:
				KMemCacheFreeWrapper(psCurrentRecord->
							     pvPrivateData,
						     psCurrentRecord->
							     pvCpuVAddr);
				break;
			default:
				PVR_ASSERT(0);
			}
			psCurrentRecord = psNextRecord;
		}
		RemoveProcEntry("meminfo");
	}
#endif

	if (psLinuxMemAreaCache) {
		kmem_cache_destroy(psLinuxMemAreaCache);
		psLinuxMemAreaCache = NULL;
	}
}

void *_KMallocWrapper(u32 ui32ByteSize, char *pszFileName,
			  u32 ui32Line)
{
	void *pvRet;
	pvRet = kmalloc(ui32ByteSize, GFP_KERNEL);
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	if (pvRet)
		DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_KMALLOC,
				       pvRet, pvRet, 0, NULL, ui32ByteSize,
				       pszFileName, ui32Line);
#endif
	return pvRet;
}

void _KFreeWrapper(void *pvCpuVAddr, char *pszFileName,
	      u32 ui32Line)
{
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_KMALLOC, pvCpuVAddr,
				  pszFileName, ui32Line);
#endif
	kfree(pvCpuVAddr);
}

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
static void DebugMemAllocRecordAdd(enum DEBUG_MEM_ALLOC_TYPE eAllocType,
				   void *pvKey, void *pvCpuVAddr,
				   u32 ulCpuPAddr, void *pvPrivateData,
				   u32 ui32Bytes, char *pszFileName,
				   u32 ui32Line)
{
	struct DEBUG_MEM_ALLOC_REC *psRecord;

	mutex_lock(&g_sDebugMutex);

	psRecord = kmalloc(sizeof(struct DEBUG_MEM_ALLOC_REC), GFP_KERNEL);

	psRecord->eAllocType = eAllocType;
	psRecord->pvKey = pvKey;
	psRecord->pvCpuVAddr = pvCpuVAddr;
	psRecord->ulCpuPAddr = ulCpuPAddr;
	psRecord->pvPrivateData = pvPrivateData;
	psRecord->pid = current->pid;
	psRecord->ui32Bytes = ui32Bytes;
	psRecord->pszFileName = pszFileName;
	psRecord->ui32Line = ui32Line;

	psRecord->psNext = g_MemoryRecords;
	g_MemoryRecords = psRecord;

	g_WaterMarkData[eAllocType] += ui32Bytes;
	if (g_WaterMarkData[eAllocType] > g_HighWaterMarkData[eAllocType])
		g_HighWaterMarkData[eAllocType] = g_WaterMarkData[eAllocType];

	if (eAllocType == DEBUG_MEM_ALLOC_TYPE_KMALLOC ||
	    eAllocType == DEBUG_MEM_ALLOC_TYPE_VMALLOC ||
	    eAllocType == DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES ||
	    eAllocType == DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE) {
		g_SysRAMWaterMark += ui32Bytes;
		if (g_SysRAMWaterMark > g_SysRAMHighWaterMark)
			g_SysRAMHighWaterMark = g_SysRAMWaterMark;
	} else if (eAllocType == DEBUG_MEM_ALLOC_TYPE_IOREMAP ||
		   eAllocType == DEBUG_MEM_ALLOC_TYPE_IO) {
		g_IOMemWaterMark += ui32Bytes;
		if (g_IOMemWaterMark > g_IOMemHighWaterMark)
			g_IOMemHighWaterMark = g_IOMemWaterMark;
	}

	mutex_unlock(&g_sDebugMutex);
}

static void DebugMemAllocRecordRemove(enum DEBUG_MEM_ALLOC_TYPE eAllocType,
				      void *pvKey, char *pszFileName,
				      u32 ui32Line)
{
	struct DEBUG_MEM_ALLOC_REC **ppsCurrentRecord;

	mutex_lock(&g_sDebugMutex);

	for (ppsCurrentRecord = &g_MemoryRecords; *ppsCurrentRecord;
	     ppsCurrentRecord = &((*ppsCurrentRecord)->psNext))
		if ((*ppsCurrentRecord)->eAllocType == eAllocType &&
		    (*ppsCurrentRecord)->pvKey == pvKey) {
			struct DEBUG_MEM_ALLOC_REC *psNextRecord;

			psNextRecord = (*ppsCurrentRecord)->psNext;
			g_WaterMarkData[eAllocType] -=
			    (*ppsCurrentRecord)->ui32Bytes;

			if (eAllocType == DEBUG_MEM_ALLOC_TYPE_KMALLOC ||
			    eAllocType == DEBUG_MEM_ALLOC_TYPE_VMALLOC ||
			    eAllocType == DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES ||
			    eAllocType == DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE) {
				g_SysRAMWaterMark -=
				    (*ppsCurrentRecord)->ui32Bytes;
			} else {
				if (eAllocType == DEBUG_MEM_ALLOC_TYPE_IOREMAP
				   || eAllocType == DEBUG_MEM_ALLOC_TYPE_IO)
					g_IOMemWaterMark -=
					    (*ppsCurrentRecord)->ui32Bytes;

			}

			kfree(*ppsCurrentRecord);
			*ppsCurrentRecord = psNextRecord;
			goto exit_unlock;
		}

	PVR_DPF(PVR_DBG_ERROR, "%s: couldn't find an entry for type=%s "
				"with pvKey=%p (called from %s, line %d\n",
		 __func__, DebugMemAllocRecordTypeToString(eAllocType), pvKey,
		 pszFileName, ui32Line);

exit_unlock:
	mutex_unlock(&g_sDebugMutex);
}

static char *DebugMemAllocRecordTypeToString(
		enum DEBUG_MEM_ALLOC_TYPE eAllocType)
{
	char *apszDebugMemoryRecordTypes[] = {
		"KMALLOC",
		"VMALLOC",
		"ALLOC_PAGES",
		"IOREMAP",
		"IO",
		"KMEM_CACHE_ALLOC"
	};
	return apszDebugMemoryRecordTypes[eAllocType];
}
#endif

void *_VMallocWrapper(u32 ui32Bytes, u32 ui32AllocFlags, char *pszFileName,
		      u32 ui32Line)
{
	pgprot_t PGProtFlags;
	void *pvRet;

	switch (ui32AllocFlags & PVRSRV_HAP_CACHETYPE_MASK) {
	case PVRSRV_HAP_CACHED:
		PGProtFlags = PAGE_KERNEL;
		break;
	case PVRSRV_HAP_WRITECOMBINE:
		PGProtFlags = PGPROT_WC(PAGE_KERNEL);
		break;
	case PVRSRV_HAP_UNCACHED:
		PGProtFlags = PGPROT_UC(PAGE_KERNEL);
		break;
	default:
		PVR_DPF(PVR_DBG_ERROR,
			 "VMAllocWrapper: unknown mapping flags=0x%08lx",
			 ui32AllocFlags);
		dump_stack();
		return NULL;
	}

	pvRet = __vmalloc(ui32Bytes, GFP_KERNEL | __GFP_HIGHMEM, PGProtFlags);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	if (pvRet)
		DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_VMALLOC,
				       pvRet, pvRet, 0, NULL,
				       PAGE_ALIGN(ui32Bytes),
				       pszFileName, ui32Line);
#endif

	return pvRet;
}

void _VFreeWrapper(void *pvCpuVAddr, char *pszFileName, u32 ui32Line)
{
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_VMALLOC, pvCpuVAddr,
				  pszFileName, ui32Line);
#endif
	vfree(pvCpuVAddr);
}

struct LinuxMemArea *NewVMallocLinuxMemArea(u32 ui32Bytes, u32 ui32AreaFlags)
{
	struct LinuxMemArea *psLinuxMemArea;
	void *pvCpuVAddr;

	psLinuxMemArea = LinuxMemAreaStructAlloc();
	if (!psLinuxMemArea)
		goto failed;

	pvCpuVAddr = VMallocWrapper(ui32Bytes, ui32AreaFlags);
	if (!pvCpuVAddr)
		goto failed;

	psLinuxMemArea->eAreaType = LINUX_MEM_AREA_VMALLOC;
	psLinuxMemArea->uData.sVmalloc.pvVmallocAddress = pvCpuVAddr;
	psLinuxMemArea->ui32ByteSize = ui32Bytes;
	psLinuxMemArea->ui32AreaFlags = ui32AreaFlags;
	psLinuxMemArea->bMMapRegistered = IMG_FALSE;
	INIT_LIST_HEAD(&psLinuxMemArea->sMMapOffsetStructList);

#if defined(DEBUG_LINUX_MEM_AREAS)
	DebugLinuxMemAreaRecordAdd(psLinuxMemArea, ui32AreaFlags);
#endif

	return psLinuxMemArea;

failed:
	PVR_DPF(PVR_DBG_ERROR, "%s: failed!", __func__);
	if (psLinuxMemArea)
		LinuxMemAreaStructFree(psLinuxMemArea);
	return NULL;
}

void FreeVMallocLinuxMemArea(struct LinuxMemArea *psLinuxMemArea)
{
	PVR_ASSERT(psLinuxMemArea);
	PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_VMALLOC);
	PVR_ASSERT(psLinuxMemArea->uData.sVmalloc.pvVmallocAddress);

#if defined(DEBUG_LINUX_MEM_AREAS)
	DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif


	PVR_DPF(PVR_DBG_MESSAGE, "%s: pvCpuVAddr: %p",
		 __func__,
		 psLinuxMemArea->uData.sVmalloc.pvVmallocAddress);
	VFreeWrapper(psLinuxMemArea->uData.sVmalloc.pvVmallocAddress);

	LinuxMemAreaStructFree(psLinuxMemArea);
}

void __iomem *_IORemapWrapper(struct IMG_CPU_PHYADDR BasePAddr,
			  u32 ui32Bytes, u32 ui32MappingFlags,
			  char *pszFileName, u32 ui32Line)
{
	void __iomem *pvIORemapCookie = NULL;

	switch (ui32MappingFlags & PVRSRV_HAP_CACHETYPE_MASK) {
	case PVRSRV_HAP_CACHED:
		pvIORemapCookie = IOREMAP(BasePAddr.uiAddr, ui32Bytes);
		break;
	case PVRSRV_HAP_WRITECOMBINE:
		pvIORemapCookie = IOREMAP_WC(BasePAddr.uiAddr, ui32Bytes);
		break;
	case PVRSRV_HAP_UNCACHED:
		pvIORemapCookie = IOREMAP_UC(BasePAddr.uiAddr, ui32Bytes);
		break;
	default:
		PVR_DPF(PVR_DBG_ERROR,
			 "IORemapWrapper: unknown mapping flags");
		return NULL;
	}

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	if (pvIORemapCookie)
		DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_IOREMAP,
				       (__force void *)pvIORemapCookie,
				       (__force void *)pvIORemapCookie,
				       BasePAddr.uiAddr,
				       NULL, ui32Bytes, pszFileName, ui32Line);
#endif

	return pvIORemapCookie;
}

void _IOUnmapWrapper(void __iomem *pvIORemapCookie, char *pszFileName,
		u32 ui32Line)
{
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_IOREMAP,
				  (__force void *)pvIORemapCookie,
				  pszFileName, ui32Line);
#endif
	iounmap(pvIORemapCookie);
}

struct LinuxMemArea *NewIORemapLinuxMemArea(struct IMG_CPU_PHYADDR BasePAddr,
					    u32 ui32Bytes, u32 ui32AreaFlags)
{
	struct LinuxMemArea *psLinuxMemArea;
	void __iomem *pvIORemapCookie;

	psLinuxMemArea = LinuxMemAreaStructAlloc();
	if (!psLinuxMemArea)
		return NULL;

	pvIORemapCookie = IORemapWrapper(BasePAddr, ui32Bytes, ui32AreaFlags);
	if (!pvIORemapCookie) {
		LinuxMemAreaStructFree(psLinuxMemArea);
		return NULL;
	}

	psLinuxMemArea->eAreaType = LINUX_MEM_AREA_IOREMAP;
	psLinuxMemArea->uData.sIORemap.pvIORemapCookie = pvIORemapCookie;
	psLinuxMemArea->uData.sIORemap.CPUPhysAddr = BasePAddr;
	psLinuxMemArea->ui32ByteSize = ui32Bytes;
	psLinuxMemArea->ui32AreaFlags = ui32AreaFlags;
	psLinuxMemArea->bMMapRegistered = IMG_FALSE;
	INIT_LIST_HEAD(&psLinuxMemArea->sMMapOffsetStructList);

#if defined(DEBUG_LINUX_MEM_AREAS)
	DebugLinuxMemAreaRecordAdd(psLinuxMemArea, ui32AreaFlags);
#endif

	return psLinuxMemArea;
}

void FreeIORemapLinuxMemArea(struct LinuxMemArea *psLinuxMemArea)
{
	PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_IOREMAP);

#if defined(DEBUG_LINUX_MEM_AREAS)
	DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif

	IOUnmapWrapper(psLinuxMemArea->uData.sIORemap.pvIORemapCookie);

	LinuxMemAreaStructFree(psLinuxMemArea);
}

static IMG_BOOL PagesAreContiguous(struct IMG_SYS_PHYADDR *psSysPhysAddr,
				   u32 ui32Bytes)
{
	u32 ui32;
	u32 ui32AddrChk;
	u32 ui32NumPages = RANGE_TO_PAGES(ui32Bytes);

	for (ui32 = 0, ui32AddrChk = psSysPhysAddr[0].uiAddr;
	     ui32 < ui32NumPages; ui32++, ui32AddrChk += PAGE_SIZE)
		if (psSysPhysAddr[ui32].uiAddr != ui32AddrChk)
			return IMG_FALSE;

	return IMG_TRUE;
}

struct LinuxMemArea *NewExternalKVLinuxMemArea(struct IMG_SYS_PHYADDR
					       *pBasePAddr, void *pvCPUVAddr,
					       u32 ui32Bytes,
					       IMG_BOOL bPhysContig,
					       u32 ui32AreaFlags)
{
	struct LinuxMemArea *psLinuxMemArea;

	psLinuxMemArea = LinuxMemAreaStructAlloc();
	if (!psLinuxMemArea)
		return NULL;

	psLinuxMemArea->eAreaType = LINUX_MEM_AREA_EXTERNAL_KV;
	psLinuxMemArea->uData.sExternalKV.pvExternalKV = pvCPUVAddr;
	psLinuxMemArea->uData.sExternalKV.bPhysContig = bPhysContig ||
				      PagesAreContiguous(pBasePAddr, ui32Bytes);

	if (psLinuxMemArea->uData.sExternalKV.bPhysContig)
		psLinuxMemArea->uData.sExternalKV.uPhysAddr.SysPhysAddr =
		    *pBasePAddr;
	else
		psLinuxMemArea->uData.sExternalKV.uPhysAddr.pSysPhysAddr =
		    pBasePAddr;
	psLinuxMemArea->ui32ByteSize = ui32Bytes;
	psLinuxMemArea->ui32AreaFlags = ui32AreaFlags;
	psLinuxMemArea->bMMapRegistered = IMG_FALSE;
	INIT_LIST_HEAD(&psLinuxMemArea->sMMapOffsetStructList);

#if defined(DEBUG_LINUX_MEM_AREAS)
	DebugLinuxMemAreaRecordAdd(psLinuxMemArea, ui32AreaFlags);
#endif

	return psLinuxMemArea;
}

void FreeExternalKVLinuxMemArea(struct LinuxMemArea *psLinuxMemArea)
{
	PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_EXTERNAL_KV);

#if defined(DEBUG_LINUX_MEM_AREAS)
	DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif

	LinuxMemAreaStructFree(psLinuxMemArea);
}

struct LinuxMemArea *NewIOLinuxMemArea(struct IMG_CPU_PHYADDR BasePAddr,
				u32 ui32Bytes, u32 ui32AreaFlags)
{
	struct LinuxMemArea *psLinuxMemArea = LinuxMemAreaStructAlloc();
	if (!psLinuxMemArea)
		return NULL;

	psLinuxMemArea->eAreaType = LINUX_MEM_AREA_IO;
	psLinuxMemArea->uData.sIO.CPUPhysAddr.uiAddr = BasePAddr.uiAddr;
	psLinuxMemArea->ui32ByteSize = ui32Bytes;
	psLinuxMemArea->ui32AreaFlags = ui32AreaFlags;
	psLinuxMemArea->bMMapRegistered = IMG_FALSE;
	INIT_LIST_HEAD(&psLinuxMemArea->sMMapOffsetStructList);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_IO,
			       (void *)BasePAddr.uiAddr, NULL,
			       BasePAddr.uiAddr, NULL, ui32Bytes, "unknown", 0);
#endif

#if defined(DEBUG_LINUX_MEM_AREAS)
	DebugLinuxMemAreaRecordAdd(psLinuxMemArea, ui32AreaFlags);
#endif

	return psLinuxMemArea;
}

void FreeIOLinuxMemArea(struct LinuxMemArea *psLinuxMemArea)
{
	PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_IO);

#if defined(DEBUG_LINUX_MEM_AREAS)
	DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_IO,
				  (void *)psLinuxMemArea->uData.sIO.
				  CPUPhysAddr.uiAddr, __FILE__, __LINE__);
#endif

	LinuxMemAreaStructFree(psLinuxMemArea);
}

struct LinuxMemArea *NewAllocPagesLinuxMemArea(u32 ui32Bytes,
					u32 ui32AreaFlags)
{
	struct LinuxMemArea *psLinuxMemArea;
	u32 ui32PageCount;
	struct page **pvPageList;
	void *hBlockPageList;
	s32 i;
	enum PVRSRV_ERROR eError;

	psLinuxMemArea = LinuxMemAreaStructAlloc();
	if (!psLinuxMemArea)
		goto failed_area_alloc;

	ui32PageCount = RANGE_TO_PAGES(ui32Bytes);
	eError = OSAllocMem(0, sizeof(*pvPageList) * ui32PageCount,
		       (void **)&pvPageList, &hBlockPageList);
	if (eError != PVRSRV_OK)
		goto failed_page_list_alloc;

	for (i = 0; i < ui32PageCount; i++) {
		pvPageList[i] = alloc_pages(GFP_KERNEL | __GFP_HIGHMEM, 0);
		if (!pvPageList[i])
			goto failed_alloc_pages;

	}

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES,
			       pvPageList, NULL, 0, NULL, PAGE_ALIGN(ui32Bytes),
			       "unknown", 0);
#endif

	psLinuxMemArea->eAreaType = LINUX_MEM_AREA_ALLOC_PAGES;
	psLinuxMemArea->uData.sPageList.pvPageList = pvPageList;
	psLinuxMemArea->uData.sPageList.hBlockPageList = hBlockPageList;
	psLinuxMemArea->ui32ByteSize = ui32Bytes;
	psLinuxMemArea->ui32AreaFlags = ui32AreaFlags;
	psLinuxMemArea->bMMapRegistered = IMG_FALSE;
	INIT_LIST_HEAD(&psLinuxMemArea->sMMapOffsetStructList);

#if defined(DEBUG_LINUX_MEM_AREAS)
	DebugLinuxMemAreaRecordAdd(psLinuxMemArea, ui32AreaFlags);
#endif

	return psLinuxMemArea;

failed_alloc_pages:
	for (i--; i >= 0; i--)
		__free_pages(pvPageList[i], 0);
	OSFreeMem(0, sizeof(*pvPageList) * ui32PageCount, pvPageList,
			hBlockPageList);
failed_page_list_alloc:
	LinuxMemAreaStructFree(psLinuxMemArea);
failed_area_alloc:
	PVR_DPF(PVR_DBG_ERROR, "%s: failed", __func__);

	return NULL;
}

void FreeAllocPagesLinuxMemArea(struct LinuxMemArea *psLinuxMemArea)
{
	u32 ui32PageCount;
	struct page **pvPageList;
	void *hBlockPageList;
	u32 i;

	PVR_ASSERT(psLinuxMemArea);
	PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_ALLOC_PAGES);

#if defined(DEBUG_LINUX_MEM_AREAS)
	DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif

	ui32PageCount = RANGE_TO_PAGES(psLinuxMemArea->ui32ByteSize);
	pvPageList = psLinuxMemArea->uData.sPageList.pvPageList;
	hBlockPageList = psLinuxMemArea->uData.sPageList.hBlockPageList;

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES, pvPageList,
				  __FILE__, __LINE__);
#endif

	for (i = 0; i < ui32PageCount; i++)
		__free_pages(pvPageList[i], 0);

	OSFreeMem(0, sizeof(*pvPageList) * ui32PageCount, pvPageList,
			hBlockPageList);

	LinuxMemAreaStructFree(psLinuxMemArea);
}

struct page *LinuxMemAreaOffsetToPage(struct LinuxMemArea *psLinuxMemArea,
				      u32 ui32ByteOffset)
{
	u32 ui32PageIndex;
	char *pui8Addr;

	switch (psLinuxMemArea->eAreaType) {
	case LINUX_MEM_AREA_ALLOC_PAGES:
		ui32PageIndex = PHYS_TO_PFN(ui32ByteOffset);
		return
		     psLinuxMemArea->uData.sPageList.pvPageList[ui32PageIndex];
		break;
	case LINUX_MEM_AREA_VMALLOC:
		pui8Addr = psLinuxMemArea->uData.sVmalloc.pvVmallocAddress;
		pui8Addr += ui32ByteOffset;
		return vmalloc_to_page(pui8Addr);
		break;
	case LINUX_MEM_AREA_SUB_ALLOC:
		return LinuxMemAreaOffsetToPage(psLinuxMemArea->
				     uData.sSubAlloc.psParentLinuxMemArea,
				     psLinuxMemArea->
						uData.sSubAlloc.ui32ByteOffset +
						ui32ByteOffset);
	default:
		PVR_DPF(PVR_DBG_ERROR, "%s: Unsupported request for "
			 "struct page from struct LinuxMemArea with type=%s",
			 LinuxMemAreaTypeToString(psLinuxMemArea->eAreaType));
		return NULL;
	}
}

void *_KMemCacheAllocWrapper(struct kmem_cache *psCache,
				 gfp_t Flags,
				 char *pszFileName, u32 ui32Line)
{
	void *pvRet;

	pvRet = kmem_cache_alloc(psCache, Flags);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE, pvRet, pvRet,
			       0, psCache, kmem_cache_size(psCache),
			       pszFileName, ui32Line);
#endif

	return pvRet;
}

void _KMemCacheFreeWrapper(struct kmem_cache *psCache, void *pvObject,
		      char *pszFileName, u32 ui32Line)
{
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE, pvObject,
				  pszFileName, ui32Line);
#endif

	kmem_cache_free(psCache, pvObject);
}

const char *KMemCacheNameWrapper(struct kmem_cache *psCache)
{

	return "";
}

struct LinuxMemArea *NewSubLinuxMemArea(struct LinuxMemArea
				*psParentLinuxMemArea, u32 ui32ByteOffset,
				u32 ui32Bytes)
{
	struct LinuxMemArea *psLinuxMemArea;

	PVR_ASSERT((ui32ByteOffset + ui32Bytes) <=
		   psParentLinuxMemArea->ui32ByteSize);

	psLinuxMemArea = LinuxMemAreaStructAlloc();
	if (!psLinuxMemArea)
		return NULL;

	psLinuxMemArea->eAreaType = LINUX_MEM_AREA_SUB_ALLOC;
	psLinuxMemArea->uData.sSubAlloc.psParentLinuxMemArea =
							psParentLinuxMemArea;
	psLinuxMemArea->uData.sSubAlloc.ui32ByteOffset = ui32ByteOffset;
	psLinuxMemArea->ui32ByteSize = ui32Bytes;
	psLinuxMemArea->ui32AreaFlags = psParentLinuxMemArea->ui32AreaFlags;
	psLinuxMemArea->bMMapRegistered = IMG_FALSE;
	INIT_LIST_HEAD(&psLinuxMemArea->sMMapOffsetStructList);

#if defined(DEBUG_LINUX_MEM_AREAS)
	{
		struct DEBUG_LINUX_MEM_AREA_REC *psParentRecord;
		psParentRecord =
		    DebugLinuxMemAreaRecordFind(psParentLinuxMemArea);
		DebugLinuxMemAreaRecordAdd(psLinuxMemArea,
					   psParentRecord->ui32Flags);
	}
#endif

	return psLinuxMemArea;
}

static void FreeSubLinuxMemArea(struct LinuxMemArea *psLinuxMemArea)
{
	PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_SUB_ALLOC);

#if defined(DEBUG_LINUX_MEM_AREAS)
	DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif

	LinuxMemAreaStructFree(psLinuxMemArea);
}

static struct LinuxMemArea *LinuxMemAreaStructAlloc(void)
{
	return KMemCacheAllocWrapper(psLinuxMemAreaCache, GFP_KERNEL);
}

static void LinuxMemAreaStructFree(struct LinuxMemArea *psLinuxMemArea)
{
	KMemCacheFreeWrapper(psLinuxMemAreaCache, psLinuxMemArea);

}

void LinuxMemAreaDeepFree(struct LinuxMemArea *psLinuxMemArea)
{
	switch (psLinuxMemArea->eAreaType) {
	case LINUX_MEM_AREA_VMALLOC:
		FreeVMallocLinuxMemArea(psLinuxMemArea);
		break;
	case LINUX_MEM_AREA_ALLOC_PAGES:
		FreeAllocPagesLinuxMemArea(psLinuxMemArea);
		break;
	case LINUX_MEM_AREA_IOREMAP:
		FreeIORemapLinuxMemArea(psLinuxMemArea);
		break;
	case LINUX_MEM_AREA_EXTERNAL_KV:
		FreeExternalKVLinuxMemArea(psLinuxMemArea);
		break;
	case LINUX_MEM_AREA_IO:
		FreeIOLinuxMemArea(psLinuxMemArea);
		break;
	case LINUX_MEM_AREA_SUB_ALLOC:
		FreeSubLinuxMemArea(psLinuxMemArea);
		break;
	default:
		PVR_DPF(PVR_DBG_ERROR, "%s: Unknown are type (%d)\n",
			 __func__, psLinuxMemArea->eAreaType);
	}
}

#if defined(DEBUG_LINUX_MEM_AREAS)
static void DebugLinuxMemAreaRecordAdd(struct LinuxMemArea *psLinuxMemArea,
				       u32 ui32Flags)
{
	struct DEBUG_LINUX_MEM_AREA_REC *psNewRecord;
	const char *pi8FlagsString;

	mutex_lock(&g_sDebugMutex);

	if (psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC) {
		g_LinuxMemAreaWaterMark += psLinuxMemArea->ui32ByteSize;
		if (g_LinuxMemAreaWaterMark > g_LinuxMemAreaHighWaterMark)
			g_LinuxMemAreaHighWaterMark = g_LinuxMemAreaWaterMark;
	}
	g_LinuxMemAreaCount++;

	psNewRecord = kmalloc(sizeof(struct DEBUG_LINUX_MEM_AREA_REC),
			      GFP_KERNEL);
	if (psNewRecord) {
		psNewRecord->psLinuxMemArea = psLinuxMemArea;
		psNewRecord->ui32Flags = ui32Flags;
		psNewRecord->pid = current->pid;
		psNewRecord->psNext = g_LinuxMemAreaRecords;
		g_LinuxMemAreaRecords = psNewRecord;
	} else {
		PVR_DPF(PVR_DBG_ERROR,
			 "%s: failed to allocate linux memory area record.",
			 __func__);
	}

	pi8FlagsString = HAPFlagsToString(ui32Flags);
	if (strstr(pi8FlagsString, "UNKNOWN"))
		PVR_DPF(PVR_DBG_ERROR, "%s: Unexpected flags "
			 "(0x%08lx) associated with psLinuxMemArea @ 0x%08lx",
			 __func__, ui32Flags, psLinuxMemArea);

	mutex_unlock(&g_sDebugMutex);
}

static struct DEBUG_LINUX_MEM_AREA_REC *DebugLinuxMemAreaRecordFind(
					   struct LinuxMemArea *psLinuxMemArea)
{
	struct DEBUG_LINUX_MEM_AREA_REC *psCurrentRecord;

	mutex_lock(&g_sDebugMutex);

	for (psCurrentRecord = g_LinuxMemAreaRecords;
	     psCurrentRecord; psCurrentRecord = psCurrentRecord->psNext)
		if (psCurrentRecord->psLinuxMemArea == psLinuxMemArea)
			goto exit_unlock;

exit_unlock:
	mutex_unlock(&g_sDebugMutex);

	return psCurrentRecord;
}

static void DebugLinuxMemAreaRecordRemove(struct LinuxMemArea *psLinuxMemArea)
{
	struct DEBUG_LINUX_MEM_AREA_REC **ppsCurrentRecord;

	mutex_lock(&g_sDebugMutex);

	if (psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC)
		g_LinuxMemAreaWaterMark -= psLinuxMemArea->ui32ByteSize;
	g_LinuxMemAreaCount--;

	for (ppsCurrentRecord = &g_LinuxMemAreaRecords;
	     *ppsCurrentRecord;
	     ppsCurrentRecord = &((*ppsCurrentRecord)->psNext))
		if ((*ppsCurrentRecord)->psLinuxMemArea == psLinuxMemArea) {
			struct DEBUG_LINUX_MEM_AREA_REC *psNextRecord;

			psNextRecord = (*ppsCurrentRecord)->psNext;
			kfree(*ppsCurrentRecord);
			*ppsCurrentRecord = psNextRecord;
			goto exit_unlock;
		}

	PVR_DPF(PVR_DBG_ERROR,
		 "%s: couldn't find an entry for psLinuxMemArea=%p\n", __func__,
		 psLinuxMemArea);

exit_unlock:
	mutex_unlock(&g_sDebugMutex);
}
#endif

void *LinuxMemAreaToCpuVAddr(struct LinuxMemArea *psLinuxMemArea)
{
	switch (psLinuxMemArea->eAreaType) {
	case LINUX_MEM_AREA_VMALLOC:
		return psLinuxMemArea->uData.sVmalloc.pvVmallocAddress;
	case LINUX_MEM_AREA_IOREMAP:
		return (void __force *)
			psLinuxMemArea->uData.sIORemap.pvIORemapCookie;
	case LINUX_MEM_AREA_EXTERNAL_KV:
		return psLinuxMemArea->uData.sExternalKV.pvExternalKV;
	case LINUX_MEM_AREA_SUB_ALLOC:
		{
			char *pAddr =
			    LinuxMemAreaToCpuVAddr(psLinuxMemArea->uData.
						   sSubAlloc.
							 psParentLinuxMemArea);
			if (!pAddr)
				return NULL;
			return pAddr +
			       psLinuxMemArea->uData.sSubAlloc.ui32ByteOffset;
		}
	default:
		return NULL;
	}
}

struct IMG_CPU_PHYADDR LinuxMemAreaToCpuPAddr(
					struct LinuxMemArea *psLinuxMemArea,
					u32 ui32ByteOffset)
{
	struct IMG_CPU_PHYADDR CpuPAddr;

	CpuPAddr.uiAddr = 0;

	switch (psLinuxMemArea->eAreaType) {
	case LINUX_MEM_AREA_IOREMAP:
		{
			CpuPAddr = psLinuxMemArea->uData.sIORemap.CPUPhysAddr;
			CpuPAddr.uiAddr += ui32ByteOffset;
			break;
		}
	case LINUX_MEM_AREA_EXTERNAL_KV:
		{
			if (psLinuxMemArea->uData.sExternalKV.bPhysContig) {
				CpuPAddr =
				    SysSysPAddrToCpuPAddr(
						 psLinuxMemArea->uData.
							  sExternalKV.uPhysAddr.
								  SysPhysAddr);
				CpuPAddr.uiAddr += ui32ByteOffset;
			} else {
				u32 ui32PageIndex =
				    PHYS_TO_PFN(ui32ByteOffset);
				struct IMG_SYS_PHYADDR SysPAddr =
				    psLinuxMemArea->uData.sExternalKV.uPhysAddr.
						    pSysPhysAddr[ui32PageIndex];

				CpuPAddr = SysSysPAddrToCpuPAddr(SysPAddr);
				CpuPAddr.uiAddr +=
				    ADDR_TO_PAGE_OFFSET(ui32ByteOffset);
			}
			break;
		}
	case LINUX_MEM_AREA_IO:
		{
			CpuPAddr = psLinuxMemArea->uData.sIO.CPUPhysAddr;
			CpuPAddr.uiAddr += ui32ByteOffset;
			break;
		}
	case LINUX_MEM_AREA_VMALLOC:
		{
			char *pCpuVAddr;
			pCpuVAddr =
			    (char *) psLinuxMemArea->uData.sVmalloc.
			    pvVmallocAddress;
			pCpuVAddr += ui32ByteOffset;
			CpuPAddr.uiAddr = VMallocToPhys(pCpuVAddr);
			break;
		}
	case LINUX_MEM_AREA_ALLOC_PAGES:
		{
			struct page *page;
			u32 ui32PageIndex = PHYS_TO_PFN(ui32ByteOffset);
			page =
			    psLinuxMemArea->uData.sPageList.
			    pvPageList[ui32PageIndex];
			CpuPAddr.uiAddr = page_to_phys(page);
			CpuPAddr.uiAddr += ADDR_TO_PAGE_OFFSET(ui32ByteOffset);
			break;
		}
	case LINUX_MEM_AREA_SUB_ALLOC:
		{
			CpuPAddr =
			    OSMemHandleToCpuPAddr(psLinuxMemArea->uData.
						  sSubAlloc.
						  psParentLinuxMemArea,
						  psLinuxMemArea->uData.
						  sSubAlloc.ui32ByteOffset +
						  ui32ByteOffset);
			break;
		}
	default:
		PVR_DPF(PVR_DBG_ERROR,
			 "%s: Unknown struct LinuxMemArea type (%d)\n",
			 __func__, psLinuxMemArea->eAreaType);
	}

	PVR_ASSERT(CpuPAddr.uiAddr);
	return CpuPAddr;
}

static void inv_cache_vmalloc(const struct LinuxMemArea *mem_area)
{
	struct page *pg;
	void *kaddr;
	size_t chunk;
	u32 pg_cnt;
	u32 pg_ofs;
	u32 vaddr, vaddr_end;

	extern void ___dma_single_dev_to_cpu(const void *, size_t,
			                enum dma_data_direction);

	vaddr = (u32)mem_area->uData.sVmalloc.pvVmallocAddress;
	vaddr_end = vaddr + mem_area->ui32ByteSize;
	pg_cnt = (PAGE_ALIGN(vaddr_end) - (vaddr & PAGE_MASK)) / PAGE_SIZE;

	while (pg_cnt--) {
		pg = pfn_to_page(VMallocToPhys((void *)vaddr) >> PAGE_SHIFT);
		kaddr = page_address(pg);
		pg_ofs = vaddr & ~PAGE_MASK;
		kaddr += pg_ofs;
		chunk = min_t(ssize_t, vaddr_end - vaddr, PAGE_SIZE - pg_ofs);
		___dma_single_dev_to_cpu(kaddr, chunk, DMA_FROM_DEVICE);
		vaddr += chunk;
	}
}

static void inv_cache_page_list(const struct LinuxMemArea *mem_area)
{
	u32 pg_cnt;
	struct page **pg_list;

	extern void ___dma_single_dev_to_cpu(const void *, size_t,
			                enum dma_data_direction);

	pg_cnt = RANGE_TO_PAGES(mem_area->ui32ByteSize);
	pg_list = mem_area->uData.sPageList.pvPageList;
	while (pg_cnt--)
		___dma_single_dev_to_cpu(page_address(*pg_list++), PAGE_SIZE,
				DMA_FROM_DEVICE);
}

void inv_cache_mem_area(const struct LinuxMemArea *mem_area)
{
	switch (mem_area->eAreaType) {
	case LINUX_MEM_AREA_VMALLOC:
		inv_cache_vmalloc(mem_area);
		break;
	case LINUX_MEM_AREA_ALLOC_PAGES:
		inv_cache_page_list(mem_area);
		break;
	case LINUX_MEM_AREA_IOREMAP:
	case LINUX_MEM_AREA_EXTERNAL_KV:
	case LINUX_MEM_AREA_IO:
	case LINUX_MEM_AREA_SUB_ALLOC:
		PVR_DPF(PVR_DBG_ERROR,
			"%s: Not implemented for type (%d)\n",
			__func__, mem_area->eAreaType);
		BUG();
	default:
		PVR_DPF(PVR_DBG_ERROR,
			"%s: Unknown LinuxMemArea type (%d)\n",
			__func__, mem_area->eAreaType);
		BUG();
	}
}

IMG_BOOL LinuxMemAreaPhysIsContig(struct LinuxMemArea *psLinuxMemArea)
{
	switch (psLinuxMemArea->eAreaType) {
	case LINUX_MEM_AREA_IOREMAP:
	case LINUX_MEM_AREA_IO:
		return IMG_TRUE;

	case LINUX_MEM_AREA_EXTERNAL_KV:
		return psLinuxMemArea->uData.sExternalKV.bPhysContig;

	case LINUX_MEM_AREA_VMALLOC:
	case LINUX_MEM_AREA_ALLOC_PAGES:
		return IMG_FALSE;

	case LINUX_MEM_AREA_SUB_ALLOC:
		return LinuxMemAreaPhysIsContig(psLinuxMemArea->uData.sSubAlloc.
						psParentLinuxMemArea);

	default:
		PVR_DPF(PVR_DBG_ERROR,
			 "%s: Unknown struct LinuxMemArea type (%d)\n",
			 __func__, psLinuxMemArea->eAreaType);
		break;
	}
	return IMG_FALSE;
}

const char *LinuxMemAreaTypeToString(enum LINUX_MEM_AREA_TYPE eMemAreaType)
{
	switch (eMemAreaType) {
	case LINUX_MEM_AREA_IOREMAP:
		return "LINUX_MEM_AREA_IOREMAP";
	case LINUX_MEM_AREA_EXTERNAL_KV:
		return "LINUX_MEM_AREA_EXTERNAL_KV";
	case LINUX_MEM_AREA_IO:
		return "LINUX_MEM_AREA_IO";
	case LINUX_MEM_AREA_VMALLOC:
		return "LINUX_MEM_AREA_VMALLOC";
	case LINUX_MEM_AREA_SUB_ALLOC:
		return "LINUX_MEM_AREA_SUB_ALLOC";
	case LINUX_MEM_AREA_ALLOC_PAGES:
		return "LINUX_MEM_AREA_ALLOC_PAGES";
	default:
		PVR_ASSERT(0);
	}

	return "";
}

#if defined(DEBUG_LINUX_MEM_AREAS)
static off_t printLinuxMemAreaRecords(char *buffer, size_t count, off_t off)
{
	struct DEBUG_LINUX_MEM_AREA_REC *psRecord;
	off_t Ret;

	mutex_lock(&g_sDebugMutex);

	if (!off) {
		if (count < 500) {
			Ret = 0;
			goto unlock_and_return;
		}
		Ret = printAppend(buffer, count, 0,
				  "Number of Linux Memory Areas: %u\n"
			  "At the current water mark these areas "
				  "correspond to %u bytes "
				  "(excluding SUB areas)\n"
			  "At the highest water mark these areas "
				  "corresponded to %u bytes "
				  "(excluding SUB areas)\n"
			  "\nDetails for all Linux Memory Areas:\n"
			  "%s %-24s %s %s %-8s %-5s %s\n",
			  g_LinuxMemAreaCount,
			  g_LinuxMemAreaWaterMark,
			  g_LinuxMemAreaHighWaterMark,
			  "psLinuxMemArea",
			  "LinuxMemType",
			  "CpuVAddr",
			  "CpuPAddr", "Bytes", "Pid", "Flags");
		goto unlock_and_return;
	}

	for (psRecord = g_LinuxMemAreaRecords; --off && psRecord;
	     psRecord = psRecord->psNext)
		;
	if (!psRecord) {
		Ret = END_OF_FILE;
		goto unlock_and_return;
	}

	if (count < 500) {
		Ret = 0;
		goto unlock_and_return;
	}

	Ret = printAppend(buffer, count, 0,
			  "%8p       %-24s %8p %08x %-8d %-5u %08x=(%s)\n",
			  psRecord->psLinuxMemArea,
			  LinuxMemAreaTypeToString(psRecord->psLinuxMemArea->
						   eAreaType),
			  LinuxMemAreaToCpuVAddr(psRecord->psLinuxMemArea),
			  LinuxMemAreaToCpuPAddr(psRecord->psLinuxMemArea,
						 0).uiAddr,
			  psRecord->psLinuxMemArea->ui32ByteSize, psRecord->pid,
			  psRecord->ui32Flags,
			  HAPFlagsToString(psRecord->ui32Flags)
	    );

unlock_and_return:
	mutex_unlock(&g_sDebugMutex);
	return Ret;
}
#endif

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
static off_t printMemoryRecords(char *buffer, size_t count, off_t off)
{
	struct DEBUG_MEM_ALLOC_REC *psRecord;
	off_t Ret;

	mutex_lock(&g_sDebugMutex);

	if (!off) {
		if (count < 1000) {
			Ret = 0;
			goto unlock_and_return;
		}

		Ret = printAppend(buffer, count, 0, "%-60s: %d bytes\n",
			  "Current Water Mark of bytes allocated via kmalloc",
				 g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMALLOC]);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
			"Highest Water Mark of bytes allocated via kmalloc",
				g_HighWaterMarkData
				[DEBUG_MEM_ALLOC_TYPE_KMALLOC]);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
			"Current Water Mark of bytes allocated via vmalloc",
			g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_VMALLOC]);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
			"Highest Water Mark of bytes allocated via vmalloc",
				g_HighWaterMarkData
				[DEBUG_MEM_ALLOC_TYPE_VMALLOC]);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
			"Current Water Mark of bytes allocated via alloc_pages",
				g_WaterMarkData
				[DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES]);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
			"Highest Water Mark of bytes allocated via alloc_pages",
				g_HighWaterMarkData
				[DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES]);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
			"Current Water Mark of bytes allocated via ioremap",
		g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_IOREMAP]);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
			"Highest Water Mark of bytes allocated via ioremap",
				g_HighWaterMarkData
				[DEBUG_MEM_ALLOC_TYPE_IOREMAP]);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
			"Current Water Mark of bytes reserved for "
			"\"IO\" memory areas",
		g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_IO]);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
			"Highest Water Mark of bytes allocated for "
			"\"IO\" memory areas",
		g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_IO]);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
			"Current Water Mark of bytes allocated via "
			"kmem_cache_alloc",
				g_WaterMarkData
				[DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE]);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
			"Highest Water Mark of bytes allocated via "
			"kmem_cache_alloc",
				g_HighWaterMarkData
				[DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE]);
		Ret = printAppend(buffer, count, Ret, "\n");

		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
		  "The Current Water Mark for memory allocated from system RAM",
		  g_SysRAMWaterMark);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
		  "The Highest Water Mark for memory allocated from system RAM",
		  g_SysRAMHighWaterMark);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
		  "The Current Water Mark for memory allocated from IO memory",
		  g_IOMemWaterMark);
		Ret = printAppend(buffer, count, Ret, "%-60s: %d bytes\n",
		  "The Highest Water Mark for memory allocated from IO memory",
		  g_IOMemHighWaterMark);

		Ret = printAppend(buffer, count, Ret, "\n");

		Ret = printAppend(buffer, count, Ret,
				"Details for all known allocations:\n"
				"%-16s %-8s %-8s %-10s %-5s %-10s %s\n", "Type",
				"CpuVAddr", "CpuPAddr", "Bytes", "PID",
				"PrivateData", "Filename:Line");


		goto unlock_and_return;
	}

	if (count < 1000) {
		Ret = 0;
		goto unlock_and_return;
	}

	for (psRecord = g_MemoryRecords; --off && psRecord;
	     psRecord = psRecord->psNext)
		;
	if (!psRecord) {
		Ret = END_OF_FILE;
		goto unlock_and_return;
	}

	if (psRecord->eAllocType != DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE)
		Ret = printAppend(buffer, count, 0,
			 "%-16s %-8p %08x %-10d %-5d %-10s %s:%d\n",
			 DebugMemAllocRecordTypeToString(psRecord->eAllocType),
			 psRecord->pvCpuVAddr, psRecord->ulCpuPAddr,
			 psRecord->ui32Bytes, psRecord->pid, "NULL",
			 psRecord->pszFileName, psRecord->ui32Line);
	else
		Ret = printAppend(buffer, count, 0,
			  "%-16s %-8p %08x %-10d %-5d %-10s %s:%d\n",
			  DebugMemAllocRecordTypeToString(psRecord->eAllocType),
			  psRecord->pvCpuVAddr, psRecord->ulCpuPAddr,
			  psRecord->ui32Bytes, psRecord->pid,
			  KMemCacheNameWrapper(psRecord->pvPrivateData),
			  psRecord->pszFileName, psRecord->ui32Line);

unlock_and_return:
	mutex_unlock(&g_sDebugMutex);
	return Ret;
}
#endif

#if defined(DEBUG_LINUX_MEM_AREAS) || defined(DEBUG_LINUX_MMAP_AREAS)
const char *HAPFlagsToString(u32 ui32Flags)
{
	static char szFlags[50];
	s32 i32Pos = 0;
	u32 ui32CacheTypeIndex, ui32MapTypeIndex;
	char *apszCacheTypes[] = {
		"UNCACHED",
		"CACHED",
		"WRITECOMBINE",
		"UNKNOWN"
	};
	char *apszMapType[] = {
		"KERNEL_ONLY",
		"SINGLE_PROCESS",
		"MULTI_PROCESS",
		"FROM_EXISTING_PROCESS",
		"NO_CPU_VIRTUAL",
		"UNKNOWN"
	};

	if (ui32Flags & PVRSRV_HAP_UNCACHED) {
		ui32CacheTypeIndex = 0;
	} else if (ui32Flags & PVRSRV_HAP_CACHED) {
		ui32CacheTypeIndex = 1;
	} else if (ui32Flags & PVRSRV_HAP_WRITECOMBINE) {
		ui32CacheTypeIndex = 2;
	} else {
		ui32CacheTypeIndex = 3;
		PVR_DPF(PVR_DBG_ERROR, "%s: unknown cache type (%u)",
			 __func__, (ui32Flags & PVRSRV_HAP_CACHETYPE_MASK));
	}

	if (ui32Flags & PVRSRV_HAP_KERNEL_ONLY) {
		ui32MapTypeIndex = 0;
	} else if (ui32Flags & PVRSRV_HAP_SINGLE_PROCESS) {
		ui32MapTypeIndex = 1;
	} else if (ui32Flags & PVRSRV_HAP_MULTI_PROCESS) {
		ui32MapTypeIndex = 2;
	} else if (ui32Flags & PVRSRV_HAP_FROM_EXISTING_PROCESS) {
		ui32MapTypeIndex = 3;
	} else if (ui32Flags & PVRSRV_HAP_NO_CPU_VIRTUAL) {
		ui32MapTypeIndex = 4;
	} else {
		ui32MapTypeIndex = 5;
		PVR_DPF(PVR_DBG_ERROR, "%s: unknown map type (%u)",
			 __func__, (ui32Flags & PVRSRV_HAP_MAPTYPE_MASK));
	}

	i32Pos = sprintf(szFlags, "%s|", apszCacheTypes[ui32CacheTypeIndex]);
	if (i32Pos <= 0) {
		PVR_DPF(PVR_DBG_ERROR,
			 "%s: sprintf for cache type %u failed (%d)", __func__,
			 ui32CacheTypeIndex, i32Pos);
		szFlags[0] = 0;
	} else {
		sprintf(szFlags + i32Pos, "%s", apszMapType[ui32MapTypeIndex]);
	}

	return szFlags;
}
#endif
