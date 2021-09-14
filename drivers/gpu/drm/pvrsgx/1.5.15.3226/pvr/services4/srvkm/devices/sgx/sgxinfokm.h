/*************************************************************************/ /*!
@Title          SGX kernel services structues/functions
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Structures and inline functions for KM services component
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/
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

/****************************************************************************/
/* kernel only defines: 													*/
/****************************************************************************/
/* SGXDeviceMap Flag defines */
#define		SGX_HOSTPORT_PRESENT			0x00000001UL


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

	/* Kernel mode linear address of device registers */
	IMG_PVOID				pvRegsBaseKM;

#if defined(SGX_FEATURE_HOST_PORT)
	/* Kernel mode linear address of host port */
	IMG_PVOID				pvHostPortBaseKM;
	/* HP size */
	IMG_UINT32				ui32HPSize;
	/* HP syspaddr */
	IMG_SYS_PHYADDR			sHPSysPAddr;
#endif

	/* FIXME: The alloc for this should go through OSAllocMem in future */
	IMG_HANDLE				hRegMapping;

	/* System physical address of device registers*/
	IMG_SYS_PHYADDR			sRegsPhysBase;
	/*  Register region size in bytes */
	IMG_UINT32				ui32RegSize;

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
	/* external system cache register region size in bytes */
	IMG_UINT32				ui32ExtSysCacheRegsSize;
	/* external system cache register device relative physical address */
	IMG_DEV_PHYADDR			sExtSysCacheRegsDevPBase;
	/* ptr to page table  */
	IMG_UINT32				*pui32ExtSystemCacheRegsPT;
	/* handle to page table alloc/mapping */
	IMG_HANDLE				hExtSystemCacheRegsPTPageOSMemHandle;
	/* sys phys addr of PT */
	IMG_SYS_PHYADDR			sExtSystemCacheRegsPTSysPAddr;
#endif

	/*  SGX clock speed */
	IMG_UINT32				ui32CoreClockSpeed;
	IMG_UINT32				ui32uKernelTimerClock;

	PVRSRV_STUB_PBDESC		*psStubPBDescListKM;


	/* kernel memory context info */
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
#endif /* PDUMP */
	PVRSRV_KERNEL_MEM_INFO	*psKernelSGXMiscMemInfo;
	IMG_UINT32				aui32HostKickAddr[SGXMKIF_CMD_MAX];
#if defined(SGX_SUPPORT_HWPROFILING)
	PPVRSRV_KERNEL_MEM_INFO psKernelHWProfilingMemInfo;
#endif
	IMG_UINT32				ui32KickTACounter;
	IMG_UINT32				ui32KickTARenderCounter;
#if defined(SUPPORT_SGX_HWPERF)
	PPVRSRV_KERNEL_MEM_INFO		psKernelHWPerfCBMemInfo;
	IMG_UINT32					ui32HWGroupRequested;
	IMG_UINT32					ui32HWReset;
#endif
#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
	PPVRSRV_KERNEL_MEM_INFO	psKernelEDMStatusBufferMemInfo;
#endif
#if defined(SGX_FEATURE_OVERLAPPED_SPM)
	PPVRSRV_KERNEL_MEM_INFO	psKernelTmpRgnHeaderMemInfo;
#endif
#if defined(SGX_FEATURE_SPM_MODE_0)
	PPVRSRV_KERNEL_MEM_INFO	psKernelTmpDPMStateMemInfo;
#endif

	/* Client reference count */
	IMG_UINT32				ui32ClientRefCount;

	/* cache control word for micro kernel cache flush/invalidates */
	IMG_UINT32				ui32CacheControl;

	/* client-side build options */
	IMG_UINT32				ui32ClientBuildOptions;

	/* client-side microkernel structure sizes */
	SGX_MISCINFO_STRUCT_SIZES	sSGXStructSizes;

	/*
		if we don't preallocate the pagetables we must 
		insert newly allocated page tables dynamically 
	*/
	IMG_VOID				*pvMMUContextList;

	/* Copy of registry ForcePTOff entry */
	IMG_BOOL				bForcePTOff;

	IMG_UINT32				ui32EDMTaskReg0;
	IMG_UINT32				ui32EDMTaskReg1;

	IMG_UINT32				ui32ClkGateStatusReg;
	IMG_UINT32				ui32ClkGateStatusMask;
#if defined(SGX_FEATURE_MP)
	IMG_UINT32				ui32MasterClkGateStatusReg;
	IMG_UINT32				ui32MasterClkGateStatusMask;
#endif
	SGX_INIT_SCRIPTS		sScripts;

	/* Members associated with dummy PD needed for BIF reset */
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
	/* Timeout callback handle */
	IMG_HANDLE				hTimer;
	/* HW recovery Time stamp */
	IMG_UINT32				ui32TimeStamp;
