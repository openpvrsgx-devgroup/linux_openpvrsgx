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

#include "img_defs.h"
#include "services.h"
#include "pvr_bridge_km.h"
#include "pvr_debug.h"
#include "ra.h"
#include "pvr_bridge.h"
#include "sgx_bridge.h"
#include "perproc.h"
#include "device.h"
#include "buffer_manager.h"

#include "pdump_km.h"
#include "syscommon.h"

#include "bridged_pvr_bridge.h"
#include "bridged_sgx_bridge.h"
#include "env_data.h"

#include "mmap.h"

#include <linux/kernel.h>
#include <linux/pagemap.h>	/* for cache flush */
#include <linux/mm.h>
#include <linux/sched.h>

struct PVRSRV_BRIDGE_DISPATCH_TABLE_ENTRY
    g_BridgeDispatchTable[BRIDGE_DISPATCH_TABLE_ENTRY_COUNT];

#if defined(DEBUG_BRIDGE_KM)
struct PVRSRV_BRIDGE_GLOBAL_STATS g_BridgeGlobalStats;
#endif

static IMG_BOOL abSharedDeviceMemHeap[PVRSRV_MAX_CLIENT_HEAPS];
static IMG_BOOL *pbSharedDeviceMemHeap = abSharedDeviceMemHeap;

#if defined(DEBUG_BRIDGE_KM)
enum PVRSRV_ERROR
CopyFromUserWrapper(struct PVRSRV_PER_PROCESS_DATA *pProcData,
		    u32 ui32BridgeID, void *pvDest, void __user *pvSrc,
		    u32 ui32Size)
{
	g_BridgeDispatchTable[ui32BridgeID].ui32CopyFromUserTotalBytes +=
	    ui32Size;
	g_BridgeGlobalStats.ui32TotalCopyFromUserBytes += ui32Size;
	return OSCopyFromUser(pProcData, pvDest, pvSrc, ui32Size);
}

enum PVRSRV_ERROR CopyToUserWrapper(struct PVRSRV_PER_PROCESS_DATA *pProcData,
		  u32 ui32BridgeID, void __user *pvDest, void *pvSrc,
		  u32 ui32Size)
{
	g_BridgeDispatchTable[ui32BridgeID].ui32CopyToUserTotalBytes +=
	    ui32Size;
	g_BridgeGlobalStats.ui32TotalCopyToUserBytes += ui32Size;
	return OSCopyToUser(pProcData, pvDest, pvSrc, ui32Size);
}
#endif

static int PVRSRVEnumerateDevicesBW(u32 ui32BridgeID, void *psBridgeIn,
			 struct PVRSRV_BRIDGE_OUT_ENUMDEVICE *psEnumDeviceOUT,
			 struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_ENUM_DEVICES);

	PVR_UNREFERENCED_PARAMETER(psPerProc);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);

	psEnumDeviceOUT->eError =
	    PVRSRVEnumerateDevicesKM(&psEnumDeviceOUT->ui32NumDevices,
				     psEnumDeviceOUT->asDeviceIdentifier);

	return 0;
}

static int PVRSRVAcquireDeviceDataBW(u32 ui32BridgeID,
	struct PVRSRV_BRIDGE_IN_ACQUIRE_DEVICEINFO *psAcquireDevInfoIN,
	struct PVRSRV_BRIDGE_OUT_ACQUIRE_DEVICEINFO *psAcquireDevInfoOUT,
	struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_ACQUIRE_DEVICEINFO);

	psAcquireDevInfoOUT->eError =
	    PVRSRVAcquireDeviceDataKM(psAcquireDevInfoIN->uiDevIndex,
				      psAcquireDevInfoIN->eDeviceType,
				      &hDevCookieInt);
	if (psAcquireDevInfoOUT->eError != PVRSRV_OK)
		return 0;

	psAcquireDevInfoOUT->eError = PVRSRVAllocHandle(psPerProc->psHandleBase,
					      &psAcquireDevInfoOUT->hDevCookie,
					      hDevCookieInt,
					      PVRSRV_HANDLE_TYPE_DEV_NODE,
					      PVRSRV_HANDLE_ALLOC_FLAG_SHARED);

	return 0;
}

static int PVRSRVCreateDeviceMemContextBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_CREATE_DEVMEMCONTEXT *psCreateDevMemContextIN,
       struct PVRSRV_BRIDGE_OUT_CREATE_DEVMEMCONTEXT *psCreateDevMemContextOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	void *hDevMemContextInt;
	u32 i;
	IMG_BOOL bCreated;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
			PVRSRV_BRIDGE_CREATE_DEVMEMCONTEXT);

	NEW_HANDLE_BATCH_OR_ERROR(psCreateDevMemContextOUT->eError, psPerProc,
			PVRSRV_MAX_CLIENT_HEAPS + 1);

	psCreateDevMemContextOUT->eError = PVRSRVLookupHandle(
				psPerProc->psHandleBase, &hDevCookieInt,
				psCreateDevMemContextIN->hDevCookie,
				PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psCreateDevMemContextOUT->eError != PVRSRV_OK)
		return 0;

	psCreateDevMemContextOUT->eError = PVRSRVCreateDeviceMemContextKM(
				hDevCookieInt, psPerProc,
				&hDevMemContextInt,
				&psCreateDevMemContextOUT->ui32ClientHeapCount,
				&psCreateDevMemContextOUT->sHeapInfo[0],
				&bCreated, pbSharedDeviceMemHeap);

	if (psCreateDevMemContextOUT->eError != PVRSRV_OK)
		return 0;

	if (bCreated) {
		PVRSRVAllocHandleNR(psPerProc->psHandleBase,
				&psCreateDevMemContextOUT->hDevMemContext,
				hDevMemContextInt,
				PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT,
				PVRSRV_HANDLE_ALLOC_FLAG_NONE);
	} else {
		psCreateDevMemContextOUT->eError =
			PVRSRVFindHandle(psPerProc->psHandleBase,
				&psCreateDevMemContextOUT->hDevMemContext,
				hDevMemContextInt,
				PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);
		if (psCreateDevMemContextOUT->eError != PVRSRV_OK)
			return 0;
	}

	for (i = 0; i < psCreateDevMemContextOUT->ui32ClientHeapCount; i++) {
		void *hDevMemHeapExt;

		if (abSharedDeviceMemHeap[i]) {
			PVRSRVAllocHandleNR(psPerProc->psHandleBase,
					&hDevMemHeapExt,
					psCreateDevMemContextOUT->
					sHeapInfo[i].hDevMemHeap,
					PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP,
					PVRSRV_HANDLE_ALLOC_FLAG_SHARED);
		} else {
			if (bCreated) {
				PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
					&hDevMemHeapExt,
					psCreateDevMemContextOUT->sHeapInfo[i].
								hDevMemHeap,
					PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP,
					PVRSRV_HANDLE_ALLOC_FLAG_NONE,
					psCreateDevMemContextOUT->
								hDevMemContext);
			} else {
				psCreateDevMemContextOUT->eError =
					PVRSRVFindHandle(
					    psPerProc->psHandleBase,
					    &hDevMemHeapExt,
					    psCreateDevMemContextOUT->
					    sHeapInfo[i].hDevMemHeap,
					    PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP);
				if (psCreateDevMemContextOUT->eError !=
						PVRSRV_OK)
					return 0;
			}
		}
		psCreateDevMemContextOUT->sHeapInfo[i].hDevMemHeap =
								hDevMemHeapExt;
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psCreateDevMemContextOUT->eError,
								psPerProc);

	return 0;
}

static int PVRSRVDestroyDeviceMemContextBW(u32 ui32BridgeID,
	struct PVRSRV_BRIDGE_IN_DESTROY_DEVMEMCONTEXT *psDestroyDevMemContextIN,
	struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	void *hDevMemContextInt;
	IMG_BOOL bDestroyed;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
			PVRSRV_BRIDGE_DESTROY_DEVMEMCONTEXT);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				&hDevCookieInt,
				psDestroyDevMemContextIN->hDevCookie,
				PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				&hDevMemContextInt,
				psDestroyDevMemContextIN->hDevMemContext,
				PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVDestroyDeviceMemContextKM(hDevCookieInt,
						hDevMemContextInt, &bDestroyed);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	if (bDestroyed)
		psRetOUT->eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
					psDestroyDevMemContextIN->
					hDevMemContext,
					PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);

	return 0;
}

static int PVRSRVGetDeviceMemHeapInfoBW(u32 ui32BridgeID,
	   struct PVRSRV_BRIDGE_IN_GET_DEVMEM_HEAPINFO *psGetDevMemHeapInfoIN,
	   struct PVRSRV_BRIDGE_OUT_GET_DEVMEM_HEAPINFO *psGetDevMemHeapInfoOUT,
	   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	void *hDevMemContextInt;
	u32 i;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_GET_DEVMEM_HEAPINFO);

	NEW_HANDLE_BATCH_OR_ERROR(psGetDevMemHeapInfoOUT->eError, psPerProc,
				  PVRSRV_MAX_CLIENT_HEAPS);

	psGetDevMemHeapInfoOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psGetDevMemHeapInfoIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psGetDevMemHeapInfoOUT->eError != PVRSRV_OK)
		return 0;

	psGetDevMemHeapInfoOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevMemContextInt,
			       psGetDevMemHeapInfoIN->hDevMemContext,
			       PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);

	if (psGetDevMemHeapInfoOUT->eError != PVRSRV_OK)
		return 0;

	psGetDevMemHeapInfoOUT->eError =
	    PVRSRVGetDeviceMemHeapInfoKM(hDevCookieInt,
				 hDevMemContextInt,
				 &psGetDevMemHeapInfoOUT->ui32ClientHeapCount,
				 &psGetDevMemHeapInfoOUT->sHeapInfo[0],
					 pbSharedDeviceMemHeap);

	if (psGetDevMemHeapInfoOUT->eError != PVRSRV_OK)
		return 0;

	for (i = 0; i < psGetDevMemHeapInfoOUT->ui32ClientHeapCount; i++) {
		void *hDevMemHeapExt;
		if (abSharedDeviceMemHeap[i]) {
			PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			       &hDevMemHeapExt,
			       psGetDevMemHeapInfoOUT->sHeapInfo[i].hDevMemHeap,
			       PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP,
			       PVRSRV_HANDLE_ALLOC_FLAG_SHARED);
		} else {
			psGetDevMemHeapInfoOUT->eError =
			    PVRSRVFindHandle(psPerProc->psHandleBase,
					     &hDevMemHeapExt,
					     psGetDevMemHeapInfoOUT->
					     sHeapInfo[i].hDevMemHeap,
					     PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP);
			if (psGetDevMemHeapInfoOUT->eError != PVRSRV_OK)
				return 0;
		}
		psGetDevMemHeapInfoOUT->sHeapInfo[i].hDevMemHeap =
		    hDevMemHeapExt;
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psGetDevMemHeapInfoOUT->eError, psPerProc);

	return 0;
}

static int PVRSRVAllocDeviceMemBW(u32 ui32BridgeID,
	       struct PVRSRV_BRIDGE_IN_ALLOCDEVICEMEM *psAllocDeviceMemIN,
	       struct PVRSRV_BRIDGE_OUT_ALLOCDEVICEMEM *psAllocDeviceMemOUT,
	       struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	struct PVRSRV_KERNEL_MEM_INFO *psMemInfo;
	void *hDevCookieInt;
	void *hDevMemHeapInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_ALLOC_DEVICEMEM);

	NEW_HANDLE_BATCH_OR_ERROR(psAllocDeviceMemOUT->eError, psPerProc, 2);

	psAllocDeviceMemOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psAllocDeviceMemIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psAllocDeviceMemOUT->eError != PVRSRV_OK)
		return 0;

	psAllocDeviceMemOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevMemHeapInt,
			       psAllocDeviceMemIN->hDevMemHeap,
			       PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP);

	if (psAllocDeviceMemOUT->eError != PVRSRV_OK)
		return 0;

	psAllocDeviceMemOUT->eError =
	    PVRSRVAllocDeviceMemKM(hDevCookieInt, psPerProc, hDevMemHeapInt,
				   psAllocDeviceMemIN->ui32Attribs,
				   psAllocDeviceMemIN->ui32Size,
				   psAllocDeviceMemIN->ui32Alignment,
				   &psMemInfo);

	if (psAllocDeviceMemOUT->eError != PVRSRV_OK)
		return 0;

	OSMemSet(&psAllocDeviceMemOUT->sClientMemInfo, 0,
		 sizeof(psAllocDeviceMemOUT->sClientMemInfo));

	psAllocDeviceMemOUT->sClientMemInfo.pvLinAddrKM =
		    psMemInfo->pvLinAddrKM;

	psAllocDeviceMemOUT->sClientMemInfo.pvLinAddr = NULL;
	psAllocDeviceMemOUT->sClientMemInfo.sDevVAddr = psMemInfo->sDevVAddr;
	psAllocDeviceMemOUT->sClientMemInfo.ui32Flags = psMemInfo->ui32Flags;
	psAllocDeviceMemOUT->sClientMemInfo.ui32AllocSize =
						    psMemInfo->ui32AllocSize;
	psAllocDeviceMemOUT->sClientMemInfo.hMappingInfo =
					    psMemInfo->sMemBlk.hOSMemHandle;

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			    &psAllocDeviceMemOUT->sClientMemInfo.hKernelMemInfo,
			    psMemInfo,
			    PVRSRV_HANDLE_TYPE_MEM_INFO,
			    PVRSRV_HANDLE_ALLOC_FLAG_NONE);

	if (psAllocDeviceMemIN->ui32Attribs & PVRSRV_MEM_NO_SYNCOBJ) {
		OSMemSet(&psAllocDeviceMemOUT->sClientSyncInfo, 0,
			 sizeof(struct PVRSRV_CLIENT_SYNC_INFO));
		psAllocDeviceMemOUT->sClientMemInfo.psClientSyncInfo = NULL;
		psAllocDeviceMemOUT->psKernelSyncInfo = NULL;
	} else {

		psAllocDeviceMemOUT->psKernelSyncInfo =
					    psMemInfo->psKernelSyncInfo;

		psAllocDeviceMemOUT->sClientSyncInfo.psSyncData =
		    psMemInfo->psKernelSyncInfo->psSyncData;
		psAllocDeviceMemOUT->sClientSyncInfo.sWriteOpsCompleteDevVAddr =
		    psMemInfo->psKernelSyncInfo->sWriteOpsCompleteDevVAddr;
		psAllocDeviceMemOUT->sClientSyncInfo.sReadOpsCompleteDevVAddr =
		    psMemInfo->psKernelSyncInfo->sReadOpsCompleteDevVAddr;

		psAllocDeviceMemOUT->sClientSyncInfo.hMappingInfo =
		    psMemInfo->psKernelSyncInfo->psSyncDataMemInfoKM->sMemBlk.
								hOSMemHandle;

		PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
				       &psAllocDeviceMemOUT->sClientSyncInfo.
				       hKernelSyncInfo,
				       psMemInfo->psKernelSyncInfo,
				       PVRSRV_HANDLE_TYPE_SYNC_INFO,
				       PVRSRV_HANDLE_ALLOC_FLAG_NONE,
				       psAllocDeviceMemOUT->sClientMemInfo.
				       hKernelMemInfo);

		psAllocDeviceMemOUT->sClientMemInfo.psClientSyncInfo =
		    &psAllocDeviceMemOUT->sClientSyncInfo;

	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psAllocDeviceMemOUT->eError, psPerProc);

	return 0;
}


