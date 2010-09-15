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

#include <linux/errno.h>

#include <stddef.h>

#include "img_defs.h"

#include "services.h"
#include "pvr_debug.h"
#include "pvr_bridge.h"
#include "sgx_bridge.h"
#include "perproc.h"
#include "power.h"
#include "pvr_bridge_km.h"
#include "sgx_bridge_km.h"
#include "bridged_pvr_bridge.h"
#include "bridged_sgx_bridge.h"
#include "sgxutils.h"
#include "pdump_km.h"
#include "pvr_events.h"

int SGXGetClientInfoBW(u32 ui32BridgeID,
	      struct PVRSRV_BRIDGE_IN_GETCLIENTINFO *psGetClientInfoIN,
	      struct PVRSRV_BRIDGE_OUT_GETCLIENTINFO *psGetClientInfoOUT,
	      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SGX_GETCLIENTINFO);

	psGetClientInfoOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psGetClientInfoIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psGetClientInfoOUT->eError != PVRSRV_OK)
		return 0;

	psGetClientInfoOUT->eError =
	    SGXGetClientInfoKM(hDevCookieInt, &psGetClientInfoOUT->sClientInfo);
	return 0;
}

int SGXReleaseClientInfoBW(u32 ui32BridgeID,
	  struct PVRSRV_BRIDGE_IN_RELEASECLIENTINFO *psReleaseClientInfoIN,
	  struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	struct PVRSRV_SGXDEV_INFO *psDevInfo;
	void *hDevCookieInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_RELEASECLIENTINFO);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psReleaseClientInfoIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psDevInfo =
	    (struct PVRSRV_SGXDEV_INFO *)((struct PVRSRV_DEVICE_NODE *)
					  hDevCookieInt)->pvDevice;

	PVR_ASSERT(psDevInfo->ui32ClientRefCount > 0);

	psDevInfo->ui32ClientRefCount--;

	psRetOUT->eError = PVRSRV_OK;

	return 0;
}

int SGXGetInternalDevInfoBW(u32 ui32BridgeID,
	struct PVRSRV_BRIDGE_IN_GETINTERNALDEVINFO *psSGXGetInternalDevInfoIN,
	struct PVRSRV_BRIDGE_OUT_GETINTERNALDEVINFO *psSGXGetInternalDevInfoOUT,
	struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_GETINTERNALDEVINFO);

	psSGXGetInternalDevInfoOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psSGXGetInternalDevInfoIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psSGXGetInternalDevInfoOUT->eError != PVRSRV_OK)
		return 0;

	psSGXGetInternalDevInfoOUT->eError =
	    SGXGetInternalDevInfoKM(hDevCookieInt,
			    &psSGXGetInternalDevInfoOUT->sSGXInternalDevInfo);

	psSGXGetInternalDevInfoOUT->eError =
	    PVRSRVAllocHandle(psPerProc->psHandleBase,
			      &psSGXGetInternalDevInfoOUT->sSGXInternalDevInfo.
					      hHostCtlKernelMemInfoHandle,
			      psSGXGetInternalDevInfoOUT->sSGXInternalDevInfo.
					      hHostCtlKernelMemInfoHandle,
			      PVRSRV_HANDLE_TYPE_MEM_INFO,
			      PVRSRV_HANDLE_ALLOC_FLAG_SHARED);

	return 0;
}

/* Convert the IOCTL parameter from "old" to "new" format. */
static int kick_compat_conv(struct PVRSRV_BRIDGE_IN_DOKICK *kick,
			    size_t in_size)
{
	struct SGX_CCB_KICK *ccb;
	size_t diff;

	ccb = &kick->sCCBKick;

	diff = sizeof(ccb->ah3DStatusSyncInfo[0]) *
		(SGX_MAX_3D_STATUS_VALS - SGX_MAX_3D_STATUS_VALS_OLD);
	if (sizeof(*kick) - in_size != diff)
		return -EINVAL;

	/* Trailing size at the end of struct to move. */
	diff = sizeof(*kick) - offsetof(typeof(*kick),
			sCCBKick.ah3DStatusSyncInfo[SGX_MAX_3D_STATUS_VALS]);
	memmove(&kick->sCCBKick.ah3DStatusSyncInfo[SGX_MAX_3D_STATUS_VALS],
		&kick->sCCBKick.ah3DStatusSyncInfo[SGX_MAX_3D_STATUS_VALS_OLD],
		diff);

	return 0;
}

