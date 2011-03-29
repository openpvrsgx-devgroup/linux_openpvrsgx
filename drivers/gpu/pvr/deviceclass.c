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
#include <linux/module.h>

#include "services_headers.h"
#include "buffer_manager.h"
#include "kernelbuffer.h"
#include "pvr_bridge_km.h"

struct PVRSRV_DC_SRV2DISP_KMJTABLE;

struct PVRSRV_DC_BUFFER {
	struct PVRSRV_DEVICECLASS_BUFFER sDeviceClassBuffer;
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct PVRSRV_DC_SWAPCHAIN *psSwapChain;
};

struct PVRSRV_DC_SWAPCHAIN {
	void *hExtSwapChain;
	struct PVRSRV_QUEUE_INFO *psQueue;
	struct PVRSRV_DC_BUFFER asBuffer[PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS];
	u32 ui32BufferCount;
	struct PVRSRV_DC_BUFFER *psLastFlipBuffer;
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	void *hResItem;
};

struct PVRSRV_DISPLAYCLASS_INFO {
	u32 ui32RefCount;
	u32 ui32DeviceID;
	void *hExtDevice;
	struct PVRSRV_DC_SRV2DISP_KMJTABLE *psFuncTable;
	void *hDevMemContext;
	struct PVRSRV_DC_BUFFER sSystemBuffer;
};

struct PVRSRV_DISPLAYCLASS_PERCONTEXT_INFO {
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct RESMAN_ITEM *hResItem;
};

struct PVRSRV_BC_SRV2BUFFER_KMJTABLE;

struct PVRSRV_BC_BUFFER {
	struct PVRSRV_DEVICECLASS_BUFFER sDeviceClassBuffer;
	struct PVRSRV_BUFFERCLASS_INFO *psBCInfo;
};

struct PVRSRV_BUFFERCLASS_INFO {
	u32 ui32RefCount;
	u32 ui32DeviceID;
	void *hExtDevice;
	struct PVRSRV_BC_SRV2BUFFER_KMJTABLE *psFuncTable;
	void *hDevMemContext;

	u32 ui32BufferCount;
	struct PVRSRV_BC_BUFFER *psBuffer;

};

struct PVRSRV_BUFFERCLASS_PERCONTEXT_INFO {
	struct PVRSRV_BUFFERCLASS_INFO *psBCInfo;
	void *hResItem;
};

static struct PVRSRV_DISPLAYCLASS_INFO *DCDeviceHandleToDCInfo(void *hDeviceKM)
{
	struct PVRSRV_DISPLAYCLASS_PERCONTEXT_INFO *psDCPerContextInfo;

	psDCPerContextInfo = (struct PVRSRV_DISPLAYCLASS_PERCONTEXT_INFO *)
				hDeviceKM;

	return psDCPerContextInfo->psDCInfo;
}

static struct PVRSRV_BUFFERCLASS_INFO *BCDeviceHandleToBCInfo(void *hDeviceKM)
{
	struct PVRSRV_BUFFERCLASS_PERCONTEXT_INFO *psBCPerContextInfo;

	psBCPerContextInfo = (struct PVRSRV_BUFFERCLASS_PERCONTEXT_INFO *)
				hDeviceKM;

	return psBCPerContextInfo->psBCInfo;
}

enum PVRSRV_ERROR PVRSRVEnumerateDCKM(enum PVRSRV_DEVICE_CLASS DeviceClass,
				     u32 *pui32DevCount, u32 *pui32DevID)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	unsigned ui32DevCount = 0;
	struct SYS_DATA *psSysData;

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVEnumerateDCKM: Failed to get SysData");
		return PVRSRV_ERROR_GENERIC;
	}

	psDeviceNode = psSysData->psDeviceNodeList;
	while (psDeviceNode) {
		if ((psDeviceNode->sDevId.eDeviceClass == DeviceClass) &&
		    (psDeviceNode->sDevId.eDeviceType ==
			PVRSRV_DEVICE_TYPE_EXT)) {
			ui32DevCount++;
			if (pui32DevID) {
				*pui32DevID++ =
				    psDeviceNode->sDevId.ui32DeviceIndex;
			}
		}
		psDeviceNode = psDeviceNode->psNext;
	}

	if (pui32DevCount) {
		*pui32DevCount = ui32DevCount;
	} else if (pui32DevID == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVEnumerateDCKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR PVRSRVRegisterDCDeviceKM(
			struct PVRSRV_DC_SRV2DISP_KMJTABLE *psFuncTable,
			u32 *pui32DeviceID)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo = NULL;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct SYS_DATA *psSysData;

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRegisterDCDeviceKM: Failed to get SysData");
		return PVRSRV_ERROR_GENERIC;
	}

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(*psDCInfo),
		       (void **) &psDCInfo, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRegisterDCDeviceKM: Failed psDCInfo alloc");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	OSMemSet(psDCInfo, 0, sizeof(*psDCInfo));

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(struct PVRSRV_DC_SRV2DISP_KMJTABLE),
		       (void **)&psDCInfo->psFuncTable,
		       NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRegisterDCDeviceKM: Failed psFuncTable alloc");
		goto ErrorExit;
	}
	OSMemSet(psDCInfo->psFuncTable, 0,
		 sizeof(struct PVRSRV_DC_SRV2DISP_KMJTABLE));

	*psDCInfo->psFuncTable = *psFuncTable;

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(struct PVRSRV_DEVICE_NODE),
		       (void **) &psDeviceNode, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			"PVRSRVRegisterDCDeviceKM: Failed psDeviceNode alloc");
		goto ErrorExit;
	}
	OSMemSet(psDeviceNode, 0, sizeof(struct PVRSRV_DEVICE_NODE));

	psDeviceNode->pvDevice = (void *) psDCInfo;
	psDeviceNode->ui32pvDeviceSize = sizeof(*psDCInfo);
	psDeviceNode->ui32RefCount = 1;
	psDeviceNode->sDevId.eDeviceType = PVRSRV_DEVICE_TYPE_EXT;
	psDeviceNode->sDevId.eDeviceClass = PVRSRV_DEVICE_CLASS_DISPLAY;
	psDeviceNode->psSysData = psSysData;

	AllocateDeviceID(psSysData, &psDeviceNode->sDevId.ui32DeviceIndex);
	psDCInfo->ui32DeviceID = psDeviceNode->sDevId.ui32DeviceIndex;
	if (pui32DeviceID)
		*pui32DeviceID = psDeviceNode->sDevId.ui32DeviceIndex;

	SysRegisterExternalDevice(psDeviceNode);

	psDeviceNode->psNext = psSysData->psDeviceNodeList;
	psSysData->psDeviceNodeList = psDeviceNode;

	return PVRSRV_OK;

