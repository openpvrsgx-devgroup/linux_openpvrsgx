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

#include "services_headers.h"

#include "proc.h"

static int QueuePrintCommands(struct PVRSRV_QUEUE_INFO *psQueue, char *buffer,
			      size_t size)
{
	off_t off = 0;
	int cmds = 0;
	u32 ui32ReadOffset = psQueue->ui32ReadOffset;
	u32 ui32WriteOffset = psQueue->ui32WriteOffset;
	struct PVRSRV_COMMAND *psCmd;

	while (ui32ReadOffset != ui32WriteOffset) {
		psCmd = (struct PVRSRV_COMMAND *)((u32) psQueue->pvLinQueueKM +
					ui32ReadOffset);

		off = printAppend(buffer, size, off,
			"%p %p  %5u  %6u  %3u  %5u   %2u   %2u    %3u  \n",
				psQueue, psCmd, psCmd->ui32ProcessID,
				psCmd->CommandType, psCmd->ui32CmdSize,
				psCmd->ui32DevIndex, psCmd->ui32DstSyncCount,
				psCmd->ui32SrcSyncCount, psCmd->ui32DataSize);

		ui32ReadOffset += psCmd->ui32CmdSize;
		ui32ReadOffset &= psQueue->ui32QueueSize - 1;
		cmds++;
	}
	if (cmds == 0)
		off = printAppend(buffer, size, off, "%p <empty>\n", psQueue);
	return off;
}

off_t QueuePrintQueues(char *buffer, size_t size, off_t off)
{
	struct SYS_DATA *psSysData;
	struct PVRSRV_QUEUE_INFO *psQueue;

	if (SysAcquireData(&psSysData) != PVRSRV_OK)
		return END_OF_FILE;

	if (!off)
		return printAppend(buffer, size, 0,
			"Command Queues\nQueue    CmdPtr      "
			   "Pid Command Size DevInd  DSC  SSC  #Data ...\n");

	for (psQueue = psSysData->psQueueList; --off && psQueue;
	     psQueue = psQueue->psNextKM)
		;

	return psQueue ?
		QueuePrintCommands(psQueue, buffer, size) : END_OF_FILE;
}

#define GET_SPACE_IN_CMDQ(psQueue)					\
	(((psQueue->ui32ReadOffset - psQueue->ui32WriteOffset) +	\
	  (psQueue->ui32QueueSize - 1)) & (psQueue->ui32QueueSize - 1))

#define UPDATE_QUEUE_WOFF(psQueue, ui32Size)				   \
	psQueue->ui32WriteOffset = (psQueue->ui32WriteOffset + ui32Size) & \
	(psQueue->ui32QueueSize - 1);

#define SYNCOPS_STALE(ui32OpsComplete, ui32OpsPending)			\
	(ui32OpsComplete >= ui32OpsPending)

static u32 NearestPower2(u32 ui32Value)
{
	u32 ui32Temp, ui32Result = 1;

	if (!ui32Value)
		return 0;

	ui32Temp = ui32Value - 1;
	while (ui32Temp) {
		ui32Result <<= 1;
		ui32Temp >>= 1;
	}

	return ui32Result;
}

enum PVRSRV_ERROR PVRSRVCreateCommandQueueKM(u32 ui32QueueSize,
				 struct PVRSRV_QUEUE_INFO **ppsQueueInfo)
{
	struct PVRSRV_QUEUE_INFO *psQueueInfo;
	u32 ui32Power2QueueSize = NearestPower2(ui32QueueSize);
	struct SYS_DATA *psSysData;
	enum PVRSRV_ERROR eError;
	void *hMemBlock;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return eError;

