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
#include "sgxapi_km.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "pvr_bridge_km.h"
#include "sgx_bridge_km.h"
#include "pdump_km.h"
#include "sgxutils.h"

static struct RESMAN_ITEM *psResItemCreateSharedPB;
static struct PVRSRV_PER_PROCESS_DATA *psPerProcCreateSharedPB;

static enum PVRSRV_ERROR SGXCleanupSharedPBDescCallback(void *pvParam,
						   u32 ui32Param);
static enum PVRSRV_ERROR SGXCleanupSharedPBDescCreateLockCallback(void *pvParam,
							     u32 ui32Param);

enum PVRSRV_ERROR SGXFindSharedPBDescKM(
	     struct PVRSRV_PER_PROCESS_DATA *psPerProc,
	     void *hDevCookie, IMG_BOOL bLockOnFailure,
	     u32 ui32TotalPBSize, void **phSharedPBDesc,
	     struct PVRSRV_KERNEL_MEM_INFO **ppsSharedPBDescKernelMemInfo,
	     struct PVRSRV_KERNEL_MEM_INFO **ppsHWPBDescKernelMemInfo,
	     struct PVRSRV_KERNEL_MEM_INFO **ppsBlockKernelMemInfo,
	     struct PVRSRV_KERNEL_MEM_INFO ***pppsSharedPBDescSubKernelMemInfos,
	     u32 *ui32SharedPBDescSubKernelMemInfosCount)
{
	struct PVRSRV_STUB_PBDESC *psStubPBDesc;
	struct PVRSRV_KERNEL_MEM_INFO **ppsSharedPBDescSubKernelMemInfos = NULL;
	struct PVRSRV_SGXDEV_INFO *psSGXDevInfo;
	enum PVRSRV_ERROR eError;

	psSGXDevInfo = ((struct PVRSRV_DEVICE_NODE *)hDevCookie)->pvDevice;

	psStubPBDesc = psSGXDevInfo->psStubPBDescListKM;
	if (psStubPBDesc != NULL) {
		u32 i;
		struct RESMAN_ITEM *psResItem;

		if (psStubPBDesc->ui32TotalPBSize != ui32TotalPBSize) {
			PVR_DPF(PVR_DBG_WARNING, "SGXFindSharedPBDescKM: "
				"Shared PB requested with different size "
				"(0x%x) from existing shared PB (0x%x) - "
				"requested size ignored",
				 ui32TotalPBSize,
				 psStubPBDesc->ui32TotalPBSize);
		}

		if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			       sizeof(struct PVRSRV_KERNEL_MEM_INFO *) *
				      psStubPBDesc->ui32SubKernelMemInfosCount,
			       (void **) &ppsSharedPBDescSubKernelMemInfos,
			       NULL) != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				 "SGXFindSharedPBDescKM: OSAllocMem failed");

				eError = PVRSRV_ERROR_OUT_OF_MEMORY;
				goto ExitNotFound;
			}

			psResItem = ResManRegisterRes(psPerProc->hResManContext,
					      RESMAN_TYPE_SHARED_PB_DESC,
					      psStubPBDesc, 0,
					      &SGXCleanupSharedPBDescCallback);

			if (psResItem == NULL) {
				OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				  sizeof(struct PVRSRV_KERNEL_MEM_INFO *)*
				  psStubPBDesc->ui32SubKernelMemInfosCount,
				  ppsSharedPBDescSubKernelMemInfos, NULL);

			PVR_DPF(PVR_DBG_ERROR, "SGXFindSharedPBDescKM: "
						"ResManRegisterRes failed");
				eError = PVRSRV_ERROR_GENERIC;
				goto ExitNotFound;
			}

			*ppsSharedPBDescKernelMemInfo =
			    psStubPBDesc->psSharedPBDescKernelMemInfo;
			*ppsHWPBDescKernelMemInfo =
			    psStubPBDesc->psHWPBDescKernelMemInfo;
			*ppsBlockKernelMemInfo =
			    psStubPBDesc->psBlockKernelMemInfo;

			*ui32SharedPBDescSubKernelMemInfosCount =
			    psStubPBDesc->ui32SubKernelMemInfosCount;

			*pppsSharedPBDescSubKernelMemInfos =
			    ppsSharedPBDescSubKernelMemInfos;

			for (i = 0;
			     i < psStubPBDesc->ui32SubKernelMemInfosCount;
			     i++) {
				ppsSharedPBDescSubKernelMemInfos[i] =
				    psStubPBDesc->ppsSubKernelMemInfos[i];
			}

			psStubPBDesc->ui32RefCount++;
			*phSharedPBDesc = (void *) psResItem;
			return PVRSRV_OK;
		}

		eError = PVRSRV_OK;
		if (bLockOnFailure) {
			if (psResItemCreateSharedPB == NULL) {
				psResItemCreateSharedPB =
				    ResManRegisterRes(psPerProc->hResManContext,
				     RESMAN_TYPE_SHARED_PB_DESC_CREATE_LOCK,
				     psPerProc, 0,
				     &SGXCleanupSharedPBDescCreateLockCallback);

			if (psResItemCreateSharedPB == NULL) {
				PVR_DPF(PVR_DBG_ERROR, "SGXFindSharedPBDescKM: "
						"ResManRegisterRes failed");

				eError = PVRSRV_ERROR_GENERIC;
				goto ExitNotFound;
			}
			PVR_ASSERT(psPerProcCreateSharedPB == NULL);
			psPerProcCreateSharedPB = psPerProc;
		} else {
			eError = PVRSRV_ERROR_PROCESSING_BLOCKED;
		}
	}
