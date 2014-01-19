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

#include <stddef.h>

#include "sgxdefs.h"
#include "services_headers.h"
#include "sgxapi_km.h"
#include "sgx_mkif_km.h"
#include "sgxutils.h"
#include "pdump_km.h"


#if defined(SUPPORT_HW_RECOVERY)
static PVRSRV_ERROR SGXAddTimer(PVRSRV_DEVICE_NODE		*psDeviceNode,
								SGX_TIMING_INFORMATION	*psSGXTimingInfo,
								IMG_HANDLE				*phTimer)
{



	*phTimer = OSAddTimer(SGXOSTimer, psDeviceNode,
						  1000 * 50 / psSGXTimingInfo->ui32uKernelFreq);
	if(*phTimer == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXAddTimer : Failed to register timer callback function"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	return PVRSRV_OK;
}
#endif


static PVRSRV_ERROR SGXUpdateTimingInfo(PVRSRV_DEVICE_NODE	*psDeviceNode)
{
	PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
#if defined(SGX_DYNAMIC_TIMING_INFO)
	SGX_TIMING_INFORMATION	sSGXTimingInfo = {0};
#else
	SGX_DEVICE_MAP		*psSGXDeviceMap;
#endif
	IMG_UINT32		ui32ActivePowManSampleRate;
	SGX_TIMING_INFORMATION	*psSGXTimingInfo;


#if defined(SGX_DYNAMIC_TIMING_INFO)
	psSGXTimingInfo = &sSGXTimingInfo;
	SysGetSGXTimingInformation(psSGXTimingInfo);
#else
	SysGetDeviceMemoryMap(PVRSRV_DEVICE_TYPE_SGX,
						  (IMG_VOID**)&psSGXDeviceMap);
	psSGXTimingInfo = &psSGXDeviceMap->sTimingInfo;
#endif

#if defined(SUPPORT_HW_RECOVERY)
	{
		PVRSRV_ERROR			eError;
		IMG_UINT32	ui32OlduKernelFreq;

		if (psDevInfo->hTimer != IMG_NULL)
		{
			ui32OlduKernelFreq = psDevInfo->ui32CoreClockSpeed / psDevInfo->ui32uKernelTimerClock;
			if (ui32OlduKernelFreq != psSGXTimingInfo->ui32uKernelFreq)
			{


				IMG_HANDLE hNewTimer;

				eError = SGXAddTimer(psDeviceNode, psSGXTimingInfo, &hNewTimer);
				if (eError == PVRSRV_OK)
				{
					eError = OSRemoveTimer(psDevInfo->hTimer);
					if (eError != PVRSRV_OK)
					{
						PVR_DPF((PVR_DBG_ERROR,"SGXUpdateTimingInfo: Failed to remove timer"));
					}
					psDevInfo->hTimer = hNewTimer;
				}
				else
				{

				}
			}
		}
		else
		{
			eError = SGXAddTimer(psDeviceNode, psSGXTimingInfo, &psDevInfo->hTimer);
			if (eError != PVRSRV_OK)
			{
				return eError;
			}
		}

		psDevInfo->psSGXHostCtl->ui32HWRecoverySampleRate =
			psSGXTimingInfo->ui32uKernelFreq / psSGXTimingInfo->ui32HWRecoveryFreq;
	}
#endif


	psDevInfo->ui32CoreClockSpeed = psSGXTimingInfo->ui32CoreClockSpeed;
	psDevInfo->ui32uKernelTimerClock = psSGXTimingInfo->ui32CoreClockSpeed / psSGXTimingInfo->ui32uKernelFreq;


	psDevInfo->psSGXHostCtl->ui32uKernelTimerClock = psDevInfo->ui32uKernelTimerClock;
#if defined(PDUMP)
	PDUMPCOMMENT("Host Control - Microkernel clock");
	PDUMPMEM(IMG_NULL, psDevInfo->psKernelSGXHostCtlMemInfo,
			 offsetof(SGXMKIF_HOST_CTL, ui32uKernelTimerClock),
			 sizeof(IMG_UINT32), PDUMP_FLAGS_CONTINUOUS,
			 MAKEUNIQUETAG(psDevInfo->psKernelSGXHostCtlMemInfo));
#endif

	if (psSGXTimingInfo->bEnableActivePM)
	{
		ui32ActivePowManSampleRate =
			psSGXTimingInfo->ui32uKernelFreq * psSGXTimingInfo->ui32ActivePowManLatencyms / 1000;








		ui32ActivePowManSampleRate += 1;
	}
	else
	{
		ui32ActivePowManSampleRate = 0;
	}

	psDevInfo->psSGXHostCtl->ui32ActivePowManSampleRate = ui32ActivePowManSampleRate;
#if defined(PDUMP)
	PDUMPMEM(IMG_NULL, psDevInfo->psKernelSGXHostCtlMemInfo,
			 offsetof(SGXMKIF_HOST_CTL, ui32ActivePowManSampleRate),
			 sizeof(IMG_UINT32), PDUMP_FLAGS_CONTINUOUS,
			 MAKEUNIQUETAG(psDevInfo->psKernelSGXHostCtlMemInfo));
#endif

	return PVRSRV_OK;
}


static IMG_VOID SGXStartTimer(PVRSRV_SGXDEV_INFO	*psDevInfo)
{
	#if defined(SUPPORT_HW_RECOVERY)
	PVRSRV_ERROR	eError;

	eError = OSEnableTimer(psDevInfo->hTimer);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXStartTimer : Failed to enable host timer"));
	}
	#else
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	#endif
}