ErrorExit:

	if (psDCInfo->psFuncTable)
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_DC_SRV2DISP_KMJTABLE),
			  psDCInfo->psFuncTable, NULL);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
		  sizeof(struct PVRSRV_DISPLAYCLASS_INFO),
		  psDCInfo, NULL);

	return PVRSRV_ERROR_OUT_OF_MEMORY;
}

static enum PVRSRV_ERROR PVRSRVRemoveDCDeviceKM(u32 ui32DevIndex)
{
	struct SYS_DATA *psSysData;
	struct PVRSRV_DEVICE_NODE **ppsDeviceNode, *psDeviceNode;
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRemoveDCDeviceKM: Failed to get SysData");
		return PVRSRV_ERROR_GENERIC;
	}

	ppsDeviceNode = &psSysData->psDeviceNodeList;
	while (*ppsDeviceNode) {
		switch ((*ppsDeviceNode)->sDevId.eDeviceClass) {
		case PVRSRV_DEVICE_CLASS_DISPLAY:
			{
				if ((*ppsDeviceNode)->sDevId.ui32DeviceIndex ==
				    ui32DevIndex)
					goto FoundDevice;
				break;
			}
		default:
			{
				break;
			}
		}
		ppsDeviceNode = &((*ppsDeviceNode)->psNext);
	}

	PVR_DPF(PVR_DBG_ERROR,
		 "PVRSRVRemoveDCDeviceKM: requested device %d not present",
		 ui32DevIndex);

	return PVRSRV_ERROR_GENERIC;

FoundDevice:

	psDeviceNode = *ppsDeviceNode;

	psDCInfo = (struct PVRSRV_DISPLAYCLASS_INFO *)psDeviceNode->pvDevice;

	if (psDCInfo->ui32RefCount == 0) {
		*ppsDeviceNode = psDeviceNode->psNext;
		SysRemoveExternalDevice(psDeviceNode);
		PVR_ASSERT(psDCInfo->ui32RefCount == 0);
		FreeDeviceID(psSysData, ui32DevIndex);
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_DC_SRV2DISP_KMJTABLE),
			  psDCInfo->psFuncTable, NULL);
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_DISPLAYCLASS_INFO), psDCInfo,
			  NULL);
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_DEVICE_NODE), psDeviceNode,
			  NULL);
	} else {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVRemoveDCDeviceKM: "
				"failed as %d Services DC API "
				"connections are still open",
			 psDCInfo->ui32RefCount);
		return PVRSRV_ERROR_GENERIC;
	}

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR PVRSRVRegisterBCDeviceKM(
		struct PVRSRV_BC_SRV2BUFFER_KMJTABLE *psFuncTable,
		u32 *pui32DeviceID)
{
	struct PVRSRV_BUFFERCLASS_INFO *psBCInfo = NULL;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct SYS_DATA *psSysData;

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRegisterBCDeviceKM: Failed to get SysData");
		return PVRSRV_ERROR_GENERIC;
	}

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(*psBCInfo),
		       (void **) &psBCInfo, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRegisterBCDeviceKM: Failed psBCInfo alloc");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	OSMemSet(psBCInfo, 0, sizeof(*psBCInfo));

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(struct PVRSRV_BC_SRV2BUFFER_KMJTABLE),
		       (void **) &psBCInfo->psFuncTable,
		       NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRegisterBCDeviceKM: Failed psFuncTable alloc");
		goto ErrorExit;
	}
	OSMemSet(psBCInfo->psFuncTable, 0,
		 sizeof(struct PVRSRV_BC_SRV2BUFFER_KMJTABLE));

	*psBCInfo->psFuncTable = *psFuncTable;

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(struct PVRSRV_DEVICE_NODE),
		       (void **) &psDeviceNode, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			"PVRSRVRegisterBCDeviceKM: Failed psDeviceNode alloc");
		goto ErrorExit;
	}
	OSMemSet(psDeviceNode, 0, sizeof(struct PVRSRV_DEVICE_NODE));

	psDeviceNode->pvDevice = (void *) psBCInfo;
	psDeviceNode->ui32pvDeviceSize = sizeof(*psBCInfo);
	psDeviceNode->ui32RefCount = 1;
	psDeviceNode->sDevId.eDeviceType = PVRSRV_DEVICE_TYPE_EXT;
	psDeviceNode->sDevId.eDeviceClass = PVRSRV_DEVICE_CLASS_BUFFER;
	psDeviceNode->psSysData = psSysData;

	AllocateDeviceID(psSysData, &psDeviceNode->sDevId.ui32DeviceIndex);
	psBCInfo->ui32DeviceID = psDeviceNode->sDevId.ui32DeviceIndex;
	if (pui32DeviceID)
		*pui32DeviceID = psDeviceNode->sDevId.ui32DeviceIndex;

	psDeviceNode->psNext = psSysData->psDeviceNodeList;
	psSysData->psDeviceNodeList = psDeviceNode;

	return PVRSRV_OK;

ErrorExit:

	if (psBCInfo->psFuncTable)
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(*psBCInfo->psFuncTable), psBCInfo->psFuncTable,
			  NULL);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*psBCInfo), psBCInfo, NULL);

	return PVRSRV_ERROR_OUT_OF_MEMORY;
}

static enum PVRSRV_ERROR PVRSRVRemoveBCDeviceKM(u32 ui32DevIndex)
{
	struct SYS_DATA *psSysData;
	struct PVRSRV_DEVICE_NODE **ppsDevNode, *psDevNode;
	struct PVRSRV_BUFFERCLASS_INFO *psBCInfo;

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVRemoveBCDeviceKM: Failed to get SysData");
		return PVRSRV_ERROR_GENERIC;
	}

	ppsDevNode = &psSysData->psDeviceNodeList;
	while (*ppsDevNode) {
		switch ((*ppsDevNode)->sDevId.eDeviceClass) {
		case PVRSRV_DEVICE_CLASS_BUFFER:
			{
				if ((*ppsDevNode)->sDevId.ui32DeviceIndex ==
				    ui32DevIndex)
					goto FoundDevice;
				break;
			}
		default:
			{
				break;
			}
		}
		ppsDevNode = &(*ppsDevNode)->psNext;
	}

	PVR_DPF(PVR_DBG_ERROR,
		 "PVRSRVRemoveBCDeviceKM: requested device %d not present",
		 ui32DevIndex);

	return PVRSRV_ERROR_GENERIC;