static int PVRSRVFreeDeviceMemBW(u32 ui32BridgeID,
		      struct PVRSRV_BRIDGE_IN_FREEDEVICEMEM *psFreeDeviceMemIN,
		      struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	void *pvKernelMemInfo;
	struct PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_FREE_DEVICEMEM);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psFreeDeviceMemIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &pvKernelMemInfo,
			       psFreeDeviceMemIN->psKernelMemInfo,
			       PVRSRV_HANDLE_TYPE_MEM_INFO);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psKernelMemInfo = (struct PVRSRV_KERNEL_MEM_INFO *)pvKernelMemInfo;
	if (psKernelMemInfo->ui32RefCount != 1) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVFreeDeviceMemBW: "
					"mappings are open in other processes");
		psRetOUT->eError = PVRSRV_ERROR_GENERIC;
		return 0;
	}

	psRetOUT->eError = PVRSRVFreeDeviceMemKM(hDevCookieInt,
						 pvKernelMemInfo);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
					psFreeDeviceMemIN->psKernelMemInfo,
					PVRSRV_HANDLE_TYPE_MEM_INFO);

	return 0;
}

static int PVRSRVExportDeviceMemBW(u32 ui32BridgeID,
	   struct PVRSRV_BRIDGE_IN_EXPORTDEVICEMEM *psExportDeviceMemIN,
	   struct PVRSRV_BRIDGE_OUT_EXPORTDEVICEMEM *psExportDeviceMemOUT,
	   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	struct PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_EXPORT_DEVICEMEM);

	psExportDeviceMemOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psExportDeviceMemIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);

	if (psExportDeviceMemOUT->eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVExportDeviceMemBW: can't find devcookie");
		return 0;
	}

	psExportDeviceMemOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       (void **)&psKernelMemInfo,
			       psExportDeviceMemIN->psKernelMemInfo,
			       PVRSRV_HANDLE_TYPE_MEM_INFO);

	if (psExportDeviceMemOUT->eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVExportDeviceMemBW: can't find kernel meminfo");
		return 0;
	}

	psExportDeviceMemOUT->eError =
	    PVRSRVFindHandle(KERNEL_HANDLE_BASE,
			     &psExportDeviceMemOUT->hMemInfo,
			     psKernelMemInfo, PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (psExportDeviceMemOUT->eError == PVRSRV_OK) {
		PVR_DPF(PVR_DBG_MESSAGE, "PVRSRVExportDeviceMemBW: "
					  "allocation is already exported");
		return 0;
	}

	psExportDeviceMemOUT->eError = PVRSRVAllocHandle(KERNEL_HANDLE_BASE,
					 &psExportDeviceMemOUT->hMemInfo,
					 psKernelMemInfo,
					 PVRSRV_HANDLE_TYPE_MEM_INFO,
					 PVRSRV_HANDLE_ALLOC_FLAG_NONE);
	if (psExportDeviceMemOUT->eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVExportDeviceMemBW: "
			"failed to allocate handle from global handle list");
		return 0;
	}

	psKernelMemInfo->ui32Flags |= PVRSRV_MEM_EXPORTED;

	return 0;
}

static int PVRSRVMapDeviceMemoryBW(u32 ui32BridgeID,
			struct PVRSRV_BRIDGE_IN_MAP_DEV_MEMORY *psMapDevMemIN,
			struct PVRSRV_BRIDGE_OUT_MAP_DEV_MEMORY *psMapDevMemOUT,
			struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	struct PVRSRV_KERNEL_MEM_INFO *psSrcKernelMemInfo = NULL;
	struct PVRSRV_KERNEL_MEM_INFO *psDstKernelMemInfo = NULL;
	void *hDstDevMemHeap = NULL;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_MAP_DEV_MEMORY);

	NEW_HANDLE_BATCH_OR_ERROR(psMapDevMemOUT->eError, psPerProc, 2);

	psMapDevMemOUT->eError = PVRSRVLookupHandle(KERNEL_HANDLE_BASE,
					    (void **)&psSrcKernelMemInfo,
					    psMapDevMemIN->hKernelMemInfo,
					    PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (psMapDevMemOUT->eError != PVRSRV_OK)
		return 0;

	psMapDevMemOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					    &hDstDevMemHeap,
					    psMapDevMemIN->hDstDevMemHeap,
					    PVRSRV_HANDLE_TYPE_DEV_MEM_HEAP);
	if (psMapDevMemOUT->eError != PVRSRV_OK)
		return 0;

	psMapDevMemOUT->eError = PVRSRVMapDeviceMemoryKM(psPerProc,
							 psSrcKernelMemInfo,
							 hDstDevMemHeap,
							 &psDstKernelMemInfo);
	if (psMapDevMemOUT->eError != PVRSRV_OK)
		return 0;

	OSMemSet(&psMapDevMemOUT->sDstClientMemInfo, 0,
		 sizeof(psMapDevMemOUT->sDstClientMemInfo));
	OSMemSet(&psMapDevMemOUT->sDstClientSyncInfo, 0,
		 sizeof(psMapDevMemOUT->sDstClientSyncInfo));

	psMapDevMemOUT->sDstClientMemInfo.pvLinAddrKM =
				    psDstKernelMemInfo->pvLinAddrKM;

	psMapDevMemOUT->sDstClientMemInfo.pvLinAddr = NULL;
	psMapDevMemOUT->sDstClientMemInfo.sDevVAddr =
				    psDstKernelMemInfo->sDevVAddr;
	psMapDevMemOUT->sDstClientMemInfo.ui32Flags =
				    psDstKernelMemInfo->ui32Flags;
	psMapDevMemOUT->sDstClientMemInfo.ui32AllocSize =
				    psDstKernelMemInfo->ui32AllocSize;
	psMapDevMemOUT->sDstClientMemInfo.hMappingInfo =
				    psDstKernelMemInfo->sMemBlk.hOSMemHandle;

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			    &psMapDevMemOUT->sDstClientMemInfo.hKernelMemInfo,
			    psDstKernelMemInfo,
			    PVRSRV_HANDLE_TYPE_MEM_INFO,
			    PVRSRV_HANDLE_ALLOC_FLAG_NONE);
	psMapDevMemOUT->sDstClientSyncInfo.hKernelSyncInfo = NULL;
	psMapDevMemOUT->psDstKernelSyncInfo = NULL;

	if (psDstKernelMemInfo->psKernelSyncInfo) {
		psMapDevMemOUT->psDstKernelSyncInfo =
		    psDstKernelMemInfo->psKernelSyncInfo;

		psMapDevMemOUT->sDstClientSyncInfo.psSyncData =
		    psDstKernelMemInfo->psKernelSyncInfo->psSyncData;
		psMapDevMemOUT->sDstClientSyncInfo.sWriteOpsCompleteDevVAddr =
		    psDstKernelMemInfo->psKernelSyncInfo->
						    sWriteOpsCompleteDevVAddr;
		psMapDevMemOUT->sDstClientSyncInfo.sReadOpsCompleteDevVAddr =
		    psDstKernelMemInfo->psKernelSyncInfo->
						    sReadOpsCompleteDevVAddr;

		psMapDevMemOUT->sDstClientSyncInfo.hMappingInfo =
		    psDstKernelMemInfo->psKernelSyncInfo->psSyncDataMemInfoKM->
						    sMemBlk.hOSMemHandle;

		psMapDevMemOUT->sDstClientMemInfo.psClientSyncInfo =
		    &psMapDevMemOUT->sDstClientSyncInfo;

		PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
				       &psMapDevMemOUT->sDstClientSyncInfo.
								hKernelSyncInfo,
				       psDstKernelMemInfo->psKernelSyncInfo,
				       PVRSRV_HANDLE_TYPE_SYNC_INFO,
				       PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
				       psMapDevMemOUT->sDstClientMemInfo.
							       hKernelMemInfo);
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psMapDevMemOUT->eError, psPerProc);

	return 0;
}

static int PVRSRVUnmapDeviceMemoryBW(u32 ui32BridgeID,
		  struct PVRSRV_BRIDGE_IN_UNMAP_DEV_MEMORY *psUnmapDevMemIN,
		  struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	struct PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo = NULL;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_UNMAP_DEV_MEMORY);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					      (void **) &psKernelMemInfo,
					      psUnmapDevMemIN->psKernelMemInfo,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVUnmapDeviceMemoryKM(psKernelMemInfo);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
					       psUnmapDevMemIN->psKernelMemInfo,
					       PVRSRV_HANDLE_TYPE_MEM_INFO);

	return 0;
}

static int FlushCacheDRI(u32 ui32Type, u32 ui32Virt, u32 ui32Length)
{
	switch (ui32Type) {
	case DRM_PVR2D_CFLUSH_FROM_GPU:
		PVR_DPF(PVR_DBG_MESSAGE,
			 "DRM_PVR2D_CFLUSH_FROM_GPU 0x%08x, length 0x%08x\n",
			 ui32Virt, ui32Length);
#ifdef CONFIG_ARM
		dmac_map_area((const void *)ui32Virt, ui32Length, DMA_FROM_DEVICE);
#endif
		return 0;
	case DRM_PVR2D_CFLUSH_TO_GPU:
		PVR_DPF(PVR_DBG_MESSAGE,
			 "DRM_PVR2D_CFLUSH_TO_GPU 0x%08x, length 0x%08x\n",
			 ui32Virt, ui32Length);
#ifdef CONFIG_ARM
		dmac_map_area((const void *)ui32Virt, ui32Length, DMA_TO_DEVICE);
#endif
		return 0;
	default:
		PVR_DPF(PVR_DBG_ERROR, "Invalid cflush type 0x%x\n",
			 ui32Type);
		return -EINVAL;
	}

	return 0;
}

static int PVRSRVCacheFlushDRIBW(u32 ui32BridgeID,
	      struct PVRSRV_BRIDGE_IN_CACHEFLUSHDRMFROMUSER *psCacheFlushIN,
	      struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	struct vm_area_struct *vma;
	unsigned long start;
	size_t len;
	int type;
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_CACHE_FLUSH_DRM);

	start = psCacheFlushIN->ui32Virt;
	len = psCacheFlushIN->ui32Length;
	type = psCacheFlushIN->ui32Type;

	down_read(&current->mm->mmap_sem);
	vma = find_vma(current->mm, start);
	if (vma == NULL || vma->vm_start > start ||
	    vma->vm_end < start + len)
		pr_err("PVR: %s: invalid address %08lx %zu %c\n",
				__func__, start, len,
				type == DRM_PVR2D_CFLUSH_TO_GPU ? 'c' :
				type == DRM_PVR2D_CFLUSH_FROM_GPU ? 'i' :
				'?');
	else
		psRetOUT->eError = FlushCacheDRI(type, start, len);

	up_read(&current->mm->mmap_sem);

	return 0;
}

static int PVRSRVMapDeviceClassMemoryBW(u32 ui32BridgeID,
	   struct PVRSRV_BRIDGE_IN_MAP_DEVICECLASS_MEMORY *psMapDevClassMemIN,
	   struct PVRSRV_BRIDGE_OUT_MAP_DEVICECLASS_MEMORY *psMapDevClassMemOUT,
	   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	struct PVRSRV_KERNEL_MEM_INFO *psMemInfo;
	void *hOSMapInfo;
	void *hDeviceClassBufferInt;
	void *hDevMemContextInt;
	enum PVRSRV_HANDLE_TYPE eHandleType;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
			PVRSRV_BRIDGE_MAP_DEVICECLASS_MEMORY);

	NEW_HANDLE_BATCH_OR_ERROR(psMapDevClassMemOUT->eError, psPerProc, 2);

	psMapDevClassMemOUT->eError =
		PVRSRVLookupHandleAnyType(psPerProc->psHandleBase,
				&hDeviceClassBufferInt, &eHandleType,
				psMapDevClassMemIN->hDeviceClassBuffer);

	if (psMapDevClassMemOUT->eError != PVRSRV_OK)
		return 0;

	psMapDevClassMemOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevMemContextInt,
			       psMapDevClassMemIN->hDevMemContext,
			       PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);

	if (psMapDevClassMemOUT->eError != PVRSRV_OK)
		return 0;

	switch (eHandleType) {
	case PVRSRV_HANDLE_TYPE_DISP_BUFFER:
	case PVRSRV_HANDLE_TYPE_BUF_BUFFER:
		break;
	default:
		psMapDevClassMemOUT->eError = PVRSRV_ERROR_GENERIC;
		return 0;
	}

	psMapDevClassMemOUT->eError =
		PVRSRVMapDeviceClassMemoryKM(psPerProc, hDevMemContextInt,
				hDeviceClassBufferInt, &psMemInfo, &hOSMapInfo);

	if (psMapDevClassMemOUT->eError != PVRSRV_OK)
		return 0;

	OSMemSet(&psMapDevClassMemOUT->sClientMemInfo, 0,
		 sizeof(psMapDevClassMemOUT->sClientMemInfo));
	OSMemSet(&psMapDevClassMemOUT->sClientSyncInfo, 0,
		 sizeof(psMapDevClassMemOUT->sClientSyncInfo));

	psMapDevClassMemOUT->sClientMemInfo.pvLinAddrKM =
						psMemInfo->pvLinAddrKM;

	psMapDevClassMemOUT->sClientMemInfo.pvLinAddr = NULL;
	psMapDevClassMemOUT->sClientMemInfo.sDevVAddr = psMemInfo->sDevVAddr;
	psMapDevClassMemOUT->sClientMemInfo.ui32Flags = psMemInfo->ui32Flags;
	psMapDevClassMemOUT->sClientMemInfo.ui32AllocSize =
						psMemInfo->ui32AllocSize;
	psMapDevClassMemOUT->sClientMemInfo.hMappingInfo =
						psMemInfo->sMemBlk.hOSMemHandle;

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			&psMapDevClassMemOUT->sClientMemInfo.hKernelMemInfo,
			psMemInfo,
			PVRSRV_HANDLE_TYPE_MEM_INFO,
			PVRSRV_HANDLE_ALLOC_FLAG_NONE,
			psMapDevClassMemIN->hDeviceClassBuffer);

	psMapDevClassMemOUT->sClientSyncInfo.hKernelSyncInfo = NULL;
	psMapDevClassMemOUT->psKernelSyncInfo = NULL;

	if (psMemInfo->psKernelSyncInfo) {
		psMapDevClassMemOUT->psKernelSyncInfo =
			psMemInfo->psKernelSyncInfo;

		psMapDevClassMemOUT->sClientSyncInfo.psSyncData =
			psMemInfo->psKernelSyncInfo->psSyncData;
		psMapDevClassMemOUT->sClientSyncInfo.sWriteOpsCompleteDevVAddr =
			psMemInfo->psKernelSyncInfo->sWriteOpsCompleteDevVAddr;
		psMapDevClassMemOUT->sClientSyncInfo.sReadOpsCompleteDevVAddr =
			psMemInfo->psKernelSyncInfo->sReadOpsCompleteDevVAddr;

		psMapDevClassMemOUT->sClientSyncInfo.hMappingInfo =
				psMemInfo->psKernelSyncInfo->
				     psSyncDataMemInfoKM->sMemBlk.hOSMemHandle;

		psMapDevClassMemOUT->sClientMemInfo.psClientSyncInfo =
			&psMapDevClassMemOUT->sClientSyncInfo;

		PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
				&psMapDevClassMemOUT->sClientSyncInfo.
								hKernelSyncInfo,
				psMemInfo->psKernelSyncInfo,
				PVRSRV_HANDLE_TYPE_SYNC_INFO,
				PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
				psMapDevClassMemOUT->sClientMemInfo.
								hKernelMemInfo);
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psMapDevClassMemOUT->eError, psPerProc);

	return 0;
}

