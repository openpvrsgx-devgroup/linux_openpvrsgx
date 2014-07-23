/*************************************************************************/ /*!
@Title          Kernel side command queue functions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#include "services_headers.h"

#include "lists.h"

DECLARE_LIST_FOR_EACH(PVRSRV_DEVICE_NODE);

#if defined(__linux__) && defined(__KERNEL__)

#include "proc.h"

static IMG_INT
QueuePrintCommands (PVRSRV_QUEUE_INFO * psQueue, IMG_CHAR * buffer, size_t size)
{
	off_t off = 0;
	IMG_INT cmds = 0;
	IMG_SIZE_T ui32ReadOffset  = psQueue->ui32ReadOffset;
	IMG_SIZE_T ui32WriteOffset = psQueue->ui32WriteOffset;
	PVRSRV_COMMAND * psCmd;

	while (ui32ReadOffset != ui32WriteOffset)
	{
		psCmd= (PVRSRV_COMMAND *)((IMG_UINTPTR_T)psQueue->pvLinQueueKM + ui32ReadOffset);

		off = printAppend(buffer, size, off, "%p %p  %5lu  %6lu  %3lu  %5lu   %2lu   %2lu    %3lu  \n",
							psQueue,
					 		psCmd,
					 		psCmd->ui32ProcessID,
							psCmd->CommandType,
							psCmd->ui32CmdSize,
							psCmd->ui32DevIndex,
							psCmd->ui32DstSyncCount,
							psCmd->ui32SrcSyncCount,
							psCmd->ui32DataSize);

		ui32ReadOffset += psCmd->ui32CmdSize;
		ui32ReadOffset &= psQueue->ui32QueueSize - 1;
		cmds++;
	}
	if (cmds == 0)
		off = printAppend(buffer, size, off, "%p <empty>\n", psQueue);
	return off;
}



#ifdef PVR_PROC_USE_SEQ_FILE

void ProcSeqShowQueue(struct seq_file *sfile,void* el)
{
	PVRSRV_QUEUE_INFO * psQueue = (PVRSRV_QUEUE_INFO*)el;
	IMG_INT cmds = 0;
	IMG_SIZE_T ui32ReadOffset;
	IMG_SIZE_T ui32WriteOffset;
	PVRSRV_COMMAND * psCmd;

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

		seq_printf(sfile, "%p %p  %5lu  %6lu  %3lu  %5lu   %2lu   %2lu    %3lu  \n",
							psQueue,
					 		psCmd,
					 		psCmd->ui32ProcessID,
							psCmd->CommandType,
							psCmd->ui32CmdSize,
							psCmd->ui32DevIndex,
							psCmd->ui32DstSyncCount,
							psCmd->ui32SrcSyncCount,
							psCmd->ui32DataSize);

		ui32ReadOffset += psCmd->ui32CmdSize;
		ui32ReadOffset &= psQueue->ui32QueueSize - 1;
		cmds++;
	}

	if (cmds == 0)
		seq_printf(sfile, "%p <empty>\n", psQueue);
}

/*****************************************************************************
 FUNCTION	:	ProcSeqOff2ElementQueue

 PURPOSE	:	Transale offset to element (/proc stuff)

 PARAMETERS	:	sfile - /proc seq_file
				off - the offset into the buffer

 RETURNS    :   element to print
*****************************************************************************/
void* ProcSeqOff2ElementQueue(struct seq_file * sfile, loff_t off)
{
	PVRSRV_QUEUE_INFO * psQueue;
	SYS_DATA * psSysData;

	if(!off)
	{
		return PVR_PROC_SEQ_START_TOKEN;
	}


	SysAcquireData(&psSysData);

	for (psQueue = psSysData->psQueueList; (((--off) > 0) && (psQueue != IMG_NULL)); psQueue = psQueue->psNextKM);
	return psQueue;
}
#endif /* __linux__ && __KERNEL__ */

off_t
QueuePrintQueues (IMG_CHAR * buffer, size_t size, off_t off)
{
	SYS_DATA * psSysData;
	PVRSRV_QUEUE_INFO * psQueue;

	SysAcquireData(&psSysData);

	 if (!off)
		  return printAppend (buffer, size, 0,
								"Command Queues\n"
								"Queue    CmdPtr      Pid Command Size DevInd  DSC  SSC  #Data ...\n");



	for (psQueue = psSysData->psQueueList; (((--off) > 0) && (psQueue != IMG_NULL)); psQueue = psQueue->psNextKM)
		;

	return psQueue ? QueuePrintCommands (psQueue, buffer, size) : END_OF_FILE;
}
#endif

#define GET_SPACE_IN_CMDQ(psQueue)										\
	(((psQueue->ui32ReadOffset - psQueue->ui32WriteOffset)				\
	+ (psQueue->ui32QueueSize - 1)) & (psQueue->ui32QueueSize - 1))

