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

#ifndef __SGXINFOKM_H__
#define __SGXINFOKM_H__

#include "sgxdefs.h"
#include "device.h"
#include "power.h"
#include "sysconfig.h"
#include "sgxscript.h"
#include "sgxinfo.h"

#if defined (__cplusplus)
extern "C" {
#endif

#define		SGX_HOSTPORT_PRESENT			0x00000001UL


#define SGX_PDUMPREG_NAME		"SGXREG"

typedef struct _PVRSRV_STUB_PBDESC_ PVRSRV_STUB_PBDESC;


typedef struct _PVRSRV_SGX_CCB_INFO_ *PPVRSRV_SGX_CCB_INFO;

typedef struct _PVRSRV_SGXDEV_INFO_
{
	PVRSRV_DEVICE_TYPE		eDeviceType;
	PVRSRV_DEVICE_CLASS		eDeviceClass;

	IMG_UINT8				ui8VersionMajor;
	IMG_UINT8				ui8VersionMinor;
	IMG_UINT32				ui32CoreConfig;
	IMG_UINT32				ui32CoreFlags;

	
	IMG_PVOID				pvRegsBaseKM;

#if defined(SGX_FEATURE_HOST_PORT)
	
	IMG_PVOID				pvHostPortBaseKM;
	
	IMG_UINT32				ui32HPSize;
	
	IMG_SYS_PHYADDR			sHPSysPAddr;
#endif

	
	IMG_HANDLE				hRegMapping;

	
	IMG_SYS_PHYADDR			sRegsPhysBase;
	
	IMG_UINT32				ui32RegSize;

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
	
	IMG_UINT32				ui32ExtSysCacheRegsSize;
	
	IMG_DEV_PHYADDR			sExtSysCacheRegsDevPBase;
	
	IMG_UINT32				*pui32ExtSystemCacheRegsPT;
	
	IMG_HANDLE				hExtSystemCacheRegsPTPageOSMemHandle;
	
	IMG_SYS_PHYADDR			sExtSystemCacheRegsPTSysPAddr;
#endif

	
	IMG_UINT32				ui32CoreClockSpeed;
	IMG_UINT32				ui32uKernelTimerClock;

	PVRSRV_STUB_PBDESC		*psStubPBDescListKM;


	
	IMG_DEV_PHYADDR			sKernelPDDevPAddr;

	IMG_VOID				*pvDeviceMemoryHeap;
	PPVRSRV_KERNEL_MEM_INFO	psKernelCCBMemInfo;			
	PVRSRV_SGX_KERNEL_CCB	*psKernelCCB;			
	PPVRSRV_SGX_CCB_INFO	psKernelCCBInfo;		
	PPVRSRV_KERNEL_MEM_INFO	psKernelCCBCtlMemInfo;	
	PVRSRV_SGX_CCB_CTL		*psKernelCCBCtl;		
	PPVRSRV_KERNEL_MEM_INFO psKernelCCBEventKickerMemInfo; 
	IMG_UINT32				*pui32KernelCCBEventKicker; 
#if defined(PDUMP)
	IMG_UINT32				ui32KernelCCBEventKickerDumpVal; 
#endif 
 	PVRSRV_KERNEL_MEM_INFO	*psKernelSGXMiscMemInfo;	
	IMG_UINT32				aui32HostKickAddr[SGXMKIF_CMD_MAX];		
#if defined(SGX_SUPPORT_HWPROFILING)
	PPVRSRV_KERNEL_MEM_INFO psKernelHWProfilingMemInfo;
#endif
	PPVRSRV_KERNEL_MEM_INFO		psKernelHWPerfCBMemInfo;		
	PPVRSRV_KERNEL_MEM_INFO		psKernelTASigBufferMemInfo;		
	PPVRSRV_KERNEL_MEM_INFO		psKernel3DSigBufferMemInfo;		
#if defined(FIX_HW_BRN_29702)
	PPVRSRV_KERNEL_MEM_INFO psKernelCFIMemInfo;	
#endif
#if defined(FIX_HW_BRN_29823)
	PPVRSRV_KERNEL_MEM_INFO	psKernelDummyTermStreamMemInfo; 
#endif
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && defined(FIX_HW_BRN_31425)
	PPVRSRV_KERNEL_MEM_INFO	psKernelVDMSnapShotBufferMemInfo; 
	PPVRSRV_KERNEL_MEM_INFO	psKernelVDMCtrlStreamBufferMemInfo; 
#endif
#if defined(SGX_FEATURE_VDM_CONTEXT_SWITCH) && \
	defined(FIX_HW_BRN_33657) && defined(SUPPORT_SECURE_33657_FIX)
	PPVRSRV_KERNEL_MEM_INFO	psKernelVDMStateUpdateBufferMemInfo; 
#endif
#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	PPVRSRV_KERNEL_MEM_INFO	psKernelEDMStatusBufferMemInfo; 
#endif
	
	IMG_UINT32				ui32ClientRefCount;

	
	IMG_UINT32				ui32CacheControl;

	
	IMG_UINT32				ui32ClientBuildOptions;

	
	SGX_MISCINFO_STRUCT_SIZES	sSGXStructSizes;

	


	IMG_VOID				*pvMMUContextList;

	
	IMG_BOOL				bForcePTOff;

	IMG_UINT32				ui32EDMTaskReg0;
	IMG_UINT32				ui32EDMTaskReg1;

	IMG_UINT32				ui32ClkGateCtl;
	IMG_UINT32				ui32ClkGateCtl2;
	IMG_UINT32				ui32ClkGateStatusReg;
	IMG_UINT32				ui32ClkGateStatusMask;
#if defined(SGX_FEATURE_MP)
	IMG_UINT32				ui32MasterClkGateStatusReg;
	IMG_UINT32				ui32MasterClkGateStatusMask;
	IMG_UINT32				ui32MasterClkGateStatus2Reg;
	IMG_UINT32				ui32MasterClkGateStatus2Mask;
#endif 
	SGX_INIT_SCRIPTS		sScripts;

	
	IMG_HANDLE 				hBIFResetPDOSMemHandle;
	IMG_DEV_PHYADDR 		sBIFResetPDDevPAddr;
	IMG_DEV_PHYADDR 		sBIFResetPTDevPAddr;
	IMG_DEV_PHYADDR 		sBIFResetPageDevPAddr;
	IMG_UINT32				*pui32BIFResetPD;
	IMG_UINT32				*pui32BIFResetPT;

#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030) && defined(SGX_FEATURE_HOST_PORT)
	
	IMG_HANDLE				hBRN22997PTPageOSMemHandle;
	IMG_HANDLE				hBRN22997PDPageOSMemHandle;
	IMG_DEV_PHYADDR 		sBRN22997PTDevPAddr;
	IMG_DEV_PHYADDR 		sBRN22997PDDevPAddr;
	IMG_UINT32				*pui32BRN22997PT;
	IMG_UINT32				*pui32BRN22997PD;
	IMG_SYS_PHYADDR 		sBRN22997SysPAddr;
#endif 

#if defined(SUPPORT_HW_RECOVERY)
	
	IMG_HANDLE				hTimer;
	
	IMG_UINT32				ui32TimeStamp;
#endif

	
	IMG_UINT32				ui32NumResets;

	
	PVRSRV_KERNEL_MEM_INFO			*psKernelSGXHostCtlMemInfo;
	SGXMKIF_HOST_CTL				*psSGXHostCtl;

	
	PVRSRV_KERNEL_MEM_INFO			*psKernelSGXTA3DCtlMemInfo;

#if defined(FIX_HW_BRN_31272) || defined(FIX_HW_BRN_31780) || defined(FIX_HW_BRN_33920)
	PVRSRV_KERNEL_MEM_INFO			*psKernelSGXPTLAWriteBackMemInfo;
#endif

	IMG_UINT32				ui32Flags;

	
	IMG_UINT32				ui32MemTilingUsage;

	#if defined(PDUMP)
	PVRSRV_SGX_PDUMP_CONTEXT	sPDContext;
	#endif

#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
	
	IMG_VOID				*pvDummyPTPageCpuVAddr;
	IMG_DEV_PHYADDR			sDummyPTDevPAddr;
	IMG_HANDLE				hDummyPTPageOSMemHandle;
	IMG_VOID				*pvDummyDataPageCpuVAddr;
	IMG_DEV_PHYADDR 		sDummyDataDevPAddr;
	IMG_HANDLE				hDummyDataPageOSMemHandle;
#endif
#if defined(PDUMP)
	PDUMP_MMU_ATTRIB sMMUAttrib;
#endif
	IMG_UINT32				asSGXDevData[SGX_MAX_DEV_DATA];

#if defined(FIX_HW_BRN_31620)
	
	IMG_VOID			*pvBRN31620DummyPageCpuVAddr;
	IMG_HANDLE			hBRN31620DummyPageOSMemHandle;
	IMG_DEV_PHYADDR			sBRN31620DummyPageDevPAddr;

	
	IMG_VOID			*pvBRN31620DummyPTCpuVAddr;
	IMG_HANDLE			hBRN31620DummyPTOSMemHandle;
	IMG_DEV_PHYADDR			sBRN31620DummyPTDevPAddr;

	IMG_HANDLE			hKernelMMUContext;
#endif

} PVRSRV_SGXDEV_INFO;


typedef struct _SGX_TIMING_INFORMATION_
{
	IMG_UINT32			ui32CoreClockSpeed;
	IMG_UINT32			ui32HWRecoveryFreq;
	IMG_BOOL			bEnableActivePM;
	IMG_UINT32			ui32ActivePowManLatencyms;
	IMG_UINT32			ui32uKernelFreq;
} SGX_TIMING_INFORMATION;

typedef struct _SGX_DEVICE_MAP_
{
	IMG_UINT32				ui32Flags;

	
	IMG_SYS_PHYADDR			sRegsSysPBase;
	IMG_CPU_PHYADDR			sRegsCpuPBase;
	IMG_CPU_VIRTADDR		pvRegsCpuVBase;
	IMG_UINT32				ui32RegsSize;

#if defined(SGX_FEATURE_HOST_PORT)
	IMG_SYS_PHYADDR			sHPSysPBase;
	IMG_CPU_PHYADDR			sHPCpuPBase;
	IMG_UINT32				ui32HPSize;
#endif

	
	IMG_SYS_PHYADDR			sLocalMemSysPBase;
	IMG_DEV_PHYADDR			sLocalMemDevPBase;
	IMG_CPU_PHYADDR			sLocalMemCpuPBase;
	IMG_UINT32				ui32LocalMemSize;

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
	IMG_UINT32				ui32ExtSysCacheRegsSize;
	IMG_DEV_PHYADDR			sExtSysCacheRegsDevPBase;
#endif

	
	IMG_UINT32				ui32IRQ;

#if !defined(SGX_DYNAMIC_TIMING_INFO)
	
	SGX_TIMING_INFORMATION	sTimingInfo;
#endif
#if defined(PDUMP)
	
	IMG_CHAR				*pszPDumpDevName;
#endif
} SGX_DEVICE_MAP;


struct _PVRSRV_STUB_PBDESC_
{
	IMG_UINT32		ui32RefCount;
	IMG_UINT32		ui32TotalPBSize;
	PVRSRV_KERNEL_MEM_INFO  *psSharedPBDescKernelMemInfo;
	PVRSRV_KERNEL_MEM_INFO  *psHWPBDescKernelMemInfo;
	PVRSRV_KERNEL_MEM_INFO	**ppsSubKernelMemInfos;
	IMG_UINT32		ui32SubKernelMemInfosCount;
	IMG_HANDLE		hDevCookie;
	PVRSRV_KERNEL_MEM_INFO  *psBlockKernelMemInfo;
	PVRSRV_KERNEL_MEM_INFO  *psHWBlockKernelMemInfo;
	IMG_DEV_VIRTADDR	sHWPBDescDevVAddr;
	PVRSRV_STUB_PBDESC	*psNext;
	PVRSRV_STUB_PBDESC	**ppsThis;
};

typedef struct _PVRSRV_SGX_CCB_INFO_
{
	PVRSRV_KERNEL_MEM_INFO	*psCCBMemInfo;			
	PVRSRV_KERNEL_MEM_INFO	*psCCBCtlMemInfo;		
	SGXMKIF_COMMAND		*psCommands;			
	IMG_UINT32				*pui32WriteOffset;		
	volatile IMG_UINT32		*pui32ReadOffset;		
#if defined(PDUMP)
	IMG_UINT32				ui32CCBDumpWOff;		
#endif
} PVRSRV_SGX_CCB_INFO;


typedef struct _SGX_BRIDGE_INIT_INFO_KM_
{
	IMG_HANDLE	hKernelCCBMemInfo;
	IMG_HANDLE	hKernelCCBCtlMemInfo;
	IMG_HANDLE	hKernelCCBEventKickerMemInfo;
	IMG_HANDLE	hKernelSGXHostCtlMemInfo;
	IMG_HANDLE	hKernelSGXTA3DCtlMemInfo;
#if defined(FIX_HW_BRN_31272) || defined(FIX_HW_BRN_31780) || defined(FIX_HW_BRN_33920)
	IMG_HANDLE	hKernelSGXPTLAWriteBackMemInfo;
#endif
	IMG_HANDLE	hKernelSGXMiscMemInfo;

	IMG_UINT32	aui32HostKickAddr[SGXMKIF_CMD_MAX];

	SGX_INIT_SCRIPTS sScripts;

	IMG_UINT32	ui32ClientBuildOptions;
	SGX_MISCINFO_STRUCT_SIZES	sSGXStructSizes;

#if defined(SGX_SUPPORT_HWPROFILING)
	IMG_HANDLE	hKernelHWProfilingMemInfo;
#endif
#if defined(SUPPORT_SGX_HWPERF)
	IMG_HANDLE	hKernelHWPerfCBMemInfo;
#endif
	IMG_HANDLE	hKernelTASigBufferMemInfo;
	IMG_HANDLE	hKernel3DSigBufferMemInfo;

#if defined(FIX_HW_BRN_29702)
	IMG_HANDLE	hKernelCFIMemInfo;
#endif
#if defined(FIX_HW_BRN_29823)
	IMG_HANDLE	hKernelDummyTermStreamMemInfo;
#endif
#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	IMG_HANDLE	hKernelEDMStatusBufferMemInfo;
#endif

	IMG_UINT32 ui32EDMTaskReg0;
	IMG_UINT32 ui32EDMTaskReg1;

	IMG_UINT32 ui32ClkGateStatusReg;
	IMG_UINT32 ui32ClkGateStatusMask;
#if defined(SGX_FEATURE_MP)
#endif 

	IMG_UINT32 ui32CacheControl;

	IMG_UINT32	asInitDevData[SGX_MAX_DEV_DATA];
	IMG_HANDLE	asInitMemHandles[SGX_MAX_INIT_MEM_HANDLES];

} SGX_BRIDGE_INIT_INFO_KM;


typedef struct _SGX_INTERNEL_STATUS_UPDATE_KM_
{
	CTL_STATUS				sCtlStatus;
	IMG_HANDLE				hKernelMemInfo;
} SGX_INTERNEL_STATUS_UPDATE_KM;


typedef struct _SGX_CCB_KICK_KM_
{
	SGXMKIF_COMMAND		sCommand;
	IMG_HANDLE	hCCBKernelMemInfo;

	IMG_UINT32	ui32NumDstSyncObjects;
	IMG_HANDLE	hKernelHWSyncListMemInfo;

	
	IMG_HANDLE	*pahDstSyncHandles;

	IMG_UINT32	ui32NumTAStatusVals;
	IMG_UINT32	ui32Num3DStatusVals;

#if defined(SUPPORT_SGX_NEW_STATUS_VALS)
	SGX_INTERNEL_STATUS_UPDATE_KM	asTAStatusUpdate[SGX_MAX_TA_STATUS_VALS];
	SGX_INTERNEL_STATUS_UPDATE_KM	as3DStatusUpdate[SGX_MAX_3D_STATUS_VALS];
#else
	IMG_HANDLE	ahTAStatusSyncInfo[SGX_MAX_TA_STATUS_VALS];
	IMG_HANDLE	ah3DStatusSyncInfo[SGX_MAX_3D_STATUS_VALS];
#endif

	IMG_BOOL	bFirstKickOrResume;
#if (defined(NO_HARDWARE) || defined(PDUMP))
	IMG_BOOL	bTerminateOrAbort;
#endif

	
	IMG_UINT32	ui32CCBOffset;

#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
	
	IMG_UINT32	ui32NumTASrcSyncs;
	IMG_HANDLE	ahTASrcKernelSyncInfo[SGX_MAX_TA_SRC_SYNCS];
	IMG_UINT32	ui32NumTADstSyncs;
	IMG_HANDLE	ahTADstKernelSyncInfo[SGX_MAX_TA_DST_SYNCS];
	IMG_UINT32	ui32Num3DSrcSyncs;
	IMG_HANDLE	ah3DSrcKernelSyncInfo[SGX_MAX_3D_SRC_SYNCS];
#else
	
	IMG_UINT32	ui32NumSrcSyncs;
	IMG_HANDLE	ahSrcKernelSyncInfo[SGX_MAX_SRC_SYNCS];
#endif

	
	IMG_BOOL	bTADependency;
	IMG_HANDLE	hTA3DSyncInfo;

	IMG_HANDLE	hTASyncInfo;
	IMG_HANDLE	h3DSyncInfo;
#if defined(PDUMP)
	IMG_UINT32	ui32CCBDumpWOff;
#endif
#if defined(NO_HARDWARE)
	IMG_UINT32	ui32WriteOpsPendingVal;
#endif
} SGX_CCB_KICK_KM;


#if defined(TRANSFER_QUEUE)
typedef struct _PVRSRV_TRANSFER_SGX_KICK_KM_
{
	IMG_HANDLE		hCCBMemInfo;
	IMG_UINT32		ui32SharedCmdCCBOffset;

	IMG_DEV_VIRTADDR 	sHWTransferContextDevVAddr;

	IMG_HANDLE		hTASyncInfo;
	IMG_HANDLE		h3DSyncInfo;

	IMG_UINT32		ui32NumSrcSync;
	IMG_HANDLE		ahSrcSyncInfo[SGX_MAX_TRANSFER_SYNC_OPS];

	IMG_UINT32		ui32NumDstSync;
	IMG_HANDLE		ahDstSyncInfo[SGX_MAX_TRANSFER_SYNC_OPS];

	IMG_UINT32		ui32Flags;

	IMG_UINT32		ui32PDumpFlags;
#if defined(PDUMP)
	IMG_UINT32		ui32CCBDumpWOff;
#endif
} PVRSRV_TRANSFER_SGX_KICK_KM, *PPVRSRV_TRANSFER_SGX_KICK_KM;

#if defined(SGX_FEATURE_2D_HARDWARE)
typedef struct _PVRSRV_2D_SGX_KICK_KM_
{
	IMG_HANDLE		hCCBMemInfo;
	IMG_UINT32		ui32SharedCmdCCBOffset;

	IMG_DEV_VIRTADDR 	sHW2DContextDevVAddr;

	IMG_UINT32		ui32NumSrcSync;
	IMG_HANDLE		ahSrcSyncInfo[SGX_MAX_2D_SRC_SYNC_OPS];

	
	IMG_HANDLE 		hDstSyncInfo;

	
	IMG_HANDLE		hTASyncInfo;

	
	IMG_HANDLE		h3DSyncInfo;

	IMG_UINT32		ui32PDumpFlags;
#if defined(PDUMP)
	IMG_UINT32		ui32CCBDumpWOff;
#endif
} PVRSRV_2D_SGX_KICK_KM, *PPVRSRV_2D_SGX_KICK_KM;
#endif	
#endif 

PVRSRV_ERROR SGXRegisterDevice (PVRSRV_DEVICE_NODE *psDeviceNode);

IMG_VOID SGXOSTimer(IMG_VOID *pvData);

IMG_VOID SGXReset(PVRSRV_SGXDEV_INFO	*psDevInfo,
				  IMG_BOOL				bHardwareRecovery,
				  IMG_UINT32			ui32PDUMPFlags);

IMG_VOID SGXInitClocks(PVRSRV_SGXDEV_INFO	*psDevInfo,
					   IMG_UINT32			ui32PDUMPFlags);

PVRSRV_ERROR SGXInitialise(PVRSRV_SGXDEV_INFO	*psDevInfo,
						   IMG_BOOL				bHardwareRecovery);
PVRSRV_ERROR SGXDeinitialise(IMG_HANDLE hDevCookie);

PVRSRV_ERROR SGXPrePowerState(IMG_HANDLE				hDevHandle, 
							  PVRSRV_DEV_POWER_STATE	eNewPowerState, 
							  PVRSRV_DEV_POWER_STATE	eCurrentPowerState);

PVRSRV_ERROR SGXPostPowerState(IMG_HANDLE				hDevHandle, 
							   PVRSRV_DEV_POWER_STATE	eNewPowerState, 
							   PVRSRV_DEV_POWER_STATE	eCurrentPowerState);

PVRSRV_ERROR SGXPreClockSpeedChange(IMG_HANDLE				hDevHandle,
									IMG_BOOL				bIdleDevice,
									PVRSRV_DEV_POWER_STATE	eCurrentPowerState);

PVRSRV_ERROR SGXPostClockSpeedChange(IMG_HANDLE				hDevHandle,
									 IMG_BOOL				bIdleDevice,
									 PVRSRV_DEV_POWER_STATE	eCurrentPowerState);

IMG_VOID SGXPanic(PVRSRV_SGXDEV_INFO	*psDevInfo);

PVRSRV_ERROR SGXDevInitCompatCheck(PVRSRV_DEVICE_NODE *psDeviceNode);

#if defined(SGX_DYNAMIC_TIMING_INFO)
IMG_VOID SysGetSGXTimingInformation(SGX_TIMING_INFORMATION *psSGXTimingInfo);
#endif

#if defined(NO_HARDWARE)
static INLINE IMG_VOID NoHardwareGenerateEvent(PVRSRV_SGXDEV_INFO		*psDevInfo,
												IMG_UINT32 ui32StatusRegister,
												IMG_UINT32 ui32StatusValue,
												IMG_UINT32 ui32StatusMask)
{
	IMG_UINT32 ui32RegVal;

	ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, ui32StatusRegister);

	ui32RegVal &= ~ui32StatusMask;
	ui32RegVal |= (ui32StatusValue & ui32StatusMask);

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, ui32StatusRegister, ui32RegVal);
}
#endif

#if defined(__cplusplus)
}
#endif

#endif 