static int PVRSRVUnmapDeviceClassMemoryBW(u32 ui32BridgeID,
	 struct PVRSRV_BRIDGE_IN_UNMAP_DEVICECLASS_MEMORY *psUnmapDevClassMemIN,
	 struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	 struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvKernelMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_UNMAP_DEVICECLASS_MEMORY);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &pvKernelMemInfo,
			       psUnmapDevClassMemIN->psKernelMemInfo,
			       PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVUnmapDeviceClassMemoryKM(pvKernelMemInfo);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVReleaseHandle(psPerProc->psHandleBase,
				psUnmapDevClassMemIN->psKernelMemInfo,
				PVRSRV_HANDLE_TYPE_MEM_INFO);

	return 0;
}

static int PVRSRVWrapExtMemoryBW(u32 ui32BridgeID,
		      struct PVRSRV_BRIDGE_IN_WRAP_EXT_MEMORY *psWrapExtMemIN,
		      struct PVRSRV_BRIDGE_OUT_WRAP_EXT_MEMORY *psWrapExtMemOUT,
		      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	void *hDevMemContextInt;
	struct PVRSRV_KERNEL_MEM_INFO *psMemInfo;
	u32 ui32PageTableSize = 0;
	struct IMG_SYS_PHYADDR *psSysPAddr = NULL;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_WRAP_EXT_MEMORY);

	NEW_HANDLE_BATCH_OR_ERROR(psWrapExtMemOUT->eError, psPerProc, 2);

	psWrapExtMemOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevCookieInt,
			       psWrapExtMemIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psWrapExtMemOUT->eError != PVRSRV_OK)
		return 0;

	psWrapExtMemOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevMemContextInt,
			       psWrapExtMemIN->hDevMemContext,
			       PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);

	if (psWrapExtMemOUT->eError != PVRSRV_OK)
		return 0;

	if (psWrapExtMemIN->ui32NumPageTableEntries) {
		ui32PageTableSize = psWrapExtMemIN->ui32NumPageTableEntries
		    * sizeof(struct IMG_SYS_PHYADDR);

		ASSIGN_AND_EXIT_ON_ERROR(psWrapExtMemOUT->eError,
					 OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						   ui32PageTableSize,
						   (void **)&psSysPAddr, NULL));

		if (CopyFromUserWrapper(psPerProc, ui32BridgeID, psSysPAddr,
					psWrapExtMemIN->psSysPAddr,
					ui32PageTableSize) != PVRSRV_OK) {
			OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, ui32PageTableSize,
				  (void *) psSysPAddr, NULL);
			return -EFAULT;
		}
	}

	psWrapExtMemOUT->eError = PVRSRVWrapExtMemoryKM(hDevCookieInt,
				  psPerProc, hDevMemContextInt,
				  psWrapExtMemIN->ui32ByteSize,
				  psWrapExtMemIN->ui32PageOffset,
				  psWrapExtMemIN->bPhysContig,
				  psSysPAddr, psWrapExtMemIN->pvLinAddr,
				  &psMemInfo);
	if (psWrapExtMemIN->ui32NumPageTableEntries)
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, ui32PageTableSize,
			  (void *)psSysPAddr, NULL);
	if (psWrapExtMemOUT->eError != PVRSRV_OK)
		return 0;

	psWrapExtMemOUT->sClientMemInfo.pvLinAddrKM = psMemInfo->pvLinAddrKM;

	psWrapExtMemOUT->sClientMemInfo.pvLinAddr = NULL;
	psWrapExtMemOUT->sClientMemInfo.sDevVAddr = psMemInfo->sDevVAddr;
	psWrapExtMemOUT->sClientMemInfo.ui32Flags = psMemInfo->ui32Flags;
	psWrapExtMemOUT->sClientMemInfo.ui32AllocSize =
						    psMemInfo->ui32AllocSize;
	psWrapExtMemOUT->sClientMemInfo.hMappingInfo =
					    psMemInfo->sMemBlk.hOSMemHandle;

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			    &psWrapExtMemOUT->sClientMemInfo.hKernelMemInfo,
			    psMemInfo, PVRSRV_HANDLE_TYPE_MEM_INFO,
			    PVRSRV_HANDLE_ALLOC_FLAG_NONE);

	psWrapExtMemOUT->sClientSyncInfo.psSyncData =
	    psMemInfo->psKernelSyncInfo->psSyncData;
	psWrapExtMemOUT->sClientSyncInfo.sWriteOpsCompleteDevVAddr =
	    psMemInfo->psKernelSyncInfo->sWriteOpsCompleteDevVAddr;
	psWrapExtMemOUT->sClientSyncInfo.sReadOpsCompleteDevVAddr =
	    psMemInfo->psKernelSyncInfo->sReadOpsCompleteDevVAddr;

	psWrapExtMemOUT->sClientSyncInfo.hMappingInfo =
	    psMemInfo->psKernelSyncInfo->psSyncDataMemInfoKM->sMemBlk.
							    hOSMemHandle;

	psWrapExtMemOUT->sClientMemInfo.psClientSyncInfo =
	    &psWrapExtMemOUT->sClientSyncInfo;

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			       &psWrapExtMemOUT->sClientSyncInfo.
							       hKernelSyncInfo,
			       (void *)psMemInfo->psKernelSyncInfo,
			       PVRSRV_HANDLE_TYPE_SYNC_INFO,
			       PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
			       psWrapExtMemOUT->sClientMemInfo.hKernelMemInfo);

	COMMIT_HANDLE_BATCH_OR_ERROR(psWrapExtMemOUT->eError, psPerProc);

	return 0;
}

static int PVRSRVUnwrapExtMemoryBW(u32 ui32BridgeID,
		struct PVRSRV_BRIDGE_IN_UNWRAP_EXT_MEMORY *psUnwrapExtMemIN,
		struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_UNWRAP_EXT_MEMORY);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvMemInfo, psUnwrapExtMemIN->hKernelMemInfo,
			       PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVUnwrapExtMemoryKM((struct PVRSRV_KERNEL_MEM_INFO *)pvMemInfo);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
				psUnwrapExtMemIN->hKernelMemInfo,
				PVRSRV_HANDLE_TYPE_MEM_INFO);

	return 0;
}

static int PVRSRVGetFreeDeviceMemBW(u32 ui32BridgeID,
	 struct PVRSRV_BRIDGE_IN_GETFREEDEVICEMEM *psGetFreeDeviceMemIN,
	 struct PVRSRV_BRIDGE_OUT_GETFREEDEVICEMEM *psGetFreeDeviceMemOUT,
	 struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_GETFREE_DEVICEMEM);

	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psGetFreeDeviceMemOUT->eError =
	    PVRSRVGetFreeDeviceMemKM(psGetFreeDeviceMemIN->ui32Flags,
				     &psGetFreeDeviceMemOUT->ui32Total,
				     &psGetFreeDeviceMemOUT->ui32Free,
				     &psGetFreeDeviceMemOUT->ui32LargestBlock);

	return 0;
}

static int PVRMMapOSMemHandleToMMapDataBW(u32 ui32BridgeID,
	  struct PVRSRV_BRIDGE_IN_MHANDLE_TO_MMAP_DATA *psMMapDataIN,
	  struct PVRSRV_BRIDGE_OUT_MHANDLE_TO_MMAP_DATA *psMMapDataOUT,
	  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_MHANDLE_TO_MMAP_DATA);

	psMMapDataOUT->eError =
	    PVRMMapOSMemHandleToMMapData(psPerProc, psMMapDataIN->hMHandle,
					 &psMMapDataOUT->ui32MMapOffset,
					 &psMMapDataOUT->ui32ByteOffset,
					 &psMMapDataOUT->ui32RealByteSize,
					 &psMMapDataOUT->ui32UserVAddr);
	return 0;
}

static int PVRMMapReleaseMMapDataBW(u32 ui32BridgeID,
		    struct PVRSRV_BRIDGE_IN_RELEASE_MMAP_DATA *psMMapDataIN,
		    struct PVRSRV_BRIDGE_OUT_RELEASE_MMAP_DATA *psMMapDataOUT,
		    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_RELEASE_MMAP_DATA);

	psMMapDataOUT->eError = PVRMMapReleaseMMapData(psPerProc,
					   psMMapDataIN->hMHandle,
					   &psMMapDataOUT->bMUnmap,
					   &psMMapDataOUT->ui32RealByteSize,
					   &psMMapDataOUT->ui32UserVAddr);
	return 0;
}

#ifdef PDUMP
static int PDumpIsCaptureFrameBW(u32 ui32BridgeID, void *psBridgeIn,
	      struct PVRSRV_BRIDGE_OUT_PDUMP_ISCAPTURING *psPDumpIsCapturingOUT,
	      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_ISCAPTURING);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psPDumpIsCapturingOUT->bIsCapturing = PDumpIsCaptureFrameKM();
	psPDumpIsCapturingOUT->eError = PVRSRV_OK;

	return 0;
}

static int PDumpCommentBW(u32 ui32BridgeID,
	       struct PVRSRV_BRIDGE_IN_PDUMP_COMMENT *psPDumpCommentIN,
	       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	       struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_COMMENT);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psRetOUT->eError = PDumpCommentKM(&psPDumpCommentIN->szComment[0],
					  psPDumpCommentIN->ui32Flags);
	return 0;
}

static int PDumpSetFrameBW(u32 ui32BridgeID,
		struct PVRSRV_BRIDGE_IN_PDUMP_SETFRAME *psPDumpSetFrameIN,
		struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_SETFRAME);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psRetOUT->eError = PDumpSetFrameKM(psPDumpSetFrameIN->ui32Frame);

	return 0;
}

static int PDumpRegWithFlagsBW(u32 ui32BridgeID,
		    struct PVRSRV_BRIDGE_IN_PDUMP_DUMPREG *psPDumpRegDumpIN,
		    struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_REG);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psRetOUT->eError =
	    PDumpRegWithFlagsKM(psPDumpRegDumpIN->sHWReg.ui32RegAddr,
				psPDumpRegDumpIN->sHWReg.ui32RegVal,
				psPDumpRegDumpIN->ui32Flags);

	return 0;
}

static int PDumpRegPolBW(u32 ui32BridgeID,
	      struct PVRSRV_BRIDGE_IN_PDUMP_REGPOL *psPDumpRegPolIN,
	      struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_REGPOL);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psRetOUT->eError =
	    PDumpRegPolWithFlagsKM(psPDumpRegPolIN->sHWReg.ui32RegAddr,
				   psPDumpRegPolIN->sHWReg.ui32RegVal,
				   psPDumpRegPolIN->ui32Mask,
				   psPDumpRegPolIN->ui32Flags);

	return 0;
}

static int PDumpMemPolBW(u32 ui32BridgeID,
	      struct PVRSRV_BRIDGE_IN_PDUMP_MEMPOL *psPDumpMemPolIN,
	      struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_MEMPOL);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					      &pvMemInfo,
					      psPDumpMemPolIN->psKernelMemInfo,
					      PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PDumpMemPolKM(((struct PVRSRV_KERNEL_MEM_INFO *)pvMemInfo),
			  psPDumpMemPolIN->ui32Offset,
			  psPDumpMemPolIN->ui32Value,
			  psPDumpMemPolIN->ui32Mask,
			  PDUMP_POLL_OPERATOR_EQUAL,
			  psPDumpMemPolIN->bLastFrame,
			  psPDumpMemPolIN->bOverwrite,
			  MAKEUNIQUETAG(pvMemInfo));

	return 0;
}

static int PDumpMemBW(u32 ui32BridgeID,
	   struct PVRSRV_BRIDGE_IN_PDUMP_DUMPMEM *psPDumpMemDumpIN,
	   struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_DUMPMEM);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					&pvMemInfo,
					psPDumpMemDumpIN->psKernelMemInfo,
					PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PDumpMemUM(psPerProc, psPDumpMemDumpIN->pvAltLinAddr,
				       psPDumpMemDumpIN->pvLinAddr,
				       pvMemInfo, psPDumpMemDumpIN->ui32Offset,
				       psPDumpMemDumpIN->ui32Bytes,
				       psPDumpMemDumpIN->ui32Flags,
						      MAKEUNIQUETAG(pvMemInfo));

	return 0;
}

static int PDumpBitmapBW(u32 ui32BridgeID,
	      struct PVRSRV_BRIDGE_IN_PDUMP_BITMAP *psPDumpBitmapIN,
	      struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVR_UNREFERENCED_PARAMETER(psPerProc);
	PVR_UNREFERENCED_PARAMETER(ui32BridgeID);

	psRetOUT->eError = PDumpBitmapKM(&psPDumpBitmapIN->szFileName[0],
			  psPDumpBitmapIN->ui32FileOffset,
			  psPDumpBitmapIN->ui32Width,
			  psPDumpBitmapIN->ui32Height,
			  psPDumpBitmapIN->ui32StrideInBytes,
			  psPDumpBitmapIN->sDevBaseAddr,
			  psPDumpBitmapIN->ui32Size,
			  psPDumpBitmapIN->ePixelFormat,
			  psPDumpBitmapIN->eMemFormat,
			  psPDumpBitmapIN->ui32Flags);

	return 0;
}

static int PDumpReadRegBW(u32 ui32BridgeID,
	       struct PVRSRV_BRIDGE_IN_PDUMP_READREG *psPDumpReadRegIN,
	       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	       struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_DUMPREADREG);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psRetOUT->eError = PDumpReadRegKM(&psPDumpReadRegIN->szFileName[0],
			   psPDumpReadRegIN->ui32FileOffset,
			   psPDumpReadRegIN->ui32Address,
			   psPDumpReadRegIN->ui32Size,
			   psPDumpReadRegIN->ui32Flags);

	return 0;
}