#define UPDATE_QUEUE_WOFF(psQueue, ui32Size)							\
	psQueue->ui32WriteOffset = (psQueue->ui32WriteOffset + ui32Size)	\
	& (psQueue->ui32QueueSize - 1);

#define SYNCOPS_STALE(ui32OpsComplete, ui32OpsPending)					\
	(ui32OpsComplete >= ui32OpsPending)


DECLARE_LIST_FOR_EACH(PVRSRV_DEVICE_NODE);

static IMG_VOID QueueDumpCmdComplete(COMMAND_COMPLETE_DATA *psCmdCompleteData,
									 IMG_UINT32				i,
									 IMG_BOOL				bIsSrc)
{
	PVRSRV_SYNC_OBJECT	*psSyncObject;

	psSyncObject = bIsSrc ? psCmdCompleteData->psSrcSync : psCmdCompleteData->psDstSync;

	if (psCmdCompleteData->bInUse)
	{
		PVR_LOG(("\t%s %lu: ROC DevVAddr:0x%lX ROP:0x%lx ROC:0x%lx, WOC DevVAddr:0x%lX WOP:0x%lx WOC:0x%lx",
				bIsSrc ? "SRC" : "DEST", i,
				psSyncObject[i].psKernelSyncInfoKM->sReadOpsCompleteDevVAddr.uiAddr,
				psSyncObject[i].psKernelSyncInfoKM->psSyncData->ui32ReadOpsPending,
				psSyncObject[i].psKernelSyncInfoKM->psSyncData->ui32ReadOpsComplete,
				psSyncObject[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psSyncObject[i].psKernelSyncInfoKM->psSyncData->ui32WriteOpsPending,
				psSyncObject[i].psKernelSyncInfoKM->psSyncData->ui32WriteOpsComplete));
	}
	else
	{
		PVR_LOG(("\t%s %lu: (Not in use)", bIsSrc ? "SRC" : "DEST", i));
	}
}


static IMG_VOID QueueDumpDebugInfo_ForEachCb(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	if (psDeviceNode->sDevId.eDeviceClass == PVRSRV_DEVICE_CLASS_DISPLAY)
	{
		IMG_UINT32				i;
		SYS_DATA				*psSysData;
		COMMAND_COMPLETE_DATA	**ppsCmdCompleteData;
		COMMAND_COMPLETE_DATA	*psCmdCompleteData;

		SysAcquireData(&psSysData);

		ppsCmdCompleteData = psSysData->ppsCmdCompleteData[psDeviceNode->sDevId.ui32DeviceIndex];

		if (ppsCmdCompleteData != IMG_NULL)
		{
			psCmdCompleteData = ppsCmdCompleteData[DC_FLIP_COMMAND];

			PVR_LOG(("Command Complete Data for display device %lu:", psDeviceNode->sDevId.ui32DeviceIndex));

			for (i = 0; i < psCmdCompleteData->ui32SrcSyncCount; i++)
			{
				QueueDumpCmdComplete(psCmdCompleteData, i, IMG_TRUE);
			}

			for (i = 0; i < psCmdCompleteData->ui32DstSyncCount; i++)
			{
				QueueDumpCmdComplete(psCmdCompleteData, i, IMG_FALSE);
			}
		}
		else
		{
			PVR_LOG(("There is no Command Complete Data for display device %u", psDeviceNode->sDevId.ui32DeviceIndex));
		}
	}
}


IMG_VOID QueueDumpDebugInfo(IMG_VOID)
{
	SYS_DATA	*psSysData;
	SysAcquireData(&psSysData);
	List_PVRSRV_DEVICE_NODE_ForEach(psSysData->psDeviceNodeList, QueueDumpDebugInfo_ForEachCb);
}


IMG_SIZE_T NearestPower2(IMG_SIZE_T ui32Value)
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


/*!
******************************************************************************

 @Function	PVRSRVCreateCommandQueueKM

 @Description
 Creates a new command queue into which render/blt commands etc can be
 inserted.

 @Input    ui32QueueSize :

 @Output   ppsQueueInfo :

 @Return   PVRSRV_ERROR  :

******************************************************************************/
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


	if(OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					 sizeof(PVRSRV_QUEUE_INFO),
					 (IMG_VOID **)&psQueueInfo, &hMemBlock,
					 "Queue Info") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVCreateCommandQueueKM: Failed to alloc queue struct"));
		goto ErrorExit;
	}
	OSMemSet(psQueueInfo, 0, sizeof(PVRSRV_QUEUE_INFO));

	psQueueInfo->hMemBlock[0] = hMemBlock;
	psQueueInfo->ui32ProcessID = OSGetCurrentProcessIDKM();


	if(OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					 ui32Power2QueueSize + PVRSRV_MAX_CMD_SIZE,
					 &psQueueInfo->pvLinQueueKM, &hMemBlock,
					 "Command Queue") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVCreateCommandQueueKM: Failed to alloc queue buffer"));
		goto ErrorExit;
	}

	psQueueInfo->hMemBlock[1] = hMemBlock;
	psQueueInfo->pvLinQueueUM = psQueueInfo->pvLinQueueKM;

	/* Sanity check: Should be zeroed by OSMemSet */
	PVR_ASSERT(psQueueInfo->ui32ReadOffset == 0);
	PVR_ASSERT(psQueueInfo->ui32WriteOffset == 0);

	psQueueInfo->ui32QueueSize = ui32Power2QueueSize;

	/* if this is the first q, create a lock resource for the q list */
	if (psSysData->psQueueList == IMG_NULL)
	{
		eError = OSCreateResource(&psSysData->sQProcessResource);
		if (eError != PVRSRV_OK)
		{
			goto ErrorExit;
		}
	}


	if (OSLockResource(&psSysData->sQProcessResource,
							KERNEL_ID) != PVRSRV_OK)
	{
		goto ErrorExit;
	}

	psQueueInfo->psNextKM = psSysData->psQueueList;
	psSysData->psQueueList = psQueueInfo;

	if (OSUnlockResource(&psSysData->sQProcessResource, KERNEL_ID) != PVRSRV_OK)
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
		/*not nulling pointer, out of scope*/
	}

	return PVRSRV_ERROR_GENERIC;
}


