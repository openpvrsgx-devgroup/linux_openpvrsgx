/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
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
#include "pvr_bridge_km.h"

#include "lists.h"
#include "ttrace.h"

#if defined(SUPPORT_DC_CMDCOMPLETE_WHEN_NO_LONGER_DISPLAYED)
#define DC_NUM_COMMANDS_PER_TYPE		2
#else
#define DC_NUM_COMMANDS_PER_TYPE		1
#endif

typedef struct _DEVICE_COMMAND_DATA_
{
	PFN_CMD_PROC			pfnCmdProc;
	PCOMMAND_COMPLETE_DATA	apsCmdCompleteData[DC_NUM_COMMANDS_PER_TYPE];
	IMG_UINT32				ui32CCBOffset;
	IMG_UINT32				ui32MaxDstSyncCount;	
	IMG_UINT32				ui32MaxSrcSyncCount;	
} DEVICE_COMMAND_DATA;


#if defined(__linux__) && defined(__KERNEL__)

#include "proc.h"

void ProcSeqShowQueue(struct seq_file *sfile,void* el)
{
	PVRSRV_QUEUE_INFO *psQueue = (PVRSRV_QUEUE_INFO*)el;
	IMG_INT cmds = 0;
	IMG_SIZE_T ui32ReadOffset;
	IMG_SIZE_T ui32WriteOffset;
	PVRSRV_COMMAND *psCmd;

	if(el == PVR_PROC_SEQ_START_TOKEN)
	{
		seq_printf( sfile,
					"Command Queues\n"
					"Queue    CmdPtr      Pid Command Size DevInd  DSC  SSC  #Data ...\n");
		return;
	}

	ui32ReadOffset = psQueue->ui32ReadOffset;
	ui32WriteOffset = psQueue->ui32WriteOffset;

	while (ui32ReadOffset != ui32WriteOffset)
	{
		psCmd= (PVRSRV_COMMAND *)((IMG_UINTPTR_T)psQueue->pvLinQueueKM + ui32ReadOffset);

		seq_printf(sfile, "%x %x  %5u  %6u  %3u  %5u   %2u   %2u    %3u  \n",
							(IMG_UINTPTR_T)psQueue,
							(IMG_UINTPTR_T)psCmd,
					 		psCmd->ui32ProcessID,
							psCmd->CommandType,
							psCmd->uCmdSize,
							psCmd->ui32DevIndex,
							psCmd->ui32DstSyncCount,
							psCmd->ui32SrcSyncCount,
							psCmd->uDataSize);
		{
			IMG_UINT32 i;
			for (i = 0; i < psCmd->ui32SrcSyncCount; i++)
			{
				PVRSRV_SYNC_DATA *psSyncData = psCmd->psSrcSync[i].psKernelSyncInfoKM->psSyncData;
				seq_printf(sfile, "  Sync %u: ROP/ROC: 0x%x/0x%x WOP/WOC: 0x%x/0x%x ROC-VA: 0x%x WOC-VA: 0x%x\n",
									i,
									psCmd->psSrcSync[i].ui32ReadOps2Pending,
									psSyncData->ui32ReadOps2Complete,
									psCmd->psSrcSync[i].ui32WriteOpsPending,
									psSyncData->ui32WriteOpsComplete,
									psCmd->psSrcSync[i].psKernelSyncInfoKM->sReadOps2CompleteDevVAddr.uiAddr,
									psCmd->psSrcSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr);
			}
		}

		
		ui32ReadOffset += psCmd->uCmdSize;
		ui32ReadOffset &= psQueue->ui32QueueSize - 1;
		cmds++;
	}

	if (cmds == 0)
	{
		seq_printf(sfile, "%x <empty>\n", (IMG_UINTPTR_T)psQueue);
	}
}

void* ProcSeqOff2ElementQueue(struct seq_file * sfile, loff_t off)
{
	PVRSRV_QUEUE_INFO *psQueue = IMG_NULL;
	SYS_DATA *psSysData;

	PVR_UNREFERENCED_PARAMETER(sfile);

	if(!off)
	{
		return PVR_PROC_SEQ_START_TOKEN;
	}


	psSysData = SysAcquireDataNoCheck();
	if (psSysData != IMG_NULL)
	{
		for (psQueue = psSysData->psQueueList; (((--off) > 0) && (psQueue != IMG_NULL)); psQueue = psQueue->psNextKM);
	}

	return psQueue;
}
#endif 

#define GET_SPACE_IN_CMDQ(psQueue)										\
	((((psQueue)->ui32ReadOffset - (psQueue)->ui32WriteOffset)				\
	+ ((psQueue)->ui32QueueSize - 1)) & ((psQueue)->ui32QueueSize - 1))

#define UPDATE_QUEUE_WOFF(psQueue, ui32Size)							\
	(psQueue)->ui32WriteOffset = ((psQueue)->ui32WriteOffset + (ui32Size))	\
	& ((psQueue)->ui32QueueSize - 1);

#define SYNCOPS_STALE(ui32OpsComplete, ui32OpsPending)					\
	((ui32OpsComplete) >= (ui32OpsPending))

