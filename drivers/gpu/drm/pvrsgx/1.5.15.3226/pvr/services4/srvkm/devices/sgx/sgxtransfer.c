/*************************************************************************/ /*!
@Title          Device specific transfer queue routines
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

#if defined(TRANSFER_QUEUE)

#include <stddef.h>

#include "sgxdefs.h"
#include "services_headers.h"
#include "buffer_manager.h"
#include "sgxinfo.h"
#include "sysconfig.h"
#include "regpaths.h"
#include "pdump_km.h"
#include "mmu.h"
#include "pvr_bridge.h"
#include "sgx_bridge_km.h"
#include "sgxinfokm.h"
#include "osfunc.h"
#include "pvr_debug.h"
#include "sgxutils.h"

int do_wait_vblank(void *display, int headline, int footline);

IMG_EXPORT PVRSRV_ERROR SGXSubmitTransferKM(IMG_HANDLE hDevHandle, PVRSRV_TRANSFER_SGX_KICK *psKick)
{
	PVRSRV_KERNEL_MEM_INFO  *psCCBMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psKick->hCCBMemInfo;
	SGXMKIF_COMMAND sCommand = {0};
	SGXMKIF_TRANSFERCMD_SHARED *psSharedTransferCmd;
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
	PVRSRV_ERROR eError;


	if (!CCB_OFFSET_IS_VALID(SGXMKIF_TRANSFERCMD_SHARED, psCCBMemInfo, psKick, ui32SharedCmdCCBOffset))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXSubmitTransferKM: Invalid CCB offset"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}


	psSharedTransferCmd =  CCB_DATA_FROM_OFFSET(SGXMKIF_TRANSFERCMD_SHARED, psCCBMemInfo, psKick, ui32SharedCmdCCBOffset);

	if (psKick->hTASyncInfo != IMG_NULL)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->hTASyncInfo;

		psSharedTransferCmd->ui32TASyncWriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending++;
		psSharedTransferCmd->ui32TASyncReadOpsPendingVal  = psSyncInfo->psSyncData->ui32ReadOpsPending;

		psSharedTransferCmd->sTASyncWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
		psSharedTransferCmd->sTASyncReadOpsCompleteDevVAddr = psSyncInfo->sReadOpsCompleteDevVAddr;
	}
	else
	{
		psSharedTransferCmd->sTASyncWriteOpsCompleteDevVAddr.uiAddr = 0;
		psSharedTransferCmd->sTASyncReadOpsCompleteDevVAddr.uiAddr = 0;
	}

	if (psKick->h3DSyncInfo != IMG_NULL)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->h3DSyncInfo;

		psSharedTransferCmd->ui323DSyncWriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending++;
		psSharedTransferCmd->ui323DSyncReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending;

		psSharedTransferCmd->s3DSyncWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
		psSharedTransferCmd->s3DSyncReadOpsCompleteDevVAddr = psSyncInfo->sReadOpsCompleteDevVAddr;
	}
	else
	{
		psSharedTransferCmd->s3DSyncWriteOpsCompleteDevVAddr.uiAddr = 0;
		psSharedTransferCmd->s3DSyncReadOpsCompleteDevVAddr.uiAddr = 0;
	}

	if ((psKick->ui32Flags & SGXMKIF_TQFLAGS_KEEPPENDING) == 0UL)
	{
		if (psKick->ui32NumSrcSync > 0)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahSrcSyncInfo[0];

			psSharedTransferCmd->ui32SrcWriteOpPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending;
			psSharedTransferCmd->ui32SrcReadOpPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending;

			psSharedTransferCmd->sSrcWriteOpsCompleteDevAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
			psSharedTransferCmd->sSrcReadOpsCompleteDevAddr = psSyncInfo->sReadOpsCompleteDevVAddr;

		}

		if (psKick->ui32NumDstSync > 0)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahDstSyncInfo[0];

			psSharedTransferCmd->ui32DstWriteOpPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending;
			psSharedTransferCmd->ui32DstReadOpPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending;

			psSharedTransferCmd->sDstWriteOpsCompleteDevAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
			psSharedTransferCmd->sDstReadOpsCompleteDevAddr = psSyncInfo->sReadOpsCompleteDevVAddr;

		}


		if (psKick->ui32NumSrcSync > 0)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahSrcSyncInfo[0];
			psSyncInfo->psSyncData->ui32ReadOpsPending++;
		}
		if (psKick->ui32NumDstSync > 0)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahDstSyncInfo[0];
			psSyncInfo->psSyncData->ui32WriteOpsPending++;
		}
	}


	if (psKick->ui32NumDstSync > 1 || psKick->ui32NumSrcSync  > 1)
	{
		PVR_DPF((PVR_DBG_ERROR,
					"Transfer command doesn't support more than 1 sync object per src/dst\ndst: %d, src: %d",
					psKick->ui32NumDstSync, psKick->ui32NumSrcSync));
	}

#if defined(PDUMP)
	if (PDumpIsCaptureFrameKM()
	|| ((psKick->ui32PDumpFlags & PDUMP_FLAGS_CONTINUOUS) != 0))
	{
		PDUMPCOMMENT("Shared part of transfer command\r\n");
		PDUMPMEM(psSharedTransferCmd,
				psCCBMemInfo,
				psKick->ui32CCBDumpWOff,
				sizeof(SGXMKIF_TRANSFERCMD_SHARED),
				psKick->ui32PDumpFlags,
				MAKEUNIQUETAG(psCCBMemInfo));

		if((psKick->ui32NumSrcSync > 0) && ((psKick->ui32Flags & SGXMKIF_TQFLAGS_KEEPPENDING) == 0UL))
		{
			psSyncInfo = psKick->ahSrcSyncInfo[0];

			PDUMPCOMMENT("Hack src surface write op in transfer cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
					psCCBMemInfo,
					psKick->ui32CCBDumpWOff + offsetof(SGXMKIF_TRANSFERCMD_SHARED, ui32SrcWriteOpPendingVal),
					sizeof(psSyncInfo->psSyncData->ui32LastOpDumpVal),
					psKick->ui32PDumpFlags,
					MAKEUNIQUETAG(psCCBMemInfo));

			PDUMPCOMMENT("Hack src surface read op in transfer cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
					psCCBMemInfo,
					psKick->ui32CCBDumpWOff + offsetof(SGXMKIF_TRANSFERCMD_SHARED, ui32SrcReadOpPendingVal),
					sizeof(psSyncInfo->psSyncData->ui32LastReadOpDumpVal),
					psKick->ui32PDumpFlags,
					MAKEUNIQUETAG(psCCBMemInfo));

		}
		if((psKick->ui32NumDstSync > 0) && ((psKick->ui32Flags & SGXMKIF_TQFLAGS_KEEPPENDING) == 0UL))
		{
			psSyncInfo = psKick->ahDstSyncInfo[0];

			PDUMPCOMMENT("Hack dest surface write op in transfer cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
					psCCBMemInfo,
					psKick->ui32CCBDumpWOff + offsetof(SGXMKIF_TRANSFERCMD_SHARED, ui32DstWriteOpPendingVal),
					sizeof(psSyncInfo->psSyncData->ui32LastOpDumpVal),
					psKick->ui32PDumpFlags,
					MAKEUNIQUETAG(psCCBMemInfo));

			PDUMPCOMMENT("Hack dest surface read op in transfer cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
					psCCBMemInfo,
					psKick->ui32CCBDumpWOff + offsetof(SGXMKIF_TRANSFERCMD_SHARED, ui32DstReadOpPendingVal),
					sizeof(psSyncInfo->psSyncData->ui32LastReadOpDumpVal),
					psKick->ui32PDumpFlags,
					MAKEUNIQUETAG(psCCBMemInfo));

		}


		if((psKick->ui32NumSrcSync > 0) && ((psKick->ui32Flags & SGXMKIF_TQFLAGS_KEEPPENDING)== 0UL))
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahSrcSyncInfo[0];
			psSyncInfo->psSyncData->ui32LastReadOpDumpVal++;
		}

		if((psKick->ui32NumDstSync > 0) && ((psKick->ui32Flags & SGXMKIF_TQFLAGS_KEEPPENDING) == 0UL))
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahDstSyncInfo[0];
			psSyncInfo->psSyncData->ui32LastOpDumpVal++;
		}
	}
#endif

	sCommand.ui32Data[1] = psKick->sHWTransferContextDevVAddr.uiAddr;

	if (psKick->display)
		do_wait_vblank(psKick->display, psKick->headline, psKick->footline);
	eError = SGXScheduleCCBCommandKM(hDevHandle, SGXMKIF_CMD_TRANSFER, &sCommand, KERNEL_ID, psKick->ui32PDumpFlags);

	if (eError == PVRSRV_ERROR_RETRY)
	{
		/* Client will retry, so undo the sync ops pending increment(s) done above. */
		if ((psKick->ui32Flags & SGXMKIF_TQFLAGS_KEEPPENDING) == 0UL)
		{
			if (psKick->ui32NumSrcSync > 0)
			{
				psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahSrcSyncInfo[0];
				psSyncInfo->psSyncData->ui32ReadOpsPending--;
			}
			if (psKick->ui32NumDstSync > 0)
			{
				psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahDstSyncInfo[0];
				psSyncInfo->psSyncData->ui32WriteOpsPending--;
			}
#if defined(PDUMP)
			if (PDumpIsCaptureFrameKM()
			|| ((psKick->ui32PDumpFlags & PDUMP_FLAGS_CONTINUOUS) != 0))
			{
				if (psKick->ui32NumSrcSync > 0)
				{
					psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahSrcSyncInfo[0];
					psSyncInfo->psSyncData->ui32LastReadOpDumpVal--;
				}
				if (psKick->ui32NumDstSync > 0)
				{
					psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahDstSyncInfo[0];
					psSyncInfo->psSyncData->ui32LastOpDumpVal--;
				}
			}
#endif
		}

		/* Command needed to be synchronised with the TA? */
		if (psKick->hTASyncInfo != IMG_NULL)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->hTASyncInfo;
			psSyncInfo->psSyncData->ui32WriteOpsPending--;
		}

		/* Command needed to be synchronised with the 3D? */
		if (psKick->h3DSyncInfo != IMG_NULL)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->h3DSyncInfo;
			psSyncInfo->psSyncData->ui32WriteOpsPending--;
		}
	}

	else if (PVRSRV_OK != eError)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXSubmitTransferKM: SGXScheduleCCBCommandKM failed."));
		return eError;
	}
	

