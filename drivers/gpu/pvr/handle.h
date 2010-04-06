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

#ifndef __HANDLE_H__
#define __HANDLE_H__

#include "img_types.h"
#include "hash.h"
#include "resman.h"

enum PVRSRV_HANDLE_TYPE {
	PVRSRV_HANDLE_TYPE_NONE = 0,
	PVRSRV_HANDLE_TYPE_PERPROC_DATA,
	PVRSRV_HANDLE_TYPE_DEV_NODE,
	PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT,
	PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP,
	PVRSRV_HANDLE_TYPE_MEM_INFO,
	PVRSRV_HANDLE_TYPE_SYNC_INFO,
	PVRSRV_HANDLE_TYPE_DISP_INFO,
	PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN,
	PVRSRV_HANDLE_TYPE_BUF_INFO,
	PVRSRV_HANDLE_TYPE_DISP_BUFFER,
	PVRSRV_HANDLE_TYPE_BUF_BUFFER,
	PVRSRV_HANDLE_TYPE_SGX_HW_RENDER_CONTEXT,
	PVRSRV_HANDLE_TYPE_SGX_HW_TRANSFER_CONTEXT,
	PVRSRV_HANDLE_TYPE_SGX_HW_2D_CONTEXT,
	PVRSRV_HANDLE_TYPE_SHARED_PB_DESC,
	PVRSRV_HANDLE_TYPE_MEM_INFO_REF,
	PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO,
	PVRSRV_HANDLE_TYPE_SHARED_EVENT_OBJECT,
	PVRSRV_HANDLE_TYPE_EVENT_OBJECT_CONNECT,
	PVRSRV_HANDLE_TYPE_MMAP_INFO,
	PVRSRV_HANDLE_TYPE_SOC_TIMER
};

enum PVRSRV_HANDLE_ALLOC_FLAG {
	PVRSRV_HANDLE_ALLOC_FLAG_NONE = 0,

	PVRSRV_HANDLE_ALLOC_FLAG_SHARED = 0x01,

	PVRSRV_HANDLE_ALLOC_FLAG_MULTI = 0x02,

	PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE = 0x04
};

struct PVRSRV_HANDLE_BASE;
struct PVRSRV_HANDLE_BASE;

extern struct PVRSRV_HANDLE_BASE *gpsKernelHandleBase;

#define	KERNEL_HANDLE_BASE (gpsKernelHandleBase)

enum PVRSRV_ERROR PVRSRVAllocHandle(struct PVRSRV_HANDLE_BASE *psBase,
			       void **phHandle, void *pvData,
			       enum PVRSRV_HANDLE_TYPE eType,
			       enum PVRSRV_HANDLE_ALLOC_FLAG eFlag);

enum PVRSRV_ERROR PVRSRVAllocSubHandle(struct PVRSRV_HANDLE_BASE *psBase,
				  void **phHandle,
				  void *pvData,
				  enum PVRSRV_HANDLE_TYPE eType,
				  enum PVRSRV_HANDLE_ALLOC_FLAG eFlag,
				  void *hParent);

enum PVRSRV_ERROR PVRSRVFindHandle(struct PVRSRV_HANDLE_BASE *psBase,
			      void **phHandle, void *pvData,
			      enum PVRSRV_HANDLE_TYPE eType);

enum PVRSRV_ERROR PVRSRVLookupHandleAnyType(struct PVRSRV_HANDLE_BASE *psBase,
				       void **ppvData,
				       enum PVRSRV_HANDLE_TYPE *peType,
				       void *hHandle);

enum PVRSRV_ERROR PVRSRVLookupHandle(struct PVRSRV_HANDLE_BASE *psBase,
				void **ppvData, void *hHandle,
				enum PVRSRV_HANDLE_TYPE eType);

enum PVRSRV_ERROR PVRSRVLookupSubHandle(struct PVRSRV_HANDLE_BASE *psBase,
				void **ppvData, void *hHandle,
				enum PVRSRV_HANDLE_TYPE eType, void *hAncestor);

enum PVRSRV_ERROR PVRSRVGetParentHandle(struct PVRSRV_HANDLE_BASE *psBase,
				void **phParent, void *hHandle,
				enum PVRSRV_HANDLE_TYPE eType);

enum PVRSRV_ERROR PVRSRVLookupAndReleaseHandle(
				struct PVRSRV_HANDLE_BASE *psBase,
				void **ppvData, void *hHandle,
				enum PVRSRV_HANDLE_TYPE eType);

enum PVRSRV_ERROR PVRSRVReleaseHandle(struct PVRSRV_HANDLE_BASE *psBase,
				 void *hHandle,
				 enum PVRSRV_HANDLE_TYPE eType);

enum PVRSRV_ERROR PVRSRVNewHandleBatch(struct PVRSRV_HANDLE_BASE *psBase,
				  u32 ui32BatchSize);

enum PVRSRV_ERROR PVRSRVCommitHandleBatch(struct PVRSRV_HANDLE_BASE *psBase);

void PVRSRVReleaseHandleBatch(struct PVRSRV_HANDLE_BASE *psBase);

enum PVRSRV_ERROR PVRSRVSetMaxHandle(struct PVRSRV_HANDLE_BASE *psBase,
				     u32 ui32MaxHandle);

u32 PVRSRVGetMaxHandle(struct PVRSRV_HANDLE_BASE *psBase);

enum PVRSRV_ERROR PVRSRVEnableHandlePurging(struct PVRSRV_HANDLE_BASE *psBase);

enum PVRSRV_ERROR PVRSRVPurgeHandles(struct PVRSRV_HANDLE_BASE *psBase);

enum PVRSRV_ERROR PVRSRVAllocHandleBase(struct PVRSRV_HANDLE_BASE **ppsBase);

enum PVRSRV_ERROR PVRSRVFreeHandleBase(struct PVRSRV_HANDLE_BASE *psBase);

enum PVRSRV_ERROR PVRSRVHandleInit(void);

enum PVRSRV_ERROR PVRSRVHandleDeInit(void);


#define PVRSRVAllocHandleNR(psBase, phHandle, pvData, eType, eFlag) \
    (void)PVRSRVAllocHandle(psBase, phHandle, pvData, eType, eFlag)

#define PVRSRVAllocSubHandleNR(psBase, phHandle, pvData, eType, eFlag, hParent)\
	(void)PVRSRVAllocSubHandle(psBase, phHandle, pvData, eType, \
				   eFlag, hParent)

#endif
