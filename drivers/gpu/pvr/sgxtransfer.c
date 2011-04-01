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


#include <stddef.h>

#include "sgxdefs.h"
#include "services_headers.h"
#include "buffer_manager.h"
#include "sgxinfo.h"
#include "sysconfig.h"
#include "pvr_pdump.h"
#include "mmu.h"
#include "pvr_bridge.h"
#include "sgx_bridge_km.h"
#include "sgxinfokm.h"
#include "osfunc.h"
#include "pvr_debug.h"
#include "sgxutils.h"
#include "perproc.h"

enum PVRSRV_ERROR SGXSubmitTransferKM(void *hDevHandle,
					struct PVRSRV_TRANSFER_SGX_KICK *psKick,
					struct PVRSRV_PER_PROCESS_DATA *proc)
{
	struct PVRSRV_KERNEL_MEM_INFO *psCCBMemInfo =
	    (struct PVRSRV_KERNEL_MEM_INFO *)psKick->hCCBMemInfo;
	struct SGXMKIF_COMMAND sCommand = { 0 };
	struct SGXMKIF_TRANSFERCMD_SHARED *psSharedTransferCmd;
	struct PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
	enum PVRSRV_ERROR eError;

	if (!CCB_OFFSET_IS_VALID
	    (struct SGXMKIF_TRANSFERCMD_SHARED, psCCBMemInfo, psKick,
	     ui32SharedCmdCCBOffset)) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SGXSubmitTransferKM: Invalid CCB offset");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psSharedTransferCmd =
	    CCB_DATA_FROM_OFFSET(struct SGXMKIF_TRANSFERCMD_SHARED,
				 psCCBMemInfo, psKick, ui32SharedCmdCCBOffset);

	if (psKick->hTASyncInfo != NULL) {
		psSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)
			psKick->hTASyncInfo;

		psSharedTransferCmd->ui32TASyncWriteOpsPendingVal =
		    psSyncInfo->psSyncData->ui32WriteOpsPending++;
		psSharedTransferCmd->ui32TASyncReadOpsPendingVal =
		    psSyncInfo->psSyncData->ui32ReadOpsPending;