/*!
******************************************************************************

 @Function	PVRSRVDestroyCommandQueueKM

 @Description	Destroys a command queue

 @Input		psQueueInfo :

 @Return	PVRSRV_ERROR

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVDestroyCommandQueueKM(PVRSRV_QUEUE_INFO *psQueueInfo)
{
	PVRSRV_QUEUE_INFO	*psQueue;
	SYS_DATA			*psSysData;
	PVRSRV_ERROR		eError;
	IMG_BOOL			bTimeout = IMG_TRUE;

	SysAcquireData(&psSysData);

	psQueue = psSysData->psQueueList;

	/* PRQA S 3415,4109 1 */ /* macro format critical - leave alone */
	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US)
	{
		if(psQueueInfo->ui32ReadOffset == psQueueInfo->ui32WriteOffset)
		{
			bTimeout = IMG_FALSE;
			break;
		}
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT();

	if (bTimeout)
	{
		/* The command queue could not be flushed within the timeout period.
		Allow the queue to be destroyed before returning the error code. */
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVDestroyCommandQueueKM : Failed to empty queue"));
		eError = PVRSRV_ERROR_CANNOT_FLUSH_QUEUE;
		goto ErrorExit;
	}

	/* Ensure we don't corrupt queue list, by blocking access */
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
		/* PRQA S 3199 1 */ /* see note */
		psQueueInfo = IMG_NULL; /*it's a copy on stack, but null it because the function doesn't end right here*/
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
				/* PRQA S 3199 1 */ /* see note */
				psQueueInfo = IMG_NULL; /*it's a copy on stack, but null it because the function doesn't end right here*/
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

	/*  unlock the Q list lock resource */
	eError = OSUnlockResource(&psSysData->sQProcessResource, KERNEL_ID);
	if (eError != PVRSRV_OK)
	{
		goto ErrorExit;
	}

	/*  if the Q list is now empty, destroy the Q list lock resource */
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


/*!
*****************************************************************************

 @Function	: PVRSRVGetQueueSpaceKM

 @Description	: Waits for queue access rights and checks for available space in
			  queue for task param structure

 @Input	: psQueue	   	- pointer to queue information struct
 @Input : ui32ParamSize - size of task data structure
 @Output : ppvSpace

 @Return	: PVRSRV_ERROR
*****************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetQueueSpaceKM(PVRSRV_QUEUE_INFO *psQueue,
												IMG_SIZE_T ui32ParamSize,
												IMG_VOID **ppvSpace)
{
	IMG_BOOL bTimeout = IMG_TRUE;

	/*	round to 4byte units */
	ui32ParamSize =  (ui32ParamSize+3) & 0xFFFFFFFC;

	if (ui32ParamSize > PVRSRV_MAX_CMD_SIZE)
	{
		PVR_DPF((PVR_DBG_WARNING,"PVRSRVGetQueueSpace: max command size is %d bytes", PVRSRV_MAX_CMD_SIZE));
		return PVRSRV_ERROR_CMD_TOO_BIG;
	}

	/* PRQA S 3415,4109 1 */ /* macro format critical - leave alone */
	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US)
	{
		if (GET_SPACE_IN_CMDQ(psQueue) > ui32ParamSize)
		{
			bTimeout = IMG_FALSE;
			break;
		}
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
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


/*!
*****************************************************************************
 @Function	PVRSRVInsertCommandKM

 @Description :
			command insertion utility
			 - waits for space in the queue for a new command
			 - fills in generic command information
			 - returns a pointer to the caller who's expected to then fill
			 	in the private data.
			The caller should follow PVRSRVInsertCommand with PVRSRVSubmitCommand
			which will update the queue's write offset so the command can be
			executed.

 @Input		psQueue : pointer to queue information struct

 @Output	ppvCmdData : holds pointer to space in queue for private cmd data

 @Return	PVRSRV_ERROR
*****************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVInsertCommandKM(PVRSRV_QUEUE_INFO	*psQueue,
												PVRSRV_COMMAND		**ppsCommand,
												IMG_UINT32			ui32DevIndex,
												IMG_UINT16			CommandType,
												IMG_UINT32			ui32DstSyncCount,
												PVRSRV_KERNEL_SYNC_INFO	*apsDstSync[],
												IMG_UINT32			ui32SrcSyncCount,
												PVRSRV_KERNEL_SYNC_INFO	*apsSrcSync[],
												IMG_SIZE_T			ui32DataByteSize )
{
	PVRSRV_ERROR 	eError;
	PVRSRV_COMMAND	*psCommand;
	IMG_SIZE_T		ui32CommandSize;
	IMG_UINT32		i;


	ui32DataByteSize = (ui32DataByteSize + 3UL) & ~3UL;

	/*  calc. command size */
	ui32CommandSize = sizeof(PVRSRV_COMMAND)
					+ ((ui32DstSyncCount + ui32SrcSyncCount) * sizeof(PVRSRV_SYNC_OBJECT))
					+ ui32DataByteSize;

	/* wait for space in queue */
	eError = PVRSRVGetQueueSpaceKM (psQueue, ui32CommandSize, (IMG_VOID**)&psCommand);
	if(eError != PVRSRV_OK)
	{
		return eError;
	}

	psCommand->ui32ProcessID	= OSGetCurrentProcessIDKM();

	/* setup the command */
	psCommand->ui32CmdSize		= ui32CommandSize; /* this may change if cmd shrinks */
	psCommand->ui32DevIndex 	= ui32DevIndex;
	psCommand->CommandType 		= CommandType;
	psCommand->ui32DstSyncCount	= ui32DstSyncCount;
	psCommand->ui32SrcSyncCount	= ui32SrcSyncCount;
	/* override QAC warning about stricter pointers */
	/* PRQA S 3305 END_PTR_ASSIGNMENTS */
	psCommand->psDstSync		= (PVRSRV_SYNC_OBJECT*)(((IMG_UINTPTR_T)psCommand) + sizeof(PVRSRV_COMMAND));


	psCommand->psSrcSync		= (PVRSRV_SYNC_OBJECT*)(((IMG_UINTPTR_T)psCommand->psDstSync)
								+ (ui32DstSyncCount * sizeof(PVRSRV_SYNC_OBJECT)));

	psCommand->pvData			= (PVRSRV_SYNC_OBJECT*)(((IMG_UINTPTR_T)psCommand->psSrcSync)
								+ (ui32SrcSyncCount * sizeof(PVRSRV_SYNC_OBJECT)));
	psCommand->ui32DataSize		= ui32DataByteSize;


	for (i=0; i<ui32DstSyncCount; i++)
	{
		psCommand->psDstSync[i].psKernelSyncInfoKM = apsDstSync[i];
		psCommand->psDstSync[i].ui32WriteOpsPending = PVRSRVGetWriteOpsPending(apsDstSync[i], IMG_FALSE);
		psCommand->psDstSync[i].ui32ReadOpsPending = PVRSRVGetReadOpsPending(apsDstSync[i], IMG_FALSE);

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVInsertCommandKM: Dst %lu RO-VA:0x%lx WO-VA:0x%lx ROP:0x%lx WOP:0x%lx",
				i, psCommand->psDstSync[i].psKernelSyncInfoKM->sReadOpsCompleteDevVAddr.uiAddr,
				psCommand->psDstSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCommand->psDstSync[i].ui32ReadOpsPending,
				psCommand->psDstSync[i].ui32WriteOpsPending));
	}


	for (i=0; i<ui32SrcSyncCount; i++)
	{
		psCommand->psSrcSync[i].psKernelSyncInfoKM = apsSrcSync[i];
		psCommand->psSrcSync[i].ui32WriteOpsPending = PVRSRVGetWriteOpsPending(apsSrcSync[i], IMG_TRUE);
		psCommand->psSrcSync[i].ui32ReadOpsPending = PVRSRVGetReadOpsPending(apsSrcSync[i], IMG_TRUE);

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVInsertCommandKM: Src %lu RO-VA:0x%lx WO-VA:0x%lx ROP:0x%lx WOP:0x%lx",
				i, psCommand->psSrcSync[i].psKernelSyncInfoKM->sReadOpsCompleteDevVAddr.uiAddr,
				psCommand->psSrcSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCommand->psSrcSync[i].ui32ReadOpsPending,
				psCommand->psSrcSync[i].ui32WriteOpsPending));
	}


	*ppsCommand = psCommand;

	return PVRSRV_OK;
}


