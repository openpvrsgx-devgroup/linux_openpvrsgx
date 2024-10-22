/*
 * Copyright (c) 2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Authors: binglin.chen@intel.com>
 *
 */

#include "msvdx_power.h"
#include "psb_msvdx.h"
#include "psb_drv.h"

#include "services_headers.h"
#include "sysconfig.h"

static PVRSRV_ERROR DevInitMSVDXPart1(IMG_VOID *pvDeviceNode)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE *)pvDeviceNode;
	PVRSRV_ERROR eError;
	PVRSRV_DEV_POWER_STATE eDefaultPowerState;

	/* register power operation function */
	/* FIXME: this should be in part2 init function, but
	 * currently here only OSPM needs IMG device... */
	eDefaultPowerState = PVRSRV_DEV_POWER_STATE_OFF;
	eError = PVRSRVRegisterPowerDevice(psDeviceNode->sDevId.ui32DeviceIndex,
					   MSVDXPrePowerState,
					   MSVDXPostPowerState,
					   MSVDXPreClockSpeedChange,
					   MSVDXPostClockSpeedChange,
					   (IMG_HANDLE)psDeviceNode,
					   PVRSRV_DEV_POWER_STATE_ON,
					   eDefaultPowerState);
	if (eError != PVRSRV_OK) {
		PVR_DPF((PVR_DBG_ERROR, "DevInitMSVDXPart1: failed to "
			 "register device with power manager"));
		return eError;
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR DevDeInitMSVDX(IMG_VOID *pvDeviceNode)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE *)pvDeviceNode;
	PVRSRV_ERROR eError;

	/* should deinit all resource */

	eError = PVRSRVRemovePowerDevice(psDeviceNode->sDevId.ui32DeviceIndex);
	if (eError != PVRSRV_OK)
		return eError;

	return PVRSRV_OK;
}

PVRSRV_ERROR MSVDXDevInitCompatCheck(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	/* version check */

	return PVRSRV_OK;
}

PVRSRV_ERROR MSVDXRegisterDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	psDeviceNode->sDevId.eDeviceType	= PVRSRV_DEVICE_TYPE_MSVDX;
	psDeviceNode->sDevId.eDeviceClass	= PVRSRV_DEVICE_CLASS_VIDEO;

	psDeviceNode->pfnInitDevice		= DevInitMSVDXPart1;
	psDeviceNode->pfnDeInitDevice		= DevDeInitMSVDX;

	psDeviceNode->pfnInitDeviceCompatCheck	= MSVDXDevInitCompatCheck;

	psDeviceNode->pfnDeviceISR = psb_msvdx_interrupt;
	psDeviceNode->pvISRData = (IMG_VOID *)gpDrmDevice;

	return PVRSRV_OK;
}

PVRSRV_ERROR MSVDXPrePowerState(IMG_HANDLE hDevHandle,
				 PVRSRV_DEV_POWER_STATE	eNewPowerState,
				 PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
#if 1
	/* ask for a change not power on*/
	if ((eNewPowerState != eCurrentPowerState) &&
	    (eNewPowerState != PVRSRV_DEV_POWER_STATE_ON)) {
		struct drm_psb_private *dev_priv = gpDrmDevice->dev_private;
		struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;
		MSVDX_NEW_PMSTATE(gpDrmDevice, msvdx_priv, PSB_PMSTATE_POWERDOWN);

		/* context save */
		psb_msvdx_save_context(gpDrmDevice);

		/* internally close the device */

		/* ask for power off */
		if (eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF) {
			/* here will deinitialize the driver if needed */
		} else {
			PVR_DPF((PVR_DBG_MESSAGE,
				"%s no action for transform from %d to %d",
				 __func__,
				eCurrentPowerState,
				eNewPowerState));
		}
	}
#endif
	return PVRSRV_OK;
}

PVRSRV_ERROR MSVDXPostPowerState(IMG_HANDLE hDevHandle,
				 PVRSRV_DEV_POWER_STATE	eNewPowerState,
				 PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
#if 1
	/* if ask for change & current status is not on */
	if ((eNewPowerState != eCurrentPowerState) &&
	    (eCurrentPowerState != PVRSRV_DEV_POWER_STATE_ON)) {
		/* internally open device */
		struct drm_psb_private *dev_priv = gpDrmDevice->dev_private;
		struct msvdx_private *msvdx_priv = dev_priv->msvdx_private;
		MSVDX_NEW_PMSTATE(gpDrmDevice, msvdx_priv, PSB_PMSTATE_POWERUP);

		/* context restore */
		psb_msvdx_restore_context(gpDrmDevice);

		if (eCurrentPowerState == PVRSRV_DEV_POWER_STATE_OFF) {
			/* here will initialize the driver if needed */
		} else {
			PVR_DPF((PVR_DBG_MESSAGE,
				"%s no action for transform from %d to %d",
				 __func__,
				eCurrentPowerState,
				eNewPowerState));
		}
	}
#endif
	return PVRSRV_OK;
}

PVRSRV_ERROR MSVDXPreClockSpeedChange(IMG_HANDLE hDevHandle,
				      IMG_BOOL bIdleDevice,
				      PVRSRV_DEV_POWER_STATE eCurrentPowerState)
{
	return PVRSRV_OK;
}

PVRSRV_ERROR MSVDXPostClockSpeedChange(IMG_HANDLE hDevHandle,
				      IMG_BOOL bIdleDevice,
				      PVRSRV_DEV_POWER_STATE eCurrentPowerState)
{
	return PVRSRV_OK;
}
