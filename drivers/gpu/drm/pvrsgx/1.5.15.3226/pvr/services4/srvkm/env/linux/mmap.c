/*************************************************************************/ /*!
@Title          Linux mmap interface
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
#include <linux/module.h>
#include <linux/vmalloc.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
#include <linux/wrapper.h>
#endif
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/shmparam.h>
#include <asm/pgtable.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22))
#include <linux/sched.h>
#include <asm/current.h>
#endif
#if defined(SUPPORT_DRI_DRM)
#include <drm/drmP.h>
#endif

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
#include "mutex.h"
#include "handle.h"
#include "perproc.h"
#include "env_perproc.h"
#include "bridged_support.h"
#if defined(SUPPORT_DRI_DRM)
#include "pvr_drm.h"
#endif

#if !defined(PVR_SECURE_HANDLES)
#error "The mmap code requires PVR_SECURE_HANDLES"
#endif

/* WARNING:
 * The mmap code has its own mutex, to prevent a possible deadlock,
 * when using gPVRSRVLock.
 * The Linux kernel takes the mm->mmap_sem before calling the mmap
 * entry points (PVRMMap, MMapVOpen, MMapVClose), but the ioctl
 * entry point may take mm->mmap_sem during fault handling, or 
 * before calling get_user_pages.  If gPVRSRVLock was used in the
 * mmap entry points, a deadlock could result, due to the ioctl
 * and mmap code taking the two locks in different orders.
 * As a corollary to this, the mmap entry points must not call
 * any driver code that relies on gPVRSRVLock is held.
 */
static struct mutex g_sMMapMutex;

static LinuxKMemCache *g_psMemmapCache = NULL;
static LIST_HEAD(g_sMMapAreaList);
static LIST_HEAD(g_sMMapOffsetStructList);
#if defined(DEBUG_LINUX_MMAP_AREAS)
static IMG_UINT32 g_ui32RegisteredAreas = 0;
static IMG_UINT32 g_ui32TotalByteSize = 0;
#endif


#if defined(PVR_PROC_USE_SEQ_FILE) && defined(DEBUG_LINUX_MMAP_AREAS)
static struct proc_dir_entry *g_ProcMMap;
#endif

#define	FIRST_PHYSICAL_PFN	0
#define	LAST_PHYSICAL_PFN	0x7fffffffUL
#define	FIRST_SPECIAL_PFN	(LAST_PHYSICAL_PFN + 1)
#define	LAST_SPECIAL_PFN	0xffffffffUL

#define	MAX_MMAP_HANDLE		0x7fffffffUL

static inline IMG_BOOL
PFNIsPhysical(IMG_UINT32 pfn)
{

	return ((pfn >= FIRST_PHYSICAL_PFN) && (pfn <= LAST_PHYSICAL_PFN)) ? IMG_TRUE : IMG_FALSE;
}

static inline IMG_BOOL
PFNIsSpecial(IMG_UINT32 pfn)
{

	return ((pfn >= FIRST_SPECIAL_PFN) && (pfn <= LAST_SPECIAL_PFN)) ? IMG_TRUE : IMG_FALSE;
}

static inline IMG_HANDLE
MMapOffsetToHandle(IMG_UINT32 pfn)
{
	if (PFNIsPhysical(pfn))
	{
		PVR_ASSERT(PFNIsPhysical(pfn));
		return IMG_NULL;
	}

	return (IMG_HANDLE)(pfn - FIRST_SPECIAL_PFN);
}

static inline IMG_UINT32
HandleToMMapOffset(IMG_HANDLE hHandle)
{
	IMG_UINT32 ulHandle = (IMG_UINT32)hHandle;

	if (PFNIsSpecial(ulHandle))
	{
		PVR_ASSERT(PFNIsSpecial(ulHandle));
		return 0;
	}

	return ulHandle + FIRST_SPECIAL_PFN;
}

static inline IMG_BOOL
LinuxMemAreaUsesPhysicalMap(LinuxMemArea *psLinuxMemArea)
{
    return LinuxMemAreaPhysIsContig(psLinuxMemArea);
}

static inline IMG_UINT32
GetCurrentThreadID(IMG_VOID)
{
	/*
 	 * The PID is the thread ID, as each thread is a
 	 * seperate process.
 	 */
	return (IMG_UINT32)current->pid;
}

/*
 * Create an offset structure, which is used to hold per-process
 * mmap data.
 */