#if defined(NO_HARDWARE)
	if ((psKick->ui32Flags & SGXMKIF_TQFLAGS_NOSYNCUPDATE) == 0)
	{
		IMG_UINT32 i;


		for(i = 0; i < psKick->ui32NumSrcSync; i++)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahSrcSyncInfo[i];
			psSyncInfo->psSyncData->ui32ReadOpsComplete = psSyncInfo->psSyncData->ui32ReadOpsPending;
		}

		for(i = 0; i < psKick->ui32NumDstSync; i++)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahDstSyncInfo[i];
			psSyncInfo->psSyncData->ui32WriteOpsComplete = psSyncInfo->psSyncData->ui32WriteOpsPending;

		}

		if (psKick->hTASyncInfo != IMG_NULL)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->hTASyncInfo;

			psSyncInfo->psSyncData->ui32WriteOpsComplete = psSyncInfo->psSyncData->ui32WriteOpsPending;
		}

		if (psKick->h3DSyncInfo != IMG_NULL)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->h3DSyncInfo;

			psSyncInfo->psSyncData->ui32WriteOpsComplete = psSyncInfo->psSyncData->ui32WriteOpsPending;
		}
	}
#endif

	return eError;
}

#if defined(SGX_FEATURE_2D_HARDWARE)
IMG_EXPORT PVRSRV_ERROR SGXSubmit2DKM(IMG_HANDLE hDevHandle, PVRSRV_2D_SGX_KICK *psKick)

