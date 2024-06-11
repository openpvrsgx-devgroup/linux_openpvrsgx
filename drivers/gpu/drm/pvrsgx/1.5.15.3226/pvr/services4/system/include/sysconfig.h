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

#ifndef _SYSCONFIG_H
#define _SYSCONFIG_H

#include "sysinfo.h"
#include "servicesint.h"
#include "queue.h"
#include "power.h"
#include "resman.h"
#include "ra.h"
#include "device.h"
#include "buffer_manager.h"


#if defined(NO_HARDWARE) && defined(__linux__) && defined(__KERNEL__)
#include <asm/io.h>
#endif

#if defined (__cplusplus)
extern "C" {
#endif

typedef struct _SYS_DEVICE_ID_TAG
{
	IMG_UINT32	uiID;
	IMG_BOOL	bInUse;

} SYS_DEVICE_ID;


#define SYS_MAX_LOCAL_DEVMEM_ARENAS	4

typedef struct _SYS_DATA_TAG_
{
    IMG_UINT32				ui32NumDevices;
	SYS_DEVICE_ID			sDeviceID[SYS_DEVICE_COUNT];
    PVRSRV_DEVICE_NODE		*psDeviceNodeList;
    PVRSRV_POWER_DEV		*psPowerDeviceList;
	PVRSRV_RESOURCE			sPowerStateChangeResource;
	PVRSRV_SYS_POWER_STATE	eCurrentPowerState;
	PVRSRV_SYS_POWER_STATE	eFailedPowerState;
	IMG_UINT32		 		ui32CurrentOSPowerState;
    PVRSRV_QUEUE_INFO		*psQueueList;
	PVRSRV_KERNEL_SYNC_INFO *psSharedSyncInfoList;
    IMG_PVOID				pvEnvSpecificData;
    IMG_PVOID				pvSysSpecificData;
	PVRSRV_RESOURCE			sQProcessResource;
	IMG_VOID				*pvSOCRegsBase;
    IMG_HANDLE				hSOCTimerRegisterOSMemHandle;
	IMG_UINT32				*pvSOCTimerRegisterKM;
	IMG_VOID				*pvSOCClockGateRegsBase;
	IMG_UINT32				ui32SOCClockGateRegsSize;
	PFN_CMD_PROC			*ppfnCmdProcList[SYS_DEVICE_COUNT];


	PCOMMAND_COMPLETE_DATA	*ppsCmdCompleteData[SYS_DEVICE_COUNT];

	IMG_BOOL				bReProcessQueues;

	RA_ARENA				*apsLocalDevMemArena[SYS_MAX_LOCAL_DEVMEM_ARENAS];

    IMG_CHAR				*pszVersionString;
	PVRSRV_EVENTOBJECT		*psGlobalEventObject;

	IMG_BOOL				bFlushAll;

} SYS_DATA;



PVRSRV_ERROR SysInitialise(IMG_VOID);
PVRSRV_ERROR SysFinalise(IMG_VOID);

IMG_VOID SysReEnableInterrupts(IMG_VOID);

PVRSRV_ERROR SysDeinitialise(SYS_DATA *psSysData);
PVRSRV_ERROR SysGetDeviceMemoryMap(PVRSRV_DEVICE_TYPE eDeviceType,
									IMG_VOID **ppvDeviceMap);

IMG_VOID SysRegisterExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode);
IMG_VOID SysRemoveExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode);

IMG_UINT32 SysGetInterruptSource(SYS_DATA			*psSysData,
								 PVRSRV_DEVICE_NODE *psDeviceNode);

IMG_VOID SysClearInterrupts(SYS_DATA* psSysData, IMG_UINT32 ui32ClearBits);

PVRSRV_ERROR SysResetDevice(IMG_UINT32 ui32DeviceIndex);

PVRSRV_ERROR SysSystemPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState);
PVRSRV_ERROR SysSystemPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState);
PVRSRV_ERROR SysDevicePrePowerState(IMG_UINT32 ui32DeviceIndex,
									PVRSRV_DEV_POWER_STATE eNewPowerState,
									PVRSRV_DEV_POWER_STATE eCurrentPowerState);
PVRSRV_ERROR SysDevicePostPowerState(IMG_UINT32 ui32DeviceIndex,
									 PVRSRV_DEV_POWER_STATE eNewPowerState,
									 PVRSRV_DEV_POWER_STATE eCurrentPowerState);