FoundDevice:

	psDevNode = *(ppsDevNode);

	psBCInfo = (struct PVRSRV_BUFFERCLASS_INFO *)psDevNode->pvDevice;

	if (psBCInfo->ui32RefCount == 0) {
		*ppsDevNode = psDevNode->psNext;
		FreeDeviceID(psSysData, ui32DevIndex);
		psBCInfo =
		    (struct PVRSRV_BUFFERCLASS_INFO *)psDevNode->pvDevice;
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_BC_SRV2BUFFER_KMJTABLE),
			  psBCInfo->psFuncTable, NULL);
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_BUFFERCLASS_INFO), psBCInfo,
			  NULL);
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_DEVICE_NODE), psDevNode, NULL);
	} else {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVRemoveBCDeviceKM: "
				"failed as %d Services BC API "
				"connections are still open",
			 psBCInfo->ui32RefCount);
		return PVRSRV_ERROR_GENERIC;
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVCloseDCDeviceKM(void *hDeviceKM,
				       IMG_BOOL bResManCallback)
{
	struct PVRSRV_DISPLAYCLASS_PERCONTEXT_INFO *psDCPerContextInfo;

	PVR_UNREFERENCED_PARAMETER(bResManCallback);

	psDCPerContextInfo = (struct PVRSRV_DISPLAYCLASS_PERCONTEXT_INFO *)
								hDeviceKM;

	ResManFreeResByPtr(psDCPerContextInfo->hResItem);

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR CloseDCDeviceCallBack(void *pvParam, u32 ui32Param)
{
	struct PVRSRV_DISPLAYCLASS_PERCONTEXT_INFO *psDCPerContextInfo;
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	psDCPerContextInfo = (struct PVRSRV_DISPLAYCLASS_PERCONTEXT_INFO *)
									pvParam;
	psDCInfo = psDCPerContextInfo->psDCInfo;

	psDCInfo->ui32RefCount--;
	if (psDCInfo->ui32RefCount == 0) {
		struct PVRSRV_DC_SRV2DISP_KMJTABLE *jtbl;

		jtbl = psDCInfo->psFuncTable;

		jtbl->pfnCloseDCDevice(psDCInfo->hExtDevice);

		PVRSRVFreeSyncInfoKM(psDCInfo->sSystemBuffer.sDeviceClassBuffer.
				     psKernelSyncInfo);

		psDCInfo->hDevMemContext = NULL;
		psDCInfo->hExtDevice = NULL;

		module_put(jtbl->owner);
	}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
		  sizeof(struct PVRSRV_DISPLAYCLASS_PERCONTEXT_INFO),
		  psDCPerContextInfo, NULL);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVOpenDCDeviceKM(
				struct PVRSRV_PER_PROCESS_DATA *psPerProc,
				u32 ui32DeviceID, void *hDevCookie,
				void **phDeviceKM)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct PVRSRV_DISPLAYCLASS_PERCONTEXT_INFO *psDCPerContextInfo;
	struct PVRSRV_DC_SRV2DISP_KMJTABLE *jtbl;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct SYS_DATA *psSysData;
	enum PVRSRV_ERROR eError;

	if (!phDeviceKM || !hDevCookie) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVOpenDCDeviceKM: Invalid params");
		return PVRSRV_ERROR_GENERIC;
	}

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVOpenDCDeviceKM: Failed to get SysData");
		return PVRSRV_ERROR_GENERIC;
	}

	psDeviceNode = psSysData->psDeviceNodeList;
	while (psDeviceNode) {
		if ((psDeviceNode->sDevId.eDeviceClass ==
		     PVRSRV_DEVICE_CLASS_DISPLAY) &&
		     (psDeviceNode->sDevId.ui32DeviceIndex == ui32DeviceID)) {

			psDCInfo = (struct PVRSRV_DISPLAYCLASS_INFO *)
							psDeviceNode->pvDevice;
			goto FoundDevice;
		}
		psDeviceNode = psDeviceNode->psNext;
	}

	PVR_DPF(PVR_DBG_ERROR,
		 "PVRSRVOpenDCDeviceKM: no devnode matching index %d",
		 ui32DeviceID);

	return PVRSRV_ERROR_GENERIC;

FoundDevice:

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*psDCPerContextInfo),
		       (void **)&psDCPerContextInfo, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVOpenDCDeviceKM: "
					"Failed psDCPerContextInfo alloc");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	OSMemSet(psDCPerContextInfo, 0, sizeof(*psDCPerContextInfo));

	if (psDCInfo->ui32RefCount++ == 0) {
		psDeviceNode = (struct PVRSRV_DEVICE_NODE *)hDevCookie;

		jtbl = psDCInfo->psFuncTable;
		if (!try_module_get(jtbl->owner)) {
			PVR_DPF(PVR_DBG_ERROR, "%s: can't get DC module");
			eError = PVRSRV_ERROR_INVALID_DEVICE;

			goto err1;
		}

		psDCInfo->hDevMemContext =
		    (void *) psDeviceNode->sDevMemoryInfo.pBMKernelContext;

		eError = PVRSRVAllocSyncInfoKM(NULL,
				       (void *)psDeviceNode->
					       sDevMemoryInfo.pBMKernelContext,
				       &psDCInfo->sSystemBuffer.
					       sDeviceClassBuffer.
						       psKernelSyncInfo);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
			       "PVRSRVOpenDCDeviceKM: Failed sync info alloc");

			goto err2;
		}

		eError = jtbl->pfnOpenDCDevice(ui32DeviceID,
			&psDCInfo->hExtDevice,
			(struct PVRSRV_SYNC_DATA *)psDCInfo->sSystemBuffer.
				sDeviceClassBuffer.psKernelSyncInfo->
					psSyncDataMemInfoKM->pvLinAddrKM);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "PVRSRVOpenDCDeviceKM: "
					"Failed to open external DC device");
			goto err3;
		}
	}

	psDCPerContextInfo->psDCInfo = psDCInfo;
	psDCPerContextInfo->hResItem =
	    ResManRegisterRes(psPerProc->hResManContext,
			      RESMAN_TYPE_DISPLAYCLASS_DEVICE,
			      psDCPerContextInfo, 0, CloseDCDeviceCallBack);

	*phDeviceKM = (void *) psDCPerContextInfo;

	return PVRSRV_OK;
err3:
	PVRSRVFreeSyncInfoKM(psDCInfo->sSystemBuffer.
			     sDeviceClassBuffer.psKernelSyncInfo);
err2:
	module_put(jtbl->owner);
	psDCInfo->ui32RefCount--;