#ifdef INLINE_IS_PRAGMA
#pragma inline(PVRSRVGetWriteOpsPending)
#endif
static INLINE
IMG_UINT32 PVRSRVGetWriteOpsPending(PVRSRV_KERNEL_SYNC_INFO *psSyncInfo, IMG_BOOL bIsReadOp)
{
	IMG_UINT32 ui32WriteOpsPending;

	if(bIsReadOp)
	{
		ui32WriteOpsPending = psSyncInfo->psSyncData->ui32WriteOpsPending;
	}
	else
	{
		


		ui32WriteOpsPending = psSyncInfo->psSyncData->ui32WriteOpsPending++;
	}

	return ui32WriteOpsPending;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(PVRSRVGetReadOpsPending)
#endif
static INLINE
IMG_UINT32 PVRSRVGetReadOpsPending(PVRSRV_KERNEL_SYNC_INFO *psSyncInfo, IMG_BOOL bIsReadOp)
{
	IMG_UINT32 ui32ReadOpsPending;

	if(bIsReadOp)
	{
		ui32ReadOpsPending = psSyncInfo->psSyncData->ui32ReadOps2Pending++;
	}
	else
	{
		ui32ReadOpsPending = psSyncInfo->psSyncData->ui32ReadOps2Pending;
	}

	return ui32ReadOpsPending;
}

static IMG_VOID QueueDumpCmdComplete(COMMAND_COMPLETE_DATA *psCmdCompleteData,
									 IMG_UINT32				i,
									 IMG_BOOL				bIsSrc)
{
	PVRSRV_SYNC_OBJECT	*psSyncObject;

	psSyncObject = bIsSrc ? psCmdCompleteData->psSrcSync : psCmdCompleteData->psDstSync;

	if (psCmdCompleteData->bInUse)
	{
		PVR_LOG(("\t%s %u: ROC DevVAddr:0x%X ROP:0x%x ROC:0x%x, WOC DevVAddr:0x%X WOP:0x%x WOC:0x%x",
				bIsSrc ? "SRC" : "DEST", i,
				psSyncObject[i].psKernelSyncInfoKM->sReadOps2CompleteDevVAddr.uiAddr,
				psSyncObject[i].psKernelSyncInfoKM->psSyncData->ui32ReadOps2Pending,
				psSyncObject[i].psKernelSyncInfoKM->psSyncData->ui32ReadOps2Complete,
				psSyncObject[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psSyncObject[i].psKernelSyncInfoKM->psSyncData->ui32WriteOpsPending,
				psSyncObject[i].psKernelSyncInfoKM->psSyncData->ui32WriteOpsComplete))
	}
	else
	{
		PVR_LOG(("\t%s %u: (Not in use)", bIsSrc ? "SRC" : "DEST", i))
	}
}


static IMG_VOID QueueDumpDebugInfo_ForEachCb(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	if (psDeviceNode->sDevId.eDeviceClass == PVRSRV_DEVICE_CLASS_DISPLAY)
	{
		IMG_UINT32				ui32CmdCounter, ui32SyncCounter;
		SYS_DATA				*psSysData;
		DEVICE_COMMAND_DATA		*psDeviceCommandData;
		PCOMMAND_COMPLETE_DATA	psCmdCompleteData;

		SysAcquireData(&psSysData);

		psDeviceCommandData = psSysData->apsDeviceCommandData[psDeviceNode->sDevId.ui32DeviceIndex];

		if (psDeviceCommandData != IMG_NULL)
		{
			for (ui32CmdCounter = 0; ui32CmdCounter < DC_NUM_COMMANDS_PER_TYPE; ui32CmdCounter++)
			{
				psCmdCompleteData = psDeviceCommandData[DC_FLIP_COMMAND].apsCmdCompleteData[ui32CmdCounter];

				PVR_LOG(("Flip Command Complete Data %u for display device %u:",
						ui32CmdCounter, psDeviceNode->sDevId.ui32DeviceIndex))

				for (ui32SyncCounter = 0;
					 ui32SyncCounter < psCmdCompleteData->ui32SrcSyncCount;
					 ui32SyncCounter++)
				{
					QueueDumpCmdComplete(psCmdCompleteData, ui32SyncCounter, IMG_TRUE);
				}

				for (ui32SyncCounter = 0;
					 ui32SyncCounter < psCmdCompleteData->ui32DstSyncCount;
					 ui32SyncCounter++)
				{
					QueueDumpCmdComplete(psCmdCompleteData, ui32SyncCounter, IMG_FALSE);
				}
			}
		}
		else
		{
			PVR_LOG(("There is no Command Complete Data for display device %u", psDeviceNode->sDevId.ui32DeviceIndex))
		}
	}
}


IMG_VOID QueueDumpDebugInfo(IMG_VOID)
{
	SYS_DATA	*psSysData;
	SysAcquireData(&psSysData);
	List_PVRSRV_DEVICE_NODE_ForEach(psSysData->psDeviceNodeList, &QueueDumpDebugInfo_ForEachCb);
}


static IMG_SIZE_T NearestPower2(IMG_SIZE_T ui32Value)
{
	IMG_SIZE_T ui32Temp, ui32Result = 1;

	if(!ui32Value)
		return 0;

	ui32Temp = ui32Value - 1;
	while(ui32Temp)
	{
		ui32Result <<= 1;
		ui32Temp >>= 1;
	}

	return ui32Result;
}


IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVCreateCommandQueueKM(IMG_SIZE_T ui32QueueSize,
													 PVRSRV_QUEUE_INFO **ppsQueueInfo)
{
	PVRSRV_QUEUE_INFO	*psQueueInfo;
	IMG_SIZE_T			ui32Power2QueueSize = NearestPower2(ui32QueueSize);
	SYS_DATA			*psSysData;
	PVRSRV_ERROR		eError;
	IMG_HANDLE			hMemBlock;

	SysAcquireData(&psSysData);

	
	eError = OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					 sizeof(PVRSRV_QUEUE_INFO),
					 (IMG_VOID **)&psQueueInfo, &hMemBlock,
					 "Queue Info");
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVCreateCommandQueueKM: Failed to alloc queue struct"));
		goto ErrorExit;
	}
	OSMemSet(psQueueInfo, 0, sizeof(PVRSRV_QUEUE_INFO));

	psQueueInfo->hMemBlock[0] = hMemBlock;
	psQueueInfo->ui32ProcessID = OSGetCurrentProcessIDKM();

	
	eError = OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					 ui32Power2QueueSize + PVRSRV_MAX_CMD_SIZE,
					 &psQueueInfo->pvLinQueueKM, &hMemBlock,
					 "Command Queue");
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVCreateCommandQueueKM: Failed to alloc queue buffer"));
		goto ErrorExit;
	}

	psQueueInfo->hMemBlock[1] = hMemBlock;
	psQueueInfo->pvLinQueueUM = psQueueInfo->pvLinQueueKM;

	
	PVR_ASSERT(psQueueInfo->ui32ReadOffset == 0);
	PVR_ASSERT(psQueueInfo->ui32WriteOffset == 0);

	psQueueInfo->ui32QueueSize = ui32Power2QueueSize;

	
	if (psSysData->psQueueList == IMG_NULL)
	{
		eError = OSCreateResource(&psSysData->sQProcessResource);
		if (eError != PVRSRV_OK)
		{
			goto ErrorExit;
		}
	}

	
	eError = OSLockResource(&psSysData->sQProcessResource,
							KERNEL_ID);
	if (eError != PVRSRV_OK)
	{
		goto ErrorExit;
	}

	psQueueInfo->psNextKM = psSysData->psQueueList;
	psSysData->psQueueList = psQueueInfo;

	eError = OSUnlockResource(&psSysData->sQProcessResource, KERNEL_ID);
	if (eError != PVRSRV_OK)
	{
		goto ErrorExit;
	}

	*ppsQueueInfo = psQueueInfo;

	return PVRSRV_OK;