static int PDumpDriverInfoBW(u32 ui32BridgeID,
		  struct PVRSRV_BRIDGE_IN_PDUMP_DRIVERINFO *psPDumpDriverInfoIN,
		  struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	u32 ui32PDumpFlags;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_DRIVERINFO);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	ui32PDumpFlags = 0;
	if (psPDumpDriverInfoIN->bContinuous)
		ui32PDumpFlags |= PDUMP_FLAGS_CONTINUOUS;
	psRetOUT->eError = PDumpDriverInfoKM(&psPDumpDriverInfoIN->szString[0],
					     ui32PDumpFlags);

	return 0;
}

static int PDumpSyncDumpBW(u32 ui32BridgeID,
		struct PVRSRV_BRIDGE_IN_PDUMP_DUMPSYNC *psPDumpSyncDumpIN,
		struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	u32 ui32Bytes = psPDumpSyncDumpIN->ui32Bytes;
	void *pvSyncInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_DUMPSYNC);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &pvSyncInfo,
			       psPDumpSyncDumpIN->psKernelSyncInfo,
			       PVRSRV_HANDLE_TYPE_SYNC_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PDumpMemUM(psPerProc, psPDumpSyncDumpIN->pvAltLinAddr, NULL,
			((struct PVRSRV_KERNEL_SYNC_INFO *)pvSyncInfo)->
							psSyncDataMemInfoKM,
			psPDumpSyncDumpIN->ui32Offset, ui32Bytes, 0,
			MAKEUNIQUETAG(((struct PVRSRV_KERNEL_SYNC_INFO *)
					    pvSyncInfo)->psSyncDataMemInfoKM));

	return 0;
}

static int PDumpSyncPolBW(u32 ui32BridgeID,
	       struct PVRSRV_BRIDGE_IN_PDUMP_SYNCPOL *psPDumpSyncPolIN,
	       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	       struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	u32 ui32Offset;
	void *pvSyncInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_SYNCPOL);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &pvSyncInfo,
			       psPDumpSyncPolIN->psKernelSyncInfo,
			       PVRSRV_HANDLE_TYPE_SYNC_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	if (psPDumpSyncPolIN->bIsRead)
		ui32Offset = offsetof(struct PVRSRV_SYNC_DATA,
				      ui32ReadOpsComplete);
	else
		ui32Offset = offsetof(struct PVRSRV_SYNC_DATA,
				      ui32WriteOpsComplete);

	psRetOUT->eError =
	    PDumpMemPolKM(((struct PVRSRV_KERNEL_SYNC_INFO *)pvSyncInfo)->
			  psSyncDataMemInfoKM, ui32Offset,
			  psPDumpSyncPolIN->ui32Value,
			  psPDumpSyncPolIN->ui32Mask, PDUMP_POLL_OPERATOR_EQUAL,
			  IMG_FALSE, IMG_FALSE,
			  MAKEUNIQUETAG(((struct PVRSRV_KERNEL_SYNC_INFO *)
					  pvSyncInfo)->psSyncDataMemInfoKM));

	return 0;
}

static int PDumpPDRegBW(u32 ui32BridgeID,
	     struct PVRSRV_BRIDGE_IN_PDUMP_DUMPPDREG *psPDumpPDRegDumpIN,
	     struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	     struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_PDUMP_PDREG);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	PDumpPDReg(psPDumpPDRegDumpIN->sHWReg.ui32RegAddr,
		   psPDumpPDRegDumpIN->sHWReg.ui32RegVal, PDUMP_PD_UNIQUETAG);

	psRetOUT->eError = PVRSRV_OK;
	return 0;
}

static int PDumpCycleCountRegReadBW(u32 ui32BridgeID,
		struct PVRSRV_BRIDGE_IN_PDUMP_CYCLE_COUNT_REG_READ
					*psPDumpCycleCountRegReadIN,
		struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_PDUMP_CYCLE_COUNT_REG_READ);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	PDumpCycleCountRegRead(psPDumpCycleCountRegReadIN->ui32RegOffset,
			       psPDumpCycleCountRegReadIN->bLastFrame);

	psRetOUT->eError = PVRSRV_OK;

	return 0;
}

static int PDumpPDDevPAddrBW(u32 ui32BridgeID,
	  struct PVRSRV_BRIDGE_IN_PDUMP_DUMPPDDEVPADDR *psPDumpPDDevPAddrIN,
	  struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_PDUMP_DUMPPDDEVPADDR);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &pvMemInfo,
			       psPDumpPDDevPAddrIN->hKernelMemInfo,
			       PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PDumpPDDevPAddrKM((struct PVRSRV_KERNEL_MEM_INFO *)pvMemInfo,
			      psPDumpPDDevPAddrIN->ui32Offset,
			      psPDumpPDDevPAddrIN->sPDDevPAddr,
			      MAKEUNIQUETAG(pvMemInfo), PDUMP_PD_UNIQUETAG);
	return 0;
}

static int PDumpStartInitPhaseBW(u32 ui32BridgeID, void *psBridgeIn,
		       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		       struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_PDUMP_STARTINITPHASE);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psRetOUT->eError = PDumpStartInitPhaseKM();

	return 0;
}

static int PDumpStopInitPhaseBW(u32 ui32BridgeID, void *psBridgeIn,
	struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_PDUMP_STOPINITPHASE);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	psRetOUT->eError = PDumpStopInitPhaseKM();

	return 0;
}

#endif

static int PVRSRVGetMiscInfoBW(u32 ui32BridgeID,
		    struct PVRSRV_BRIDGE_IN_GET_MISC_INFO *psGetMiscInfoIN,
		    struct PVRSRV_BRIDGE_OUT_GET_MISC_INFO *psGetMiscInfoOUT,
		    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	enum PVRSRV_ERROR eError;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_GET_MISC_INFO);

	OSMemCopy(&psGetMiscInfoOUT->sMiscInfo, &psGetMiscInfoIN->sMiscInfo,
		  sizeof(struct PVRSRV_MISC_INFO));

	if (((psGetMiscInfoIN->sMiscInfo.ui32StateRequest &
		PVRSRV_MISC_INFO_MEMSTATS_PRESENT) != 0) &&
	    ((psGetMiscInfoIN->sMiscInfo.ui32StateRequest &
		PVRSRV_MISC_INFO_DDKVERSION_PRESENT) != 0)) {

		psGetMiscInfoOUT->eError = PVRSRV_ERROR_INVALID_PARAMS;
		return 0;
	}

	if (((psGetMiscInfoIN->sMiscInfo.ui32StateRequest &
		PVRSRV_MISC_INFO_MEMSTATS_PRESENT) != 0) ||
	    ((psGetMiscInfoIN->sMiscInfo.ui32StateRequest &
		PVRSRV_MISC_INFO_DDKVERSION_PRESENT) != 0)) {

		ASSIGN_AND_EXIT_ON_ERROR(
			psGetMiscInfoOUT->eError,
			OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			   psGetMiscInfoOUT->sMiscInfo.ui32MemoryStrLen,
			   (void **)&psGetMiscInfoOUT->sMiscInfo.pszMemoryStr,
			   NULL));

		psGetMiscInfoOUT->eError =
		    PVRSRVGetMiscInfoKM(&psGetMiscInfoOUT->sMiscInfo);

		eError = CopyToUserWrapper(psPerProc, ui32BridgeID,
				(void __force __user *)
					psGetMiscInfoIN->sMiscInfo.pszMemoryStr,
				psGetMiscInfoOUT->sMiscInfo.pszMemoryStr,
				psGetMiscInfoOUT->sMiscInfo.ui32MemoryStrLen);

		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  psGetMiscInfoOUT->sMiscInfo.ui32MemoryStrLen,
			  (void *)psGetMiscInfoOUT->sMiscInfo.pszMemoryStr,
			  NULL);

		psGetMiscInfoOUT->sMiscInfo.pszMemoryStr =
					psGetMiscInfoIN->sMiscInfo.pszMemoryStr;

		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				 "PVRSRVGetMiscInfoBW Error copy to user");
			return -EFAULT;
		}
	} else {
		psGetMiscInfoOUT->eError =
			    PVRSRVGetMiscInfoKM(&psGetMiscInfoOUT->sMiscInfo);
	}

	if (psGetMiscInfoOUT->eError != PVRSRV_OK)
		return 0;

	if (psGetMiscInfoIN->sMiscInfo.ui32StateRequest &
	    PVRSRV_MISC_INFO_GLOBALEVENTOBJECT_PRESENT) {
		psGetMiscInfoOUT->eError =
		    PVRSRVAllocHandle(psPerProc->psHandleBase,
				      &psGetMiscInfoOUT->sMiscInfo.
					      sGlobalEventObject.hOSEventKM,
				      psGetMiscInfoOUT->sMiscInfo.
					      sGlobalEventObject.hOSEventKM,
				      PVRSRV_HANDLE_TYPE_SHARED_EVENT_OBJECT,
				      PVRSRV_HANDLE_ALLOC_FLAG_SHARED);

		if (psGetMiscInfoOUT->eError != PVRSRV_OK)
			return 0;
	}

	if (psGetMiscInfoOUT->sMiscInfo.hSOCTimerRegisterOSMemHandle) {
		psGetMiscInfoOUT->eError =
		    PVRSRVAllocHandle(psPerProc->psHandleBase,
				      &psGetMiscInfoOUT->sMiscInfo.
					      hSOCTimerRegisterOSMemHandle,
				      psGetMiscInfoOUT->sMiscInfo.
					      hSOCTimerRegisterOSMemHandle,
				      PVRSRV_HANDLE_TYPE_SOC_TIMER,
				      PVRSRV_HANDLE_ALLOC_FLAG_SHARED);

		if (psGetMiscInfoOUT->eError != PVRSRV_OK)
			return 0;
	}

	return 0;
}

static int PVRSRVConnectBW(u32 ui32BridgeID, void *psBridgeIn,
		struct PVRSRV_BRIDGE_OUT_CONNECT_SERVICES *psConnectServicesOUT,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_CONNECT_SERVICES);

	psConnectServicesOUT->hKernelServices = psPerProc->hPerProcData;
	psConnectServicesOUT->eError = PVRSRV_OK;

#if defined(PDUMP)

	{
		struct SYS_DATA *psSysData;
		SysAcquireData(&psSysData);
		psSysData->bPowerUpPDumped = IMG_FALSE;
	}
#endif

	return 0;
}

static int PVRSRVDisconnectBW(u32 ui32BridgeID, void *psBridgeIn,
		   struct PVRSRV_BRIDGE_RETURN *psRetOUT,
		   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVR_UNREFERENCED_PARAMETER(psPerProc);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_DISCONNECT_SERVICES);

	psRetOUT->eError = PVRSRV_OK;

	return 0;
}

static int PVRSRVEnumerateDCBW(u32 ui32BridgeID,
		    struct PVRSRV_BRIDGE_IN_ENUMCLASS *psEnumDispClassIN,
		    struct PVRSRV_BRIDGE_OUT_ENUMCLASS *psEnumDispClassOUT,
		    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVR_UNREFERENCED_PARAMETER(psPerProc);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_ENUM_CLASS);

	psEnumDispClassOUT->eError =
	    PVRSRVEnumerateDCKM(psEnumDispClassIN->sDeviceClass,
				&psEnumDispClassOUT->ui32NumDevices,
				&psEnumDispClassOUT->ui32DevID[0]);

	return 0;
}

static int PVRSRVOpenDCDeviceBW(u32 ui32BridgeID,
     struct PVRSRV_BRIDGE_IN_OPEN_DISPCLASS_DEVICE *psOpenDispClassDeviceIN,
     struct PVRSRV_BRIDGE_OUT_OPEN_DISPCLASS_DEVICE *psOpenDispClassDeviceOUT,
     struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	void *hDispClassInfoInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_OPEN_DISPCLASS_DEVICE);

	NEW_HANDLE_BATCH_OR_ERROR(psOpenDispClassDeviceOUT->eError, psPerProc,
				  1);

	psOpenDispClassDeviceOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &hDevCookieInt,
			       psOpenDispClassDeviceIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psOpenDispClassDeviceOUT->eError != PVRSRV_OK)
		return 0;

	psOpenDispClassDeviceOUT->eError = PVRSRVOpenDCDeviceKM(psPerProc,
				 psOpenDispClassDeviceIN->ui32DeviceID,
				 hDevCookieInt, &hDispClassInfoInt);

	if (psOpenDispClassDeviceOUT->eError != PVRSRV_OK)
		return 0;

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			    &psOpenDispClassDeviceOUT->hDeviceKM,
			    hDispClassInfoInt,
			    PVRSRV_HANDLE_TYPE_DISP_INFO,
			    PVRSRV_HANDLE_ALLOC_FLAG_NONE);
	COMMIT_HANDLE_BATCH_OR_ERROR(psOpenDispClassDeviceOUT->eError,
				     psPerProc);

	return 0;
}

static int PVRSRVCloseDCDeviceBW(u32 ui32BridgeID,
     struct PVRSRV_BRIDGE_IN_CLOSE_DISPCLASS_DEVICE *psCloseDispClassDeviceIN,
     struct PVRSRV_BRIDGE_RETURN *psRetOUT,
     struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfoInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_CLOSE_DISPCLASS_DEVICE);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfoInt,
			       psCloseDispClassDeviceIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVCloseDCDeviceKM(pvDispClassInfoInt, IMG_FALSE);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVReleaseHandle(psPerProc->psHandleBase,
				psCloseDispClassDeviceIN->hDeviceKM,
				PVRSRV_HANDLE_TYPE_DISP_INFO);
	return 0;
}

static int PVRSRVEnumDCFormatsBW(u32 ui32BridgeID,
     struct PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_FORMATS *psEnumDispClassFormatsIN,
     struct PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_FORMATS *psEnumDispClassFormatsOUT,
     struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfoInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_ENUM_DISPCLASS_FORMATS);

	psEnumDispClassFormatsOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfoInt,
			       psEnumDispClassFormatsIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);
	if (psEnumDispClassFormatsOUT->eError != PVRSRV_OK)
		return 0;

	psEnumDispClassFormatsOUT->eError =
	    PVRSRVEnumDCFormatsKM(pvDispClassInfoInt,
				  &psEnumDispClassFormatsOUT->ui32Count,
				  psEnumDispClassFormatsOUT->asFormat);

	return 0;
}

static int PVRSRVEnumDCDimsBW(u32 ui32BridgeID,
	   struct PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_DIMS *psEnumDispClassDimsIN,
	   struct PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_DIMS *psEnumDispClassDimsOUT,
	   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfoInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_ENUM_DISPCLASS_DIMS);

	psEnumDispClassDimsOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfoInt,
			       psEnumDispClassDimsIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);

	if (psEnumDispClassDimsOUT->eError != PVRSRV_OK)
		return 0;

	psEnumDispClassDimsOUT->eError =
	    PVRSRVEnumDCDimsKM(pvDispClassInfoInt,
			       &psEnumDispClassDimsIN->sFormat,
			       &psEnumDispClassDimsOUT->ui32Count,
			       psEnumDispClassDimsOUT->asDim);

	return 0;
}

