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

#include <linux/io.h>

#include "services_headers.h"
#include "buffer_manager.h"
#include "pvr_bridge_km.h"
#include "handle.h"
#include "perproc.h"
#include "pdump_km.h"
#include "ra.h"

#include "pvrversion.h"
enum PVRSRV_ERROR AllocateDeviceID(struct SYS_DATA *psSysData, u32 *pui32DevID)
{
	struct SYS_DEVICE_ID *psDeviceWalker;
	struct SYS_DEVICE_ID *psDeviceEnd;

	psDeviceWalker = &psSysData->sDeviceID[0];
	psDeviceEnd = psDeviceWalker + psSysData->ui32NumDevices;

	while (psDeviceWalker < psDeviceEnd) {
		if (!psDeviceWalker->bInUse) {
			psDeviceWalker->bInUse = IMG_TRUE;
			*pui32DevID = psDeviceWalker->uiID;
			return PVRSRV_OK;
		}
		psDeviceWalker++;
	}

	PVR_DPF(PVR_DBG_ERROR,
		 "AllocateDeviceID: No free and valid device IDs available!");

	PVR_ASSERT(psDeviceWalker < psDeviceEnd);

	return PVRSRV_ERROR_GENERIC;
}

enum PVRSRV_ERROR FreeDeviceID(struct SYS_DATA *psSysData, u32 ui32DevID)
{
	struct SYS_DEVICE_ID *psDeviceWalker;
	struct SYS_DEVICE_ID *psDeviceEnd;

	psDeviceWalker = &psSysData->sDeviceID[0];
	psDeviceEnd = psDeviceWalker + psSysData->ui32NumDevices;

	while (psDeviceWalker < psDeviceEnd) {
		if ((psDeviceWalker->uiID == ui32DevID) &&
		    (psDeviceWalker->bInUse)) {
			psDeviceWalker->bInUse = IMG_FALSE;
			return PVRSRV_OK;
		}
		psDeviceWalker++;
	}

	PVR_DPF(PVR_DBG_ERROR,
		 "FreeDeviceID: no matching dev ID that is in use!");

	PVR_ASSERT(psDeviceWalker < psDeviceEnd);

	return PVRSRV_ERROR_GENERIC;
}

enum PVRSRV_ERROR PVRSRVEnumerateDevicesKM(u32 *pui32NumDevices,
			       struct PVRSRV_DEVICE_IDENTIFIER *psDevIdList)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	u32 i;

	if (!pui32NumDevices || !psDevIdList) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVEnumerateDevicesKM: Invalid params");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVEnumerateDevicesKM: Failed to get SysData");
		return eError;
	}

	for (i = 0; i < PVRSRV_MAX_DEVICES; i++)
		psDevIdList[i].eDeviceType = PVRSRV_DEVICE_TYPE_UNKNOWN;

	*pui32NumDevices = 0;

	psDeviceNode = psSysData->psDeviceNodeList;
	for (i = 0; psDeviceNode != NULL; i++) {
		if (psDeviceNode->sDevId.eDeviceType !=
		    PVRSRV_DEVICE_TYPE_EXT) {
			*psDevIdList++ = psDeviceNode->sDevId;
			(*pui32NumDevices)++;
		}
		psDeviceNode = psDeviceNode->psNext;
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVInit(struct SYS_DATA *psSysData)
{
	enum PVRSRV_ERROR eError;

	eError = ResManInit();
	if (eError != PVRSRV_OK)
		goto Error;

	eError = PVRSRVPerProcessDataInit();
	if (eError != PVRSRV_OK)
		goto Error;

	eError = PVRSRVHandleInit();
	if (eError != PVRSRV_OK)
		goto Error;

	eError = OSCreateResource(&psSysData->sPowerStateChangeResource);
	if (eError != PVRSRV_OK)
		goto Error;

	psSysData->eCurrentPowerState = PVRSRV_POWER_STATE_D0;
	psSysData->eFailedPowerState = PVRSRV_POWER_Unspecified;

#if defined(PDUMP)
	psSysData->bPowerUpPDumped = IMG_FALSE;
#endif

	if (OSAllocMem(PVRSRV_PAGEABLE_SELECT,
		       sizeof(struct PVRSRV_EVENTOBJECT),
		       (void **) &psSysData->psGlobalEventObject,
		       NULL) != PVRSRV_OK)

		goto Error;

	if (OSEventObjectCreate("PVRSRV_GLOBAL_EVENTOBJECT",
				psSysData->psGlobalEventObject) != PVRSRV_OK)
		goto Error;

	return eError;

Error:
	PVRSRVDeInit(psSysData);
	return eError;
}

void PVRSRVDeInit(struct SYS_DATA *psSysData)
{
	enum PVRSRV_ERROR eError;

	PVR_UNREFERENCED_PARAMETER(psSysData);

	if (psSysData == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVDeInit: "
				"PVRSRVHandleDeInit failed - invalid param");
		return;
	}

	if (psSysData->psGlobalEventObject) {
		OSEventObjectDestroy(psSysData->psGlobalEventObject);
		OSFreeMem(PVRSRV_PAGEABLE_SELECT,
			  sizeof(struct PVRSRV_EVENTOBJECT),
			  psSysData->psGlobalEventObject, NULL);
	}

	eError = PVRSRVHandleDeInit();
	if (eError != PVRSRV_OK)
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVDeInit: PVRSRVHandleDeInit failed");

	eError = PVRSRVPerProcessDataDeInit();
	if (eError != PVRSRV_OK)
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVDeInit: PVRSRVPerProcessDataDeInit failed");

	ResManDeInit();
}