#endif

	/* Number of SGX resets */
	IMG_UINT32				ui32NumResets;

	/* host control */
	PVRSRV_KERNEL_MEM_INFO			*psKernelSGXHostCtlMemInfo;
	SGXMKIF_HOST_CTL				*psSGXHostCtl;

	/* TA/3D control */
	PVRSRV_KERNEL_MEM_INFO			*psKernelSGXTA3DCtlMemInfo;

	IMG_UINT32				ui32Flags;

	#if defined(PDUMP)
	PVRSRV_SGX_PDUMP_CONTEXT	sPDContext;
	#endif

#if defined(SUPPORT_SGX_MMU_DUMMY_PAGE)
	/* SGX MMU dummy page details */
	IMG_VOID				*pvDummyPTPageCpuVAddr;
	IMG_DEV_PHYADDR			sDummyPTDevPAddr;
	IMG_HANDLE				hDummyPTPageOSMemHandle;
	IMG_VOID				*pvDummyDataPageCpuVAddr;
	IMG_DEV_PHYADDR 		sDummyDataDevPAddr;
	IMG_HANDLE				hDummyDataPageOSMemHandle;
#endif

	IMG_UINT32				asSGXDevData[SGX_MAX_DEV_DATA];

} PVRSRV_SGXDEV_INFO;


typedef struct _SGX_TIMING_INFORMATION_
{
	IMG_UINT32			ui32CoreClockSpeed;
	IMG_UINT32			ui32HWRecoveryFreq;
	IMG_BOOL			bEnableActivePM;
	IMG_UINT32			ui32ActivePowManLatencyms;
	IMG_UINT32			ui32uKernelFreq;
} SGX_TIMING_INFORMATION;

/* FIXME Rename this structure to sg more generalised as it's been extended*/
/* SGX device map */
typedef struct _SGX_DEVICE_MAP_
{
	IMG_UINT32				ui32Flags;

	/* Registers */
	IMG_SYS_PHYADDR			sRegsSysPBase;
	IMG_CPU_PHYADDR			sRegsCpuPBase;
	IMG_CPU_VIRTADDR		pvRegsCpuVBase;
	IMG_UINT32				ui32RegsSize;

#if defined(SGX_FEATURE_HOST_PORT)
	IMG_SYS_PHYADDR			sHPSysPBase;
	IMG_CPU_PHYADDR			sHPCpuPBase;
	IMG_UINT32				ui32HPSize;
#endif

	/* Local Device Memory Region: (if present) */
	IMG_SYS_PHYADDR			sLocalMemSysPBase;
	IMG_DEV_PHYADDR			sLocalMemDevPBase;
	IMG_CPU_PHYADDR			sLocalMemCpuPBase;
	IMG_UINT32				ui32LocalMemSize;

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)
	IMG_UINT32				ui32ExtSysCacheRegsSize;
	IMG_DEV_PHYADDR			sExtSysCacheRegsDevPBase;
#endif

	/* device interrupt IRQ */
	IMG_UINT32				ui32IRQ;

#if !defined(SGX_DYNAMIC_TIMING_INFO)
	/* timing information*/
	SGX_TIMING_INFORMATION	sTimingInfo;
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
	PVRSRV_STUB_PBDESC	*psNext;
	PVRSRV_STUB_PBDESC	**ppsThis;
};

/*!
 ******************************************************************************
 * CCB control structure for SGX
 *****************************************************************************/
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

PVRSRV_ERROR SGXRegisterDevice (PVRSRV_DEVICE_NODE *psDeviceNode);

IMG_VOID SGXOSTimer(IMG_VOID *pvData);

IMG_VOID SGXReset(PVRSRV_SGXDEV_INFO	*psDevInfo,
				  IMG_UINT32			 ui32PDUMPFlags);

PVRSRV_ERROR SGXInitialise(PVRSRV_SGXDEV_INFO	*psDevInfo);
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

IMG_VOID SGXPanic(PVRSRV_DEVICE_NODE	*psDeviceNode);

PVRSRV_ERROR SGXDevInitCompatCheck(PVRSRV_DEVICE_NODE *psDeviceNode);

#if defined(SGX_DYNAMIC_TIMING_INFO)
IMG_VOID SysGetSGXTimingInformation(SGX_TIMING_INFORMATION *psSGXTimingInfo);
#endif

/****************************************************************************/
/* kernel only functions: 													*/
/****************************************************************************/
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

#endif /* __SGXINFOKM_H__ */

/*****************************************************************************
 End of file (sgxinfokm.h)
*****************************************************************************/