static int PVRSRVGetDCSystemBufferBW(u32 ui32BridgeID,
   struct PVRSRV_BRIDGE_IN_GET_DISPCLASS_SYSBUFFER *psGetDispClassSysBufferIN,
   struct PVRSRV_BRIDGE_OUT_GET_DISPCLASS_SYSBUFFER *psGetDispClassSysBufferOUT,
   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hBufferInt;
	void *pvDispClassInfoInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_GET_DISPCLASS_SYSBUFFER);

	NEW_HANDLE_BATCH_OR_ERROR(psGetDispClassSysBufferOUT->eError, psPerProc,
				  1);

	psGetDispClassSysBufferOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfoInt,
			       psGetDispClassSysBufferIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);
	if (psGetDispClassSysBufferOUT->eError != PVRSRV_OK)
		return 0;

	psGetDispClassSysBufferOUT->eError =
	    PVRSRVGetDCSystemBufferKM(pvDispClassInfoInt, &hBufferInt);

	if (psGetDispClassSysBufferOUT->eError != PVRSRV_OK)
		return 0;

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			       &psGetDispClassSysBufferOUT->hBuffer,
			       hBufferInt,
			       PVRSRV_HANDLE_TYPE_DISP_BUFFER,
			       (enum PVRSRV_HANDLE_ALLOC_FLAG)
			       (PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE |
				PVRSRV_HANDLE_ALLOC_FLAG_SHARED),
			       psGetDispClassSysBufferIN->hDeviceKM);

	COMMIT_HANDLE_BATCH_OR_ERROR(psGetDispClassSysBufferOUT->eError,
				     psPerProc);

	return 0;
}

static int PVRSRVGetDCInfoBW(u32 ui32BridgeID,
	  struct PVRSRV_BRIDGE_IN_GET_DISPCLASS_INFO *psGetDispClassInfoIN,
	  struct PVRSRV_BRIDGE_OUT_GET_DISPCLASS_INFO *psGetDispClassInfoOUT,
	  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_GET_DISPCLASS_INFO);

	psGetDispClassInfoOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfo,
			       psGetDispClassInfoIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);
	if (psGetDispClassInfoOUT->eError != PVRSRV_OK)
		return 0;

	psGetDispClassInfoOUT->eError =
	    PVRSRVGetDCInfoKM(pvDispClassInfo,
			      &psGetDispClassInfoOUT->sDisplayInfo);

	return 0;
}

static int PVRSRVCreateDCSwapChainBW(u32 ui32BridgeID,
		  struct PVRSRV_BRIDGE_IN_CREATE_DISPCLASS_SWAPCHAIN
				*psCreateDispClassSwapChainIN,
		  struct PVRSRV_BRIDGE_OUT_CREATE_DISPCLASS_SWAPCHAIN
				*psCreateDispClassSwapChainOUT,
		  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfo;
	void *hSwapChainInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_CREATE_DISPCLASS_SWAPCHAIN);

	NEW_HANDLE_BATCH_OR_ERROR(psCreateDispClassSwapChainOUT->eError,
				  psPerProc, 1);

	psCreateDispClassSwapChainOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfo,
			       psCreateDispClassSwapChainIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);

	if (psCreateDispClassSwapChainOUT->eError != PVRSRV_OK)
		return 0;

	psCreateDispClassSwapChainOUT->eError =
	    PVRSRVCreateDCSwapChainKM(psPerProc, pvDispClassInfo,
			psCreateDispClassSwapChainIN->ui32Flags,
			&psCreateDispClassSwapChainIN->sDstSurfAttrib,
			&psCreateDispClassSwapChainIN->sSrcSurfAttrib,
			psCreateDispClassSwapChainIN->ui32BufferCount,
			psCreateDispClassSwapChainIN->ui32OEMFlags,
			&hSwapChainInt,
			&psCreateDispClassSwapChainOUT->ui32SwapChainID);

	if (psCreateDispClassSwapChainOUT->eError != PVRSRV_OK)
		return 0;

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			       &psCreateDispClassSwapChainOUT->hSwapChain,
			       hSwapChainInt,
			       PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN,
			       PVRSRV_HANDLE_ALLOC_FLAG_NONE,
			       psCreateDispClassSwapChainIN->hDeviceKM);

	COMMIT_HANDLE_BATCH_OR_ERROR(psCreateDispClassSwapChainOUT->eError,
				     psPerProc);

	return 0;
}

static int PVRSRVDestroyDCSwapChainBW(u32 ui32BridgeID,
			   struct PVRSRV_BRIDGE_IN_DESTROY_DISPCLASS_SWAPCHAIN
					*psDestroyDispClassSwapChainIN,
			   struct PVRSRV_BRIDGE_RETURN *psRetOUT,
			   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_DESTROY_DISPCLASS_SWAPCHAIN);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &pvSwapChain,
			       psDestroyDispClassSwapChainIN->hSwapChain,
			       PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVDestroyDCSwapChainKM(pvSwapChain);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVReleaseHandle(psPerProc->psHandleBase,
				psDestroyDispClassSwapChainIN->hSwapChain,
				PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);

	return 0;
}

static int PVRSRVSetDCDstRectBW(u32 ui32BridgeID,
	 struct PVRSRV_BRIDGE_IN_SET_DISPCLASS_RECT *psSetDispClassDstRectIN,
	 struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	 struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfo;
	void *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SET_DISPCLASS_DSTRECT);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfo,
			       psSetDispClassDstRectIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvSwapChain,
			       psSetDispClassDstRectIN->hSwapChain,
			       PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVSetDCDstRectKM(pvDispClassInfo,
				 pvSwapChain, &psSetDispClassDstRectIN->sRect);

	return 0;
}

static int PVRSRVSetDCSrcRectBW(u32 ui32BridgeID,
     struct PVRSRV_BRIDGE_IN_SET_DISPCLASS_RECT *psSetDispClassSrcRectIN,
     struct PVRSRV_BRIDGE_RETURN *psRetOUT,
     struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfo;
	void *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SET_DISPCLASS_SRCRECT);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfo,
			       psSetDispClassSrcRectIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvSwapChain,
			       psSetDispClassSrcRectIN->hSwapChain,
			       PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVSetDCSrcRectKM(pvDispClassInfo,
				 pvSwapChain, &psSetDispClassSrcRectIN->sRect);

	return 0;
}

static int PVRSRVSetDCDstColourKeyBW(u32 ui32BridgeID,
       struct PVRSRV_BRIDGE_IN_SET_DISPCLASS_COLOURKEY *psSetDispClassColKeyIN,
       struct PVRSRV_BRIDGE_RETURN *psRetOUT,
       struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfo;
	void *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SET_DISPCLASS_DSTCOLOURKEY);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfo,
			       psSetDispClassColKeyIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvSwapChain,
			       psSetDispClassColKeyIN->hSwapChain,
			       PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVSetDCDstColourKeyKM(pvDispClassInfo,
				      pvSwapChain,
				      psSetDispClassColKeyIN->ui32CKColour);

	return 0;
}

static int PVRSRVSetDCSrcColourKeyBW(u32 ui32BridgeID,
	struct PVRSRV_BRIDGE_IN_SET_DISPCLASS_COLOURKEY *psSetDispClassColKeyIN,
	struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfo;
	void *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SET_DISPCLASS_SRCCOLOURKEY);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfo,
			       psSetDispClassColKeyIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvSwapChain,
			       psSetDispClassColKeyIN->hSwapChain,
			       PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVSetDCSrcColourKeyKM(pvDispClassInfo,
				      pvSwapChain,
				      psSetDispClassColKeyIN->ui32CKColour);

	return 0;
}

static int PVRSRVGetDCBuffersBW(u32 ui32BridgeID,
     struct PVRSRV_BRIDGE_IN_GET_DISPCLASS_BUFFERS *psGetDispClassBuffersIN,
     struct PVRSRV_BRIDGE_OUT_GET_DISPCLASS_BUFFERS *psGetDispClassBuffersOUT,
     struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfo;
	void *pvSwapChain;
	u32 i;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_GET_DISPCLASS_BUFFERS);

	NEW_HANDLE_BATCH_OR_ERROR(psGetDispClassBuffersOUT->eError, psPerProc,
				  PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS);

	psGetDispClassBuffersOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfo,
			       psGetDispClassBuffersIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);
	if (psGetDispClassBuffersOUT->eError != PVRSRV_OK)
		return 0;

	psGetDispClassBuffersOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvSwapChain,
			       psGetDispClassBuffersIN->hSwapChain,
			       PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN);
	if (psGetDispClassBuffersOUT->eError != PVRSRV_OK)
		return 0;

	psGetDispClassBuffersOUT->eError =
	    PVRSRVGetDCBuffersKM(pvDispClassInfo,
				 pvSwapChain,
				 &psGetDispClassBuffersOUT->ui32BufferCount,
				 psGetDispClassBuffersOUT->ahBuffer);
	if (psGetDispClassBuffersOUT->eError != PVRSRV_OK)
		return 0;

	PVR_ASSERT(psGetDispClassBuffersOUT->ui32BufferCount <=
		   PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS);

	for (i = 0; i < psGetDispClassBuffersOUT->ui32BufferCount; i++) {
		void *hBufferExt;

		PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			       &hBufferExt,
			       psGetDispClassBuffersOUT->ahBuffer[i],
			       PVRSRV_HANDLE_TYPE_DISP_BUFFER,
			       (enum PVRSRV_HANDLE_ALLOC_FLAG)
				       (PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE |
					PVRSRV_HANDLE_ALLOC_FLAG_SHARED),
			       psGetDispClassBuffersIN->hSwapChain);

		psGetDispClassBuffersOUT->ahBuffer[i] = hBufferExt;
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psGetDispClassBuffersOUT->eError,
				     psPerProc);

	return 0;
}

static int PVRSRVSwapToDCBufferBW(u32 ui32BridgeID,
      struct PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_BUFFER *psSwapDispClassBufferIN,
      struct PVRSRV_BRIDGE_RETURN *psRetOUT,
      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfo;
	void *pvSwapChainBuf;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_BUFFER);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					&pvDispClassInfo,
					psSwapDispClassBufferIN->hDeviceKM,
					PVRSRV_HANDLE_TYPE_DISP_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVLookupSubHandle(psPerProc->psHandleBase,
				  &pvSwapChainBuf,
				  psSwapDispClassBufferIN->hBuffer,
				  PVRSRV_HANDLE_TYPE_DISP_BUFFER,
				  psSwapDispClassBufferIN->hDeviceKM);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVSwapToDCBufferKM(pvDispClassInfo,
				   pvSwapChainBuf,
				   psSwapDispClassBufferIN->ui32SwapInterval,
				   psSwapDispClassBufferIN->hPrivateTag,
				   psSwapDispClassBufferIN->ui32ClipRectCount,
				   psSwapDispClassBufferIN->sClipRect);

	return 0;
}

static int PVRSRVSwapToDCSystemBW(u32 ui32BridgeID,
      struct PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_SYSTEM *psSwapDispClassSystemIN,
      struct PVRSRV_BRIDGE_RETURN *psRetOUT,
      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvDispClassInfo;
	void *pvSwapChain;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_SYSTEM);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvDispClassInfo,
			       psSwapDispClassSystemIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_DISP_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    PVRSRVLookupSubHandle(psPerProc->psHandleBase,
				  &pvSwapChain,
				  psSwapDispClassSystemIN->hSwapChain,
				  PVRSRV_HANDLE_TYPE_DISP_SWAP_CHAIN,
				  psSwapDispClassSystemIN->hDeviceKM);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;
	psRetOUT->eError = PVRSRVSwapToDCSystemKM(pvDispClassInfo, pvSwapChain);

	return 0;
}

static int PVRSRVOpenBCDeviceBW(u32 ui32BridgeID,
   struct PVRSRV_BRIDGE_IN_OPEN_BUFFERCLASS_DEVICE *psOpenBufferClassDeviceIN,
   struct PVRSRV_BRIDGE_OUT_OPEN_BUFFERCLASS_DEVICE *psOpenBufferClassDeviceOUT,
   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevCookieInt;
	void *hBufClassInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_OPEN_BUFFERCLASS_DEVICE);

	NEW_HANDLE_BATCH_OR_ERROR(psOpenBufferClassDeviceOUT->eError, psPerProc,
				  1);

	psOpenBufferClassDeviceOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &hDevCookieInt,
			       psOpenBufferClassDeviceIN->hDevCookie,
			       PVRSRV_HANDLE_TYPE_DEV_NODE);
	if (psOpenBufferClassDeviceOUT->eError != PVRSRV_OK)
		return 0;

	psOpenBufferClassDeviceOUT->eError =
	    PVRSRVOpenBCDeviceKM(psPerProc,
				 psOpenBufferClassDeviceIN->ui32DeviceID,
				 hDevCookieInt, &hBufClassInfo);
	if (psOpenBufferClassDeviceOUT->eError != PVRSRV_OK)
		return 0;

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			    &psOpenBufferClassDeviceOUT->hDeviceKM,
			    hBufClassInfo,
			    PVRSRV_HANDLE_TYPE_BUF_INFO,
			    PVRSRV_HANDLE_ALLOC_FLAG_NONE);

	COMMIT_HANDLE_BATCH_OR_ERROR(psOpenBufferClassDeviceOUT->eError,
				     psPerProc);

	return 0;
}

static int PVRSRVCloseBCDeviceBW(u32 ui32BridgeID,
   struct PVRSRV_BRIDGE_IN_CLOSE_BUFFERCLASS_DEVICE *psCloseBufferClassDeviceIN,
   struct PVRSRV_BRIDGE_RETURN *psRetOUT,
   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvBufClassInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_CLOSE_BUFFERCLASS_DEVICE);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvBufClassInfo,
			       psCloseBufferClassDeviceIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_BUF_INFO);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVCloseBCDeviceKM(pvBufClassInfo, IMG_FALSE);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVReleaseHandle(psPerProc->psHandleBase,
					       psCloseBufferClassDeviceIN->
					       hDeviceKM,
					       PVRSRV_HANDLE_TYPE_BUF_INFO);

	return 0;
}

static int PVRSRVGetBCInfoBW(u32 ui32BridgeID,
   struct PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_INFO *psGetBufferClassInfoIN,
   struct PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_INFO *psGetBufferClassInfoOUT,
   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvBufClassInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_GET_BUFFERCLASS_INFO);

	psGetBufferClassInfoOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvBufClassInfo,
			       psGetBufferClassInfoIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_BUF_INFO);
	if (psGetBufferClassInfoOUT->eError != PVRSRV_OK)
		return 0;

	psGetBufferClassInfoOUT->eError =
	    PVRSRVGetBCInfoKM(pvBufClassInfo,
			      &psGetBufferClassInfoOUT->sBufferInfo);
	return 0;
}

