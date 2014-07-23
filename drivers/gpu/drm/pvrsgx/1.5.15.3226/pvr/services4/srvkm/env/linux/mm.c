/*************************************************************************/ /*!
@Title          Misc memory management utility functions for Linux
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#include <linux/version.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <asm/io.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
#include <linux/wrapper.h>
#endif
#include <linux/slab.h>
#include <linux/highmem.h>
#include <linux/sched.h>

#include "img_defs.h"
#include "services.h"
#include "servicesint.h"
#include "sysconfig.h"
#include "mutils.h"
#include "mm.h"
#include "pvrmmap.h"
#include "mmap.h"
#include "osfunc.h"
#include "pvr_debug.h"
#include "proc.h"
#include "mutex.h"
#include "lock.h"

#if defined(DEBUG_LINUX_MEM_AREAS) || defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	#include "lists.h"
#endif

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
typedef enum {
    DEBUG_MEM_ALLOC_TYPE_KMALLOC,
    DEBUG_MEM_ALLOC_TYPE_VMALLOC,
    DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES,
    DEBUG_MEM_ALLOC_TYPE_IOREMAP,
    DEBUG_MEM_ALLOC_TYPE_IO,
    DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE,
    DEBUG_MEM_ALLOC_TYPE_COUNT
}DEBUG_MEM_ALLOC_TYPE;

typedef struct _DEBUG_MEM_ALLOC_REC
{
    DEBUG_MEM_ALLOC_TYPE    eAllocType;
	IMG_VOID				*pvKey;
    IMG_VOID                *pvCpuVAddr;
    IMG_UINT32              ulCpuPAddr;
    IMG_VOID                *pvPrivateData;
	IMG_UINT32				ui32Bytes;
	pid_t					pid;
    IMG_CHAR                *pszFileName;
    IMG_UINT32              ui32Line;

    struct _DEBUG_MEM_ALLOC_REC   *psNext;
	struct _DEBUG_MEM_ALLOC_REC   **ppsThis;
}DEBUG_MEM_ALLOC_REC;

static IMPLEMENT_LIST_ANY_VA_2(DEBUG_MEM_ALLOC_REC, IMG_BOOL, IMG_FALSE)
static IMPLEMENT_LIST_ANY_VA(DEBUG_MEM_ALLOC_REC)
static IMPLEMENT_LIST_FOR_EACH(DEBUG_MEM_ALLOC_REC)
static IMPLEMENT_LIST_INSERT(DEBUG_MEM_ALLOC_REC)
static IMPLEMENT_LIST_REMOVE(DEBUG_MEM_ALLOC_REC)


static DEBUG_MEM_ALLOC_REC *g_MemoryRecords;

static IMG_UINT32 g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_COUNT];
static IMG_UINT32 g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_COUNT];

static IMG_UINT32 g_SysRAMWaterMark;
static IMG_UINT32 g_SysRAMHighWaterMark;

static IMG_UINT32 g_IOMemWaterMark;
static IMG_UINT32 g_IOMemHighWaterMark;

static IMG_VOID DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE eAllocType,
                                       IMG_VOID *pvKey,
                                       IMG_VOID *pvCpuVAddr,
                                       IMG_UINT32 ulCpuPAddr,
                                       IMG_VOID *pvPrivateData,
                                       IMG_UINT32 ui32Bytes,
                                       IMG_CHAR *pszFileName,
                                       IMG_UINT32 ui32Line);

static IMG_VOID DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE eAllocType, IMG_VOID *pvKey, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line);

static IMG_CHAR *DebugMemAllocRecordTypeToString(DEBUG_MEM_ALLOC_TYPE eAllocType);


#ifdef PVR_PROC_USE_SEQ_FILE
static struct proc_dir_entry *g_SeqFileMemoryRecords =0;
static void* ProcSeqNextMemoryRecords(struct seq_file *sfile,void* el,loff_t off);
static void ProcSeqShowMemoryRecords(struct seq_file *sfile,void* el);
static void* ProcSeqOff2ElementMemoryRecords(struct seq_file * sfile, loff_t off);

#else
static off_t printMemoryRecords(IMG_CHAR * buffer, size_t size, off_t off);
#endif

#endif


#if defined(DEBUG_LINUX_MEM_AREAS)
typedef struct _DEBUG_LINUX_MEM_AREA_REC
{
	LinuxMemArea                *psLinuxMemArea;
    IMG_UINT32                  ui32Flags;
	pid_t					    pid;

	struct _DEBUG_LINUX_MEM_AREA_REC  *psNext;
	struct _DEBUG_LINUX_MEM_AREA_REC  **ppsThis;
}DEBUG_LINUX_MEM_AREA_REC;


static IMPLEMENT_LIST_ANY_VA(DEBUG_LINUX_MEM_AREA_REC)
static IMPLEMENT_LIST_FOR_EACH(DEBUG_LINUX_MEM_AREA_REC)
static IMPLEMENT_LIST_INSERT(DEBUG_LINUX_MEM_AREA_REC)
static IMPLEMENT_LIST_REMOVE(DEBUG_LINUX_MEM_AREA_REC)



#if defined(DEBUG_LINUX_MEM_AREAS) || defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
static struct mutex g_sDebugMutex;
#endif

static DEBUG_LINUX_MEM_AREA_REC *g_LinuxMemAreaRecords;
static IMG_UINT32 g_LinuxMemAreaCount;
static IMG_UINT32 g_LinuxMemAreaWaterMark;
static IMG_UINT32 g_LinuxMemAreaHighWaterMark;


#ifdef PVR_PROC_USE_SEQ_FILE
static struct proc_dir_entry *g_SeqFileMemArea=0;

static void* ProcSeqNextMemArea(struct seq_file *sfile,void* el,loff_t off);
static void ProcSeqShowMemArea(struct seq_file *sfile,void* el);
static void* ProcSeqOff2ElementMemArea(struct seq_file *sfile, loff_t off);

#else
static off_t printLinuxMemAreaRecords(IMG_CHAR * buffer, size_t size, off_t off);
#endif

#endif

#ifdef PVR_PROC_USE_SEQ_FILE
#if (defined(DEBUG_LINUX_MEM_AREAS) || defined(DEBUG_LINUX_MEMORY_ALLOCATIONS))
static void ProcSeqStartstopDebugMutex(struct seq_file *sfile,IMG_BOOL start);
#endif
#endif

static LinuxKMemCache *psLinuxMemAreaCache;


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15))
static IMG_VOID ReservePages(IMG_VOID *pvAddress, IMG_UINT32 ui32Length);
static IMG_VOID UnreservePages(IMG_VOID *pvAddress, IMG_UINT32 ui32Length);
#endif

static LinuxMemArea *LinuxMemAreaStructAlloc(IMG_VOID);
static IMG_VOID LinuxMemAreaStructFree(LinuxMemArea *psLinuxMemArea);
#if defined(DEBUG_LINUX_MEM_AREAS)
static IMG_VOID DebugLinuxMemAreaRecordAdd(LinuxMemArea *psLinuxMemArea, IMG_UINT32 ui32Flags);
static DEBUG_LINUX_MEM_AREA_REC *DebugLinuxMemAreaRecordFind(LinuxMemArea *psLinuxMemArea);
static IMG_VOID DebugLinuxMemAreaRecordRemove(LinuxMemArea *psLinuxMemArea);
#endif

PVRSRV_ERROR
LinuxMMInit(IMG_VOID)
{
#if defined(DEBUG_LINUX_MEM_AREAS) || defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
	mutex_init(&g_sDebugMutex);
#endif

#if defined(DEBUG_LINUX_MEM_AREAS)
    {
        IMG_INT iStatus;
#ifdef PVR_PROC_USE_SEQ_FILE
		g_SeqFileMemArea = CreateProcReadEntrySeq(
									"mem_areas",
									NULL,
									ProcSeqNextMemArea,
									ProcSeqShowMemArea,
									ProcSeqOff2ElementMemArea,
									ProcSeqStartstopDebugMutex
								   );
		iStatus = !g_SeqFileMemArea ? -1 : 0;
#else
   iStatus = CreateProcReadEntry("mem_areas", printLinuxMemAreaRecords);
#endif
        if(iStatus!=0)
        {
            return PVRSRV_ERROR_OUT_OF_MEMORY;
        }
    }
#endif


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    {
        IMG_INT iStatus;
#ifdef PVR_PROC_USE_SEQ_FILE
		g_SeqFileMemoryRecords =CreateProcReadEntrySeq(
									"meminfo",
									NULL,
									ProcSeqNextMemoryRecords,
									ProcSeqShowMemoryRecords,
									ProcSeqOff2ElementMemoryRecords,
									ProcSeqStartstopDebugMutex
								   );

		iStatus = !g_SeqFileMemoryRecords ? -1 : 0;
#else
        iStatus = CreateProcReadEntry("meminfo", printMemoryRecords);
#endif
        if(iStatus!=0)
        {
            return PVRSRV_ERROR_OUT_OF_MEMORY;
        }
    }
#endif

    psLinuxMemAreaCache = KMemCacheCreateWrapper("img-mm", sizeof(LinuxMemArea), 0, 0);
    if(!psLinuxMemAreaCache)
    {
        PVR_DPF((PVR_DBG_ERROR,"%s: failed to allocate kmem_cache", __FUNCTION__));
        return PVRSRV_ERROR_OUT_OF_MEMORY;
    }

    return PVRSRV_OK;
}

#if defined(DEBUG_LINUX_MEM_AREAS)
IMG_VOID LinuxMMCleanup_MemAreas_ForEachCb(DEBUG_LINUX_MEM_AREA_REC *psCurrentRecord)
{
	LinuxMemArea *psLinuxMemArea;

	psLinuxMemArea = psCurrentRecord->psLinuxMemArea;
	PVR_DPF((PVR_DBG_ERROR, "%s: BUG!: Cleaning up Linux memory area (%p), type=%s, size=%ld bytes",
				__FUNCTION__,
				psCurrentRecord->psLinuxMemArea,
				LinuxMemAreaTypeToString(psCurrentRecord->psLinuxMemArea->eAreaType),
				psCurrentRecord->psLinuxMemArea->ui32ByteSize));

	LinuxMemAreaDeepFree(psLinuxMemArea);
}
#endif

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
IMG_VOID LinuxMMCleanup_MemRecords_ForEachVa(DEBUG_MEM_ALLOC_REC *psCurrentRecord)

{

	PVR_DPF((PVR_DBG_ERROR, "%s: BUG!: Cleaning up memory: "
							"type=%s "
							"CpuVAddr=%p "
							"CpuPAddr=0x%08lx, "
							"allocated @ file=%s,line=%d",
			__FUNCTION__,
			DebugMemAllocRecordTypeToString(psCurrentRecord->eAllocType),
			psCurrentRecord->pvCpuVAddr,
			psCurrentRecord->ulCpuPAddr,
			psCurrentRecord->pszFileName,
			psCurrentRecord->ui32Line));
	switch(psCurrentRecord->eAllocType)
	{
		case DEBUG_MEM_ALLOC_TYPE_KMALLOC:
			KFreeWrapper(psCurrentRecord->pvCpuVAddr);
			break;
		case DEBUG_MEM_ALLOC_TYPE_IOREMAP:
			IOUnmapWrapper(psCurrentRecord->pvCpuVAddr);
			break;
		case DEBUG_MEM_ALLOC_TYPE_IO:

			DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_IO, psCurrentRecord->pvKey, __FILE__, __LINE__);
			break;
		case DEBUG_MEM_ALLOC_TYPE_VMALLOC:
			VFreeWrapper(psCurrentRecord->pvCpuVAddr);
			break;
		case DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES:

			DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES, psCurrentRecord->pvKey, __FILE__, __LINE__);
			break;
		case DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE:
			KMemCacheFreeWrapper(psCurrentRecord->pvPrivateData, psCurrentRecord->pvCpuVAddr);
			break;
		default:
			PVR_ASSERT(0);
	}
}
#endif


IMG_VOID
LinuxMMCleanup(IMG_VOID)
{

#if defined(DEBUG_LINUX_MEM_AREAS)
    {
        if(g_LinuxMemAreaCount)
        {
            PVR_DPF((PVR_DBG_ERROR, "%s: BUG!: There are %d LinuxMemArea allocation unfreed (%ld bytes)",
                    __FUNCTION__, g_LinuxMemAreaCount, g_LinuxMemAreaWaterMark));
        }

		List_DEBUG_LINUX_MEM_AREA_REC_ForEach(g_LinuxMemAreaRecords,
											LinuxMMCleanup_MemAreas_ForEachCb);

#ifdef PVR_PROC_USE_SEQ_FILE
		RemoveProcEntrySeq( g_SeqFileMemArea );
#else
        RemoveProcEntry("mem_areas");
#endif
    }
#endif


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    {


		List_DEBUG_MEM_ALLOC_REC_ForEach(g_MemoryRecords,
										LinuxMMCleanup_MemRecords_ForEachVa);

#ifdef PVR_PROC_USE_SEQ_FILE
		RemoveProcEntrySeq( g_SeqFileMemoryRecords );
#else
        RemoveProcEntry("meminfo");
#endif

    }
#endif

    if(psLinuxMemAreaCache)
    {
        KMemCacheDestroyWrapper(psLinuxMemAreaCache);
        psLinuxMemAreaCache=NULL;
    }
}


IMG_VOID *
_KMallocWrapper(IMG_UINT32 ui32ByteSize, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line)
{
    IMG_VOID *pvRet;
    pvRet = kmalloc(ui32ByteSize, GFP_KERNEL);
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    if (pvRet)
    {
        DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_KMALLOC,
                               pvRet,
                               pvRet,
                               0,
                               NULL,
                               ui32ByteSize,
                               pszFileName,
                               ui32Line
                              );
    }
#else
    PVR_UNREFERENCED_PARAMETER(pszFileName);
    PVR_UNREFERENCED_PARAMETER(ui32Line);
#endif
    return pvRet;
}


IMG_VOID
_KFreeWrapper(IMG_VOID *pvCpuVAddr, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line)
{
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_KMALLOC, pvCpuVAddr, pszFileName,  ui32Line);
#else
    PVR_UNREFERENCED_PARAMETER(pszFileName);
    PVR_UNREFERENCED_PARAMETER(ui32Line);
#endif
    kfree(pvCpuVAddr);
}


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
static IMG_VOID
DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE eAllocType,
                       IMG_VOID *pvKey,
                       IMG_VOID *pvCpuVAddr,
                       IMG_UINT32 ulCpuPAddr,
                       IMG_VOID *pvPrivateData,
                       IMG_UINT32 ui32Bytes,
                       IMG_CHAR *pszFileName,
                       IMG_UINT32 ui32Line)
{
    DEBUG_MEM_ALLOC_REC *psRecord;

    mutex_lock(&g_sDebugMutex);

    psRecord = kmalloc(sizeof(DEBUG_MEM_ALLOC_REC), GFP_KERNEL);

    psRecord->eAllocType = eAllocType;
    psRecord->pvKey = pvKey;
    psRecord->pvCpuVAddr = pvCpuVAddr;
    psRecord->ulCpuPAddr = ulCpuPAddr;
    psRecord->pvPrivateData = pvPrivateData;
    psRecord->pid = current->pid;
    psRecord->ui32Bytes = ui32Bytes;
    psRecord->pszFileName = pszFileName;
    psRecord->ui32Line = ui32Line;
    
	List_DEBUG_MEM_ALLOC_REC_Insert(&g_MemoryRecords, psRecord);

    g_WaterMarkData[eAllocType] += ui32Bytes;
    if(g_WaterMarkData[eAllocType] > g_HighWaterMarkData[eAllocType])
    {
        g_HighWaterMarkData[eAllocType] = g_WaterMarkData[eAllocType];
    }

    if(eAllocType == DEBUG_MEM_ALLOC_TYPE_KMALLOC
       || eAllocType == DEBUG_MEM_ALLOC_TYPE_VMALLOC
       || eAllocType == DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES
       || eAllocType == DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE)
    {
        g_SysRAMWaterMark += ui32Bytes;
        if(g_SysRAMWaterMark > g_SysRAMHighWaterMark)
        {
            g_SysRAMHighWaterMark = g_SysRAMWaterMark;
        }
    }
    else if (eAllocType == DEBUG_MEM_ALLOC_TYPE_IOREMAP
            || eAllocType == DEBUG_MEM_ALLOC_TYPE_IO)
    {
        g_IOMemWaterMark += ui32Bytes;
        if (g_IOMemWaterMark > g_IOMemHighWaterMark)
        {
            g_IOMemHighWaterMark = g_IOMemWaterMark;
        }
    }

    mutex_unlock(&g_sDebugMutex);
}


IMG_BOOL DebugMemAllocRecordRemove_AnyVaCb(DEBUG_MEM_ALLOC_REC *psCurrentRecord, va_list va)
{
	DEBUG_MEM_ALLOC_TYPE eAllocType;
	IMG_VOID *pvKey;
	
	eAllocType = va_arg(va, DEBUG_MEM_ALLOC_TYPE);
	pvKey = va_arg(va, IMG_VOID*);

	if (psCurrentRecord->eAllocType == eAllocType
		&& psCurrentRecord->pvKey == pvKey)
	{
		eAllocType = psCurrentRecord->eAllocType;
		g_WaterMarkData[eAllocType] -= psCurrentRecord->ui32Bytes;
		
		if (eAllocType == DEBUG_MEM_ALLOC_TYPE_KMALLOC
		   || eAllocType == DEBUG_MEM_ALLOC_TYPE_VMALLOC
		   || eAllocType == DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES
		   || eAllocType == DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE)
		{
			g_SysRAMWaterMark -= psCurrentRecord->ui32Bytes;
		}
		else if (eAllocType == DEBUG_MEM_ALLOC_TYPE_IOREMAP
				|| eAllocType == DEBUG_MEM_ALLOC_TYPE_IO)
		{
			g_IOMemWaterMark -= psCurrentRecord->ui32Bytes;
		}
		
		List_DEBUG_MEM_ALLOC_REC_Remove(psCurrentRecord);
		kfree(psCurrentRecord);

		return IMG_TRUE;
	}
	else
	{
		return IMG_FALSE;
	}
}


static IMG_VOID
DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE eAllocType, IMG_VOID *pvKey, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line)
{
    mutex_lock(&g_sDebugMutex);


	if(!List_DEBUG_MEM_ALLOC_REC_IMG_BOOL_Any_va(g_MemoryRecords,
												DebugMemAllocRecordRemove_AnyVaCb,
												eAllocType,
												pvKey))
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: couldn't find an entry for type=%s with pvKey=%p (called from %s, line %d\n",
		__FUNCTION__, DebugMemAllocRecordTypeToString(eAllocType), pvKey,
		pszFileName, ui32Line));
	}

    mutex_unlock(&g_sDebugMutex);
}


static IMG_CHAR *
DebugMemAllocRecordTypeToString(DEBUG_MEM_ALLOC_TYPE eAllocType)
{
    IMG_CHAR *apszDebugMemoryRecordTypes[] = {
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



IMG_VOID *
_VMallocWrapper(IMG_UINT32 ui32Bytes,
                IMG_UINT32 ui32AllocFlags,
                IMG_CHAR *pszFileName,
                IMG_UINT32 ui32Line)
{
    pgprot_t PGProtFlags;
    IMG_VOID *pvRet;

    switch(ui32AllocFlags & PVRSRV_HAP_CACHETYPE_MASK)
    {
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
            PVR_DPF((PVR_DBG_ERROR,
                     "VMAllocWrapper: unknown mapping flags=0x%08lx",
                     ui32AllocFlags));
            dump_stack();
            return NULL;
    }

	/* Allocate virtually contiguous pages */
    pvRet = __vmalloc(ui32Bytes, GFP_KERNEL | __GFP_HIGHMEM, PGProtFlags);
    
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    if (pvRet)
    {
        DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_VMALLOC,
                               pvRet,
                               pvRet,
                               0,
                               NULL,
                               PAGE_ALIGN(ui32Bytes),
                               pszFileName,
                               ui32Line
                              );
    }