enum PVRSRV_ERROR PVRSRVRegisterDevice(struct SYS_DATA *psSysData,
				  enum PVRSRV_ERROR(*pfnRegisterDevice)
				  (struct PVRSRV_DEVICE_NODE *),
				  u32 ui32SOCInterruptBit,
				  u32 *pui32DeviceIndex)
{
	enum PVRSRV_ERROR eError;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;

	if (OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
		       sizeof(struct PVRSRV_DEVICE_NODE),
		       (void **) &psDeviceNode, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVRegisterDevice : "
				"Failed to alloc memory for psDeviceNode");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	OSMemSet(psDeviceNode, 0, sizeof(struct PVRSRV_DEVICE_NODE));

	eError = pfnRegisterDevice(psDeviceNode);
	if (eError != PVRSRV_OK) {
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_DEVICE_NODE), psDeviceNode,
			  NULL);
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVRegisterDevice : "
					"Failed to register device");
		return PVRSRV_ERROR_DEVICE_REGISTER_FAILED;
	}

	psDeviceNode->ui32RefCount = 1;
	psDeviceNode->psSysData = psSysData;
	psDeviceNode->ui32SOCInterruptBit = ui32SOCInterruptBit;

	AllocateDeviceID(psSysData, &psDeviceNode->sDevId.ui32DeviceIndex);

	psDeviceNode->psNext = psSysData->psDeviceNodeList;
	psSysData->psDeviceNodeList = psDeviceNode;

	*pui32DeviceIndex = psDeviceNode->sDevId.ui32DeviceIndex;

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVInitialiseDevice(u32 ui32DevIndex)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct SYS_DATA *psSysData;
	enum PVRSRV_ERROR eError;

	PVR_DPF(PVR_DBG_MESSAGE, "PVRSRVInitialiseDevice");

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVInitialiseDevice: Failed to get SysData");
		return eError;
	}

	psDeviceNode = psSysData->psDeviceNodeList;

	while (psDeviceNode) {
		if (psDeviceNode->sDevId.ui32DeviceIndex == ui32DevIndex)
			goto FoundDevice;
		psDeviceNode = psDeviceNode->psNext;
	}

	PVR_DPF(PVR_DBG_ERROR,
		 "PVRSRVInitialiseDevice: requested device is not present");
	return PVRSRV_ERROR_INIT_FAILURE;