ErrorExit:

	if(psQueueInfo)
	{
		if(psQueueInfo->pvLinQueueKM)
		{
			OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
						psQueueInfo->ui32QueueSize,
						psQueueInfo->pvLinQueueKM,
						psQueueInfo->hMemBlock[1]);
			psQueueInfo->pvLinQueueKM = IMG_NULL;
		}

		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					sizeof(PVRSRV_QUEUE_INFO),
					psQueueInfo,
					psQueueInfo->hMemBlock[0]);
		
	}

	return eError;
}


IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVDestroyCommandQueueKM(PVRSRV_QUEUE_INFO *psQueueInfo)
{
	PVRSRV_QUEUE_INFO	*psQueue;
	SYS_DATA			*psSysData;
	PVRSRV_ERROR		eError;
	IMG_BOOL			bTimeout = IMG_TRUE;

	SysAcquireData(&psSysData);

	psQueue = psSysData->psQueueList;

	 
	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US)
	{
		if(psQueueInfo->ui32ReadOffset == psQueueInfo->ui32WriteOffset)
		{
			bTimeout = IMG_FALSE;
			break;
		}
		OSSleepms(1);
	} END_LOOP_UNTIL_TIMEOUT();

	if (bTimeout)
	{
		
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVDestroyCommandQueueKM : Failed to empty queue"));
		eError = PVRSRV_ERROR_CANNOT_FLUSH_QUEUE;
		goto ErrorExit;
	}

	
	eError = OSLockResource(&psSysData->sQProcessResource,
								KERNEL_ID);
	if (eError != PVRSRV_OK)
	{
		goto ErrorExit;
	}

	if(psQueue == psQueueInfo)
	{
		psSysData->psQueueList = psQueueInfo->psNextKM;

		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					NearestPower2(psQueueInfo->ui32QueueSize) + PVRSRV_MAX_CMD_SIZE,
					psQueueInfo->pvLinQueueKM,
					psQueueInfo->hMemBlock[1]);
		psQueueInfo->pvLinQueueKM = IMG_NULL;
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					sizeof(PVRSRV_QUEUE_INFO),
					psQueueInfo,
					psQueueInfo->hMemBlock[0]);
		 
		psQueueInfo = IMG_NULL; 
	}
	else
	{
		while(psQueue)
		{
			if(psQueue->psNextKM == psQueueInfo)
			{
				psQueue->psNextKM = psQueueInfo->psNextKM;

				OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
							psQueueInfo->ui32QueueSize,
							psQueueInfo->pvLinQueueKM,
							psQueueInfo->hMemBlock[1]);
				psQueueInfo->pvLinQueueKM = IMG_NULL;
				OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
							sizeof(PVRSRV_QUEUE_INFO),
							psQueueInfo,
							psQueueInfo->hMemBlock[0]);
				 
				psQueueInfo = IMG_NULL; 
				break;
			}
			psQueue = psQueue->psNextKM;
		}

		if(!psQueue)
		{
			eError = OSUnlockResource(&psSysData->sQProcessResource, KERNEL_ID);
			if (eError != PVRSRV_OK)
			{
				goto ErrorExit;
			}
			eError = PVRSRV_ERROR_INVALID_PARAMS;
			goto ErrorExit;
		}
	}

	
	eError = OSUnlockResource(&psSysData->sQProcessResource, KERNEL_ID);
	if (eError != PVRSRV_OK)
	{
		goto ErrorExit;
	}

	
	if (psSysData->psQueueList == IMG_NULL)
	{
		eError = OSDestroyResource(&psSysData->sQProcessResource);
		if (eError != PVRSRV_OK)
		{
			goto ErrorExit;
		}
	}

ErrorExit:

	return eError;
}


IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetQueueSpaceKM(PVRSRV_QUEUE_INFO *psQueue,
												IMG_SIZE_T ui32ParamSize,
												IMG_VOID **ppvSpace)
{
	IMG_BOOL bTimeout = IMG_TRUE;

	
	ui32ParamSize =  (ui32ParamSize+3) & 0xFFFFFFFC;

	if (ui32ParamSize > PVRSRV_MAX_CMD_SIZE)
	{
		PVR_DPF((PVR_DBG_WARNING,"PVRSRVGetQueueSpace: max command size is %d bytes", PVRSRV_MAX_CMD_SIZE));
		return PVRSRV_ERROR_CMD_TOO_BIG;
	}

	 
	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US)
	{
		if (GET_SPACE_IN_CMDQ(psQueue) > ui32ParamSize)
		{
			bTimeout = IMG_FALSE;
			break;
		}
		OSSleepms(1);
	} END_LOOP_UNTIL_TIMEOUT();

	if (bTimeout == IMG_TRUE)
	{
		*ppvSpace = IMG_NULL;

		return PVRSRV_ERROR_CANNOT_GET_QUEUE_SPACE;
	}
	else
	{
		*ppvSpace = (IMG_VOID *)((IMG_UINTPTR_T)psQueue->pvLinQueueUM + psQueue->ui32WriteOffset);
	}

	return PVRSRV_OK;
}


IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVInsertCommandKM(PVRSRV_QUEUE_INFO	*psQueue,
												PVRSRV_COMMAND		**ppsCommand,
												IMG_UINT32			ui32DevIndex,
												IMG_UINT16			CommandType,
												IMG_UINT32			ui32DstSyncCount,
												PVRSRV_KERNEL_SYNC_INFO	*apsDstSync[],
												IMG_UINT32			ui32SrcSyncCount,
												PVRSRV_KERNEL_SYNC_INFO	*apsSrcSync[],
												IMG_SIZE_T			ui32DataByteSize,
												PFN_QUEUE_COMMAND_COMPLETE pfnCommandComplete,
												IMG_HANDLE			hCallbackData)
{
	PVRSRV_ERROR 	eError;
	PVRSRV_COMMAND	*psCommand;
	IMG_SIZE_T		ui32CommandSize;
	IMG_UINT32		i;
	SYS_DATA *psSysData;
	DEVICE_COMMAND_DATA *psDeviceCommandData;

	
	SysAcquireData(&psSysData);
	psDeviceCommandData = psSysData->apsDeviceCommandData[ui32DevIndex];

	if ((psDeviceCommandData[CommandType].ui32MaxDstSyncCount < ui32DstSyncCount) ||
	   (psDeviceCommandData[CommandType].ui32MaxSrcSyncCount < ui32SrcSyncCount))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVInsertCommandKM: Too many syncs"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	
	ui32DataByteSize = (ui32DataByteSize + 3UL) & ~3UL;

	
	ui32CommandSize = sizeof(PVRSRV_COMMAND)
					+ ((ui32DstSyncCount + ui32SrcSyncCount) * sizeof(PVRSRV_SYNC_OBJECT))
					+ ui32DataByteSize;

	
	eError = PVRSRVGetQueueSpaceKM (psQueue, ui32CommandSize, (IMG_VOID**)&psCommand);
	if(eError != PVRSRV_OK)
	{
		return eError;
	}

	psCommand->ui32ProcessID	= OSGetCurrentProcessIDKM();

	
	psCommand->uCmdSize		= ui32CommandSize; 
	psCommand->ui32DevIndex 	= ui32DevIndex;
	psCommand->CommandType 		= CommandType;
	psCommand->ui32DstSyncCount	= ui32DstSyncCount;
	psCommand->ui32SrcSyncCount	= ui32SrcSyncCount;
	
	
	psCommand->psDstSync		= (PVRSRV_SYNC_OBJECT*)(((IMG_UINTPTR_T)psCommand) + sizeof(PVRSRV_COMMAND));


	psCommand->psSrcSync		= (PVRSRV_SYNC_OBJECT*)(((IMG_UINTPTR_T)psCommand->psDstSync)
								+ (ui32DstSyncCount * sizeof(PVRSRV_SYNC_OBJECT)));

	psCommand->pvData			= (PVRSRV_SYNC_OBJECT*)(((IMG_UINTPTR_T)psCommand->psSrcSync)
								+ (ui32SrcSyncCount * sizeof(PVRSRV_SYNC_OBJECT)));
	psCommand->uDataSize		= ui32DataByteSize;

	psCommand->pfnCommandComplete = pfnCommandComplete;
	psCommand->hCallbackData = hCallbackData;

	PVR_TTRACE(PVRSRV_TRACE_GROUP_QUEUE, PVRSRV_TRACE_CLASS_CMD_START, QUEUE_TOKEN_INSERTKM);
	PVR_TTRACE_UI32(PVRSRV_TRACE_GROUP_QUEUE, PVRSRV_TRACE_CLASS_NONE,
			QUEUE_TOKEN_COMMAND_TYPE, CommandType);

	
	for (i=0; i<ui32DstSyncCount; i++)
	{
		PVR_TTRACE_SYNC_OBJECT(PVRSRV_TRACE_GROUP_QUEUE, QUEUE_TOKEN_DST_SYNC,
						apsDstSync[i], PVRSRV_SYNCOP_SAMPLE);

		psCommand->psDstSync[i].psKernelSyncInfoKM = apsDstSync[i];
		psCommand->psDstSync[i].ui32WriteOpsPending = PVRSRVGetWriteOpsPending(apsDstSync[i], IMG_FALSE);
		psCommand->psDstSync[i].ui32ReadOps2Pending = PVRSRVGetReadOpsPending(apsDstSync[i], IMG_FALSE);

		PVRSRVKernelSyncInfoIncRef(apsDstSync[i], IMG_NULL);

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVInsertCommandKM: Dst %u RO-VA:0x%x WO-VA:0x%x ROP:0x%x WOP:0x%x",
				i, psCommand->psDstSync[i].psKernelSyncInfoKM->sReadOps2CompleteDevVAddr.uiAddr,
				psCommand->psDstSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCommand->psDstSync[i].ui32ReadOps2Pending,
				psCommand->psDstSync[i].ui32WriteOpsPending));
	}

	
	for (i=0; i<ui32SrcSyncCount; i++)
	{
		PVR_TTRACE_SYNC_OBJECT(PVRSRV_TRACE_GROUP_QUEUE, QUEUE_TOKEN_DST_SYNC,
						apsSrcSync[i], PVRSRV_SYNCOP_SAMPLE);

		psCommand->psSrcSync[i].psKernelSyncInfoKM = apsSrcSync[i];
		psCommand->psSrcSync[i].ui32WriteOpsPending = PVRSRVGetWriteOpsPending(apsSrcSync[i], IMG_TRUE);
		psCommand->psSrcSync[i].ui32ReadOps2Pending = PVRSRVGetReadOpsPending(apsSrcSync[i], IMG_TRUE);

		PVRSRVKernelSyncInfoIncRef(apsSrcSync[i], IMG_NULL);

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVInsertCommandKM: Src %u RO-VA:0x%x WO-VA:0x%x ROP:0x%x WOP:0x%x",
				i, psCommand->psSrcSync[i].psKernelSyncInfoKM->sReadOps2CompleteDevVAddr.uiAddr,
				psCommand->psSrcSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCommand->psSrcSync[i].ui32ReadOps2Pending,
				psCommand->psSrcSync[i].ui32WriteOpsPending));
	}
	PVR_TTRACE(PVRSRV_TRACE_GROUP_QUEUE, PVRSRV_TRACE_CLASS_CMD_END, QUEUE_TOKEN_INSERTKM);

	
	*ppsCommand = psCommand;

	return PVRSRV_OK;
}


IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSubmitCommandKM(PVRSRV_QUEUE_INFO *psQueue,
												PVRSRV_COMMAND *psCommand)
{
	
	
	
	if (psCommand->ui32DstSyncCount > 0)
	{
		psCommand->psDstSync = (PVRSRV_SYNC_OBJECT*)(((IMG_UINTPTR_T)psQueue->pvLinQueueKM)
									+ psQueue->ui32WriteOffset + sizeof(PVRSRV_COMMAND));
	}

	if (psCommand->ui32SrcSyncCount > 0)
	{
		psCommand->psSrcSync = (PVRSRV_SYNC_OBJECT*)(((IMG_UINTPTR_T)psQueue->pvLinQueueKM)
									+ psQueue->ui32WriteOffset + sizeof(PVRSRV_COMMAND)
									+ (psCommand->ui32DstSyncCount * sizeof(PVRSRV_SYNC_OBJECT)));
	}

	psCommand->pvData = (PVRSRV_SYNC_OBJECT*)(((IMG_UINTPTR_T)psQueue->pvLinQueueKM)
									+ psQueue->ui32WriteOffset + sizeof(PVRSRV_COMMAND)
									+ (psCommand->ui32DstSyncCount * sizeof(PVRSRV_SYNC_OBJECT))
									+ (psCommand->ui32SrcSyncCount * sizeof(PVRSRV_SYNC_OBJECT)));

	
	UPDATE_QUEUE_WOFF(psQueue, psCommand->uCmdSize);

	return PVRSRV_OK;
}

static
PVRSRV_ERROR CheckIfSyncIsQueued(PVRSRV_SYNC_OBJECT *psSync, COMMAND_COMPLETE_DATA *psCmdData)
{
	IMG_UINT32 k;
 
	if (psCmdData->bInUse)
	{
		for (k=0;k<psCmdData->ui32SrcSyncCount;k++)
		{
			if (psSync->psKernelSyncInfoKM == psCmdData->psSrcSync[k].psKernelSyncInfoKM)
			{
				PVRSRV_SYNC_DATA *psSyncData = psSync->psKernelSyncInfoKM->psSyncData;
				IMG_UINT32 ui32WriteOpsComplete = psSyncData->ui32WriteOpsComplete;

				


				if (ui32WriteOpsComplete == psSync->ui32WriteOpsPending)
				{
					return PVRSRV_OK;
				}
				else
				{
					if (SYNCOPS_STALE(ui32WriteOpsComplete, psSync->ui32WriteOpsPending))
					{
						PVR_DPF((PVR_DBG_WARNING,
								"CheckIfSyncIsQueued: Stale syncops psSyncData:0x%x ui32WriteOpsComplete:0x%x ui32WriteOpsPending:0x%x",
								(IMG_UINTPTR_T)psSyncData, ui32WriteOpsComplete, psSync->ui32WriteOpsPending));
						return PVRSRV_OK;
					}
				}
			}
		}
	}
	return PVRSRV_ERROR_FAILED_DEPENDENCIES;
}