static PKV_OFFSET_STRUCT
CreateOffsetStruct(LinuxMemArea *psLinuxMemArea, IMG_UINT32 ui32Offset, IMG_UINT32 ui32RealByteSize)
{
    PKV_OFFSET_STRUCT psOffsetStruct;
#if defined(DEBUG) || defined(DEBUG_LINUX_MMAP_AREAS)
    const IMG_CHAR *pszName = LinuxMemAreaTypeToString(LinuxMemAreaRootType(psLinuxMemArea));
#endif

#if defined(DEBUG) || defined(DEBUG_LINUX_MMAP_AREAS)
    PVR_DPF((PVR_DBG_MESSAGE,
             "%s(%s, psLinuxMemArea: 0x%p, ui32AllocFlags: 0x%8lx)",
             __FUNCTION__, pszName, psLinuxMemArea, psLinuxMemArea->ui32AreaFlags));
#endif

    PVR_ASSERT(psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC || LinuxMemAreaRoot(psLinuxMemArea)->eAreaType != LINUX_MEM_AREA_SUB_ALLOC);

    PVR_ASSERT(psLinuxMemArea->bMMapRegistered);

    psOffsetStruct = KMemCacheAllocWrapper(g_psMemmapCache, GFP_KERNEL);
    if(psOffsetStruct == IMG_NULL)
    {
        PVR_DPF((PVR_DBG_ERROR,"PVRMMapRegisterArea: Couldn't alloc another mapping record from cache"));
        return IMG_NULL;
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
    /* Extra entries to support proc filesystem debug info */
    psOffsetStruct->pszName = pszName;
#endif

    list_add_tail(&psOffsetStruct->sAreaItem, &psLinuxMemArea->sMMapOffsetStructList);

    return psOffsetStruct;
}


static IMG_VOID
DestroyOffsetStruct(PKV_OFFSET_STRUCT psOffsetStruct)
{
    list_del(&psOffsetStruct->sAreaItem);

    if (psOffsetStruct->bOnMMapList)
    {
        list_del(&psOffsetStruct->sMMapItem);
    }

    PVR_DPF((PVR_DBG_MESSAGE, "%s: Table entry: "
             "psLinuxMemArea=0x%08lX, CpuPAddr=0x%08lX", __FUNCTION__,
             psOffsetStruct->psLinuxMemArea,
             LinuxMemAreaToCpuPAddr(psOffsetStruct->psLinuxMemArea, 0)));

    KMemCacheFreeWrapper(g_psMemmapCache, psOffsetStruct);
}


/*
 * There are no alignment constraints for mapping requests made by user
 * mode Services.  For this, and potentially other reasons, the
 * mapping created for a users request may look different to the
 * original request in terms of size and alignment.
 *
 * This function determines an offset that the user can add to the mapping
 * that is _actually_ created which will point to the memory they are
 * _really_ interested in.
 *
 */
static inline IMG_VOID
DetermineUsersSizeAndByteOffset(LinuxMemArea *psLinuxMemArea,
                               IMG_UINT32 *pui32RealByteSize,
                               IMG_UINT32 *pui32ByteOffset)
{
    IMG_UINT32 ui32PageAlignmentOffset;
    IMG_CPU_PHYADDR CpuPAddr;

    CpuPAddr = LinuxMemAreaToCpuPAddr(psLinuxMemArea, 0);
    ui32PageAlignmentOffset = ADDR_TO_PAGE_OFFSET(CpuPAddr.uiAddr);

    *pui32ByteOffset = ui32PageAlignmentOffset;

    *pui32RealByteSize = PAGE_ALIGN(psLinuxMemArea->ui32ByteSize + ui32PageAlignmentOffset);
}


PVRSRV_ERROR
PVRMMapOSMemHandleToMMapData(PVRSRV_PER_PROCESS_DATA *psPerProc,
				IMG_HANDLE hMHandle,
                                IMG_UINT32 *pui32MMapOffset,
                                IMG_UINT32 *pui32ByteOffset,
                                IMG_UINT32 *pui32RealByteSize,
				IMG_UINT32 *pui32UserVAddr)
{
    LinuxMemArea *psLinuxMemArea;
    PKV_OFFSET_STRUCT psOffsetStruct;
    IMG_HANDLE hOSMemHandle;
    PVRSRV_ERROR eError;

    mutex_lock(&g_sMMapMutex);

    PVR_ASSERT(PVRSRVGetMaxHandle(psPerProc->psHandleBase) <= MAX_MMAP_HANDLE);

    eError = PVRSRVLookupOSMemHandle(psPerProc->psHandleBase, &hOSMemHandle, hMHandle);
    if (eError != PVRSRV_OK)
    {
	PVR_DPF((PVR_DBG_ERROR, "%s: Lookup of handle 0x%lx failed", __FUNCTION__, hMHandle));

	goto exit_unlock;
    }

    psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;

    DetermineUsersSizeAndByteOffset(psLinuxMemArea,
                                   pui32RealByteSize,
                                   pui32ByteOffset);


    list_for_each_entry(psOffsetStruct, &psLinuxMemArea->sMMapOffsetStructList, sAreaItem)
    {
        if (psPerProc->ui32PID == psOffsetStruct->ui32PID)
        {

	   PVR_ASSERT(*pui32RealByteSize == psOffsetStruct->ui32RealByteSize);

	   *pui32MMapOffset = psOffsetStruct->ui32MMapOffset;
	   *pui32UserVAddr = psOffsetStruct->ui32UserVAddr;
	   psOffsetStruct->ui32RefCount++;

	   eError = PVRSRV_OK;
	   goto exit_unlock;
        }
    }

    /* Memory area won't have been mapped yet */
    *pui32UserVAddr = 0;

    if (LinuxMemAreaUsesPhysicalMap(psLinuxMemArea))
    {
        *pui32MMapOffset = LinuxMemAreaToCpuPFN(psLinuxMemArea, 0);
        PVR_ASSERT(PFNIsPhysical(*pui32MMapOffset));
    }
    else
    {
        *pui32MMapOffset = HandleToMMapOffset(hMHandle);
        PVR_ASSERT(PFNIsSpecial(*pui32MMapOffset));
    }

    psOffsetStruct = CreateOffsetStruct(psLinuxMemArea, *pui32MMapOffset, *pui32RealByteSize);
    if (psOffsetStruct == IMG_NULL)
    {
        eError = PVRSRV_ERROR_OUT_OF_MEMORY;
	goto exit_unlock;
    }

    /*
    * Offset structures representing physical mappings are added to
    * a list, so that they can be located when the memory area is mapped.
    */
    list_add_tail(&psOffsetStruct->sMMapItem, &g_sMMapOffsetStructList);

    psOffsetStruct->bOnMMapList = IMG_TRUE;

    psOffsetStruct->ui32RefCount++;

    eError = PVRSRV_OK;

exit_unlock:
    mutex_unlock(&g_sMMapMutex);

    return eError;
}


/*!
 *******************************************************************************

 @Function  PVRMMapReleaseMMapData

 @Description

 Release mmap data.

 @input psPerProc : Per-process data.
 @input hMHandle : Memory handle.
 @input pbMUnmap : pointer to location for munmap flag.
 @input pui32UserVAddr : pointer to location for user mode address of mapping.
 @input pui32ByteSize : pointer to location for size of mapping.

 @Output pbMUnmap : points to flag that indicates whether an munmap is
		    required.
 @output pui32UserVAddr : points to user mode address to munmap.

 @Return PVRSRV_ERROR : PVRSRV_OK, or error code.

 ******************************************************************************/
PVRSRV_ERROR
PVRMMapReleaseMMapData(PVRSRV_PER_PROCESS_DATA *psPerProc,
				IMG_HANDLE hMHandle,
				IMG_BOOL *pbMUnmap,
				IMG_UINT32 *pui32RealByteSize,
                                IMG_UINT32 *pui32UserVAddr)
{
    LinuxMemArea *psLinuxMemArea;
    PKV_OFFSET_STRUCT psOffsetStruct;
    IMG_HANDLE hOSMemHandle;
    PVRSRV_ERROR eError;
    IMG_UINT32 ui32PID = OSGetCurrentProcessIDKM();

    mutex_lock(&g_sMMapMutex);

    PVR_ASSERT(PVRSRVGetMaxHandle(psPerProc->psHandleBase) <= MAX_MMAP_HANDLE);

    eError = PVRSRVLookupOSMemHandle(psPerProc->psHandleBase, &hOSMemHandle, hMHandle);
    if (eError != PVRSRV_OK)
    {
	PVR_DPF((PVR_DBG_ERROR, "%s: Lookup of handle 0x%lx failed", __FUNCTION__, hMHandle));

	goto exit_unlock;
    }

    psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;


    list_for_each_entry(psOffsetStruct, &psLinuxMemArea->sMMapOffsetStructList, sAreaItem)
    {
        if (psOffsetStruct->ui32PID == ui32PID)
        {
	    if (psOffsetStruct->ui32RefCount == 0)
	    {
		PVR_DPF((PVR_DBG_ERROR, "%s: Attempt to release mmap data with zero reference count for offset struct 0x%p, memory area 0x%p", __FUNCTION__, psOffsetStruct, psLinuxMemArea));
		eError = PVRSRV_ERROR_GENERIC;
		goto exit_unlock;
	    }

	    psOffsetStruct->ui32RefCount--;

	    *pbMUnmap = (IMG_BOOL)((psOffsetStruct->ui32RefCount == 0) && (psOffsetStruct->ui32UserVAddr != 0));

	    *pui32UserVAddr = (*pbMUnmap) ? psOffsetStruct->ui32UserVAddr : 0;
	    *pui32RealByteSize = (*pbMUnmap) ? psOffsetStruct->ui32RealByteSize : 0;

	    eError = PVRSRV_OK;
	    goto exit_unlock;
        }
    }


    PVR_DPF((PVR_DBG_ERROR, "%s: Mapping data not found for handle 0x%lx (memory area 0x%p)", __FUNCTION__, hMHandle, psLinuxMemArea));

    eError =  PVRSRV_ERROR_GENERIC;

exit_unlock:
    mutex_unlock(&g_sMMapMutex);

    return eError;
}

static inline PKV_OFFSET_STRUCT
FindOffsetStructByOffset(IMG_UINT32 ui32Offset, IMG_UINT32 ui32RealByteSize)
{
    PKV_OFFSET_STRUCT psOffsetStruct;
    IMG_UINT32 ui32TID = GetCurrentThreadID();
    IMG_UINT32 ui32PID = OSGetCurrentProcessIDKM();

    list_for_each_entry(psOffsetStruct, &g_sMMapOffsetStructList, sMMapItem)
    {
        if (ui32Offset == psOffsetStruct->ui32MMapOffset && ui32RealByteSize == psOffsetStruct->ui32RealByteSize && psOffsetStruct->ui32PID == ui32PID)
        {

	    if (!PFNIsPhysical(ui32Offset) || psOffsetStruct->ui32TID == ui32TID)
	    {
	        return psOffsetStruct;
	    }
        }
    }

    return IMG_NULL;
}


static IMG_BOOL
DoMapToUser(LinuxMemArea *psLinuxMemArea,
            struct vm_area_struct* ps_vma,
            IMG_UINT32 ui32ByteOffset)
{
    IMG_UINT32 ui32ByteSize;

    if (psLinuxMemArea->eAreaType == LINUX_MEM_AREA_SUB_ALLOC)
    {
        return DoMapToUser(LinuxMemAreaRoot(psLinuxMemArea),
                    ps_vma,
                    psLinuxMemArea->uData.sSubAlloc.ui32ByteOffset + ui32ByteOffset);
    }

    /*
     * Note that ui32ByteSize may be larger than the size of the memory
     * area being mapped, as the former is a multiple of the page size.
     */
    ui32ByteSize = ps_vma->vm_end - ps_vma->vm_start;
    PVR_ASSERT(ADDR_TO_PAGE_OFFSET(ui32ByteSize) == 0);

#if defined (__sparc__)
    /*
     * For LINUX_MEM_AREA_EXTERNAL_KV, we don't know where the address range
     * we are being asked to map has come from, that is, whether it is memory
     * or I/O.  For all architectures other than SPARC, there is no distinction.
     * Since we don't currently support SPARC, we won't worry about it.
     */
#error "SPARC not supported"
#endif

    if (PFNIsPhysical(ps_vma->vm_pgoff))
    {
	IMG_INT result;

	PVR_ASSERT(LinuxMemAreaPhysIsContig(psLinuxMemArea));
	PVR_ASSERT(LinuxMemAreaToCpuPFN(psLinuxMemArea, ui32ByteOffset) == ps_vma->vm_pgoff);


	result = IO_REMAP_PFN_RANGE(ps_vma, ps_vma->vm_start, ps_vma->vm_pgoff, ui32ByteSize, ps_vma->vm_page_prot);

        if(result == 0)
        {
            return IMG_TRUE;
        }

        PVR_DPF((PVR_DBG_MESSAGE, "%s: Failed to map contiguous physical address range (%d), trying non-contiguous path", __FUNCTION__, result));
    }

    {
        /*
         * Memory may be non-contiguous, so we map the range page,
	 * by page.  Since VM_PFNMAP mappings are assumed to be physically
	 * contiguous, we can't legally use REMAP_PFN_RANGE (that is, we
	 * could, but the resulting VMA may confuse other bits of the kernel
	 * that attempt to interpret it).
	 * The only alternative is to use VM_INSERT_PAGE, which requires
	 * finding the page structure corresponding to each page, or
	 * if mixed maps are supported (VM_MIXEDMAP), vm_insert_mixed.
	 */
        IMG_UINT32 ulVMAPos;
	IMG_UINT32 ui32ByteEnd = ui32ByteOffset + ui32ByteSize;
	IMG_UINT32 ui32PA;


	for(ui32PA = ui32ByteOffset; ui32PA < ui32ByteEnd; ui32PA += PAGE_SIZE)
	{
	    IMG_UINT32 pfn =  LinuxMemAreaToCpuPFN(psLinuxMemArea, ui32PA);

	    if (!pfn_valid(pfn))
	    {
                PVR_DPF((PVR_DBG_ERROR,"%s: Error - PFN invalid: 0x%lx", __FUNCTION__, pfn));
                return IMG_FALSE;
	    }
	}


        ulVMAPos = ps_vma->vm_start;
	for(ui32PA = ui32ByteOffset; ui32PA < ui32ByteEnd; ui32PA += PAGE_SIZE)
	{
	    IMG_UINT32 pfn;
	    struct page *psPage;
	    IMG_INT result;

	    pfn =  LinuxMemAreaToCpuPFN(psLinuxMemArea, ui32PA);
	    PVR_ASSERT(pfn_valid(pfn));

	    psPage = pfn_to_page(pfn);

	    result = VM_INSERT_PAGE(ps_vma,  ulVMAPos, psPage);
            if(result != 0)
            {
                PVR_DPF((PVR_DBG_ERROR,"%s: Error - VM_INSERT_PAGE failed (%d)", __FUNCTION__, result));
                return IMG_FALSE;
            }
            ulVMAPos += PAGE_SIZE;
        }
    }

    return IMG_TRUE;
}


static IMG_VOID
MMapVOpenNoLock(struct vm_area_struct* ps_vma)
{
    PKV_OFFSET_STRUCT psOffsetStruct = (PKV_OFFSET_STRUCT)ps_vma->vm_private_data;
    PVR_ASSERT(psOffsetStruct != IMG_NULL)
    psOffsetStruct->ui32Mapped++;
    PVR_ASSERT(!psOffsetStruct->bOnMMapList);

    if (psOffsetStruct->ui32Mapped > 1)
    {
	PVR_DPF((PVR_DBG_WARNING, "%s: Offset structure 0x%p is being shared across processes (psOffsetStruct->ui32Mapped: %lu)", __FUNCTION__, psOffsetStruct, psOffsetStruct->ui32Mapped));
        PVR_ASSERT((ps_vma->vm_flags & VM_DONTCOPY) == 0);
    }

#if defined(DEBUG_LINUX_MMAP_AREAS)

    PVR_DPF((PVR_DBG_MESSAGE,
             "%s: psLinuxMemArea 0x%p, KVAddress 0x%p MMapOffset %ld, ui32Mapped %d",
             __FUNCTION__,
             psOffsetStruct->psLinuxMemArea,
             LinuxMemAreaToCpuVAddr(psOffsetStruct->psLinuxMemArea),
             psOffsetStruct->ui32MMapOffset,
             psOffsetStruct->ui32Mapped));
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
    MOD_INC_USE_COUNT;
#endif
}


/*
 * Linux mmap open entry point.
 */
static void
MMapVOpen(struct vm_area_struct* ps_vma)
{
    mutex_lock(&g_sMMapMutex);

    MMapVOpenNoLock(ps_vma);

    mutex_unlock(&g_sMMapMutex);
}


static IMG_VOID
MMapVCloseNoLock(struct vm_area_struct* ps_vma)
{
    PKV_OFFSET_STRUCT psOffsetStruct = (PKV_OFFSET_STRUCT)ps_vma->vm_private_data;
    PVR_ASSERT(psOffsetStruct != IMG_NULL)

#if defined(DEBUG_LINUX_MMAP_AREAS)
    PVR_DPF((PVR_DBG_MESSAGE,
             "%s: psLinuxMemArea 0x%p, CpuVAddr 0x%p ui32MMapOffset %ld, ui32Mapped %d",
             __FUNCTION__,
             psOffsetStruct->psLinuxMemArea,
             LinuxMemAreaToCpuVAddr(psOffsetStruct->psLinuxMemArea),
             psOffsetStruct->ui32MMapOffset,
             psOffsetStruct->ui32Mapped));
#endif

    PVR_ASSERT(!psOffsetStruct->bOnMMapList);
    psOffsetStruct->ui32Mapped--;
    if (psOffsetStruct->ui32Mapped == 0)
    {
	if (psOffsetStruct->ui32RefCount != 0)
	{
	        PVR_DPF((PVR_DBG_MESSAGE, "%s: psOffsetStruct 0x%p has non-zero reference count (ui32RefCount = %lu). User mode address of start of mapping: 0x%lx", __FUNCTION__, psOffsetStruct, psOffsetStruct->ui32RefCount, psOffsetStruct->ui32UserVAddr));
	}

	DestroyOffsetStruct(psOffsetStruct);
    }

    ps_vma->vm_private_data = NULL;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
    MOD_DEC_USE_COUNT;
#endif
}

/*
 * Linux mmap close entry point.
 */
static void
MMapVClose(struct vm_area_struct* ps_vma)
{
    mutex_lock(&g_sMMapMutex);

    MMapVCloseNoLock(ps_vma);

    mutex_unlock(&g_sMMapMutex);
}


static struct vm_operations_struct MMapIOOps =
{
	.open=MMapVOpen,
	.close=MMapVClose
};


/*!
 *******************************************************************************

 @Function  PVRMMap

 @Description

 Driver mmap entry point.

 @input pFile : unused.
 @input ps_vma : pointer to linux memory area descriptor.

 @Return 0, or Linux error code.

 ******************************************************************************/
int
PVRMMap(struct file* pFile, struct vm_area_struct* ps_vma)
{
    IMG_UINT32 ui32ByteSize;
    PKV_OFFSET_STRUCT psOffsetStruct;
    int iRetVal = 0;

    PVR_UNREFERENCED_PARAMETER(pFile);

    mutex_lock(&g_sMMapMutex);

    ui32ByteSize = ps_vma->vm_end - ps_vma->vm_start;


    PVR_DPF((PVR_DBG_MESSAGE, "%s: Received mmap(2) request with ui32MMapOffset 0x%08lx,"
                              " and ui32ByteSize %ld(0x%08lx)",
            __FUNCTION__,
            ps_vma->vm_pgoff,
            ui32ByteSize, ui32ByteSize));

    psOffsetStruct = FindOffsetStructByOffset(ps_vma->vm_pgoff, ui32ByteSize);
    if (psOffsetStruct == IMG_NULL)
    {
#if defined(SUPPORT_DRI_DRM)
        mutex_unlock(&g_sMMapMutex);


        return drm_mmap(pFile, ps_vma);
#else
        PVR_UNREFERENCED_PARAMETER(pFile);

#if 0 /* FIXME: crash when call to print debug messages */
        PVR_DPF((PVR_DBG_ERROR,
             "%s: Attempted to mmap unregistered area at vm_pgoff %ld",
             __FUNCTION__, ps_vma->vm_pgoff));
#endif
        iRetVal = -EINVAL;
#endif
        goto unlock_and_return;
    }

    list_del(&psOffsetStruct->sMMapItem);
    psOffsetStruct->bOnMMapList = IMG_FALSE;


    if (((ps_vma->vm_flags & VM_WRITE) != 0) &&
        ((ps_vma->vm_flags & VM_SHARED) == 0))
    {
#if 0 /* FIXME: crash when call to print debug messages */
        PVR_DPF((PVR_DBG_ERROR, "%s: Cannot mmap non-shareable writable areas", __FUNCTION__));
#endif
        iRetVal = -EINVAL;
        goto unlock_and_return;
    }


#if 0 /* FIXME: crash when call to print debug messages */
    PVR_DPF((PVR_DBG_MESSAGE, "%s: Mapped psLinuxMemArea 0x%p\n",
         __FUNCTION__, psOffsetStruct->psLinuxMemArea));
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0)
    ps_vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
#else
    ps_vma->vm_flags |= VM_RESERVED;
#endif
    ps_vma->vm_flags |= VM_IO;

    /*
     * Disable mremap because our nopage handler assumes all
     * page requests have already been validated.
     */
    ps_vma->vm_flags |= VM_DONTEXPAND;
    
    /* Don't allow mapping to be inherited across a process fork */
    ps_vma->vm_flags |= VM_DONTCOPY;

    ps_vma->vm_private_data = (void *)psOffsetStruct;
    
    switch(psOffsetStruct->psLinuxMemArea->ui32AreaFlags & PVRSRV_HAP_CACHETYPE_MASK)
    {
        case PVRSRV_HAP_CACHED:

            break;
        case PVRSRV_HAP_WRITECOMBINE:
	    ps_vma->vm_page_prot = PGPROT_WC(ps_vma->vm_page_prot);
            break;
        case PVRSRV_HAP_UNCACHED:
            ps_vma->vm_page_prot = PGPROT_UC(ps_vma->vm_page_prot);
            break;
        default:
            PVR_DPF((PVR_DBG_ERROR, "%s: unknown cache type", __FUNCTION__));
	    iRetVal = -EINVAL;
	    goto unlock_and_return;
    }
    
    /* Install open and close handlers for ref-counting */
    ps_vma->vm_ops = &MMapIOOps;
    
    if(!DoMapToUser(psOffsetStruct->psLinuxMemArea, ps_vma, 0))
    {
        iRetVal = -EAGAIN;
        goto unlock_and_return;
    }
    
    PVR_ASSERT(psOffsetStruct->ui32UserVAddr == 0);

    psOffsetStruct->ui32UserVAddr = ps_vma->vm_start;


    MMapVOpenNoLock(ps_vma);

#if 0 /* FIXME: crash when call to print debug messages */
    PVR_DPF((PVR_DBG_MESSAGE, "%s: Mapped area at offset 0x%08lx\n",
             __FUNCTION__, ps_vma->vm_pgoff));
#endif

unlock_and_return:
    if (iRetVal != 0 && psOffsetStruct != IMG_NULL)
    {
        DestroyOffsetStruct(psOffsetStruct);
    }

    mutex_unlock(&g_sMMapMutex);

    return iRetVal;
}