static IMG_VOID SGXPollForClockGating (PVRSRV_SGXDEV_INFO	*psDevInfo,
									   IMG_UINT32			ui32Register,
									   IMG_UINT32			ui32RegisterValue,
									   IMG_CHAR				*pszComment)
{
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(ui32Register);
	PVR_UNREFERENCED_PARAMETER(ui32RegisterValue);
	PVR_UNREFERENCED_PARAMETER(pszComment);

	#if !defined(NO_HARDWARE)
	PVR_ASSERT(psDevInfo != IMG_NULL);


	if (PollForValueKM((IMG_UINT32 *)psDevInfo->pvRegsBaseKM + (ui32Register >> 2),
						0,
						ui32RegisterValue,
						MAX_HW_TIME_US/WAIT_TRY_COUNT,
						WAIT_TRY_COUNT) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXPrePowerState: %s failed.", pszComment));
	}
	#endif

	PDUMPCOMMENT(pszComment);
	PDUMPREGPOL(ui32Register, 0, ui32RegisterValue);
}


PVRSRV_ERROR SGXPrePowerState (IMG_HANDLE				hDevHandle,
							   PVRSRV_DEV_POWER_STATE	eNewPowerState,
							   PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if ((eNewPowerState != eCurrentPowerState) &&
		(eNewPowerState != PVRSRV_DEV_POWER_STATE_ON))
	{
		PVRSRV_ERROR		eError;
		PVRSRV_DEVICE_NODE	*psDeviceNode = hDevHandle;
		PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
		IMG_UINT32			ui32PowerCmd, ui32CompleteStatus;
		SGXMKIF_COMMAND		sCommand = {0};
		IMG_UINT32			ui32Core;

		#if defined(SUPPORT_HW_RECOVERY)

		eError = OSDisableTimer(psDevInfo->hTimer);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXPrePowerState: Failed to disable timer"));
			return eError;
		}
		#endif

		if (eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF)
		{

			ui32PowerCmd = PVRSRV_POWERCMD_POWEROFF;
			ui32CompleteStatus = PVRSRV_USSE_EDM_POWMAN_POWEROFF_COMPLETE;
			PDUMPCOMMENT("SGX power off request");
		}
		else
		{

			ui32PowerCmd = PVRSRV_POWERCMD_IDLE;
			ui32CompleteStatus = PVRSRV_USSE_EDM_POWMAN_IDLE_COMPLETE;
			PDUMPCOMMENT("SGX idle request");
		}

		sCommand.ui32Data[1] = ui32PowerCmd;

		eError = SGXScheduleCCBCommand(psDevInfo, SGXMKIF_CMD_POWER, &sCommand, KERNEL_ID, 0);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXPrePowerState: Failed to submit power down command"));
			return eError;
		}


		#if !defined(NO_HARDWARE)
		if (PollForValueKM(&psDevInfo->psSGXHostCtl->ui32PowerStatus,
							ui32CompleteStatus,
							ui32CompleteStatus,
							MAX_HW_TIME_US/WAIT_TRY_COUNT,
							WAIT_TRY_COUNT) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXPrePowerState: Wait for SGX ukernel power transition failed."));
			PVR_DBG_BREAK;
		}
		#endif

		#if defined(PDUMP)
		PDUMPCOMMENT("TA/3D CCB Control - Wait for power event on uKernel.");
		PDUMPMEMPOL(psDevInfo->psKernelSGXHostCtlMemInfo,
					offsetof(SGXMKIF_HOST_CTL, ui32PowerStatus),
					ui32CompleteStatus,
					ui32CompleteStatus,
					PDUMP_POLL_OPERATOR_EQUAL,
					0,
					MAKEUNIQUETAG(psDevInfo->psKernelSGXHostCtlMemInfo));
		#endif

		for (ui32Core = 0; ui32Core < SGX_FEATURE_MP_CORE_COUNT; ui32Core++)
		{

			SGXPollForClockGating(psDevInfo,
								  SGX_MP_CORE_SELECT(psDevInfo->ui32ClkGateStatusReg, ui32Core),
								  psDevInfo->ui32ClkGateStatusMask,
								  "Wait for SGX clock gating");
		}

		#if defined(SGX_FEATURE_MP)

		SGXPollForClockGating(psDevInfo,
							  psDevInfo->ui32MasterClkGateStatusReg,
							  psDevInfo->ui32MasterClkGateStatusMask,
							  "Wait for SGX master clock gating");
		#endif

		if (eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF)
		{

			eError = SGXDeinitialise(psDevInfo);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SGXPrePowerState: SGXDeinitialise failed: %lu", eError));
				return eError;
			}
		}
	}

	return PVRSRV_OK;
}