int SGXDoKickBW(u32 ui32BridgeID,
		       struct PVRSRV_BRIDGE_IN_DOKICK *psDoKickIN,
		       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		       size_t in_size,
		       struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	int max_3dstat_vals;
	u32 i;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SGX_DOKICK);

	max_3dstat_vals = SGX_MAX_3D_STATUS_VALS;
	if (unlikely(in_size) != sizeof(*psDoKickIN)) {
		max_3dstat_vals = SGX_MAX_3D_STATUS_VALS_OLD;
		if (kick_compat_conv(psDoKickIN, in_size) != 0) {
			psRetOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;
			return -EINVAL;
		}

	}
	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					      &hDevCookieInt,
					      psDoKickIN->hDevCookie,
					      PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				&psDoKickIN->sCCBKick.hCCBKernelMemInfo,
				psDoKickIN->sCCBKick.hCCBKernelMemInfo,
				PVRSRV_HANDLE_TYPE_MEM_INFO);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	if (psDoKickIN->sCCBKick.hTA3DSyncInfo != NULL) {
		psRetOUT->eError =
		    PVRSRVLookupHandle(psPerProc->psHandleBase,
				       &psDoKickIN->sCCBKick.hTA3DSyncInfo,
				       psDoKickIN->sCCBKick.hTA3DSyncInfo,
				       PVRSRV_HANDLE_TYPE_SYNC_INFO);

		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	if (psDoKickIN->sCCBKick.hTASyncInfo != NULL) {
		psRetOUT->eError =
		    PVRSRVLookupHandle(psPerProc->psHandleBase,
				       &psDoKickIN->sCCBKick.hTASyncInfo,
				       psDoKickIN->sCCBKick.hTASyncInfo,
				       PVRSRV_HANDLE_TYPE_SYNC_INFO);

		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	if (psDoKickIN->sCCBKick.h3DSyncInfo != NULL) {
		psRetOUT->eError =
		    PVRSRVLookupHandle(psPerProc->psHandleBase,
				       &psDoKickIN->sCCBKick.h3DSyncInfo,
				       psDoKickIN->sCCBKick.h3DSyncInfo,
				       PVRSRV_HANDLE_TYPE_SYNC_INFO);

		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	if (psDoKickIN->sCCBKick.ui32NumSrcSyncs > SGX_MAX_SRC_SYNCS) {
		psRetOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;
		return 0;
	}
	for (i = 0; i < psDoKickIN->sCCBKick.ui32NumSrcSyncs; i++) {
		psRetOUT->eError =
		    PVRSRVLookupHandle(psPerProc->psHandleBase,
				       &psDoKickIN->sCCBKick.
				       ahSrcKernelSyncInfo[i],
				       psDoKickIN->sCCBKick.
				       ahSrcKernelSyncInfo[i],
				       PVRSRV_HANDLE_TYPE_SYNC_INFO);

		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	if (psDoKickIN->sCCBKick.ui32NumTAStatusVals > SGX_MAX_TA_STATUS_VALS) {
		psRetOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;
		return 0;
	}
	for (i = 0; i < psDoKickIN->sCCBKick.ui32NumTAStatusVals; i++) {
		psRetOUT->eError =
		    PVRSRVLookupHandle(psPerProc->psHandleBase,
				    &psDoKickIN->sCCBKick.ahTAStatusSyncInfo[i],
				    psDoKickIN->sCCBKick.ahTAStatusSyncInfo[i],
				    PVRSRV_HANDLE_TYPE_SYNC_INFO);
		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	if (psDoKickIN->sCCBKick.ui32Num3DStatusVals > SGX_MAX_3D_STATUS_VALS) {
		psRetOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;
		return 0;
	}
	for (i = 0; i < psDoKickIN->sCCBKick.ui32Num3DStatusVals; i++) {
		psRetOUT->eError =
		    PVRSRVLookupHandle(psPerProc->psHandleBase,
				    &psDoKickIN->sCCBKick.ah3DStatusSyncInfo[i],
				    psDoKickIN->sCCBKick.ah3DStatusSyncInfo[i],
				    PVRSRV_HANDLE_TYPE_SYNC_INFO);

		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	if (psDoKickIN->sCCBKick.ui32NumDstSyncObjects > 0) {
		psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				       &psDoKickIN->sCCBKick.
						       hKernelHWSyncListMemInfo,
				       psDoKickIN->sCCBKick.
						       hKernelHWSyncListMemInfo,
				       PVRSRV_HANDLE_TYPE_MEM_INFO);

		if (psRetOUT->eError != PVRSRV_OK)
			return 0;

		psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				      &psDoKickIN->sCCBKick.sDstSyncHandle,
				      psDoKickIN->sCCBKick.sDstSyncHandle,
				      PVRSRV_HANDLE_TYPE_SYNC_INFO);

		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	psRetOUT->eError = SGXDoKickKM(hDevCookieInt, &psDoKickIN->sCCBKick,
					max_3dstat_vals);

	return 0;
}

int SGXScheduleProcessQueuesBW(u32 ui32BridgeID,
      struct PVRSRV_BRIDGE_IN_SGX_SCHEDULE_PROCESS_QUEUES *psScheduleProcQIN,
      struct PVRSRV_BRIDGE_RETURN *psRetOUT,
      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_SCHEDULE_PROCESS_QUEUES);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &hDevCookieInt,
			       psScheduleProcQIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = SGXScheduleProcessQueuesKM(hDevCookieInt);

	return 0;
}

int SGXSubmitTransferBW(u32 ui32BridgeID,
	       struct PVRSRV_BRIDGE_IN_SUBMITTRANSFER *psSubmitTransferIN,
	       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	       struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	struct PVRSRV_TRANSFER_SGX_KICK *psKick;
	u32 i;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_SUBMITTRANSFER);
	PVR_UNREFERENCED_PARAMETER(ui32BridgeID);

	psKick = &psSubmitTransferIN->sKick;

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					       &hDevCookieInt,
					       psSubmitTransferIN->hDevCookie,
					       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					      &psKick->hCCBMemInfo,
					      psKick->hCCBMemInfo,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	if (psKick->hTASyncInfo != NULL) {
		psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
						  &psKick->hTASyncInfo,
						  psKick->hTASyncInfo,
						  PVRSRV_HANDLE_TYPE_SYNC_INFO);
		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	if (psKick->h3DSyncInfo != NULL) {
		psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					     &psKick->h3DSyncInfo,
					     psKick->h3DSyncInfo,
					     PVRSRV_HANDLE_TYPE_SYNC_INFO);
		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	if (psKick->ui32NumSrcSync > SGX_MAX_TRANSFER_SYNC_OPS) {
		psRetOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;
		return 0;
	}
	for (i = 0; i < psKick->ui32NumSrcSync; i++) {
		psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				       &psKick->ahSrcSyncInfo[i],
				       psKick->ahSrcSyncInfo[i],
				       PVRSRV_HANDLE_TYPE_SYNC_INFO);
		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	if (psKick->ui32NumDstSync > SGX_MAX_TRANSFER_SYNC_OPS) {
		psRetOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;
		return 0;
	}
	for (i = 0; i < psKick->ui32NumDstSync; i++) {
		psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				       &psKick->ahDstSyncInfo[i],
				       psKick->ahDstSyncInfo[i],
				       PVRSRV_HANDLE_TYPE_SYNC_INFO);
		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	psRetOUT->eError = SGXSubmitTransferKM(hDevCookieInt, psKick);

	return 0;
}

int SGXGetMiscInfoBW(u32 ui32BridgeID,
		    struct PVRSRV_BRIDGE_IN_SGXGETMISCINFO *psSGXGetMiscInfoIN,
		    struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	struct PVRSRV_SGXDEV_INFO *psDevInfo;
	struct SGX_MISC_INFO sMiscInfo;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SGX_GETMISCINFO);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					      &hDevCookieInt,
					      psSGXGetMiscInfoIN->hDevCookie,
					      PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psDeviceNode = hDevCookieInt;
	PVR_ASSERT(psDeviceNode != NULL);
	if (psDeviceNode == NULL)
		return -EFAULT;

	psDevInfo = psDeviceNode->pvDevice;

	psRetOUT->eError = CopyFromUserWrapper(psPerProc, ui32BridgeID,
					       &sMiscInfo,
					       psSGXGetMiscInfoIN->psMiscInfo,
					       sizeof(struct SGX_MISC_INFO));
	if (psRetOUT->eError != PVRSRV_OK)
		return -EFAULT;

	if (sMiscInfo.eRequest == SGX_MISC_INFO_REQUEST_HWPERF_RETRIEVE_CB) {
		void *pAllocated;
		void *hAllocatedHandle;
		void __user *psTmpUserData;
		u32 allocatedSize;

		allocatedSize =
		    (u32) (sMiscInfo.uData.sRetrieveCB.ui32ArraySize *
			   sizeof(struct PVRSRV_SGX_HWPERF_CBDATA));

		ASSIGN_AND_EXIT_ON_ERROR(psRetOUT->eError,
					 OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						    allocatedSize,
						    &pAllocated,
						    &hAllocatedHandle));

		psTmpUserData = (void __force __user *)
				sMiscInfo.uData.sRetrieveCB.psHWPerfData;
		sMiscInfo.uData.sRetrieveCB.psHWPerfData = pAllocated;

		psRetOUT->eError = SGXGetMiscInfoKM(psDevInfo,
						    &sMiscInfo, psDeviceNode);
		if (psRetOUT->eError != PVRSRV_OK) {
			OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  allocatedSize, pAllocated, hAllocatedHandle);
			return 0;
		}

		psRetOUT->eError = CopyToUserWrapper(psPerProc,
				     ui32BridgeID, psTmpUserData,
				     sMiscInfo.uData.sRetrieveCB.psHWPerfData,
				     allocatedSize);

		sMiscInfo.uData.sRetrieveCB.psHWPerfData =
						(void __force *)psTmpUserData;

		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  allocatedSize, pAllocated, hAllocatedHandle);

		if (psRetOUT->eError != PVRSRV_OK)
			return -EFAULT;
	} else {
		psRetOUT->eError = SGXGetMiscInfoKM(psDevInfo,
						    &sMiscInfo, psDeviceNode);

		if (psRetOUT->eError != PVRSRV_OK)
			return 0;
	}

	psRetOUT->eError = CopyToUserWrapper(psPerProc,
					     ui32BridgeID,
					     psSGXGetMiscInfoIN->psMiscInfo,
					     &sMiscInfo,
					     sizeof(struct SGX_MISC_INFO));
	if (psRetOUT->eError != PVRSRV_OK)
		return -EFAULT;
	return 0;
}

int SGXReadDiffCountersBW(u32 ui32BridgeID,
	struct PVRSRV_BRIDGE_IN_SGX_READ_DIFF_COUNTERS *psSGXReadDiffCountersIN,
	struct PVRSRV_BRIDGE_OUT_SGX_READ_DIFF_COUNTERS
						*psSGXReadDiffCountersOUT,
	struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_READ_DIFF_COUNTERS);

	psSGXReadDiffCountersOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psSGXReadDiffCountersIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psSGXReadDiffCountersOUT->eError != PVRSRV_OK)
		return 0;

	psSGXReadDiffCountersOUT->eError = SGXReadDiffCountersKM(
				hDevCookieInt,
				psSGXReadDiffCountersIN->ui32Reg,
				&psSGXReadDiffCountersOUT->ui32Old,
				psSGXReadDiffCountersIN->bNew,
				psSGXReadDiffCountersIN->ui32New,
				psSGXReadDiffCountersIN->ui32NewReset,
				psSGXReadDiffCountersIN->ui32CountersReg,
				&psSGXReadDiffCountersOUT->ui32Time,
				&psSGXReadDiffCountersOUT->bActive,
				&psSGXReadDiffCountersOUT->sDiffs);

	return 0;
}