err1:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*psDCPerContextInfo),
		  psDCPerContextInfo, NULL);

	return eError;
}

enum PVRSRV_ERROR PVRSRVEnumDCFormatsKM(void *hDeviceKM,
				       u32 *pui32Count,
				       struct DISPLAY_FORMAT *psFormat)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;

	if (!hDeviceKM || !pui32Count || !psFormat) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVEnumDCFormatsKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);

	return psDCInfo->psFuncTable->pfnEnumDCFormats(psDCInfo->hExtDevice,
						       pui32Count, psFormat);
}

enum PVRSRV_ERROR PVRSRVEnumDCDimsKM(void *hDeviceKM,
				    struct DISPLAY_FORMAT *psFormat,
				    u32 *pui32Count, struct DISPLAY_DIMS *psDim)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;

	if (!hDeviceKM || !pui32Count || !psFormat) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVEnumDCDimsKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);

	return psDCInfo->psFuncTable->pfnEnumDCDims(psDCInfo->hExtDevice,
						  psFormat, pui32Count, psDim);
}

enum PVRSRV_ERROR PVRSRVGetDCSystemBufferKM(void *hDeviceKM, void **phBuffer)
{
	enum PVRSRV_ERROR eError;
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	void *hExtBuffer;

	if (!hDeviceKM || !phBuffer) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVGetDCSystemBufferKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);

	eError =
	    psDCInfo->psFuncTable->pfnGetDCSystemBuffer(psDCInfo->hExtDevice,
							&hExtBuffer);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVGetDCSystemBufferKM: "
		     "Failed to get valid buffer handle from external driver");
		return eError;
	}

	psDCInfo->sSystemBuffer.sDeviceClassBuffer.pfnGetBufferAddr =
	    psDCInfo->psFuncTable->pfnGetBufferAddr;
	psDCInfo->sSystemBuffer.sDeviceClassBuffer.hDevMemContext =
	    psDCInfo->hDevMemContext;
	psDCInfo->sSystemBuffer.sDeviceClassBuffer.hExtDevice =
	    psDCInfo->hExtDevice;
	psDCInfo->sSystemBuffer.sDeviceClassBuffer.hExtBuffer = hExtBuffer;

	psDCInfo->sSystemBuffer.psDCInfo = psDCInfo;

	*phBuffer = (void *) &(psDCInfo->sSystemBuffer);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVGetDCInfoKM(void *hDeviceKM,
				struct DISPLAY_INFO *psDisplayInfo)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	enum PVRSRV_ERROR eError;

	if (!hDeviceKM || !psDisplayInfo) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVGetDCInfoKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);

	eError = psDCInfo->psFuncTable->pfnGetDCInfo(psDCInfo->hExtDevice,
							psDisplayInfo);
	if (eError != PVRSRV_OK)
		return eError;

	if (psDisplayInfo->ui32MaxSwapChainBuffers >
	    PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS) {
		psDisplayInfo->ui32MaxSwapChainBuffers =
		    PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS;
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVDestroyDCSwapChainKM(void *hSwapChain)
{
	struct PVRSRV_DC_SWAPCHAIN *psSwapChain;

	if (!hSwapChain) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVDestroyDCSwapChainKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psSwapChain = hSwapChain;

	ResManFreeResByPtr(psSwapChain->hResItem);

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR DestroyDCSwapChainCallBack(void *pvParam,
					       u32 ui32Param)
{
	enum PVRSRV_ERROR eError;
	struct PVRSRV_DC_SWAPCHAIN *psSwapChain = pvParam;
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo = psSwapChain->psDCInfo;
	u32 i;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	PVRSRVDestroyCommandQueueKM(psSwapChain->psQueue);

	eError =
	    psDCInfo->psFuncTable->pfnDestroyDCSwapChain(psDCInfo->hExtDevice,
							 psSwapChain->
								 hExtSwapChain);

	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "DestroyDCSwapChainCallBack: "
					"Failed to destroy DC swap chain");
		return eError;
	}

	for (i = 0; i < psSwapChain->ui32BufferCount; i++)
		if (psSwapChain->asBuffer[i].sDeviceClassBuffer.
		    psKernelSyncInfo)
			PVRSRVFreeSyncInfoKM(psSwapChain->asBuffer[i].
						     sDeviceClassBuffer.
							     psKernelSyncInfo);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct PVRSRV_DC_SWAPCHAIN),
		  psSwapChain, NULL);

	return eError;
}

enum PVRSRV_ERROR PVRSRVCreateDCSwapChainKM(
				struct PVRSRV_PER_PROCESS_DATA *psPerProc,
				void *hDeviceKM, u32 ui32Flags,
				struct DISPLAY_SURF_ATTRIBUTES *psDstSurfAttrib,
				struct DISPLAY_SURF_ATTRIBUTES *psSrcSurfAttrib,
				u32 ui32BufferCount, u32 ui32OEMFlags,
				void **phSwapChain, u32 *pui32SwapChainID)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct PVRSRV_DC_SWAPCHAIN *psSwapChain = NULL;
	struct PVRSRV_SYNC_DATA *apsSyncData[PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS];
	struct PVRSRV_QUEUE_INFO *psQueue = NULL;
	enum PVRSRV_ERROR eError;
	u32 i;

	if (!hDeviceKM || !psDstSurfAttrib || !psSrcSurfAttrib ||
	    !phSwapChain || !pui32SwapChainID) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVCreateDCSwapChainKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (ui32BufferCount > PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVCreateDCSwapChainKM: Too many buffers");
		return PVRSRV_ERROR_TOOMANYBUFFERS;
	}

	if (ui32BufferCount < 2) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVCreateDCSwapChainKM: Too few buffers");
		return PVRSRV_ERROR_TOO_FEW_BUFFERS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(struct PVRSRV_DC_SWAPCHAIN),
		       (void **) &psSwapChain, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVCreateDCSwapChainKM: Failed psSwapChain alloc");
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorExit;
	}
	OSMemSet(psSwapChain, 0, sizeof(struct PVRSRV_DC_SWAPCHAIN));

	eError = PVRSRVCreateCommandQueueKM(1024, &psQueue);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			"PVRSRVCreateDCSwapChainKM: Failed to create CmdQueue");
		goto ErrorExit;
	}

	psSwapChain->psQueue = psQueue;

	for (i = 0; i < ui32BufferCount; i++) {
		eError = PVRSRVAllocSyncInfoKM(NULL,
					       psDCInfo->hDevMemContext,
					       &psSwapChain->asBuffer[i].
						       sDeviceClassBuffer.
							      psKernelSyncInfo);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "PVRSRVCreateDCSwapChainKM: "
				"Failed to alloc syninfo for psSwapChain");
			goto ErrorExit;
		}

		psSwapChain->asBuffer[i].sDeviceClassBuffer.pfnGetBufferAddr =
		    psDCInfo->psFuncTable->pfnGetBufferAddr;
		psSwapChain->asBuffer[i].sDeviceClassBuffer.hDevMemContext =
		    psDCInfo->hDevMemContext;
		psSwapChain->asBuffer[i].sDeviceClassBuffer.hExtDevice =
		    psDCInfo->hExtDevice;

		psSwapChain->asBuffer[i].psDCInfo = psDCInfo;
		psSwapChain->asBuffer[i].psSwapChain = psSwapChain;

		apsSyncData[i] =
		    (struct PVRSRV_SYNC_DATA *)psSwapChain->asBuffer[i].
			    sDeviceClassBuffer.psKernelSyncInfo->
					psSyncDataMemInfoKM->pvLinAddrKM;
	}

	psSwapChain->ui32BufferCount = ui32BufferCount;
	psSwapChain->psDCInfo = psDCInfo;

	eError =
	    psDCInfo->psFuncTable->pfnCreateDCSwapChain(psDCInfo->hExtDevice,
						ui32Flags,
						psDstSurfAttrib,
						psSrcSurfAttrib,
						ui32BufferCount,
						apsSyncData,
						ui32OEMFlags,
						&psSwapChain->hExtSwapChain,
						pui32SwapChainID);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVCreateDCSwapChainKM: "
				"Failed to create 3rd party SwapChain");
		goto ErrorExit;
	}

	*phSwapChain = (void *) psSwapChain;

	psSwapChain->hResItem = ResManRegisterRes(psPerProc->hResManContext,
					  RESMAN_TYPE_DISPLAYCLASS_SWAPCHAIN,
					  psSwapChain, 0,
					  DestroyDCSwapChainCallBack);

	return eError;