/*!
*******************************************************************************
 @Function	: PVRSRVSubmitCommandKM

 @Description :
 			 updates the queue's write offset so the command can be executed.

 @Input	: psQueue 		-	queue command is in
 @Input	: psCommand

 @Return : PVRSRV_ERROR
******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSubmitCommandKM(PVRSRV_QUEUE_INFO *psQueue,
												PVRSRV_COMMAND *psCommand)
{
	/* override QAC warnings about stricter pointers */
	/* PRQA S 3305 END_PTR_ASSIGNMENTS2 */
	/* patch pointers in the command to be kernel pointers */
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

/* PRQA L:END_PTR_ASSIGNMENTS2 */

	UPDATE_QUEUE_WOFF(psQueue, psCommand->ui32CmdSize);

	return PVRSRV_OK;
}



IMG_EXPORT
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

	/* satisfy sync dependencies on the DST(s) */
	psWalkerObj = psCommand->psDstSync;
	psEndObj = psWalkerObj + psCommand->ui32DstSyncCount;
	while (psWalkerObj < psEndObj)
	{
		PVRSRV_SYNC_DATA *psSyncData = psWalkerObj->psKernelSyncInfoKM->psSyncData;

		ui32WriteOpsComplete = psSyncData->ui32WriteOpsComplete;
		ui32ReadOpsComplete = psSyncData->ui32ReadOpsComplete;

		if ((ui32WriteOpsComplete != psWalkerObj->ui32WriteOpsPending)
		||	(ui32ReadOpsComplete != psWalkerObj->ui32ReadOpsPending))
		{
			if (!bFlush ||
				!SYNCOPS_STALE(ui32WriteOpsComplete, psWalkerObj->ui32WriteOpsPending) ||
				!SYNCOPS_STALE(ui32ReadOpsComplete, psWalkerObj->ui32ReadOpsPending))
			{
				return PVRSRV_ERROR_FAILED_DEPENDENCIES;
			}
		}

		psWalkerObj++;
	}

	/* satisfy sync dependencies on the SRC(s) */
	psWalkerObj = psCommand->psSrcSync;
	psEndObj = psWalkerObj + psCommand->ui32SrcSyncCount;
	while (psWalkerObj < psEndObj)
	{
		PVRSRV_SYNC_DATA *psSyncData = psWalkerObj->psKernelSyncInfoKM->psSyncData;

		ui32ReadOpsComplete = psSyncData->ui32ReadOpsComplete;
		ui32WriteOpsComplete = psSyncData->ui32WriteOpsComplete;

		if ((ui32WriteOpsComplete != psWalkerObj->ui32WriteOpsPending)
		|| (ui32ReadOpsComplete != psWalkerObj->ui32ReadOpsPending))
		{
			if (!bFlush &&
				SYNCOPS_STALE(ui32WriteOpsComplete, psWalkerObj->ui32WriteOpsPending) &&
				SYNCOPS_STALE(ui32ReadOpsComplete, psWalkerObj->ui32ReadOpsPending))
			{
				PVR_DPF((PVR_DBG_WARNING,
						"PVRSRVProcessCommand: Stale syncops psSyncData:0x%x ui32WriteOpsComplete:0x%x ui32WriteOpsPending:0x%x",
						psSyncData, ui32WriteOpsComplete, psWalkerObj->ui32WriteOpsPending));
			}

			if (!bFlush ||
				!SYNCOPS_STALE(ui32WriteOpsComplete, psWalkerObj->ui32WriteOpsPending) ||
				!SYNCOPS_STALE(ui32ReadOpsComplete, psWalkerObj->ui32ReadOpsPending))
			{
				return PVRSRV_ERROR_FAILED_DEPENDENCIES;
			}
		}
		psWalkerObj++;
	}

	/* validate device type */
	if (psCommand->ui32DevIndex >= SYS_DEVICE_COUNT)
	{
		PVR_DPF((PVR_DBG_ERROR,
					"PVRSRVProcessCommand: invalid DeviceType 0x%x",
					psCommand->ui32DevIndex));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}


	psCmdCompleteData = psSysData->ppsCmdCompleteData[psCommand->ui32DevIndex][psCommand->CommandType];
	if (psCmdCompleteData->bInUse)
	{
		/* can use this to protect against concurrent execution of same command */
		return PVRSRV_ERROR_FAILED_DEPENDENCIES;
	}

	/* mark the structure as in use */
	psCmdCompleteData->bInUse = IMG_TRUE;

	/* copy src updates over */
	psCmdCompleteData->ui32DstSyncCount = psCommand->ui32DstSyncCount;
	for (i=0; i<psCommand->ui32DstSyncCount; i++)
	{
		psCmdCompleteData->psDstSync[i] = psCommand->psDstSync[i];

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVProcessCommand: Dst %lu RO-VA:0x%lx WO-VA:0x%lx ROP:0x%lx WOP:0x%lx",
				i, psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM->sReadOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psDstSync[i].ui32ReadOpsPending,
				psCmdCompleteData->psDstSync[i].ui32WriteOpsPending));
	}


	psCmdCompleteData->ui32SrcSyncCount = psCommand->ui32SrcSyncCount;
	for (i=0; i<psCommand->ui32SrcSyncCount; i++)
	{
		psCmdCompleteData->psSrcSync[i] = psCommand->psSrcSync[i];

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVProcessCommand: Src %lu RO-VA:0x%lx WO-VA:0x%lx ROP:0x%lx WOP:0x%lx",
				i, psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM->sReadOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psSrcSync[i].ui32ReadOpsPending,
				psCmdCompleteData->psSrcSync[i].ui32WriteOpsPending));
	}

	/*
		call the cmd specific handler:
		it should:
		 - check the cmd specific dependencies
		 - setup private cmd complete structure
		 - execute cmd on HW
		 - store psCmdCompleteData `cookie' and later pass as
			argument to Generic Command Complete Callback

		n.b. ui32DataSize (packet size) is useful for packet validation
	*/










	if (psSysData->ppfnCmdProcList[psCommand->ui32DevIndex][psCommand->CommandType]((IMG_HANDLE)psCmdCompleteData,
																				psCommand->ui32DataSize,
																				psCommand->pvData) == IMG_FALSE)
	{
		/*
			clean-up:
			free cmd complete structure
		*/
		psCmdCompleteData->bInUse = IMG_FALSE;
		eError = PVRSRV_ERROR_CMD_NOT_PROCESSED;
	}

	return eError;
}