{
	PVRSRV_KERNEL_MEM_INFO  *psCCBMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psKick->hCCBMemInfo;
	SGXMKIF_COMMAND sCommand = {0};
	SGXMKIF_2DCMD_SHARED *ps2DCmd;
	PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
	PVRSRV_ERROR eError;
	IMG_UINT32 i;

	if (!CCB_OFFSET_IS_VALID(SGXMKIF_2DCMD_SHARED, psCCBMemInfo, psKick, ui32SharedCmdCCBOffset))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXSubmit2DKM: Invalid CCB offset"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	/* override QAC warning about stricter alignment */
	/* PRQA S 3305 1 */
	ps2DCmd =  CCB_DATA_FROM_OFFSET(SGXMKIF_2DCMD_SHARED, psCCBMemInfo, psKick, ui32SharedCmdCCBOffset);

	OSMemSet(ps2DCmd, 0, sizeof(*ps2DCmd));

	/* Command needs to be synchronised with the TA? */
	if (psKick->hTASyncInfo != IMG_NULL)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->hTASyncInfo;

		ps2DCmd->sTASyncData.ui32WriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending++;
		ps2DCmd->sTASyncData.ui32ReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending;

		ps2DCmd->sTASyncData.sWriteOpsCompleteDevVAddr 	= psSyncInfo->sWriteOpsCompleteDevVAddr;
		ps2DCmd->sTASyncData.sReadOpsCompleteDevVAddr 	= psSyncInfo->sReadOpsCompleteDevVAddr;
	}

	/* Command needs to be synchronised with the 3D? */
	if (psKick->h3DSyncInfo != IMG_NULL)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->h3DSyncInfo;

		ps2DCmd->s3DSyncData.ui32WriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending++;
		ps2DCmd->s3DSyncData.ui32ReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending;

		ps2DCmd->s3DSyncData.sWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
		ps2DCmd->s3DSyncData.sReadOpsCompleteDevVAddr = psSyncInfo->sReadOpsCompleteDevVAddr;
	}

	/*
	 * We allow the first source and destination sync objects to be the
	 * same, which is why the read/write pending updates are delayed
	 * until the transfer command has been updated with the current
	 * values from the objects.
	 */
	ps2DCmd->ui32NumSrcSync = psKick->ui32NumSrcSync;
	for (i = 0; i < psKick->ui32NumSrcSync; i++)
	{
		psSyncInfo = psKick->ahSrcSyncInfo[i];

		ps2DCmd->sSrcSyncData[i].ui32WriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending;
		ps2DCmd->sSrcSyncData[i].ui32ReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending;

		ps2DCmd->sSrcSyncData[i].sWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
		ps2DCmd->sSrcSyncData[i].sReadOpsCompleteDevVAddr = psSyncInfo->sReadOpsCompleteDevVAddr;
	}

	if (psKick->hDstSyncInfo != IMG_NULL)
	{
		psSyncInfo = psKick->hDstSyncInfo;

		ps2DCmd->sDstSyncData.ui32WriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending;
		ps2DCmd->sDstSyncData.ui32ReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending;

		ps2DCmd->sDstSyncData.sWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
		ps2DCmd->sDstSyncData.sReadOpsCompleteDevVAddr = psSyncInfo->sReadOpsCompleteDevVAddr;
	}

	/* Read/Write ops pending updates, delayed from above */
	for (i = 0; i < psKick->ui32NumSrcSync; i++)
	{
		psSyncInfo = psKick->ahSrcSyncInfo[i];
		psSyncInfo->psSyncData->ui32ReadOpsPending++;
	}

	if (psKick->hDstSyncInfo != IMG_NULL)
	{
		psSyncInfo = psKick->hDstSyncInfo;
		psSyncInfo->psSyncData->ui32WriteOpsPending++;
	}

