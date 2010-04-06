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

#if !defined(__MMAP_H__)
#define __MMAP_H__

#include <linux/mm.h>
#include <linux/list.h>

#include "perproc.h"
#include "mm.h"

struct KV_OFFSET_STRUCT {
	u32 ui32Mapped;
	u32 ui32MMapOffset;
	u32 ui32RealByteSize;
	struct LinuxMemArea *psLinuxMemArea;
	u32 ui32TID;
	u32 ui32PID;
	IMG_BOOL bOnMMapList;
	u32 ui32RefCount;
	u32 ui32UserVAddr;
#if defined(DEBUG_LINUX_MMAP_AREAS)
	const char *pszName;
#endif
	struct list_head sMMapItem;
	struct list_head sAreaItem;
};

void PVRMMapInit(void);
void PVRMMapCleanup(void);

enum PVRSRV_ERROR PVRMMapRegisterArea(struct LinuxMemArea *psLinuxMemArea);

enum PVRSRV_ERROR PVRMMapRemoveRegisteredArea(
			struct LinuxMemArea *psLinuxMemArea);

enum PVRSRV_ERROR PVRMMapOSMemHandleToMMapData(
			struct PVRSRV_PER_PROCESS_DATA *psPerProc,
			void *hMHandle, u32 *pui32MMapOffset,
			u32 *pui32ByteOffset, u32 *pui32RealByteSize,
			u32 *pui32UserVAddr);

enum PVRSRV_ERROR PVRMMapReleaseMMapData(
			struct PVRSRV_PER_PROCESS_DATA *psPerProc,
			void *hMHandle, IMG_BOOL *pbMUnmap,
			u32 *pui32RealByteSize, u32 *pui32UserVAddr);

int PVRMMap(struct file *pFile, struct vm_area_struct *ps_vma);

#endif