IMG_VOID PVRSRVProcessQueues_ForEachCb(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	if (psDeviceNode->bReProcessDeviceCommandComplete &&
		psDeviceNode->pfnDeviceCommandComplete != IMG_NULL)
	{
		(*psDeviceNode->pfnDeviceCommandComplete)(psDeviceNode);
	}
}

/*!
******************************************************************************

 @Function	PVRSRVProcessQueues

 @Description	Tries to process a command from each Q

 @input ui32CallerID - used to distinguish between async ISR/DPC type calls
 						the synchronous services driver
 @input	bFlush - flush commands with stale dependencies (only used for HW recovery)

 @Return	PVRSRV_ERROR

******************************************************************************/

IMG_EXPORT
PVRSRV_ERROR PVRSRVProcessQueues(IMG_UINT32	ui32CallerID,
								 IMG_BOOL	bFlush)
{
	PVRSRV_QUEUE_INFO 	*psQueue;
	SYS_DATA			*psSysData;
	PVRSRV_COMMAND 		*psCommand;
	PVRSRV_ERROR		eError;

	SysAcquireData(&psSysData);


	psSysData->bReProcessQueues = IMG_FALSE;


	eError = OSLockResource(&psSysData->sQProcessResource,
							ui32CallerID);
	if(eError != PVRSRV_OK)
	{

		psSysData->bReProcessQueues = IMG_TRUE;


		if(ui32CallerID == ISR_ID)
		{
			if (bFlush)
			{
				PVR_DPF((PVR_DBG_ERROR,"PVRSRVProcessQueues: Couldn't acquire queue processing lock for FLUSH"));
			}
			else
			{
				PVR_DPF((PVR_DBG_MESSAGE,"PVRSRVProcessQueues: Couldn't acquire queue processing lock"));
			}
		}
		else
		{
			PVR_DPF((PVR_DBG_MESSAGE,"PVRSRVProcessQueues: Queue processing lock-acquire failed when called from the Services driver."));
			PVR_DPF((PVR_DBG_MESSAGE,"                     This is due to MISR queue processing being interrupted by the Services driver."));
		}

		return PVRSRV_OK;
	}

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

				UPDATE_QUEUE_ROFF(psQueue, psCommand->ui32CmdSize)

				if (bFlush)
				{
					continue;
				}
			}

			break;
		}
		psQueue = psQueue->psNextKM;
	}

	if (bFlush)
	{
		PVRSRVSetDCState(DC_STATE_NO_FLUSH_COMMANDS);
	}

	/* Re-process command complete handlers if necessary. */
	List_PVRSRV_DEVICE_NODE_ForEach(psSysData->psDeviceNodeList,
									PVRSRVProcessQueues_ForEachCb);



	OSUnlockResource(&psSysData->sQProcessResource, ui32CallerID);


	if(psSysData->bReProcessQueues)
	{
		return PVRSRV_ERROR_PROCESSING_BLOCKED;
	}

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


	PVRSRVCommandCompleteCallbacks();