#if defined(PDUMP)
	if (PDumpIsCaptureFrameKM()
	|| ((psKick->ui32PDumpFlags & PDUMP_FLAGS_CONTINUOUS) != 0))
	{
		/* Pdump the command from the per context CCB */
		PDUMPCOMMENT("Shared part of 2D command\r\n");
		PDUMPMEM(ps2DCmd,
				psCCBMemInfo,
				psKick->ui32CCBDumpWOff,
				sizeof(SGXMKIF_2DCMD_SHARED),
				psKick->ui32PDumpFlags,
				MAKEUNIQUETAG(psCCBMemInfo));

		for (i = 0; i < psKick->ui32NumSrcSync; i++)
		{
			psSyncInfo = psKick->ahSrcSyncInfo[i];

			PDUMPCOMMENT("Hack src surface write op in 2D cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
					psCCBMemInfo,
					psKick->ui32CCBDumpWOff + offsetof(SGXMKIF_2DCMD_SHARED, sSrcSyncData[i].ui32WriteOpsPendingVal),
					sizeof(psSyncInfo->psSyncData->ui32LastOpDumpVal),
					psKick->ui32PDumpFlags,
					MAKEUNIQUETAG(psCCBMemInfo));

			PDUMPCOMMENT("Hack src surface read op in 2D cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
					psCCBMemInfo,
					psKick->ui32CCBDumpWOff + offsetof(SGXMKIF_2DCMD_SHARED, sSrcSyncData[i].ui32ReadOpsPendingVal),
					sizeof(psSyncInfo->psSyncData->ui32LastReadOpDumpVal),
					psKick->ui32PDumpFlags,
					MAKEUNIQUETAG(psCCBMemInfo));
		}

		if (psKick->hDstSyncInfo != IMG_NULL)
		{
			psSyncInfo = psKick->hDstSyncInfo;

			PDUMPCOMMENT("Hack dest surface write op in 2D cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
					psCCBMemInfo,
					psKick->ui32CCBDumpWOff + offsetof(SGXMKIF_2DCMD_SHARED, sDstSyncData.ui32WriteOpsPendingVal),
					sizeof(psSyncInfo->psSyncData->ui32LastOpDumpVal),
					psKick->ui32PDumpFlags,
					MAKEUNIQUETAG(psCCBMemInfo));

			PDUMPCOMMENT("Hack dest surface read op in 2D cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
					psCCBMemInfo,
					psKick->ui32CCBDumpWOff + offsetof(SGXMKIF_2DCMD_SHARED, sDstSyncData.ui32ReadOpsPendingVal),
					sizeof(psSyncInfo->psSyncData->ui32LastReadOpDumpVal),
					psKick->ui32PDumpFlags,
					MAKEUNIQUETAG(psCCBMemInfo));
		}

		/* Read/Write ops pending updates, delayed from above */
		for (i = 0; i < psKick->ui32NumSrcSync; i++)
		{
			psSyncInfo = psKick->ahSrcSyncInfo[i];
			psSyncInfo->psSyncData->ui32LastReadOpDumpVal++;
		}

		if (psKick->hDstSyncInfo != IMG_NULL)
		{
			psSyncInfo = psKick->hDstSyncInfo;
			psSyncInfo->psSyncData->ui32LastOpDumpVal++;
		}
	}
