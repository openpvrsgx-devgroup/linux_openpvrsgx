/*************************************************************************/ /*!
@Title          Common Device header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Device related function templates and defines
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

#ifndef __DEVICE_H__
#define __DEVICE_H__

#if defined(__cplusplus)
extern "C" {
#endif

/* BM context forward reference */
typedef struct _BM_CONTEXT_ BM_CONTEXT;

/* pre-defined MMU structure forward references */
typedef struct _MMU_HEAP_ MMU_HEAP;
typedef struct _MMU_CONTEXT_ MMU_CONTEXT;

/* physical resource types: */
/* contiguous system memory */
#define PVRSRV_BACKINGSTORE_SYSMEM_CONTIG		(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+0))
/* non-contiguous system memory */
#define PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG	(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+1))
/* contiguous local device memory */
#define PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG		(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+2))
/* non-contiguous local device memory */
#define PVRSRV_BACKINGSTORE_LOCALMEM_NONCONTIG	(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+3))

/* heap types: */
typedef IMG_UINT32 DEVICE_MEMORY_HEAP_TYPE;
#define DEVICE_MEMORY_HEAP_PERCONTEXT		0
#define DEVICE_MEMORY_HEAP_KERNEL			1
#define DEVICE_MEMORY_HEAP_SHARED			2
#define DEVICE_MEMORY_HEAP_SHARED_EXPORTED	3

#define PVRSRV_DEVICE_NODE_FLAGS_PORT80DISPLAY	1
#define PVRSRV_DEVICE_NODE_FLAGS_MMU_OPT_INV	2	/* FIXME : Optimal Invalidation is not default */

typedef struct _DEVICE_MEMORY_HEAP_INFO_
{
	/* heap identifier */
	IMG_UINT32				ui32HeapID;

	/* heap identifier string */
	IMG_CHAR				*pszName;

	/* backing store identifier string */
	IMG_CHAR				*pszBSName;
	
	/* Device virtual address of base of heap */
	IMG_DEV_VIRTADDR		sDevVAddrBase;

	/* heapsize in bytes */
	IMG_UINT32				ui32HeapSize;

	/* Flags, includes physical resource (backing store type). Must be available to SOC */
	IMG_UINT32				ui32Attribs;

	/* Heap type: per device, kernel only, shared, shared_exported */
	DEVICE_MEMORY_HEAP_TYPE	DevMemHeapType;
	
	/* kernel heap handle */
	IMG_HANDLE				hDevMemHeap;
	
	/* ptr to local memory allocator for this heap */
	RA_ARENA				*psLocalDevMemArena;

	/* MMU data page size (4kb, 16kb, 256kb, 1Mb, 4Mb) */
	IMG_UINT32				ui32DataPageSize;

} DEVICE_MEMORY_HEAP_INFO;

typedef struct _DEVICE_MEMORY_INFO_
{
	/* size of address space, as log2 */
	IMG_UINT32				ui32AddressSpaceSizeLog2;

	/* 
		flags, includes physical memory resource types available to the system.  
		Allows for validation at heap creation, define PVRSRV_BACKINGSTORE_XXX 
	*/
	IMG_UINT32				ui32Flags;

	/* heap count.  Doesn't include additional heaps from PVRSRVCreateDeviceMemHeap */
	IMG_UINT32				ui32HeapCount;
	
	/* the sync heap id - common code needs to know */
	IMG_UINT32				ui32SyncHeapID;
	
	/* heap for buffer mappings  */
	IMG_UINT32				ui32MappingHeapID;


	/* device memory heap info about each heap in a device address space */
	DEVICE_MEMORY_HEAP_INFO	*psDeviceMemoryHeap;

	/* BM kernel context for the device */
    BM_CONTEXT				*pBMKernelContext;

	/* BM context list for the device*/
    BM_CONTEXT				*pBMContext;

} DEVICE_MEMORY_INFO;


/*!
 ****************************************************************************
	Device memory descriptor for a given system
 ****************************************************************************/