	if (OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
		       sizeof(struct PVRSRV_QUEUE_INFO),
		       (void **) &psQueueInfo, &hMemBlock) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVCreateCommandQueueKM: "
					"Failed to alloc queue struct");
		goto ErrorExit;
	}
	OSMemSet(psQueueInfo, 0, sizeof(struct PVRSRV_QUEUE_INFO));

	psQueueInfo->hMemBlock[0] = hMemBlock;
	psQueueInfo->ui32ProcessID = OSGetCurrentProcessIDKM();

	if (OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
		       ui32Power2QueueSize + PVRSRV_MAX_CMD_SIZE,
		       &psQueueInfo->pvLinQueueKM, &hMemBlock) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVCreateCommandQueueKM: "
					"Failed to alloc queue buffer");
		goto ErrorExit;
	}

	psQueueInfo->hMemBlock[1] = hMemBlock;
	psQueueInfo->pvLinQueueUM = psQueueInfo->pvLinQueueKM;

	PVR_ASSERT(psQueueInfo->ui32ReadOffset == 0);
	PVR_ASSERT(psQueueInfo->ui32WriteOffset == 0);

	psQueueInfo->ui32QueueSize = ui32Power2QueueSize;

	if (psSysData->psQueueList == NULL) {
		eError = OSCreateResource(&psSysData->sQProcessResource);
		if (eError != PVRSRV_OK)
			goto ErrorExit;
	}

	if (OSLockResource(&psSysData->sQProcessResource,
			   KERNEL_ID) != PVRSRV_OK)
		goto ErrorExit;

	psQueueInfo->psNextKM = psSysData->psQueueList;
	psSysData->psQueueList = psQueueInfo;

	if (OSUnlockResource(&psSysData->sQProcessResource, KERNEL_ID) !=
	    PVRSRV_OK)
		goto ErrorExit;

	*ppsQueueInfo = psQueueInfo;

	return PVRSRV_OK;

ErrorExit:

	if (psQueueInfo) {
		if (psQueueInfo->pvLinQueueKM)
			OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				  psQueueInfo->ui32QueueSize,
				  psQueueInfo->pvLinQueueKM,
				  psQueueInfo->hMemBlock[1]);

		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_QUEUE_INFO),
			  psQueueInfo, psQueueInfo->hMemBlock[0]);
	}

	return PVRSRV_ERROR_GENERIC;
}

enum PVRSRV_ERROR PVRSRVDestroyCommandQueueKM(
				struct PVRSRV_QUEUE_INFO *psQueueInfo)
{
	struct PVRSRV_QUEUE_INFO *psQueue;
	struct SYS_DATA *psSysData;
	enum PVRSRV_ERROR eError;
	IMG_BOOL bTimeout = IMG_TRUE;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return eError;

	psQueue = psSysData->psQueueList;

	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US) {
		if (psQueueInfo->ui32ReadOffset ==
		    psQueueInfo->ui32WriteOffset) {
			bTimeout = IMG_FALSE;
			break;
		}
		OSWaitus(MAX_HW_TIME_US / WAIT_TRY_COUNT);
	}
	END_LOOP_UNTIL_TIMEOUT();

	if (bTimeout) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVDestroyCommandQueueKM : Failed to empty queue");
		eError = PVRSRV_ERROR_CANNOT_FLUSH_QUEUE;
		goto ErrorExit;
	}

	eError = OSLockResource(&psSysData->sQProcessResource, KERNEL_ID);
	if (eError != PVRSRV_OK)
		goto ErrorExit;

	if (psQueue == psQueueInfo) {
		psSysData->psQueueList = psQueueInfo->psNextKM;

		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			  psQueueInfo->ui32QueueSize,
			  psQueueInfo->pvLinQueueKM, psQueueInfo->hMemBlock[1]);
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_QUEUE_INFO),
			  psQueueInfo, psQueueInfo->hMemBlock[0]);
	} else {
		while (psQueue) {
			if (psQueue->psNextKM == psQueueInfo) {
				psQueue->psNextKM = psQueueInfo->psNextKM;

				OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					  psQueueInfo->ui32QueueSize,
					  psQueueInfo->pvLinQueueKM,
					  psQueueInfo->hMemBlock[1]);
				OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					  sizeof(struct PVRSRV_QUEUE_INFO),
					  psQueueInfo,
					  psQueueInfo->hMemBlock[0]);
				break;
			}
			psQueue = psQueue->psNextKM;
		}

		if (!psQueue) {
			eError =
			    OSUnlockResource(&psSysData->sQProcessResource,
					     KERNEL_ID);
			if (eError != PVRSRV_OK)
				goto ErrorExit;
			eError = PVRSRV_ERROR_INVALID_PARAMS;
			goto ErrorExit;
		}
	}

	eError = OSUnlockResource(&psSysData->sQProcessResource, KERNEL_ID);
	if (eError != PVRSRV_OK)
		goto ErrorExit;

	if (psSysData->psQueueList == NULL) {
		eError = OSDestroyResource(&psSysData->sQProcessResource);
		if (eError != PVRSRV_OK)
			goto ErrorExit;
	}