ErrorExit:

	for (i = 0; i < ui32BufferCount; i++) {
		if (psSwapChain->asBuffer[i].sDeviceClassBuffer.
		    psKernelSyncInfo) {
			PVRSRVFreeSyncInfoKM(psSwapChain->asBuffer[i].
						     sDeviceClassBuffer.
							     psKernelSyncInfo);
		}
	}

	if (psQueue)
		PVRSRVDestroyCommandQueueKM(psQueue);

	if (psSwapChain) {
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_DC_SWAPCHAIN), psSwapChain,
			  NULL);
	}

	return eError;
}

enum PVRSRV_ERROR PVRSRVSetDCDstRectKM(void *hDeviceKM, void *hSwapChain,
				       struct IMG_RECT *psRect)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct PVRSRV_DC_SWAPCHAIN *psSwapChain;

	if (!hDeviceKM || !hSwapChain) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVSetDCDstRectKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);
	psSwapChain = (struct PVRSRV_DC_SWAPCHAIN *)hSwapChain;

	return psDCInfo->psFuncTable->pfnSetDCDstRect(psDCInfo->hExtDevice,
					psSwapChain->hExtSwapChain, psRect);
}

enum PVRSRV_ERROR PVRSRVSetDCSrcRectKM(void *hDeviceKM, void *hSwapChain,
				       struct IMG_RECT *psRect)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct PVRSRV_DC_SWAPCHAIN *psSwapChain;

	if (!hDeviceKM || !hSwapChain) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVSetDCSrcRectKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);
	psSwapChain = (struct PVRSRV_DC_SWAPCHAIN *)hSwapChain;

	return psDCInfo->psFuncTable->pfnSetDCSrcRect(psDCInfo->hExtDevice,
					psSwapChain->hExtSwapChain, psRect);
}

enum PVRSRV_ERROR PVRSRVSetDCDstColourKeyKM(void *hDeviceKM, void *hSwapChain,
				       u32 ui32CKColour)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct PVRSRV_DC_SWAPCHAIN *psSwapChain;

	if (!hDeviceKM || !hSwapChain) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVSetDCDstColourKeyKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);
	psSwapChain = (struct PVRSRV_DC_SWAPCHAIN *)hSwapChain;

	return psDCInfo->psFuncTable->pfnSetDCDstColourKey(psDCInfo->hExtDevice,
				     psSwapChain->hExtSwapChain, ui32CKColour);
}

enum PVRSRV_ERROR PVRSRVSetDCSrcColourKeyKM(void *hDeviceKM, void *hSwapChain,
					   u32 ui32CKColour)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct PVRSRV_DC_SWAPCHAIN *psSwapChain;

	if (!hDeviceKM || !hSwapChain) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVSetDCSrcColourKeyKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);
	psSwapChain = (struct PVRSRV_DC_SWAPCHAIN *)hSwapChain;

	return psDCInfo->psFuncTable->pfnSetDCSrcColourKey(psDCInfo->hExtDevice,
				     psSwapChain->hExtSwapChain, ui32CKColour);
}

enum PVRSRV_ERROR PVRSRVGetDCBuffersKM(void *hDeviceKM, void *hSwapChain,
				      u32 *pui32BufferCount, void **phBuffer)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct PVRSRV_DC_SWAPCHAIN *psSwapChain;
	void *ahExtBuffer[PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS];
	enum PVRSRV_ERROR eError;
	u32 i;

	if (!hDeviceKM || !hSwapChain || !phBuffer) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVGetDCBuffersKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);
	psSwapChain = (struct PVRSRV_DC_SWAPCHAIN *)hSwapChain;

	eError = psDCInfo->psFuncTable->pfnGetDCBuffers(psDCInfo->hExtDevice,
						psSwapChain->hExtSwapChain,
						pui32BufferCount, ahExtBuffer);

	PVR_ASSERT(*pui32BufferCount <= PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS);

	for (i = 0; i < *pui32BufferCount; i++) {
		psSwapChain->asBuffer[i].sDeviceClassBuffer.hExtBuffer =
								ahExtBuffer[i];
		phBuffer[i] = (void *)&psSwapChain->asBuffer[i];
	}

	return eError;
}

enum PVRSRV_ERROR PVRSRVSwapToDCBufferKM(void *hDeviceKM, void *hBuffer,
					u32 ui32SwapInterval, void *hPrivateTag,
					u32 ui32ClipRectCount,
					struct IMG_RECT *psClipRect)
{
	enum PVRSRV_ERROR eError;
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct PVRSRV_DC_BUFFER *psBuffer;
	struct PVRSRV_QUEUE_INFO *psQueue;
	struct DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	u32 i;
	u32 ui32NumSrcSyncs = 1;
	struct PVRSRV_KERNEL_SYNC_INFO *apsSrcSync[2];
	struct PVRSRV_COMMAND *psCommand;