FoundDevice:

	PVR_ASSERT(psDeviceNode->ui32RefCount > 0);

	eError = PVRSRVResManConnect(NULL, &psDeviceNode->hResManContext);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVInitialiseDevice: "
					"Failed PVRSRVResManConnect call");
		return eError;
	}

	if (psDeviceNode->pfnInitDevice != NULL) {
		eError = psDeviceNode->pfnInitDevice(psDeviceNode);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "PVRSRVInitialiseDevice: "
						"Failed InitDevice call");
			return eError;
		}
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVFinaliseSystem(IMG_BOOL bInitSuccessful)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct SYS_DATA *psSysData;
	enum PVRSRV_ERROR eError;

	PVR_DPF(PVR_DBG_MESSAGE, "PVRSRVFinaliseSystem");

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVFinaliseSystem: Failed to get SysData");
		return eError;
	}

	if (bInitSuccessful) {
		eError = SysFinalise();
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				"PVRSRVFinaliseSystem: SysFinalise failed (%d)",
				 eError);
			return eError;
		}

		psDeviceNode = psSysData->psDeviceNodeList;
		while (psDeviceNode) {
			eError =
			    PVRSRVSetDevicePowerStateKM(psDeviceNode->sDevId.
						ui32DeviceIndex,
						PVRSRV_POWER_Unspecified,
						KERNEL_ID, IMG_FALSE);
			if (eError != PVRSRV_OK)
				PVR_DPF(PVR_DBG_ERROR, "PVRSRVFinaliseSystem: "
					"Failed PVRSRVSetDevicePowerStateKM "
					"call (device index: %d)",
				 psDeviceNode->sDevId.ui32DeviceIndex);
			psDeviceNode = psDeviceNode->psNext;
		}

		psDeviceNode = psSysData->psDeviceNodeList;
		while (psDeviceNode) {
			if (psDeviceNode->pfnInitDeviceCompatCheck) {
				eError = PVRSRVDevInitCompatCheck(psDeviceNode);
				if (eError != PVRSRV_OK) {
					PVR_DPF(PVR_DBG_ERROR,
					"PVRSRVFinaliseSystem: "
					"Failed PVRSRVDevInitCompatCheck "
					"call (device index: %d)",
						 psDeviceNode->sDevId.
						 ui32DeviceIndex);
					return eError;
				}
			}
			psDeviceNode = psDeviceNode->psNext;

		}

	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVDevInitCompatCheck(struct PVRSRV_DEVICE_NODE
					   *psDeviceNode)
{

	return psDeviceNode->pfnInitDeviceCompatCheck(psDeviceNode);
}

enum PVRSRV_ERROR PVRSRVAcquireDeviceDataKM(u32 ui32DevIndex,
				enum PVRSRV_DEVICE_TYPE eDeviceType,
				void **phDevCookie)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct SYS_DATA *psSysData;
	enum PVRSRV_ERROR eError;

	PVR_DPF(PVR_DBG_MESSAGE, "PVRSRVAcquireDeviceDataKM");

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVAcquireDeviceDataKM: Failed to get SysData");
		return eError;
	}

	psDeviceNode = psSysData->psDeviceNodeList;

	if (eDeviceType != PVRSRV_DEVICE_TYPE_UNKNOWN) {
		while (psDeviceNode) {
			if (psDeviceNode->sDevId.eDeviceType == eDeviceType)
				goto FoundDevice;
			psDeviceNode = psDeviceNode->psNext;
		}
	} else {
		while (psDeviceNode) {
			if (psDeviceNode->sDevId.ui32DeviceIndex ==
			    ui32DevIndex) {
				goto FoundDevice;
			}
			psDeviceNode = psDeviceNode->psNext;
		}
	}

	PVR_DPF(PVR_DBG_ERROR,
		 "PVRSRVAcquireDeviceDataKM: requested device is not present");
	return PVRSRV_ERROR_INIT_FAILURE;

FoundDevice:

	PVR_ASSERT(psDeviceNode->ui32RefCount > 0);

	if (phDevCookie)
		*phDevCookie = (void *) psDeviceNode;

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVDeinitialiseDevice(u32 ui32DevIndex)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct PVRSRV_DEVICE_NODE **ppsDevNode;
	struct SYS_DATA *psSysData;
	enum PVRSRV_ERROR eError;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVDeinitialiseDevice: Failed to get SysData");
		return eError;
	}

	ppsDevNode = &psSysData->psDeviceNodeList;
	while (*ppsDevNode) {
		if ((*ppsDevNode)->sDevId.ui32DeviceIndex == ui32DevIndex) {
			psDeviceNode = *ppsDevNode;
			goto FoundDevice;
		}
		ppsDevNode = &((*ppsDevNode)->psNext);
	}

	PVR_DPF(PVR_DBG_ERROR,
		 "PVRSRVDeinitialiseDevice: requested device %d is not present",
		 ui32DevIndex);

	return PVRSRV_ERROR_GENERIC;

