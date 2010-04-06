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

#if !defined(__SGXINFO_H__)
#define __SGXINFO_H__

#include "sgxscript.h"

#include "servicesint.h"

#include "services.h"
#include "sgxapi_km.h"

#define SGX_MP_CORE_SELECT(x, i)	(x)

#define SGX_MAX_DEV_DATA		24
#define	SGX_MAX_INIT_MEM_HANDLES	16

#define SGX_BIF_DIR_LIST_INDEX_EDM	0

struct SGX_BRIDGE_INFO_FOR_SRVINIT {
	struct IMG_DEV_PHYADDR sPDDevPAddr;
	struct PVRSRV_HEAP_INFO asHeapInfo[PVRSRV_MAX_CLIENT_HEAPS];
};

struct SGX_BRIDGE_INIT_INFO {
	void *hKernelCCBMemInfo;
	void *hKernelCCBCtlMemInfo;
	void *hKernelCCBEventKickerMemInfo;
	void *hKernelSGXHostCtlMemInfo;
	void *hKernelSGXTA3DCtlMemInfo;
	void *hKernelSGXMiscMemInfo;
	u32 ui32HostKickAddress;
	u32 ui32GetMiscInfoAddress;
	void *hKernelHWPerfCBMemInfo;
#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	void *hKernelEDMStatusBufferMemInfo;
#endif

	u32 ui32EDMTaskReg0;
	u32 ui32EDMTaskReg1;

	u32 ui32ClkGateStatusReg;
	u32 ui32ClkGateStatusMask;

	u32 ui32CacheControl;

	u32 asInitDevData[SGX_MAX_DEV_DATA];
	void *asInitMemHandles[SGX_MAX_INIT_MEM_HANDLES];

	struct SGX_INIT_SCRIPTS sScripts;

};

struct SGXMKIF_COMMAND {
	u32 ui32ServiceAddress;
	u32 ui32Data[3];
};

struct PVRSRV_SGX_KERNEL_CCB {
	struct SGXMKIF_COMMAND asCommands[256];
};

struct PVRSRV_SGX_CCB_CTL {
	u32 ui32WriteOffset;
	u32 ui32ReadOffset;
};

#define SGX_AUXCCBFLAGS_SHARED					0x00000001

enum SGXMKIF_COMMAND_TYPE {
	SGXMKIF_COMMAND_EDM_KICK = 0,
	SGXMKIF_COMMAND_VIDEO_KICK = 1,
	SGXMKIF_COMMAND_REQUEST_SGXMISCINFO = 2,

	SGXMKIF_COMMAND_FORCE_I32 = -1,

};

#define PVRSRV_CCBFLAGS_RASTERCMD			0x1
#define PVRSRV_CCBFLAGS_TRANSFERCMD			0x2
#define PVRSRV_CCBFLAGS_PROCESS_QUEUESCMD		0x3
#define	PVRSRV_CCBFLAGS_POWERCMD			0x5

#define PVRSRV_POWERCMD_POWEROFF			0x1
#define PVRSRV_POWERCMD_IDLE				0x2

#define	SGX_BIF_INVALIDATE_PTCACHE			0x1
#define	SGX_BIF_INVALIDATE_PDCACHE			0x2

struct SGXMKIF_HWDEVICE_SYNC_LIST {
	struct IMG_DEV_VIRTADDR sAccessDevAddr;
	u32 ui32NumSyncObjects;

	struct PVRSRV_DEVICE_SYNC_OBJECT asSyncData[1];
};

struct SGX_DEVICE_SYNC_LIST {
	struct SGXMKIF_HWDEVICE_SYNC_LIST *psHWDeviceSyncList;

	void *hKernelHWSyncListMemInfo;
	struct PVRSRV_CLIENT_MEM_INFO *psHWDeviceSyncListClientMemInfo;
	struct PVRSRV_CLIENT_MEM_INFO *psAccessResourceClientMemInfo;

	volatile u32 *pui32Lock;

	struct SGX_DEVICE_SYNC_LIST *psNext;

	u32 ui32NumSyncObjects;
	void *ahSyncHandles[1];
};