static
PVRSRV_ERROR PVRSRVProcessCommand(SYS_DATA			*psSysData,
								  PVRSRV_COMMAND	*psCommand,
								  IMG_BOOL			bFlush)
{
	PVRSRV_SYNC_OBJECT		*psWalkerObj;
	PVRSRV_SYNC_OBJECT		*psEndObj;
	IMG_UINT32				i;
	COMMAND_COMPLETE_DATA	*psCmdCompleteData;
	PVRSRV_ERROR			eError = PVRSRV_OK;
	IMG_UINT32				ui32WriteOpsComplete;
	IMG_UINT32				ui32ReadOpsComplete;
	DEVICE_COMMAND_DATA		*psDeviceCommandData;
	IMG_UINT32				ui32CCBOffset;

	
	psWalkerObj = psCommand->psDstSync;
	psEndObj = psWalkerObj + psCommand->ui32DstSyncCount;
	while (psWalkerObj < psEndObj)
	{
		PVRSRV_SYNC_DATA *psSyncData = psWalkerObj->psKernelSyncInfoKM->psSyncData;

		ui32WriteOpsComplete = psSyncData->ui32WriteOpsComplete;
		ui32ReadOpsComplete = psSyncData->ui32ReadOps2Complete;
		
		if ((ui32WriteOpsComplete != psWalkerObj->ui32WriteOpsPending)
		||	(ui32ReadOpsComplete != psWalkerObj->ui32ReadOps2Pending))
		{
			if (!bFlush ||
				!SYNCOPS_STALE(ui32WriteOpsComplete, psWalkerObj->ui32WriteOpsPending) ||
				!SYNCOPS_STALE(ui32ReadOpsComplete, psWalkerObj->ui32ReadOps2Pending))
			{
				return PVRSRV_ERROR_FAILED_DEPENDENCIES;
			}
		}

		psWalkerObj++;
	}

	
	psWalkerObj = psCommand->psSrcSync;
	psEndObj = psWalkerObj + psCommand->ui32SrcSyncCount;
	while (psWalkerObj < psEndObj)
	{
		PVRSRV_SYNC_DATA *psSyncData = psWalkerObj->psKernelSyncInfoKM->psSyncData;

		ui32ReadOpsComplete = psSyncData->ui32ReadOps2Complete;
		ui32WriteOpsComplete = psSyncData->ui32WriteOpsComplete;
		
		if ((ui32WriteOpsComplete != psWalkerObj->ui32WriteOpsPending)
		|| (ui32ReadOpsComplete != psWalkerObj->ui32ReadOps2Pending))
		{
			if (!bFlush &&
				SYNCOPS_STALE(ui32WriteOpsComplete, psWalkerObj->ui32WriteOpsPending) &&
				SYNCOPS_STALE(ui32ReadOpsComplete, psWalkerObj->ui32ReadOps2Pending))
			{
				PVR_DPF((PVR_DBG_WARNING,
						"PVRSRVProcessCommand: Stale syncops psSyncData:0x%x ui32WriteOpsComplete:0x%x ui32WriteOpsPending:0x%x",
						(IMG_UINTPTR_T)psSyncData, ui32WriteOpsComplete, psWalkerObj->ui32WriteOpsPending));
			}

			if (!bFlush ||
				!SYNCOPS_STALE(ui32WriteOpsComplete, psWalkerObj->ui32WriteOpsPending) ||
				!SYNCOPS_STALE(ui32ReadOpsComplete, psWalkerObj->ui32ReadOps2Pending))
			{
				IMG_UINT32 j;
				PVRSRV_ERROR eError;
				IMG_BOOL bFound = IMG_FALSE;

				psDeviceCommandData = psSysData->apsDeviceCommandData[psCommand->ui32DevIndex];
				for (j=0;j<DC_NUM_COMMANDS_PER_TYPE;j++)
				{
					eError = CheckIfSyncIsQueued(psWalkerObj, psDeviceCommandData[psCommand->CommandType].apsCmdCompleteData[j]);

					if (eError == PVRSRV_OK)
					{
						bFound = IMG_TRUE;
					}
				}
				if (!bFound)
					return PVRSRV_ERROR_FAILED_DEPENDENCIES;
			}
		}
		psWalkerObj++;
	}

	
	if (psCommand->ui32DevIndex >= SYS_DEVICE_COUNT)
	{
		PVR_DPF((PVR_DBG_ERROR,
					"PVRSRVProcessCommand: invalid DeviceType 0x%x",
					psCommand->ui32DevIndex));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	
	psDeviceCommandData = psSysData->apsDeviceCommandData[psCommand->ui32DevIndex];
	ui32CCBOffset = psDeviceCommandData[psCommand->CommandType].ui32CCBOffset;
	psCmdCompleteData = psDeviceCommandData[psCommand->CommandType].apsCmdCompleteData[ui32CCBOffset];
	if (psCmdCompleteData->bInUse)
	{
		
		return PVRSRV_ERROR_FAILED_DEPENDENCIES;
	}

	
	psCmdCompleteData->bInUse = IMG_TRUE;

	
	psCmdCompleteData->ui32DstSyncCount = psCommand->ui32DstSyncCount;
	for (i=0; i<psCommand->ui32DstSyncCount; i++)
	{
		psCmdCompleteData->psDstSync[i] = psCommand->psDstSync[i];

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVProcessCommand: Dst %u RO-VA:0x%x WO-VA:0x%x ROP:0x%x WOP:0x%x (CCB:%u)",
				i, psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM->sReadOps2CompleteDevVAddr.uiAddr,
				psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psDstSync[i].ui32ReadOps2Pending,
				psCmdCompleteData->psDstSync[i].ui32WriteOpsPending,
				ui32CCBOffset));
	}

	psCmdCompleteData->pfnCommandComplete = psCommand->pfnCommandComplete;
	psCmdCompleteData->hCallbackData = psCommand->hCallbackData;

	
	psCmdCompleteData->ui32SrcSyncCount = psCommand->ui32SrcSyncCount;
	for (i=0; i<psCommand->ui32SrcSyncCount; i++)
	{
		psCmdCompleteData->psSrcSync[i] = psCommand->psSrcSync[i];

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVProcessCommand: Src %u RO-VA:0x%x WO-VA:0x%x ROP:0x%x WOP:0x%x (CCB:%u)",
				i, psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM->sReadOps2CompleteDevVAddr.uiAddr,
				psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psSrcSync[i].ui32ReadOps2Pending,
				psCmdCompleteData->psSrcSync[i].ui32WriteOpsPending,
				ui32CCBOffset));
	}

	









	if (psDeviceCommandData[psCommand->CommandType].pfnCmdProc((IMG_HANDLE)psCmdCompleteData,
															   (IMG_UINT32)psCommand->uDataSize,
															   psCommand->pvData) == IMG_FALSE)
	{
		


		psCmdCompleteData->bInUse = IMG_FALSE;
		eError = PVRSRV_ERROR_CMD_NOT_PROCESSED;
	}
	
	
	psDeviceCommandData[psCommand->CommandType].ui32CCBOffset = (ui32CCBOffset + 1) % DC_NUM_COMMANDS_PER_TYPE;

	return eError;
}


static IMG_VOID PVRSRVProcessQueues_ForEachCb(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	if (psDeviceNode->bReProcessDeviceCommandComplete &&
		psDeviceNode->pfnDeviceCommandComplete != IMG_NULL)
	{
		(*psDeviceNode->pfnDeviceCommandComplete)(psDeviceNode);
	}
}