#if defined(DEBUG_LINUX_MMAP_AREAS)

#ifdef PVR_PROC_USE_SEQ_FILE

static void ProcSeqStartstopMMapRegistations(struct seq_file *sfile,IMG_BOOL start)
{
	if(start)
	{
	    mutex_lock(&g_sMMapMutex);
	}
	else
	{
	    mutex_unlock(&g_sMMapMutex);
	}
}


static void* ProcSeqOff2ElementMMapRegistrations(struct seq_file *sfile, loff_t off)
{
    LinuxMemArea *psLinuxMemArea;
	if(!off) 
	{
		return PVR_PROC_SEQ_START_TOKEN;
	}

    list_for_each_entry(psLinuxMemArea, &g_sMMapAreaList, sMMapItem)
    {
        PKV_OFFSET_STRUCT psOffsetStruct;

	 	list_for_each_entry(psOffsetStruct, &psLinuxMemArea->sMMapOffsetStructList, sAreaItem)
        {
	    	off--;
	    	if (off == 0)
	    	{				
				PVR_ASSERT(psOffsetStruct->psLinuxMemArea == psLinuxMemArea);
				return (void*)psOffsetStruct;
		    }
        }
    }
	return (void*)0;
}

/*
 * Gets next MMap element to show. (called when reading /proc/mmap file)

 * sfile : seq_file that handles /proc file
 * el : actual element
 * off : index into the KVOffsetTable from which to print
 *  
 * returns void* : Pointer to element to show (0 ends iteration)
*/
static void* ProcSeqNextMMapRegistrations(struct seq_file *sfile,void* el,loff_t off)
{
	return ProcSeqOff2ElementMMapRegistrations(sfile,off);
}


