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

#if !defined(__MMAP_H__)
#define __MMAP_H__

#include <linux/mm.h>
#include <linux/list.h>

#if defined(VM_MIXEDMAP)
#define	PVR_MAKE_ALL_PFNS_SPECIAL
#endif

#include "perproc.h"
#include "mm.h"

/*
 * This structure represents the relationship between an mmap2 file
 * offset and a LinuxMemArea for a given process.
 */
typedef struct KV_OFFSET_STRUCT_TAG
{
    /*
     * Mapping count.  Incremented when the mapping is created, and
     * if the mapping is inherited across a process fork.
     */
    IMG_UINT32			ui32Mapped;

    /*
     * Offset to be passed to mmap2 to map the associated memory area
     * into user space.  The offset may represent the page frame number
     * of the first page in the area (if the area is physically
     * contiguous), or it may represent the secure handle associated
     * with the area.
     */
    IMG_UINTPTR_T               uiMMapOffset;
    
    IMG_SIZE_T			uiRealByteSize;

    /* Memory area associated with this offset structure */
    LinuxMemArea                *psLinuxMemArea;
    
#if !defined(PVR_MAKE_ALL_PFNS_SPECIAL)
    /* ID of the thread that owns this structure */
    IMG_UINT32			ui32TID;
#endif

    /* ID of the process that owns this structure */
    IMG_UINT32			ui32PID;

    /*
     * For offsets that represent actual page frame numbers, this structure
     * is temporarily put on a list so that it can be found from the
     * driver mmap entry point.  This flag indicates the structure is
     * on the list.
     */
    IMG_BOOL			bOnMMapList;

    /* Reference count for this structure */
    IMG_UINT32			ui32RefCount;

    /*
     * User mode address of start of mapping.  This is not necessarily the
     * first user mode address of the memory area.
     */
    IMG_UINTPTR_T		uiUserVAddr;

    /* Extra entries to support proc filesystem debug info */
#if defined(DEBUG_LINUX_MMAP_AREAS)
    const IMG_CHAR		*pszName;
#endif
    
   /* List entry field for MMap list */
   struct list_head		sMMapItem;

   /* List entry field for per-memory area list */
   struct list_head		sAreaItem;
}KV_OFFSET_STRUCT, *PKV_OFFSET_STRUCT;



IMG_VOID PVRMMapInit(IMG_VOID);


IMG_VOID PVRMMapCleanup(IMG_VOID);


PVRSRV_ERROR PVRMMapRegisterArea(LinuxMemArea *psLinuxMemArea);


PVRSRV_ERROR PVRMMapRemoveRegisteredArea(LinuxMemArea *psLinuxMemArea);


PVRSRV_ERROR PVRMMapOSMemHandleToMMapData(PVRSRV_PER_PROCESS_DATA *psPerProc,
#if defined (SUPPORT_SID_INTERFACE)
                                          IMG_SID     hMHandle,
#else
                                          IMG_HANDLE hMHandle,
#endif
                                          IMG_UINTPTR_T *puiMMapOffset,
                                          IMG_UINTPTR_T *puiByteOffset,
                                          IMG_SIZE_T *puiRealByteSize,
                                          IMG_UINTPTR_T *puiUserVAddr);

PVRSRV_ERROR
PVRMMapReleaseMMapData(PVRSRV_PER_PROCESS_DATA *psPerProc,
#if defined (SUPPORT_SID_INTERFACE)
				IMG_SID   hMHandle,
#else
				IMG_HANDLE hMHandle,
#endif
				IMG_BOOL *pbMUnmap,
				IMG_SIZE_T *puiRealByteSize,
                                IMG_UINTPTR_T *puiUserVAddr);

int PVRMMap(struct file* pFile, struct vm_area_struct* ps_vma);


#endif	