#else
    PVR_UNREFERENCED_PARAMETER(pszFileName);
    PVR_UNREFERENCED_PARAMETER(ui32Line);
#endif

    return pvRet;
}


IMG_VOID
_VFreeWrapper(IMG_VOID *pvCpuVAddr, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line)
{
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_VMALLOC, pvCpuVAddr, pszFileName, ui32Line);
#else
    PVR_UNREFERENCED_PARAMETER(pszFileName);
    PVR_UNREFERENCED_PARAMETER(ui32Line);
#endif
    vfree(pvCpuVAddr);
}


LinuxMemArea *
NewVMallocLinuxMemArea(IMG_UINT32 ui32Bytes, IMG_UINT32 ui32AreaFlags)
{
    LinuxMemArea *psLinuxMemArea;
    IMG_VOID *pvCpuVAddr;

    psLinuxMemArea = LinuxMemAreaStructAlloc();
    if (!psLinuxMemArea)
    {
        goto failed;
    }

    pvCpuVAddr = VMallocWrapper(ui32Bytes, ui32AreaFlags);
    if (!pvCpuVAddr)
    {
        goto failed;
    }
/* PG_reserved was deprecated in linux-2.6.15 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15))
    /* Reserve those pages to allow them to be re-mapped to user space */
    ReservePages(pvCpuVAddr, ui32Bytes);
#endif

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
    PVR_DPF((PVR_DBG_ERROR, "%s: failed!", __FUNCTION__));
    if(psLinuxMemArea)
        LinuxMemAreaStructFree(psLinuxMemArea);
    return NULL;
}


