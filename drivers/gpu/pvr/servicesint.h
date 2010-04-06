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

#if !defined(__SERVICESINT_H__)
#define __SERVICESINT_H__


#include "services.h"
#include "sysinfo.h"

#define HWREC_DEFAULT_TIMEOUT		500

#define DRIVERNAME_MAXLENGTH		100

struct PVRSRV_KERNEL_MEM_INFO {

	void *pvLinAddrKM;
	struct IMG_DEV_VIRTADDR sDevVAddr;
	u32 ui32Flags;
	u32 ui32AllocSize;
	struct PVRSRV_MEMBLK sMemBlk;

	void *pvSysBackupBuffer;

	u32 ui32RefCount;

	struct PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo;
};

struct PVRSRV_KERNEL_SYNC_INFO {
	struct PVRSRV_SYNC_DATA *psSyncData;
	struct IMG_DEV_VIRTADDR sWriteOpsCompleteDevVAddr;
	struct IMG_DEV_VIRTADDR sReadOpsCompleteDevVAddr;
	struct PVRSRV_KERNEL_MEM_INFO *psSyncDataMemInfoKM;
};

struct PVRSRV_DEVICE_SYNC_OBJECT {

	u32 ui32ReadOpsPendingVal;
	struct IMG_DEV_VIRTADDR sReadOpsCompleteDevVAddr;
	u32 ui32WriteOpsPendingVal;
	struct IMG_DEV_VIRTADDR sWriteOpsCompleteDevVAddr;
};

struct PVRSRV_SYNC_OBJECT {
	struct PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfoKM;
	u32 ui32WriteOpsPending;
	u32 ui32ReadOpsPending;
};

struct PVRSRV_COMMAND {
	u32 ui32CmdSize;
	u32 ui32DevIndex;
	u32 CommandType;
	u32 ui32DstSyncCount;
	u32 ui32SrcSyncCount;
	struct PVRSRV_SYNC_OBJECT *psDstSync;
	struct PVRSRV_SYNC_OBJECT *psSrcSync;
	u32 ui32DataSize;
	u32 ui32ProcessID;
	void *pvData;
};

struct PVRSRV_QUEUE_INFO {
	void *pvLinQueueKM;
	void *pvLinQueueUM;
	volatile u32 ui32ReadOffset;
	volatile u32 ui32WriteOffset;
	u32 *pui32KickerAddrKM;
	u32 *pui32KickerAddrUM;
	u32 ui32QueueSize;

	u32 ui32ProcessID;

	void *hMemBlock[2];

	struct PVRSRV_QUEUE_INFO *psNextKM;
};

struct PVRSRV_DEVICECLASS_BUFFER {
	enum PVRSRV_ERROR (*pfnGetBufferAddr)(void *, void *,
				    struct IMG_SYS_PHYADDR **, u32 *,
				    void __iomem **, void **, IMG_BOOL *);
	void *hDevMemContext;
	void *hExtDevice;
	void *hExtBuffer;
	struct PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo;
};

struct PVRSRV_CLIENT_DEVICECLASS_INFO {
	void *hDeviceKM;
	void *hServices;
};

static inline u32 PVRSRVGetWriteOpsPending(
		struct PVRSRV_KERNEL_SYNC_INFO *psSyncInfo, IMG_BOOL bIsReadOp)
{
	u32 ui32WriteOpsPending;

	if (bIsReadOp)
		ui32WriteOpsPending =
		    psSyncInfo->psSyncData->ui32WriteOpsPending;
	else
		ui32WriteOpsPending =
		    psSyncInfo->psSyncData->ui32WriteOpsPending++;

	return ui32WriteOpsPending;
}

static inline u32 PVRSRVGetReadOpsPending(
		struct PVRSRV_KERNEL_SYNC_INFO *psSyncInfo, IMG_BOOL bIsReadOp)
{
	u32 ui32ReadOpsPending;

	if (bIsReadOp)
		ui32ReadOpsPending =
		    psSyncInfo->psSyncData->ui32ReadOpsPending++;
	else
		ui32ReadOpsPending = psSyncInfo->psSyncData->ui32ReadOpsPending;

	return ui32ReadOpsPending;
}

enum PVRSRV_ERROR PVRSRVQueueCommand(void *hQueueInfo,
				    struct PVRSRV_COMMAND *psCommand);

enum PVRSRV_ERROR PVRSRVGetMMUContextPDDevPAddr(
			const struct PVRSRV_CONNECTION *psConnection,
			void *hDevMemContext,
			struct IMG_DEV_PHYADDR *sPDDevPAddr);

enum PVRSRV_ERROR PVRSRVAllocSharedSysMem(
			const struct PVRSRV_CONNECTION *psConnection,
			u32 ui32Flags, u32 ui32Size,
			struct PVRSRV_CLIENT_MEM_INFO **ppsClientMemInfo);

enum PVRSRV_ERROR PVRSRVFreeSharedSysMem(
			const struct PVRSRV_CONNECTION *psConnection,
			struct PVRSRV_CLIENT_MEM_INFO *psClientMemInfo);

enum PVRSRV_ERROR PVRSRVUnrefSharedSysMem(
			const struct PVRSRV_CONNECTION *psConnection,
			struct PVRSRV_CLIENT_MEM_INFO *psClientMemInfo);

enum PVRSRV_ERROR PVRSRVMapMemInfoMem(
			const struct PVRSRV_CONNECTION *psConnection,
			void *hKernelMemInfo,
			struct PVRSRV_CLIENT_MEM_INFO **ppsClientMemInfo);

#endif