FoundDevice:

	eError = PVRSRVSetDevicePowerStateKM(ui32DevIndex,
					     PVRSRV_POWER_STATE_D3,
					     KERNEL_ID, IMG_FALSE);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVDeinitialiseDevice: "
				"Failed PVRSRVSetDevicePowerStateKM call");
		return eError;
	}

	ResManFreeResByCriteria(psDeviceNode->hResManContext,
					 RESMAN_CRITERIA_RESTYPE,
					 RESMAN_TYPE_DEVICEMEM_ALLOCATION,
					 NULL, 0);

	if (psDeviceNode->pfnDeInitDevice != NULL) {
		eError = psDeviceNode->pfnDeInitDevice(psDeviceNode);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "PVRSRVDeinitialiseDevice: "
						"Failed DeInitDevice call");
			return eError;
		}
	}

	PVRSRVResManDisconnect(psDeviceNode->hResManContext, IMG_TRUE);
	psDeviceNode->hResManContext = NULL;

	*ppsDevNode = psDeviceNode->psNext;

	FreeDeviceID(psSysData, ui32DevIndex);
	OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
		  sizeof(struct PVRSRV_DEVICE_NODE), psDeviceNode, NULL);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PollForValueKM(u32 __iomem *pui32LinMemAddr,
				 u32 ui32Value, u32 ui32Mask, u32 ui32Waitus,
				 u32 ui32Tries)
{
	u32 uiMaxTime;

	uiMaxTime = ui32Tries * ui32Waitus;

	LOOP_UNTIL_TIMEOUT(uiMaxTime) {
		if ((readl(pui32LinMemAddr) & ui32Mask) == ui32Value)
			return PVRSRV_OK;
		OSWaitus(ui32Waitus);
	}
	END_LOOP_UNTIL_TIMEOUT();

	return PVRSRV_ERROR_GENERIC;
}


enum PVRSRV_ERROR PVRSRVGetMiscInfoKM(struct PVRSRV_MISC_INFO *psMiscInfo)
{
	struct SYS_DATA *psSysData;
	enum PVRSRV_ERROR eError;

	if (!psMiscInfo) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVGetMiscInfoKM: invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psMiscInfo->ui32StatePresent = 0;

