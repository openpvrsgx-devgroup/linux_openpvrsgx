/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
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
#include "pvr_debug.h"
#include "services.h"

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

typedef IMG_HANDLE (*PFN_HTIMER_CREATE) (IMG_VOID);
typedef IMG_UINT32 (*PFN_HTIMER_GETUS) (IMG_HANDLE);
typedef IMG_VOID (*PFN_HTIMER_DESTROY) (IMG_HANDLE);
typedef struct _SYS_DATA_TAG_
{
    IMG_UINT32                  ui32NumDevices;      	   	
	SYS_DEVICE_ID				sDeviceID[SYS_DEVICE_COUNT];
    PVRSRV_DEVICE_NODE			*psDeviceNodeList;			
    PVRSRV_POWER_DEV			*psPowerDeviceList;			
	PVRSRV_RESOURCE				sPowerStateChangeResource;	
   	PVRSRV_SYS_POWER_STATE		eCurrentPowerState;			
   	PVRSRV_SYS_POWER_STATE		eFailedPowerState;			
   	IMG_UINT32		 			ui32CurrentOSPowerState;	
    PVRSRV_QUEUE_INFO           *psQueueList;           	
   	PVRSRV_KERNEL_SYNC_INFO 	*psSharedSyncInfoList;		
    IMG_PVOID                   pvEnvSpecificData;      	
    IMG_PVOID                   pvSysSpecificData;    	  	
	PVRSRV_RESOURCE				sQProcessResource;			
	IMG_VOID					*pvSOCRegsBase;				
    IMG_HANDLE                  hSOCTimerRegisterOSMemHandle; 
	IMG_UINT32					*pvSOCTimerRegisterKM;		
	IMG_VOID					*pvSOCClockGateRegsBase;	
	IMG_UINT32					ui32SOCClockGateRegsSize;
															
	struct _DEVICE_COMMAND_DATA_ *apsDeviceCommandData[SYS_DEVICE_COUNT];
															

	RA_ARENA					*apsLocalDevMemArena[SYS_MAX_LOCAL_DEVMEM_ARENAS]; 

    IMG_CHAR                    *pszVersionString;          
#if defined (SUPPORT_SID_INTERFACE)
	PVRSRV_EVENTOBJECT_KM		*psGlobalEventObject;		
#else
	PVRSRV_EVENTOBJECT			*psGlobalEventObject;		
#endif

	PVRSRV_MISC_INFO_CPUCACHEOP_TYPE ePendingCacheOpType;	

	PFN_HTIMER_CREATE	pfnHighResTimerCreate;
	PFN_HTIMER_GETUS	pfnHighResTimerGetus;
	PFN_HTIMER_DESTROY	pfnHighResTimerDestroy;
} SYS_DATA;



#if defined (CUSTOM_DISPLAY_SEGMENT)
PVRSRV_ERROR SysGetDisplaySegmentAddress (IMG_VOID *pvDevInfo, IMG_VOID *pvPhysicalAddress, IMG_UINT32 *pui32Length);
#endif

PVRSRV_ERROR SysInitialise(IMG_VOID);
PVRSRV_ERROR SysFinalise(IMG_VOID);

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

#if defined(SYS_SUPPORTS_SGX_IDLE_CALLBACK)
IMG_VOID SysSGXIdleTransition(IMG_BOOL bSGXIdle);
#endif 

#if defined(SYS_CUSTOM_POWERLOCK_WRAP)
PVRSRV_ERROR SysPowerLockWrap(IMG_BOOL bTryLock);
IMG_VOID SysPowerLockUnwrap(IMG_VOID);
#endif

PVRSRV_ERROR SysOEMFunction (	IMG_UINT32	ui32ID,
								IMG_VOID	*pvIn,
								IMG_UINT32  ulInSize,
								IMG_VOID	*pvOut,
								IMG_UINT32	ulOutSize);


IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_CPU_PHYADDR cpu_paddr);
IMG_DEV_PHYADDR SysSysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr);
IMG_SYS_PHYADDR SysDevPAddrToSysPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_DEV_PHYADDR SysPAddr);
IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr (IMG_SYS_PHYADDR SysPAddr);
IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr (IMG_CPU_PHYADDR cpu_paddr);
#if defined(PVR_LMA)
IMG_BOOL SysVerifyCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_CPU_PHYADDR CpuPAddr);
IMG_BOOL SysVerifySysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr);
#endif

extern SYS_DATA* gpsSysData;


#if !defined(USE_CODE)

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysAcquireData)
#endif
static INLINE IMG_VOID SysAcquireData(SYS_DATA **ppsSysData)
{
	
	*ppsSysData = gpsSysData;

	



	PVR_ASSERT (gpsSysData != IMG_NULL);
}


#ifdef INLINE_IS_PRAGMA
#pragma inline(SysAcquireDataNoCheck)
#endif
static INLINE SYS_DATA * SysAcquireDataNoCheck(IMG_VOID)
{
	
	return gpsSysData;
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

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysHighResTimerCreate)
#endif
static INLINE IMG_HANDLE SysHighResTimerCreate(IMG_VOID)
{
	SYS_DATA *psSysData;

	SysAcquireData(&psSysData);
	return psSysData->pfnHighResTimerCreate();
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysHighResTimerGetus)
#endif
static INLINE IMG_UINT32 SysHighResTimerGetus(IMG_HANDLE hTimer)
{
	SYS_DATA *psSysData;

	SysAcquireData(&psSysData);
	return psSysData->pfnHighResTimerGetus(hTimer);
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(SysHighResTimerDestroy)
#endif
static INLINE IMG_VOID SysHighResTimerDestroy(IMG_HANDLE hTimer)
{
	SYS_DATA *psSysData;

	SysAcquireData(&psSysData);
	psSysData->pfnHighResTimerDestroy(hTimer);
}
#endif