#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
PVRSRV_ERROR SysPowerLockWrap(SYS_DATA *psSysData);
IMG_VOID SysPowerLockUnwrap(SYS_DATA *psSysData);
#endif
PVRSRV_ERROR SysOEMFunction(		IMG_UINT32 ui32ID,
						IMG_VOID *pvIn,
						IMG_UINT32 ulInSize,
						IMG_VOID *pvOut,
						IMG_UINT32 ulOutSize);
IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr(	PVRSRV_DEVICE_TYPE eDeviceType,
						IMG_CPU_PHYADDR cpu_paddr);
IMG_DEV_PHYADDR SysSysPAddrToDevPAddr(	PVRSRV_DEVICE_TYPE eDeviceType,
						IMG_SYS_PHYADDR SysPAddr);
IMG_SYS_PHYADDR SysDevPAddrToSysPAddr(	PVRSRV_DEVICE_TYPE eDeviceType,
						IMG_DEV_PHYADDR SysPAddr);
IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr(	IMG_SYS_PHYADDR SysPAddr);
IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr(	IMG_CPU_PHYADDR cpu_paddr);

/* --------------------------------------------------------------------------*/
/**
* @Synopsis  platform interface definition
*/
/* ----------------------------------------------------------------------------*/
typedef struct _SYS_PLATFORM_INTERFACE {
	char* 		sProductName;
	IMG_UINT32	uiSgxDevDeviceID;
	IMG_UINT32	uiSgxRegsOffset;
	IMG_UINT32	uiMsvdxRegsOffset;
	IMG_UINT32	uiSysSgxClockSpeed;
	IMG_UINT32	uiSysSgxActivePowerLatencyMs;
} SYS_PLATFORM_INTERFACE;

/* #define MAP_UNUSED_MAPPINGS */

#if defined(MAP_UNUSED_MAPPINGS)
#define SGX_FEATURE_HOST_PORT
#endif

extern SYS_PLATFORM_INTERFACE *gpsSysPlatformInterface;

#define VS_PRODUCT_NAME  	((gpsSysPlatformInterface) ? (gpsSysPlatformInterface->sProductName) : "Unknown Product")
#define SYS_SGX_DEV_DEVICE_ID  	((gpsSysPlatformInterface) ? (gpsSysPlatformInterface->uiSgxDevDeviceID) : 0x0)
#define SGX_REGS_OFFSET  	((gpsSysPlatformInterface) ? (gpsSysPlatformInterface->uiSgxRegsOffset) : 0x0)
#ifdef SUPPORT_MSVDX
#define MSVDX_REGS_OFFSET  	((gpsSysPlatformInterface) ? (gpsSysPlatformInterface->uiMsvdxRegsOffset) : 0x0)
#endif
#define SYS_SGX_CLOCK_SPEED 	((gpsSysPlatformInterface) ? (gpsSysPlatformInterface->uiSysSgxClockSpeed) : 0)
#define SYS_SGX_ACTIVE_POWER_LATENCY_MS ((gpsSysPlatformInterface) ? (gpsSysPlatformInterface->uiSysSgxActivePowerLatencyMs) : 0)

#define SYS_SGX_DEV_VENDOR_ID           0x8086

#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ         (100)
#define SYS_SGX_PDS_TIMER_FREQ                  (1000)


#define REGS_OFFSET	0x00000
#define REG_SIZE	0x2100

#define SGX_REG_SIZE 		0x4000

#ifdef SUPPORT_MSVDX
#define	MAX_OFFSET	(MSVDX_REGS_OFFSET + MSVDX_REG_SIZE)
#else
#define	MAX_OFFSET	(SGX_REGS_OFFSET + SGX_REG_SIZE)
#endif

#define MMADR_INDEX			4

#define DEVICE_SGX_INTERRUPT		(1UL<<0)
#define DEVICE_MSVDX_INTERRUPT		(1UL<<1)
#define DEVICE_DISP_INTERRUPT		(1UL<<2)

#define INTERRUPT_ENABLE_REG		0x20A0
#define INTERRUPT_IDENTITY_REG		0x20A4
#define INTERRUPT_MASK_REG			0x20A8
#define INTERRUPT_STATUS_REG		0x20AC