ExitNotFound:
	*phSharedPBDesc = NULL;

	return eError;
}

static enum PVRSRV_ERROR SGXCleanupSharedPBDescKM(
				struct PVRSRV_STUB_PBDESC *psStubPBDescIn)
{
	struct PVRSRV_STUB_PBDESC **ppsStubPBDesc;
	u32 i;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct PVRSRV_SGXDEV_INFO *psSGXDevInfo;

	psDeviceNode = (struct PVRSRV_DEVICE_NODE *)psStubPBDescIn->hDevCookie;
	psSGXDevInfo = (struct PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;

	for (ppsStubPBDesc = (struct PVRSRV_STUB_PBDESC **)
			&psSGXDevInfo->psStubPBDescListKM;
	     *ppsStubPBDesc != NULL;
	     ppsStubPBDesc = &(*ppsStubPBDesc)->psNext) {
		struct PVRSRV_STUB_PBDESC *psStubPBDesc = *ppsStubPBDesc;

		if (psStubPBDesc == psStubPBDescIn) {
			psStubPBDesc->ui32RefCount--;
			PVR_ASSERT((s32) psStubPBDesc->ui32RefCount >= 0);

			if (psStubPBDesc->ui32RefCount == 0) {
				*ppsStubPBDesc = psStubPBDesc->psNext;

				for (i = 0;
				   i < psStubPBDesc->ui32SubKernelMemInfosCount;
				   i++)
					PVRSRVFreeDeviceMemKM(psStubPBDesc->
								   hDevCookie,
					 psStubPBDesc->ppsSubKernelMemInfos[i]);

				OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					sizeof(struct PVRSRV_KERNEL_MEM_INFO *)*
					  psStubPBDesc->
						ui32SubKernelMemInfosCount,
					  psStubPBDesc->ppsSubKernelMemInfos,
					  NULL);

				PVRSRVFreeSharedSysMemoryKM(psStubPBDesc->
						psBlockKernelMemInfo);

				PVRSRVFreeDeviceMemKM(psStubPBDesc->hDevCookie,
						psStubPBDesc->
						psHWPBDescKernelMemInfo);

				PVRSRVFreeSharedSysMemoryKM(psStubPBDesc->
						psSharedPBDescKernelMemInfo);

				OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
					     sizeof(struct PVRSRV_STUB_PBDESC),
					     psStubPBDesc, NULL);

				SGXCleanupRequest(psDeviceNode, NULL,
				   PVRSRV_USSE_EDM_RESMAN_CLEANUP_SHAREDPBDESC);
			}
			return PVRSRV_OK;
		}
	}

	return PVRSRV_ERROR_INVALID_PARAMS;
}