IMG_VOID
FreeVMallocLinuxMemArea(LinuxMemArea *psLinuxMemArea)
{
    PVR_ASSERT(psLinuxMemArea);
    PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_VMALLOC);
    PVR_ASSERT(psLinuxMemArea->uData.sVmalloc.pvVmallocAddress);

#if defined(DEBUG_LINUX_MEM_AREAS)
    DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15))
	UnreservePages(psLinuxMemArea->uData.sVmalloc.pvVmallocAddress,
                    psLinuxMemArea->ui32ByteSize);
#endif

    PVR_DPF((PVR_DBG_MESSAGE,"%s: pvCpuVAddr: %p",
             __FUNCTION__, psLinuxMemArea->uData.sVmalloc.pvVmallocAddress));
    VFreeWrapper(psLinuxMemArea->uData.sVmalloc.pvVmallocAddress);

    LinuxMemAreaStructFree(psLinuxMemArea);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15))
static IMG_VOID
ReservePages(IMG_VOID *pvAddress, IMG_UINT32 ui32Length)
{
	IMG_VOID *pvPage;
	IMG_VOID *pvEnd = pvAddress + ui32Length;

	for(pvPage = pvAddress; pvPage < pvEnd;  pvPage += PAGE_SIZE)
	{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
		SetPageReserved(vmalloc_to_page(pvPage));
#else
		mem_map_reserve(vmalloc_to_page(pvPage));
#endif
	}
}


/* Un-reserve pages of memory in order that they can be freed. */
static IMG_VOID
UnreservePages(IMG_VOID *pvAddress, IMG_UINT32 ui32Length)
{
	IMG_VOID *pvPage;
	IMG_VOID *pvEnd = pvAddress + ui32Length;

	for(pvPage = pvAddress; pvPage < pvEnd;  pvPage += PAGE_SIZE)
	{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
		ClearPageReserved(vmalloc_to_page(pvPage));
#else
		mem_map_unreserve(vmalloc_to_page(pvPage));
#endif
	}
}
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)) */


IMG_VOID *
_IORemapWrapper(IMG_CPU_PHYADDR BasePAddr,
               IMG_UINT32 ui32Bytes,
               IMG_UINT32 ui32MappingFlags,
               IMG_CHAR *pszFileName,
               IMG_UINT32 ui32Line)
{
    IMG_VOID *pvIORemapCookie;
    
    switch (ui32MappingFlags & PVRSRV_HAP_CACHETYPE_MASK)
    {
        case PVRSRV_HAP_CACHED:
	    pvIORemapCookie = (IMG_VOID *)IOREMAP(BasePAddr.uiAddr, ui32Bytes);
            break;
        case PVRSRV_HAP_WRITECOMBINE:
	    pvIORemapCookie = (IMG_VOID *)IOREMAP_WC(BasePAddr.uiAddr, ui32Bytes);
            break;
        case PVRSRV_HAP_UNCACHED:
            pvIORemapCookie = (IMG_VOID *)IOREMAP_UC(BasePAddr.uiAddr, ui32Bytes);
            break;
        default:
            PVR_DPF((PVR_DBG_ERROR, "IORemapWrapper: unknown mapping flags"));
            return NULL;
    }
    
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    if (pvIORemapCookie)
    {
        DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_IOREMAP,
                               pvIORemapCookie,
                               pvIORemapCookie,
                               BasePAddr.uiAddr,
                               NULL,
                               ui32Bytes,
                               pszFileName,
                               ui32Line
                              );
    }
#else
    PVR_UNREFERENCED_PARAMETER(pszFileName);
    PVR_UNREFERENCED_PARAMETER(ui32Line);
#endif

    return pvIORemapCookie;
}


IMG_VOID
_IOUnmapWrapper(IMG_VOID *pvIORemapCookie, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line)
{
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_IOREMAP, pvIORemapCookie, pszFileName, ui32Line);
#else
    PVR_UNREFERENCED_PARAMETER(pszFileName);
    PVR_UNREFERENCED_PARAMETER(ui32Line);
#endif
    iounmap(pvIORemapCookie);
}


LinuxMemArea *
NewIORemapLinuxMemArea(IMG_CPU_PHYADDR BasePAddr,
                       IMG_UINT32 ui32Bytes,
                       IMG_UINT32 ui32AreaFlags)
{
    LinuxMemArea *psLinuxMemArea;
    IMG_VOID *pvIORemapCookie;

    psLinuxMemArea = LinuxMemAreaStructAlloc();
    if (!psLinuxMemArea)
    {
        return NULL;
    }

    pvIORemapCookie = IORemapWrapper(BasePAddr, ui32Bytes, ui32AreaFlags);
    if (!pvIORemapCookie)
    {
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


IMG_VOID
FreeIORemapLinuxMemArea(LinuxMemArea *psLinuxMemArea)
{
    PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_IOREMAP);

#if defined(DEBUG_LINUX_MEM_AREAS)
    DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif
    
    IOUnmapWrapper(psLinuxMemArea->uData.sIORemap.pvIORemapCookie);

    LinuxMemAreaStructFree(psLinuxMemArea);
}


static IMG_BOOL
TreatExternalPagesAsContiguous(IMG_SYS_PHYADDR *psSysPhysAddr, IMG_UINT32 ui32Bytes, IMG_BOOL bPhysContig)
{
	IMG_UINT32 ui32;
	IMG_UINT32 ui32AddrChk;
	IMG_UINT32 ui32NumPages = RANGE_TO_PAGES(ui32Bytes);

	/*
	 * If bPhysContig is IMG_TRUE, we must assume psSysPhysAddr points
	 * to the address of the first page, not an array of page addresses.
	 */
	for (ui32 = 0, ui32AddrChk = psSysPhysAddr[0].uiAddr;
		ui32 < ui32NumPages;
		ui32++, ui32AddrChk = (bPhysContig) ? (ui32AddrChk + PAGE_SIZE) : psSysPhysAddr[ui32].uiAddr)
	{
		if (!pfn_valid(PHYS_TO_PFN(ui32AddrChk)))
		{
			break;
		}
	}
	if (ui32 == ui32NumPages)
	{
		return IMG_FALSE;
	}

	if (!bPhysContig)
	{
		for (ui32 = 0, ui32AddrChk = psSysPhysAddr[0].uiAddr;
			ui32 < ui32NumPages;
			ui32++, ui32AddrChk += PAGE_SIZE)
		{
			if (psSysPhysAddr[ui32].uiAddr != ui32AddrChk)
			{
				return IMG_FALSE;
			}
		}
	}

	return IMG_TRUE;
}

LinuxMemArea *NewExternalKVLinuxMemArea(IMG_SYS_PHYADDR *pBasePAddr, IMG_VOID *pvCPUVAddr, IMG_UINT32 ui32Bytes, IMG_BOOL bPhysContig, IMG_UINT32 ui32AreaFlags)
{
    LinuxMemArea *psLinuxMemArea;

    psLinuxMemArea = LinuxMemAreaStructAlloc();
    if (!psLinuxMemArea)
    {
        return NULL;
    }

    psLinuxMemArea->eAreaType = LINUX_MEM_AREA_EXTERNAL_KV;
    psLinuxMemArea->uData.sExternalKV.pvExternalKV = pvCPUVAddr;
    psLinuxMemArea->uData.sExternalKV.bPhysContig = (IMG_BOOL)(bPhysContig || TreatExternalPagesAsContiguous(pBasePAddr, ui32Bytes, bPhysContig));

    if (psLinuxMemArea->uData.sExternalKV.bPhysContig)
    {
	psLinuxMemArea->uData.sExternalKV.uPhysAddr.SysPhysAddr = *pBasePAddr;
    }
    else
    {
	psLinuxMemArea->uData.sExternalKV.uPhysAddr.pSysPhysAddr = pBasePAddr;
    }
    psLinuxMemArea->ui32ByteSize = ui32Bytes;
    psLinuxMemArea->ui32AreaFlags = ui32AreaFlags;
    psLinuxMemArea->bMMapRegistered = IMG_FALSE;
    INIT_LIST_HEAD(&psLinuxMemArea->sMMapOffsetStructList);

#if defined(DEBUG_LINUX_MEM_AREAS)
    DebugLinuxMemAreaRecordAdd(psLinuxMemArea, ui32AreaFlags);
#endif

    return psLinuxMemArea;
}


IMG_VOID
FreeExternalKVLinuxMemArea(LinuxMemArea *psLinuxMemArea)
{
    PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_EXTERNAL_KV);

#if defined(DEBUG_LINUX_MEM_AREAS)
    DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif
    
    LinuxMemAreaStructFree(psLinuxMemArea);
}