ErrorExit:

	return eError;
}

enum PVRSRV_ERROR PVRSRVGetQueueSpaceKM(struct PVRSRV_QUEUE_INFO *psQueue,
				    u32 ui32ParamSize, void **ppvSpace)
{
	IMG_BOOL bTimeout = IMG_TRUE;

	ui32ParamSize = (ui32ParamSize + 3) & 0xFFFFFFFC;

	if (ui32ParamSize > PVRSRV_MAX_CMD_SIZE) {
		PVR_DPF(PVR_DBG_WARNING,
			 "PVRSRVGetQueueSpace: max command size is %d bytes",
			 PVRSRV_MAX_CMD_SIZE);
		return PVRSRV_ERROR_CMD_TOO_BIG;
	}

	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US) {
		if (GET_SPACE_IN_CMDQ(psQueue) > ui32ParamSize) {
			bTimeout = IMG_FALSE;
			break;
		}
		OSWaitus(MAX_HW_TIME_US / WAIT_TRY_COUNT);
	}
	END_LOOP_UNTIL_TIMEOUT();

	if (bTimeout == IMG_TRUE) {
		*ppvSpace = NULL;

		return PVRSRV_ERROR_CANNOT_GET_QUEUE_SPACE;
	} else {
		*ppvSpace = (void *)(psQueue->ui32WriteOffset +
						(u32)psQueue->pvLinQueueUM);
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVInsertCommandKM(struct PVRSRV_QUEUE_INFO *psQueue,
				   struct PVRSRV_COMMAND **ppsCommand,
				   u32 ui32DevIndex, u16 CommandType,
				   u32 ui32DstSyncCount,
				   struct PVRSRV_KERNEL_SYNC_INFO *apsDstSync[],
				   u32 ui32SrcSyncCount,
				   struct PVRSRV_KERNEL_SYNC_INFO *apsSrcSync[],
				   u32 ui32DataByteSize)
{
	enum PVRSRV_ERROR eError;
	struct PVRSRV_COMMAND *psCommand;
	u32 ui32CommandSize;
	u32 i;

	ui32DataByteSize = (ui32DataByteSize + 3) & 0xFFFFFFFC;

	ui32CommandSize = sizeof(struct PVRSRV_COMMAND) +
	    ((ui32DstSyncCount + ui32SrcSyncCount) *
	     sizeof(struct PVRSRV_SYNC_OBJECT)) + ui32DataByteSize;

	eError = PVRSRVGetQueueSpaceKM(psQueue, ui32CommandSize,
				  (void **) &psCommand);
	if (eError != PVRSRV_OK)
		return eError;

	psCommand->ui32ProcessID = OSGetCurrentProcessIDKM();

	psCommand->ui32CmdSize = ui32CommandSize;
	psCommand->ui32DevIndex = ui32DevIndex;
	psCommand->CommandType = CommandType;
	psCommand->ui32DstSyncCount = ui32DstSyncCount;
	psCommand->ui32SrcSyncCount = ui32SrcSyncCount;
	psCommand->psDstSync =
	    (struct PVRSRV_SYNC_OBJECT *)(((u8 *) psCommand) +
				    sizeof(struct PVRSRV_COMMAND));

	psCommand->psSrcSync =
	    (struct PVRSRV_SYNC_OBJECT *)(((u8 *) psCommand->psDstSync) +
				    (ui32DstSyncCount *
				     sizeof(struct PVRSRV_SYNC_OBJECT)));

	psCommand->pvData =
	    (struct PVRSRV_SYNC_OBJECT *)(((u8 *) psCommand->psSrcSync) +
				    (ui32SrcSyncCount *
				     sizeof(struct PVRSRV_SYNC_OBJECT)));

	psCommand->ui32DataSize = ui32DataByteSize;

	for (i = 0; i < ui32DstSyncCount; i++) {
		psCommand->psDstSync[i].psKernelSyncInfoKM = apsDstSync[i];
		psCommand->psDstSync[i].ui32WriteOpsPending =
		    PVRSRVGetWriteOpsPending(apsDstSync[i], IMG_FALSE);
		psCommand->psDstSync[i].ui32ReadOpsPending =
		    PVRSRVGetReadOpsPending(apsDstSync[i], IMG_FALSE);
	}

	for (i = 0; i < ui32SrcSyncCount; i++) {
		psCommand->psSrcSync[i].psKernelSyncInfoKM = apsSrcSync[i];
		psCommand->psSrcSync[i].ui32WriteOpsPending =
		    PVRSRVGetWriteOpsPending(apsSrcSync[i], IMG_TRUE);
		psCommand->psSrcSync[i].ui32ReadOpsPending =
		    PVRSRVGetReadOpsPending(apsSrcSync[i], IMG_TRUE);
	}

	*ppsCommand = psCommand;

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVSubmitCommandKM(struct PVRSRV_QUEUE_INFO *psQueue,
				    struct PVRSRV_COMMAND *psCommand)
{

	if (psCommand->ui32DstSyncCount > 0) {
		psCommand->psDstSync = (struct PVRSRV_SYNC_OBJECT *)
					(((u8 *)psQueue->pvLinQueueKM) +
					 psQueue->ui32WriteOffset +
					 sizeof(struct PVRSRV_COMMAND));
	}

	if (psCommand->ui32SrcSyncCount > 0) {
		psCommand->psSrcSync = (struct PVRSRV_SYNC_OBJECT *)
					(((u8 *)psQueue->pvLinQueueKM) +
					psQueue->ui32WriteOffset +
					sizeof(struct PVRSRV_COMMAND) +
					(psCommand->ui32DstSyncCount *
					   sizeof(struct PVRSRV_SYNC_OBJECT)));
	}

	psCommand->pvData = (struct PVRSRV_SYNC_OBJECT *)
			(((u8 *)psQueue->pvLinQueueKM) +
			 psQueue->ui32WriteOffset +
			 sizeof(struct PVRSRV_COMMAND) +
			 (psCommand->ui32DstSyncCount *
				sizeof(struct PVRSRV_SYNC_OBJECT)) +
			 (psCommand->ui32SrcSyncCount *
				sizeof(struct PVRSRV_SYNC_OBJECT)));

	UPDATE_QUEUE_WOFF(psQueue, psCommand->ui32CmdSize);

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR PVRSRVProcessCommand(struct SYS_DATA *psSysData,
				  struct PVRSRV_COMMAND *psCommand,
				  IMG_BOOL bFlush)
{
	struct PVRSRV_SYNC_OBJECT *psWalkerObj;
	struct PVRSRV_SYNC_OBJECT *psEndObj;
	u32 i;
	struct COMMAND_COMPLETE_DATA *psCmdCompleteData;
	enum PVRSRV_ERROR eError = PVRSRV_OK;
	u32 ui32WriteOpsComplete;
	u32 ui32ReadOpsComplete;

	psWalkerObj = psCommand->psDstSync;
	psEndObj = psWalkerObj + psCommand->ui32DstSyncCount;
	while (psWalkerObj < psEndObj) {
		struct PVRSRV_SYNC_DATA *psSyncData =
		    psWalkerObj->psKernelSyncInfoKM->psSyncData;

		ui32WriteOpsComplete = psSyncData->ui32WriteOpsComplete;
		ui32ReadOpsComplete = psSyncData->ui32ReadOpsComplete;

		if ((ui32WriteOpsComplete != psWalkerObj->ui32WriteOpsPending)
		    || (ui32ReadOpsComplete !=
			psWalkerObj->ui32ReadOpsPending)) {
			if (!bFlush ||
			    !SYNCOPS_STALE(ui32WriteOpsComplete,
					   psWalkerObj->ui32WriteOpsPending) ||
			    !SYNCOPS_STALE(ui32ReadOpsComplete,
					   psWalkerObj->ui32ReadOpsPending)) {
				return PVRSRV_ERROR_FAILED_DEPENDENCIES;
			}
		}

		psWalkerObj++;
	}

	psWalkerObj = psCommand->psSrcSync;
	psEndObj = psWalkerObj + psCommand->ui32SrcSyncCount;
	while (psWalkerObj < psEndObj) {
		struct PVRSRV_SYNC_DATA *psSyncData =
		    psWalkerObj->psKernelSyncInfoKM->psSyncData;

		ui32ReadOpsComplete = psSyncData->ui32ReadOpsComplete;
		ui32WriteOpsComplete = psSyncData->ui32WriteOpsComplete;

		if ((ui32WriteOpsComplete !=
		     psWalkerObj->ui32WriteOpsPending) ||
		    (ui32ReadOpsComplete !=
			psWalkerObj->ui32ReadOpsPending)) {
			if (!bFlush &&
			    SYNCOPS_STALE(ui32WriteOpsComplete,
					  psWalkerObj->ui32WriteOpsPending) &&
			    SYNCOPS_STALE(ui32ReadOpsComplete,
					psWalkerObj->ui32ReadOpsPending)) {
				PVR_DPF(PVR_DBG_WARNING,
			"PVRSRVProcessCommand: Stale syncops psSyncData:0x%x "
			"ui32WriteOpsComplete:0x%x ui32WriteOpsPending:0x%x",
					 psSyncData, ui32WriteOpsComplete,
					 psWalkerObj->ui32WriteOpsPending);
			}

			if (!bFlush ||
			    !SYNCOPS_STALE(ui32WriteOpsComplete,
					   psWalkerObj->ui32WriteOpsPending) ||
			    !SYNCOPS_STALE(ui32ReadOpsComplete,
					   psWalkerObj->ui32ReadOpsPending)) {
				return PVRSRV_ERROR_FAILED_DEPENDENCIES;
			}
		}
		psWalkerObj++;
	}

	if (psCommand->ui32DevIndex >= SYS_DEVICE_COUNT) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVProcessCommand: invalid DeviceType 0x%x",
			 psCommand->ui32DevIndex);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psCmdCompleteData =
	    psSysData->ppsCmdCompleteData[psCommand->ui32DevIndex][psCommand->
								   CommandType];
	if (psCmdCompleteData->bInUse)

		return PVRSRV_ERROR_FAILED_DEPENDENCIES;

	psCmdCompleteData->bInUse = IMG_TRUE;

	psCmdCompleteData->ui32DstSyncCount = psCommand->ui32DstSyncCount;
	for (i = 0; i < psCommand->ui32DstSyncCount; i++)
		psCmdCompleteData->psDstSync[i] = psCommand->psDstSync[i];

	psCmdCompleteData->ui32SrcSyncCount = psCommand->ui32SrcSyncCount;
	for (i = 0; i < psCommand->ui32SrcSyncCount; i++)
		psCmdCompleteData->psSrcSync[i] = psCommand->psSrcSync[i];

	if (psSysData->ppfnCmdProcList[psCommand->ui32DevIndex]
				      [psCommand->CommandType]((void *)
					   psCmdCompleteData,
					   psCommand->ui32DataSize,
					   psCommand->pvData) == IMG_FALSE) {
		psCmdCompleteData->bInUse = IMG_FALSE;
		eError = PVRSRV_ERROR_CMD_NOT_PROCESSED;
	}

	return eError;
}

enum PVRSRV_ERROR PVRSRVProcessQueues(u32 ui32CallerID, IMG_BOOL bFlush)
{
	struct PVRSRV_QUEUE_INFO *psQueue;
	struct SYS_DATA *psSysData;
	struct PVRSRV_COMMAND *psCommand;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	enum PVRSRV_ERROR eError;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return eError;

	eError = OSLockResource(&psSysData->sQProcessResource, ui32CallerID);
	if (eError != PVRSRV_OK) {
		if (ui32CallerID == ISR_ID) {
			if (bFlush) {
				PVR_DPF(PVR_DBG_ERROR, "PVRSRVProcessQueues: "
					"Couldn't acquire queue processing "
					"lock for FLUSH");
			} else {
				PVR_DPF(PVR_DBG_MESSAGE,
						"PVRSRVProcessQueues: "
						"Couldn't acquire queue "
						"processing lock");
			}
		} else {
			PVR_DPF(PVR_DBG_MESSAGE, "PVRSRVProcessQueues: "
				"Queue processing lock-acquire failed "
				"when called from the Services driver.");
			PVR_DPF(PVR_DBG_MESSAGE,
				"This is due to MISR queue processing "
				"being interrupted by the Services driver.");
		}

		return PVRSRV_OK;
	}

	psQueue = psSysData->psQueueList;

	if (!psQueue) {
		PVR_DPF(PVR_DBG_MESSAGE,
			 "No Queues installed - cannot process commands");
	}

	if (bFlush)
		PVRSRVSetDCState(DC_STATE_FLUSH_COMMANDS);

	while (psQueue) {
		while (psQueue->ui32ReadOffset != psQueue->ui32WriteOffset) {
			psCommand = (struct PVRSRV_COMMAND *)((u32) psQueue->
					pvLinQueueKM + psQueue->ui32ReadOffset);

			if (PVRSRVProcessCommand(psSysData, psCommand, bFlush)
			    == PVRSRV_OK) {
				UPDATE_QUEUE_ROFF(psQueue,
						  psCommand->ui32CmdSize)
				if (bFlush)
					continue;
			}
			break;
		}
		psQueue = psQueue->psNextKM;
	}

	if (bFlush)
		PVRSRVSetDCState(DC_STATE_NO_FLUSH_COMMANDS);

	psDeviceNode = psSysData->psDeviceNodeList;
	while (psDeviceNode != NULL) {
		if (psDeviceNode->bReProcessDeviceCommandComplete &&
		    psDeviceNode->pfnDeviceCommandComplete != NULL) {
			(*psDeviceNode->
			 pfnDeviceCommandComplete) (psDeviceNode);
		}
		psDeviceNode = psDeviceNode->psNext;
	}

	OSUnlockResource(&psSysData->sQProcessResource, ui32CallerID);

	return PVRSRV_OK;
}

void PVRSRVCommandCompleteKM(void *hCmdCookie, IMG_BOOL bScheduleMISR)
{
	u32 i;
	struct COMMAND_COMPLETE_DATA *psCmdCompleteData =
	    (struct COMMAND_COMPLETE_DATA *)hCmdCookie;
	struct SYS_DATA *psSysData;

	if (SysAcquireData(&psSysData) != PVRSRV_OK)
		return;

	for (i = 0; i < psCmdCompleteData->ui32DstSyncCount; i++) {
		psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM->psSyncData->
		    ui32WriteOpsComplete++;
	}

	for (i = 0; i < psCmdCompleteData->ui32SrcSyncCount; i++) {
		psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM->psSyncData->
		    ui32ReadOpsComplete++;
	}

	psCmdCompleteData->bInUse = IMG_FALSE;

	PVRSRVCommandCompleteCallbacks();

	if (bScheduleMISR)
		OSScheduleMISR(psSysData);
}

void PVRSRVCommandCompleteCallbacks(void)
{
	struct SYS_DATA *psSysData;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVCommandCompleteCallbacks: "
					"SysAcquireData failed");
		return;
	}

	psDeviceNode = psSysData->psDeviceNodeList;
	while (psDeviceNode != NULL) {
		if (psDeviceNode->pfnDeviceCommandComplete != NULL)
			(*psDeviceNode->pfnDeviceCommandComplete)(psDeviceNode);
		psDeviceNode = psDeviceNode->psNext;
	}
}