int SGXReadHWPerfCBBW(u32 ui32BridgeID,
	     struct PVRSRV_BRIDGE_IN_SGX_READ_HWPERF_CB *psSGXReadHWPerfCBIN,
	     struct PVRSRV_BRIDGE_OUT_SGX_READ_HWPERF_CB *psSGXReadHWPerfCBOUT,
	     struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	struct PVRSRV_SGX_HWPERF_CB_ENTRY *psAllocated;
	void *hAllocatedHandle;
	u32 ui32AllocatedSize;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_READ_HWPERF_CB);

	psSGXReadHWPerfCBOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psSGXReadHWPerfCBIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psSGXReadHWPerfCBOUT->eError != PVRSRV_OK)
		return 0;

	ui32AllocatedSize = psSGXReadHWPerfCBIN->ui32ArraySize *
				sizeof(psSGXReadHWPerfCBIN->psHWPerfCBData[0]);
	ASSIGN_AND_EXIT_ON_ERROR(psSGXReadHWPerfCBOUT->eError,
				 OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
					    ui32AllocatedSize,
					    (void **)&psAllocated,
					    &hAllocatedHandle));

	psSGXReadHWPerfCBOUT->eError = SGXReadHWPerfCBKM(hDevCookieInt,
				 psSGXReadHWPerfCBIN->ui32ArraySize,
				 psAllocated,
				 &psSGXReadHWPerfCBOUT->ui32DataCount,
				 &psSGXReadHWPerfCBOUT->ui32ClockSpeed,
				 &psSGXReadHWPerfCBOUT->ui32HostTimeStamp);
	if (psSGXReadHWPerfCBOUT->eError == PVRSRV_OK)
		psSGXReadHWPerfCBOUT->eError = CopyToUserWrapper(
					 psPerProc, ui32BridgeID,
					 psSGXReadHWPerfCBIN->psHWPerfCBData,
					 psAllocated, ui32AllocatedSize);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
		  ui32AllocatedSize, psAllocated, hAllocatedHandle);

	return 0;
}

int SGXDevInitPart2BW(u32 ui32BridgeID,
		struct PVRSRV_BRIDGE_IN_SGXDEVINITPART2 *psSGXDevInitPart2IN,
		struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	enum PVRSRV_ERROR eError;
	IMG_BOOL bDissociateFailed = IMG_FALSE;
	IMG_BOOL bLookupFailed = IMG_FALSE;
	IMG_BOOL bReleaseFailed = IMG_FALSE;
	void *hDummy;
	u32 i;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_SGX_DEVINITPART2);

	if (!psPerProc->bInitProcess) {
		psRetOUT->eError = PVRSRV_ERROR_GENERIC;
		return 0;
	}

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
						&hDevCookieInt,
						psSGXDevInitPart2IN->hDevCookie,
						PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase, &hDummy,
				    psSGXDevInitPart2IN->sInitInfo.
					    hKernelCCBMemInfo,
				    PVRSRV_HANDLE_TYPE_MEM_INFO);
	bLookupFailed |= (IMG_BOOL) (eError != PVRSRV_OK);

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase, &hDummy,
				    psSGXDevInitPart2IN->sInitInfo.
					    hKernelCCBCtlMemInfo,
				    PVRSRV_HANDLE_TYPE_MEM_INFO);
	bLookupFailed |= (IMG_BOOL) (eError != PVRSRV_OK);

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase, &hDummy,
				    psSGXDevInitPart2IN->sInitInfo.
					    hKernelCCBEventKickerMemInfo,
				    PVRSRV_HANDLE_TYPE_MEM_INFO);
	bLookupFailed |= (IMG_BOOL) (eError != PVRSRV_OK);

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase, &hDummy,
				    psSGXDevInitPart2IN->sInitInfo.
					    hKernelSGXHostCtlMemInfo,
				    PVRSRV_HANDLE_TYPE_MEM_INFO);
	bLookupFailed |= (IMG_BOOL) (eError != PVRSRV_OK);

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase, &hDummy,
				    psSGXDevInitPart2IN->sInitInfo.
					    hKernelSGXTA3DCtlMemInfo,
				    PVRSRV_HANDLE_TYPE_MEM_INFO);
	bLookupFailed |= (IMG_BOOL) (eError != PVRSRV_OK);

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase, &hDummy,
				    psSGXDevInitPart2IN->sInitInfo.
					    hKernelSGXMiscMemInfo,
				    PVRSRV_HANDLE_TYPE_MEM_INFO);
	bLookupFailed |= (IMG_BOOL) (eError != PVRSRV_OK);

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase, &hDummy,
				    psSGXDevInitPart2IN->sInitInfo.
					    hKernelHWPerfCBMemInfo,
				    PVRSRV_HANDLE_TYPE_MEM_INFO);
	bLookupFailed |= (IMG_BOOL) (eError != PVRSRV_OK);

#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	eError = PVRSRVLookupHandle(psPerProc->psHandleBase, &hDummy,
				    psSGXDevInitPart2IN->sInitInfo.
					    hKernelEDMStatusBufferMemInfo,
				    PVRSRV_HANDLE_TYPE_MEM_INFO);
	bLookupFailed |= (IMG_BOOL) (eError != PVRSRV_OK);
