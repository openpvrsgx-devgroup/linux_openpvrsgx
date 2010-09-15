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
#include "services_headers.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#if defined(PDUMP)
#include "sgxapi_km.h"
#include "pdump_km.h"
#endif
#include "sgx_bridge_km.h"
#include "osfunc.h"
#include "pvr_debug.h"
#include "sgxutils.h"

enum PVRSRV_ERROR SGXDoKickKM(void *hDevHandle, struct SGX_CCB_KICK *psCCBKick,
			      int max_3dstat_vals)
{
	enum PVRSRV_ERROR eError;
	struct PVRSRV_KERNEL_SYNC_INFO *psSyncInfo;
	struct PVRSRV_KERNEL_MEM_INFO *psCCBMemInfo =
	    (struct PVRSRV_KERNEL_MEM_INFO *)psCCBKick->hCCBKernelMemInfo;
	struct SGXMKIF_CMDTA_SHARED *psTACmd;
	u32 i;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct PVRSRV_SGXDEV_INFO *psDevInfo;

	psDeviceNode = (struct PVRSRV_DEVICE_NODE *)hDevHandle;
	psDevInfo = (struct PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;

	if (psCCBKick->bKickRender)
		++psDevInfo->ui32KickTARenderCounter;
	++psDevInfo->ui32KickTACounter;

	if (!CCB_OFFSET_IS_VALID
	    (struct SGXMKIF_CMDTA_SHARED, psCCBMemInfo, psCCBKick,
	     ui32CCBOffset)) {
		PVR_DPF(PVR_DBG_ERROR, "SGXDoKickKM: Invalid CCB offset");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	psTACmd =
	    CCB_DATA_FROM_OFFSET(struct SGXMKIF_CMDTA_SHARED, psCCBMemInfo,
				 psCCBKick, ui32CCBOffset);

	if (psCCBKick->hTA3DSyncInfo) {
		struct PVRSRV_DEVICE_SYNC_OBJECT *ta3d_dep;

		psSyncInfo =
		    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->hTA3DSyncInfo;

		ta3d_dep = &psTACmd->sTA3DDependency;
		/*
		 * Ugly hack to account for the two possible sizes of
		 * struct SGXMKIF_CMDTA_SHARED which is based on the
		 * corresponding IOCTL ABI version used.
		 */
		if (max_3dstat_vals != SGX_MAX_3D_STATUS_VALS)
			ta3d_dep =  (struct PVRSRV_DEVICE_SYNC_OBJECT *)
				((u8 *)ta3d_dep - sizeof(struct CTL_STATUS) *
				 (SGX_MAX_3D_STATUS_VALS - max_3dstat_vals));

		ta3d_dep->sWriteOpsCompleteDevVAddr =
		    psSyncInfo->sWriteOpsCompleteDevVAddr;

		ta3d_dep->ui32WriteOpsPendingVal =
		    psSyncInfo->psSyncData->ui32WriteOpsPending;

		if (psCCBKick->bTADependency)
			psSyncInfo->psSyncData->ui32WriteOpsPending++;
	}

	if (psCCBKick->hTASyncInfo != NULL) {
		psSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)
						psCCBKick->hTASyncInfo;

		psTACmd->sTATQSyncReadOpsCompleteDevVAddr =
		    psSyncInfo->sReadOpsCompleteDevVAddr;
		psTACmd->sTATQSyncWriteOpsCompleteDevVAddr =
		    psSyncInfo->sWriteOpsCompleteDevVAddr;

		psTACmd->ui32TATQSyncReadOpsPendingVal =
		    psSyncInfo->psSyncData->ui32ReadOpsPending++;
		psTACmd->ui32TATQSyncWriteOpsPendingVal =
		    psSyncInfo->psSyncData->ui32WriteOpsPending;
	}

	if (psCCBKick->h3DSyncInfo != NULL) {
		psSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)
							psCCBKick->h3DSyncInfo;

		psTACmd->s3DTQSyncReadOpsCompleteDevVAddr =
		    psSyncInfo->sReadOpsCompleteDevVAddr;
		psTACmd->s3DTQSyncWriteOpsCompleteDevVAddr =
		    psSyncInfo->sWriteOpsCompleteDevVAddr;

		psTACmd->ui323DTQSyncReadOpsPendingVal =
		    psSyncInfo->psSyncData->ui32ReadOpsPending++;
		psTACmd->ui323DTQSyncWriteOpsPendingVal =
		    psSyncInfo->psSyncData->ui32WriteOpsPending;
	}

	psTACmd->ui32NumTAStatusVals = psCCBKick->ui32NumTAStatusVals;
	if (psCCBKick->ui32NumTAStatusVals != 0) {
		for (i = 0; i < psCCBKick->ui32NumTAStatusVals; i++) {
			psSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)
				psCCBKick->ahTAStatusSyncInfo[i];

			psTACmd->sCtlTAStatusInfo[i].sStatusDevAddr =
				psSyncInfo->sReadOpsCompleteDevVAddr;

			psTACmd->sCtlTAStatusInfo[i].ui32StatusValue =
				psSyncInfo->psSyncData->ui32ReadOpsPending;
		}
	}

	psTACmd->ui32Num3DStatusVals = psCCBKick->ui32Num3DStatusVals;
	if (psCCBKick->ui32Num3DStatusVals != 0) {
		for (i = 0; i < psCCBKick->ui32Num3DStatusVals; i++) {
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->
			    ah3DStatusSyncInfo[i];

			psTACmd->sCtl3DStatusInfo[i].sStatusDevAddr =
			    psSyncInfo->sReadOpsCompleteDevVAddr;

			psTACmd->sCtl3DStatusInfo[i].ui32StatusValue =
			    psSyncInfo->psSyncData->ui32ReadOpsPending;
		}
	}

	psTACmd->ui32NumSrcSyncs = psCCBKick->ui32NumSrcSyncs;
	for (i = 0; i < psCCBKick->ui32NumSrcSyncs; i++) {
		psSyncInfo =
		    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->
		    ahSrcKernelSyncInfo[i];

		psTACmd->asSrcSyncs[i].sWriteOpsCompleteDevVAddr =
			psSyncInfo->sWriteOpsCompleteDevVAddr;
		psTACmd->asSrcSyncs[i].sReadOpsCompleteDevVAddr =
			psSyncInfo->sReadOpsCompleteDevVAddr;

		psTACmd->asSrcSyncs[i].ui32ReadOpsPendingVal =
			psSyncInfo->psSyncData->ui32ReadOpsPending++;

		psTACmd->asSrcSyncs[i].ui32WriteOpsPendingVal =
			psSyncInfo->psSyncData->ui32WriteOpsPending;

	}

	if (psCCBKick->bFirstKickOrResume &&
	    psCCBKick->ui32NumDstSyncObjects > 0) {
		struct PVRSRV_KERNEL_MEM_INFO *psHWDstSyncListMemInfo =
		    (struct PVRSRV_KERNEL_MEM_INFO *)psCCBKick->
		    hKernelHWSyncListMemInfo;
		struct SGXMKIF_HWDEVICE_SYNC_LIST *psHWDeviceSyncList =
		    psHWDstSyncListMemInfo->pvLinAddrKM;
		u32 ui32NumDstSyncs = psCCBKick->ui32NumDstSyncObjects;

		PVR_ASSERT(((struct PVRSRV_KERNEL_MEM_INFO *)psCCBKick->
			    hKernelHWSyncListMemInfo)->ui32AllocSize >=
			   (sizeof(struct SGXMKIF_HWDEVICE_SYNC_LIST) +
			    (sizeof(struct PVRSRV_DEVICE_SYNC_OBJECT) *
			     ui32NumDstSyncs)));

		psHWDeviceSyncList->ui32NumSyncObjects = ui32NumDstSyncs;
#if defined(PDUMP)
		if (PDumpIsCaptureFrameKM()) {
			PDUMPCOMMENT("HWDeviceSyncList for TACmd\r\n");
			PDUMPMEM(NULL,
				 psHWDstSyncListMemInfo, 0,
				 sizeof(struct SGXMKIF_HWDEVICE_SYNC_LIST),
				 0, MAKEUNIQUETAG(psHWDstSyncListMemInfo));
		}
#endif
		psSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->
			    sDstSyncHandle;
		i = 0;
		if (psSyncInfo) {
			psHWDeviceSyncList->asSyncData[i].
			    sWriteOpsCompleteDevVAddr =
					psSyncInfo->sWriteOpsCompleteDevVAddr;

			psHWDeviceSyncList->asSyncData[i].
			    sReadOpsCompleteDevVAddr =
					psSyncInfo->sReadOpsCompleteDevVAddr;

			psHWDeviceSyncList->asSyncData[i].
			    ui32ReadOpsPendingVal =
				psSyncInfo->psSyncData->ui32ReadOpsPending;

			psHWDeviceSyncList->asSyncData[i].
			    ui32WriteOpsPendingVal =
				    psSyncInfo->psSyncData->
							ui32WriteOpsPending++;

#if defined(PDUMP)
			if (PDumpIsCaptureFrameKM()) {
				u32 ui32ModifiedValue;
				u32 ui32SyncOffset = offsetof(
					struct SGXMKIF_HWDEVICE_SYNC_LIST,
					asSyncData) + (i *
					sizeof(
					struct PVRSRV_DEVICE_SYNC_OBJECT));
				u32 ui32WOpsOffset = ui32SyncOffset +
					offsetof(
					struct PVRSRV_DEVICE_SYNC_OBJECT,
					ui32WriteOpsPendingVal);
				u32 ui32ROpsOffset = ui32SyncOffset +
					offsetof(
					struct PVRSRV_DEVICE_SYNC_OBJECT,
					ui32ReadOpsPendingVal);

				PDUMPCOMMENT("HWDeviceSyncObject for RT: "
					     "%i\r\n", i);

				PDUMPMEM(NULL, psHWDstSyncListMemInfo,
					ui32SyncOffset, sizeof(
					struct PVRSRV_DEVICE_SYNC_OBJECT),
					0, MAKEUNIQUETAG(
						psHWDstSyncListMemInfo));

				if ((psSyncInfo->psSyncData->
						ui32LastOpDumpVal == 0) &&
				    (psSyncInfo->psSyncData->
						ui32LastReadOpDumpVal == 0)) {

					PDUMPCOMMENT("Init RT ROpsComplete\r\n",
							 i);
					PDUMPMEM(&psSyncInfo->psSyncData->
							ui32LastReadOpDumpVal,
						psSyncInfo->psSyncDataMemInfoKM,
						offsetof(struct
							PVRSRV_SYNC_DATA,
							ui32ReadOpsComplete),
						sizeof(psSyncInfo->psSyncData->
							   ui32ReadOpsComplete),
						0,
						MAKEUNIQUETAG(psSyncInfo->
							psSyncDataMemInfoKM));

				PDUMPCOMMENT("Init RT WOpsComplete\r\n");
				PDUMPMEM(&psSyncInfo->psSyncData->
						ui32LastOpDumpVal,
					 psSyncInfo->psSyncDataMemInfoKM,
					 offsetof(struct PVRSRV_SYNC_DATA,
							ui32WriteOpsComplete),
					 sizeof(psSyncInfo->psSyncData->
							ui32WriteOpsComplete),
					 0, MAKEUNIQUETAG(psSyncInfo->
							psSyncDataMemInfoKM));
				}

				psSyncInfo->psSyncData->ui32LastOpDumpVal++;

				ui32ModifiedValue = psSyncInfo->psSyncData->
					    ui32LastOpDumpVal - 1;

				PDUMPCOMMENT("Modify RT %d WOpPendingVal "
					     "in HWDevSyncList\r\n", i);

				PDUMPMEM(&ui32ModifiedValue,
					 psHWDstSyncListMemInfo, ui32WOpsOffset,
					 sizeof(u32), 0,
					 MAKEUNIQUETAG(psHWDstSyncListMemInfo));

				PDUMPCOMMENT("Modify RT %d ROpsPendingVal "
					     "in HWDevSyncList\r\n", i);

				PDUMPMEM(&psSyncInfo->psSyncData->
						 ui32LastReadOpDumpVal,
					 psHWDstSyncListMemInfo,
					 ui32ROpsOffset, sizeof(u32), 0,
					 MAKEUNIQUETAG(psHWDstSyncListMemInfo));
			}
#endif
		} else {
			psHWDeviceSyncList->asSyncData[i].
			    sWriteOpsCompleteDevVAddr.uiAddr = 0;
			psHWDeviceSyncList->asSyncData[i].
			    sReadOpsCompleteDevVAddr.uiAddr = 0;

			psHWDeviceSyncList->asSyncData[i].
			    ui32ReadOpsPendingVal = 0;
			psHWDeviceSyncList->asSyncData[i].
			    ui32WriteOpsPendingVal = 0;
		}
	}
