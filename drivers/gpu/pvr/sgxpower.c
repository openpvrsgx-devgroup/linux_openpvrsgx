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
#include <linux/io.h>
#include <linux/sched.h>

#include "sgxdefs.h"
#include "services_headers.h"
#include "sgxapi_km.h"
#include "sgxinfokm.h"
#include "sgxutils.h"
#include "pdump_km.h"

#define MS_TO_NS(x)			((x) * 1000000ULL)
#define SGX_CMD_BURST_THRESHOLD_NS	MS_TO_NS(3)
#define SGX_CMD_BURST_MAX_SIZE		4
#define SGX_POWER_DOWN_DELAY_LONG_MS	4
#define SGX_POWER_DOWN_DELAY_SHORT_MS	0

enum PVR_DEVICE_POWER_STATE {

	PVR_DEVICE_POWER_STATE_ON = 0,
	PVR_DEVICE_POWER_STATE_IDLE = 1,
	PVR_DEVICE_POWER_STATE_OFF = 2,

	PVR_DEVICE_POWER_STATE_FORCE_I32 = 0x7fffffff
};

static enum PVR_DEVICE_POWER_STATE MapDevicePowerState(enum PVR_POWER_STATE
						       ePowerState)
{
	enum PVR_DEVICE_POWER_STATE eDevicePowerState;

	switch (ePowerState) {
	case PVRSRV_POWER_STATE_D0:
		{
			eDevicePowerState = PVR_DEVICE_POWER_STATE_ON;
			break;
		}
	case PVRSRV_POWER_STATE_D3:
		{
			eDevicePowerState = PVR_DEVICE_POWER_STATE_OFF;
			break;
		}
	default:
		{
			PVR_DPF(PVR_DBG_ERROR,
				 "MapDevicePowerState: Invalid state: %ld",
				 ePowerState);
			eDevicePowerState = PVR_DEVICE_POWER_STATE_FORCE_I32;
			PVR_DBG_BREAK;
		}
	}

	return eDevicePowerState;
}

static void sgx_set_pwrdown_delay(struct PVRSRV_DEVICE_NODE *node,
				  unsigned ukernel_freq, int msec)
{
	struct PVRSRV_SGXDEV_INFO *dev_info = node->pvDevice;
	unsigned delay;

	delay = ukernel_freq * msec / 1000 + 1;
	if (dev_info->power_down_delay != delay) {
		writel(delay,
			&dev_info->psSGXHostCtl->ui32ActivePowManSampleRate);
		dev_info->power_down_delay = delay;
	}
}

static int sgx_calc_power_down_delay(struct PVRSRV_DEVICE_NODE *node)
{
	struct PVRSRV_SGXDEV_INFO *info = node->pvDevice;

	/*
	 * Set the power down delay to short if the command to be executed is
	 * the last one in the burst, except if the burst size is at the
	 * maximum.
	 */
	if (info->burst_size < SGX_CMD_BURST_MAX_SIZE &&
	    info->burst_cnt == info->burst_size)
		return SGX_POWER_DOWN_DELAY_SHORT_MS;
	else
		return SGX_POWER_DOWN_DELAY_LONG_MS;

}

void sgx_mark_new_command(struct PVRSRV_DEVICE_NODE *node)
{
	struct PVRSRV_SGXDEV_INFO *info = node->pvDevice;
	struct SGX_TIMING_INFORMATION tinfo = { 0 };
	unsigned long long cmd_start;
	bool new_burst = false;

	cmd_start = cpu_clock(smp_processor_id());

	if (unlikely(info->last_idle == info->burst_start)) {
		/*
		 * This is the initial case, when we haven't yet any commands
		 * issued.
		 */
		new_burst = true;
	} else {
		/*
		 * If the last idle occurred after the current burst started
		 * and the time since the idle is greater than the threshold
		 * allowed for delays within a burst then this is a new burst.
		 */
		if (time_after64(info->last_idle, info->burst_start) &&
		    cmd_start - info->last_idle > SGX_CMD_BURST_THRESHOLD_NS)
			new_burst = true;
	}

	if (new_burst) {
		info->burst_start = cmd_start;
		/*
		 * We predict the length of this new burst to be that of the
		 * previous burst.
		 */
		info->burst_size = info->burst_cnt;
		info->burst_cnt = 0;
	} else if (info->burst_cnt < SGX_CMD_BURST_MAX_SIZE) {
		info->burst_cnt++;
	}

	SysGetSGXTimingInformation(&tinfo);
	sgx_set_pwrdown_delay(node, tinfo.ui32uKernelFreq,
			      sgx_calc_power_down_delay(node));
}