#endif

	for (i = 0; i < SGX_MAX_INIT_MEM_HANDLES; i++) {
		void *hHandle =
		    psSGXDevInitPart2IN->sInitInfo.asInitMemHandles[i];

		if (hHandle == NULL)
			continue;

		eError = PVRSRVLookupHandle(psPerProc->psHandleBase, &hDummy,
					    hHandle,
					    PVRSRV_HANDLE_TYPE_MEM_INFO);
		bLookupFailed |= (IMG_BOOL) (eError != PVRSRV_OK);
	}

	if (bLookupFailed) {
		PVR_DPF(PVR_DBG_ERROR,
			 "DevInitSGXPart2BW: A handle lookup failed");
		psRetOUT->eError = PVRSRV_ERROR_GENERIC;
		return 0;
	}

	eError = PVRSRVLookupAndReleaseHandle(psPerProc->psHandleBase,
					      &psSGXDevInitPart2IN->sInitInfo.
						      hKernelCCBMemInfo,
					      psSGXDevInitPart2IN->sInitInfo.
						      hKernelCCBMemInfo,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
	bReleaseFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

	eError = PVRSRVLookupAndReleaseHandle(psPerProc->psHandleBase,
					      &psSGXDevInitPart2IN->sInitInfo.
						      hKernelCCBCtlMemInfo,
					      psSGXDevInitPart2IN->sInitInfo.
						      hKernelCCBCtlMemInfo,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
	bReleaseFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

	eError = PVRSRVLookupAndReleaseHandle(psPerProc->psHandleBase,
					      &psSGXDevInitPart2IN->sInitInfo.
						  hKernelCCBEventKickerMemInfo,
					      psSGXDevInitPart2IN->sInitInfo.
						  hKernelCCBEventKickerMemInfo,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
	bReleaseFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

	eError = PVRSRVLookupAndReleaseHandle(psPerProc->psHandleBase,
					      &psSGXDevInitPart2IN->sInitInfo.
						  hKernelSGXHostCtlMemInfo,
					      psSGXDevInitPart2IN->sInitInfo.
						  hKernelSGXHostCtlMemInfo,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
	bReleaseFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

	eError = PVRSRVLookupAndReleaseHandle(psPerProc->psHandleBase,
					      &psSGXDevInitPart2IN->sInitInfo.
						  hKernelSGXTA3DCtlMemInfo,
					      psSGXDevInitPart2IN->sInitInfo.
						  hKernelSGXTA3DCtlMemInfo,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
	bReleaseFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

	eError = PVRSRVLookupAndReleaseHandle(psPerProc->psHandleBase,
					      &psSGXDevInitPart2IN->sInitInfo.
						  hKernelSGXMiscMemInfo,
					      psSGXDevInitPart2IN->sInitInfo.
						  hKernelSGXMiscMemInfo,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
	bReleaseFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

	eError = PVRSRVLookupAndReleaseHandle(psPerProc->psHandleBase,
					      &psSGXDevInitPart2IN->sInitInfo.
						  hKernelHWPerfCBMemInfo,
					      psSGXDevInitPart2IN->sInitInfo.
						  hKernelHWPerfCBMemInfo,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
	bReleaseFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	eError = PVRSRVLookupAndReleaseHandle(psPerProc->psHandleBase,
					      &psSGXDevInitPart2IN->sInitInfo.
						  hKernelEDMStatusBufferMemInfo,
					      psSGXDevInitPart2IN->sInitInfo.
						  hKernelEDMStatusBufferMemInfo,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
	bReleaseFailed |= (IMG_BOOL)(eError != PVRSRV_OK);
#endif

	for (i = 0; i < SGX_MAX_INIT_MEM_HANDLES; i++) {
		void **phHandle =
		    &psSGXDevInitPart2IN->sInitInfo.asInitMemHandles[i];

		if (*phHandle == NULL)
			continue;

		eError = PVRSRVLookupAndReleaseHandle(psPerProc->psHandleBase,
					      phHandle, *phHandle,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
		bReleaseFailed |= (IMG_BOOL) (eError != PVRSRV_OK);
	}

	if (bReleaseFailed) {
		PVR_DPF(PVR_DBG_ERROR,
			 "DevInitSGXPart2BW: A handle release failed");
		psRetOUT->eError = PVRSRV_ERROR_GENERIC;

		PVR_DBG_BREAK;
		return 0;
	}

	eError = PVRSRVDissociateDeviceMemKM(hDevCookieInt,
					psSGXDevInitPart2IN->sInitInfo.
						hKernelCCBMemInfo);
	bDissociateFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

	eError = PVRSRVDissociateDeviceMemKM(hDevCookieInt,
					psSGXDevInitPart2IN->sInitInfo.
						hKernelCCBCtlMemInfo);
	bDissociateFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

	eError = PVRSRVDissociateDeviceMemKM(hDevCookieInt,
					psSGXDevInitPart2IN->sInitInfo.
						hKernelCCBEventKickerMemInfo);
	bDissociateFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

	eError = PVRSRVDissociateDeviceMemKM(hDevCookieInt,
					psSGXDevInitPart2IN->sInitInfo.
						hKernelSGXHostCtlMemInfo);
	bDissociateFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

	eError = PVRSRVDissociateDeviceMemKM(hDevCookieInt,
					psSGXDevInitPart2IN->sInitInfo.
						hKernelSGXTA3DCtlMemInfo);
	bDissociateFailed |= (IMG_BOOL)(eError != PVRSRV_OK);

	eError = PVRSRVDissociateDeviceMemKM(hDevCookieInt,
					psSGXDevInitPart2IN->sInitInfo.
						hKernelSGXMiscMemInfo);
	bDissociateFailed |= (IMG_BOOL) (eError != PVRSRV_OK);

	eError = PVRSRVDissociateDeviceMemKM(hDevCookieInt,
					psSGXDevInitPart2IN->sInitInfo.
						hKernelHWPerfCBMemInfo);
	bDissociateFailed |= (IMG_BOOL) (eError != PVRSRV_OK);

#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	eError = PVRSRVDissociateDeviceMemKM(hDevCookieInt,
					psSGXDevInitPart2IN->sInitInfo.
						hKernelEDMStatusBufferMemInfo);
	bDissociateFailed |= (IMG_BOOL) (eError != PVRSRV_OK);
#endif

	for (i = 0; i < SGX_MAX_INIT_MEM_HANDLES; i++) {
		void *hHandle =
		    psSGXDevInitPart2IN->sInitInfo.asInitMemHandles[i];

		if (hHandle == NULL)
			continue;

		eError = PVRSRVDissociateDeviceMemKM(hDevCookieInt, hHandle);
		bDissociateFailed |= (IMG_BOOL) (eError != PVRSRV_OK);
	}

	if (bDissociateFailed) {
		PVRSRVFreeDeviceMemKM(hDevCookieInt,
				      psSGXDevInitPart2IN->sInitInfo.
						hKernelCCBMemInfo);
		PVRSRVFreeDeviceMemKM(hDevCookieInt,
				      psSGXDevInitPart2IN->sInitInfo.
						hKernelCCBCtlMemInfo);
		PVRSRVFreeDeviceMemKM(hDevCookieInt,
				      psSGXDevInitPart2IN->sInitInfo.
						hKernelSGXHostCtlMemInfo);
		PVRSRVFreeDeviceMemKM(hDevCookieInt,
				      psSGXDevInitPart2IN->sInitInfo.
						hKernelSGXTA3DCtlMemInfo);
		PVRSRVFreeDeviceMemKM(hDevCookieInt,
				      psSGXDevInitPart2IN->sInitInfo.
						hKernelSGXMiscMemInfo);

		for (i = 0; i < SGX_MAX_INIT_MEM_HANDLES; i++) {
			void *hHandle =
			    psSGXDevInitPart2IN->sInitInfo.asInitMemHandles[i];

			if (hHandle == NULL)
				continue;

			PVRSRVFreeDeviceMemKM(hDevCookieInt,
					      (struct PVRSRV_KERNEL_MEM_INFO *)
					      hHandle);

		}

		PVR_DPF(PVR_DBG_ERROR,
			 "DevInitSGXPart2BW: A dissociate failed");

		psRetOUT->eError = PVRSRV_ERROR_GENERIC;

		PVR_DBG_BREAK;
		return 0;
	}

	psRetOUT->eError = DevInitSGXPart2KM(psPerProc, hDevCookieInt,
					     &psSGXDevInitPart2IN->sInitInfo);

	return 0;
}

int SGXRegisterHWRenderContextBW(u32 ui32BridgeID,
		struct PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_RENDER_CONTEXT
						*psSGXRegHWRenderContextIN,
		struct PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_RENDER_CONTEXT
						*psSGXRegHWRenderContextOUT,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	void *hHWRenderContextInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_REGISTER_HW_RENDER_CONTEXT);

	NEW_HANDLE_BATCH_OR_ERROR(psSGXRegHWRenderContextOUT->eError, psPerProc,
				  1);

	psSGXRegHWRenderContextOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psSGXRegHWRenderContextIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psSGXRegHWRenderContextOUT->eError != PVRSRV_OK)
		return 0;

	hHWRenderContextInt =
	    SGXRegisterHWRenderContextKM(hDevCookieInt,
			 &psSGXRegHWRenderContextIN->sHWRenderContextDevVAddr,
			 psPerProc);

	if (hHWRenderContextInt == NULL) {
		psSGXRegHWRenderContextOUT->eError = PVRSRV_ERROR_GENERIC;
		return 0;
	}

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			    &psSGXRegHWRenderContextOUT->hHWRenderContext,
			    hHWRenderContextInt,
			    PVRSRV_HANDLE_TYPE_SGX_HW_RENDER_CONTEXT,
			    PVRSRV_HANDLE_ALLOC_FLAG_NONE);

	COMMIT_HANDLE_BATCH_OR_ERROR(psSGXRegHWRenderContextOUT->eError,
				     psPerProc);

	return 0;
}

int SGXUnregisterHWRenderContextBW(u32 ui32BridgeID,
		  struct PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_RENDER_CONTEXT
					  *psSGXUnregHWRenderContextIN,
		  struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hHWRenderContextInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
			 PVRSRV_BRIDGE_SGX_UNREGISTER_HW_RENDER_CONTEXT);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &hHWRenderContextInt,
			       psSGXUnregHWRenderContextIN->hHWRenderContext,
			       PVRSRV_HANDLE_TYPE_SGX_HW_RENDER_CONTEXT);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = SGXUnregisterHWRenderContextKM(hHWRenderContextInt);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVReleaseHandle(psPerProc->psHandleBase,
				psSGXUnregHWRenderContextIN->hHWRenderContext,
				PVRSRV_HANDLE_TYPE_SGX_HW_RENDER_CONTEXT);

	return 0;
}

int SGXRegisterHWTransferContextBW(u32 ui32BridgeID,
		  struct PVRSRV_BRIDGE_IN_SGX_REGISTER_HW_TRANSFER_CONTEXT
					  *psSGXRegHWTransferContextIN,
		  struct PVRSRV_BRIDGE_OUT_SGX_REGISTER_HW_TRANSFER_CONTEXT
					  *psSGXRegHWTransferContextOUT,
		  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	void *hHWTransferContextInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
			 PVRSRV_BRIDGE_SGX_REGISTER_HW_TRANSFER_CONTEXT);

	NEW_HANDLE_BATCH_OR_ERROR(psSGXRegHWTransferContextOUT->eError,
				  psPerProc, 1);

	psSGXRegHWTransferContextOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psSGXRegHWTransferContextIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psSGXRegHWTransferContextOUT->eError != PVRSRV_OK)
		return 0;

	hHWTransferContextInt =
	    SGXRegisterHWTransferContextKM(hDevCookieInt,
					   &psSGXRegHWTransferContextIN->
						   sHWTransferContextDevVAddr,
					   psPerProc);

	if (hHWTransferContextInt == NULL) {
		psSGXRegHWTransferContextOUT->eError = PVRSRV_ERROR_GENERIC;
		return 0;
	}

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			    &psSGXRegHWTransferContextOUT->hHWTransferContext,
			    hHWTransferContextInt,
			    PVRSRV_HANDLE_TYPE_SGX_HW_TRANSFER_CONTEXT,
			    PVRSRV_HANDLE_ALLOC_FLAG_NONE);

	COMMIT_HANDLE_BATCH_OR_ERROR(psSGXRegHWTransferContextOUT->eError,
				     psPerProc);

	return 0;
}