LinuxMemArea *
NewIOLinuxMemArea(IMG_CPU_PHYADDR BasePAddr,
                  IMG_UINT32 ui32Bytes,
                  IMG_UINT32 ui32AreaFlags)
{
    LinuxMemArea *psLinuxMemArea = LinuxMemAreaStructAlloc();
    if (!psLinuxMemArea)
    {
        return NULL;
    }

    /* Nothing to activly do. We just keep a record of the physical range. */
    psLinuxMemArea->eAreaType = LINUX_MEM_AREA_IO;
    psLinuxMemArea->uData.sIO.CPUPhysAddr.uiAddr = BasePAddr.uiAddr;
    psLinuxMemArea->ui32ByteSize = ui32Bytes;
    psLinuxMemArea->ui32AreaFlags = ui32AreaFlags;
    psLinuxMemArea->bMMapRegistered = IMG_FALSE;
    INIT_LIST_HEAD(&psLinuxMemArea->sMMapOffsetStructList);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_IO,
                           (IMG_VOID *)BasePAddr.uiAddr,
                           0,
                           BasePAddr.uiAddr,
                           NULL,
                           ui32Bytes,
                           "unknown",
                           0
                          );
#endif
   
#if defined(DEBUG_LINUX_MEM_AREAS)
    DebugLinuxMemAreaRecordAdd(psLinuxMemArea, ui32AreaFlags);
#endif

    return psLinuxMemArea;
}


IMG_VOID
FreeIOLinuxMemArea(LinuxMemArea *psLinuxMemArea)
{
    PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_IO);
    
#if defined(DEBUG_LINUX_MEM_AREAS)
    DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_IO,
                              (IMG_VOID *)psLinuxMemArea->uData.sIO.CPUPhysAddr.uiAddr, __FILE__, __LINE__);
#endif

    /* Nothing more to do than free the LinuxMemArea struct */

    LinuxMemAreaStructFree(psLinuxMemArea);
}


LinuxMemArea *
NewAllocPagesLinuxMemArea(IMG_UINT32 ui32Bytes, IMG_UINT32 ui32AreaFlags)
{
    LinuxMemArea *psLinuxMemArea;
    IMG_UINT32 ui32PageCount;
    struct page **pvPageList;
    IMG_HANDLE hBlockPageList;
    IMG_INT32 i;
    PVRSRV_ERROR eError;

    psLinuxMemArea = LinuxMemAreaStructAlloc();
    if (!psLinuxMemArea)
    {
        goto failed_area_alloc;
    }

    ui32PageCount = RANGE_TO_PAGES(ui32Bytes);
    eError = OSAllocMem(0, sizeof(*pvPageList) * ui32PageCount, (IMG_VOID **)&pvPageList, &hBlockPageList,
							"Array of pages");
    if(eError != PVRSRV_OK)
    {
        goto failed_page_list_alloc;
    }

    for(i=0; i<(IMG_INT32)ui32PageCount; i++)
    {
        pvPageList[i] = alloc_pages(GFP_KERNEL | __GFP_HIGHMEM, 0);
        if(!pvPageList[i])
        {
            goto failed_alloc_pages;
        }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15))

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
	SetPageReserved(pvPageList[i]);
#else
        mem_map_reserve(pvPageList[i]);
#endif
#endif

    }

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES,
                           pvPageList,
                           0,
                           0,
                           NULL,
                           PAGE_ALIGN(ui32Bytes),
                           "unknown",
                           0
                           );
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
    for(i--; i >= 0; i--)
    {
        __free_pages(pvPageList[i], 0);
    }
    (IMG_VOID) OSFreeMem(0, sizeof(*pvPageList) * ui32PageCount, pvPageList, hBlockPageList);
	psLinuxMemArea->uData.sPageList.pvPageList = IMG_NULL;
failed_page_list_alloc:
    LinuxMemAreaStructFree(psLinuxMemArea);
failed_area_alloc:
    PVR_DPF((PVR_DBG_ERROR, "%s: failed", __FUNCTION__));

    return NULL;
}


IMG_VOID
FreeAllocPagesLinuxMemArea(LinuxMemArea *psLinuxMemArea)
{
    IMG_UINT32 ui32PageCount;
    struct page **pvPageList;
    IMG_HANDLE hBlockPageList;
    IMG_INT32 i;

    PVR_ASSERT(psLinuxMemArea);
    PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_ALLOC_PAGES);

#if defined(DEBUG_LINUX_MEM_AREAS)
    DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif

    ui32PageCount = RANGE_TO_PAGES(psLinuxMemArea->ui32ByteSize);
    pvPageList = psLinuxMemArea->uData.sPageList.pvPageList;
    hBlockPageList = psLinuxMemArea->uData.sPageList.hBlockPageList;

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES, pvPageList, __FILE__, __LINE__);
#endif

    for(i=0;i<(IMG_INT32)ui32PageCount;i++)
    {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15))
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
        ClearPageReserved(pvPageList[i]);
#else
        mem_map_reserve(pvPageList[i]);
#endif
#endif
        __free_pages(pvPageList[i], 0);
    }

    (IMG_VOID) OSFreeMem(0, sizeof(*pvPageList) * ui32PageCount, pvPageList, hBlockPageList);
	psLinuxMemArea->uData.sPageList.pvPageList = IMG_NULL;

    LinuxMemAreaStructFree(psLinuxMemArea);
}


struct page*
LinuxMemAreaOffsetToPage(LinuxMemArea *psLinuxMemArea,
                         IMG_UINT32 ui32ByteOffset)
{
    IMG_UINT32 ui32PageIndex;
    IMG_CHAR *pui8Addr;

    switch (psLinuxMemArea->eAreaType)
    {
        case LINUX_MEM_AREA_ALLOC_PAGES:
            ui32PageIndex = PHYS_TO_PFN(ui32ByteOffset);
            return psLinuxMemArea->uData.sPageList.pvPageList[ui32PageIndex];
 
        case LINUX_MEM_AREA_VMALLOC:
            pui8Addr = psLinuxMemArea->uData.sVmalloc.pvVmallocAddress;
            pui8Addr += ui32ByteOffset;
            return vmalloc_to_page(pui8Addr);
 
        case LINUX_MEM_AREA_SUB_ALLOC:
            /* PRQA S 3670 3 */ /* ignore recursive warning */
            return LinuxMemAreaOffsetToPage(psLinuxMemArea->uData.sSubAlloc.psParentLinuxMemArea,
                                            psLinuxMemArea->uData.sSubAlloc.ui32ByteOffset
                                             + ui32ByteOffset);
        default:
            PVR_DPF((PVR_DBG_ERROR,
                    "%s: Unsupported request for struct page from LinuxMemArea with type=%s",
                    __FUNCTION__, LinuxMemAreaTypeToString(psLinuxMemArea->eAreaType)));
            return NULL;
    }
}


LinuxKMemCache *
KMemCacheCreateWrapper(IMG_CHAR *pszName,
                       size_t Size,
                       size_t Align,
                       IMG_UINT32 ui32Flags)
{
#if defined(DEBUG_LINUX_SLAB_ALLOCATIONS)
    ui32Flags |= SLAB_POISON|SLAB_RED_ZONE;
#endif
    return kmem_cache_create(pszName, Size, Align, ui32Flags, NULL
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,22))
				, NULL
#endif
			    );
}


IMG_VOID
KMemCacheDestroyWrapper(LinuxKMemCache *psCache)
{
    kmem_cache_destroy(psCache);
}


IMG_VOID *
_KMemCacheAllocWrapper(LinuxKMemCache *psCache,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14))
                      gfp_t Flags,
#else
                      IMG_INT Flags,
#endif
                      IMG_CHAR *pszFileName,
                      IMG_UINT32 ui32Line)
{
    IMG_VOID *pvRet;

    pvRet = kmem_cache_alloc(psCache, Flags);

#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    DebugMemAllocRecordAdd(DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE,
                           pvRet,
                           pvRet,
                           0,
                           psCache,
                           kmem_cache_size(psCache),
                           pszFileName,
                           ui32Line
                          );
#else
    PVR_UNREFERENCED_PARAMETER(pszFileName);
    PVR_UNREFERENCED_PARAMETER(ui32Line);
#endif
    
    return pvRet;
}


IMG_VOID
_KMemCacheFreeWrapper(LinuxKMemCache *psCache, IMG_VOID *pvObject, IMG_CHAR *pszFileName, IMG_UINT32 ui32Line)
{
#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
    DebugMemAllocRecordRemove(DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE, pvObject, pszFileName, ui32Line);
#else
    PVR_UNREFERENCED_PARAMETER(pszFileName);
    PVR_UNREFERENCED_PARAMETER(ui32Line);
#endif

    kmem_cache_free(psCache, pvObject);
}


const IMG_CHAR *
KMemCacheNameWrapper(LinuxKMemCache *psCache)
{
    PVR_UNREFERENCED_PARAMETER(psCache);


    return "";
}