static int PVRSRVGetBCBufferBW(u32 ui32BridgeID,
    struct PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_BUFFER *psGetBufferClassBufferIN,
    struct PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_BUFFER *psGetBufferClassBufferOUT,
    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *pvBufClassInfo;
	void *hBufferInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_GET_BUFFERCLASS_BUFFER);

	NEW_HANDLE_BATCH_OR_ERROR(psGetBufferClassBufferOUT->eError, psPerProc,
				  1);

	psGetBufferClassBufferOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &pvBufClassInfo,
			       psGetBufferClassBufferIN->hDeviceKM,
			       PVRSRV_HANDLE_TYPE_BUF_INFO);
	if (psGetBufferClassBufferOUT->eError != PVRSRV_OK)
		return 0;

	psGetBufferClassBufferOUT->eError =
	    PVRSRVGetBCBufferKM(pvBufClassInfo,
				psGetBufferClassBufferIN->ui32BufferIndex,
				&hBufferInt);

	if (psGetBufferClassBufferOUT->eError != PVRSRV_OK)
		return 0;

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			       &psGetBufferClassBufferOUT->hBuffer,
			       hBufferInt,
			       PVRSRV_HANDLE_TYPE_BUF_BUFFER,
			       (enum PVRSRV_HANDLE_ALLOC_FLAG)
				       (PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE |
					PVRSRV_HANDLE_ALLOC_FLAG_SHARED),
			       psGetBufferClassBufferIN->hDeviceKM);

	COMMIT_HANDLE_BATCH_OR_ERROR(psGetBufferClassBufferOUT->eError,
				     psPerProc);

	return 0;
}

static int PVRSRVAllocSharedSysMemoryBW(u32 ui32BridgeID,
	  struct PVRSRV_BRIDGE_IN_ALLOC_SHARED_SYS_MEM *psAllocSharedSysMemIN,
	  struct PVRSRV_BRIDGE_OUT_ALLOC_SHARED_SYS_MEM *psAllocSharedSysMemOUT,
	  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	struct PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_ALLOC_SHARED_SYS_MEM);

	NEW_HANDLE_BATCH_OR_ERROR(psAllocSharedSysMemOUT->eError, psPerProc, 1);

	psAllocSharedSysMemOUT->eError =
	    PVRSRVAllocSharedSysMemoryKM(psPerProc,
					 psAllocSharedSysMemIN->ui32Flags,
					 psAllocSharedSysMemIN->ui32Size,
					 &psKernelMemInfo);
	if (psAllocSharedSysMemOUT->eError != PVRSRV_OK)
		return 0;

	OSMemSet(&psAllocSharedSysMemOUT->sClientMemInfo,
		 0, sizeof(psAllocSharedSysMemOUT->sClientMemInfo));

	psAllocSharedSysMemOUT->sClientMemInfo.pvLinAddrKM =
		    psKernelMemInfo->pvLinAddrKM;

	psAllocSharedSysMemOUT->sClientMemInfo.pvLinAddr = NULL;
	psAllocSharedSysMemOUT->sClientMemInfo.ui32Flags =
					psKernelMemInfo->ui32Flags;
	psAllocSharedSysMemOUT->sClientMemInfo.ui32AllocSize =
					psKernelMemInfo->ui32AllocSize;
	psAllocSharedSysMemOUT->sClientMemInfo.hMappingInfo =
					psKernelMemInfo->sMemBlk.hOSMemHandle;

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			 &psAllocSharedSysMemOUT->sClientMemInfo.hKernelMemInfo,
			 psKernelMemInfo,
			 PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO,
			 PVRSRV_HANDLE_ALLOC_FLAG_NONE);

	COMMIT_HANDLE_BATCH_OR_ERROR(psAllocSharedSysMemOUT->eError, psPerProc);

	return 0;
}

static int PVRSRVFreeSharedSysMemoryBW(u32 ui32BridgeID,
	    struct PVRSRV_BRIDGE_IN_FREE_SHARED_SYS_MEM *psFreeSharedSysMemIN,
	    struct PVRSRV_BRIDGE_OUT_FREE_SHARED_SYS_MEM *psFreeSharedSysMemOUT,
	    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	struct PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_FREE_SHARED_SYS_MEM);

	psFreeSharedSysMemOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       (void **)&psKernelMemInfo,
			       psFreeSharedSysMemIN->psKernelMemInfo,
			       PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO);

	if (psFreeSharedSysMemOUT->eError != PVRSRV_OK)
		return 0;

	psFreeSharedSysMemOUT->eError =
	    PVRSRVFreeSharedSysMemoryKM(psKernelMemInfo);
	if (psFreeSharedSysMemOUT->eError != PVRSRV_OK)
		return 0;

	psFreeSharedSysMemOUT->eError =
	    PVRSRVReleaseHandle(psPerProc->psHandleBase,
				psFreeSharedSysMemIN->psKernelMemInfo,
				PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO);
	return 0;
}

static int PVRSRVMapMemInfoMemBW(u32 ui32BridgeID,
	      struct PVRSRV_BRIDGE_IN_MAP_MEMINFO_MEM *psMapMemInfoMemIN,
	      struct PVRSRV_BRIDGE_OUT_MAP_MEMINFO_MEM *psMapMemInfoMemOUT,
	      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	struct PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;
	enum PVRSRV_HANDLE_TYPE eHandleType;
	void *hParent;
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_MAP_MEMINFO_MEM);

	NEW_HANDLE_BATCH_OR_ERROR(psMapMemInfoMemOUT->eError, psPerProc, 2);

	psMapMemInfoMemOUT->eError =
	    PVRSRVLookupHandleAnyType(psPerProc->psHandleBase,
				      (void **)&psKernelMemInfo, &eHandleType,
				      psMapMemInfoMemIN->hKernelMemInfo);
	if (psMapMemInfoMemOUT->eError != PVRSRV_OK)
		return 0;

	switch (eHandleType) {
	case PVRSRV_HANDLE_TYPE_MEM_INFO:
	case PVRSRV_HANDLE_TYPE_MEM_INFO_REF:
	case PVRSRV_HANDLE_TYPE_SHARED_SYS_MEM_INFO:
		break;
	default:
		psMapMemInfoMemOUT->eError = PVRSRV_ERROR_GENERIC;
		return 0;
	}

	psMapMemInfoMemOUT->eError =
	    PVRSRVGetParentHandle(psPerProc->psHandleBase, &hParent,
				  psMapMemInfoMemIN->hKernelMemInfo,
				  eHandleType);
	if (psMapMemInfoMemOUT->eError != PVRSRV_OK)
		return 0;
	if (hParent == NULL)
		hParent = psMapMemInfoMemIN->hKernelMemInfo;

	OSMemSet(&psMapMemInfoMemOUT->sClientMemInfo,
		 0, sizeof(psMapMemInfoMemOUT->sClientMemInfo));

	psMapMemInfoMemOUT->sClientMemInfo.pvLinAddrKM =
					psKernelMemInfo->pvLinAddrKM;

	psMapMemInfoMemOUT->sClientMemInfo.pvLinAddr = NULL;
	psMapMemInfoMemOUT->sClientMemInfo.sDevVAddr =
					psKernelMemInfo->sDevVAddr;
	psMapMemInfoMemOUT->sClientMemInfo.ui32Flags =
					psKernelMemInfo->ui32Flags;
	psMapMemInfoMemOUT->sClientMemInfo.ui32AllocSize =
					psKernelMemInfo->ui32AllocSize;
	psMapMemInfoMemOUT->sClientMemInfo.hMappingInfo =
					psKernelMemInfo->sMemBlk.hOSMemHandle;

	PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			&psMapMemInfoMemOUT->sClientMemInfo.hKernelMemInfo,
			psKernelMemInfo,
			PVRSRV_HANDLE_TYPE_MEM_INFO_REF,
			PVRSRV_HANDLE_ALLOC_FLAG_MULTI, hParent);

	if (psKernelMemInfo->ui32Flags & PVRSRV_MEM_NO_SYNCOBJ) {
		OSMemSet(&psMapMemInfoMemOUT->sClientSyncInfo, 0,
		sizeof(struct PVRSRV_CLIENT_SYNC_INFO));
		psMapMemInfoMemOUT->psKernelSyncInfo = NULL;
	} else {
		psMapMemInfoMemOUT->sClientSyncInfo.psSyncData =
		    psKernelMemInfo->psKernelSyncInfo->psSyncData;
		psMapMemInfoMemOUT->sClientSyncInfo.sWriteOpsCompleteDevVAddr =
		    psKernelMemInfo->psKernelSyncInfo->
						    sWriteOpsCompleteDevVAddr;
		psMapMemInfoMemOUT->sClientSyncInfo.sReadOpsCompleteDevVAddr =
		    psKernelMemInfo->psKernelSyncInfo->sReadOpsCompleteDevVAddr;

		psMapMemInfoMemOUT->sClientSyncInfo.hMappingInfo =
		    psKernelMemInfo->psKernelSyncInfo->psSyncDataMemInfoKM->
						    sMemBlk.hOSMemHandle;

		psMapMemInfoMemOUT->sClientMemInfo.psClientSyncInfo =
		    &psMapMemInfoMemOUT->sClientSyncInfo;

		PVRSRVAllocSubHandleNR(psPerProc->psHandleBase,
			&psMapMemInfoMemOUT->sClientSyncInfo.hKernelSyncInfo,
			psKernelMemInfo->psKernelSyncInfo,
			PVRSRV_HANDLE_TYPE_SYNC_INFO,
			PVRSRV_HANDLE_ALLOC_FLAG_MULTI,
			psMapMemInfoMemOUT->sClientMemInfo.hKernelMemInfo);
	}

	COMMIT_HANDLE_BATCH_OR_ERROR(psMapMemInfoMemOUT->eError, psPerProc);

	return 0;
}

static int PVRSRVModifySyncOpsBW(u32 ui32BridgeID,
		 struct PVRSRV_BRIDGE_IN_MODIFY_SYNC_OPS *psModifySyncOpsIN,
		 struct PVRSRV_BRIDGE_OUT_MODIFY_SYNC_OPS *psModifySyncOpsOUT,
		 struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hKernelSyncInfo;
	struct PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_MODIFY_SYNC_OPS);

	psModifySyncOpsOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
					&hKernelSyncInfo,
					psModifySyncOpsIN->hKernelSyncInfo,
					PVRSRV_HANDLE_TYPE_SYNC_INFO);
	if (psModifySyncOpsOUT->eError != PVRSRV_OK)
		return 0;

	psKernelSyncInfo = (struct PVRSRV_KERNEL_SYNC_INFO *)hKernelSyncInfo;

	/* We return PRE-INCREMENTED versions of all sync Op Values */

	psModifySyncOpsOUT->ui32ReadOpsPending =
	    psKernelSyncInfo->psSyncData->ui32ReadOpsPending;

	psModifySyncOpsOUT->ui32WriteOpsPending =
	    psKernelSyncInfo->psSyncData->ui32WriteOpsPending;

	psModifySyncOpsOUT->ui32ReadOpsComplete =
	    psKernelSyncInfo->psSyncData->ui32ReadOpsComplete;

	psModifySyncOpsOUT->ui32WriteOpsComplete =
	    psKernelSyncInfo->psSyncData->ui32WriteOpsComplete;

	if (psModifySyncOpsIN->ui32ModifyFlags &
	    PVRSRV_MODIFYSYNCOPS_FLAGS_WOP_INC)
		psKernelSyncInfo->psSyncData->ui32WriteOpsPending++;

	if (psModifySyncOpsIN->ui32ModifyFlags &
	    PVRSRV_MODIFYSYNCOPS_FLAGS_ROP_INC)
		psKernelSyncInfo->psSyncData->ui32ReadOpsPending++;

	if (psModifySyncOpsIN->ui32ModifyFlags &
	    PVRSRV_MODIFYSYNCOPS_FLAGS_WOC_INC)
		psKernelSyncInfo->psSyncData->ui32WriteOpsComplete++;

	if (psModifySyncOpsIN->ui32ModifyFlags &
	    PVRSRV_MODIFYSYNCOPS_FLAGS_ROC_INC)
		psKernelSyncInfo->psSyncData->ui32ReadOpsComplete++;

	return 0;
}

static int MMU_GetPDDevPAddrBW(u32 ui32BridgeID,
	    struct PVRSRV_BRIDGE_IN_GETMMU_PD_DEVPADDR *psGetMmuPDDevPAddrIN,
	    struct PVRSRV_BRIDGE_OUT_GETMMU_PD_DEVPADDR *psGetMmuPDDevPAddrOUT,
	    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hDevMemContextInt;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_GETMMU_PD_DEVPADDR);

	psGetMmuPDDevPAddrOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase, &hDevMemContextInt,
			       psGetMmuPDDevPAddrIN->hDevMemContext,
			       PVRSRV_HANDLE_TYPE_DEV_MEM_CONTEXT);
	if (psGetMmuPDDevPAddrOUT->eError != PVRSRV_OK)
		return 0;

	psGetMmuPDDevPAddrOUT->sPDDevPAddr =
	    BM_GetDeviceNode(hDevMemContextInt)->
	    pfnMMUGetPDDevPAddr(BM_GetMMUContextFromMemContext
			      (hDevMemContextInt));
	if (psGetMmuPDDevPAddrOUT->sPDDevPAddr.uiAddr)
		psGetMmuPDDevPAddrOUT->eError = PVRSRV_OK;
	else
		psGetMmuPDDevPAddrOUT->eError = PVRSRV_ERROR_GENERIC;
	return 0;
}

int DummyBW(u32 ui32BridgeID, void *psBridgeIn, void *psBridgeOut,
	    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
#if !defined(CONFIG_PVR_DEBUG_EXTRA)
	PVR_UNREFERENCED_PARAMETER(ui32BridgeID);
#endif
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);
	PVR_UNREFERENCED_PARAMETER(psBridgeOut);
	PVR_UNREFERENCED_PARAMETER(psPerProc);

#if defined(DEBUG_BRIDGE_KM)
	PVR_DPF(PVR_DBG_ERROR, "%s: BRIDGE ERROR: BridgeID %lu (%s) mapped to "
		 "Dummy Wrapper (probably not what you want!)",
		 __func__, ui32BridgeID,
		 g_BridgeDispatchTable[ui32BridgeID].pszIOCName);
#else
	PVR_DPF(PVR_DBG_ERROR, "%s: BRIDGE ERROR: BridgeID %lu mapped to "
		 "Dummy Wrapper (probably not what you want!)",
		 __func__, ui32BridgeID);
#endif
	return -ENOTTY;
}