int SGXUnregisterHWTransferContextBW(u32 ui32BridgeID,
		    struct PVRSRV_BRIDGE_IN_SGX_UNREGISTER_HW_TRANSFER_CONTEXT
					    *psSGXUnregHWTransferContextIN,
		    struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hHWTransferContextInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
			 PVRSRV_BRIDGE_SGX_UNREGISTER_HW_TRANSFER_CONTEXT);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &hHWTransferContextInt,
			       psSGXUnregHWTransferContextIN->
						     hHWTransferContext,
			       PVRSRV_HANDLE_TYPE_SGX_HW_TRANSFER_CONTEXT);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
		    SGXUnregisterHWTransferContextKM(hHWTransferContextInt);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVReleaseHandle(psPerProc->psHandleBase,
				psSGXUnregHWTransferContextIN->
							hHWTransferContext,
				PVRSRV_HANDLE_TYPE_SGX_HW_TRANSFER_CONTEXT);

	return 0;
}

int SGXFlushHWRenderTargetBW(u32 ui32BridgeID,
	    struct PVRSRV_BRIDGE_IN_SGX_FLUSH_HW_RENDER_TARGET
				    *psSGXFlushHWRenderTargetIN,
	    struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_FLUSH_HW_RENDER_TARGET);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &hDevCookieInt,
			       psSGXFlushHWRenderTargetIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	SGXFlushHWRenderTargetKM(hDevCookieInt,
			 psSGXFlushHWRenderTargetIN->sHWRTDataSetDevVAddr);

	return 0;
}

int SGX2DQueryBlitsCompleteBW(struct file *filp, u32 ui32BridgeID,
     struct PVRSRV_BRIDGE_IN_2DQUERYBLTSCOMPLETE *ps2DQueryBltsCompleteIN,
     struct PVRSRV_BRIDGE_RETURN *psRetOUT,
     struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	void *pvSyncInfo;
	struct PVRSRV_SGXDEV_INFO *psDevInfo;
	struct PVRSRV_FILE_PRIVATE_DATA *priv = filp->private_data;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_2DQUERYBLTSCOMPLETE);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       ps2DQueryBltsCompleteIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	if (ps2DQueryBltsCompleteIN->type == _PVR_SYNC_WAIT_FLIP) {
		if (pvr_flip_event_req(priv,
				       (long)ps2DQueryBltsCompleteIN->
						       hKernSyncInfo,
				       ps2DQueryBltsCompleteIN->user_data))
			psRetOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;

		return 0;
	}

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &pvSyncInfo,
			       ps2DQueryBltsCompleteIN->hKernSyncInfo,
			       PVRSRV_HANDLE_TYPE_SYNC_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psDevInfo =
	    (struct PVRSRV_SGXDEV_INFO *)((struct PVRSRV_DEVICE_NODE *)
					  hDevCookieInt)->pvDevice;

	if (ps2DQueryBltsCompleteIN->type == _PVR_SYNC_WAIT_EVENT) {
		if (pvr_sync_event_req(priv,
				(struct PVRSRV_KERNEL_SYNC_INFO *)pvSyncInfo,
				ps2DQueryBltsCompleteIN->user_data))
			psRetOUT->eError = PVRSRV_ERROR_OUT_OF_MEMORY;

		return 0;
	}

	psRetOUT->eError =
	    SGX2DQueryBlitsCompleteKM(psDevInfo,
				      (struct PVRSRV_KERNEL_SYNC_INFO *)
							      pvSyncInfo,
			ps2DQueryBltsCompleteIN->type == _PVR_SYNC_WAIT_BLOCK);

	return 0;
}

int SGXFindSharedPBDescBW(u32 ui32BridgeID,
	struct PVRSRV_BRIDGE_IN_SGXFINDSHAREDPBDESC *psSGXFindSharedPBDescIN,
	struct PVRSRV_BRIDGE_OUT_SGXFINDSHAREDPBDESC *psSGXFindSharedPBDescOUT,
	struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	struct PVRSRV_KERNEL_MEM_INFO *psSharedPBDescKernelMemInfo;
	struct PVRSRV_KERNEL_MEM_INFO *psHWPBDescKernelMemInfo;
	struct PVRSRV_KERNEL_MEM_INFO *psBlockKernelMemInfo;
	struct PVRSRV_KERNEL_MEM_INFO **ppsSharedPBDescSubKernelMemInfos = NULL;
	u32 ui32SharedPBDescSubKernelMemInfosCount = 0;
	u32 i;
	void *hSharedPBDesc = NULL;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_FINDSHAREDPBDESC);

	NEW_HANDLE_BATCH_OR_ERROR(psSGXFindSharedPBDescOUT->eError, psPerProc,
				  PVRSRV_BRIDGE_SGX_SHAREDPBDESC_MAX_SUBMEMINFOS
				  + 4);

	psSGXFindSharedPBDescOUT->hSharedPBDesc = NULL;

	psSGXFindSharedPBDescOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &hDevCookieInt,
			       psSGXFindSharedPBDescIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psSGXFindSharedPBDescOUT->eError != PVRSRV_OK)
		goto PVRSRV_BRIDGE_SGX_FINDSHAREDPBDESC_EXIT;

	psSGXFindSharedPBDescOUT->eError =
	    SGXFindSharedPBDescKM(psPerProc, hDevCookieInt,
				  psSGXFindSharedPBDescIN->bLockOnFailure,
				  psSGXFindSharedPBDescIN->ui32TotalPBSize,
				  &hSharedPBDesc,
				  &psSharedPBDescKernelMemInfo,
				  &psHWPBDescKernelMemInfo,
				  &psBlockKernelMemInfo,
				  &ppsSharedPBDescSubKernelMemInfos,
				  &ui32SharedPBDescSubKernelMemInfosCount);
	if (psSGXFindSharedPBDescOUT->eError != PVRSRV_OK)
		goto PVRSRV_BRIDGE_SGX_FINDSHAREDPBDESC_EXIT;

	PVR_ASSERT(ui32SharedPBDescSubKernelMemInfosCount <=
		   PVRSRV_BRIDGE_SGX_SHAREDPBDESC_MAX_SUBMEMINFOS);

	psSGXFindSharedPBDescOUT->ui32SharedPBDescSubKernelMemInfoHandlesCount =
	    ui32SharedPBDescSubKernelMemInfosCount;

	if (hSharedPBDesc == NULL) {
		psSGXFindSharedPBDescOUT->hSharedPBDescKernelMemInfoHandle =
									NULL;

		goto PVRSRV_BRIDGE_SGX_FINDSHAREDPBDESC_EXIT;
	}

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			    &psSGXFindSharedPBDescOUT->hSharedPBDesc,
			    hSharedPBDesc,
			    PVRSRV_HANDLE_TYPE_SHARED_PB_DESC,
			    PVRSRV_HANDLE_ALLOC_FLAG_NONE);

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			       &psSGXFindSharedPBDescOUT->
				       hSharedPBDescKernelMemInfoHandle,
			       psSharedPBDescKernelMemInfo,
			       PVRSRV_HANDLE_TYPE_MEM_INFO_REF,
			       PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
			       psSGXFindSharedPBDescOUT->hSharedPBDesc);

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			       &psSGXFindSharedPBDescOUT->
				       hHWPBDescKernelMemInfoHandle,
			       psHWPBDescKernelMemInfo,
			       PVRSRV_HANDLE_TYPE_MEM_INFO_REF,
			       PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
			       psSGXFindSharedPBDescOUT->hSharedPBDesc);

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			       &psSGXFindSharedPBDescOUT->
				       hBlockKernelMemInfoHandle,
			       psBlockKernelMemInfo,
			       PVRSRV_HANDLE_TYPE_MEM_INFO_REF,
			       PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
			       psSGXFindSharedPBDescOUT->hSharedPBDesc);

	for (i = 0; i < ui32SharedPBDescSubKernelMemInfosCount; i++) {
		struct PVRSRV_BRIDGE_OUT_SGXFINDSHAREDPBDESC
		    *psSGXFindSharedPBDescOut = psSGXFindSharedPBDescOUT;

		PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			       &psSGXFindSharedPBDescOut->
				       ahSharedPBDescSubKernelMemInfoHandles[i],
			       ppsSharedPBDescSubKernelMemInfos[i],
			       PVRSRV_HANDLE_TYPE_MEM_INFO_REF,
			       PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
			       psSGXFindSharedPBDescOUT->
				       hSharedPBDescKernelMemInfoHandle);
	}