static enum PVRSRV_ERROR SGXCleanupSharedPBDescCallback(void *pvParam,
							u32 ui32Param)
{
	struct PVRSRV_STUB_PBDESC *psStubPBDesc =
					(struct PVRSRV_STUB_PBDESC *)pvParam;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	return SGXCleanupSharedPBDescKM(psStubPBDesc);
}

static enum PVRSRV_ERROR SGXCleanupSharedPBDescCreateLockCallback(void *pvParam,
							     u32 ui32Param)
{
#ifdef CONFIG_PVR_DEBUG_EXTRA
	struct PVRSRV_PER_PROCESS_DATA *psPerProc =
	    (struct PVRSRV_PER_PROCESS_DATA *)pvParam;
	PVR_ASSERT(psPerProc == psPerProcCreateSharedPB);
#else
	PVR_UNREFERENCED_PARAMETER(pvParam);
#endif

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	psPerProcCreateSharedPB = NULL;
	psResItemCreateSharedPB = NULL;

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SGXUnrefSharedPBDescKM(void *hSharedPBDesc)
{
	PVR_ASSERT(hSharedPBDesc != NULL);

	ResManFreeResByPtr(hSharedPBDesc);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SGXAddSharedPBDescKM(
	struct PVRSRV_PER_PROCESS_DATA *psPerProc,
	void *hDevCookie,
	struct PVRSRV_KERNEL_MEM_INFO *psSharedPBDescKernelMemInfo,
	struct PVRSRV_KERNEL_MEM_INFO *psHWPBDescKernelMemInfo,
	struct PVRSRV_KERNEL_MEM_INFO *psBlockKernelMemInfo,
	u32 ui32TotalPBSize, void **phSharedPBDesc,
	struct PVRSRV_KERNEL_MEM_INFO **ppsSharedPBDescSubKernelMemInfos,
	u32 ui32SharedPBDescSubKernelMemInfosCount)
{
	struct PVRSRV_STUB_PBDESC *psStubPBDesc = NULL;
	enum PVRSRV_ERROR eRet = PVRSRV_ERROR_GENERIC;
	u32 i;
	struct PVRSRV_SGXDEV_INFO *psSGXDevInfo;
	struct RESMAN_ITEM *psResItem;

	if (psPerProcCreateSharedPB != psPerProc) {
		goto NoAdd;
	} else {
		PVR_ASSERT(psResItemCreateSharedPB != NULL);

		ResManFreeResByPtr(psResItemCreateSharedPB);

		PVR_ASSERT(psResItemCreateSharedPB == NULL);
		PVR_ASSERT(psPerProcCreateSharedPB == NULL);
	}

	psSGXDevInfo = (struct PVRSRV_SGXDEV_INFO *)
		((struct PVRSRV_DEVICE_NODE *)hDevCookie)->pvDevice;

	psStubPBDesc = psSGXDevInfo->psStubPBDescListKM;
	if (psStubPBDesc != NULL) {
		if (psStubPBDesc->ui32TotalPBSize != ui32TotalPBSize) {
			PVR_DPF(PVR_DBG_WARNING, "SGXAddSharedPBDescKM: "
				"Shared PB requested with different size "
				"(0x%x) from existing shared PB (0x%x) - "
				"requested size ignored",
				 ui32TotalPBSize,
				 psStubPBDesc->ui32TotalPBSize);

		}

		psResItem = ResManRegisterRes(psPerProc->hResManContext,
					      RESMAN_TYPE_SHARED_PB_DESC,
					      psStubPBDesc, 0,
					      &SGXCleanupSharedPBDescCallback);
		if (psResItem == NULL) {
			PVR_DPF(PVR_DBG_ERROR,
					 "SGXAddSharedPBDescKM: "
					 "Failed to register existing shared "
					 "PBDesc with the resource manager");
			goto NoAddKeepPB;
		}

		psStubPBDesc->ui32RefCount++;

		*phSharedPBDesc = (void *) psResItem;
		eRet = PVRSRV_OK;
		goto NoAddKeepPB;
	}

	if (OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
		       sizeof(struct PVRSRV_STUB_PBDESC),
		       (void **)&psStubPBDesc, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "SGXAddSharedPBDescKM: Failed to alloc "
			 "StubPBDesc");
		eRet = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto NoAdd;
	}

	psStubPBDesc->ppsSubKernelMemInfos = NULL;

	if (OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
		       sizeof(struct PVRSRV_KERNEL_MEM_INFO *) *
					ui32SharedPBDescSubKernelMemInfosCount,
		       (void **)&psStubPBDesc->ppsSubKernelMemInfos, NULL) !=
			PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "SGXAddSharedPBDescKM: "
			 "Failed to alloc "
			 "StubPBDesc->ppsSubKernelMemInfos");
		eRet = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto NoAdd;
	}

	if (PVRSRVDissociateMemFromResmanKM(psSharedPBDescKernelMemInfo)
	    != PVRSRV_OK)
		goto NoAdd;

	if (PVRSRVDissociateMemFromResmanKM(psHWPBDescKernelMemInfo)
	    != PVRSRV_OK)
		goto NoAdd;

	if (PVRSRVDissociateMemFromResmanKM(psBlockKernelMemInfo)
	    != PVRSRV_OK)
		goto NoAdd;

	psStubPBDesc->ui32RefCount = 1;
	psStubPBDesc->ui32TotalPBSize = ui32TotalPBSize;
	psStubPBDesc->psSharedPBDescKernelMemInfo = psSharedPBDescKernelMemInfo;
	psStubPBDesc->psHWPBDescKernelMemInfo = psHWPBDescKernelMemInfo;
	psStubPBDesc->psBlockKernelMemInfo = psBlockKernelMemInfo;

	psStubPBDesc->ui32SubKernelMemInfosCount =
	    ui32SharedPBDescSubKernelMemInfosCount;
	for (i = 0; i < ui32SharedPBDescSubKernelMemInfosCount; i++) {
		psStubPBDesc->ppsSubKernelMemInfos[i] =
		    ppsSharedPBDescSubKernelMemInfos[i];
		if (PVRSRVDissociateMemFromResmanKM
		    (ppsSharedPBDescSubKernelMemInfos[i]) != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "SGXAddSharedPBDescKM: "
				 "Failed to dissociate shared PBDesc "
				 "from process");
			goto NoAdd;
		}
	}

	psResItem = ResManRegisterRes(psPerProc->hResManContext,
				      RESMAN_TYPE_SHARED_PB_DESC,
				      psStubPBDesc,
				      0, &SGXCleanupSharedPBDescCallback);
	if (psResItem == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "SGXAddSharedPBDescKM: "
			 "Failed to register shared PBDesc "
			 " with the resource manager");
		goto NoAdd;
	}
	psStubPBDesc->hDevCookie = hDevCookie;

	psStubPBDesc->psNext = psSGXDevInfo->psStubPBDescListKM;
	psSGXDevInfo->psStubPBDescListKM = psStubPBDesc;

	*phSharedPBDesc = (void *) psResItem;

	return PVRSRV_OK;

NoAdd:
	if (psStubPBDesc) {
		if (psStubPBDesc->ppsSubKernelMemInfos) {
			OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				  sizeof(struct PVRSRV_KERNEL_MEM_INFO *) *
					ui32SharedPBDescSubKernelMemInfosCount,
				  psStubPBDesc->ppsSubKernelMemInfos, NULL);
		}
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_STUB_PBDESC), psStubPBDesc,
			  NULL);
	}

NoAddKeepPB:
	for (i = 0; i < ui32SharedPBDescSubKernelMemInfosCount; i++)
		PVRSRVFreeDeviceMemKM(hDevCookie,
				      ppsSharedPBDescSubKernelMemInfos[i]);

	PVRSRVFreeSharedSysMemoryKM(psSharedPBDescKernelMemInfo);
	PVRSRVFreeDeviceMemKM(hDevCookie, psHWPBDescKernelMemInfo);

	PVRSRVFreeSharedSysMemoryKM(psBlockKernelMemInfo);

	return eRet;
}