#define DISP_MASK					(1UL<<17)
#define THALIA_MASK					(1UL<<18)
#define MSVDX_MASK					(1UL<<19)
#define VSYNC_PIPEA_VBLANK_MASK		(1UL<<7)
#define VSYNC_PIPEA_EVENT_MASK		(1UL<<6)
#define VSYNC_PIPEB_VBLANK_MASK		(1UL<<5)
#define VSYNC_PIPEB_EVENT_MASK		(1UL<<4)

#define DISPLAY_REGS_OFFSET			0x70000
#define DISPLAY_REG_SIZE			0x2000

#define DISPLAY_A_CONFIG			0x00008UL
#define DISPLAY_A_STATUS_SELECT		0x00024UL
#define DISPLAY_B_CONFIG			0x01008UL
#define DISPLAY_B_STATUS_SELECT		0x01024UL

#define DISPLAY_PIPE_ENABLE			(1UL<<31)
#define DISPLAY_VSYNC_STS_EN		(1UL<<25)
#define DISPLAY_VSYNC_STS			(1UL<<9)

#if defined(SGX_FEATURE_HOST_PORT)
	#define SYS_SGX_HP_SIZE		0x8000000

	#define SYS_SGX_HOSTPORT_BASE_DEVVADDR 0xD0000000
	#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030)
		#define SYS_SGX_HOSTPORT_BRN23030_OFFSET 0x7C00000
	#endif
#endif


typedef struct
{
	union
	{
#if !defined(VISTA)
		IMG_UINT8	aui8PCISpace[256];
		IMG_UINT16	aui16PCISpace[128];
		IMG_UINT32	aui32PCISpace[64];
#endif
		struct
		{
			IMG_UINT16	ui16VenID;
			IMG_UINT16	ui16DevID;
			IMG_UINT16	ui16PCICmd;
			IMG_UINT16	ui16PCIStatus;
		}s;
	}u;
} PCICONFIG_SPACE, *PPCICONFIG_SPACE;


extern SYS_DATA* gpsSysData;

PVRSRV_ERROR SysResetDevice(IMG_UINT32 ui32DeviceIndex);

#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
PVRSRV_ERROR SysPowerLockWrap(SYS_DATA *psSysData);
IMG_VOID SysPowerLockUnwrap(SYS_DATA *psSysData);
#endif


#if defined(PVR_LMA)
IMG_BOOL SysVerifyCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_CPU_PHYADDR CpuPAddr);
IMG_BOOL SysVerifySysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr);
#endif

extern SYS_DATA* gpsSysData;

#if !defined(USE_CODE)

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysAcquireData)
#endif
static INLINE PVRSRV_ERROR SysAcquireData(SYS_DATA **ppsSysData)
{

	*ppsSysData = gpsSysData;

	if (!gpsSysData)
	{
		return PVRSRV_ERROR_GENERIC;
	}

	return PVRSRV_OK;
}


#ifdef INLINE_IS_PRAGMA
#pragma inline(SysInitialiseCommon)
#endif
static INLINE PVRSRV_ERROR SysInitialiseCommon(SYS_DATA *psSysData)
{
	PVRSRV_ERROR	eError;
	eError = PVRSRVInit(psSysData);

	return eError;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysDeinitialiseCommon)
#endif
static INLINE IMG_VOID SysDeinitialiseCommon(SYS_DATA *psSysData)
{
	PVRSRVDeInit(psSysData);

	OSDestroyResource(&psSysData->sPowerStateChangeResource);
}
#endif


#if !(defined(NO_HARDWARE) && defined(__linux__) && defined(__KERNEL__))
#define	SysReadHWReg(p, o) OSReadHWReg(p, o)
#define SysWriteHWReg(p, o, v) OSWriteHWReg(p, o, v)
#else
static inline IMG_UINT32 SysReadHWReg(IMG_PVOID pvLinRegBaseAddr, IMG_UINT32 ui32Offset)
{
	return (IMG_UINT32) readl(pvLinRegBaseAddr + ui32Offset);
}

static inline IMG_VOID SysWriteHWReg(IMG_PVOID pvLinRegBaseAddr, IMG_UINT32 ui32Offset, IMG_UINT32 ui32Value)
{
	writel(ui32Value, pvLinRegBaseAddr + ui32Offset);
}
#endif

#if defined(__cplusplus)
}
#endif

#endif

