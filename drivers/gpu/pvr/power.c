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
#include "services_headers.h"
#include "pdump_km.h"
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/wait.h>

static IMG_BOOL gbInitServerRunning;
static IMG_BOOL gbInitServerRan;
static IMG_BOOL gbInitSuccessful;
static DEFINE_MUTEX(hPowerAndFreqLock);

enum PVRSRV_ERROR PVRSRVSetInitServerState(enum PVRSRV_INIT_SERVER_STATE
					  eInitServerState, IMG_BOOL bState)
{

	switch (eInitServerState) {
	case PVRSRV_INIT_SERVER_RUNNING:
		gbInitServerRunning = bState;
		break;
	case PVRSRV_INIT_SERVER_RAN:
		gbInitServerRan = bState;
		break;
	case PVRSRV_INIT_SERVER_SUCCESSFUL:
		gbInitSuccessful = bState;
		break;
	default:
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVSetInitServerState : Unknown state %lx",
			 eInitServerState);
		return PVRSRV_ERROR_GENERIC;
	}

	return PVRSRV_OK;
}

IMG_BOOL PVRSRVGetInitServerState(
				enum PVRSRV_INIT_SERVER_STATE eInitServerState)
{
	IMG_BOOL bReturnVal;

	switch (eInitServerState) {
	case PVRSRV_INIT_SERVER_RUNNING:
		bReturnVal = gbInitServerRunning;
		break;
	case PVRSRV_INIT_SERVER_RAN:
		bReturnVal = gbInitServerRan;
		break;
	case PVRSRV_INIT_SERVER_SUCCESSFUL:
		bReturnVal = gbInitSuccessful;
		break;
	default:
		PVR_DPF(PVR_DBG_ERROR,
			 "PVRSRVGetInitServerState : Unknown state %lx",
			 eInitServerState);
		bReturnVal = IMG_FALSE;
	}

	return bReturnVal;
}

static IMG_BOOL _IsSystemStatePowered(enum PVR_POWER_STATE eSystemPowerState)
{
	return (IMG_BOOL)(eSystemPowerState < PVRSRV_POWER_STATE_D2);
}