LinuxMemArea *
NewSubLinuxMemArea(LinuxMemArea *psParentLinuxMemArea,
                   IMG_UINT32 ui32ByteOffset,
                   IMG_UINT32 ui32Bytes)
{
    LinuxMemArea *psLinuxMemArea;
    
    PVR_ASSERT((ui32ByteOffset+ui32Bytes) <= psParentLinuxMemArea->ui32ByteSize);
    
    psLinuxMemArea = LinuxMemAreaStructAlloc();
    if (!psLinuxMemArea)
    {
        return NULL;
    }
    
    psLinuxMemArea->eAreaType = LINUX_MEM_AREA_SUB_ALLOC;
    psLinuxMemArea->uData.sSubAlloc.psParentLinuxMemArea = psParentLinuxMemArea;
    psLinuxMemArea->uData.sSubAlloc.ui32ByteOffset = ui32ByteOffset;
    psLinuxMemArea->ui32ByteSize = ui32Bytes;
    psLinuxMemArea->ui32AreaFlags = psParentLinuxMemArea->ui32AreaFlags;
    psLinuxMemArea->bMMapRegistered = IMG_FALSE;
    INIT_LIST_HEAD(&psLinuxMemArea->sMMapOffsetStructList);
    
#if defined(DEBUG_LINUX_MEM_AREAS)
    {
        DEBUG_LINUX_MEM_AREA_REC *psParentRecord;
        psParentRecord = DebugLinuxMemAreaRecordFind(psParentLinuxMemArea);
        DebugLinuxMemAreaRecordAdd(psLinuxMemArea, psParentRecord->ui32Flags);
    }
#endif
    
    return psLinuxMemArea;
}


IMG_VOID
FreeSubLinuxMemArea(LinuxMemArea *psLinuxMemArea)
{
    PVR_ASSERT(psLinuxMemArea->eAreaType == LINUX_MEM_AREA_SUB_ALLOC);

#if defined(DEBUG_LINUX_MEM_AREAS)
    DebugLinuxMemAreaRecordRemove(psLinuxMemArea);
#endif
    
    /* Nothing more to do than free the LinuxMemArea structure */

    LinuxMemAreaStructFree(psLinuxMemArea);
}


static LinuxMemArea *
LinuxMemAreaStructAlloc(IMG_VOID)
{
/* debug */
#if 0
    LinuxMemArea *psLinuxMemArea;
    psLinuxMemArea = kmem_cache_alloc(psLinuxMemAreaCache, GFP_KERNEL);
    printk(KERN_ERR "%s: psLinuxMemArea=%p\n", __FUNCTION__, psLinuxMemArea);
    dump_stack();
    return psLinuxMemArea;
#else
    return KMemCacheAllocWrapper(psLinuxMemAreaCache, GFP_KERNEL);
#endif
}


static IMG_VOID
LinuxMemAreaStructFree(LinuxMemArea *psLinuxMemArea)
{
    KMemCacheFreeWrapper(psLinuxMemAreaCache, psLinuxMemArea);


}


IMG_VOID
LinuxMemAreaDeepFree(LinuxMemArea *psLinuxMemArea)
{
    switch (psLinuxMemArea->eAreaType)
    {
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
            PVR_DPF((PVR_DBG_ERROR, "%s: Unknown are type (%d)\n",
                     __FUNCTION__, psLinuxMemArea->eAreaType));
            break;
    }
}


#if defined(DEBUG_LINUX_MEM_AREAS)
static IMG_VOID
DebugLinuxMemAreaRecordAdd(LinuxMemArea *psLinuxMemArea, IMG_UINT32 ui32Flags)
{
    DEBUG_LINUX_MEM_AREA_REC *psNewRecord;
    const IMG_CHAR *pi8FlagsString;
    
    mutex_lock(&g_sDebugMutex);

    if (psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC)
    {
        g_LinuxMemAreaWaterMark += psLinuxMemArea->ui32ByteSize;
        if(g_LinuxMemAreaWaterMark > g_LinuxMemAreaHighWaterMark)
        {
            g_LinuxMemAreaHighWaterMark = g_LinuxMemAreaWaterMark;
        }
    }
    g_LinuxMemAreaCount++;
    
    /* Create a new memory allocation record */
    psNewRecord = kmalloc(sizeof(DEBUG_LINUX_MEM_AREA_REC), GFP_KERNEL);
    if (psNewRecord)
    {
        /* Record the allocation */
        psNewRecord->psLinuxMemArea = psLinuxMemArea;
        psNewRecord->ui32Flags = ui32Flags;
        psNewRecord->pid = current->pid;
		
		List_DEBUG_LINUX_MEM_AREA_REC_Insert(&g_LinuxMemAreaRecords, psNewRecord);
    }
    else
    {
        PVR_DPF((PVR_DBG_ERROR,
                 "%s: failed to allocate linux memory area record.",
                 __FUNCTION__));
    }
    
    /* Sanity check the flags */
    pi8FlagsString = HAPFlagsToString(ui32Flags);
    if(strstr(pi8FlagsString, "UNKNOWN"))
    {
        PVR_DPF((PVR_DBG_ERROR,
                 "%s: Unexpected flags (0x%08lx) associated with psLinuxMemArea @ 0x%08lx",
                 __FUNCTION__,
                 ui32Flags,
                 psLinuxMemArea));
        //dump_stack();
    }

    mutex_unlock(&g_sDebugMutex);
}



IMG_VOID* MatchLinuxMemArea_AnyVaCb(DEBUG_LINUX_MEM_AREA_REC *psCurrentRecord,
												va_list va)
{
	LinuxMemArea *psLinuxMemArea;
	
	psLinuxMemArea = va_arg(va, LinuxMemArea*);
	if(psCurrentRecord->psLinuxMemArea == psLinuxMemArea)
	{
		return psCurrentRecord;
	}
	else
	{
		return IMG_NULL;
	}
}


static DEBUG_LINUX_MEM_AREA_REC *
DebugLinuxMemAreaRecordFind(LinuxMemArea *psLinuxMemArea)
{
    DEBUG_LINUX_MEM_AREA_REC *psCurrentRecord;

    mutex_lock(&g_sDebugMutex);
	psCurrentRecord = List_DEBUG_LINUX_MEM_AREA_REC_Any_va(g_LinuxMemAreaRecords,
														MatchLinuxMemArea_AnyVaCb,
														psLinuxMemArea);

    mutex_unlock(&g_sDebugMutex);

    return psCurrentRecord;
}


static IMG_VOID
DebugLinuxMemAreaRecordRemove(LinuxMemArea *psLinuxMemArea)
{
    DEBUG_LINUX_MEM_AREA_REC *psCurrentRecord;

    mutex_lock(&g_sDebugMutex);

    if (psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC)
    {
        g_LinuxMemAreaWaterMark -= psLinuxMemArea->ui32ByteSize;
    }
    g_LinuxMemAreaCount--;

    /* Locate the corresponding allocation entry */
	psCurrentRecord = List_DEBUG_LINUX_MEM_AREA_REC_Any_va(g_LinuxMemAreaRecords,
														MatchLinuxMemArea_AnyVaCb,
														psLinuxMemArea);
	if (psCurrentRecord)
	{
		/* Unlink the allocation record */
		List_DEBUG_LINUX_MEM_AREA_REC_Remove(psCurrentRecord);
		kfree(psCurrentRecord);
	}
	else
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: couldn't find an entry for psLinuxMemArea=%p\n",
	     __FUNCTION__, psLinuxMemArea));
	}

    mutex_unlock(&g_sDebugMutex);
}
#endif


IMG_VOID *
LinuxMemAreaToCpuVAddr(LinuxMemArea *psLinuxMemArea)
{
    switch(psLinuxMemArea->eAreaType)
    {
        case LINUX_MEM_AREA_VMALLOC:
            return psLinuxMemArea->uData.sVmalloc.pvVmallocAddress;
        case LINUX_MEM_AREA_IOREMAP:
            return psLinuxMemArea->uData.sIORemap.pvIORemapCookie;
	case LINUX_MEM_AREA_EXTERNAL_KV:
	    return psLinuxMemArea->uData.sExternalKV.pvExternalKV;
        case LINUX_MEM_AREA_SUB_ALLOC:
        {
            IMG_CHAR *pAddr =
                LinuxMemAreaToCpuVAddr(psLinuxMemArea->uData.sSubAlloc.psParentLinuxMemArea);
            if (!pAddr)
            {
                return NULL;
            }
            return pAddr + psLinuxMemArea->uData.sSubAlloc.ui32ByteOffset;
        }
        default:
            return NULL;
    }
}