PVRSRV_BRIDGE_SGX_FINDSHAREDPBDESC_EXIT:
	if (ppsSharedPBDescSubKernelMemInfos != NULL)
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_KERNEL_MEM_INFO *) *
				  ui32SharedPBDescSubKernelMemInfosCount,
			  ppsSharedPBDescSubKernelMemInfos, NULL);

	if (psSGXFindSharedPBDescOUT->eError != PVRSRV_OK) {
		if (hSharedPBDesc != NULL)
			SGXUnrefSharedPBDescKM(hSharedPBDesc);
	} else
		COMMIT_HANDLE_BATCH_OR_ERROR(psSGXFindSharedPBDescOUT->eError,
					     psPerProc);

	return 0;
}

int SGXUnrefSharedPBDescBW(u32 ui32BridgeID,
	struct PVRSRV_BRIDGE_IN_SGXUNREFSHAREDPBDESC *psSGXUnrefSharedPBDescIN,
	struct PVRSRV_BRIDGE_OUT_SGXUNREFSHAREDPBDESC
						*psSGXUnrefSharedPBDescOUT,
	struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hSharedPBDesc;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_UNREFSHAREDPBDESC);

	psSGXUnrefSharedPBDescOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &hSharedPBDesc,
			       psSGXUnrefSharedPBDescIN->hSharedPBDesc,
			       PVRSRV_HANDLE_TYPE_SHARED_PB_DESC);
	if (psSGXUnrefSharedPBDescOUT->eError != PVRSRV_OK)
		return 0;

	psSGXUnrefSharedPBDescOUT->eError =
	    SGXUnrefSharedPBDescKM(hSharedPBDesc);

	if (psSGXUnrefSharedPBDescOUT->eError != PVRSRV_OK)
		return 0;

	psSGXUnrefSharedPBDescOUT->eError =
	    PVRSRVReleaseHandle(psPerProc->psHandleBase,
				psSGXUnrefSharedPBDescIN->hSharedPBDesc,
				PVRSRV_HANDLE_TYPE_SHARED_PB_DESC);

	return 0;
}

int SGXAddSharedPBDescBW(u32 ui32BridgeID,
	struct PVRSRV_BRIDGE_IN_SGXADDSHAREDPBDESC *psSGXAddSharedPBDescIN,
	struct PVRSRV_BRIDGE_OUT_SGXADDSHAREDPBDESC *psSGXAddSharedPBDescOUT,
	struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	struct PVRSRV_KERNEL_MEM_INFO *psSharedPBDescKernelMemInfo;
	struct PVRSRV_KERNEL_MEM_INFO *psHWPBDescKernelMemInfo;
	struct PVRSRV_KERNEL_MEM_INFO *psBlockKernelMemInfo;
	u32 ui32KernelMemInfoHandlesCount =
	    psSGXAddSharedPBDescIN->ui32KernelMemInfoHandlesCount;
	int ret = 0;
	void **phKernelMemInfoHandles = NULL;
	struct PVRSRV_KERNEL_MEM_INFO **ppsKernelMemInfos = NULL;
	u32 i;
	enum PVRSRV_ERROR eError;
	void *hSharedPBDesc = NULL;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC);

	NEW_HANDLE_BATCH_OR_ERROR(psSGXAddSharedPBDescOUT->eError, psPerProc,
				  1);

	psSGXAddSharedPBDescOUT->hSharedPBDesc = NULL;

	PVR_ASSERT(ui32KernelMemInfoHandlesCount <=
		   PVRSRV_BRIDGE_SGX_SHAREDPBDESC_MAX_SUBMEMINFOS);

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				    &hDevCookieInt,
				    psSGXAddSharedPBDescIN->hDevCookie,
				    PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (eError != PVRSRV_OK)
		goto PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC_RETURN_RESULT;

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				    (void **)&psSharedPBDescKernelMemInfo,
				    psSGXAddSharedPBDescIN->
						    hSharedPBDescKernelMemInfo,
				    PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO);
	if (eError != PVRSRV_OK)
		goto PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC_RETURN_RESULT;

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				    (void **)&psHWPBDescKernelMemInfo,
				    psSGXAddSharedPBDescIN->
						    hHWPBDescKernelMemInfo,
				    PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (eError != PVRSRV_OK)
		goto PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC_RETURN_RESULT;

	eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				    (void **)&psBlockKernelMemInfo,
				    psSGXAddSharedPBDescIN->hBlockKernelMemInfo,
				    PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO);
	if (eError != PVRSRV_OK)
		goto PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC_RETURN_RESULT;

	if (!OSAccessOK(PVR_VERIFY_READ,
			psSGXAddSharedPBDescIN->phKernelMemInfoHandles,
			ui32KernelMemInfoHandlesCount * sizeof(void *))) {
		PVR_DPF(PVR_DBG_ERROR, "%s: PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC:"
			 " Invalid phKernelMemInfos pointer", __func__);
		ret = -EFAULT;
		goto PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC_RETURN_RESULT;
	}

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			    ui32KernelMemInfoHandlesCount * sizeof(void *),
			    (void **)&phKernelMemInfoHandles, NULL);
	if (eError != PVRSRV_OK)
		goto PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC_RETURN_RESULT;

	if (CopyFromUserWrapper(psPerProc,
				ui32BridgeID,
				phKernelMemInfoHandles,
				psSGXAddSharedPBDescIN->phKernelMemInfoHandles,
				ui32KernelMemInfoHandlesCount * sizeof(void *))
	    != PVRSRV_OK) {
		ret = -EFAULT;
		goto PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC_RETURN_RESULT;
	}

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			    ui32KernelMemInfoHandlesCount *
			    sizeof(struct PVRSRV_KERNEL_MEM_INFO *),
			    (void **)&ppsKernelMemInfos, NULL);
	if (eError != PVRSRV_OK)
		goto PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC_RETURN_RESULT;

	for (i = 0; i < ui32KernelMemInfoHandlesCount; i++) {
		eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					    (void **)&ppsKernelMemInfos[i],
					    phKernelMemInfoHandles[i],
					    PVRSRV_HANDLE_TYPE_MEM_INFO);
		if (eError != PVRSRV_OK)
			goto PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC_RETURN_RESULT;
	}

	eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
				     psSGXAddSharedPBDescIN->
						     hSharedPBDescKernelMemInfo,
				     PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO);
	PVR_ASSERT(eError == PVRSRV_OK);

	eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
				     psSGXAddSharedPBDescIN->
						     hHWPBDescKernelMemInfo,
				     PVRSRV_HANDLE_TYPE_MEM_INFO);
	PVR_ASSERT(eError == PVRSRV_OK);

	eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
				     psSGXAddSharedPBDescIN->
						     hBlockKernelMemInfo,
				     PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO);
	PVR_ASSERT(eError == PVRSRV_OK);

	for (i = 0; i < ui32KernelMemInfoHandlesCount; i++) {
		eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
					     phKernelMemInfoHandles[i],
					     PVRSRV_HANDLE_TYPE_MEM_INFO);
		PVR_ASSERT(eError == PVRSRV_OK);
	}

	eError = SGXAddSharedPBDescKM(psPerProc, hDevCookieInt,
				      psSharedPBDescKernelMemInfo,
				      psHWPBDescKernelMemInfo,
				      psBlockKernelMemInfo,
				      psSGXAddSharedPBDescIN->ui32TotalPBSize,
				      &hSharedPBDesc,
				      ppsKernelMemInfos,
				      ui32KernelMemInfoHandlesCount);

	if (eError != PVRSRV_OK)
		goto PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC_RETURN_RESULT;

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			    &psSGXAddSharedPBDescOUT->hSharedPBDesc,
			    hSharedPBDesc,
			    PVRSRV_HANDLE_TYPE_SHARED_PB_DESC,
			    PVRSRV_HANDLE_ALLOC_FLAG_NONE);

PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC_RETURN_RESULT:

	if (phKernelMemInfoHandles)
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  psSGXAddSharedPBDescIN->ui32KernelMemInfoHandlesCount
							  * sizeof(void *),
			  (void *)phKernelMemInfoHandles, NULL);
	if (ppsKernelMemInfos)
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  psSGXAddSharedPBDescIN->ui32KernelMemInfoHandlesCount
				  * sizeof(struct PVRSRV_KERNEL_MEM_INFO *),
			  (void *)ppsKernelMemInfos, NULL);

	if (ret == 0 && eError == PVRSRV_OK)
		COMMIT_HANDLE_BATCH_OR_ERROR(psSGXAddSharedPBDescOUT->eError,
					     psPerProc);

	psSGXAddSharedPBDescOUT->eError = eError;

	return ret;
}