	if (!hDeviceKM || !hBuffer || !psClipRect) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVSwapToDCBufferKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);
	psBuffer = (struct PVRSRV_DC_BUFFER *)hBuffer;

	psQueue = psBuffer->psSwapChain->psQueue;

	apsSrcSync[0] = psBuffer->sDeviceClassBuffer.psKernelSyncInfo;
	if (psBuffer->psSwapChain->psLastFlipBuffer &&
	    psBuffer != psBuffer->psSwapChain->psLastFlipBuffer) {
		apsSrcSync[1] =
		    psBuffer->psSwapChain->psLastFlipBuffer->sDeviceClassBuffer.
		    psKernelSyncInfo;
		ui32NumSrcSyncs++;
	}

	eError = PVRSRVInsertCommandKM(psQueue, &psCommand,
				      psDCInfo->ui32DeviceID, DC_FLIP_COMMAND,
				      0, NULL, ui32NumSrcSyncs, apsSrcSync,
				      sizeof(struct DISPLAYCLASS_FLIP_COMMAND) +
				      (sizeof(struct IMG_RECT) *
							   ui32ClipRectCount));
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			"PVRSRVSwapToDCBufferKM: Failed to get space in queue");
		goto Exit;
	}

	psFlipCmd = (struct DISPLAYCLASS_FLIP_COMMAND *)psCommand->pvData;
	psFlipCmd->hExtDevice = psDCInfo->hExtDevice;
	psFlipCmd->hExtSwapChain = psBuffer->psSwapChain->hExtSwapChain;
	psFlipCmd->hExtBuffer = psBuffer->sDeviceClassBuffer.hExtBuffer;
	psFlipCmd->hPrivateTag = hPrivateTag;
	psFlipCmd->ui32ClipRectCount = ui32ClipRectCount;
	psFlipCmd->psClipRect =
	    (struct IMG_RECT *)((u8 *) psFlipCmd +
			  sizeof(struct DISPLAYCLASS_FLIP_COMMAND));

	for (i = 0; i < ui32ClipRectCount; i++)
		psFlipCmd->psClipRect[i] = psClipRect[i];

	psFlipCmd->ui32SwapInterval = ui32SwapInterval;

	eError = PVRSRVSubmitCommandKM(psQueue, psCommand);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVSwapToDCBufferKM: Failed to submit command");
		goto Exit;
	}

	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US) {
		if (PVRSRVProcessQueues(IMG_FALSE) !=
		    PVRSRV_ERROR_PROCESSING_BLOCKED) {
			goto ProcessedQueues;
		}
		OSWaitus(MAX_HW_TIME_US / WAIT_TRY_COUNT);
	}
	END_LOOP_UNTIL_TIMEOUT();

	PVR_DPF(PVR_DBG_ERROR,
		 "PVRSRVSwapToDCBufferKM: Failed to process queues");

	eError = PVRSRV_ERROR_GENERIC;
	goto Exit;

ProcessedQueues:

	psBuffer->psSwapChain->psLastFlipBuffer = psBuffer;

Exit:
	return eError;
}

enum PVRSRV_ERROR PVRSRVSwapToDCSystemKM(void *hDeviceKM, void *hSwapChain)
{
	enum PVRSRV_ERROR eError;
	struct PVRSRV_QUEUE_INFO *psQueue;
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct PVRSRV_DC_SWAPCHAIN *psSwapChain;
	struct DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	u32 ui32NumSrcSyncs = 1;
	struct PVRSRV_KERNEL_SYNC_INFO *apsSrcSync[2];
	struct PVRSRV_COMMAND *psCommand;

	if (!hDeviceKM || !hSwapChain) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVSwapToDCSystemKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDCInfo = DCDeviceHandleToDCInfo(hDeviceKM);
	psSwapChain = (struct PVRSRV_DC_SWAPCHAIN *)hSwapChain;

	psQueue = psSwapChain->psQueue;

	apsSrcSync[0] =
	    psDCInfo->sSystemBuffer.sDeviceClassBuffer.psKernelSyncInfo;
	if (psSwapChain->psLastFlipBuffer) {
		if (apsSrcSync[0] !=
		    psSwapChain->psLastFlipBuffer->sDeviceClassBuffer.
			    psKernelSyncInfo) {
			apsSrcSync[1] =
			    psSwapChain->psLastFlipBuffer->sDeviceClassBuffer.
					    psKernelSyncInfo;
			ui32NumSrcSyncs++;
		}
	}

	eError = PVRSRVInsertCommandKM(psQueue, &psCommand,
				      psDCInfo->ui32DeviceID, DC_FLIP_COMMAND,
				      0, NULL, ui32NumSrcSyncs, apsSrcSync,
				      sizeof(struct DISPLAYCLASS_FLIP_COMMAND));
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			"PVRSRVSwapToDCSystemKM: Failed to get space in queue");
		goto Exit;
	}

	psFlipCmd = (struct DISPLAYCLASS_FLIP_COMMAND *)psCommand->pvData;
	psFlipCmd->hExtDevice = psDCInfo->hExtDevice;
	psFlipCmd->hExtSwapChain = psSwapChain->hExtSwapChain;
	psFlipCmd->hExtBuffer =
	    psDCInfo->sSystemBuffer.sDeviceClassBuffer.hExtBuffer;
	psFlipCmd->hPrivateTag = NULL;
	psFlipCmd->ui32ClipRectCount = 0;
	psFlipCmd->ui32SwapInterval = 1;

	eError = PVRSRVSubmitCommandKM(psQueue, psCommand);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVSwapToDCSystemKM: Failed to submit command");
		goto Exit;
	}

	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US) {
		if (PVRSRVProcessQueues(IMG_FALSE) !=
		    PVRSRV_ERROR_PROCESSING_BLOCKED) {
			goto ProcessedQueues;
		}
		OSWaitus(MAX_HW_TIME_US / WAIT_TRY_COUNT);
	}
	END_LOOP_UNTIL_TIMEOUT();

	PVR_DPF(PVR_DBG_ERROR,
		 "PVRSRVSwapToDCSystemKM: Failed to process queues");
	eError = PVRSRV_ERROR_GENERIC;
	goto Exit;

ProcessedQueues:

	psSwapChain->psLastFlipBuffer = &psDCInfo->sSystemBuffer;

	eError = PVRSRV_OK;

Exit:
	return eError;
}