void sgx_mark_power_down(struct PVRSRV_DEVICE_NODE *node)
{
	struct PVRSRV_SGXDEV_INFO *info = node->pvDevice;

	info->last_idle = cpu_clock(smp_processor_id());
	/*
	 * After the last command completes power down happens in a delayed
	 * manner. The current value of this delay is in power_down_delay.
	 * To get the command complete time - which is the actual idle start
	 * time - we have to deduct the amount of delay from the current time.
	 */
	info->last_idle -= MS_TO_NS(info->power_down_delay);
}

static void SGXGetTimingInfo(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	struct SGX_TIMING_INFORMATION sSGXTimingInfo = { 0 };
	struct timer_work_data *data = psDevInfo->hTimer;

	SysGetSGXTimingInformation(&sSGXTimingInfo);

	if (data) {
		BUG_ON(data->armed);
		/*
		 * The magic calculation below sets the hardware lock-up
		 * detection and recovery timer interval to ~150msecs.
		 * The interval length will be scaled based on the SGX
		 * functional clock frequency. The higher the frequency
		 * the shorter the interval and vice versa.
		 */
		data->interval = 150 * SYS_SGX_PDS_TIMER_FREQ /
			sSGXTimingInfo.ui32uKernelFreq;
	}

	writel(sSGXTimingInfo.ui32uKernelFreq /
		sSGXTimingInfo.ui32HWRecoveryFreq,
		&psDevInfo->psSGXHostCtl->ui32HWRecoverySampleRate);

	psDevInfo->ui32CoreClockSpeed = sSGXTimingInfo.ui32CoreClockSpeed;
	psDevInfo->ui32uKernelTimerClock =
		sSGXTimingInfo.ui32CoreClockSpeed /
		sSGXTimingInfo.ui32uKernelFreq;

	sgx_set_pwrdown_delay(psDeviceNode, sSGXTimingInfo.ui32uKernelFreq,
			      sgx_calc_power_down_delay(psDeviceNode));
}

void SGXStartTimer(struct PVRSRV_SGXDEV_INFO *psDevInfo, IMG_BOOL bStartOSTimer)
{
	u32 ui32RegVal;

	ui32RegVal =
	    EUR_CR_EVENT_TIMER_ENABLE_MASK | psDevInfo->ui32uKernelTimerClock;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_TIMER, ui32RegVal);
	PDUMPREGWITHFLAGS(EUR_CR_EVENT_TIMER, ui32RegVal,
			  PDUMP_FLAGS_CONTINUOUS);

	if (bStartOSTimer) {
		enum PVRSRV_ERROR eError;

		eError = SGXOSTimerEnable(psDevInfo->hTimer);
		if (eError != PVRSRV_OK)
			PVR_DPF(PVR_DBG_ERROR,
				 "SGXStartTimer : Failed to enable host timer");
	}
}

static void SGXPollForClockGating(struct PVRSRV_SGXDEV_INFO *psDevInfo,
				  u32 ui32Register,
				  u32 ui32RegisterValue, char *pszComment)
{
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(ui32Register);
	PVR_UNREFERENCED_PARAMETER(ui32RegisterValue);
	PVR_UNREFERENCED_PARAMETER(pszComment);

#if !defined(NO_HARDWARE)
	if (psDevInfo != NULL)
		if (PollForValueKM
		    ((u32 __iomem *)psDevInfo->pvRegsBaseKM +
					(ui32Register >> 2), 0,
		     ui32RegisterValue, MAX_HW_TIME_US / WAIT_TRY_COUNT,
		     WAIT_TRY_COUNT) != PVRSRV_OK)
			PVR_DPF(PVR_DBG_ERROR, "SGXPrePowerState: %s failed.",
				 pszComment);

#endif

	PDUMPCOMMENT(pszComment);
	PDUMPREGPOL(ui32Register, 0, ui32RegisterValue);
}