int SGXGetInfoForSrvinitBW(u32 ui32BridgeID,
	  struct PVRSRV_BRIDGE_IN_SGXINFO_FOR_SRVINIT *psSGXInfoForSrvinitIN,
	  struct PVRSRV_BRIDGE_OUT_SGXINFO_FOR_SRVINIT *psSGXInfoForSrvinitOUT,
	  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	u32 i;
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGXINFO_FOR_SRVINIT);

	NEW_HANDLE_BATCH_OR_ERROR(psSGXInfoForSrvinitOUT->eError, psPerProc,
				  PVRSRV_MAX_CLIENT_HEAPS);

	if (!psPerProc->bInitProcess) {
		psSGXInfoForSrvinitOUT->eError = PVRSRV_ERROR_GENERIC;
		return 0;
	}

	psSGXInfoForSrvinitOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psSGXInfoForSrvinitIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psSGXInfoForSrvinitOUT->eError != PVRSRV_OK)
		return 0;

	psSGXInfoForSrvinitOUT->eError =
	    SGXGetInfoForSrvinitKM(hDevCookieInt,
				   &psSGXInfoForSrvinitOUT->sInitInfo);

	if (psSGXInfoForSrvinitOUT->eError != PVRSRV_OK)
		return 0;

	for (i = 0; i < PVRSRV_MAX_CLIENT_HEAPS; i++) {
		struct PVRSRV_HEAP_INFO *psHeapInfo;

		psHeapInfo = &psSGXInfoForSrvinitOUT->sInitInfo.asHeapInfo[i];

		if (psHeapInfo->ui32HeapID != (u32)SGX_UNDEFINED_HEAP_ID) {
			void *hDevMemHeapExt;

			if (psHeapInfo->hDevMemHeap != NULL) {

				PVRSRVAllocHandleNR(psPerProc->psHandleBase,
					    &hDevMemHeapExt,
					    psHeapInfo->hDevMemHeap,
					    PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP,
					    PVRSRV_HANDLE_ALLOC_FLAG_SHARED);
				psHeapInfo->hDevMemHeap = hDevMemHeapExt;
			}
		}
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psSGXInfoForSrvinitOUT->eError, psPerProc);

	return 0;
}

#if defined(PDUMP)
static void DumpBufferArray(struct PVRSRV_PER_PROCESS_DATA *psPerProc,
			    struct SGX_KICKTA_DUMP_BUFFER *psBufferArray,
			    u32 ui32BufferArrayLength, IMG_BOOL bDumpPolls)
{
	u32 i;

	for (i = 0; i < ui32BufferArrayLength; i++) {
		struct SGX_KICKTA_DUMP_BUFFER *psBuffer;
		struct PVRSRV_KERNEL_MEM_INFO *psCtrlMemInfoKM;
		char *pszName;
		void *hUniqueTag;
		u32 ui32Offset;

		psBuffer = &psBufferArray[i];
		pszName = psBuffer->pszName;
		if (!pszName)
			pszName = "Nameless buffer";

		hUniqueTag =
		    MAKEUNIQUETAG((struct PVRSRV_KERNEL_MEM_INFO *)psBuffer->
				  hKernelMemInfo);

		psCtrlMemInfoKM =
		    ((struct PVRSRV_KERNEL_MEM_INFO *)psBuffer->
		     hKernelMemInfo)->psKernelSyncInfo->psSyncDataMemInfoKM;
		ui32Offset =
		    offsetof(struct PVRSRV_SYNC_DATA, ui32ReadOpsComplete);

		if (psBuffer->ui32Start <= psBuffer->ui32End) {
			if (bDumpPolls) {
				PDUMPCOMMENTWITHFLAGS(0,
						      "Wait for %s space\r\n",
						      pszName);
				PDUMPCBP(psCtrlMemInfoKM, ui32Offset,
					 psBuffer->ui32Start,
					 psBuffer->ui32SpaceUsed,
					 psBuffer->ui32BufferSize, 0,
					 MAKEUNIQUETAG(psCtrlMemInfoKM));
			}

			PDUMPCOMMENTWITHFLAGS(0, "%s\r\n", pszName);
			PDUMPMEMUM(psPerProc,
				   NULL, psBuffer->pvLinAddr,
				   (struct PVRSRV_KERNEL_MEM_INFO *)psBuffer->
							   hKernelMemInfo,
				   psBuffer->ui32Start,
				   psBuffer->ui32End - psBuffer->ui32Start, 0,
				   hUniqueTag);
		} else {

			if (bDumpPolls) {
				PDUMPCOMMENTWITHFLAGS(0,
						      "Wait for %s space\r\n",
						      pszName);
				PDUMPCBP(psCtrlMemInfoKM, ui32Offset,
					 psBuffer->ui32Start,
					 psBuffer->ui32BackEndLength,
					 psBuffer->ui32BufferSize, 0,
					 MAKEUNIQUETAG(psCtrlMemInfoKM));
			}
			PDUMPCOMMENTWITHFLAGS(0, "%s (part 1)\r\n", pszName);
			PDUMPMEMUM(psPerProc,
				   NULL, psBuffer->pvLinAddr,
				   (struct PVRSRV_KERNEL_MEM_INFO *)psBuffer->
							   hKernelMemInfo,
				   psBuffer->ui32Start,
				   psBuffer->ui32BackEndLength, 0, hUniqueTag);

			if (bDumpPolls) {
				PDUMPMEMPOL(psCtrlMemInfoKM, ui32Offset,
					    0, 0xFFFFFFFF,
					    PDUMP_POLL_OPERATOR_NOTEQUAL,
					    IMG_FALSE, IMG_FALSE,
					    MAKEUNIQUETAG(psCtrlMemInfoKM));

				PDUMPCOMMENTWITHFLAGS(0,
						      "Wait for %s space\r\n",
						      pszName);
				PDUMPCBP(psCtrlMemInfoKM, ui32Offset, 0,
					 psBuffer->ui32End,
					 psBuffer->ui32BufferSize, 0,
					 MAKEUNIQUETAG(psCtrlMemInfoKM));
			}
			PDUMPCOMMENTWITHFLAGS(0, "%s (part 2)\r\n", pszName);
			PDUMPMEMUM(psPerProc, NULL, psBuffer->pvLinAddr,
				   (struct PVRSRV_KERNEL_MEM_INFO *)psBuffer->
						   hKernelMemInfo,
				   0, psBuffer->ui32End, 0, hUniqueTag);
		}
	}
}

int SGXPDumpBufferArrayBW(u32 ui32BridgeID,
	 struct PVRSRV_BRIDGE_IN_PDUMP_BUFFER_ARRAY *psPDumpBufferArrayIN,
	 void *psBridgeOut, struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	u32 i;
	struct SGX_KICKTA_DUMP_BUFFER *psKickTADumpBuffer;
	u32 ui32BufferArrayLength = psPDumpBufferArrayIN->ui32BufferArrayLength;
	u32 ui32BufferArraySize =
	    ui32BufferArrayLength * sizeof(struct SGX_KICKTA_DUMP_BUFFER);
	enum PVRSRV_ERROR eError = PVRSRV_ERROR_GENERIC;

	PVR_UNREFERENCED_PARAMETER(psBridgeOut);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_PDUMP_BUFFER_ARRAY);

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, ui32BufferArraySize,
		       (void **)&psKickTADumpBuffer, NULL) != PVRSRV_OK)
		return -ENOMEM;

	if (CopyFromUserWrapper(psPerProc, ui32BridgeID, psKickTADumpBuffer,
				psPDumpBufferArrayIN->psBufferArray,
				ui32BufferArraySize) != PVRSRV_OK) {
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, ui32BufferArraySize,
			  psKickTADumpBuffer, NULL);
		return -EFAULT;
	}

	for (i = 0; i < ui32BufferArrayLength; i++) {
		void *pvMemInfo;

		eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					    &pvMemInfo,
					    psKickTADumpBuffer[i].
							    hKernelMemInfo,
					    PVRSRV_HANDLE_TYPE_MEM_INFO);

		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				 "PVRSRV_BRIDGE_SGX_PDUMP_BUFFER_ARRAY: "
				 "PVRSRVLookupHandle failed (%d)", eError);
			break;
		}
		psKickTADumpBuffer[i].hKernelMemInfo = pvMemInfo;

	}

	if (eError == PVRSRV_OK)
		DumpBufferArray(psPerProc, psKickTADumpBuffer,
				ui32BufferArrayLength,
				psPDumpBufferArrayIN->bDumpPolls);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, ui32BufferArraySize,
		  psKickTADumpBuffer, NULL);

	return 0;
}