IMG_CPU_PHYADDR
LinuxMemAreaToCpuPAddr(LinuxMemArea *psLinuxMemArea, IMG_UINT32 ui32ByteOffset)
{
    IMG_CPU_PHYADDR CpuPAddr;

    CpuPAddr.uiAddr = 0;

    switch (psLinuxMemArea->eAreaType)
    {
        case LINUX_MEM_AREA_IOREMAP:
        {
            CpuPAddr = psLinuxMemArea->uData.sIORemap.CPUPhysAddr;
            CpuPAddr.uiAddr += ui32ByteOffset;
            break;
        }
	case LINUX_MEM_AREA_EXTERNAL_KV:
	{
	    if (psLinuxMemArea->uData.sExternalKV.bPhysContig)
	    {
		CpuPAddr = SysSysPAddrToCpuPAddr(psLinuxMemArea->uData.sExternalKV.uPhysAddr.SysPhysAddr);
		CpuPAddr.uiAddr += ui32ByteOffset;
	    }
	    else
	    {
		IMG_UINT32 ui32PageIndex = PHYS_TO_PFN(ui32ByteOffset);
		IMG_SYS_PHYADDR SysPAddr = psLinuxMemArea->uData.sExternalKV.uPhysAddr.pSysPhysAddr[ui32PageIndex];

		CpuPAddr = SysSysPAddrToCpuPAddr(SysPAddr);
                CpuPAddr.uiAddr += ADDR_TO_PAGE_OFFSET(ui32ByteOffset);
		if (0 == CpuPAddr.uiAddr) {
			/* FIXME: REPLACE THIS WITH A DIFFERENT FIX SOMEDAY.
			 * has only seen this code path be triggered when
			 * DestroyOffsetStruct() calls this as a parameter for a debug
			 * statement.  The times its been seen has been when the user-mode
			 * program (i.e. X server) crashed when doing mode changes, or was
			 * killed after an unhealthy mode change, and the kernel was trying
			 * to clean up all of the VMA's associated with that client.  For
			 * now, don't leave the address as 0 (which causes an Oops), but
			 * work-around this by setting the CpuPAddr.uiAddr to 7.  As long
			 * as 7 is used for a debug statement, it's no big deal.  If 7 were
			 * used as a real address, an Oops would be caused anyway.
			 */
			printk(KERN_ERR "ERROR DESCRIPTION TO FOLLOW:\n");
			printk(KERN_ERR "  LinuxMemAreaToCpuPAddr() failed to look up a "
				"physical\n  address of a memory mapped area (i.e. got an "
				"address\n  of 0x00000000).  If this occured after increasing "
				"the\n  resolution of a display, it probably indicates that "
				"the\n  client stepped past the end of the old frame buffer's\n"
				"  memory, and needed to re-initialize its connection with\n"
				"  PVR services in order to map the new frame buffer address."
				"\n\n");
			CpuPAddr.uiAddr = 7;
		}
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
            IMG_CHAR *pCpuVAddr;
            pCpuVAddr =
                (IMG_CHAR *)psLinuxMemArea->uData.sVmalloc.pvVmallocAddress;
            pCpuVAddr += ui32ByteOffset;
            CpuPAddr.uiAddr = VMallocToPhys(pCpuVAddr);
            break;
        }
        case LINUX_MEM_AREA_ALLOC_PAGES:
        {
            struct page *page;
            IMG_UINT32 ui32PageIndex = PHYS_TO_PFN(ui32ByteOffset);
            page = psLinuxMemArea->uData.sPageList.pvPageList[ui32PageIndex];
            CpuPAddr.uiAddr = page_to_phys(page);
            CpuPAddr.uiAddr += ADDR_TO_PAGE_OFFSET(ui32ByteOffset);
            break;
        }
        case LINUX_MEM_AREA_SUB_ALLOC:
        {
            CpuPAddr =
                OSMemHandleToCpuPAddr(psLinuxMemArea->uData.sSubAlloc.psParentLinuxMemArea,
                                      psLinuxMemArea->uData.sSubAlloc.ui32ByteOffset
                                        + ui32ByteOffset);
            break;
        }
        default:
            PVR_DPF((PVR_DBG_ERROR, "%s: Unknown LinuxMemArea type (%d)\n",
                     __FUNCTION__, psLinuxMemArea->eAreaType));
            break;
   }

    PVR_ASSERT(CpuPAddr.uiAddr);
    return CpuPAddr;
}


IMG_BOOL
LinuxMemAreaPhysIsContig(LinuxMemArea *psLinuxMemArea)
{
    switch (psLinuxMemArea->eAreaType)
    {
        case LINUX_MEM_AREA_IOREMAP:
        case LINUX_MEM_AREA_IO:
            return IMG_TRUE;

	case LINUX_MEM_AREA_EXTERNAL_KV:
	    return psLinuxMemArea->uData.sExternalKV.bPhysContig;

        case LINUX_MEM_AREA_VMALLOC:
        case LINUX_MEM_AREA_ALLOC_PAGES:
	    return IMG_FALSE;

        case LINUX_MEM_AREA_SUB_ALLOC:

	    return LinuxMemAreaPhysIsContig(psLinuxMemArea->uData.sSubAlloc.psParentLinuxMemArea);

        default:
            PVR_DPF((PVR_DBG_ERROR, "%s: Unknown LinuxMemArea type (%d)\n",
                     __FUNCTION__, psLinuxMemArea->eAreaType));
	    break;
    }
    return IMG_FALSE;
}


const IMG_CHAR *
LinuxMemAreaTypeToString(LINUX_MEM_AREA_TYPE eMemAreaType)
{
    /* Note we explicitly check the types instead of e.g.
     * using the type to index an array of strings so
     * we remain orthogonal to enum changes */
    switch (eMemAreaType)
    {
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


#ifdef PVR_PROC_USE_SEQ_FILE
#if defined(DEBUG_LINUX_MEM_AREAS) || defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)
static void ProcSeqStartstopDebugMutex(struct seq_file *sfile, IMG_BOOL start)
{
	if (start) 
	{
	    mutex_lock(&g_sDebugMutex);
	}
	else
	{
	    mutex_unlock(&g_sDebugMutex);
	}
}
#endif /* defined(DEBUG_LINUX_MEM_AREAS) || defined(DEBUG_LINUX_MEMORY_ALLOCATIONS) */
#endif

#if defined(DEBUG_LINUX_MEM_AREAS)

IMG_VOID* DecOffMemAreaRec_AnyVaCb(DEBUG_LINUX_MEM_AREA_REC *psNode, va_list va)
{
	off_t *pOff = va_arg(va, off_t*);
	if (--(*pOff))
	{
		return IMG_NULL;
	}
	else
	{
		return psNode;
	}
}

#ifdef PVR_PROC_USE_SEQ_FILE

static void* ProcSeqNextMemArea(struct seq_file *sfile,void* el,loff_t off)
{
    DEBUG_LINUX_MEM_AREA_REC *psRecord;
	psRecord = (DEBUG_LINUX_MEM_AREA_REC*)
				List_DEBUG_LINUX_MEM_AREA_REC_Any_va(g_LinuxMemAreaRecords,
													DecOffMemAreaRec_AnyVaCb,
													&off);
	return (void*)psRecord;
}

static void* ProcSeqOff2ElementMemArea(struct seq_file * sfile, loff_t off)
{
    DEBUG_LINUX_MEM_AREA_REC *psRecord;
	if (!off) 
	{
		return PVR_PROC_SEQ_START_TOKEN;
	}

	psRecord = (DEBUG_LINUX_MEM_AREA_REC*)
				List_DEBUG_LINUX_MEM_AREA_REC_Any_va(g_LinuxMemAreaRecords,
													DecOffMemAreaRec_AnyVaCb,
													&off);
	return (void*)psRecord;
}


static void ProcSeqShowMemArea(struct seq_file *sfile,void* el)
{
    DEBUG_LINUX_MEM_AREA_REC *psRecord = (DEBUG_LINUX_MEM_AREA_REC*)el;
	if (el == PVR_PROC_SEQ_START_TOKEN) 
	{

#if !defined(DEBUG_LINUX_XML_PROC_FILES)
        seq_printf( sfile,
			  "Number of Linux Memory Areas: %lu\n"
                          "At the current water mark these areas correspond to %lu bytes (excluding SUB areas)\n"
                          "At the highest water mark these areas corresponded to %lu bytes (excluding SUB areas)\n"
                          "\nDetails for all Linux Memory Areas:\n"
                          "%s %-24s %s %s %-8s %-5s %s\n",
                          g_LinuxMemAreaCount,
                          g_LinuxMemAreaWaterMark,
                          g_LinuxMemAreaHighWaterMark,
                          "psLinuxMemArea",
                          "LinuxMemType",
                          "CpuVAddr",
                          "CpuPAddr",
                          "Bytes",
                          "Pid",
                          "Flags"
                        );
#else
        seq_printf(sfile,
                          "<mem_areas_header>\n"
                          "\t<count>%lu</count>\n"
                          "\t<watermark key=\"mar0\" description=\"current\" bytes=\"%lu\"/>\n"
                          "\t<watermark key=\"mar1\" description=\"high\" bytes=\"%lu\"/>\n"
                          "</mem_areas_header>\n",
                          g_LinuxMemAreaCount,
                          g_LinuxMemAreaWaterMark,
                          g_LinuxMemAreaHighWaterMark
                        );
#endif
		return;
	}

        seq_printf(sfile,
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
                       "%8p       %-24s %8p %08lx %-8ld %-5u %08lx=(%s)\n",
#else
                       "<linux_mem_area>\n"
                       "\t<pointer>%8p</pointer>\n"
                       "\t<type>%s</type>\n"
                       "\t<cpu_virtual>%8p</cpu_virtual>\n"
                       "\t<cpu_physical>%08lx</cpu_physical>\n"
                       "\t<bytes>%ld</bytes>\n"
                       "\t<pid>%u</pid>\n"
                       "\t<flags>%08lx</flags>\n"
                       "\t<flags_string>%s</flags_string>\n"
                       "</linux_mem_area>\n",
#endif
                       psRecord->psLinuxMemArea,
                       LinuxMemAreaTypeToString(psRecord->psLinuxMemArea->eAreaType),
                       LinuxMemAreaToCpuVAddr(psRecord->psLinuxMemArea),
                       LinuxMemAreaToCpuPAddr(psRecord->psLinuxMemArea,0).uiAddr,
                       psRecord->psLinuxMemArea->ui32ByteSize,
                       psRecord->pid,
                       psRecord->ui32Flags,
                       HAPFlagsToString(psRecord->ui32Flags)
                      );

}

#else

static off_t
printLinuxMemAreaRecords(IMG_CHAR * buffer, size_t count, off_t off)
{
    DEBUG_LINUX_MEM_AREA_REC *psRecord;
    off_t Ret;

    mutex_lock(&g_sDebugMutex);

    if(!off)
    {
        if(count < 500)
        {
            Ret = 0;
            goto unlock_and_return;
        }
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
        Ret = printAppend(buffer, count, 0,
                          "Number of Linux Memory Areas: %lu\n"
                          "At the current water mark these areas correspond to %lu bytes (excluding SUB areas)\n"
                          "At the highest water mark these areas corresponded to %lu bytes (excluding SUB areas)\n"
                          "\nDetails for all Linux Memory Areas:\n"
                          "%s %-24s %s %s %-8s %-5s %s\n",
                          g_LinuxMemAreaCount,
                          g_LinuxMemAreaWaterMark,
                          g_LinuxMemAreaHighWaterMark,
                          "psLinuxMemArea",
                          "LinuxMemType",
                          "CpuVAddr",
                          "CpuPAddr",
                          "Bytes",
                          "Pid",
                          "Flags"
                         );
#else
        Ret = printAppend(buffer, count, 0,
                          "<mem_areas_header>\n"
                          "\t<count>%lu</count>\n"
                          "\t<watermark key=\"mar0\" description=\"current\" bytes=\"%lu\"/>\n"
                          "\t<watermark key=\"mar1\" description=\"high\" bytes=\"%lu\"/>\n"
                          "</mem_areas_header>\n",
                          g_LinuxMemAreaCount,
                          g_LinuxMemAreaWaterMark,
                          g_LinuxMemAreaHighWaterMark
                         );
#endif
        goto unlock_and_return;
    }

	psRecord = (DEBUG_LINUX_MEM_AREA_REC*)
				List_DEBUG_LINUX_MEM_AREA_REC_Any_va(g_LinuxMemAreaRecords,
													DecOffMemAreaRec_AnyVaCb,
													&off);

    if(!psRecord)
    {
        Ret = END_OF_FILE;
        goto unlock_and_return;
    }

    if(count < 500)
    {
        Ret = 0;
        goto unlock_and_return;
    }

    Ret =  printAppend(buffer, count, 0,
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
                       "%8p       %-24s %8p %08lx %-8ld %-5u %08lx=(%s)\n",
#else
                       "<linux_mem_area>\n"
                       "\t<pointer>%8p</pointer>\n"
                       "\t<type>%s</type>\n"
                       "\t<cpu_virtual>%8p</cpu_virtual>\n"
                       "\t<cpu_physical>%08lx</cpu_physical>\n"
                       "\t<bytes>%ld</bytes>\n"
                       "\t<pid>%u</pid>\n"
                       "\t<flags>%08lx</flags>\n"
                       "\t<flags_string>%s</flags_string>\n"
                       "</linux_mem_area>\n",
#endif
                       psRecord->psLinuxMemArea,
                       LinuxMemAreaTypeToString(psRecord->psLinuxMemArea->eAreaType),
                       LinuxMemAreaToCpuVAddr(psRecord->psLinuxMemArea),
                       LinuxMemAreaToCpuPAddr(psRecord->psLinuxMemArea,0).uiAddr,
                       psRecord->psLinuxMemArea->ui32ByteSize,
                       psRecord->pid,
                       psRecord->ui32Flags,
                       HAPFlagsToString(psRecord->ui32Flags)
                      );

unlock_and_return:
    mutex_unlock(&g_sDebugMutex);
    return Ret;
}
#endif

#endif


#if defined(DEBUG_LINUX_MEMORY_ALLOCATIONS)

IMG_VOID* DecOffMemAllocRec_AnyVaCb(DEBUG_MEM_ALLOC_REC *psNode, va_list va)
{
	off_t *pOff = va_arg(va, off_t*);
	if (--(*pOff))
	{
		return IMG_NULL;
	}
	else
	{
		return psNode;
	}
}


#ifdef PVR_PROC_USE_SEQ_FILE

static void* ProcSeqNextMemoryRecords(struct seq_file *sfile,void* el,loff_t off)
{
    DEBUG_MEM_ALLOC_REC *psRecord;
	psRecord = (DEBUG_MEM_ALLOC_REC*)
		List_DEBUG_MEM_ALLOC_REC_Any_va(g_MemoryRecords,
										DecOffMemAllocRec_AnyVaCb,
										&off);
#if defined(DEBUG_LINUX_XML_PROC_FILES)
	if (!psRecord) 
	{
		seq_printf(sfile, "</meminfo>\n");
	}
#endif

	return (void*)psRecord;
}

static void* ProcSeqOff2ElementMemoryRecords(struct seq_file *sfile, loff_t off)
{
    DEBUG_MEM_ALLOC_REC *psRecord;
	if (!off) 
	{
		return PVR_PROC_SEQ_START_TOKEN;
	}

	psRecord = (DEBUG_MEM_ALLOC_REC*)
		List_DEBUG_MEM_ALLOC_REC_Any_va(g_MemoryRecords,
										DecOffMemAllocRec_AnyVaCb,
										&off);

#if defined(DEBUG_LINUX_XML_PROC_FILES)
	if (!psRecord) 
	{
		seq_printf(sfile, "</meminfo>\n");
	}
#endif

	return (void*)psRecord;
}

static void ProcSeqShowMemoryRecords(struct seq_file *sfile,void* el)
{
    DEBUG_MEM_ALLOC_REC *psRecord = (DEBUG_MEM_ALLOC_REC*)el;
	if (el == PVR_PROC_SEQ_START_TOKEN) 
	{
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
        /* NOTE: If you update this code, please also update the XML varient below
         * too! */

        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes allocated via kmalloc",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMALLOC]);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated via kmalloc",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMALLOC]);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes allocated via vmalloc",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_VMALLOC]);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated via vmalloc",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_VMALLOC]);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes allocated via alloc_pages",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES]);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated via alloc_pages",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES]);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes allocated via ioremap",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_IOREMAP]);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated via ioremap",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_IOREMAP]);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes reserved for \"IO\" memory areas",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_IO]);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated for \"IO\" memory areas",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_IO]);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes allocated via kmem_cache_alloc",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE]);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated via kmem_cache_alloc",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE]);
        seq_printf( sfile, "\n");

        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "The Current Water Mark for memory allocated from system RAM",
                           g_SysRAMWaterMark);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "The Highest Water Mark for memory allocated from system RAM",
                           g_SysRAMHighWaterMark);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "The Current Water Mark for memory allocated from IO memory",
                           g_IOMemWaterMark);
        seq_printf( sfile, "%-60s: %ld bytes\n",
                           "The Highest Water Mark for memory allocated from IO memory",
                           g_IOMemHighWaterMark);

        seq_printf( sfile, "\n");

		seq_printf(sfile, "Details for all known allocations:\n"
                           "%-16s %-8s %-8s %-10s %-5s %-10s %s\n",
                           "Type",
                           "CpuVAddr",
                           "CpuPAddr",
                           "Bytes",
                           "PID",
                           "PrivateData",
                           "Filename:Line");