static void ProcSeqShowMMapRegistrations(struct seq_file *sfile,void* el)
{
	KV_OFFSET_STRUCT *psOffsetStruct = (KV_OFFSET_STRUCT*)el;
    LinuxMemArea *psLinuxMemArea;
	IMG_UINT32 ui32RealByteSize;
	IMG_UINT32 ui32ByteOffset;

	if(el == PVR_PROC_SEQ_START_TOKEN) 
	{
        seq_printf( sfile,
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
						  "Allocations registered for mmap: %lu\n"
                          "In total these areas correspond to %lu bytes\n"
                          "psLinuxMemArea "
						  "UserVAddr "
						  "KernelVAddr "
						  "CpuPAddr "
                          "MMapOffset "
                          "ByteLength "
                          "LinuxMemType             "
						  "Pid   Name     Flags\n",
#else
                          "<mmap_header>\n"
                          "\t<count>%lu</count>\n"
                          "\t<bytes>%lu</bytes>\n"
                          "</mmap_header>\n",
#endif
						  g_ui32RegisteredAreas,
                          g_ui32TotalByteSize
                          );
		return;
	}

   	psLinuxMemArea = psOffsetStruct->psLinuxMemArea;

	DetermineUsersSizeAndByteOffset(psLinuxMemArea,
									&ui32RealByteSize,
									&ui32ByteOffset);

	seq_printf( sfile,
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
						"%-8p       %08lx %-8p %08lx %08lx   %-8ld   %-24s %-5lu %-8s %08lx(%s)\n",
#else
                        "<mmap_record>\n"
						"\t<pointer>%-8p</pointer>\n"
                        "\t<user_virtual>%-8lx</user_virtual>\n"
                        "\t<kernel_virtual>%-8p</kernel_virtual>\n"
                        "\t<cpu_physical>%08lx</cpu_physical>\n"
                        "\t<mmap_offset>%08lx</mmap_offset>\n"
                        "\t<bytes>%-8ld</bytes>\n"
                        "\t<linux_mem_area_type>%-24s</linux_mem_area_type>\n"
                        "\t<pid>%-5lu</pid>\n"
                        "\t<name>%-8s</name>\n"
                        "\t<flags>%08lx</flags>\n"
                        "\t<flags_string>%s</flags_string>\n"
                        "</mmap_record>\n",
#endif
                        psLinuxMemArea,
						psOffsetStruct->ui32UserVAddr + ui32ByteOffset,
						LinuxMemAreaToCpuVAddr(psLinuxMemArea),
                        LinuxMemAreaToCpuPAddr(psLinuxMemArea,0).uiAddr,
						psOffsetStruct->ui32MMapOffset,
						psLinuxMemArea->ui32ByteSize,
                        LinuxMemAreaTypeToString(psLinuxMemArea->eAreaType),
						psOffsetStruct->ui32PID,
						psOffsetStruct->pszName,
						psLinuxMemArea->ui32AreaFlags,
                        HAPFlagsToString(psLinuxMemArea->ui32AreaFlags));
}