static enum PVRSRV_ERROR PVRSRVDevicePrePowerStateKM(IMG_BOOL bAllDevices,
					 u32 ui32DeviceIndex,
					 enum PVR_POWER_STATE eNewPowerState)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;
	struct PVRSRV_POWER_DEV *psPowerDevice;
	enum PVR_POWER_STATE eNewDevicePowerState;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return eError;

	psPowerDevice = psSysData->psPowerDeviceList;
	while (psPowerDevice) {
		if (bAllDevices ||
		    (ui32DeviceIndex == psPowerDevice->ui32DeviceIndex)) {
			eNewDevicePowerState =
			    (eNewPowerState == PVRSRV_POWER_Unspecified) ?
					psPowerDevice->eDefaultPowerState :
					eNewPowerState;

			if (psPowerDevice->eCurrentPowerState !=
			    eNewDevicePowerState) {
				if (psPowerDevice->pfnPrePower != NULL) {
					eError =
					    psPowerDevice->
					    pfnPrePower(psPowerDevice->
							hDevCookie,
							eNewDevicePowerState,
							psPowerDevice->
							eCurrentPowerState);
					if (eError != PVRSRV_OK) {
						pr_err
						   ("pfnPrePower failed (%u)\n",
						     eError);
						return eError;
					}
				}

				eError = SysDevicePrePowerState(
					     psPowerDevice->ui32DeviceIndex,
					     eNewDevicePowerState,
					     psPowerDevice->eCurrentPowerState);
				if (eError != PVRSRV_OK) {
					pr_err("SysDevicePrePowerState failed "
						"(%u)\n", eError);
					return eError;
				}
			}
		}

		psPowerDevice = psPowerDevice->psNext;
	}

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR PVRSRVDevicePostPowerStateKM(IMG_BOOL bAllDevices,
					  u32 ui32DeviceIndex,
					  enum PVR_POWER_STATE eNewPowerState)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;
	struct PVRSRV_POWER_DEV *psPowerDevice;
	enum PVR_POWER_STATE eNewDevicePowerState;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return eError;

	psPowerDevice = psSysData->psPowerDeviceList;
	while (psPowerDevice) {
		if (bAllDevices ||
		    (ui32DeviceIndex == psPowerDevice->ui32DeviceIndex)) {
			eNewDevicePowerState = (eNewPowerState ==
			     PVRSRV_POWER_Unspecified) ? psPowerDevice->
			     eDefaultPowerState : eNewPowerState;

			if (psPowerDevice->eCurrentPowerState !=
			    eNewDevicePowerState) {
				eError = SysDevicePostPowerState(
					    psPowerDevice->ui32DeviceIndex,
					    eNewDevicePowerState,
					    psPowerDevice->eCurrentPowerState);
				if (eError != PVRSRV_OK) {
					pr_err("SysDevicePostPowerState "
						"failed (%u)\n", eError);
					return eError;
				}

				if (psPowerDevice->pfnPostPower != NULL) {
					eError =
					    psPowerDevice->
					    pfnPostPower(psPowerDevice->
							 hDevCookie,
							 eNewDevicePowerState,
							 psPowerDevice->
							 eCurrentPowerState);
					if (eError != PVRSRV_OK) {
						pr_err
						    ("pfnPostPower failed "
						     "(%u)\n", eError);
						return eError;
					}
				}

				psPowerDevice->eCurrentPowerState =
				    eNewDevicePowerState;
			}
		}

		psPowerDevice = psPowerDevice->psNext;
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVSetDevicePowerStateKM(u32 ui32DeviceIndex,
				     enum PVR_POWER_STATE eNewPowerState,
				     u32 ui32CallerID, IMG_BOOL bRetainMutex)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return eError;

#if defined(PDUMP)
	if (eNewPowerState == PVRSRV_POWER_Unspecified) {
		eError =
		    PVRSRVDevicePrePowerStateKM(IMG_FALSE, ui32DeviceIndex,
							PVRSRV_POWER_STATE_D0);
		if (eError != PVRSRV_OK)
			goto Exit;
		eError =
		    PVRSRVDevicePostPowerStateKM(IMG_FALSE, ui32DeviceIndex,
							 PVRSRV_POWER_STATE_D0);
		if (eError != PVRSRV_OK)
			goto Exit;

		PDUMPSUSPEND();
	}
#endif

	eError = PVRSRVDevicePrePowerStateKM(IMG_FALSE, ui32DeviceIndex,
						eNewPowerState);
	if (eError != PVRSRV_OK) {
		if (eNewPowerState == PVRSRV_POWER_Unspecified)
			PDUMPRESUME();
		goto Exit;
	}

	eError = PVRSRVDevicePostPowerStateKM(IMG_FALSE, ui32DeviceIndex,
					 eNewPowerState);

	if (eNewPowerState == PVRSRV_POWER_Unspecified)
		PDUMPRESUME();

Exit:

	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVSetDevicePowerStateKM : "
					"Transition to %d FAILED 0x%x",
			 eNewPowerState, eError);
	}

	return eError;
}

enum PVRSRV_ERROR PVRSRVSystemPrePowerStateKM(
		enum PVR_POWER_STATE eNewPowerState)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;
	enum PVR_POWER_STATE eNewDevicePowerState;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return eError;

	if (_IsSystemStatePowered(eNewPowerState) !=
	    _IsSystemStatePowered(psSysData->eCurrentPowerState)) {
		if (_IsSystemStatePowered(eNewPowerState))
			eNewDevicePowerState = PVRSRV_POWER_Unspecified;
		else
			eNewDevicePowerState = PVRSRV_POWER_STATE_D3;

		eError = PVRSRVDevicePrePowerStateKM(IMG_TRUE, 0,
						eNewDevicePowerState);
		if (eError != PVRSRV_OK)
			goto ErrorExit;
	}

	if (eNewPowerState != psSysData->eCurrentPowerState) {
		eError = SysSystemPrePowerState(eNewPowerState);
		if (eError != PVRSRV_OK)
			goto ErrorExit;
	}

	return eError;

ErrorExit:

	PVR_DPF(PVR_DBG_ERROR, "PVRSRVSystemPrePowerStateKM: "
				"Transition from %d to %d FAILED 0x%x",
		 psSysData->eCurrentPowerState, eNewPowerState, eError);

	psSysData->eFailedPowerState = eNewPowerState;

	return eError;
}

