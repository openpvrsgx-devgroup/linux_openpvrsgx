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

#ifndef _BUFFER_MANAGER_H_
#define _BUFFER_MANAGER_H_

#include "img_types.h"
#include "ra.h"
#include "perproc.h"


struct BM_HEAP;

struct BM_MAPPING {
	enum {
		hm_wrapped = 1,
		hm_wrapped_scatter,
		hm_wrapped_virtaddr,
		hm_wrapped_scatter_virtaddr,
		hm_env,
		hm_contiguous
	} eCpuMemoryOrigin;

	struct BM_HEAP *pBMHeap;
	struct RA_ARENA *pArena;

	void *CpuVAddr;
	struct IMG_CPU_PHYADDR CpuPAddr;
	struct IMG_DEV_VIRTADDR DevVAddr;
	struct IMG_SYS_PHYADDR *psSysAddr;
	size_t uSize;
	void *hOSMemHandle;
	u32 ui32Flags;
};

struct BM_BUF {
	void **CpuVAddr;
	void *hOSMemHandle;
	struct IMG_CPU_PHYADDR CpuPAddr;
	struct IMG_DEV_VIRTADDR DevVAddr;

	struct BM_MAPPING *pMapping;
	u32 ui32RefCount;
};

struct BM_HEAP {
	u32 ui32Attribs;
	struct BM_CONTEXT *pBMContext;
	struct RA_ARENA *pImportArena;
	struct RA_ARENA *pLocalDevMemArena;
	struct RA_ARENA *pVMArena;
	struct DEV_ARENA_DESCRIPTOR sDevArena;
	struct MMU_HEAP *pMMUHeap;

	struct BM_HEAP *psNext;
};

struct BM_CONTEXT {
	struct MMU_CONTEXT *psMMUContext;
	struct BM_HEAP *psBMHeap;
	struct BM_HEAP *psBMSharedHeap;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct HASH_TABLE *pBufferHash;
	void *hResItem;
	u32 ui32RefCount;
	struct BM_CONTEXT *psNext;
};

#define BP_POOL_MASK	 0x7

#define BP_CONTIGUOUS			(1 << 3)
#define BP_PARAMBUFFER			(1 << 4)

#define BM_MAX_DEVMEM_ARENAS  2

void *BM_CreateContext(struct PVRSRV_DEVICE_NODE *psDeviceNode,
		struct IMG_DEV_PHYADDR *psPDDevPAddr,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc,
		IMG_BOOL *pbCreated);

void BM_DestroyContext(void *hBMContext);

static inline void pvr_get_ctx(struct BM_CONTEXT *ctx)
{
	WARN_ON(!ctx->ui32RefCount);
	ctx->ui32RefCount++;
}

static inline bool pvr_put_ctx(struct BM_CONTEXT *ctx)
{
	BUG_ON(!ctx->ui32RefCount);
	ctx->ui32RefCount--;
	if (!ctx->ui32RefCount) {
		BM_DestroyContext(ctx);

		return true;
	}

	return false;
}

void *BM_CreateHeap(void *hBMContext,
		    struct DEVICE_MEMORY_HEAP_INFO *psDevMemHeapInfo);
void BM_DestroyHeap(void *hDevMemHeap);
IMG_BOOL BM_Reinitialise(struct PVRSRV_DEVICE_NODE *psDeviceNode);

IMG_BOOL BM_Alloc(void *hDevMemHeap,
	 struct IMG_DEV_VIRTADDR *psDevVAddr,
	 size_t uSize, u32 *pui32Flags, u32 uDevVAddrAlignment, void **phBuf);

IMG_BOOL BM_Wrap(void *hDevMemHeap,
	u32 ui32Size,
	u32 ui32Offset,
	IMG_BOOL bPhysContig,
	struct IMG_SYS_PHYADDR *psSysAddr,
	void *pvCPUVAddr, u32 *pui32Flags, void **phBuf);

void BM_Free(void *hBuf, u32 ui32Flags);
void *BM_HandleToCpuVaddr(void *hBuf);
struct IMG_DEV_VIRTADDR BM_HandleToDevVaddr(void *hBuf);

struct IMG_SYS_PHYADDR BM_HandleToSysPaddr(void *hBuf);

void *BM_HandleToOSMemHandle(void *hBuf);

IMG_BOOL BM_ContiguousStatistics(u32 uFlags, u32 *pTotalBytes,
		u32 *pAvailableBytes);

enum PVRSRV_ERROR BM_GetPhysPageAddr(struct PVRSRV_KERNEL_MEM_INFO *psMemInfo,
		struct IMG_DEV_VIRTADDR sDevVPageAddr,
		struct IMG_DEV_PHYADDR *psDevPAddr);

enum PVRSRV_ERROR BM_GetHeapInfo(void *hDevMemHeap,
			    struct PVRSRV_HEAP_INFO *psHeapInfo);

struct MMU_CONTEXT *BM_GetMMUContext(void *hDevMemHeap);

struct MMU_CONTEXT *BM_GetMMUContextFromMemContext(void *hDevMemContext);

void *BM_GetMMUHeap(void *hDevMemHeap);

struct PVRSRV_DEVICE_NODE *BM_GetDeviceNode(void *hDevMemContext);

void *BM_GetMappingHandle(struct PVRSRV_KERNEL_MEM_INFO *psMemInfo);

#endif