IMG_EXPORT
PVRSRV_ERROR PVRSRVProcessQueues(IMG_BOOL	bFlush)
{
	PVRSRV_QUEUE_INFO 	*psQueue;
	SYS_DATA			*psSysData;
	PVRSRV_COMMAND 		*psCommand;
	SysAcquireData(&psSysData);

	

	while (OSLockResource(&psSysData->sQProcessResource, ISR_ID) != PVRSRV_OK)
	{
		OSWaitus(1);
	};
	
	psQueue = psSysData->psQueueList;

	if(!psQueue)
	{
		PVR_DPF((PVR_DBG_MESSAGE,"No Queues installed - cannot process commands"));
	}

	if (bFlush)
	{
		PVRSRVSetDCState(DC_STATE_FLUSH_COMMANDS);
	}

	while (psQueue)
	{
		while (psQueue->ui32ReadOffset != psQueue->ui32WriteOffset)
		{
			psCommand = (PVRSRV_COMMAND*)((IMG_UINTPTR_T)psQueue->pvLinQueueKM + psQueue->ui32ReadOffset);

			if (PVRSRVProcessCommand(psSysData, psCommand, bFlush) == PVRSRV_OK)
			{
				
				UPDATE_QUEUE_ROFF(psQueue, psCommand->uCmdSize)
				continue;
			}

			break;
		}
		psQueue = psQueue->psNextKM;
	}

	if (bFlush)
	{
		PVRSRVSetDCState(DC_STATE_NO_FLUSH_COMMANDS);
	}

	
	List_PVRSRV_DEVICE_NODE_ForEach(psSysData->psDeviceNodeList,
									&PVRSRVProcessQueues_ForEachCb);

	OSUnlockResource(&psSysData->sQProcessResource, ISR_ID);

	return PVRSRV_OK;
}

#if defined(SUPPORT_CUSTOM_SWAP_OPERATIONS)
IMG_INTERNAL
IMG_VOID PVRSRVFreeCommandCompletePacketKM(IMG_HANDLE	hCmdCookie,
										   IMG_BOOL		bScheduleMISR)
{
	COMMAND_COMPLETE_DATA	*psCmdCompleteData = (COMMAND_COMPLETE_DATA *)hCmdCookie;
	SYS_DATA				*psSysData;

	SysAcquireData(&psSysData);

	
	psCmdCompleteData->bInUse = IMG_FALSE;

	
	PVRSRVScheduleDeviceCallbacks();

	if(bScheduleMISR)
	{
		OSScheduleMISR(psSysData);
	}
}

#endif 


IMG_EXPORT
IMG_VOID PVRSRVCommandCompleteKM(IMG_HANDLE	hCmdCookie,
								 IMG_BOOL	bScheduleMISR)
{
	IMG_UINT32				i;
	COMMAND_COMPLETE_DATA	*psCmdCompleteData = (COMMAND_COMPLETE_DATA *)hCmdCookie;
	SYS_DATA				*psSysData;

	SysAcquireData(&psSysData);

	PVR_TTRACE(PVRSRV_TRACE_GROUP_QUEUE, PVRSRV_TRACE_CLASS_CMD_COMP_START,
			QUEUE_TOKEN_COMMAND_COMPLETE);

	
	for (i=0; i<psCmdCompleteData->ui32DstSyncCount; i++)
	{
		psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM->psSyncData->ui32WriteOpsComplete++;

		PVRSRVKernelSyncInfoDecRef(psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM, IMG_NULL);

		PVR_TTRACE_SYNC_OBJECT(PVRSRV_TRACE_GROUP_QUEUE, QUEUE_TOKEN_UPDATE_DST,
					  psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM,
					  PVRSRV_SYNCOP_COMPLETE);

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVCommandCompleteKM: Dst %u RO-VA:0x%x WO-VA:0x%x ROP:0x%x WOP:0x%x",
				i, psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM->sReadOps2CompleteDevVAddr.uiAddr,
				psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psDstSync[i].ui32ReadOps2Pending,
				psCmdCompleteData->psDstSync[i].ui32WriteOpsPending));
	}

	
	for (i=0; i<psCmdCompleteData->ui32SrcSyncCount; i++)
	{
		psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM->psSyncData->ui32ReadOps2Complete++;

		PVRSRVKernelSyncInfoDecRef(psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM, IMG_NULL);

		PVR_TTRACE_SYNC_OBJECT(PVRSRV_TRACE_GROUP_QUEUE, QUEUE_TOKEN_UPDATE_SRC,
					  psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM,
					  PVRSRV_SYNCOP_COMPLETE);

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVCommandCompleteKM: Src %u RO-VA:0x%x WO-VA:0x%x ROP:0x%x WOP:0x%x",
				i, psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM->sReadOps2CompleteDevVAddr.uiAddr,
				psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psSrcSync[i].ui32ReadOps2Pending,
				psCmdCompleteData->psSrcSync[i].ui32WriteOpsPending));
	}

	PVR_TTRACE(PVRSRV_TRACE_GROUP_QUEUE, PVRSRV_TRACE_CLASS_CMD_COMP_END,
			QUEUE_TOKEN_COMMAND_COMPLETE);

	if (psCmdCompleteData->pfnCommandComplete)
	{
		psCmdCompleteData->pfnCommandComplete(psCmdCompleteData->hCallbackData);
	}

	
	psCmdCompleteData->bInUse = IMG_FALSE;

	
	PVRSRVScheduleDeviceCallbacks();

	if(bScheduleMISR)
	{
		OSScheduleMISR(psSysData);
	}
}