enum PVRSRV_ERROR PVRSRVRegisterCmdProcListKM(u32 ui32DevIndex,
			     IMG_BOOL (**ppfnCmdProcList)(void *, u32, void *),
			     u32 ui32MaxSyncsPerCmd[][2], u32 ui32CmdCount)
{
	struct SYS_DATA *psSysData;
	enum PVRSRV_ERROR eError;
	u32 i;
	u32 ui32AllocSize;
	IMG_BOOL (**ppfnCmdProc)(void *, u32, void *);
	struct COMMAND_COMPLETE_DATA *psCmdCompleteData;

	if (ui32DevIndex >= SYS_DEVICE_COUNT) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRegisterCmdProcListKM: invalid DeviceType 0x%x",
			 ui32DevIndex);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRegisterCmdProcListKM: SysAcquireData failed");
		return eError;
	}

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, ui32CmdCount *
				     sizeof(IMG_BOOL (*)(void *, u32, void *)),
			    (void **)&psSysData->ppfnCmdProcList[ui32DevIndex],
			    NULL);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRegisterCmdProcListKM: Failed to alloc queue");
		return eError;
	}

	ppfnCmdProc = psSysData->ppfnCmdProcList[ui32DevIndex];

	for (i = 0; i < ui32CmdCount; i++)
		ppfnCmdProc[i] = ppfnCmdProcList[i];

	ui32AllocSize = ui32CmdCount * sizeof(struct COMMAND_COMPLETE_DATA *);
	eError = OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			    ui32AllocSize,
			    (void **) &psSysData->
			    ppsCmdCompleteData[ui32DevIndex], NULL);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			"PVRSRVRegisterCmdProcListKM: Failed to alloc CC data");
		goto ErrorExit;
	}

	/* clear the list to ensure that we don't try to access uninitialised
	 * pointer in the 'error' execution path */
	OSMemSet(psSysData->ppsCmdCompleteData[ui32DevIndex], 0x00,
		 ui32AllocSize);

	for (i = 0; i < ui32CmdCount; i++) {
		ui32AllocSize = sizeof(struct COMMAND_COMPLETE_DATA)
		    + ((ui32MaxSyncsPerCmd[i][0] + ui32MaxSyncsPerCmd[i][1])
		       * sizeof(struct PVRSRV_SYNC_OBJECT));

		eError = OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				    ui32AllocSize,
				    (void **)&psSysData->
					    ppsCmdCompleteData[ui32DevIndex][i],
				    NULL);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "PVRSRVRegisterCmdProcListKM: "
						"Failed to alloc cmd %d",
				 i);
			goto ErrorExit;
		}

		OSMemSet(psSysData->ppsCmdCompleteData[ui32DevIndex][i], 0x00,
			 ui32AllocSize);

		psCmdCompleteData =
		    psSysData->ppsCmdCompleteData[ui32DevIndex][i];

		psCmdCompleteData->psDstSync = (struct PVRSRV_SYNC_OBJECT *)
		     (((u32) psCmdCompleteData) +
					sizeof(struct COMMAND_COMPLETE_DATA));
		psCmdCompleteData->psSrcSync = (struct PVRSRV_SYNC_OBJECT *)
		     (((u32) psCmdCompleteData->psDstSync) +
					(sizeof(struct PVRSRV_SYNC_OBJECT) *
						 ui32MaxSyncsPerCmd[i][0]));
		psCmdCompleteData->ui32AllocSize = ui32AllocSize;
	}

	return PVRSRV_OK;