#if defined(PDUMP)
	if (PDumpIsCaptureFrameKM()) {
		PDUMPCOMMENT("Shared part of TA command\r\n");

		PDUMPMEM(psTACmd, psCCBMemInfo, psCCBKick->ui32CCBDumpWOff,
			 sizeof(struct SGXMKIF_CMDTA_SHARED), 0,
			 MAKEUNIQUETAG(psCCBMemInfo));

		for (i = 0; i < psCCBKick->ui32NumSrcSyncs; i++) {
			u32 ui32ModifiedValue;
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->
			    ahSrcKernelSyncInfo[i];

			if ((psSyncInfo->psSyncData->ui32LastOpDumpVal == 0) &&
			    (psSyncInfo->psSyncData->ui32LastReadOpDumpVal ==
			     0)) {
				PDUMPCOMMENT("Init RT ROpsComplete\r\n", i);
				PDUMPMEM(&psSyncInfo->psSyncData->
					 ui32LastReadOpDumpVal,
					 psSyncInfo->psSyncDataMemInfoKM,
					 offsetof(struct PVRSRV_SYNC_DATA,
						  ui32ReadOpsComplete),
					 sizeof(psSyncInfo->psSyncData->
						ui32ReadOpsComplete), 0,
					 MAKEUNIQUETAG(psSyncInfo->
						       psSyncDataMemInfoKM));
				PDUMPCOMMENT("Init RT WOpsComplete\r\n");
				PDUMPMEM(&psSyncInfo->psSyncData->
					 ui32LastOpDumpVal,
					 psSyncInfo->psSyncDataMemInfoKM,
					 offsetof(struct PVRSRV_SYNC_DATA,
						  ui32WriteOpsComplete),
					 sizeof(psSyncInfo->psSyncData->
						ui32WriteOpsComplete), 0,
					 MAKEUNIQUETAG(psSyncInfo->
						       psSyncDataMemInfoKM));
			}

			psSyncInfo->psSyncData->ui32LastReadOpDumpVal++;

			ui32ModifiedValue =
			    psSyncInfo->psSyncData->ui32LastReadOpDumpVal - 1;

			PDUMPCOMMENT("Modify SrcSync %d ROpsPendingVal\r\n", i);

			PDUMPMEM(&ui32ModifiedValue,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff +
				 offsetof(struct SGXMKIF_CMDTA_SHARED,
					  asSrcSyncs) +
				 (i *
				  sizeof(struct PVRSRV_DEVICE_SYNC_OBJECT)) +
				 offsetof(struct PVRSRV_DEVICE_SYNC_OBJECT,
					  ui32ReadOpsPendingVal), sizeof(u32),
				 0, MAKEUNIQUETAG(psCCBMemInfo));

			PDUMPCOMMENT("Modify SrcSync %d WOpPendingVal\r\n", i);

			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff +
				 offsetof(struct SGXMKIF_CMDTA_SHARED,
					  asSrcSyncs) +
				 (i *
				  sizeof(struct PVRSRV_DEVICE_SYNC_OBJECT)) +
				 offsetof(struct PVRSRV_DEVICE_SYNC_OBJECT,
					  ui32WriteOpsPendingVal), sizeof(u32),
				 0, MAKEUNIQUETAG(psCCBMemInfo));

		}

		for (i = 0; i < psCCBKick->ui32NumTAStatusVals; i++) {
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->
			    ahTAStatusSyncInfo[i];
			PDUMPCOMMENT("Modify TA status value in TA cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff +
				 offsetof(struct SGXMKIF_CMDTA_SHARED,
					  sCtlTAStatusInfo[i].ui32StatusValue),
				 sizeof(u32), 0, MAKEUNIQUETAG(psCCBMemInfo));
		}

		for (i = 0; i < psCCBKick->ui32Num3DStatusVals; i++) {
			psSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)
				psCCBKick->ah3DStatusSyncInfo[i];

			PDUMPCOMMENT("Modify 3D status value in TA cmd\r\n");

			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff +
				 offsetof(struct SGXMKIF_CMDTA_SHARED,
					  sCtl3DStatusInfo[i].ui32StatusValue),
				 sizeof(u32), 0, MAKEUNIQUETAG(psCCBMemInfo));
		}
	}