	if (psMiscInfo->ui32StateRequest & ~(
				PVRSRV_MISC_INFO_TIMER_PRESENT		   |
			  PVRSRV_MISC_INFO_CLOCKGATE_PRESENT |
			  PVRSRV_MISC_INFO_MEMSTATS_PRESENT |
				PVRSRV_MISC_INFO_GLOBALEVENTOBJECT_PRESENT |
				PVRSRV_MISC_INFO_DDKVERSION_PRESENT)) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVGetMiscInfoKM: invalid state request flags");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVGetMiscInfoKM: Failed to get SysData");
		return eError;
	}

	if (((psMiscInfo->ui32StateRequest &
	     PVRSRV_MISC_INFO_TIMER_PRESENT) != 0UL) &&
	     (psSysData->pvSOCTimerRegisterKM != NULL)) {
		psMiscInfo->ui32StatePresent |= PVRSRV_MISC_INFO_TIMER_PRESENT;
		psMiscInfo->pvSOCTimerRegisterKM =
		    psSysData->pvSOCTimerRegisterKM;
		psMiscInfo->hSOCTimerRegisterOSMemHandle =
		    psSysData->hSOCTimerRegisterOSMemHandle;
	} else {
		psMiscInfo->pvSOCTimerRegisterKM = NULL;
		psMiscInfo->hSOCTimerRegisterOSMemHandle = NULL;
	}

	if (((psMiscInfo->ui32StateRequest &
	    PVRSRV_MISC_INFO_CLOCKGATE_PRESENT) != 0UL) &&
	    (psSysData->pvSOCClockGateRegsBase != NULL)) {
		psMiscInfo->ui32StatePresent |=
		    PVRSRV_MISC_INFO_CLOCKGATE_PRESENT;
		psMiscInfo->pvSOCClockGateRegs =
		    psSysData->pvSOCClockGateRegsBase;
		psMiscInfo->ui32SOCClockGateRegsSize =
		    psSysData->ui32SOCClockGateRegsSize;
	}

	if (((psMiscInfo->ui32StateRequest &
	      PVRSRV_MISC_INFO_MEMSTATS_PRESENT) != 0UL) &&
	     (psMiscInfo->pszMemoryStr != NULL)) {
		struct RA_ARENA **ppArena;
		struct BM_HEAP *psBMHeap;
		struct BM_CONTEXT *psBMContext;
		struct PVRSRV_DEVICE_NODE *psDeviceNode;
		char *pszStr;
		u32 ui32StrLen;
		s32 i32Count;

		pszStr = psMiscInfo->pszMemoryStr;
		ui32StrLen = psMiscInfo->ui32MemoryStrLen;

		psMiscInfo->ui32StatePresent |=
		    PVRSRV_MISC_INFO_MEMSTATS_PRESENT;

		ppArena = &psSysData->apsLocalDevMemArena[0];
		while (*ppArena) {
			CHECK_SPACE(ui32StrLen);
			i32Count =
			    OSSNPrintf(pszStr, 100, "\nLocal Backing Store:\n");
			UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

			RA_GetStats(*ppArena, &pszStr, &ui32StrLen);

			ppArena++;
		}

		psDeviceNode = psSysData->psDeviceNodeList;
		while (psDeviceNode) {
			CHECK_SPACE(ui32StrLen);
			i32Count =
			    OSSNPrintf(pszStr, 100, "\n\nDevice Type %d:\n",
				       psDeviceNode->sDevId.eDeviceType);
			UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

			if (psDeviceNode->sDevMemoryInfo.pBMKernelContext) {
				CHECK_SPACE(ui32StrLen);
				i32Count =
				    OSSNPrintf(pszStr, 100,
					       "\nKernel Context:\n");
				UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

				psBMHeap =
				    psDeviceNode->sDevMemoryInfo.
				    pBMKernelContext->psBMHeap;
				while (psBMHeap) {
					if (psBMHeap->pImportArena) {
						RA_GetStats(psBMHeap->
								pImportArena,
							    &pszStr,
							    &ui32StrLen);
					}

					if (psBMHeap->pVMArena) {
						RA_GetStats(psBMHeap->pVMArena,
							    &pszStr,
							    &ui32StrLen);
					}
					psBMHeap = psBMHeap->psNext;
				}
			}

			psBMContext = psDeviceNode->sDevMemoryInfo.pBMContext;
			while (psBMContext) {
				CHECK_SPACE(ui32StrLen);
				i32Count =
				    OSSNPrintf(pszStr, 100,
			"\nApplication Context (hDevMemContext) 0x%08X:\n",
						   (void *)psBMContext);
				UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

				psBMHeap = psBMContext->psBMHeap;
				while (psBMHeap) {
					if (psBMHeap->pImportArena) {
						RA_GetStats(psBMHeap->
								pImportArena,
							    &pszStr,
							    &ui32StrLen);
					}

					if (psBMHeap->pVMArena) {
						RA_GetStats(psBMHeap->pVMArena,
							    &pszStr,
							    &ui32StrLen);
					}
					psBMHeap = psBMHeap->psNext;
				}
				psBMContext = psBMContext->psNext;
			}
			psDeviceNode = psDeviceNode->psNext;
		}

		i32Count = OSSNPrintf(pszStr, 100, "\n\0");
		UPDATE_SPACE(pszStr, i32Count, ui32StrLen);
	}

	if (((psMiscInfo->ui32StateRequest &
	      PVRSRV_MISC_INFO_GLOBALEVENTOBJECT_PRESENT) != 0UL) &&
	     (psSysData->psGlobalEventObject != NULL)) {
		psMiscInfo->ui32StatePresent |=
		    PVRSRV_MISC_INFO_GLOBALEVENTOBJECT_PRESENT;
		psMiscInfo->sGlobalEventObject =
		    *psSysData->psGlobalEventObject;
	}

	if (((psMiscInfo->ui32StateRequest &
			PVRSRV_MISC_INFO_DDKVERSION_PRESENT) != 0UL) &&
	    ((psMiscInfo->ui32StateRequest &
			PVRSRV_MISC_INFO_MEMSTATS_PRESENT) == 0UL) &&
	     (psMiscInfo->pszMemoryStr != NULL)) {
		char *pszStr;
		u32 ui32StrLen;
		u32 ui32LenStrPerNum = 12;
		s32 i32Count;
		int i;
		psMiscInfo->ui32StatePresent |=
		    PVRSRV_MISC_INFO_DDKVERSION_PRESENT;

		psMiscInfo->aui32DDKVersion[0] = PVRVERSION_MAJ;
		psMiscInfo->aui32DDKVersion[1] = PVRVERSION_MIN;
		psMiscInfo->aui32DDKVersion[2] = PVRVERSION_BRANCH;
		psMiscInfo->aui32DDKVersion[3] = PVRVERSION_BUILD;

		pszStr = psMiscInfo->pszMemoryStr;
		ui32StrLen = psMiscInfo->ui32MemoryStrLen;

		for (i = 0; i < 4; i++) {
			if (ui32StrLen < ui32LenStrPerNum)
				return PVRSRV_ERROR_INVALID_PARAMS;

			i32Count = OSSNPrintf(pszStr, ui32LenStrPerNum, "%ld",
				       psMiscInfo->aui32DDKVersion[i]);
			UPDATE_SPACE(pszStr, i32Count, ui32StrLen);
			if (i != 3) {
				i32Count = OSSNPrintf(pszStr, 2, ".");
				UPDATE_SPACE(pszStr, i32Count, ui32StrLen);
			}
		}
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVGetFBStatsKM(u32 *pui32Total, u32 *pui32Available)
{
	u32 ui32Total = 0, i = 0;
	u32 ui32Available = 0;

	*pui32Total = 0;
	*pui32Available = 0;

	while (BM_ContiguousStatistics(i, &ui32Total, &ui32Available) ==
	       IMG_TRUE) {
		*pui32Total += ui32Total;
		*pui32Available += ui32Available;

		i++;
	}

	return PVRSRV_OK;
}

IMG_BOOL PVRSRVDeviceLISR(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	struct SYS_DATA *psSysData;
	IMG_BOOL bStatus = IMG_FALSE;
	u32 ui32InterruptSource;

	if (!psDeviceNode) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVDeviceLISR: Invalid params\n");
		goto out;
	}
	psSysData = psDeviceNode->psSysData;

	ui32InterruptSource = SysGetInterruptSource(psSysData, psDeviceNode);
	if (ui32InterruptSource & psDeviceNode->ui32SOCInterruptBit) {
		if (psDeviceNode->pfnDeviceISR != NULL)
			bStatus =
			    (*psDeviceNode->pfnDeviceISR) (psDeviceNode->
							   pvISRData);

		SysClearInterrupts(psSysData,
				   psDeviceNode->ui32SOCInterruptBit);
	}

out:
	return bStatus;
}