ErrorExit:

	if (psSysData->ppsCmdCompleteData[ui32DevIndex] != NULL) {
		for (i = 0; i < ui32CmdCount; i++) {
			if (psSysData->ppsCmdCompleteData[ui32DevIndex][i] !=
			    NULL) {
				psCmdCompleteData =
				    psSysData->
					    ppsCmdCompleteData[ui32DevIndex][i];
				OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					  psCmdCompleteData->ui32AllocSize,
					  psCmdCompleteData, NULL);
			}
		}

		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			  ui32CmdCount * sizeof(struct COMMAND_COMPLETE_DATA *),
			  psSysData->ppsCmdCompleteData[ui32DevIndex],
			  NULL);
	}

	if (psSysData->ppfnCmdProcList[ui32DevIndex] != NULL) {
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP, 0,
			  psSysData->ppfnCmdProcList[ui32DevIndex], NULL);
	}

	return eError;
}

enum PVRSRV_ERROR PVRSRVRemoveCmdProcListKM(u32 ui32DevIndex, u32 ui32CmdCount)
{
	struct SYS_DATA *psSysData;
	enum PVRSRV_ERROR eError;
	u32 i;

	if (ui32DevIndex >= SYS_DEVICE_COUNT) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRemoveCmdProcListKM: invalid DeviceType 0x%x",
			 ui32DevIndex);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRemoveCmdProcListKM: SysAcquireData failed");
		return eError;
	}

	if (psSysData->ppsCmdCompleteData[ui32DevIndex] == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRemoveCmdProcListKM: Invalid command array");
		return PVRSRV_ERROR_INVALID_PARAMS;
	} else {
		for (i = 0; i < ui32CmdCount; i++) {

			if (psSysData->ppsCmdCompleteData[ui32DevIndex][i] !=
			    NULL) {
				struct COMMAND_COMPLETE_DATA *
					psCmdCompleteData = psSysData->
					ppsCmdCompleteData[ui32DevIndex][i];
				OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					  psCmdCompleteData->ui32AllocSize,
					  psCmdCompleteData, NULL);
			}
		}

		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			  ui32CmdCount * sizeof(struct COMMAND_COMPLETE_DATA *),
			  psSysData->ppsCmdCompleteData[ui32DevIndex], NULL);
	}

	if (psSysData->ppfnCmdProcList[ui32DevIndex] != NULL) {
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			  ui32CmdCount *
				sizeof(IMG_BOOL (*)(void *, u32, void *)),
			  psSysData->ppfnCmdProcList[ui32DevIndex], NULL);
	}

	return PVRSRV_OK;
}