#if defined(SYS_USING_INTERRUPTS)
	if(bScheduleMISR)
	{
		OSScheduleMISR(psSysData);
	}
#else
	PVR_UNREFERENCED_PARAMETER(bScheduleMISR);
#endif
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


	for (i=0; i<psCmdCompleteData->ui32DstSyncCount; i++)
	{
		psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM->psSyncData->ui32WriteOpsComplete++;

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVCommandCompleteKM: Dst %lu RO-VA:0x%lx WO-VA:0x%lx ROP:0x%lx WOP:0x%lx",
				i, psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM->sReadOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psDstSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psDstSync[i].ui32ReadOpsPending,
				psCmdCompleteData->psDstSync[i].ui32WriteOpsPending));
	}


	for (i=0; i<psCmdCompleteData->ui32SrcSyncCount; i++)
	{
		psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM->psSyncData->ui32ReadOpsComplete++;

		PVR_DPF((PVR_DBG_MESSAGE, "PVRSRVCommandCompleteKM: Src %lu RO-VA:0x%lx WO-VA:0x%lx ROP:0x%lx WOP:0x%lx",
				i, psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM->sReadOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psSrcSync[i].psKernelSyncInfoKM->sWriteOpsCompleteDevVAddr.uiAddr,
				psCmdCompleteData->psSrcSync[i].ui32ReadOpsPending,
				psCmdCompleteData->psSrcSync[i].ui32WriteOpsPending));
	}


	psCmdCompleteData->bInUse = IMG_FALSE;


	PVRSRVCommandCompleteCallbacks();