static enum PVRSRV_ERROR SGXPrePowerState(void *hDevHandle,
			  enum PVR_DEVICE_POWER_STATE eNewPowerState,
			  enum PVR_DEVICE_POWER_STATE eCurrentPowerState)
{
	if ((eNewPowerState != eCurrentPowerState) &&
	    (eNewPowerState != PVR_DEVICE_POWER_STATE_ON)) {
		enum PVRSRV_ERROR eError;
		struct PVRSRV_DEVICE_NODE *psDeviceNode = hDevHandle;
		struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
		u32 ui32PowerCmd, ui32CompleteStatus;
		struct SGXMKIF_COMMAND sCommand = { 0 };

		eError = SGXOSTimerCancel(psDevInfo->hTimer);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
				 "SGXPrePowerState: Failed to disable timer");
			return eError;
		}

		if (eNewPowerState == PVR_DEVICE_POWER_STATE_OFF) {

			ui32PowerCmd = PVRSRV_POWERCMD_POWEROFF;
			ui32CompleteStatus =
			    PVRSRV_USSE_EDM_POWMAN_POWEROFF_COMPLETE;
			PDUMPCOMMENT("SGX power off request");
		} else {

			ui32PowerCmd = PVRSRV_POWERCMD_IDLE;
			ui32CompleteStatus =
			    PVRSRV_USSE_EDM_POWMAN_IDLE_COMPLETE;
			PDUMPCOMMENT("SGX idle request");
		}

		sCommand.ui32Data[0] = PVRSRV_CCBFLAGS_POWERCMD;
		sCommand.ui32Data[1] = ui32PowerCmd;

		eError =
		    SGXScheduleCCBCommand(psDevInfo, SGXMKIF_COMMAND_EDM_KICK,
					  &sCommand, KERNEL_ID, 0);
		if (eError != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "SGXPrePowerState: "
					"Failed to submit power down command");
			return eError;
		}

#if !defined(NO_HARDWARE)
		if (PollForValueKM(&psDevInfo->psSGXHostCtl->ui32PowerStatus,
				   ui32CompleteStatus,
				   ui32CompleteStatus,
				   MAX_HW_TIME_US / WAIT_TRY_COUNT,
				   WAIT_TRY_COUNT) != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "SGXPrePowerState: "
			      "Wait for SGX ukernel power transition failed.");
			PVR_DBG_BREAK;
		}
#endif

#if defined(PDUMP)
		PDUMPCOMMENT
		    ("TA/3D CCB Control - Wait for power event on uKernel.");
		PDUMPMEMPOL(psDevInfo->psKernelSGXHostCtlMemInfo,
			    offsetof(struct SGXMKIF_HOST_CTL, ui32PowerStatus),
			    ui32CompleteStatus, ui32CompleteStatus,
			    PDUMP_POLL_OPERATOR_EQUAL, IMG_FALSE, IMG_FALSE,
			    MAKEUNIQUETAG(psDevInfo->
					  psKernelSGXHostCtlMemInfo));
#endif

		SGXPollForClockGating(psDevInfo,
				      psDevInfo->ui32ClkGateStatusReg,
				      psDevInfo->ui32ClkGateStatusMask,
				      "Wait for SGX clock gating");

		if (eNewPowerState == PVR_DEVICE_POWER_STATE_OFF) {
			eError = SGXDeinitialise(psDevInfo);
			if (eError != PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR, "SGXPrePowerState: "
						"SGXDeinitialise failed: %lu",
					 eError);
				return eError;
			}
		}
	}

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR SGXPostPowerState(void *hDevHandle,
			   enum PVR_DEVICE_POWER_STATE eNewPowerState,
			   enum PVR_DEVICE_POWER_STATE eCurrentPowerState)
{
	if ((eNewPowerState != eCurrentPowerState) &&
	    (eCurrentPowerState != PVR_DEVICE_POWER_STATE_ON)) {
		enum PVRSRV_ERROR eError;
		struct PVRSRV_DEVICE_NODE *psDeviceNode = hDevHandle;
		struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
		struct SGXMKIF_HOST_CTL __iomem *psSGXHostCtl =
						psDevInfo->psSGXHostCtl;

		writel(0, &psSGXHostCtl->ui32PowerStatus);
		PDUMPCOMMENT("TA/3D CCB Control - Reset power status");
#if defined(PDUMP)
		PDUMPMEM(NULL, psDevInfo->psKernelSGXHostCtlMemInfo,
			 offsetof(struct SGXMKIF_HOST_CTL, ui32PowerStatus),
			 sizeof(u32), PDUMP_FLAGS_CONTINUOUS,
			 MAKEUNIQUETAG(psDevInfo->psKernelSGXHostCtlMemInfo));
#endif

		if (eCurrentPowerState == PVR_DEVICE_POWER_STATE_OFF) {

			SGXGetTimingInfo(psDeviceNode);

			eError = SGXInitialise(psDevInfo, IMG_FALSE);
			if (eError != PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR,
				"SGXPostPowerState: SGXInitialise failed");
				return eError;
			}
		} else {

			struct SGXMKIF_COMMAND sCommand = { 0 };

			SGXStartTimer(psDevInfo, IMG_TRUE);

			sCommand.ui32Data[0] =
			    PVRSRV_CCBFLAGS_PROCESS_QUEUESCMD;
			eError =
			    SGXScheduleCCBCommand(psDevInfo,
						  SGXMKIF_COMMAND_EDM_KICK,
						  &sCommand, ISR_ID, 0);
			if (eError != PVRSRV_OK) {
				PVR_DPF(PVR_DBG_ERROR,
			"SGXPostPowerState failed to schedule CCB command: %lu",
					 eError);
				return PVRSRV_ERROR_GENERIC;
			}
		}
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SGXPrePowerStateExt(void *hDevHandle,
				      enum PVR_POWER_STATE eNewPowerState,
				      enum PVR_POWER_STATE eCurrentPowerState)
{
	enum PVR_DEVICE_POWER_STATE eNewDevicePowerState =
	    MapDevicePowerState(eNewPowerState);
	enum PVR_DEVICE_POWER_STATE eCurrentDevicePowerState =
	    MapDevicePowerState(eCurrentPowerState);

