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

#ifndef POWER_H
#define POWER_H

struct PVRSRV_POWER_DEV {
	enum PVRSRV_ERROR (*pfnPrePower)(void *, enum PVR_POWER_STATE,
					 enum PVR_POWER_STATE);
	enum PVRSRV_ERROR (*pfnPostPower)(void *, enum PVR_POWER_STATE,
					  enum PVR_POWER_STATE);
	enum PVRSRV_ERROR (*pfnPreClockSpeedChange)(void *, IMG_BOOL,
						    enum PVR_POWER_STATE);
	enum PVRSRV_ERROR (*pfnPostClockSpeedChange)(void *, IMG_BOOL,
						     enum PVR_POWER_STATE);
	void *hDevCookie;
	u32 ui32DeviceIndex;
	enum PVR_POWER_STATE eDefaultPowerState;
	enum PVR_POWER_STATE eCurrentPowerState;
	struct PVRSRV_POWER_DEV *psNext;

};

enum PVRSRV_INIT_SERVER_STATE {
	PVRSRV_INIT_SERVER_Unspecified = -1,
	PVRSRV_INIT_SERVER_RUNNING = 0,
	PVRSRV_INIT_SERVER_RAN = 1,
	PVRSRV_INIT_SERVER_SUCCESSFUL = 2,
	PVRSRV_INIT_SERVER_NUM = 3,
	PVRSRV_INIT_SERVER_FORCE_I32 = 0x7fffffff
};

IMG_BOOL PVRSRVGetInitServerState(enum PVRSRV_INIT_SERVER_STATE
		eInitServerState);

enum PVRSRV_ERROR PVRSRVSetInitServerState(enum PVRSRV_INIT_SERVER_STATE
		eInitServerState,
		IMG_BOOL bState);

enum PVRSRV_ERROR PVRSRVPowerLock(u32 ui32CallerID,
		IMG_BOOL bSystemPowerEvent);
void PVRSRVPowerUnlock(u32 ui32CallerID);
void PVRSRVDvfsLock(void);
void PVRSRVDvfsUnlock(void);

enum PVRSRV_ERROR PVRSRVSetDevicePowerStateKM(u32 ui32DeviceIndex,
		enum PVR_POWER_STATE eNewPowerState, u32 ui32CallerID,
		IMG_BOOL bRetainMutex);

enum PVRSRV_ERROR PVRSRVSystemPrePowerStateKM(
		enum PVR_POWER_STATE eNewPowerState);
enum PVRSRV_ERROR PVRSRVSystemPostPowerStateKM(
		enum PVR_POWER_STATE eNewPowerState);

enum PVRSRV_ERROR PVRSRVSetPowerStateKM(enum PVR_POWER_STATE ePVRState);

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
	enum PVR_POWER_STATE eDefaultPowerState);

enum PVRSRV_ERROR PVRSRVRemovePowerDevice(u32 ui32DeviceIndex);

IMG_BOOL PVRSRVIsDevicePowered(u32 ui32DeviceIndex);

enum PVRSRV_ERROR PVRSRVDevicePreClockSpeedChange(u32 ui32DeviceIndex,
		IMG_BOOL bIdleDevice,
		void *pvInfo);

void PVRSRVDevicePostClockSpeedChange(u32 ui32DeviceIndex,
		IMG_BOOL bIdleDevice,
		void *pvInfo);

#endif