struct SGX_INTERNEL_STATUS_UPDATE {
	struct CTL_STATUS sCtlStatus;
	void *hKernelMemInfo;
	/* pdump specific - required? */
	u32 ui32LastStatusUpdateDumpVal;
};

struct SGX_CCB_KICK {
	enum SGXMKIF_COMMAND_TYPE eCommand;
	struct SGXMKIF_COMMAND sCommand;
	void *hCCBKernelMemInfo;

	u32 ui32NumDstSyncObjects;
	void *hKernelHWSyncListMemInfo;
	void *sDstSyncHandle;

	u32 ui32NumTAStatusVals;
	u32 ui32Num3DStatusVals;

	void *ahTAStatusSyncInfo[SGX_MAX_TA_STATUS_VALS];
	void *ah3DStatusSyncInfo[SGX_MAX_3D_STATUS_VALS];

	IMG_BOOL bFirstKickOrResume;
#if (defined(NO_HARDWARE) || defined(PDUMP))
	IMG_BOOL bTerminateOrAbort;
#endif
	IMG_BOOL bKickRender;

	u32 ui32CCBOffset;

	u32 ui32NumSrcSyncs;
	void *ahSrcKernelSyncInfo[SGX_MAX_SRC_SYNCS];

	IMG_BOOL bTADependency;
	void *hTA3DSyncInfo;

	void *hTASyncInfo;
	void *h3DSyncInfo;
#if defined(PDUMP)
	u32 ui32CCBDumpWOff;
#endif
#if defined(NO_HARDWARE)
	u32 ui32WriteOpsPendingVal;
#endif
};

#define SGX_KERNEL_USE_CODE_BASE_INDEX		15

struct SGXMKIF_HOST_CTL {

	u32 ui32PowerStatus;
	u32 ui32uKernelDetectedLockups;
	u32 ui32HostDetectedLockups;
	u32 ui32HWRecoverySampleRate;
	u32 ui32ActivePowManSampleRate;
	u32 ui32InterruptFlags;
	u32 ui32InterruptClearFlags;

	u32 ui32ResManFlags;
	struct IMG_DEV_VIRTADDR sResManCleanupData;

	u32 ui32NumActivePowerEvents;

	u32 ui32HWPerfFlags;

#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	/* !< See SGXMK_STATUS_BUFFER */
	struct IMG_DEV_VIRTADDR sEDMStatusBuffer;
#endif

	/*< to count time wraps in the Timer task */
	u32 ui32TimeWraps;
};

struct SGX_CLIENT_INFO {
	u32 ui32ProcessID;
	void *pvProcess;
	struct PVRSRV_MISC_INFO sMiscInfo;

	u32 asDevData[SGX_MAX_DEV_DATA];

};

struct SGX_INTERNAL_DEVINFO {
	u32 ui32Flags;
	void *hHostCtlKernelMemInfoHandle;
	IMG_BOOL bForcePTOff;
};

#define SGXTQ_MAX_STATUS		(SGX_MAX_TRANSFER_STATUS_VALS + 2)

#define SGXMKIF_TQFLAGS_NOSYNCUPDATE				0x00000001
#define SGXMKIF_TQFLAGS_KEEPPENDING				0x00000002
#define SGXMKIF_TQFLAGS_TATQ_SYNC				0x00000004
#define SGXMKIF_TQFLAGS_3DTQ_SYNC				0x00000008
struct SGXMKIF_CMDTA_SHARED {
	u32 ui32NumTAStatusVals;
	u32 ui32Num3DStatusVals;

	u32 ui32TATQSyncWriteOpsPendingVal;
	struct IMG_DEV_VIRTADDR sTATQSyncWriteOpsCompleteDevVAddr;
	u32 ui32TATQSyncReadOpsPendingVal;
	struct IMG_DEV_VIRTADDR sTATQSyncReadOpsCompleteDevVAddr;

	u32 ui323DTQSyncWriteOpsPendingVal;
	struct IMG_DEV_VIRTADDR s3DTQSyncWriteOpsCompleteDevVAddr;
	u32 ui323DTQSyncReadOpsPendingVal;
	struct IMG_DEV_VIRTADDR s3DTQSyncReadOpsCompleteDevVAddr;