enum PVRSRV_ERROR PVRSRVSystemPostPowerStateKM(
		enum PVR_POWER_STATE eNewPowerState)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;
	enum PVR_POWER_STATE eNewDevicePowerState;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		goto Exit;

	if (eNewPowerState != psSysData->eCurrentPowerState) {
		eError = SysSystemPostPowerState(eNewPowerState);
		if (eError != PVRSRV_OK)
			goto Exit;
	}

	if (_IsSystemStatePowered(eNewPowerState) !=
	    _IsSystemStatePowered(psSysData->eCurrentPowerState)) {
		if (_IsSystemStatePowered(eNewPowerState))
			eNewDevicePowerState = PVRSRV_POWER_Unspecified;
		else
			eNewDevicePowerState = PVRSRV_POWER_STATE_D3;

		eError =
		    PVRSRVDevicePostPowerStateKM(IMG_TRUE, 0,
						 eNewDevicePowerState);
		if (eError != PVRSRV_OK)
			goto Exit;
	}

	PVR_DPF(PVR_DBG_WARNING, "PVRSRVSystemPostPowerStateKM: "
				  "System Power Transition from %d to %d OK",
		 psSysData->eCurrentPowerState, eNewPowerState);

	psSysData->eCurrentPowerState = eNewPowerState;

Exit:

	if (_IsSystemStatePowered(eNewPowerState) &&
	    PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_SUCCESSFUL))
		PVRSRVCommandCompleteCallbacks();

	return eError;
}

enum PVRSRV_ERROR PVRSRVSetPowerStateKM(enum PVR_POWER_STATE eNewPowerState)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return eError;

	eError = PVRSRVSystemPrePowerStateKM(eNewPowerState);
	if (eError != PVRSRV_OK)
		goto ErrorExit;

	eError = PVRSRVSystemPostPowerStateKM(eNewPowerState);
	if (eError != PVRSRV_OK)
		goto ErrorExit;

	psSysData->eFailedPowerState = PVRSRV_POWER_Unspecified;

	return PVRSRV_OK;

ErrorExit:

	PVR_DPF(PVR_DBG_ERROR, "PVRSRVSetPowerStateKM: "
				"Transition from %d to %d FAILED 0x%x",
		 psSysData->eCurrentPowerState, eNewPowerState, eError);

	psSysData->eFailedPowerState = eNewPowerState;

	return eError;
}

enum PVRSRV_ERROR PVRSRVRegisterPowerDevice(u32 ui32DeviceIndex,
	enum PVRSRV_ERROR (*pfnPrePower)(void *, enum PVR_POWER_STATE,
					 enum PVR_POWER_STATE),
	enum PVRSRV_ERROR (*pfnPostPower)(void *, enum PVR_POWER_STATE,
					  enum PVR_POWER_STATE),
	enum PVRSRV_ERROR (*pfnPreClockSpeedChange)(void *, IMG_BOOL,
						    enum PVR_POWER_STATE),
	enum PVRSRV_ERROR (*pfnPostClockSpeedChange)(void *, IMG_BOOL,
						     enum PVR_POWER_STATE),
	void *hDevCookie, enum PVR_POWER_STATE eCurrentPowerState,
	enum PVR_POWER_STATE eDefaultPowerState)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;
	struct PVRSRV_POWER_DEV *psPowerDevice;

	if (pfnPrePower == NULL && pfnPostPower == NULL)
		return PVRSRVRemovePowerDevice(ui32DeviceIndex);

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return eError;

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			    sizeof(struct PVRSRV_POWER_DEV),
			    (void **) &psPowerDevice, NULL);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVRegisterPowerDevice: "
				"Failed to alloc struct PVRSRV_POWER_DEV");
		return eError;
	}

	psPowerDevice->pfnPrePower = pfnPrePower;
	psPowerDevice->pfnPostPower = pfnPostPower;
	psPowerDevice->pfnPreClockSpeedChange = pfnPreClockSpeedChange;
	psPowerDevice->pfnPostClockSpeedChange = pfnPostClockSpeedChange;
	psPowerDevice->hDevCookie = hDevCookie;
	psPowerDevice->ui32DeviceIndex = ui32DeviceIndex;
	psPowerDevice->eCurrentPowerState = eCurrentPowerState;
	psPowerDevice->eDefaultPowerState = eDefaultPowerState;

	psPowerDevice->psNext = psSysData->psPowerDeviceList;
	psSysData->psPowerDeviceList = psPowerDevice;

	return PVRSRV_OK;
}