typedef struct DEV_ARENA_DESCRIPTOR_TAG
{
	IMG_UINT32				ui32HeapID;		/*!< memory pool has a unique id for diagnostic purposes */

	IMG_CHAR				*pszName;		/*!< memory pool has a unique string for diagnostic purposes */

	IMG_DEV_VIRTADDR		BaseDevVAddr;	/*!< Device virtual base address of the managed memory pool. */

	IMG_UINT32 				ui32Size;		/*!< Size in bytes of the managed memory pool. */

	DEVICE_MEMORY_HEAP_TYPE	DevMemHeapType;/*!< heap type */

	/* MMU data page size (4kb, 16kb, 256kb, 1Mb, 4Mb) */
	IMG_UINT32				ui32DataPageSize;

	DEVICE_MEMORY_HEAP_INFO	*psDeviceMemoryHeapInfo;

} DEV_ARENA_DESCRIPTOR;

typedef struct _SYS_DATA_TAG_ *PSYS_DATA;

typedef struct _PVRSRV_DEVICE_NODE_
{
	PVRSRV_DEVICE_IDENTIFIER	sDevId;
	IMG_UINT32					ui32RefCount;

	/*
		callbacks the device must support:
	*/
	/* device initialiser */
	PVRSRV_ERROR			(*pfnInitDevice) (IMG_VOID*);
	/* device deinitialiser */
	PVRSRV_ERROR			(*pfnDeInitDevice) (IMG_VOID*);

	/* device post-finalise compatibility check */
	PVRSRV_ERROR			(*pfnInitDeviceCompatCheck) (struct _PVRSRV_DEVICE_NODE_*);

	/* device MMU interface */
	PVRSRV_ERROR			(*pfnMMUInitialise)(struct _PVRSRV_DEVICE_NODE_*, MMU_CONTEXT**, IMG_DEV_PHYADDR*);
	IMG_VOID				(*pfnMMUFinalise)(MMU_CONTEXT*);
	IMG_VOID				(*pfnMMUInsertHeap)(MMU_CONTEXT*, MMU_HEAP*);
	MMU_HEAP*				(*pfnMMUCreate)(MMU_CONTEXT*,DEV_ARENA_DESCRIPTOR*,RA_ARENA**);
	IMG_VOID				(*pfnMMUDelete)(MMU_HEAP*);
	IMG_BOOL				(*pfnMMUAlloc)(MMU_HEAP*pMMU,
										   IMG_SIZE_T uSize,
										   IMG_SIZE_T *pActualSize,
										   IMG_UINT32 uFlags,
										   IMG_UINT32 uDevVAddrAlignment,
										   IMG_DEV_VIRTADDR *pDevVAddr);
	IMG_VOID				(*pfnMMUFree)(MMU_HEAP*,IMG_DEV_VIRTADDR,IMG_UINT32);
	IMG_VOID 				(*pfnMMUEnable)(MMU_HEAP*);
	IMG_VOID				(*pfnMMUDisable)(MMU_HEAP*);
	IMG_VOID				(*pfnMMUMapPages)(MMU_HEAP *pMMU,
											  IMG_DEV_VIRTADDR devVAddr,
											  IMG_SYS_PHYADDR SysPAddr,
											  IMG_SIZE_T uSize,
											  IMG_UINT32 ui32MemFlags,
											  IMG_HANDLE hUniqueTag);
	IMG_VOID				(*pfnMMUMapShadow)(MMU_HEAP            *pMMU,
											   IMG_DEV_VIRTADDR    MapBaseDevVAddr,
											   IMG_SIZE_T          uSize,
											   IMG_CPU_VIRTADDR    CpuVAddr,
											   IMG_HANDLE          hOSMemHandle,
											   IMG_DEV_VIRTADDR    *pDevVAddr,
											   IMG_UINT32 ui32MemFlags,
											   IMG_HANDLE hUniqueTag);
	IMG_VOID				(*pfnMMUUnmapPages)(MMU_HEAP *pMMU,
												IMG_DEV_VIRTADDR dev_vaddr,
												IMG_UINT32 ui32PageCount,
												IMG_HANDLE hUniqueTag);

	IMG_VOID				(*pfnMMUMapScatter)(MMU_HEAP *pMMU,
												IMG_DEV_VIRTADDR DevVAddr,
												IMG_SYS_PHYADDR *psSysAddr,
												IMG_SIZE_T uSize,
												IMG_UINT32 ui32MemFlags,
												IMG_HANDLE hUniqueTag);

	IMG_DEV_PHYADDR			(*pfnMMUGetPhysPageAddr)(MMU_HEAP *pMMUHeap, IMG_DEV_VIRTADDR sDevVPageAddr);
	IMG_DEV_PHYADDR			(*pfnMMUGetPDDevPAddr)(MMU_CONTEXT *pMMUContext);


	IMG_BOOL				(*pfnDeviceISR)(IMG_VOID*);

	IMG_VOID				*pvISRData;

	IMG_UINT32 				ui32SOCInterruptBit;

	IMG_VOID				(*pfnDeviceMISR)(IMG_VOID*);


	IMG_VOID				(*pfnDeviceCommandComplete)(struct _PVRSRV_DEVICE_NODE_ *psDeviceNode);

	IMG_BOOL				bReProcessDeviceCommandComplete;


	/* information about the device's address space and heaps */
	DEVICE_MEMORY_INFO		sDevMemoryInfo;


	IMG_VOID				*pvDevice;
	IMG_UINT32				ui32pvDeviceSize;


	PRESMAN_CONTEXT			hResManContext;


	PSYS_DATA				psSysData;


	RA_ARENA				*psLocalDevMemArena;

	IMG_UINT32				ui32Flags;

	struct _PVRSRV_DEVICE_NODE_	*psNext;
	struct _PVRSRV_DEVICE_NODE_	**ppsThis;
} PVRSRV_DEVICE_NODE;