static enum PVRSRV_ERROR PVRSRVRegisterSystemISRHandler(
					IMG_BOOL (*pfnISRHandler)(void *),
					void *pvISRHandlerData,
					u32 ui32ISRSourceMask,
					u32 ui32DeviceID)
{
	struct SYS_DATA *psSysData;
	struct PVRSRV_DEVICE_NODE *psDevNode;

	PVR_UNREFERENCED_PARAMETER(ui32ISRSourceMask);

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVRegisterSystemISRHandler: "
					"Failed to get SysData");
		return PVRSRV_ERROR_GENERIC;
	}

	psDevNode = psSysData->psDeviceNodeList;
	while (psDevNode) {
		if (psDevNode->sDevId.ui32DeviceIndex == ui32DeviceID)
			break;
		psDevNode = psDevNode->psNext;
	}

	if (psDevNode == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVRegisterSystemISRHandler: "
					"Failed to get psDevNode");
		PVR_DBG_BREAK;
		return PVRSRV_ERROR_GENERIC;
	}

	psDevNode->pvISRData = (void *) pvISRHandlerData;

	psDevNode->pfnDeviceISR = pfnISRHandler;

	return PVRSRV_OK;
}

void PVRSRVSetDCState(u32 ui32State)
{
	struct PVRSRV_DISPLAYCLASS_INFO *psDCInfo;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct SYS_DATA *psSysData;

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVSetDCState: Failed to get SysData");
		return;
	}

	psDeviceNode = psSysData->psDeviceNodeList;
	while (psDeviceNode != NULL) {
		if (psDeviceNode->sDevId.eDeviceClass ==
		    PVRSRV_DEVICE_CLASS_DISPLAY) {
			psDCInfo = (struct PVRSRV_DISPLAYCLASS_INFO *)
							psDeviceNode->pvDevice;
			if (psDCInfo->psFuncTable->pfnSetDCState &&
			    psDCInfo->hExtDevice)
				psDCInfo->psFuncTable->pfnSetDCState(
							psDCInfo->hExtDevice,
							ui32State);
		}
		psDeviceNode = psDeviceNode->psNext;
	}
}

IMG_BOOL PVRGetDisplayClassJTable(struct PVRSRV_DC_DISP2SRV_KMJTABLE *psJTable)
{
	psJTable->ui32TableSize = sizeof(struct PVRSRV_DC_DISP2SRV_KMJTABLE);
	psJTable->pfnPVRSRVRegisterDCDevice = PVRSRVRegisterDCDeviceKM;
	psJTable->pfnPVRSRVRemoveDCDevice = PVRSRVRemoveDCDeviceKM;
	psJTable->pfnPVRSRVOEMFunction = SysOEMFunction;
	psJTable->pfnPVRSRVRegisterCmdProcList = PVRSRVRegisterCmdProcListKM;
	psJTable->pfnPVRSRVRemoveCmdProcList = PVRSRVRemoveCmdProcListKM;
	psJTable->pfnPVRSRVCmdComplete = PVRSRVCommandCompleteKM;
	psJTable->pfnPVRSRVRegisterSystemISRHandler =
	    PVRSRVRegisterSystemISRHandler;
	psJTable->pfnPVRSRVRegisterPowerDevice = PVRSRVRegisterPowerDevice;

	return IMG_TRUE;
}
EXPORT_SYMBOL(PVRGetDisplayClassJTable);

enum PVRSRV_ERROR PVRSRVCloseBCDeviceKM(void *hDeviceKM,
				       IMG_BOOL bResManCallback)
{
	struct PVRSRV_BUFFERCLASS_PERCONTEXT_INFO *psBCPerContextInfo;

	PVR_UNREFERENCED_PARAMETER(bResManCallback);

	psBCPerContextInfo = (struct PVRSRV_BUFFERCLASS_PERCONTEXT_INFO *)
				hDeviceKM;

	ResManFreeResByPtr(psBCPerContextInfo->hResItem);

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR CloseBCDeviceCallBack(void *pvParam, u32 ui32Param)
{
	struct PVRSRV_BUFFERCLASS_PERCONTEXT_INFO *psBCPerContextInfo;
	struct PVRSRV_BUFFERCLASS_INFO *psBCInfo;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	psBCPerContextInfo = (struct PVRSRV_BUFFERCLASS_PERCONTEXT_INFO *)
				pvParam;
	psBCInfo = psBCPerContextInfo->psBCInfo;

	psBCInfo->ui32RefCount--;
	if (psBCInfo->ui32RefCount == 0) {
		u32 i;

		psBCInfo->psFuncTable->pfnCloseBCDevice(psBCInfo->hExtDevice);

		for (i = 0; i < psBCInfo->ui32BufferCount; i++)
			if (psBCInfo->psBuffer[i].sDeviceClassBuffer.
				    psKernelSyncInfo)
				PVRSRVFreeSyncInfoKM(psBCInfo->psBuffer[i].
							sDeviceClassBuffer.
							      psKernelSyncInfo);

		if (psBCInfo->psBuffer)
			OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(struct PVRSRV_BC_BUFFER),
				  psBCInfo->psBuffer, NULL);
	}

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
		  sizeof(struct PVRSRV_BUFFERCLASS_PERCONTEXT_INFO),
		  psBCPerContextInfo, NULL);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVOpenBCDeviceKM(
				struct PVRSRV_PER_PROCESS_DATA *psPerProc,
				u32 ui32DeviceID, void *hDevCookie,
				void **phDeviceKM)
{
	struct PVRSRV_BUFFERCLASS_INFO *psBCInfo;
	struct PVRSRV_BUFFERCLASS_PERCONTEXT_INFO *psBCPerContextInfo;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct BUFFER_INFO sBufferInfo;
	struct SYS_DATA *psSysData;
	u32 i;
	enum PVRSRV_ERROR eError;

	if (!phDeviceKM || !hDevCookie) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVOpenBCDeviceKM: Invalid params");
		return PVRSRV_ERROR_GENERIC;
	}

	if (SysAcquireData(&psSysData) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVOpenBCDeviceKM: Failed to get SysData");
		return PVRSRV_ERROR_GENERIC;
	}

	psDeviceNode = psSysData->psDeviceNodeList;
	while (psDeviceNode) {
		if ((psDeviceNode->sDevId.eDeviceClass ==
		     PVRSRV_DEVICE_CLASS_BUFFER) &&
		      (psDeviceNode->sDevId.ui32DeviceIndex == ui32DeviceID)) {

			psBCInfo = (struct PVRSRV_BUFFERCLASS_INFO *)
							psDeviceNode->pvDevice;
			goto FoundDevice;
		}
		psDeviceNode = psDeviceNode->psNext;
	}

	PVR_DPF(PVR_DBG_ERROR,
		 "PVRSRVOpenBCDeviceKM: No devnode matching index %d",
		 ui32DeviceID);

	return PVRSRV_ERROR_GENERIC;

