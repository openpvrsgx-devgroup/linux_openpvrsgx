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

#ifndef MSVDX_POWER_H_
#define MSVDX_POWER_H_

#include "services_headers.h"
#include "sysconfig.h"

extern struct drm_device *gpDrmDevice;

/* function define */
PVRSRV_ERROR MSVDXRegisterDevice(PVRSRV_DEVICE_NODE *psDeviceNode);
PVRSRV_ERROR MSVDXDevInitCompatCheck(PVRSRV_DEVICE_NODE *psDeviceNode);

/* power function define */
PVRSRV_ERROR MSVDXPrePowerState(IMG_HANDLE	hDevHandle,
			PVRSRV_DEV_POWER_STATE	eNewPowerState,
			PVRSRV_DEV_POWER_STATE	eCurrentPowerState);
PVRSRV_ERROR MSVDXPostPowerState(IMG_HANDLE	hDevHandle,
			 PVRSRV_DEV_POWER_STATE	eNewPowerState,
			 PVRSRV_DEV_POWER_STATE	eCurrentPowerState);
PVRSRV_ERROR MSVDXPreClockSpeedChange(IMG_HANDLE	hDevHandle,
			      IMG_BOOL			bIdleDevice,
			      PVRSRV_DEV_POWER_STATE	eCurrentPowerState);
PVRSRV_ERROR MSVDXPostClockSpeedChange(IMG_HANDLE	hDevHandle,
			       IMG_BOOL			bIdleDevice,
			       PVRSRV_DEV_POWER_STATE	eCurrentPowerState);
PVRSRV_ERROR MSVDXInitOSPM(PVRSRV_DEVICE_NODE *psDeviceNode);

#endif /* !MSVDX_POWER_H_ */