#if defined(SYS_USING_INTERRUPTS)
	if(bScheduleMISR)
	{
		OSScheduleMISR(psSysData);
	}
#else
	PVR_UNREFERENCED_PARAMETER(bScheduleMISR);
#endif
}


IMG_VOID PVRSRVCommandCompleteCallbacks_ForEachCb(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	if(psDeviceNode->pfnDeviceCommandComplete != IMG_NULL)
	{

		(*psDeviceNode->pfnDeviceCommandComplete)(psDeviceNode);
	}
}

IMG_VOID PVRSRVCommandCompleteCallbacks(IMG_VOID)
{
	SYS_DATA				*psSysData;
	SysAcquireData(&psSysData);


	List_PVRSRV_DEVICE_NODE_ForEach(psSysData->psDeviceNodeList,
									PVRSRVCommandCompleteCallbacks_ForEachCb);
}

IMG_EXPORT
PVRSRV_ERROR PVRSRVRegisterCmdProcListKM(IMG_UINT32		ui32DevIndex,
										 PFN_CMD_PROC	*ppfnCmdProcList,
										 IMG_UINT32		ui32MaxSyncsPerCmd[][2],
										 IMG_UINT32		ui32CmdCount)
{
	SYS_DATA				*psSysData;
	PVRSRV_ERROR			eError;
	IMG_UINT32				i;
	IMG_SIZE_T				ui32AllocSize;
	PFN_CMD_PROC			*ppfnCmdProc;
	COMMAND_COMPLETE_DATA	*psCmdCompleteData;

	/* validate device type */
	if(ui32DevIndex >= SYS_DEVICE_COUNT)
	{
		PVR_DPF((PVR_DBG_ERROR,
					"PVRSRVRegisterCmdProcListKM: invalid DeviceType 0x%x",
					ui32DevIndex));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* acquire system data structure */
	SysAcquireData(&psSysData);


	eError = OSAllocMem( PVRSRV_OS_PAGEABLE_HEAP,
					 ui32CmdCount * sizeof(PFN_CMD_PROC),
					 (IMG_VOID **)&psSysData->ppfnCmdProcList[ui32DevIndex], IMG_NULL,
					 "Internal Queue Info structure");
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVRegisterCmdProcListKM: Failed to alloc queue"));
		return eError;
	}


	ppfnCmdProc = psSysData->ppfnCmdProcList[ui32DevIndex];


	for (i=0; i<ui32CmdCount; i++)
	{
		ppfnCmdProc[i] = ppfnCmdProcList[i];
	}


	ui32AllocSize = ui32CmdCount * sizeof(COMMAND_COMPLETE_DATA*);
	eError = OSAllocMem( PVRSRV_OS_NON_PAGEABLE_HEAP,
					 ui32AllocSize,
					 (IMG_VOID **)&psSysData->ppsCmdCompleteData[ui32DevIndex], IMG_NULL,
					 "Array of Pointers for Command Store");
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVRegisterCmdProcListKM: Failed to alloc CC data"));
		goto ErrorExit;
	}

	for (i=0; i<ui32CmdCount; i++)
	{


		ui32AllocSize = sizeof(COMMAND_COMPLETE_DATA)
					  + ((ui32MaxSyncsPerCmd[i][0]
					  +	ui32MaxSyncsPerCmd[i][1])
					  * sizeof(PVRSRV_SYNC_OBJECT));

		eError = OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
							ui32AllocSize,
							(IMG_VOID **)&psSysData->ppsCmdCompleteData[ui32DevIndex][i],
							IMG_NULL,
							"Command Complete Data");
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"PVRSRVRegisterCmdProcListKM: Failed to alloc cmd %d",i));
			goto ErrorExit;
		}


		OSMemSet(psSysData->ppsCmdCompleteData[ui32DevIndex][i], 0x00, ui32AllocSize);

		psCmdCompleteData = psSysData->ppsCmdCompleteData[ui32DevIndex][i];


		psCmdCompleteData->psDstSync = (PVRSRV_SYNC_OBJECT*)
										(((IMG_UINTPTR_T)psCmdCompleteData)
										+ sizeof(COMMAND_COMPLETE_DATA));
		psCmdCompleteData->psSrcSync = (PVRSRV_SYNC_OBJECT*)
										(((IMG_UINTPTR_T)psCmdCompleteData->psDstSync)
										+ (sizeof(PVRSRV_SYNC_OBJECT) * ui32MaxSyncsPerCmd[i][0]));

		psCmdCompleteData->ui32AllocSize = ui32AllocSize;
	}

	return PVRSRV_OK;