#endif

	sCommand.ui32Data[1] = psKick->sHW2DContextDevVAddr.uiAddr;
	
	eError = SGXScheduleCCBCommandKM(hDevHandle, SGXMKIF_CMD_2D, &sCommand, KERNEL_ID, psKick->ui32PDumpFlags);

	if (eError == PVRSRV_ERROR_RETRY)
	{
		/* Client will retry, so undo the write ops pending increment
		   done above.
		 */
#if defined(PDUMP)
		if (PDumpIsCaptureFrameKM())
		{
			for (i = 0; i < psKick->ui32NumSrcSync; i++)
			{
				psSyncInfo = psKick->ahSrcSyncInfo[i];
				psSyncInfo->psSyncData->ui32LastReadOpDumpVal--;
			}

			if (psKick->hDstSyncInfo != IMG_NULL)
			{
				psSyncInfo = psKick->hDstSyncInfo;
				psSyncInfo->psSyncData->ui32LastOpDumpVal--;
			}
		}
#endif

		for (i = 0; i < psKick->ui32NumSrcSync; i++)
		{
			psSyncInfo = psKick->ahSrcSyncInfo[i];
			psSyncInfo->psSyncData->ui32ReadOpsPending--;
		}

		if (psKick->hDstSyncInfo != IMG_NULL)
		{
			psSyncInfo = psKick->hDstSyncInfo;
			psSyncInfo->psSyncData->ui32WriteOpsPending--;
		}

		/* Command needed to be synchronised with the TA? */
		if (psKick->hTASyncInfo != IMG_NULL)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->hTASyncInfo;

			psSyncInfo->psSyncData->ui32WriteOpsPending--;
		}

		/* Command needed to be synchronised with the 3D? */
		if (psKick->h3DSyncInfo != IMG_NULL)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->h3DSyncInfo;

			psSyncInfo->psSyncData->ui32WriteOpsPending--;
		}
	}

	


#if defined(NO_HARDWARE)
	/* Update sync objects pretending that we have done the job*/
	for(i = 0; i < psKick->ui32NumSrcSync; i++)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->ahSrcSyncInfo[i];
		psSyncInfo->psSyncData->ui32ReadOpsComplete = psSyncInfo->psSyncData->ui32ReadOpsPending;
	}

	psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->hDstSyncInfo;
	psSyncInfo->psSyncData->ui32WriteOpsComplete = psSyncInfo->psSyncData->ui32WriteOpsPending;

	if (psKick->hTASyncInfo != IMG_NULL)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->hTASyncInfo;

		psSyncInfo->psSyncData->ui32WriteOpsComplete = psSyncInfo->psSyncData->ui32WriteOpsPending;
	}

	if (psKick->h3DSyncInfo != IMG_NULL)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psKick->h3DSyncInfo;

		psSyncInfo->psSyncData->ui32WriteOpsComplete = psSyncInfo->psSyncData->ui32WriteOpsPending;
	}
#endif

	return eError;
}
#endif	/* SGX_FEATURE_2D_HARDWARE */
#endif	/* TRANSFER_QUEUE */