FoundDevice:

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(*psBCPerContextInfo),
		       (void **)&psBCPerContextInfo, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVOpenBCDeviceKM: "
					"Failed psBCPerContextInfo alloc");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	OSMemSet(psBCPerContextInfo, 0, sizeof(*psBCPerContextInfo));

	if (psBCInfo->ui32RefCount++ == 0) {
		psDeviceNode = (struct PVRSRV_DEVICE_NODE *)hDevCookie;

		psBCInfo->hDevMemContext =
		    (void *) psDeviceNode->sDevMemoryInfo.pBMKernelContext;

		eError =
		    psBCInfo->psFuncTable->pfnOpenBCDevice(&psBCInfo->
							   hExtDevice);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "PVRSRVOpenBCDeviceKM: "
					"Failed to open external BC device");
			goto err1;
		}

		eError =
		    psBCInfo->psFuncTable->pfnGetBCInfo(psBCInfo->hExtDevice,
							&sBufferInfo);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				"PVRSRVOpenBCDeviceKM : Failed to get BC Info");
			goto err2;
		}

		psBCInfo->ui32BufferCount = sBufferInfo.ui32BufferCount;

		eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
				    sizeof(struct PVRSRV_BC_BUFFER) *
				    sBufferInfo.ui32BufferCount,
				    (void **) &psBCInfo->psBuffer,
				    NULL);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "PVRSRVOpenBCDeviceKM: "
					"Failed to allocate BC buffers");
			goto err2;
		}
		OSMemSet(psBCInfo->psBuffer, 0,
			 sizeof(struct PVRSRV_BC_BUFFER) *
			 sBufferInfo.ui32BufferCount);

		for (i = 0; i < psBCInfo->ui32BufferCount; i++) {

			eError = PVRSRVAllocSyncInfoKM(NULL,
				      psBCInfo->hDevMemContext,
				      &psBCInfo->psBuffer[i].sDeviceClassBuffer.
							psKernelSyncInfo);
			if (eError != PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR, "PVRSRVOpenBCDeviceKM: "
						"Failed sync info alloc");
				goto err3;
			}

			eError = psBCInfo->psFuncTable->pfnGetBCBuffer(
				    psBCInfo->hExtDevice, i,
				    psBCInfo->psBuffer[i].sDeviceClassBuffer.
							  psKernelSyncInfo->
								     psSyncData,
				    &psBCInfo->psBuffer[i].sDeviceClassBuffer.
							  hExtBuffer);
			if (eError != PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR, "PVRSRVOpenBCDeviceKM: "
						"Failed to get BC buffers");
				goto err3;
			}

			psBCInfo->psBuffer[i].sDeviceClassBuffer.
			    pfnGetBufferAddr =
				    psBCInfo->psFuncTable->pfnGetBufferAddr;
			psBCInfo->psBuffer[i].sDeviceClassBuffer.
			    hDevMemContext = psBCInfo->hDevMemContext;
			psBCInfo->psBuffer[i].sDeviceClassBuffer.hExtDevice =
			    psBCInfo->hExtDevice;
		}
	}

	psBCPerContextInfo->psBCInfo = psBCInfo;
	psBCPerContextInfo->hResItem =
	    ResManRegisterRes(psPerProc->hResManContext,
			      RESMAN_TYPE_BUFFERCLASS_DEVICE,
			      psBCPerContextInfo, 0, CloseBCDeviceCallBack);

	*phDeviceKM = (void *)psBCPerContextInfo;

	return PVRSRV_OK;

err3:
	for (i = 0; i < psBCInfo->ui32BufferCount; i++) {
		if (psBCInfo->psBuffer[i].sDeviceClassBuffer.psKernelSyncInfo) {
			PVRSRVFreeSyncInfoKM(psBCInfo->psBuffer[i].
						     sDeviceClassBuffer.
							     psKernelSyncInfo);
		}
	}
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
		  sizeof(struct PVRSRV_BC_BUFFER) * sBufferInfo.ui32BufferCount,
		  psBCInfo->psBuffer, NULL);
err2:
	psBCInfo->psFuncTable->pfnCloseBCDevice(psBCInfo->hExtDevice);
err1:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*psBCPerContextInfo),
		  psBCPerContextInfo, NULL);
	psBCInfo->ui32RefCount--;

	return eError;
}

enum PVRSRV_ERROR PVRSRVGetBCInfoKM(void *hDeviceKM,
				    struct BUFFER_INFO *psBufferInfo)
{
	struct PVRSRV_BUFFERCLASS_INFO *psBCInfo;
	enum PVRSRV_ERROR eError;

	if (!hDeviceKM || !psBufferInfo) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVGetBCInfoKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psBCInfo = BCDeviceHandleToBCInfo(hDeviceKM);

	eError =
	    psBCInfo->psFuncTable->pfnGetBCInfo(psBCInfo->hExtDevice,
						psBufferInfo);

	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVGetBCInfoKM : Failed to get BC Info");
		return eError;
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVGetBCBufferKM(void *hDeviceKM, u32 ui32BufferIndex,
				 void **phBuffer)
{
	struct PVRSRV_BUFFERCLASS_INFO *psBCInfo;

	if (!hDeviceKM || !phBuffer) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVGetBCBufferKM: Invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psBCInfo = BCDeviceHandleToBCInfo(hDeviceKM);

	if (ui32BufferIndex < psBCInfo->ui32BufferCount) {
		*phBuffer = (void *)&psBCInfo->psBuffer[ui32BufferIndex];
	} else {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVGetBCBufferKM: "
				"Buffer index %d out of range (%d)",
			 ui32BufferIndex, psBCInfo->ui32BufferCount);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	return PVRSRV_OK;
}

IMG_BOOL PVRGetBufferClassJTable(struct PVRSRV_BC_BUFFER2SRV_KMJTABLE *psJTable)
{
	psJTable->ui32TableSize = sizeof(struct PVRSRV_BC_BUFFER2SRV_KMJTABLE);

	psJTable->pfnPVRSRVRegisterBCDevice = PVRSRVRegisterBCDeviceKM;
	psJTable->pfnPVRSRVRemoveBCDevice = PVRSRVRemoveBCDeviceKM;

	return IMG_TRUE;
}
EXPORT_SYMBOL(PVRGetBufferClassJTable);