PVRSRV_ERROR SGXPostPowerState (IMG_HANDLE				hDevHandle,
								PVRSRV_DEV_POWER_STATE	eNewPowerState,
								PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	if ((eNewPowerState != eCurrentPowerState) &&
		(eCurrentPowerState != PVRSRV_DEV_POWER_STATE_ON))
	{
		PVRSRV_ERROR		eError;
		PVRSRV_DEVICE_NODE	*psDeviceNode = hDevHandle;
		PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
		SGXMKIF_HOST_CTL *psSGXHostCtl = psDevInfo->psSGXHostCtl;


		psSGXHostCtl->ui32PowerStatus = 0;
		#if defined(PDUMP)
		PDUMPCOMMENT("TA/3D CCB Control - Reset power status");
		PDUMPMEM(IMG_NULL, psDevInfo->psKernelSGXHostCtlMemInfo,
				 offsetof(SGXMKIF_HOST_CTL, ui32PowerStatus),
				 sizeof(IMG_UINT32), PDUMP_FLAGS_CONTINUOUS,
				 MAKEUNIQUETAG(psDevInfo->psKernelSGXHostCtlMemInfo));
		#endif

		if (eCurrentPowerState == PVRSRV_DEV_POWER_STATE_OFF)
		{





			eError = SGXUpdateTimingInfo(psDeviceNode);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SGXPostPowerState: SGXUpdateTimingInfo failed"));
				return eError;
			}



			eError = SGXInitialise(psDevInfo);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SGXPostPowerState: SGXInitialise failed"));
				return eError;
			}
		}
		else
		{


			SGXMKIF_COMMAND		sCommand = {0};

			sCommand.ui32Data[1] = PVRSRV_POWERCMD_RESUME;
			eError = SGXScheduleCCBCommand(psDevInfo, SGXMKIF_CMD_POWER, &sCommand, ISR_ID, 0);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"SGXPostPowerState failed to schedule CCB command: %lu", eError));
				return PVRSRV_ERROR_GENERIC;
			}
		}

		SGXStartTimer(psDevInfo);
	}

	return PVRSRV_OK;
}


PVRSRV_ERROR SGXPreClockSpeedChange (IMG_HANDLE				hDevHandle,
									 IMG_BOOL				bIdleDevice,
									 PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	PVRSRV_ERROR		eError;
	PVRSRV_DEVICE_NODE	*psDeviceNode = hDevHandle;
	PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;

	PVR_UNREFERENCED_PARAMETER(psDevInfo);

	if (eCurrentPowerState == PVRSRV_DEV_POWER_STATE_ON)
	{
		if (bIdleDevice)
		{

			PDUMPSUSPEND();

			eError = SGXPrePowerState(hDevHandle, PVRSRV_DEV_POWER_STATE_IDLE,
									  PVRSRV_DEV_POWER_STATE_ON);

			if (eError != PVRSRV_OK)
			{
				PDUMPRESUME();
				return eError;
			}
		}
	}

	PVR_DPF((PVR_DBG_MESSAGE,"SGXPreClockSpeedChange: SGX clock speed was %luHz",
			psDevInfo->ui32CoreClockSpeed));

	return PVRSRV_OK;
}


PVRSRV_ERROR SGXPostClockSpeedChange (IMG_HANDLE				hDevHandle,
									  IMG_BOOL					bIdleDevice,
									  PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	PVRSRV_DEVICE_NODE	*psDeviceNode = hDevHandle;
	PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	IMG_UINT32			ui32OldClockSpeed = psDevInfo->ui32CoreClockSpeed;

	PVR_UNREFERENCED_PARAMETER(ui32OldClockSpeed);

	if (eCurrentPowerState == PVRSRV_DEV_POWER_STATE_ON)
	{
		PVRSRV_ERROR eError;



		eError = SGXUpdateTimingInfo(psDeviceNode);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXPostPowerState: SGXUpdateTimingInfo failed"));
			return eError;
		}

		if (bIdleDevice)
		{

			eError = SGXPostPowerState(hDevHandle, PVRSRV_DEV_POWER_STATE_ON,
									   PVRSRV_DEV_POWER_STATE_IDLE);

			PDUMPRESUME();

			if (eError != PVRSRV_OK)
			{
				return eError;
			}
		}
		else
		{
			SGXStartTimer(psDevInfo);
		}

	}

	PVR_DPF((PVR_DBG_MESSAGE,"SGXPostClockSpeedChange: SGX clock speed changed from %luHz to %luHz",
			ui32OldClockSpeed, psDevInfo->ui32CoreClockSpeed));

	return PVRSRV_OK;
}