void _SetDispatchTableEntry(u32 ui32Index, const char *pszIOCName,
			    int (*pfFunction)(u32 ui32BridgeID,
					      void *psBridgeIn,
					      void *psBridgeOut,
					      struct PVRSRV_PER_PROCESS_DATA
								   *psPerProc),
			    const char *pszFunctionName)
{
	static u32 ui32PrevIndex = ~0UL;
#if !defined(CONFIG_PVR_DEBUG_EXTRA)
	PVR_UNREFERENCED_PARAMETER(pszIOCName);
#endif
#if !defined(DEBUG_BRIDGE_KM_DISPATCH_TABLE) && !defined(DEBUG_BRIDGE_KM)
	PVR_UNREFERENCED_PARAMETER(pszFunctionName);
#endif


	if (g_BridgeDispatchTable[ui32Index].pfFunction) {
#if defined(DEBUG_BRIDGE_KM)
		PVR_DPF(PVR_DBG_ERROR, "%s: BUG!: "
			"Adding dispatch table entry for %s "
			"clobbers an existing entry for %s",
			 __func__, pszIOCName,
			 g_BridgeDispatchTable[ui32Index].pszIOCName);
#else
		PVR_DPF(PVR_DBG_ERROR, "%s: BUG!: "
			"Adding dispatch table entry for %s "
			"clobbers an existing entry (index=%lu)",
			 __func__, pszIOCName, ui32Index);
#endif
		PVR_DPF(PVR_DBG_ERROR,
"NOTE: Enabling DEBUG_BRIDGE_KM_DISPATCH_TABLE may help debug this issue.",
			 __func__);
	}

	if ((ui32PrevIndex != ~0UL) &&
	    ((ui32Index >= ui32PrevIndex + DISPATCH_TABLE_GAP_THRESHOLD) ||
	     (ui32Index <= ui32PrevIndex))) {
#if defined(DEBUG_BRIDGE_KM)
		PVR_DPF(PVR_DBG_WARNING,
			 "%s: There is a gap in the dispatch table "
			 "between indices %lu (%s) and %lu (%s)",
			 __func__, ui32PrevIndex,
			 g_BridgeDispatchTable[ui32PrevIndex].pszIOCName,
			 ui32Index, pszIOCName);
#else
		PVR_DPF(PVR_DBG_WARNING,
			 "%s: There is a gap in the dispatch table "
			"between indices %u and %u (%s)",
			 __func__, (unsigned)ui32PrevIndex, (unsigned)ui32Index,
			 pszIOCName);
#endif
		PVR_DPF(PVR_DBG_ERROR,
			"NOTE: Enabling DEBUG_BRIDGE_KM_DISPATCH_TABLE "
			"may help debug this issue.",
			 __func__);
	}

	g_BridgeDispatchTable[ui32Index].pfFunction = pfFunction;
#if defined(DEBUG_BRIDGE_KM)
	g_BridgeDispatchTable[ui32Index].pszIOCName = pszIOCName;
	g_BridgeDispatchTable[ui32Index].pszFunctionName = pszFunctionName;
	g_BridgeDispatchTable[ui32Index].ui32CallCount = 0;
	g_BridgeDispatchTable[ui32Index].ui32CopyFromUserTotalBytes = 0;
#endif

	ui32PrevIndex = ui32Index;
}

static int PVRSRVInitSrvConnectBW(u32 ui32BridgeID, void *psBridgeIn,
				  struct PVRSRV_BRIDGE_RETURN *psRetOUT,
				  struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_INITSRV_CONNECT);
	PVR_UNREFERENCED_PARAMETER(psBridgeIn);

	if (!OSProcHasPrivSrvInit() ||
	    PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_RUNNING) ||
	    PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_RAN)) {
		psRetOUT->eError = PVRSRV_ERROR_GENERIC;
		return 0;
	}

	PVRSRVSetInitServerState(PVRSRV_INIT_SERVER_RUNNING, IMG_TRUE);
	psPerProc->bInitProcess = IMG_TRUE;

	psRetOUT->eError = PVRSRV_OK;

	return 0;
}

static int PVRSRVInitSrvDisconnectBW(u32 ui32BridgeID,
	     struct PVRSRV_BRIDGE_IN_INITSRV_DISCONNECT *psInitSrvDisconnectIN,
	     struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	     struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_INITSRV_DISCONNECT);

	if (!psPerProc->bInitProcess) {
		psRetOUT->eError = PVRSRV_ERROR_GENERIC;
		return 0;
	}

	psPerProc->bInitProcess = IMG_FALSE;

	PVRSRVSetInitServerState(PVRSRV_INIT_SERVER_RUNNING, IMG_FALSE);
	PVRSRVSetInitServerState(PVRSRV_INIT_SERVER_RAN, IMG_TRUE);

	psRetOUT->eError =
	    PVRSRVFinaliseSystem(psInitSrvDisconnectIN->bInitSuccesful);

	PVRSRVSetInitServerState(PVRSRV_INIT_SERVER_SUCCESSFUL,
				 (IMG_BOOL)(((psRetOUT->eError == PVRSRV_OK) &&
					      (psInitSrvDisconnectIN->
							    bInitSuccesful))));

	return 0;
}

static int PVRSRVEventObjectWaitBW(u32 ui32BridgeID,
	   struct PVRSRV_BRIDGE_IN_EVENT_OBJECT_WAIT *psEventObjectWaitIN,
	   struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hOSEventKM;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_EVENT_OBJECT_WAIT);

	psRetOUT->eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				      &hOSEventKM,
				      psEventObjectWaitIN->hOSEventKM,
				      PVRSRV_HANDLE_TYPE_EVENT_OBJECT_CONNECT);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = OSEventObjectWait(hOSEventKM);

	return 0;
}

static int PVRSRVEventObjectOpenBW(u32 ui32BridgeID,
	   struct PVRSRV_BRIDGE_IN_EVENT_OBJECT_OPEN *psEventObjectOpenIN,
	   struct PVRSRV_BRIDGE_OUT_EVENT_OBJECT_OPEN *psEventObjectOpenOUT,
	   struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID, PVRSRV_BRIDGE_EVENT_OBJECT_OPEN);

	NEW_HANDLE_BATCH_OR_ERROR(psEventObjectOpenOUT->eError, psPerProc, 1);

	psEventObjectOpenOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &psEventObjectOpenIN->sEventObject.hOSEventKM,
			       psEventObjectOpenIN->sEventObject.hOSEventKM,
			       PVRSRV_HANDLE_TYPE_SHARED_EVENT_OBJECT);

	if (psEventObjectOpenOUT->eError != PVRSRV_OK)
		return 0;

	psEventObjectOpenOUT->eError =
	    OSEventObjectOpen(&psEventObjectOpenIN->sEventObject,
			      &psEventObjectOpenOUT->hOSEvent);

	if (psEventObjectOpenOUT->eError != PVRSRV_OK)
		return 0;

	PVRSRVAllocHandleNR(psPerProc->psHandleBase,
			    &psEventObjectOpenOUT->hOSEvent,
			    psEventObjectOpenOUT->hOSEvent,
			    PVRSRV_HANDLE_TYPE_EVENT_OBJECT_CONNECT,
			    PVRSRV_HANDLE_ALLOC_FLAG_MULTI);

	COMMIT_HANDLE_BATCH_OR_ERROR(psEventObjectOpenOUT->eError, psPerProc);

	return 0;
}

static int PVRSRVEventObjectCloseBW(u32 ui32BridgeID,
	    struct PVRSRV_BRIDGE_IN_EVENT_OBJECT_CLOSE *psEventObjectCloseIN,
	    struct PVRSRV_BRIDGE_RETURN *psRetOUT,
	    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	void *hOSEventKM;

	PVRSRV_BRIDGE_ASSERT_CMD(ui32BridgeID,
				 PVRSRV_BRIDGE_EVENT_OBJECT_CLOSE);

	psRetOUT->eError =
	    PVRSRVLookupHandle(psPerProc->psHandleBase,
			       &psEventObjectCloseIN->sEventObject.hOSEventKM,
			       psEventObjectCloseIN->sEventObject.hOSEventKM,
			       PVRSRV_HANDLE_TYPE_SHARED_EVENT_OBJECT);
	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError = PVRSRVLookupAndReleaseHandle(psPerProc->psHandleBase,
				&hOSEventKM,
				psEventObjectCloseIN->hOSEventKM,
				PVRSRV_HANDLE_TYPE_EVENT_OBJECT_CONNECT);

	if (psRetOUT->eError != PVRSRV_OK)
		return 0;

	psRetOUT->eError =
	    OSEventObjectClose(&psEventObjectCloseIN->sEventObject, hOSEventKM);

	return 0;
}

enum PVRSRV_ERROR CommonBridgeInit(void)
{
	u32 i;

	SetDispatchTableEntry(PVRSRV_BRIDGE_ENUM_DEVICES,
			      PVRSRVEnumerateDevicesBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_ACQUIRE_DEVICEINFO,
			      PVRSRVAcquireDeviceDataBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_RELEASE_DEVICEINFO, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CREATE_DEVMEMCONTEXT,
			      PVRSRVCreateDeviceMemContextBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_DESTROY_DEVMEMCONTEXT,
			      PVRSRVDestroyDeviceMemContextBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_DEVMEM_HEAPINFO,
			      PVRSRVGetDeviceMemHeapInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_ALLOC_DEVICEMEM,
			      PVRSRVAllocDeviceMemBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_FREE_DEVICEMEM,
			      PVRSRVFreeDeviceMemBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GETFREE_DEVICEMEM,
			      PVRSRVGetFreeDeviceMemBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CREATE_COMMANDQUEUE, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_DESTROY_COMMANDQUEUE, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MHANDLE_TO_MMAP_DATA,
			      PVRMMapOSMemHandleToMMapDataBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CONNECT_SERVICES, PVRSRVConnectBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_DISCONNECT_SERVICES,
			      PVRSRVDisconnectBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_WRAP_DEVICE_MEM, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_DEVICEMEMINFO, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_RESERVE_DEV_VIRTMEM, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_FREE_DEV_VIRTMEM, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MAP_EXT_MEMORY, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNMAP_EXT_MEMORY, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MAP_DEV_MEMORY,
			      PVRSRVMapDeviceMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNMAP_DEV_MEMORY,
			      PVRSRVUnmapDeviceMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MAP_DEVICECLASS_MEMORY,
			      PVRSRVMapDeviceClassMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNMAP_DEVICECLASS_MEMORY,
			      PVRSRVUnmapDeviceClassMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MAP_MEM_INFO_TO_USER, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNMAP_MEM_INFO_FROM_USER, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_EXPORT_DEVICEMEM,
			      PVRSRVExportDeviceMemBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_RELEASE_MMAP_DATA,
			      PVRMMapReleaseMMapDataBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CACHE_FLUSH_DRM,
			      PVRSRVCacheFlushDRIBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_PROCESS_SIMISR_EVENT, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_REGISTER_SIM_PROCESS, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNREGISTER_SIM_PROCESS, DummyBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_MAPPHYSTOUSERSPACE, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNMAPPHYSTOUSERSPACE, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GETPHYSTOUSERSPACEMAP, DummyBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_FB_STATS, DummyBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_MISC_INFO, PVRSRVGetMiscInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_RELEASE_MISC_INFO, DummyBW);


#if defined(PDUMP)
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_INIT, DummyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_MEMPOL, PDumpMemPolBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DUMPMEM, PDumpMemBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_REG, PDumpRegWithFlagsBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_REGPOL, PDumpRegPolBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_COMMENT, PDumpCommentBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_SETFRAME, PDumpSetFrameBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_ISCAPTURING,
			      PDumpIsCaptureFrameBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DUMPBITMAP, PDumpBitmapBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DUMPREADREG, PDumpReadRegBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_SYNCPOL, PDumpSyncPolBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DUMPSYNC, PDumpSyncDumpBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DRIVERINFO,
			      PDumpDriverInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_PDREG, PDumpPDRegBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_DUMPPDDEVPADDR,
			      PDumpPDDevPAddrBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_CYCLE_COUNT_REG_READ,
			      PDumpCycleCountRegReadBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_STARTINITPHASE,
			      PDumpStartInitPhaseBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_PDUMP_STOPINITPHASE,
			      PDumpStopInitPhaseBW);
#endif

	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_OEMJTABLE, DummyBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_ENUM_CLASS, PVRSRVEnumerateDCBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_OPEN_DISPCLASS_DEVICE,
			      PVRSRVOpenDCDeviceBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CLOSE_DISPCLASS_DEVICE,
			      PVRSRVCloseDCDeviceBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_ENUM_DISPCLASS_FORMATS,
			      PVRSRVEnumDCFormatsBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_ENUM_DISPCLASS_DIMS,
			      PVRSRVEnumDCDimsBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_DISPCLASS_SYSBUFFER,
			      PVRSRVGetDCSystemBufferBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_DISPCLASS_INFO,
			      PVRSRVGetDCInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CREATE_DISPCLASS_SWAPCHAIN,
			      PVRSRVCreateDCSwapChainBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_DESTROY_DISPCLASS_SWAPCHAIN,
			      PVRSRVDestroyDCSwapChainBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SET_DISPCLASS_DSTRECT,
			      PVRSRVSetDCDstRectBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SET_DISPCLASS_SRCRECT,
			      PVRSRVSetDCSrcRectBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SET_DISPCLASS_DSTCOLOURKEY,
			      PVRSRVSetDCDstColourKeyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SET_DISPCLASS_SRCCOLOURKEY,
			      PVRSRVSetDCSrcColourKeyBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_DISPCLASS_BUFFERS,
			      PVRSRVGetDCBuffersBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_BUFFER,
			      PVRSRVSwapToDCBufferBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_SYSTEM,
			      PVRSRVSwapToDCSystemBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_OPEN_BUFFERCLASS_DEVICE,
			      PVRSRVOpenBCDeviceBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_CLOSE_BUFFERCLASS_DEVICE,
			      PVRSRVCloseBCDeviceBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_BUFFERCLASS_INFO,
			      PVRSRVGetBCInfoBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_GET_BUFFERCLASS_BUFFER,
			      PVRSRVGetBCBufferBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_WRAP_EXT_MEMORY,
			      PVRSRVWrapExtMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_UNWRAP_EXT_MEMORY,
			      PVRSRVUnwrapExtMemoryBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_ALLOC_SHARED_SYS_MEM,
			      PVRSRVAllocSharedSysMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_FREE_SHARED_SYS_MEM,
			      PVRSRVFreeSharedSysMemoryBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_MAP_MEMINFO_MEM,
			      PVRSRVMapMemInfoMemBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_GETMMU_PD_DEVPADDR,
			      MMU_GetPDDevPAddrBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_INITSRV_CONNECT,
			      PVRSRVInitSrvConnectBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_INITSRV_DISCONNECT,
			      PVRSRVInitSrvDisconnectBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_EVENT_OBJECT_WAIT,
			      PVRSRVEventObjectWaitBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_EVENT_OBJECT_OPEN,
			      PVRSRVEventObjectOpenBW);
	SetDispatchTableEntry(PVRSRV_BRIDGE_EVENT_OBJECT_CLOSE,
			      PVRSRVEventObjectCloseBW);

	SetDispatchTableEntry(PVRSRV_BRIDGE_MODIFY_SYNC_OPS,
			      PVRSRVModifySyncOpsBW);

	SetSGXDispatchTableEntry();

	for (i = 0; i < BRIDGE_DISPATCH_TABLE_ENTRY_COUNT; i++)
		if (!g_BridgeDispatchTable[i].pfFunction) {
			g_BridgeDispatchTable[i].pfFunction = DummyBW;
#if defined(DEBUG_BRIDGE_KM)
			g_BridgeDispatchTable[i].pszIOCName =
						    "_PVRSRV_BRIDGE_DUMMY";
			g_BridgeDispatchTable[i].pszFunctionName = "DummyBW";
			g_BridgeDispatchTable[i].ui32CallCount = 0;
			g_BridgeDispatchTable[i].ui32CopyFromUserTotalBytes = 0;
			g_BridgeDispatchTable[i].ui32CopyToUserTotalBytes = 0;
#endif
		}

	return PVRSRV_OK;
}

