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

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "ra.h"
#include "resman.h"

struct BM_CONTEXT;

struct MMU_HEAP;
struct MMU_CONTEXT;

#define PVRSRV_BACKINGSTORE_SYSMEM_CONTIG		\
		(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+0))
#define PVRSRV_BACKINGSTORE_SYSMEM_NONCONTIG		\
		(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+1))
#define PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG		\
		(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+2))
#define PVRSRV_BACKINGSTORE_LOCALMEM_NONCONTIG		\
		(1<<(PVRSRV_MEM_BACKINGSTORE_FIELD_SHIFT+3))

#define DEVICE_MEMORY_HEAP_PERCONTEXT			0
#define DEVICE_MEMORY_HEAP_KERNEL			1
#define DEVICE_MEMORY_HEAP_SHARED			2
#define DEVICE_MEMORY_HEAP_SHARED_EXPORTED		3

#define PVRSRV_DEVICE_NODE_FLAGS_PORT80DISPLAY		1
#define PVRSRV_DEVICE_NODE_FLAGS_MMU_OPT_INV		2

struct DEVICE_MEMORY_HEAP_INFO {
	u32 ui32HeapID;
	char *pszName;
	char *pszBSName;
	struct IMG_DEV_VIRTADDR sDevVAddrBase;
	u32 ui32HeapSize;
	u32 ui32Attribs;
	u32 DevMemHeapType;
	void *hDevMemHeap;
	struct RA_ARENA *psLocalDevMemArena;

	u32 ui32DataPageSize;

};

struct DEVICE_MEMORY_INFO {
	u32 ui32AddressSpaceSizeLog2;
	u32 ui32Flags;
	u32 ui32HeapCount;
	u32 ui32SyncHeapID;
	u32 ui32MappingHeapID;
	struct DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap;
	struct BM_CONTEXT *pBMKernelContext;
	struct BM_CONTEXT *pBMContext;
};

struct DEV_ARENA_DESCRIPTOR {
	u32 ui32HeapID;
	char *pszName;
	struct IMG_DEV_VIRTADDR BaseDevVAddr;
	u32 ui32Size;
	u32 DevMemHeapType;

	u32 ui32DataPageSize;

	struct DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeapInfo;
};

struct SYS_DATA;

struct PVRSRV_DEVICE_NODE {
	struct PVRSRV_DEVICE_IDENTIFIER sDevId;
	u32 ui32RefCount;

	enum PVRSRV_ERROR (*pfnInitDevice)(void *);
	enum PVRSRV_ERROR (*pfnDeInitDevice)(void *);

	enum PVRSRV_ERROR (*pfnInitDeviceCompatCheck)(
						struct PVRSRV_DEVICE_NODE *);

	enum PVRSRV_ERROR (*pfnMMUInitialise)(struct PVRSRV_DEVICE_NODE *,
			struct MMU_CONTEXT **,
			struct IMG_DEV_PHYADDR *);
	void (*pfnMMUFinalise)(struct MMU_CONTEXT *);
	void (*pfnMMUInsertHeap)(struct MMU_CONTEXT *, struct MMU_HEAP *);
	struct MMU_HEAP *(*pfnMMUCreate)(struct MMU_CONTEXT *,
			struct DEV_ARENA_DESCRIPTOR *, struct RA_ARENA **);
	void (*pfnMMUDelete)(struct MMU_HEAP *);
	IMG_BOOL (*pfnMMUAlloc)(struct MMU_HEAP *pMMU, size_t uSize, u32 uFlags,
			u32 uDevVAddrAlignment,
			struct IMG_DEV_VIRTADDR *pDevVAddr);
	void (*pfnMMUFree)(struct MMU_HEAP *, struct IMG_DEV_VIRTADDR, u32);
	void (*pfnMMUEnable)(struct MMU_HEAP *);
	void (*pfnMMUDisable)(struct MMU_HEAP *);
	void (*pfnMMUMapPages)(struct MMU_HEAP *pMMU,
			struct IMG_DEV_VIRTADDR devVAddr,
			struct IMG_SYS_PHYADDR SysPAddr,
			size_t uSize, u32 ui32MemFlags, void *hUniqueTag);
	void (*pfnMMUMapShadow)(struct MMU_HEAP *pMMU,
			struct IMG_DEV_VIRTADDR MapBaseDevVAddr,
			size_t uSize, void *CpuVAddr, void *hOSMemHandle,
			struct IMG_DEV_VIRTADDR *pDevVAddr, u32 ui32MemFlags,
			void *hUniqueTag);
	void (*pfnMMUUnmapPages)(struct MMU_HEAP *pMMU,
			struct IMG_DEV_VIRTADDR dev_vaddr, u32 ui32PageCount,
			void *hUniqueTag);

	void (*pfnMMUMapScatter)(struct MMU_HEAP *pMMU,
			struct IMG_DEV_VIRTADDR DevVAddr,
			struct IMG_SYS_PHYADDR *psSysAddr,
			size_t uSize, u32 ui32MemFlags, void *hUniqueTag);

	struct IMG_DEV_PHYADDR(*pfnMMUGetPhysPageAddr)(
			struct MMU_HEAP *pMMUHeap,
			struct IMG_DEV_VIRTADDR sDevVPageAddr);
	struct IMG_DEV_PHYADDR(*pfnMMUGetPDDevPAddr)(
			struct MMU_CONTEXT *pMMUContext);

	IMG_BOOL (*pfnDeviceISR)(void *);

	void *pvISRData;
	u32 ui32SOCInterruptBit;

	void (*pfnDeviceMISR)(void *);
	void (*pfnDeviceCommandComplete)(struct PVRSRV_DEVICE_NODE *
			psDeviceNode);

	IMG_BOOL bReProcessDeviceCommandComplete;
	struct DEVICE_MEMORY_INFO sDevMemoryInfo;
	void *pvDevice;
	u32 ui32pvDeviceSize;

	struct RESMAN_CONTEXT *hResManContext;
	struct SYS_DATA *psSysData;
	struct RA_ARENA *psLocalDevMemArena;
	u32 ui32Flags;
	struct PVRSRV_DEVICE_NODE *psNext;
};

enum PVRSRV_ERROR PVRSRVRegisterDevice(struct SYS_DATA *psSysData,
	    enum PVRSRV_ERROR (*pfnRegisterDevice)(struct PVRSRV_DEVICE_NODE *),
	    u32 ui32SOCInterruptBit, u32 *pui32DeviceIndex);

enum PVRSRV_ERROR PVRSRVInitialiseDevice(u32 ui32DevIndex);
enum PVRSRV_ERROR PVRSRVFinaliseSystem(IMG_BOOL bInitSuccesful);

enum PVRSRV_ERROR PVRSRVDevInitCompatCheck(struct PVRSRV_DEVICE_NODE
					   *psDeviceNode);

enum PVRSRV_ERROR PVRSRVDeinitialiseDevice(u32 ui32DevIndex);


enum PVRSRV_ERROR PollForValueKM(u32 __iomem *pui32LinMemAddr,
	    u32 ui32Value, u32 ui32Mask, u32 ui32Waitus, u32 ui32Tries);

enum PVRSRV_ERROR PVRSRVInit(struct SYS_DATA *psSysData);
void PVRSRVDeInit(struct SYS_DATA *psSysData);
IMG_BOOL PVRSRVDeviceLISR(struct PVRSRV_DEVICE_NODE *psDeviceNode);
IMG_BOOL PVRSRVSystemLISR(void *pvSysData);
void PVRSRVMISR(void *pvSysData);

#endif