#else /* DEBUG_LINUX_XML_PROC_FILES */
		
		/* Note: If you want to update the description property of a watermark
		 * ensure that the key property remains unchanged so that watermark data
		 * logged over time from different driver revisions may remain comparable
		 */
		seq_printf(sfile, "<meminfo>\n<meminfo_header>\n");
		seq_printf(sfile,
                           "<watermark key=\"mr0\" description=\"kmalloc_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMALLOC]);
		seq_printf(sfile,
                           "<watermark key=\"mr1\" description=\"kmalloc_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMALLOC]);
		seq_printf(sfile,
                           "<watermark key=\"mr2\" description=\"vmalloc_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_VMALLOC]);
		seq_printf(sfile,
                           "<watermark key=\"mr3\" description=\"vmalloc_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_VMALLOC]);
		seq_printf(sfile,
                           "<watermark key=\"mr4\" description=\"alloc_pages_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES]);
		seq_printf(sfile,
                           "<watermark key=\"mr5\" description=\"alloc_pages_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES]);
		seq_printf(sfile,
                           "<watermark key=\"mr6\" description=\"ioremap_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_IOREMAP]);
		seq_printf(sfile,
                           "<watermark key=\"mr7\" description=\"ioremap_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_IOREMAP]);
		seq_printf(sfile,
                           "<watermark key=\"mr8\" description=\"io_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_IO]);
		seq_printf(sfile,
                           "<watermark key=\"mr9\" description=\"io_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_IO]);
		seq_printf(sfile,
                           "<watermark key=\"mr10\" description=\"kmem_cache_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE]);
		seq_printf(sfile,
                           "<watermark key=\"mr11\" description=\"kmem_cache_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE]);
		seq_printf(sfile,"\n" );

		seq_printf(sfile,
                           "<watermark key=\"mr14\" description=\"system_ram_current\" bytes=\"%ld\"/>\n",
                           g_SysRAMWaterMark);
		seq_printf(sfile,
                           "<watermark key=\"mr15\" description=\"system_ram_high\" bytes=\"%ld\"/>\n",
                           g_SysRAMHighWaterMark);
		seq_printf(sfile,
                           "<watermark key=\"mr16\" description=\"system_io_current\" bytes=\"%ld\"/>\n",
                           g_IOMemWaterMark);
		seq_printf(sfile,
                           "<watermark key=\"mr17\" description=\"system_io_high\" bytes=\"%ld\"/>\n",
                           g_IOMemHighWaterMark);

		seq_printf(sfile, "</meminfo_header>\n");

#endif /* DEBUG_LINUX_XML_PROC_FILES */
		return;
	}

    if(psRecord->eAllocType != DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE)
    {
		seq_printf( sfile,
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
                           "%-16s %-8p %08lx %-10ld %-5d %-10s %s:%ld\n",
#else
                           "<allocation>\n"
                           "\t<type>%s</type>\n"
                           "\t<cpu_virtual>%-8p</cpu_virtual>\n"
                           "\t<cpu_physical>%08lx</cpu_physical>\n"
                           "\t<bytes>%ld</bytes>\n"
                           "\t<pid>%d</pid>\n"
                           "\t<private>%s</private>\n"
                           "\t<filename>%s</filename>\n"
                           "\t<line>%ld</line>\n"
                           "</allocation>\n",
#endif
                           DebugMemAllocRecordTypeToString(psRecord->eAllocType),
                           psRecord->pvCpuVAddr,
                           psRecord->ulCpuPAddr,
                           psRecord->ui32Bytes,
                           psRecord->pid,
                           "NULL",
                           psRecord->pszFileName,
                           psRecord->ui32Line);
    }
    else
    {
		seq_printf( sfile,
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
                           "%-16s %-8p %08lx %-10ld %-5d %-10s %s:%ld\n",
#else
                           "<allocation>\n"
                           "\t<type>%s</type>\n"
                           "\t<cpu_virtual>%-8p</cpu_virtual>\n"
                           "\t<cpu_physical>%08lx</cpu_physical>\n"
                           "\t<bytes>%ld</bytes>\n"
                           "\t<pid>%d</pid>\n"
                           "\t<private>%s</private>\n"
                           "\t<filename>%s</filename>\n"
                           "\t<line>%ld</line>\n"
                           "</allocation>\n",
#endif
                           DebugMemAllocRecordTypeToString(psRecord->eAllocType),
                           psRecord->pvCpuVAddr,
                           psRecord->ulCpuPAddr,
                           psRecord->ui32Bytes,
                           psRecord->pid,
                           KMemCacheNameWrapper(psRecord->pvPrivateData),
                           psRecord->pszFileName,
                           psRecord->ui32Line);
    }
}



#else

static off_t
printMemoryRecords(IMG_CHAR * buffer, size_t count, off_t off)
{
    DEBUG_MEM_ALLOC_REC *psRecord;
    off_t Ret;

    mutex_lock(&g_sDebugMutex);

    if(!off)
    {
        if(count < 1000)
        {
            Ret = 0;
            goto unlock_and_return;
        }

#if !defined(DEBUG_LINUX_XML_PROC_FILES)

        Ret =  printAppend(buffer, count, 0, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes allocated via kmalloc",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMALLOC]);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated via kmalloc",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMALLOC]);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes allocated via vmalloc",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_VMALLOC]);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated via vmalloc",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_VMALLOC]);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes allocated via alloc_pages",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES]);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated via alloc_pages",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES]);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes allocated via ioremap",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_IOREMAP]);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated via ioremap",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_IOREMAP]);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes reserved for \"IO\" memory areas",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_IO]);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated for \"IO\" memory areas",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_IO]);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "Current Water Mark of bytes allocated via kmem_cache_alloc",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE]);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "Highest Water Mark of bytes allocated via kmem_cache_alloc",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE]);
        Ret =  printAppend(buffer, count, Ret, "\n");

        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "The Current Water Mark for memory allocated from system RAM",
                           g_SysRAMWaterMark);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "The Highest Water Mark for memory allocated from system RAM",
                           g_SysRAMHighWaterMark);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "The Current Water Mark for memory allocated from IO memory",
                           g_IOMemWaterMark);
        Ret =  printAppend(buffer, count, Ret, "%-60s: %ld bytes\n",
                           "The Highest Water Mark for memory allocated from IO memory",
                           g_IOMemHighWaterMark);

        Ret =  printAppend(buffer, count, Ret, "\n");

        Ret =  printAppend(buffer, count, Ret, "Details for all known allocations:\n"
                           "%-16s %-8s %-8s %-10s %-5s %-10s %s\n",
                           "Type",
                           "CpuVAddr",
                           "CpuPAddr",
                           "Bytes",
                           "PID",
                           "PrivateData",
                           "Filename:Line");

