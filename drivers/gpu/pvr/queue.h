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

#ifndef QUEUE_H
#define QUEUE_H

#define UPDATE_QUEUE_ROFF(psQueue, ui32Size)				\
	psQueue->ui32ReadOffset = (psQueue->ui32ReadOffset + ui32Size)	\
					& (psQueue->ui32QueueSize - 1);

struct COMMAND_COMPLETE_DATA {
	IMG_BOOL bInUse;

	u32 ui32DstSyncCount;
	u32 ui32SrcSyncCount;
	struct PVRSRV_SYNC_OBJECT *psDstSync;
	struct PVRSRV_SYNC_OBJECT *psSrcSync;
	u32 ui32AllocSize;
};

enum PVRSRV_ERROR PVRSRVProcessQueues(IMG_BOOL bFlush);

#if defined(__KERNEL__)
#include <linux/types.h>
off_t QueuePrintQueues(char *buffer, size_t size, off_t off);
#endif

enum PVRSRV_ERROR PVRSRVCreateCommandQueueKM(u32 ui32QueueSize,
		struct PVRSRV_QUEUE_INFO **ppsQueueInfo);
enum PVRSRV_ERROR PVRSRVDestroyCommandQueueKM(
		struct PVRSRV_QUEUE_INFO *psQueueInfo);

enum PVRSRV_ERROR PVRSRVInsertCommandKM(struct PVRSRV_QUEUE_INFO *psQueue,
		struct PVRSRV_COMMAND **ppsCommand, u32 ui32DevIndex,
		u16 CommandType, u32 ui32DstSyncCount,
		struct PVRSRV_KERNEL_SYNC_INFO *apsDstSync[],
		u32 ui32SrcSyncCount,
		struct PVRSRV_KERNEL_SYNC_INFO *apsSrcSync[],
		u32 ui32DataByteSize);

enum PVRSRV_ERROR PVRSRVGetQueueSpaceKM(struct PVRSRV_QUEUE_INFO *psQueue,
		u32 ui32ParamSize, void **ppvSpace);

enum PVRSRV_ERROR PVRSRVSubmitCommandKM(struct PVRSRV_QUEUE_INFO *psQueue,
		struct PVRSRV_COMMAND *psCommand);

void PVRSRVCommandCompleteKM(void *hCmdCookie, IMG_BOOL bScheduleMISR);

void PVRSRVCommandCompleteCallbacks(void);

enum PVRSRV_ERROR PVRSRVRegisterCmdProcListKM(u32 ui32DevIndex,
		IMG_BOOL (**ppfnCmdProcList)(void *, u32, void *),
		u32 ui32MaxSyncsPerCmd[][2], u32 ui32CmdCount);
enum PVRSRV_ERROR PVRSRVRemoveCmdProcListKM(u32 ui32DevIndex,
		u32 ui32CmdCount);


#endif