enum PVRSRV_ERROR PVRSRVRemovePowerDevice(u32 ui32DeviceIndex)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;
	struct PVRSRV_POWER_DEV *psCurrent, *psPrevious;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return eError;

	psCurrent = psSysData->psPowerDeviceList;
	psPrevious = NULL;

	while (psCurrent)
		if (psCurrent->ui32DeviceIndex == ui32DeviceIndex) {
			if (psPrevious)
				psPrevious->psNext = psCurrent->psNext;
			else
				psSysData->psPowerDeviceList =
				    psCurrent->psNext;
			OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(struct PVRSRV_POWER_DEV), psCurrent,
				  NULL);
			break;
		} else {
			psPrevious = psCurrent;
			psCurrent = psCurrent->psNext;
		}
	return PVRSRV_OK;
}

IMG_BOOL PVRSRVIsDevicePowered(u32 ui32DeviceIndex)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;
	struct PVRSRV_POWER_DEV *psPowerDevice;

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return IMG_FALSE;

	psPowerDevice = psSysData->psPowerDeviceList;
	while (psPowerDevice) {
		if (psPowerDevice->ui32DeviceIndex == ui32DeviceIndex)
			return (IMG_BOOL)(psPowerDevice->eCurrentPowerState ==
						PVRSRV_POWER_STATE_D0);
		psPowerDevice = psPowerDevice->psNext;
	}

	return IMG_FALSE;
}

enum PVRSRV_ERROR PVRSRVDevicePreClockSpeedChange(u32 ui32DeviceIndex,
					     IMG_BOOL bIdleDevice,
					     void *pvInfo)
{
	enum PVRSRV_ERROR eError = PVRSRV_OK;
	struct SYS_DATA *psSysData;
	struct PVRSRV_POWER_DEV *psPowerDevice;

	PVR_UNREFERENCED_PARAMETER(pvInfo);

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return eError;

	psPowerDevice = psSysData->psPowerDeviceList;
	while (psPowerDevice) {
		if (ui32DeviceIndex == psPowerDevice->ui32DeviceIndex)
			if (psPowerDevice->pfnPreClockSpeedChange) {
				eError =
				    psPowerDevice->
				    pfnPreClockSpeedChange(psPowerDevice->
							   hDevCookie,
							   bIdleDevice,
							   psPowerDevice->
							   eCurrentPowerState);
				if (eError != PVRSRV_OK) {
					pr_err
					    ("pfnPreClockSpeedChange failed\n");
					PVR_DPF(PVR_DBG_ERROR,
					    "PVRSRVDevicePreClockSpeedChange : "
					    "Device %lu failed, error:0x%lx",
						 ui32DeviceIndex, eError);
					break;
				}
			}
		psPowerDevice = psPowerDevice->psNext;
	}

	return eError;
}

void PVRSRVDevicePostClockSpeedChange(u32 ui32DeviceIndex, IMG_BOOL bIdleDevice,
				      void *pvInfo)
{
	enum PVRSRV_ERROR eError;
	struct SYS_DATA *psSysData;
	struct PVRSRV_POWER_DEV *psPowerDevice;

	PVR_UNREFERENCED_PARAMETER(pvInfo);

	eError = SysAcquireData(&psSysData);
	if (eError != PVRSRV_OK)
		return;

	psPowerDevice = psSysData->psPowerDeviceList;
	while (psPowerDevice) {
		if (ui32DeviceIndex == psPowerDevice->ui32DeviceIndex)
			if (psPowerDevice->pfnPostClockSpeedChange) {
				eError =
				    psPowerDevice->
				    pfnPostClockSpeedChange(psPowerDevice->
							    hDevCookie,
							    bIdleDevice,
							    psPowerDevice->
							    eCurrentPowerState);
				if (eError != PVRSRV_OK) {
					pr_err
					    ("pfnPostClockSpeedChange "
					     "failed\n");
					PVR_DPF(PVR_DBG_ERROR,
					  "PVRSRVDevicePostClockSpeedChange : "
					  "Device %lu failed, error:0x%lx",
						 ui32DeviceIndex, eError);
				}
			}
		psPowerDevice = psPowerDevice->psNext;
	}
}