		psSharedTransferCmd->sTASyncWriteOpsCompleteDevVAddr =
		    psSyncInfo->sWriteOpsCompleteDevVAddr;
		psSharedTransferCmd->sTASyncReadOpsCompleteDevVAddr =
		    psSyncInfo->sReadOpsCompleteDevVAddr;
	} else {
		psSharedTransferCmd->sTASyncWriteOpsCompleteDevVAddr.uiAddr = 0;
		psSharedTransferCmd->sTASyncReadOpsCompleteDevVAddr.uiAddr = 0;
	}

	if (psKick->h3DSyncInfo != NULL) {
		psSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)
							psKick->h3DSyncInfo;

		psSharedTransferCmd->ui323DSyncWriteOpsPendingVal =
		    psSyncInfo->psSyncData->ui32WriteOpsPending++;
		psSharedTransferCmd->ui323DSyncReadOpsPendingVal =
		    psSyncInfo->psSyncData->ui32ReadOpsPending;

		psSharedTransferCmd->s3DSyncWriteOpsCompleteDevVAddr =
		    psSyncInfo->sWriteOpsCompleteDevVAddr;
		psSharedTransferCmd->s3DSyncReadOpsCompleteDevVAddr =
		    psSyncInfo->sReadOpsCompleteDevVAddr;
	} else {
		psSharedTransferCmd->s3DSyncWriteOpsCompleteDevVAddr.uiAddr = 0;
		psSharedTransferCmd->s3DSyncReadOpsCompleteDevVAddr.uiAddr = 0;
	}

	if ((psKick->ui32Flags & SGXMKIF_TQFLAGS_KEEPPENDING) == 0UL) {
		if (psKick->ui32NumSrcSync > 0) {
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)
						psKick->ahSrcSyncInfo[0];

			psSharedTransferCmd->ui32SrcWriteOpPendingVal =
			    psSyncInfo->psSyncData->ui32WriteOpsPending;
			psSharedTransferCmd->ui32SrcReadOpPendingVal =
			    psSyncInfo->psSyncData->ui32ReadOpsPending;

			psSharedTransferCmd->sSrcWriteOpsCompleteDevAddr =
			    psSyncInfo->sWriteOpsCompleteDevVAddr;
			psSharedTransferCmd->sSrcReadOpsCompleteDevAddr =
			    psSyncInfo->sReadOpsCompleteDevVAddr;
		}
		if (psKick->ui32NumDstSync > 0) {
			psSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)
						psKick->ahDstSyncInfo[0];
			psSharedTransferCmd->ui32DstWriteOpPendingVal =
			    psSyncInfo->psSyncData->ui32WriteOpsPending;
			psSharedTransferCmd->ui32DstReadOpPendingVal =
			    psSyncInfo->psSyncData->ui32ReadOpsPending;
			psSharedTransferCmd->sDstWriteOpsCompleteDevAddr =
			    psSyncInfo->sWriteOpsCompleteDevVAddr;
			psSharedTransferCmd->sDstReadOpsCompleteDevAddr =
			    psSyncInfo->sReadOpsCompleteDevVAddr;
		}

		if (psKick->ui32NumSrcSync > 0) {
			psSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)
						psKick->ahSrcSyncInfo[0];
			psSyncInfo->psSyncData->ui32ReadOpsPending++;

		}
		if (psKick->ui32NumDstSync > 0) {
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psKick->
							    ahDstSyncInfo[0];
			psSyncInfo->psSyncData->ui32WriteOpsPending++;
		}
	}

	if (psKick->ui32NumDstSync > 1 || psKick->ui32NumSrcSync > 1) {
		PVR_DPF(PVR_DBG_ERROR,
			"Transfer command doesn't support "
			"more than 1 sync object per src/dst\ndst: %d, src: %d",
			 psKick->ui32NumDstSync, psKick->ui32NumSrcSync);
	}