PVRSRV_ERROR IMG_CALLCONV PVRSRVRegisterDevice(PSYS_DATA psSysData,
											  PVRSRV_ERROR (*pfnRegisterDevice)(PVRSRV_DEVICE_NODE*),
											  IMG_UINT32 ui32SOCInterruptBit,
			 								  IMG_UINT32 *pui32DeviceIndex );

PVRSRV_ERROR IMG_CALLCONV PVRSRVInitialiseDevice(IMG_UINT32 ui32DevIndex);
PVRSRV_ERROR IMG_CALLCONV PVRSRVFinaliseSystem(IMG_BOOL bInitSuccesful);

PVRSRV_ERROR IMG_CALLCONV PVRSRVDevInitCompatCheck(PVRSRV_DEVICE_NODE *psDeviceNode);

PVRSRV_ERROR IMG_CALLCONV PVRSRVDeinitialiseDevice(IMG_UINT32 ui32DevIndex);

#if !defined(USE_CODE)

IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV PollForValueKM(volatile IMG_UINT32* pui32LinMemAddr,
												   IMG_UINT32 ui32Value,
												   IMG_UINT32 ui32Mask,
												   IMG_UINT32 ui32Waitus,
												   IMG_UINT32 ui32Tries);

#endif


#if defined (USING_ISR_INTERRUPTS)
PVRSRV_ERROR IMG_CALLCONV PollForInterruptKM(IMG_UINT32 ui32Value,
								IMG_UINT32 ui32Mask,
								IMG_UINT32 ui32Waitus,
								IMG_UINT32 ui32Tries);
#endif

PVRSRV_ERROR IMG_CALLCONV PVRSRVInit(PSYS_DATA psSysData);
IMG_VOID IMG_CALLCONV PVRSRVDeInit(PSYS_DATA psSysData);
IMG_BOOL IMG_CALLCONV PVRSRVDeviceLISR(PVRSRV_DEVICE_NODE *psDeviceNode);
IMG_BOOL IMG_CALLCONV PVRSRVSystemLISR(IMG_VOID *pvSysData);
IMG_VOID IMG_CALLCONV PVRSRVMISR(IMG_VOID *pvSysData);

#if defined(__cplusplus)
}
#endif
	
#endif /* __DEVICE_H__ */

/******************************************************************************
 End of file (device.h)
******************************************************************************/
