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

#ifndef _SYSCOMMON_H
#define _SYSCOMMON_H

#include "sysconfig.h"
#include "sysinfo.h"
#include "servicesint.h"
#include "queue.h"
#include "power.h"
#include "resman.h"
#include "ra.h"
#include "device.h"
#include "buffer_manager.h"

#include <linux/platform_device.h>
#include <linux/pvr.h>

#if defined(NO_HARDWARE) && defined(__KERNEL__)
#include <linux/io.h>
#endif

struct SYS_DEVICE_ID {
	u32 uiID;
	IMG_BOOL bInUse;

};

#define SYS_MAX_LOCAL_DEVMEM_ARENAS	4

struct SYS_DATA {
	u32 ui32NumDevices;
	struct SYS_DEVICE_ID sDeviceID[SYS_DEVICE_COUNT];
	struct PVRSRV_DEVICE_NODE *psDeviceNodeList;
	struct PVRSRV_POWER_DEV *psPowerDeviceList;
	struct PVRSRV_RESOURCE sPowerStateChangeResource;
	enum PVR_POWER_STATE eCurrentPowerState;
	enum PVR_POWER_STATE eFailedPowerState;
	u32 ui32CurrentOSPowerState;
	struct PVRSRV_QUEUE_INFO *psQueueList;
	struct PVRSRV_KERNEL_SYNC_INFO *psSharedSyncInfoList;
	void *pvEnvSpecificData;
	void *pvSysSpecificData;
	struct PVRSRV_RESOURCE sQProcessResource;
	void *pvSOCRegsBase;
	void *hSOCTimerRegisterOSMemHandle;
	u32 *pvSOCTimerRegisterKM;
	void *pvSOCClockGateRegsBase;
	u32 ui32SOCClockGateRegsSize;
	IMG_BOOL (**ppfnCmdProcList[SYS_DEVICE_COUNT])(void *, u32, void *);

	struct COMMAND_COMPLETE_DATA **ppsCmdCompleteData[SYS_DEVICE_COUNT];

	struct RA_ARENA *apsLocalDevMemArena[SYS_MAX_LOCAL_DEVMEM_ARENAS];

	char *pszVersionString;
	struct PVRSRV_EVENTOBJECT *psGlobalEventObject;
#if defined(PDUMP)
	IMG_BOOL bPowerUpPDumped;
#endif
};

enum PVRSRV_ERROR SysInitialise(struct platform_device *spd);
enum PVRSRV_ERROR SysFinalise(void);

enum PVRSRV_ERROR SysDeinitialise(struct SYS_DATA *psSysData);

enum PVRSRV_ERROR SysGetDeviceMemoryMap(enum PVRSRV_DEVICE_TYPE eDeviceType,
				   void **ppvDeviceMap);

void SysRegisterExternalDevice(struct PVRSRV_DEVICE_NODE *psDeviceNode);
void SysRemoveExternalDevice(struct PVRSRV_DEVICE_NODE *psDeviceNode);

u32 SysGetInterruptSource(struct SYS_DATA *psSysData,
				 struct PVRSRV_DEVICE_NODE *psDeviceNode);

void SysClearInterrupts(struct SYS_DATA *psSysData,
			    u32 ui32ClearBits);

enum PVRSRV_ERROR SysResetDevice(u32 ui32DeviceIndex);

enum PVRSRV_ERROR SysSystemPrePowerState(enum PVR_POWER_STATE eNewPowerState);
enum PVRSRV_ERROR SysSystemPostPowerState(enum PVR_POWER_STATE eNewPowerState);
enum PVRSRV_ERROR SysDevicePrePowerState(u32 ui32DeviceIndex,
				    enum PVR_POWER_STATE eNewPowerState,
				    enum PVR_POWER_STATE eCurrentPowerState);
enum PVRSRV_ERROR SysDevicePostPowerState(u32 ui32DeviceIndex,
				     enum PVR_POWER_STATE eNewPowerState,
				     enum PVR_POWER_STATE
				     eCurrentPowerState);

enum PVRSRV_ERROR SysOEMFunction(u32 ui32ID,
			    void *pvIn,
			    u32 ulInSize,
			    void *pvOut, u32 ulOutSize);

struct IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr(
			enum PVRSRV_DEVICE_TYPE eDeviceType,
			struct IMG_CPU_PHYADDR cpu_paddr);
struct IMG_DEV_PHYADDR SysSysPAddrToDevPAddr(
			enum PVRSRV_DEVICE_TYPE eDeviceType,
			struct IMG_SYS_PHYADDR SysPAddr);
struct IMG_SYS_PHYADDR SysDevPAddrToSysPAddr(
			enum PVRSRV_DEVICE_TYPE eDeviceType,
			struct IMG_DEV_PHYADDR SysPAddr);
struct IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr(struct IMG_SYS_PHYADDR SysPAddr);
struct IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr(struct IMG_CPU_PHYADDR cpu_paddr);

extern struct SYS_DATA *gpsSysData;


static inline enum PVRSRV_ERROR SysAcquireData(struct SYS_DATA **ppsSysData)
{
	*ppsSysData = gpsSysData;

	if (!gpsSysData)
		return PVRSRV_ERROR_GENERIC;

	return PVRSRV_OK;
}

static inline enum PVRSRV_ERROR SysInitialiseCommon(struct SYS_DATA *psSysData)
{
	enum PVRSRV_ERROR eError;
	eError = PVRSRVInit(psSysData);
	return eError;
}

static inline void SysDeinitialiseCommon(struct SYS_DATA *psSysData)
{
	PVRSRVDeInit(psSysData);
	OSDestroyResource(&psSysData->sPowerStateChangeResource);
}

#if !(defined(NO_HARDWARE) && defined(__KERNEL__))
#define	SysReadHWReg(p, o)	OSReadHWReg(p, o)
#define SysWriteHWReg(p, o, v)	OSWriteHWReg(p, o, v)
#else
static inline u32 SysReadHWReg(void *pvLinRegBaseAddr, u32 ui32Offset)
{
	return (u32)readl(pvLinRegBaseAddr + ui32Offset);
}

static inline void SysWriteHWReg(void *pvLinRegBaseAddr, u32 ui32Offset,
				 u32 ui32Value)
{
	writel(ui32Value, pvLinRegBaseAddr + ui32Offset);
}
#endif

bool sgx_is_530(void);
u32 sgx_get_rev(void);
void sgx_ocp_write_reg(u32 reg, u32 val);
unsigned long sgx_get_max_freq(void);

#endif