	return SGXPrePowerState(hDevHandle, eNewDevicePowerState,
				eCurrentDevicePowerState);
}

enum PVRSRV_ERROR SGXPostPowerStateExt(void *hDevHandle,
				       enum PVR_POWER_STATE eNewPowerState,
				       enum PVR_POWER_STATE eCurrentPowerState)
{
	enum PVRSRV_ERROR eError;
	enum PVR_DEVICE_POWER_STATE eNewDevicePowerState =
	    MapDevicePowerState(eNewPowerState);
	enum PVR_DEVICE_POWER_STATE eCurrentDevicePowerState =
	    MapDevicePowerState(eCurrentPowerState);

	eError =
	    SGXPostPowerState(hDevHandle, eNewDevicePowerState,
			      eCurrentDevicePowerState);
	if (eError != PVRSRV_OK)
		return eError;

	PVR_DPF(PVR_DBG_MESSAGE,
		 "SGXPostPowerState : SGX Power Transition from %d to %d OK",
		 eCurrentPowerState, eNewPowerState);

	return eError;
}

enum PVRSRV_ERROR SGXPreClockSpeedChange(void *hDevHandle, IMG_BOOL bIdleDevice,
				 enum PVR_POWER_STATE eCurrentPowerState)
{
	enum PVRSRV_ERROR eError;
	struct PVRSRV_DEVICE_NODE *psDeviceNode = hDevHandle;
	struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	PVR_UNREFERENCED_PARAMETER(psDevInfo);

	if (eCurrentPowerState == PVRSRV_POWER_STATE_D0)
		if (bIdleDevice) {
			PDUMPSUSPEND();
			eError =
			    SGXPrePowerState(hDevHandle,
					     PVR_DEVICE_POWER_STATE_IDLE,
					     PVR_DEVICE_POWER_STATE_ON);
			if (eError != PVRSRV_OK) {
				PDUMPRESUME();
				return eError;
			}
		}

	PVR_DPF(PVR_DBG_MESSAGE,
		 "SGXPreClockSpeedChange: SGX clock speed was %luHz",
		 psDevInfo->ui32CoreClockSpeed);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SGXPostClockSpeedChange(void *hDevHandle,
				  IMG_BOOL bIdleDevice,
				  enum PVR_POWER_STATE eCurrentPowerState)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode = hDevHandle;
	struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	u32 ui32OldClockSpeed = psDevInfo->ui32CoreClockSpeed;

	PVR_UNREFERENCED_PARAMETER(ui32OldClockSpeed);

	if (eCurrentPowerState == PVRSRV_POWER_STATE_D0) {
		SGXGetTimingInfo(psDeviceNode);
		if (bIdleDevice) {
			enum PVRSRV_ERROR eError;
			eError =
			    SGXPostPowerState(hDevHandle,
					      PVR_DEVICE_POWER_STATE_ON,
					      PVR_DEVICE_POWER_STATE_IDLE);
			PDUMPRESUME();
			if (eError != PVRSRV_OK)
				return eError;
		} else {
			SGXStartTimer(psDevInfo, IMG_TRUE);
		}

	}

	PVR_DPF(PVR_DBG_MESSAGE, "SGXPostClockSpeedChange: "
				"SGX clock speed changed from %luHz to %luHz",
		 ui32OldClockSpeed, psDevInfo->ui32CoreClockSpeed);

	return PVRSRV_OK;
}