IMG_BOOL PVRSRVSystemLISR(void *pvSysData)
{
	struct SYS_DATA *psSysData = pvSysData;
	IMG_BOOL bStatus = IMG_FALSE;
	u32 ui32InterruptSource;
	u32 ui32ClearInterrupts = 0;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;

	if (!psSysData) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVSystemLISR: Invalid params\n");
		goto out;
	}

	ui32InterruptSource = SysGetInterruptSource(psSysData, NULL);

	if (ui32InterruptSource == 0)
		goto out;

	psDeviceNode = psSysData->psDeviceNodeList;
	while (psDeviceNode != NULL) {
		if (psDeviceNode->pfnDeviceISR != NULL)
			if (ui32InterruptSource & psDeviceNode->
			    ui32SOCInterruptBit) {
				if ((*psDeviceNode->pfnDeviceISR)
						(psDeviceNode->pvISRData))
					bStatus = IMG_TRUE;

				ui32ClearInterrupts |=
				    psDeviceNode->ui32SOCInterruptBit;
			}
		psDeviceNode = psDeviceNode->psNext;
	}

	SysClearInterrupts(psSysData, ui32ClearInterrupts);

out:
	return bStatus;
}

void PVRSRVMISR(void *pvSysData)
{
	struct SYS_DATA *psSysData = pvSysData;
	struct PVRSRV_DEVICE_NODE *psDeviceNode;

	if (!psSysData) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVMISR: Invalid params\n");
		return;
	}

	pvr_lock();

	psDeviceNode = psSysData->psDeviceNodeList;
	while (psDeviceNode != NULL) {
		if (psDeviceNode->pfnDeviceMISR != NULL)
			(*psDeviceNode->pfnDeviceMISR)(psDeviceNode->
								pvISRData);
		psDeviceNode = psDeviceNode->psNext;
	}

	if (PVRSRVProcessQueues(ISR_ID, IMG_FALSE) ==
	    PVRSRV_ERROR_PROCESSING_BLOCKED)
		PVRSRVProcessQueues(ISR_ID, IMG_FALSE);

	if (psSysData->psGlobalEventObject) {
		void *hOSEventKM =
		    psSysData->psGlobalEventObject->hOSEventKM;
		if (hOSEventKM)
			OSEventObjectSignal(hOSEventKM);
	}

	pvr_unlock();
}

