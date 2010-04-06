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

#ifndef _MMU_H_
#define _MMU_H_

#include "sgxinfokm.h"

enum PVRSRV_ERROR MMU_Initialise(struct PVRSRV_DEVICE_NODE *psDeviceNode,
		struct MMU_CONTEXT **ppsMMUContext,
		struct IMG_DEV_PHYADDR *psPDDevPAddr);

void MMU_Finalise(struct MMU_CONTEXT *psMMUContext);

void MMU_InsertHeap(struct MMU_CONTEXT *psMMUContext,
		struct MMU_HEAP *psMMUHeap);

struct MMU_HEAP *MMU_Create(struct MMU_CONTEXT *psMMUContext,
		struct DEV_ARENA_DESCRIPTOR *psDevArena,
		struct RA_ARENA **ppsVMArena);

void MMU_Delete(struct MMU_HEAP *pMMU);

IMG_BOOL MMU_Alloc(struct MMU_HEAP *pMMU, size_t uSize, u32 uFlags,
		u32 uDevVAddrAlignment, struct IMG_DEV_VIRTADDR *pDevVAddr);

void MMU_Free(struct MMU_HEAP *pMMU, struct IMG_DEV_VIRTADDR DevVAddr,
	      u32 ui32Size);

void MMU_Enable(struct MMU_HEAP *pMMU);

void MMU_Disable(struct MMU_HEAP *pMMU);

void MMU_MapPages(struct MMU_HEAP *pMMU, struct IMG_DEV_VIRTADDR devVAddr,
		struct IMG_SYS_PHYADDR SysPAddr, size_t uSize, u32 ui32MemFlags,
		void *hUniqueTag);

void MMU_MapShadow(struct MMU_HEAP *pMMU,
		struct IMG_DEV_VIRTADDR MapBaseDevVAddr, size_t uSize,
		void *CpuVAddr, void *hOSMemHandle,
		struct IMG_DEV_VIRTADDR *pDevVAddr, u32 ui32MemFlags,
		void *hUniqueTag);

void MMU_UnmapPages(struct MMU_HEAP *pMMU, struct IMG_DEV_VIRTADDR dev_vaddr,
		u32 ui32PageCount, void *hUniqueTag);

void MMU_MapScatter(struct MMU_HEAP *pMMU, struct IMG_DEV_VIRTADDR DevVAddr,
		struct IMG_SYS_PHYADDR *psSysAddr, size_t uSize,
		u32 ui32MemFlags, void *hUniqueTag);

struct IMG_DEV_PHYADDR MMU_GetPhysPageAddr(struct MMU_HEAP *pMMUHeap,
					struct IMG_DEV_VIRTADDR sDevVPageAddr);

struct IMG_DEV_PHYADDR MMU_GetPDDevPAddr(struct MMU_CONTEXT *pMMUContext);

void MMU_InvalidateDirectoryCache(struct PVRSRV_SGXDEV_INFO *psDevInfo);

enum PVRSRV_ERROR MMU_BIFResetPDAlloc(struct PVRSRV_SGXDEV_INFO *psDevInfo);

void MMU_BIFResetPDFree(struct PVRSRV_SGXDEV_INFO *psDevInfo);

#endif
