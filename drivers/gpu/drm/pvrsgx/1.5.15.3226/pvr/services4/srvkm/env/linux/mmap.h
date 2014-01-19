/**********************************************************************
 Copyright (c) Imagination Technologies Ltd.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ******************************************************************************/

#if !defined(__MMAP_H__)
#define __MMAP_H__

#include <linux/mm.h>
#include <linux/list.h>

#include "perproc.h"
#include "mm.h"

typedef struct KV_OFFSET_STRUCT_TAG
{

    IMG_UINT32			ui32Mapped;


    IMG_UINT32                  ui32MMapOffset;

    IMG_UINT32			ui32RealByteSize;


    LinuxMemArea                *psLinuxMemArea;


    IMG_UINT32			ui32TID;


    IMG_UINT32			ui32PID;


    IMG_BOOL			bOnMMapList;


    IMG_UINT32			ui32RefCount;


    IMG_UINT32			ui32UserVAddr;


#if defined(DEBUG_LINUX_MMAP_AREAS)
    const IMG_CHAR		*pszName;
#endif


   struct list_head		sMMapItem;


   struct list_head		sAreaItem;
}KV_OFFSET_STRUCT, *PKV_OFFSET_STRUCT;



IMG_VOID PVRMMapInit(IMG_VOID);


IMG_VOID PVRMMapCleanup(IMG_VOID);


PVRSRV_ERROR PVRMMapRegisterArea(LinuxMemArea *psLinuxMemArea);


PVRSRV_ERROR PVRMMapRemoveRegisteredArea(LinuxMemArea *psLinuxMemArea);


PVRSRV_ERROR PVRMMapOSMemHandleToMMapData(PVRSRV_PER_PROCESS_DATA *psPerProc,
					     IMG_HANDLE hMHandle,
                                             IMG_UINT32 *pui32MMapOffset,
                                             IMG_UINT32 *pui32ByteOffset,
                                             IMG_UINT32 *pui32RealByteSize,						     IMG_UINT32 *pui32UserVAddr);

PVRSRV_ERROR
PVRMMapReleaseMMapData(PVRSRV_PER_PROCESS_DATA *psPerProc,
				IMG_HANDLE hMHandle,
				IMG_BOOL *pbMUnmap,
				IMG_UINT32 *pui32RealByteSize,
                                IMG_UINT32 *pui32UserVAddr);

int PVRMMap(struct file* pFile, struct vm_area_struct* ps_vma);


#endif