#else


        Ret =  printAppend(buffer, count, 0, "<meminfo>\n<meminfo_header>\n");
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr0\" description=\"kmalloc_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMALLOC]);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr1\" description=\"kmalloc_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMALLOC]);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr2\" description=\"vmalloc_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_VMALLOC]);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr3\" description=\"vmalloc_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_VMALLOC]);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr4\" description=\"alloc_pages_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES]);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr5\" description=\"alloc_pages_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_ALLOC_PAGES]);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr6\" description=\"ioremap_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_IOREMAP]);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr7\" description=\"ioremap_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_IOREMAP]);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr8\" description=\"io_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_IO]);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr9\" description=\"io_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_IO]);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr10\" description=\"kmem_cache_current\" bytes=\"%ld\"/>\n",
                           g_WaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE]);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr11\" description=\"kmem_cache_high\" bytes=\"%ld\"/>\n",
                           g_HighWaterMarkData[DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE]);
        Ret =  printAppend(buffer, count, Ret, "\n");

        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr14\" description=\"system_ram_current\" bytes=\"%ld\"/>\n",
                           g_SysRAMWaterMark);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr15\" description=\"system_ram_high\" bytes=\"%ld\"/>\n",
                           g_SysRAMHighWaterMark);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr16\" description=\"system_io_current\" bytes=\"%ld\"/>\n",
                           g_IOMemWaterMark);
        Ret =  printAppend(buffer, count, Ret,
                           "<watermark key=\"mr17\" description=\"system_io_high\" bytes=\"%ld\"/>\n",
                           g_IOMemHighWaterMark);

        Ret =  printAppend(buffer, count, Ret, "</meminfo_header>\n");

#endif

        goto unlock_and_return;
    }

    if(count < 1000)
    {
        Ret = 0;
        goto unlock_and_return;
    }

	psRecord = (DEBUG_MEM_ALLOC_REC*)
		List_DEBUG_MEM_ALLOC_REC_Any_va(g_MemoryRecords,
										DecOffMemAllocRec_AnyVaCb,
										&off);
    if(!psRecord)
    {
#if defined(DEBUG_LINUX_XML_PROC_FILES)
		if(off == 0)
		{
			Ret =  printAppend(buffer, count, 0, "</meminfo>\n");
			goto unlock_and_return;
		}
#endif
        Ret = END_OF_FILE;
        goto unlock_and_return;
    }

    if(psRecord->eAllocType != DEBUG_MEM_ALLOC_TYPE_KMEM_CACHE)
    {
        Ret =  printAppend(buffer, count, 0,
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
                           "%-16s %-8p %08lx %-10ld %-5d %-10s %s:%ld\n",
#else
                           "<allocation>\n"
                           "\t<type>%s</type>\n"
                           "\t<cpu_virtual>%-8p</cpu_virtual>\n"
                           "\t<cpu_physical>%08lx</cpu_physical>\n"
                           "\t<bytes>%ld</bytes>\n"
                           "\t<pid>%d</pid>\n"
                           "\t<private>%s</private>\n"
                           "\t<filename>%s</filename>\n"
                           "\t<line>%ld</line>\n"
                           "</allocation>\n",
#endif
                           DebugMemAllocRecordTypeToString(psRecord->eAllocType),
                           psRecord->pvCpuVAddr,
                           psRecord->ulCpuPAddr,
                           psRecord->ui32Bytes,
                           psRecord->pid,
                           "NULL",
                           psRecord->pszFileName,
                           psRecord->ui32Line);
    }
    else
    {
        Ret =  printAppend(buffer, count, 0,
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
                           "%-16s %-8p %08lx %-10ld %-5d %-10s %s:%ld\n",
#else
                           "<allocation>\n"
                           "\t<type>%s</type>\n"
                           "\t<cpu_virtual>%-8p</cpu_virtual>\n"
                           "\t<cpu_physical>%08lx</cpu_physical>\n"
                           "\t<bytes>%ld</bytes>\n"
                           "\t<pid>%d</pid>\n"
                           "\t<private>%s</private>\n"
                           "\t<filename>%s</filename>\n"
                           "\t<line>%ld</line>\n"
                           "</allocation>\n",
#endif
                           DebugMemAllocRecordTypeToString(psRecord->eAllocType),
                           psRecord->pvCpuVAddr,
                           psRecord->ulCpuPAddr,
                           psRecord->ui32Bytes,
                           psRecord->pid,
                           KMemCacheNameWrapper(psRecord->pvPrivateData),
                           psRecord->pszFileName,
                           psRecord->ui32Line);
    }

unlock_and_return:
    mutex_unlock(&g_sDebugMutex);
    return Ret;
}
#endif
#endif


#if defined(DEBUG_LINUX_MEM_AREAS) || defined(DEBUG_LINUX_MMAP_AREAS)
/* This could be moved somewhere more general */
const IMG_CHAR *
HAPFlagsToString(IMG_UINT32 ui32Flags)
{
    static IMG_CHAR szFlags[50];
    IMG_INT32 i32Pos = 0;
    IMG_UINT32 ui32CacheTypeIndex, ui32MapTypeIndex;
    IMG_CHAR *apszCacheTypes[] = {
        "UNCACHED",
        "CACHED",
        "WRITECOMBINE",
        "UNKNOWN"
    };
    IMG_CHAR *apszMapType[] = {
        "KERNEL_ONLY",
        "SINGLE_PROCESS",
        "MULTI_PROCESS",
        "FROM_EXISTING_PROCESS",
        "NO_CPU_VIRTUAL",
        "UNKNOWN"
    };


    if(ui32Flags & PVRSRV_HAP_UNCACHED){
        ui32CacheTypeIndex=0;
    }else if(ui32Flags & PVRSRV_HAP_CACHED){
        ui32CacheTypeIndex=1;
    }else if(ui32Flags & PVRSRV_HAP_WRITECOMBINE){
        ui32CacheTypeIndex=2;
    }else{
        ui32CacheTypeIndex=3;
        PVR_DPF((PVR_DBG_ERROR, "%s: unknown cache type (%u)",
                 __FUNCTION__, (ui32Flags & PVRSRV_HAP_CACHETYPE_MASK)));
    }


    if(ui32Flags & PVRSRV_HAP_KERNEL_ONLY){
        ui32MapTypeIndex = 0;
    }else if(ui32Flags & PVRSRV_HAP_SINGLE_PROCESS){
        ui32MapTypeIndex = 1;
    }else if(ui32Flags & PVRSRV_HAP_MULTI_PROCESS){
        ui32MapTypeIndex = 2;
    }else if(ui32Flags & PVRSRV_HAP_FROM_EXISTING_PROCESS){
        ui32MapTypeIndex = 3;
    }else if(ui32Flags & PVRSRV_HAP_NO_CPU_VIRTUAL){
        ui32MapTypeIndex = 4;
    }else{
        ui32MapTypeIndex = 5;
        PVR_DPF((PVR_DBG_ERROR, "%s: unknown map type (%u)",
                 __FUNCTION__, (ui32Flags & PVRSRV_HAP_MAPTYPE_MASK)));
    }

    i32Pos = sprintf(szFlags, "%s|", apszCacheTypes[ui32CacheTypeIndex]);
    if (i32Pos <= 0)
    {
	PVR_DPF((PVR_DBG_ERROR, "%s: sprintf for cache type %u failed (%d)",
		__FUNCTION__, ui32CacheTypeIndex, i32Pos));
	szFlags[0] = 0;
    }
    else
    {
        sprintf(szFlags + i32Pos, "%s", apszMapType[ui32MapTypeIndex]);
    }

    return szFlags;
}
#endif

