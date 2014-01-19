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

#ifndef QUEUE_H
#define QUEUE_H


#if defined(__cplusplus)
extern "C" {
#endif

#define UPDATE_QUEUE_ROFF(psQueue, ui32Size)						\
	psQueue->ui32ReadOffset = (psQueue->ui32ReadOffset + ui32Size)	\
	& (psQueue->ui32QueueSize - 1);

 typedef struct _COMMAND_COMPLETE_DATA_
 {
	IMG_BOOL			bInUse;

	IMG_UINT32			ui32DstSyncCount;
	IMG_UINT32			ui32SrcSyncCount;
	PVRSRV_SYNC_OBJECT	*psDstSync;
	PVRSRV_SYNC_OBJECT	*psSrcSync;
	IMG_UINT32			ui32AllocSize;
 }COMMAND_COMPLETE_DATA, *PCOMMAND_COMPLETE_DATA;

#if !defined(USE_CODE)
IMG_VOID QueueDumpDebugInfo(IMG_VOID);

IMG_IMPORT
PVRSRV_ERROR PVRSRVProcessQueues (IMG_UINT32	ui32CallerID,
								  IMG_BOOL		bFlush);

#if defined(__linux__) && defined(__KERNEL__)
#include <linux/types.h>
#include <linux/seq_file.h>
off_t
QueuePrintQueues (IMG_CHAR * buffer, size_t size, off_t off);

#ifdef PVR_PROC_USE_SEQ_FILE
void* ProcSeqOff2ElementQueue(struct seq_file * sfile, loff_t off);
void ProcSeqShowQueue(struct seq_file *sfile,void* el);
#endif

#endif


IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVCreateCommandQueueKM(IMG_SIZE_T ui32QueueSize,
													 PVRSRV_QUEUE_INFO **ppsQueueInfo);
IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVDestroyCommandQueueKM(PVRSRV_QUEUE_INFO *psQueueInfo);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVInsertCommandKM(PVRSRV_QUEUE_INFO	*psQueue,
												PVRSRV_COMMAND		**ppsCommand,
												IMG_UINT32			ui32DevIndex,
												IMG_UINT16			CommandType,
												IMG_UINT32			ui32DstSyncCount,
												PVRSRV_KERNEL_SYNC_INFO	*apsDstSync[],
												IMG_UINT32			ui32SrcSyncCount,
												PVRSRV_KERNEL_SYNC_INFO	*apsSrcSync[],
												IMG_SIZE_T			ui32DataByteSize );

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVGetQueueSpaceKM(PVRSRV_QUEUE_INFO *psQueue,
												IMG_SIZE_T ui32ParamSize,
												IMG_VOID **ppvSpace);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV PVRSRVSubmitCommandKM(PVRSRV_QUEUE_INFO *psQueue,
												PVRSRV_COMMAND *psCommand);

IMG_IMPORT
IMG_VOID PVRSRVCommandCompleteKM(IMG_HANDLE hCmdCookie, IMG_BOOL bScheduleMISR);

IMG_VOID PVRSRVCommandCompleteCallbacks(IMG_VOID);

IMG_IMPORT
PVRSRV_ERROR PVRSRVRegisterCmdProcListKM(IMG_UINT32		ui32DevIndex,
										 PFN_CMD_PROC	*ppfnCmdProcList,
										 IMG_UINT32		ui32MaxSyncsPerCmd[][2],
										 IMG_UINT32		ui32CmdCount);
IMG_IMPORT
PVRSRV_ERROR PVRSRVRemoveCmdProcListKM(IMG_UINT32	ui32DevIndex,
									   IMG_UINT32	ui32CmdCount);

#endif


#if defined (__cplusplus)
}
#endif

#endif