#endif

	eError = SGXScheduleCCBCommandKM(hDevHandle, psCCBKick->eCommand,
				    &psCCBKick->sCommand, KERNEL_ID, 0);
	if (eError == PVRSRV_ERROR_RETRY) {
		if (psCCBKick->bFirstKickOrResume &&
		    psCCBKick->ui32NumDstSyncObjects > 0) {
			psSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)
				    psCCBKick->sDstSyncHandle;
			if (psSyncInfo) {
				psSyncInfo->psSyncData->ui32WriteOpsPending--;
#if defined(PDUMP)
				if (PDumpIsCaptureFrameKM())
					psSyncInfo->psSyncData->
							ui32LastOpDumpVal--;
#endif
			}
		}

		for (i = 0; i < psCCBKick->ui32NumSrcSyncs; i++) {
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->
			    ahSrcKernelSyncInfo[i];
			psSyncInfo->psSyncData->ui32ReadOpsPending--;
		}

		return eError;
	} else if (PVRSRV_OK != eError) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SGXDoKickKM: SGXScheduleCCBCommandKM failed.");
		return eError;
	}

#if defined(NO_HARDWARE)

	if (psCCBKick->hTA3DSyncInfo) {
		psSyncInfo =
		    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->hTA3DSyncInfo;

		if (psCCBKick->bTADependency) {
			psSyncInfo->psSyncData->ui32WriteOpsComplete =
			    psSyncInfo->psSyncData->ui32WriteOpsPending;
		}
	}

	if (psCCBKick->hTASyncInfo != NULL) {
		psSyncInfo =
		    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->hTASyncInfo;

		psSyncInfo->psSyncData->ui32ReadOpsComplete =
		    psSyncInfo->psSyncData->ui32ReadOpsPending;
	}

	if (psCCBKick->h3DSyncInfo != NULL) {
		psSyncInfo =
		    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->h3DSyncInfo;

		psSyncInfo->psSyncData->ui32ReadOpsComplete =
		    psSyncInfo->psSyncData->ui32ReadOpsPending;
	}

	for (i = 0; i < psCCBKick->ui32NumTAStatusVals; i++) {
		psSyncInfo =
		    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->
		    ahTAStatusSyncInfo[i];
		psSyncInfo->psSyncData->ui32ReadOpsComplete =
		    psTACmd->sCtlTAStatusInfo[i].ui32StatusValue;
	}

	for (i = 0; i < psCCBKick->ui32NumSrcSyncs; i++) {
		psSyncInfo =
		    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->
		    ahSrcKernelSyncInfo[i];

		psSyncInfo->psSyncData->ui32ReadOpsComplete =
		    psSyncInfo->psSyncData->ui32ReadOpsPending;

	}

	if (psCCBKick->bTerminateOrAbort) {
		if (psCCBKick->ui32NumDstSyncObjects > 0) {
			struct PVRSRV_KERNEL_MEM_INFO *psHWDstSyncListMemInfo =
			    (struct PVRSRV_KERNEL_MEM_INFO *)psCCBKick->
			    hKernelHWSyncListMemInfo;
			struct SGXMKIF_HWDEVICE_SYNC_LIST *psHWDeviceSyncList =
			    psHWDstSyncListMemInfo->pvLinAddrKM;

			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->
			    sDstSyncHandle;
			if (psSyncInfo)
				psSyncInfo->psSyncData->ui32WriteOpsComplete =
				    psHWDeviceSyncList->asSyncData[0].
				    ui32WriteOpsPendingVal + 1;
		}

		for (i = 0; i < psCCBKick->ui32Num3DStatusVals; i++) {
			psSyncInfo =
			    (struct PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->
			    ah3DStatusSyncInfo[i];
			psSyncInfo->psSyncData->ui32ReadOpsComplete =
			    psTACmd->sCtl3DStatusInfo[i].ui32StatusValue;
		}
	}
#endif

	return eError;
}