ErrorExit:



	if(psSysData->ppsCmdCompleteData[ui32DevIndex] != IMG_NULL)
	{
		for (i=0; i<ui32CmdCount; i++)
		{
			if (psSysData->ppsCmdCompleteData[ui32DevIndex][i] != IMG_NULL)
			{
				ui32AllocSize = sizeof(COMMAND_COMPLETE_DATA)
							  + ((ui32MaxSyncsPerCmd[i][0]
							  +	ui32MaxSyncsPerCmd[i][1])
							  * sizeof(PVRSRV_SYNC_OBJECT));
				OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP, ui32AllocSize, psSysData->ppsCmdCompleteData[ui32DevIndex][i], IMG_NULL);
				psSysData->ppsCmdCompleteData[ui32DevIndex][i] = IMG_NULL;
			}
		}
		ui32AllocSize = ui32CmdCount * sizeof(COMMAND_COMPLETE_DATA*);
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP, ui32AllocSize, psSysData->ppsCmdCompleteData[ui32DevIndex], IMG_NULL);
		psSysData->ppsCmdCompleteData[ui32DevIndex] = IMG_NULL;
	}

	if(psSysData->ppfnCmdProcList[ui32DevIndex] != IMG_NULL)
	{
		ui32AllocSize = ui32CmdCount * sizeof(PFN_CMD_PROC);
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP, ui32AllocSize, psSysData->ppfnCmdProcList[ui32DevIndex], IMG_NULL);
		psSysData->ppfnCmdProcList[ui32DevIndex] = IMG_NULL;
	}

	return eError;
}


IMG_EXPORT
PVRSRV_ERROR PVRSRVRemoveCmdProcListKM(IMG_UINT32 ui32DevIndex,
									   IMG_UINT32 ui32CmdCount)
{
	SYS_DATA		*psSysData;
	IMG_UINT32		i;


	if(ui32DevIndex >= SYS_DEVICE_COUNT)
	{
		PVR_DPF((PVR_DBG_ERROR,
					"PVRSRVRemoveCmdProcListKM: invalid DeviceType 0x%x",
					ui32DevIndex));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	/* acquire system data structure */
	SysAcquireData(&psSysData);

	if(psSysData->ppsCmdCompleteData[ui32DevIndex] == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"PVRSRVRemoveCmdProcListKM: Invalid command array"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	else
	{
		for(i=0; i<ui32CmdCount; i++)
		{

			if(psSysData->ppsCmdCompleteData[ui32DevIndex][i] != IMG_NULL)
			{
				OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
						 psSysData->ppsCmdCompleteData[ui32DevIndex][i]->ui32AllocSize,
						 psSysData->ppsCmdCompleteData[ui32DevIndex][i],
						 IMG_NULL);
				psSysData->ppsCmdCompleteData[ui32DevIndex][i] = IMG_NULL;
			}
		}


		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				 ui32CmdCount * sizeof(COMMAND_COMPLETE_DATA*),
				 psSysData->ppsCmdCompleteData[ui32DevIndex],
				 IMG_NULL);
		psSysData->ppsCmdCompleteData[ui32DevIndex] = IMG_NULL;
	}


	if(psSysData->ppfnCmdProcList[ui32DevIndex] != IMG_NULL)
	{
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				 ui32CmdCount * sizeof(PFN_CMD_PROC),
				 psSysData->ppfnCmdProcList[ui32DevIndex],
				 IMG_NULL);
		psSysData->ppfnCmdProcList[ui32DevIndex] = IMG_NULL;
	}

	return PVRSRV_OK;
}

/******************************************************************************
 End of file (queue.c)
******************************************************************************/