#if defined(PDUMP)
	if (PDumpIsCaptureFrameKM() ||
	    ((psKick->ui32PDumpFlags & PDUMP_FLAGS_CONTINUOUS) != 0)) {
		PDUMPCOMMENT("Shared part of transfer command\r\n");
		PDUMPMEM(psSharedTransferCmd,
			 psCCBMemInfo,
			 psKick->ui32CCBDumpWOff,
			 sizeof(struct SGXMKIF_TRANSFERCMD_SHARED),
			 psKick->ui32PDumpFlags, MAKEUNIQUETAG(psCCBMemInfo));

		if ((psKick->ui32NumSrcSync > 0) &&
		    ((psKick->ui32Flags & SGXMKIF_TQFLAGS_KEEPPENDING) ==
		     0UL)) {
			psSyncInfo = psKick->ahSrcSyncInfo[0];

			PDUMPCOMMENT
			    ("Hack src surface write op in transfer cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
				 psCCBMemInfo,
				 psKick->ui32CCBDumpWOff +
				 offsetof(struct SGXMKIF_TRANSFERCMD_SHARED,
					    ui32SrcWriteOpPendingVal),
				 sizeof(psSyncInfo->psSyncData->
					ui32LastOpDumpVal),
				 psKick->ui32PDumpFlags,
				 MAKEUNIQUETAG(psCCBMemInfo));

			PDUMPCOMMENT
			    ("Hack src surface read op in transfer cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
				 psCCBMemInfo,
				 psKick->ui32CCBDumpWOff +
				 offsetof(struct SGXMKIF_TRANSFERCMD_SHARED,
					  ui32SrcReadOpPendingVal),
				 sizeof(psSyncInfo->psSyncData->
					ui32LastReadOpDumpVal),
				 psKick->ui32PDumpFlags,
				 MAKEUNIQUETAG(psCCBMemInfo));
		}
		if ((psKick->ui32NumDstSync > 0) &&
		    ((psKick->ui32Flags & SGXMKIF_TQFLAGS_KEEPPENDING) ==
		      0UL)) {
			psSyncInfo = psKick->ahDstSyncInfo[0];

			PDUMPCOMMENT
			    ("Hack dest surface write op in transfer cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
				 psCCBMemInfo,
				 psKick->ui32CCBDumpWOff +
				 offsetof(struct SGXMKIF_TRANSFERCMD_SHARED,
					  ui32DstWriteOpPendingVal),
				 sizeof(psSyncInfo->psSyncData->
					ui32LastOpDumpVal),
				 psKick->ui32PDumpFlags,
				 MAKEUNIQUETAG(psCCBMemInfo));

			PDUMPCOMMENT
			    ("Hack dest surface read op in transfer cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
				 psCCBMemInfo,
				 psKick->ui32CCBDumpWOff +
				 offsetof(struct SGXMKIF_TRANSFERCMD_SHARED,
					  ui32DstReadOpPendingVal),
				 sizeof(psSyncInfo->psSyncData->
					ui32LastReadOpDumpVal),
				 psKick->ui32PDumpFlags,
				 MAKEUNIQUETAG(psCCBMemInfo));
		}

		if ((psKick->ui32NumSrcSync > 0) &&
		    ((psKick->ui32Flags & SGXMKIF_TQFLAGS_KEEPPENDING) ==
		     0UL)) {
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psKick->
							    ahSrcSyncInfo[0];
			psSyncInfo->psSyncData->ui32LastReadOpDumpVal++;

		}

		if ((psKick->ui32NumDstSync > 0) &&
		    ((psKick->ui32Flags & SGXMKIF_TQFLAGS_KEEPPENDING) ==
		     0UL)) {
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psKick->
							    ahDstSyncInfo[0];
			psSyncInfo->psSyncData->ui32LastOpDumpVal++;
		}
	}
#endif

	sCommand.ui32Data[0] = PVRSRV_CCBFLAGS_TRANSFERCMD;
	sCommand.ui32Data[1] = psKick->sHWTransferContextDevVAddr.uiAddr;

	/* To aid in determining the next power down delay */
	sgx_mark_new_command(hDevHandle);

	eError = SGXScheduleCCBCommandKM(hDevHandle, SGXMKIF_COMMAND_EDM_KICK,
				    &sCommand, KERNEL_ID,
				    psKick->ui32PDumpFlags);

#if defined(NO_HARDWARE)
	if (!(psKick->ui32Flags & SGXMKIF_TQFLAGS_NOSYNCUPDATE)) {
		u32 i;

		for (i = 0; i < psKick->ui32NumSrcSync; i++) {
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psKick->
							    ahSrcSyncInfo[i];
			psSyncInfo->psSyncData->ui32ReadOpsComplete =
			    psSyncInfo->psSyncData->ui32ReadOpsPending;
		}

		for (i = 0; i < psKick->ui32NumDstSync; i++) {
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psKick->
							    ahDstSyncInfo[i];
			psSyncInfo->psSyncData->ui32WriteOpsComplete =
			    psSyncInfo->psSyncData->ui32WriteOpsPending;

		}

		if (psKick->hTASyncInfo != NULL) {
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psKick->
							    hTASyncInfo;

			psSyncInfo->psSyncData->ui32WriteOpsComplete =
			    psSyncInfo->psSyncData->ui32WriteOpsPending;
		}

		if (psKick->h3DSyncInfo != NULL) {
			psSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)psKick->
						    h3DSyncInfo;

			psSyncInfo->psSyncData->ui32WriteOpsComplete =
			    psSyncInfo->psSyncData->ui32WriteOpsPending;
		}
	}
#endif

	return eError;
}