#else

static off_t
PrintMMapRegistrations(IMG_CHAR *buffer, size_t size, off_t off)
{
    LinuxMemArea *psLinuxMemArea;
    off_t Ret;

    mutex_lock(&g_sMMapMutex);

    if(!off)
    {
		Ret = printAppend(buffer, size, 0,
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
						  "Allocations registered for mmap: %lu\n"
                          "In total these areas correspond to %lu bytes\n"
                          "psLinuxMemArea "
						  "UserVAddr "
						  "KernelVAddr "
						  "CpuPAddr "
                          "MMapOffset "
                          "ByteLength "
                          "LinuxMemType             "
						  "Pid   Name     Flags\n",
#else
                          "<mmap_header>\n"
                          "\t<count>%lu</count>\n"
                          "\t<bytes>%lu</bytes>\n"
                          "</mmap_header>\n",
#endif
						  g_ui32RegisteredAreas,
                          g_ui32TotalByteSize
                          );

        goto unlock_and_return;
    }

    if (size < 135)
    {
		Ret = 0;
        goto unlock_and_return;
    }

    PVR_ASSERT(off != 0);
    list_for_each_entry(psLinuxMemArea, &g_sMMapAreaList, sMMapItem)
    {
        PKV_OFFSET_STRUCT psOffsetStruct;

	list_for_each_entry(psOffsetStruct, &psLinuxMemArea->sMMapOffsetStructList, sAreaItem)
        {
	    off--;
	    if (off == 0)
	    {
		IMG_UINT32 ui32RealByteSize;
		IMG_UINT32 ui32ByteOffset;

		PVR_ASSERT(psOffsetStruct->psLinuxMemArea == psLinuxMemArea);

		DetermineUsersSizeAndByteOffset(psLinuxMemArea,
                                   &ui32RealByteSize,
                                   &ui32ByteOffset);

                Ret =  printAppend (buffer, size, 0,
#if !defined(DEBUG_LINUX_XML_PROC_FILES)
						"%-8p       %08lx %-8p %08lx %08lx   %-8ld   %-24s %-5lu %-8s %08lx(%s)\n",
#else
                        "<mmap_record>\n"
						"\t<pointer>%-8p</pointer>\n"
                        "\t<user_virtual>%-8lx</user_virtual>\n"
                        "\t<kernel_virtual>%-8p</kernel_virtual>\n"
                        "\t<cpu_physical>%08lx</cpu_physical>\n"
                        "\t<mmap_offset>%08lx</mmap_offset>\n"
                        "\t<bytes>%-8ld</bytes>\n"
                        "\t<linux_mem_area_type>%-24s</linux_mem_area_type>\n"
                        "\t<pid>%-5lu</pid>\n"
                        "\t<name>%-8s</name>\n"
                        "\t<flags>%08lx</flags>\n"
                        "\t<flags_string>%s</flags_string>\n"
                        "</mmap_record>\n",
#endif
                        psLinuxMemArea,
			psOffsetStruct->ui32UserVAddr + ui32ByteOffset,
						LinuxMemAreaToCpuVAddr(psLinuxMemArea),
                        LinuxMemAreaToCpuPAddr(psLinuxMemArea,0).uiAddr,
						psOffsetStruct->ui32MMapOffset,
						psLinuxMemArea->ui32ByteSize,
                        LinuxMemAreaTypeToString(psLinuxMemArea->eAreaType),
						psOffsetStruct->ui32PID,
						psOffsetStruct->pszName,
						psLinuxMemArea->ui32AreaFlags,
                        HAPFlagsToString(psLinuxMemArea->ui32AreaFlags));
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
#endif


PVRSRV_ERROR
PVRMMapRegisterArea(LinuxMemArea *psLinuxMemArea)
{
    PVRSRV_ERROR eError;
#if defined(DEBUG) || defined(DEBUG_LINUX_MMAP_AREAS)
    const IMG_CHAR *pszName = LinuxMemAreaTypeToString(LinuxMemAreaRootType(psLinuxMemArea));
#endif

    mutex_lock(&g_sMMapMutex);

#if defined(DEBUG) || defined(DEBUG_LINUX_MMAP_AREAS)
    PVR_DPF((PVR_DBG_MESSAGE,
             "%s(%s, psLinuxMemArea 0x%p, ui32AllocFlags 0x%8lx)",
             __FUNCTION__, pszName, psLinuxMemArea,  psLinuxMemArea->ui32AreaFlags));
#endif

    PVR_ASSERT(psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC || LinuxMemAreaRoot(psLinuxMemArea)->eAreaType != LINUX_MEM_AREA_SUB_ALLOC);

    /* Check this mem area hasn't already been registered */
    if(psLinuxMemArea->bMMapRegistered)
    {
        PVR_DPF((PVR_DBG_ERROR, "%s: psLinuxMemArea 0x%p is already registered",
                __FUNCTION__, psLinuxMemArea));
        eError = PVRSRV_ERROR_INVALID_PARAMS;
	goto exit_unlock;
    }

    list_add_tail(&psLinuxMemArea->sMMapItem, &g_sMMapAreaList);

    psLinuxMemArea->bMMapRegistered = IMG_TRUE;

#if defined(DEBUG_LINUX_MMAP_AREAS)
    g_ui32RegisteredAreas++;
    /*
     * Sub memory areas are excluded from g_ui32TotalByteSize so that we
     * don't count memory twice, once for the parent and again for sub
     * allocationis.
     */
    if (psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC)
    {
        g_ui32TotalByteSize += psLinuxMemArea->ui32ByteSize;
    }
#endif

    eError = PVRSRV_OK;

exit_unlock:
    mutex_unlock(&g_sMMapMutex);

    return eError;
}


/*!
 *******************************************************************************

 @Function  PVRMMapRemoveRegisterArea

 @Description

 Unregister a memory area with the mmap code.

 @input psLinuxMemArea : pointer to memory area.

 @Return PVRSRV_OK, or PVRSRV_ERROR.

 ******************************************************************************/
PVRSRV_ERROR
PVRMMapRemoveRegisteredArea(LinuxMemArea *psLinuxMemArea)
{
    PVRSRV_ERROR eError;
    PKV_OFFSET_STRUCT psOffsetStruct, psTmpOffsetStruct;

    mutex_lock(&g_sMMapMutex);

    PVR_ASSERT(psLinuxMemArea->bMMapRegistered);

    list_for_each_entry_safe(psOffsetStruct, psTmpOffsetStruct, &psLinuxMemArea->sMMapOffsetStructList, sAreaItem)
    {
	if (psOffsetStruct->ui32Mapped != 0)
	{
	     PVR_DPF((PVR_DBG_ERROR, "%s: psOffsetStruct 0x%p for memory area 0x0x%p is still mapped; psOffsetStruct->ui32Mapped %lu",  __FUNCTION__, psOffsetStruct, psLinuxMemArea, psOffsetStruct->ui32Mapped));
		eError = PVRSRV_ERROR_GENERIC;
		goto exit_unlock;
	}
	else
	{
	      /*
	      * An offset structure is created when a call is made to get
	      * the mmap data for a physical mapping.  If the data is never
	      * used for mmap, we will be left with an umapped offset
	      * structure.
	      */
	     PVR_DPF((PVR_DBG_WARNING, "%s: psOffsetStruct 0x%p was never mapped",  __FUNCTION__, psOffsetStruct));
	}

	PVR_ASSERT((psOffsetStruct->ui32Mapped == 0) && psOffsetStruct->bOnMMapList);

	DestroyOffsetStruct(psOffsetStruct);
    }

    list_del(&psLinuxMemArea->sMMapItem);

    psLinuxMemArea->bMMapRegistered = IMG_FALSE;

#if defined(DEBUG_LINUX_MMAP_AREAS)
    g_ui32RegisteredAreas--;
    if (psLinuxMemArea->eAreaType != LINUX_MEM_AREA_SUB_ALLOC)
    {
        g_ui32TotalByteSize -= psLinuxMemArea->ui32ByteSize;
    }
#endif

    eError = PVRSRV_OK;

exit_unlock:
    mutex_unlock(&g_sMMapMutex);
    return eError;
}


/*!
 *******************************************************************************

 @Function  LinuxMMapPerProcessConnect

 @Description

 Per-process mmap initialisation code.

 @input psEnvPerProc : pointer to OS specific per-process data.

 @Return PVRSRV_OK, or PVRSRV_ERROR.

 ******************************************************************************/
PVRSRV_ERROR
LinuxMMapPerProcessConnect(PVRSRV_ENV_PER_PROCESS_DATA *psEnvPerProc)
{
    PVR_UNREFERENCED_PARAMETER(psEnvPerProc);

    return PVRSRV_OK;
}

/*!
 *******************************************************************************

 @Function  LinuxMMapPerProcessDisconnect

 @Description

 Per-process mmap deinitialisation code.

 @input psEnvPerProc : pointer to OS specific per-process data.

 ******************************************************************************/
IMG_VOID
LinuxMMapPerProcessDisconnect(PVRSRV_ENV_PER_PROCESS_DATA *psEnvPerProc)
{
    PKV_OFFSET_STRUCT psOffsetStruct, psTmpOffsetStruct;
    IMG_BOOL bWarn = IMG_FALSE;
    IMG_UINT32 ui32PID = OSGetCurrentProcessIDKM();

    PVR_UNREFERENCED_PARAMETER(psEnvPerProc);

    mutex_lock(&g_sMMapMutex);

    list_for_each_entry_safe(psOffsetStruct, psTmpOffsetStruct, &g_sMMapOffsetStructList, sMMapItem)
    {
	if (psOffsetStruct->ui32PID == ui32PID)
	{
	    if (!bWarn)
	    {
		PVR_DPF((PVR_DBG_WARNING, "%s: process has unmapped offset structures. Removing them", __FUNCTION__));
		bWarn = IMG_TRUE;
	    }
	    PVR_ASSERT(psOffsetStruct->ui32Mapped == 0);
	    PVR_ASSERT(psOffsetStruct->bOnMMapList);

	    DestroyOffsetStruct(psOffsetStruct);
	}
    }

    mutex_unlock(&g_sMMapMutex);
}


/*!
 *******************************************************************************

 @Function  LinuxMMapPerProcessHandleOptions

 @Description

 Set secure handle options required by mmap code.

 @input psHandleBase : pointer to handle base.

 @Return PVRSRV_OK, or PVRSRV_ERROR.

 ******************************************************************************/
PVRSRV_ERROR LinuxMMapPerProcessHandleOptions(PVRSRV_HANDLE_BASE *psHandleBase)
{
    PVRSRV_ERROR eError;

    eError = PVRSRVSetMaxHandle(psHandleBase, MAX_MMAP_HANDLE);
    if (eError != PVRSRV_OK)
    {
	PVR_DPF((PVR_DBG_ERROR,"%s: failed to set handle limit (%d)", __FUNCTION__, eError));
	return eError;
    }

    return eError;
}


/*!
 *******************************************************************************

 @Function  PVRMMapInit

 @Description

 MMap initialisation code

 ******************************************************************************/
IMG_VOID
PVRMMapInit(IMG_VOID)
{
    mutex_init(&g_sMMapMutex);

    g_psMemmapCache = KMemCacheCreateWrapper("img-mmap", sizeof(KV_OFFSET_STRUCT), 0, 0);
    if (!g_psMemmapCache)
    {
        PVR_DPF((PVR_DBG_ERROR,"%s: failed to allocate kmem_cache", __FUNCTION__));
	goto error;
    }

#if defined(DEBUG_LINUX_MMAP_AREAS)
#ifdef PVR_PROC_USE_SEQ_FILE
	g_ProcMMap = CreateProcReadEntrySeq("mmap", NULL,
						  ProcSeqNextMMapRegistrations,
						  ProcSeqShowMMapRegistrations,
						  ProcSeqOff2ElementMMapRegistrations,
						  ProcSeqStartstopMMapRegistations
						 );
#else
    CreateProcReadEntry("mmap", PrintMMapRegistrations);
#endif
#endif
    return;

error:
    PVRMMapCleanup();
    return;
}


/*!
 *******************************************************************************

 @Function  PVRMMapCleanup

 @Description

 Mmap deinitialisation code

 ******************************************************************************/
IMG_VOID
PVRMMapCleanup(IMG_VOID)
{
    PVRSRV_ERROR eError;

    if (!list_empty(&g_sMMapAreaList))
    {
	LinuxMemArea *psLinuxMemArea, *psTmpMemArea;

	PVR_DPF((PVR_DBG_ERROR, "%s: Memory areas are still registered with MMap", __FUNCTION__));
	
	PVR_TRACE(("%s: Unregistering memory areas", __FUNCTION__));
 	list_for_each_entry_safe(psLinuxMemArea, psTmpMemArea, &g_sMMapAreaList, sMMapItem)
	{
		eError = PVRMMapRemoveRegisteredArea(psLinuxMemArea);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: PVRMMapRemoveRegisteredArea failed (%d)", __FUNCTION__, eError));
		}
		PVR_ASSERT(eError == PVRSRV_OK);

		LinuxMemAreaDeepFree(psLinuxMemArea);
	}
    }
    PVR_ASSERT(list_empty((&g_sMMapAreaList)));

#if defined(DEBUG_LINUX_MMAP_AREAS)
#ifdef PVR_PROC_USE_SEQ_FILE
    RemoveProcEntrySeq(g_ProcMMap);
#else
    RemoveProcEntry("mmap");
#endif
#endif

    if(g_psMemmapCache)
    {
        KMemCacheDestroyWrapper(g_psMemmapCache);
        g_psMemmapCache = NULL;
    }
}