	u32 ui32NumSrcSyncs;
	struct PVRSRV_DEVICE_SYNC_OBJECT asSrcSyncs[SGX_MAX_SRC_SYNCS];

	struct CTL_STATUS sCtlTAStatusInfo[SGX_MAX_TA_STATUS_VALS];
	struct CTL_STATUS sCtl3DStatusInfo[SGX_MAX_3D_STATUS_VALS];

	struct PVRSRV_DEVICE_SYNC_OBJECT sTA3DDependency;

};

struct SGXMKIF_TRANSFERCMD_SHARED {

	u32 ui32SrcReadOpPendingVal;
	struct IMG_DEV_VIRTADDR sSrcReadOpsCompleteDevAddr;

	u32 ui32SrcWriteOpPendingVal;
	struct IMG_DEV_VIRTADDR sSrcWriteOpsCompleteDevAddr;

	u32 ui32DstReadOpPendingVal;
	struct IMG_DEV_VIRTADDR sDstReadOpsCompleteDevAddr;

	u32 ui32DstWriteOpPendingVal;
	struct IMG_DEV_VIRTADDR sDstWriteOpsCompleteDevAddr;

	u32 ui32TASyncWriteOpsPendingVal;
	struct IMG_DEV_VIRTADDR sTASyncWriteOpsCompleteDevVAddr;
	u32 ui32TASyncReadOpsPendingVal;
	struct IMG_DEV_VIRTADDR sTASyncReadOpsCompleteDevVAddr;

	u32 ui323DSyncWriteOpsPendingVal;
	struct IMG_DEV_VIRTADDR s3DSyncWriteOpsCompleteDevVAddr;
	u32 ui323DSyncReadOpsPendingVal;
	struct IMG_DEV_VIRTADDR s3DSyncReadOpsCompleteDevVAddr;

	u32 ui32NumStatusVals;
	struct CTL_STATUS sCtlStatusInfo[SGXTQ_MAX_STATUS];
};

struct PVRSRV_TRANSFER_SGX_KICK {
	void *hCCBMemInfo;
	u32 ui32SharedCmdCCBOffset;

	struct IMG_DEV_VIRTADDR sHWTransferContextDevVAddr;

	void *hTASyncInfo;
	void *h3DSyncInfo;

	u32 ui32NumSrcSync;
	void *ahSrcSyncInfo[SGX_MAX_TRANSFER_SYNC_OPS];

	u32 ui32NumDstSync;
	void *ahDstSyncInfo[SGX_MAX_TRANSFER_SYNC_OPS];

	u32 ui32Flags;

	u32 ui32PDumpFlags;
#if defined(PDUMP)
	u32 ui32CCBDumpWOff;
#endif
};

#define PVRSRV_SGX_DIFF_NUM_COUNTERS	9

struct PVRSRV_SGXDEV_DIFF_INFO {
	u32 aui32Counters[PVRSRV_SGX_DIFF_NUM_COUNTERS];
	u32 ui32Time[2];
	u32 ui32Marker[2];
};

#define SGXMKIF_HWPERF_CB_SIZE					0x100

struct SGXMKIF_HWPERF_CB_ENTRY {
	u32 ui32FrameNo;
	u32 ui32Type;
	u32 ui32Ordinal;
	u32 ui32TimeWraps;
	u32 ui32Time;
	u32 ui32Counters[PVRSRV_SGX_HWPERF_NUM_COUNTERS];
};

struct SGXMKIF_HWPERF_CB {
	u32 ui32Woff;
	u32 ui32Roff;
	u32 ui32OrdinalGRAPHICS;
	u32 ui32OrdinalMK_EXECUTION;
	struct SGXMKIF_HWPERF_CB_ENTRY psHWPerfCBData[SGXMKIF_HWPERF_CB_SIZE];
};

struct PVRSRV_SGX_MISCINFO_INFO {
	u32 ui32MiscInfoFlags;
	struct PVRSRV_SGX_MISCINFO_FEATURES sSGXFeatures;
};

#endif
