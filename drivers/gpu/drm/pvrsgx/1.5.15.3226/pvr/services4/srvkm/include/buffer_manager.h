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

#ifndef _BUFFER_MANAGER_H_
#define _BUFFER_MANAGER_H_

#include "img_types.h"
#include "ra.h"
#include "perproc.h"

#if defined(__cplusplus)
extern "C"{
#endif

typedef struct _BM_HEAP_ BM_HEAP;

struct _BM_MAPPING_
{
	enum
	{
		hm_wrapped = 1,
		hm_wrapped_scatter,
		hm_wrapped_virtaddr,
		hm_wrapped_scatter_virtaddr,
		hm_env,
		hm_contiguous
	} eCpuMemoryOrigin;

	BM_HEAP				*pBMHeap;
	RA_ARENA			*pArena;

	IMG_CPU_VIRTADDR	CpuVAddr;
	IMG_CPU_PHYADDR		CpuPAddr;
	IMG_DEV_VIRTADDR	DevVAddr;
	IMG_SYS_PHYADDR		*psSysAddr;
	IMG_SIZE_T			uSize;
    IMG_HANDLE          hOSMemHandle;
	IMG_UINT32			ui32Flags;
};

typedef struct _BM_BUF_
{
	IMG_CPU_VIRTADDR	*CpuVAddr;
    IMG_VOID            *hOSMemHandle;
	IMG_CPU_PHYADDR		CpuPAddr;
	IMG_DEV_VIRTADDR	DevVAddr;

	BM_MAPPING			*pMapping;
	IMG_UINT32			ui32RefCount;
	IMG_UINT32			ui32ExportCount;
} BM_BUF;

struct _BM_HEAP_
{
	IMG_UINT32				ui32Attribs;
	BM_CONTEXT				*pBMContext;
	RA_ARENA				*pImportArena;
	RA_ARENA				*pLocalDevMemArena;
	RA_ARENA				*pVMArena;
	DEV_ARENA_DESCRIPTOR	sDevArena;
	MMU_HEAP				*pMMUHeap;

	struct _BM_HEAP_ 		*psNext;
	struct _BM_HEAP_ 		**ppsThis;
};

struct _BM_CONTEXT_
{
	MMU_CONTEXT	*psMMUContext;


	 BM_HEAP *psBMHeap;


	 BM_HEAP *psBMSharedHeap;

	PVRSRV_DEVICE_NODE *psDeviceNode;


	HASH_TABLE *pBufferHash;


	IMG_HANDLE hResItem;

	IMG_UINT32 ui32RefCount;



	struct _BM_CONTEXT_ *psNext;
	struct _BM_CONTEXT_ **ppsThis;
};



typedef IMG_VOID *BM_HANDLE;

#define BP_POOL_MASK         0x7

#define BP_CONTIGUOUS			(1 << 3)
#define BP_PARAMBUFFER			(1 << 4)

#define BM_MAX_DEVMEM_ARENAS  2

IMG_HANDLE
BM_CreateContext(PVRSRV_DEVICE_NODE			*psDeviceNode,
				 IMG_DEV_PHYADDR			*psPDDevPAddr,
				 PVRSRV_PER_PROCESS_DATA	*psPerProc,
				 IMG_BOOL					*pbCreated);


PVRSRV_ERROR
BM_DestroyContext (IMG_HANDLE hBMContext,
					IMG_BOOL *pbCreated);


IMG_HANDLE
BM_CreateHeap (IMG_HANDLE hBMContext,
				DEVICE_MEMORY_HEAP_INFO *psDevMemHeapInfo);

IMG_VOID
BM_DestroyHeap (IMG_HANDLE hDevMemHeap);


IMG_BOOL
BM_Reinitialise (PVRSRV_DEVICE_NODE *psDeviceNode);

IMG_BOOL
BM_Alloc (IMG_HANDLE			hDevMemHeap,
			IMG_DEV_VIRTADDR	*psDevVAddr,
			IMG_SIZE_T			uSize,
			IMG_UINT32			*pui32Flags,
			IMG_UINT32			uDevVAddrAlignment,
			BM_HANDLE			*phBuf);

IMG_BOOL
BM_Wrap (	IMG_HANDLE hDevMemHeap,
		    IMG_SIZE_T ui32Size,
			IMG_SIZE_T ui32Offset,
			IMG_BOOL bPhysContig,
			IMG_SYS_PHYADDR *psSysAddr,
			IMG_VOID *pvCPUVAddr,
			IMG_UINT32 *pui32Flags,
			BM_HANDLE *phBuf);

IMG_VOID
BM_Free (BM_HANDLE hBuf,
		IMG_UINT32 ui32Flags);


IMG_CPU_VIRTADDR
BM_HandleToCpuVaddr (BM_HANDLE hBuf);

IMG_DEV_VIRTADDR
BM_HandleToDevVaddr (BM_HANDLE hBuf);

IMG_SYS_PHYADDR
BM_HandleToSysPaddr (BM_HANDLE hBuf);

IMG_HANDLE
BM_HandleToOSMemHandle (BM_HANDLE hBuf);

IMG_BOOL
BM_ContiguousStatistics (IMG_UINT32 uFlags,
                         IMG_UINT32 *pTotalBytes,
                         IMG_UINT32 *pAvailableBytes);


IMG_VOID BM_GetPhysPageAddr(PVRSRV_KERNEL_MEM_INFO *psMemInfo,
								IMG_DEV_VIRTADDR sDevVPageAddr,
								IMG_DEV_PHYADDR *psDevPAddr);

PVRSRV_ERROR BM_GetHeapInfo(IMG_HANDLE hDevMemHeap,
							PVRSRV_HEAP_INFO *psHeapInfo);

MMU_CONTEXT* BM_GetMMUContext(IMG_HANDLE hDevMemHeap);

MMU_CONTEXT* BM_GetMMUContextFromMemContext(IMG_HANDLE hDevMemContext);

IMG_HANDLE BM_GetMMUHeap(IMG_HANDLE hDevMemHeap);

PVRSRV_DEVICE_NODE* BM_GetDeviceNode(IMG_HANDLE hDevMemContext);


IMG_HANDLE BM_GetMappingHandle(PVRSRV_KERNEL_MEM_INFO *psMemInfo);

IMG_VOID BM_Export(BM_HANDLE hBuf);

IMG_VOID BM_FreeExport(BM_HANDLE hBuf, IMG_UINT32 ui32Flags);

#if defined(__cplusplus)
}
#endif

#endif