enum PVRSRV_ERROR PVRSRVProcessConnect(u32 ui32PID)
{
	return PVRSRVPerProcessDataConnect(ui32PID);
}

void PVRSRVProcessDisconnect(u32 ui32PID)
{
	PVRSRVPerProcessDataDisconnect(ui32PID);
}

enum PVRSRV_ERROR PVRSRVSaveRestoreLiveSegments(void *hArena, u8 *pbyBuffer,
					   u32 *puiBufSize, IMG_BOOL bSave)
{
	u32 uiBytesSaved = 0;
	void *pvLocalMemCPUVAddr;
	struct RA_SEGMENT_DETAILS sSegDetails;

	if (hArena == NULL)
		return PVRSRV_ERROR_INVALID_PARAMS;

	sSegDetails.uiSize = 0;
	sSegDetails.sCpuPhyAddr.uiAddr = 0;
	sSegDetails.hSegment = NULL;

	while (RA_GetNextLiveSegment(hArena, &sSegDetails))
		if (pbyBuffer == NULL) {
			uiBytesSaved +=
			    sizeof(sSegDetails.uiSize) + sSegDetails.uiSize;
		} else {
			if ((uiBytesSaved + sizeof(sSegDetails.uiSize) +
			     sSegDetails.uiSize) > *puiBufSize)
				return PVRSRV_ERROR_OUT_OF_MEMORY;

			PVR_DPF(PVR_DBG_MESSAGE,
				 "PVRSRVSaveRestoreLiveSegments: "
				 "Base %08x size %08x",
				 sSegDetails.sCpuPhyAddr.uiAddr,
				 sSegDetails.uiSize);

			pvLocalMemCPUVAddr = (void __force *)
			    OSMapPhysToLin(sSegDetails.sCpuPhyAddr,
					   sSegDetails.uiSize,
					   PVRSRV_HAP_KERNEL_ONLY |
					   PVRSRV_HAP_UNCACHED, NULL);
			if (pvLocalMemCPUVAddr == NULL) {
				PVR_DPF(PVR_DBG_ERROR,
					 "PVRSRVSaveRestoreLiveSegments: "
					 "Failed to map local memory to host");
				return PVRSRV_ERROR_OUT_OF_MEMORY;
			}

			if (bSave) {
				OSMemCopy(pbyBuffer, &sSegDetails.uiSize,
					  sizeof(sSegDetails.uiSize));
				pbyBuffer += sizeof(sSegDetails.uiSize);

				OSMemCopy(pbyBuffer, pvLocalMemCPUVAddr,
					  sSegDetails.uiSize);
				pbyBuffer += sSegDetails.uiSize;
			} else {
				u32 uiSize;

				OSMemCopy(&uiSize, pbyBuffer,
					  sizeof(sSegDetails.uiSize));

				if (uiSize != sSegDetails.uiSize) {
					PVR_DPF(PVR_DBG_ERROR,
						"PVRSRVSaveRestoreLiveSegments:"
						" Segment size error");
				} else {
					pbyBuffer += sizeof(sSegDetails.uiSize);

					OSMemCopy(pvLocalMemCPUVAddr, pbyBuffer,
						  sSegDetails.uiSize);
					pbyBuffer += sSegDetails.uiSize;
				}
			}

			uiBytesSaved +=
			    sizeof(sSegDetails.uiSize) + sSegDetails.uiSize;

			OSUnMapPhysToLin((void __force __iomem *)
							pvLocalMemCPUVAddr,
					 sSegDetails.uiSize,
					 PVRSRV_HAP_KERNEL_ONLY |
					 PVRSRV_HAP_UNCACHED, NULL);
		}

	if (pbyBuffer == NULL)
		*puiBufSize = uiBytesSaved;

	return PVRSRV_OK;
}