static int bridged_check_cmd(u32 cmd_id)
{
	if (PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_RAN)) {
		if (!PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_SUCCESSFUL)) {
			PVR_DPF(PVR_DBG_ERROR,
			"%s: Initialisation failed.  Driver unusable.",
				 __func__);
			return 1;
		}
	} else {
		if (PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_RUNNING)) {
			PVR_DPF(PVR_DBG_ERROR,
				 "%s: Initialisation is in progress",
				 __func__);
			return 1;
		} else {
			switch (cmd_id) {
			case PVRSRV_GET_BRIDGE_ID(
				PVRSRV_BRIDGE_CONNECT_SERVICES):
			case PVRSRV_GET_BRIDGE_ID(
				PVRSRV_BRIDGE_DISCONNECT_SERVICES):
			case PVRSRV_GET_BRIDGE_ID(
				PVRSRV_BRIDGE_INITSRV_CONNECT):
			case PVRSRV_GET_BRIDGE_ID(
				PVRSRV_BRIDGE_INITSRV_DISCONNECT):
				break;
			default:
				PVR_DPF(PVR_DBG_ERROR,
			"%s: Driver initialisation not completed yet.",
					 __func__);
				return 1;
			}
		}
	}

	return 0;
}

static int bridged_ioctl(struct file *filp, u32 cmd, void *in, void *out,
			 size_t in_size,
			 struct PVRSRV_PER_PROCESS_DATA *per_proc)
{
	int err = -EFAULT;

	switch (PVRSRV_IOWR(cmd)) {
	case PVRSRV_BRIDGE_ENUM_DEVICES:
		err = PVRSRVEnumerateDevicesBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_ACQUIRE_DEVICEINFO:
		err = PVRSRVAcquireDeviceDataBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_RELEASE_DEVICEINFO:
		err = DummyBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_CREATE_DEVMEMCONTEXT:
		err = PVRSRVCreateDeviceMemContextBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_DESTROY_DEVMEMCONTEXT:
		err = PVRSRVDestroyDeviceMemContextBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_GET_DEVMEM_HEAPINFO:
		err = PVRSRVGetDeviceMemHeapInfoBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_ALLOC_DEVICEMEM:
		err = PVRSRVAllocDeviceMemBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_FREE_DEVICEMEM:
		err = PVRSRVFreeDeviceMemBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_GETFREE_DEVICEMEM:
		err = PVRSRVGetFreeDeviceMemBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_CREATE_COMMANDQUEUE:
	case PVRSRV_BRIDGE_DESTROY_COMMANDQUEUE:
		err = DummyBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_MHANDLE_TO_MMAP_DATA:
		err = PVRMMapOSMemHandleToMMapDataBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_CONNECT_SERVICES:
		err = PVRSRVConnectBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_DISCONNECT_SERVICES:
		err = PVRSRVDisconnectBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_WRAP_DEVICE_MEM:
	case PVRSRV_BRIDGE_GET_DEVICEMEMINFO:
	case PVRSRV_BRIDGE_RESERVE_DEV_VIRTMEM:
	case PVRSRV_BRIDGE_FREE_DEV_VIRTMEM:
	case PVRSRV_BRIDGE_MAP_EXT_MEMORY:
	case PVRSRV_BRIDGE_UNMAP_EXT_MEMORY:
		err = DummyBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_MAP_DEV_MEMORY:
		err = PVRSRVMapDeviceMemoryBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_UNMAP_DEV_MEMORY:
		err = PVRSRVUnmapDeviceMemoryBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_MAP_DEVICECLASS_MEMORY:
		err = PVRSRVMapDeviceClassMemoryBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_UNMAP_DEVICECLASS_MEMORY:
		err = PVRSRVUnmapDeviceClassMemoryBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_MAP_MEM_INFO_TO_USER:
	case PVRSRV_BRIDGE_UNMAP_MEM_INFO_FROM_USER:
		err = DummyBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_EXPORT_DEVICEMEM:
		err = PVRSRVExportDeviceMemBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_RELEASE_MMAP_DATA:
		err = PVRMMapReleaseMMapDataBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_CACHE_FLUSH_DRM:
		err = PVRSRVCacheFlushDRIBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_PROCESS_SIMISR_EVENT:
	case PVRSRV_BRIDGE_REGISTER_SIM_PROCESS:
	case PVRSRV_BRIDGE_UNREGISTER_SIM_PROCESS:
	case PVRSRV_BRIDGE_MAPPHYSTOUSERSPACE:
	case PVRSRV_BRIDGE_UNMAPPHYSTOUSERSPACE:
	case PVRSRV_BRIDGE_GETPHYSTOUSERSPACEMAP:
	case PVRSRV_BRIDGE_GET_FB_STATS:
		err = DummyBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_GET_MISC_INFO:
		err = PVRSRVGetMiscInfoBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_RELEASE_MISC_INFO:
		err = DummyBW(cmd, in, out, per_proc);
		break;

#if defined(PDUMP)
	case PVRSRV_BRIDGE_PDUMP_INIT:
		err = DummyBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_MEMPOL:
		err = PDumpMemPolBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_DUMPMEM:
		err = PDumpMemBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_REG:
		err = PDumpRegWithFlagsBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_REGPOL:
		err = PDumpRegPolBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_COMMENT:
		err = PDumpCommentBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_SETFRAME:
		err = PDumpSetFrameBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_ISCAPTURING:
		err = PDumpIsCaptureFrameBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_DUMPBITMAP:
		err = PDumpBitmapBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_DUMPREADREG:
		err = PDumpReadRegBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_SYNCPOL:
		err = PDumpSyncPolBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_DUMPSYNC:
		err = PDumpSyncDumpBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_DRIVERINFO:
		err = PDumpDriverInfoBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_PDREG:
		err = PDumpPDRegBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_DUMPPDDEVPADDR:
		err = PDumpPDDevPAddrBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_CYCLE_COUNT_REG_READ:
		err = PDumpCycleCountRegReadBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_STARTINITPHASE:
		err = PDumpStartInitPhaseBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_PDUMP_STOPINITPHASE:
		err = PDumpStopInitPhaseBW(cmd, in, out, per_proc);
		break;
#endif

	case PVRSRV_BRIDGE_GET_OEMJTABLE:
		err = DummyBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_ENUM_CLASS:
		err = PVRSRVEnumerateDCBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_OPEN_DISPCLASS_DEVICE:
		err = PVRSRVOpenDCDeviceBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_CLOSE_DISPCLASS_DEVICE:
		err = PVRSRVCloseDCDeviceBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_ENUM_DISPCLASS_FORMATS:
		err = PVRSRVEnumDCFormatsBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_ENUM_DISPCLASS_DIMS:
		err = PVRSRVEnumDCDimsBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_GET_DISPCLASS_SYSBUFFER:
		err = PVRSRVGetDCSystemBufferBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_GET_DISPCLASS_INFO:
		err = PVRSRVGetDCInfoBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_CREATE_DISPCLASS_SWAPCHAIN:
		err = PVRSRVCreateDCSwapChainBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_DESTROY_DISPCLASS_SWAPCHAIN:
		err = PVRSRVDestroyDCSwapChainBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SET_DISPCLASS_DSTRECT:
		err = PVRSRVSetDCDstRectBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SET_DISPCLASS_SRCRECT:
		err = PVRSRVSetDCSrcRectBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SET_DISPCLASS_DSTCOLOURKEY:
		err = PVRSRVSetDCDstColourKeyBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SET_DISPCLASS_SRCCOLOURKEY:
		err = PVRSRVSetDCSrcColourKeyBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_GET_DISPCLASS_BUFFERS:
		err = PVRSRVGetDCBuffersBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_BUFFER:
		err = PVRSRVSwapToDCBufferBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_SYSTEM:
		err = PVRSRVSwapToDCSystemBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_OPEN_BUFFERCLASS_DEVICE:
		err = PVRSRVOpenBCDeviceBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_CLOSE_BUFFERCLASS_DEVICE:
		err = PVRSRVCloseBCDeviceBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_GET_BUFFERCLASS_INFO:
		err = PVRSRVGetBCInfoBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_GET_BUFFERCLASS_BUFFER:
		err = PVRSRVGetBCBufferBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_WRAP_EXT_MEMORY:
		err = PVRSRVWrapExtMemoryBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_UNWRAP_EXT_MEMORY:
		err = PVRSRVUnwrapExtMemoryBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_ALLOC_SHARED_SYS_MEM:
		err = PVRSRVAllocSharedSysMemoryBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_FREE_SHARED_SYS_MEM:
		err = PVRSRVFreeSharedSysMemoryBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_MAP_MEMINFO_MEM:
		err = PVRSRVMapMemInfoMemBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_GETMMU_PD_DEVPADDR:
		err = MMU_GetPDDevPAddrBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_INITSRV_CONNECT:
		err = PVRSRVInitSrvConnectBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_INITSRV_DISCONNECT:
		err = PVRSRVInitSrvDisconnectBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_EVENT_OBJECT_WAIT:
		err = PVRSRVEventObjectWaitBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_EVENT_OBJECT_OPEN:
		err = PVRSRVEventObjectOpenBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_EVENT_OBJECT_CLOSE:
		err = PVRSRVEventObjectCloseBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_MODIFY_SYNC_OPS:
		err = PVRSRVModifySyncOpsBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_SGX_GETCLIENTINFO:
		err = SGXGetClientInfoBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_RELEASECLIENTINFO:
		err = SGXReleaseClientInfoBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_GETINTERNALDEVINFO:
		err = SGXGetInternalDevInfoBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_DOKICK:
		err = SGXDoKickBW(cmd, in, out, in_size, per_proc);
		break;

	case PVRSRV_BRIDGE_SGX_GETPHYSPAGEADDR:
	case PVRSRV_BRIDGE_SGX_READREGISTRYDWORD:
	case PVRSRV_BRIDGE_SGX_SCHEDULECOMMAND:
		err = DummyBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_SGX_2DQUERYBLTSCOMPLETE:
		err = SGX2DQueryBlitsCompleteBW(filp, cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_SGX_GETMMUPDADDR:
		err = DummyBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_SGX_SUBMITTRANSFER:
		err = SGXSubmitTransferBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_GETMISCINFO:
		err = SGXGetMiscInfoBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGXINFO_FOR_SRVINIT:
		err = SGXGetInfoForSrvinitBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_DEVINITPART2:
		err = SGXDevInitPart2BW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_SGX_FINDSHAREDPBDESC:
		err = SGXFindSharedPBDescBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_UNREFSHAREDPBDESC:
		err = SGXUnrefSharedPBDescBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_ADDSHAREDPBDESC:
		err = SGXAddSharedPBDescBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_REGISTER_HW_RENDER_CONTEXT:
		err = SGXRegisterHWRenderContextBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_FLUSH_HW_RENDER_TARGET:
		err = SGXFlushHWRenderTargetBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_UNREGISTER_HW_RENDER_CONTEXT:
		err = SGXUnregisterHWRenderContextBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_REGISTER_HW_TRANSFER_CONTEXT:
		err = SGXRegisterHWTransferContextBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_UNREGISTER_HW_TRANSFER_CONTEXT:
		err = SGXUnregisterHWTransferContextBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_SGX_READ_DIFF_COUNTERS:
		err = SGXReadDiffCountersBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_READ_HWPERF_CB:
		err = SGXReadHWPerfCBBW(cmd, in, out, per_proc);
		break;

	case PVRSRV_BRIDGE_SGX_SCHEDULE_PROCESS_QUEUES:
		err = SGXScheduleProcessQueuesBW(cmd, in, out, per_proc);
		break;

#if defined(PDUMP)
	case PVRSRV_BRIDGE_SGX_PDUMP_BUFFER_ARRAY:
		err = SGXPDumpBufferArrayBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_PDUMP_3D_SIGNATURE_REGISTERS:
		err = SGXPDump3DSignatureRegistersBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_PDUMP_COUNTER_REGISTERS:
		err = SGXPDumpCounterRegistersBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_PDUMP_TA_SIGNATURE_REGISTERS:
		err = SGXPDumpTASignatureRegistersBW(cmd, in, out, per_proc);
		break;
	case PVRSRV_BRIDGE_SGX_PDUMP_HWPERFCB:
		err = SGXPDumpHWPerfCBBW(cmd, in, out, per_proc);
		break;
#endif

	default:
		PVR_DPF(PVR_DBG_ERROR, "%s: cmd = %d is out if range!",
			__func__, cmd);
       }

       return err;
}

int BridgedDispatchKM(struct file *filp, struct PVRSRV_PER_PROCESS_DATA *pd,
		      struct PVRSRV_BRIDGE_PACKAGE *pkg)
{

	void *in;
	void *out;
	u32 bid = pkg->ui32BridgeID;
	int err = -EFAULT;
	struct SYS_DATA *psSysData;

#if defined(DEBUG_BRIDGE_KM)
	g_BridgeDispatchTable[bid].ui32CallCount++;
	g_BridgeGlobalStats.ui32IOCTLCount++;
#endif
	if (!pd->bInitProcess && bridged_check_cmd(bid))
		goto return_fault;

	if (SysAcquireData(&psSysData) != PVRSRV_OK)
		goto return_fault;

	in = ((struct ENV_DATA *)psSysData->pvEnvSpecificData)->pvBridgeData;
	out = (void *)((u8 *)in + PVRSRV_MAX_BRIDGE_IN_SIZE);

	if (pkg->ui32InBufferSize > 0 &&
	    CopyFromUserWrapper(pd, bid, in, pkg->pvParamIn,
				pkg->ui32InBufferSize) != PVRSRV_OK)
		goto return_fault;

	if (bid >= (BRIDGE_DISPATCH_TABLE_ENTRY_COUNT)) {
		PVR_DPF(PVR_DBG_ERROR,
			 "%s: ui32BridgeID = %d is out if range!", __func__,
			 bid);
		goto return_fault;
	}

	err = bridged_ioctl(filp, bid, in, out, pkg->ui32InBufferSize, pd);

	if (err < 0)
		goto return_fault;

	if (CopyToUserWrapper(pd, bid, pkg->pvParamOut, out,
			      pkg->ui32OutBufferSize) != PVRSRV_OK)
		goto return_fault;

	err = 0;
return_fault:
	ReleaseHandleBatch(pd);
	return err;
}