IMG_EXPORT
PVRSRV_ERROR PVRSRVRegisterCmdProcListKM(IMG_UINT32		ui32DevIndex,
										 PFN_CMD_PROC	*ppfnCmdProcList,
										 IMG_UINT32		ui32MaxSyncsPerCmd[][2],
										 IMG_UINT32		ui32CmdCount)
{
	SYS_DATA				*psSysData;
	PVRSRV_ERROR			eError;
	IMG_UINT32				ui32CmdCounter, ui32CmdTypeCounter;
	IMG_SIZE_T				ui32AllocSize;
	DEVICE_COMMAND_DATA		*psDeviceCommandData;
	COMMAND_COMPLETE_DATA	*psCmdCompleteData;

	
	if(ui32DevIndex >= SYS_DEVICE_COUNT)
	{
		PVR_DPF((PVR_DBG_ERROR,
					"PVRSRVRegisterCmdProcListKM: invalid DeviceType 0x%x",
					ui32DevIndex));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	
	SysAcquireData(&psSysData);

	
	ui32AllocSize = ui32CmdCount * sizeof(*psDeviceCommandData);
	eError = OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
						ui32AllocSize,
						(IMG_VOID **)&psDeviceCommandData, IMG_NULL,
						"Array of Pointers for Command Store");
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVRegisterCmdProcListKM: Failed to alloc CC data"));
		goto ErrorExit;
	}

	psSysData->apsDeviceCommandData[ui32DevIndex] = psDeviceCommandData;

	for (ui32CmdTypeCounter = 0; ui32CmdTypeCounter < ui32CmdCount; ui32CmdTypeCounter++)
	{
		psDeviceCommandData[ui32CmdTypeCounter].pfnCmdProc = ppfnCmdProcList[ui32CmdTypeCounter];
		psDeviceCommandData[ui32CmdTypeCounter].ui32CCBOffset = 0;
		psDeviceCommandData[ui32CmdTypeCounter].ui32MaxDstSyncCount = ui32MaxSyncsPerCmd[ui32CmdTypeCounter][0];
		psDeviceCommandData[ui32CmdTypeCounter].ui32MaxSrcSyncCount = ui32MaxSyncsPerCmd[ui32CmdTypeCounter][1];
		for (ui32CmdCounter = 0; ui32CmdCounter < DC_NUM_COMMANDS_PER_TYPE; ui32CmdCounter++)
		{
			

			ui32AllocSize = sizeof(COMMAND_COMPLETE_DATA) 
						  + ((ui32MaxSyncsPerCmd[ui32CmdTypeCounter][0]
						  +	ui32MaxSyncsPerCmd[ui32CmdTypeCounter][1])
						  * sizeof(PVRSRV_SYNC_OBJECT));	 

			eError = OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
								ui32AllocSize,
								(IMG_VOID **)&psCmdCompleteData,
								IMG_NULL,
								"Command Complete Data");
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"PVRSRVRegisterCmdProcListKM: Failed to alloc cmd %d", ui32CmdTypeCounter));
				goto ErrorExit;
			}
			
			psDeviceCommandData[ui32CmdTypeCounter].apsCmdCompleteData[ui32CmdCounter] = psCmdCompleteData;
			
			
			OSMemSet(psCmdCompleteData, 0x00, ui32AllocSize);

			
			psCmdCompleteData->psDstSync = (PVRSRV_SYNC_OBJECT*)
											(((IMG_UINTPTR_T)psCmdCompleteData)
											+ sizeof(COMMAND_COMPLETE_DATA));
			psCmdCompleteData->psSrcSync = (PVRSRV_SYNC_OBJECT*)
											(((IMG_UINTPTR_T)psCmdCompleteData->psDstSync)
											+ (sizeof(PVRSRV_SYNC_OBJECT) * ui32MaxSyncsPerCmd[ui32CmdTypeCounter][0]));

			psCmdCompleteData->ui32AllocSize = (IMG_UINT32)ui32AllocSize;
		}
	}

	return PVRSRV_OK;

ErrorExit:

	
	if (PVRSRVRemoveCmdProcListKM(ui32DevIndex, ui32CmdCount) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"PVRSRVRegisterCmdProcListKM: Failed to clean up after error, device 0x%x",
				ui32DevIndex));
	}
	
	return eError;
}


IMG_EXPORT
PVRSRV_ERROR PVRSRVRemoveCmdProcListKM(IMG_UINT32 ui32DevIndex,
									   IMG_UINT32 ui32CmdCount)
{
	SYS_DATA				*psSysData;
	IMG_UINT32				ui32CmdTypeCounter, ui32CmdCounter;
	DEVICE_COMMAND_DATA		*psDeviceCommandData;
	COMMAND_COMPLETE_DATA	*psCmdCompleteData;
	IMG_SIZE_T				ui32AllocSize;

	
	if(ui32DevIndex >= SYS_DEVICE_COUNT)
	{
		PVR_DPF((PVR_DBG_ERROR,
				"PVRSRVRemoveCmdProcListKM: invalid DeviceType 0x%x",
				ui32DevIndex));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	
	SysAcquireData(&psSysData);

	psDeviceCommandData = psSysData->apsDeviceCommandData[ui32DevIndex];
	if(psDeviceCommandData != IMG_NULL)
	{
		for (ui32CmdTypeCounter = 0; ui32CmdTypeCounter < ui32CmdCount; ui32CmdTypeCounter++)
		{
			for (ui32CmdCounter = 0; ui32CmdCounter < DC_NUM_COMMANDS_PER_TYPE; ui32CmdCounter++)
			{
				psCmdCompleteData = psDeviceCommandData[ui32CmdTypeCounter].apsCmdCompleteData[ui32CmdCounter];
				
				
				if (psCmdCompleteData != IMG_NULL)
				{
					PVR_ASSERT(psCmdCompleteData->bInUse == IMG_FALSE);
					OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP, psCmdCompleteData->ui32AllocSize,
							  psCmdCompleteData, IMG_NULL);
					psDeviceCommandData[ui32CmdTypeCounter].apsCmdCompleteData[ui32CmdCounter] = IMG_NULL;
				}
			}
		}

		
		ui32AllocSize = ui32CmdCount * sizeof(*psDeviceCommandData);
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP, ui32AllocSize, psDeviceCommandData, IMG_NULL);
		psSysData->apsDeviceCommandData[ui32DevIndex] = IMG_NULL;
	}

	return PVRSRV_OK;
}