int SGXPDump3DSignatureRegistersBW(u32 ui32BridgeID,
		  struct PVRSRV_BRIDGE_IN_PDUMP_3D_SIGNATURE_REGISTERS
					  *psPDump3DSignatureRegistersIN,
		  void *psBridgeOut, struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	u32 ui32RegisterArraySize =
	    psPDump3DSignatureRegistersIN->ui32NumRegisters * sizeof(u32);
	u32 *pui32Registers = NULL;
	int ret = -EFAULT;

	PVR_UNREFERENCED_PARAMETER(psBridgeOut);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
			 PVRSRV_BRIDGE_SGX_PDUMP_3D_SIGNATURE_REGISTERS);

	if (ui32RegisterArraySize == 0)
		goto ExitNoError;

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       ui32RegisterArraySize,
		       (void **)&pui32Registers, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PDump3DSignatureRegistersBW: OSAllocMem failed");
		goto Exit;
	}

	if (CopyFromUserWrapper(psPerProc, ui32BridgeID, pui32Registers,
				psPDump3DSignatureRegistersIN->pui32Registers,
				ui32RegisterArraySize) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PDump3DSignatureRegistersBW: "
					"CopyFromUserWrapper failed");
		goto Exit;
	}

	PDump3DSignatureRegisters(psPDump3DSignatureRegistersIN->
							  ui32DumpFrameNum,
				  psPDump3DSignatureRegistersIN->bLastFrame,
				  pui32Registers,
				  psPDump3DSignatureRegistersIN->
							  ui32NumRegisters);

ExitNoError:
	ret = 0;
Exit:
	if (pui32Registers != NULL)
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, ui32RegisterArraySize,
			  pui32Registers, NULL);

	return ret;
}

int SGXPDumpCounterRegistersBW(u32 ui32BridgeID,
	      struct PVRSRV_BRIDGE_IN_PDUMP_COUNTER_REGISTERS
				      *psPDumpCounterRegistersIN,
	      void *psBridgeOut, struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	u32 ui32RegisterArraySize =
	    psPDumpCounterRegistersIN->ui32NumRegisters * sizeof(u32);
	u32 *pui32Registers = NULL;
	int ret = -EFAULT;

	PVR_UNREFERENCED_PARAMETER(psBridgeOut);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_PDUMP_COUNTER_REGISTERS);

	if (ui32RegisterArraySize == 0)
		goto ExitNoError;

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, ui32RegisterArraySize,
		       (void **)&pui32Registers, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PDumpCounterRegistersBW: OSAllocMem failed");
		ret = -ENOMEM;
		goto Exit;
	}

	if (CopyFromUserWrapper(psPerProc, ui32BridgeID, pui32Registers,
				psPDumpCounterRegistersIN->pui32Registers,
				ui32RegisterArraySize) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PDumpCounterRegistersBW: CopyFromUserWrapper failed");
		goto Exit;
	}

	PDumpCounterRegisters(psPDumpCounterRegistersIN->ui32DumpFrameNum,
			      psPDumpCounterRegistersIN->bLastFrame,
			      pui32Registers,
			      psPDumpCounterRegistersIN->ui32NumRegisters);

ExitNoError:
	ret = 0;
Exit:
	if (pui32Registers != NULL)
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, ui32RegisterArraySize,
			  pui32Registers, NULL);

	return ret;
}

int SGXPDumpTASignatureRegistersBW(u32 ui32BridgeID,
	  struct PVRSRV_BRIDGE_IN_PDUMP_TA_SIGNATURE_REGISTERS
					  *psPDumpTASignatureRegistersIN,
	  void *psBridgeOut, struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	u32 ui32RegisterArraySize =
	    psPDumpTASignatureRegistersIN->ui32NumRegisters * sizeof(u32);
	u32 *pui32Registers = NULL;
	int ret = -EFAULT;

	PVR_UNREFERENCED_PARAMETER(psBridgeOut);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
			 PVRSRV_BRIDGE_SGX_PDUMP_TA_SIGNATURE_REGISTERS);

	if (ui32RegisterArraySize == 0)
		goto ExitNoError;

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       ui32RegisterArraySize,
		       (void **)&pui32Registers, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PDumpTASignatureRegistersBW: OSAllocMem failed");
		ret = -ENOMEM;
		goto Exit;
	}

	if (CopyFromUserWrapper(psPerProc, ui32BridgeID, pui32Registers,
				psPDumpTASignatureRegistersIN->pui32Registers,
				ui32RegisterArraySize) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PDumpTASignatureRegistersBW: "
					"CopyFromUserWrapper failed");
		goto Exit;
	}

	PDumpTASignatureRegisters(psPDumpTASignatureRegistersIN->
							  ui32DumpFrameNum,
				  psPDumpTASignatureRegistersIN->
							  ui32TAKickCount,
				  psPDumpTASignatureRegistersIN->bLastFrame,
				  pui32Registers,
				  psPDumpTASignatureRegistersIN->
							  ui32NumRegisters);

ExitNoError:
	ret = 0;
Exit:
	if (pui32Registers != NULL)
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, ui32RegisterArraySize,
			  pui32Registers, NULL);

	return ret;
}

int SGXPDumpHWPerfCBBW(u32 ui32BridgeID,
		struct PVRSRV_BRIDGE_IN_PDUMP_HWPERFCB *psPDumpHWPerfCBIN,
		struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	struct PVRSRV_SGXDEV_INFO *psDevInfo;
	void *hDevCookieInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SGX_PDUMP_HWPERFCB);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psPDumpHWPerfCBIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psDevInfo = ((struct PVRSRV_DEVICE_NODE *)hDevCookieInt)->pvDevice;

	PDumpHWPerfCBKM(&psPDumpHWPerfCBIN->szFileName[0],
			psPDumpHWPerfCBIN->ui32FileOffset,
			psDevInfo->psKernelHWPerfCBMemInfo->sDevVAddr,
			psDevInfo->psKernelHWPerfCBMemInfo->ui32AllocSize,
			psPDumpHWPerfCBIN->ui32PDumpFlags);

	return 0;
}

#endif

void SetSGXDispatchTableEntry(void)
{

	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_GETCLIENTINFO,
			      SGXGetClientInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_RELEASECLIENTINFO,
			      SGXReleaseClientInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_GETINTERNALDEVINFO,
			      SGXGetInternalDevInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_DOKICK, SGXDoKickBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_GETPHYSPAGEADDR, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_READREGISTRYDWORD, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_SCHEDULECOMMAND, DummyBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_2DQUERYBLTSCOMPLETE,
			      SGX2DQueryBlitsCompleteBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_GETMMUPDADDR, DummyBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_SUBMITTRANSFER,
			      SGXSubmitTransferBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_GETMISCINFO, SGXGetMiscInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGXINFO_FOR_SRVINIT,
			      SGXGetInfoForSrvinitBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_DEVINITPART2,
			      SGXDevInitPart2BW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_FINDSHAREDPBDESC,
			      SGXFindSharedPBDescBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_UNREFSHAREDPBDESC,
			      SGXUnrefSharedPBDescBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC,
			      SGXAddSharedPBDescBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_REGISTER_HW_RENDER_CONTEXT,
			      SGXRegisterHWRenderContextBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_FLUSH_HW_RENDER_TARGET,
			      SGXFlushHWRenderTargetBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_UNREGISTER_HW_RENDER_CONTEXT,
			      SGXUnregisterHWRenderContextBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_REGISTER_HW_TRANSFER_CONTEXT,
			      SGXRegisterHWTransferContextBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_UNREGISTER_HW_TRANSFER_CONTEXT,
			      SGXUnregisterHWTransferContextBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_READ_DIFF_COUNTERS,
			      SGXReadDiffCountersBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_READ_HWPERF_CB,
			      SGXReadHWPerfCBBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_SCHEDULE_PROCESS_QUEUES,
			      SGXScheduleProcessQueuesBW);

#if defined(PDUMP)
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_PDUMP_BUFFER_ARRAY,
			      SGXPDumpBufferArrayBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_PDUMP_3D_SIGNATURE_REGISTERS,
			      SGXPDump3DSignatureRegistersBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_PDUMP_COUNTER_REGISTERS,
			      SGXPDumpCounterRegistersBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_PDUMP_TA_SIGNATURE_REGISTERS,
			      SGXPDumpTASignatureRegistersBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SGX_PDUMP_HWPERFCB,
			      SGXPDumpHWPerfCBBW);
#endif
}
