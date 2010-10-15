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

#ifndef __SGXINFOKM_H__
#define __SGXINFOKM_H__

#include <linux/workqueue.h>
#include "sgxdefs.h"
#include "device.h"
#include "sysconfig.h"
#include "sgxscript.h"
#include "sgxinfo.h"


#define	SGX_HOSTPORT_PRESENT					0x00000001UL

#define PVRSRV_USSE_EDM_POWMAN_IDLE_COMPLETE			(1UL << 2)
#define PVRSRV_USSE_EDM_POWMAN_POWEROFF_COMPLETE		(1UL << 3)
#define PVRSRV_USSE_EDM_POWMAN_POWEROFF_RESTART_IMMEDIATE	(1UL << 4)
#define PVRSRV_USSE_EDM_POWMAN_NO_WORK				(1UL << 5)

#define PVRSRV_USSE_EDM_INTERRUPT_HWR				(1UL << 0)
#define PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER			(1UL << 1)

#define PVRSRV_USSE_EDM_RESMAN_CLEANUP_RT_REQUEST		0x01UL
#define PVRSRV_USSE_EDM_RESMAN_CLEANUP_RC_REQUEST		0x02UL
#define PVRSRV_USSE_EDM_RESMAN_CLEANUP_TC_REQUEST		0x04UL
#define PVRSRV_USSE_EDM_RESMAN_CLEANUP_2DC_REQUEST		0x08UL
#define PVRSRV_USSE_EDM_RESMAN_CLEANUP_SHAREDPBDESC		0x10UL
#define PVRSRV_USSE_EDM_RESMAN_CLEANUP_INVALPD			0x20UL
#define PVRSRV_USSE_EDM_RESMAN_CLEANUP_INVALPT			0x40UL
#define PVRSRV_USSE_EDM_RESMAN_CLEANUP_COMPLETE			0x80UL

#define PVRSRV_USSE_MISCINFO_READY				0x1UL

struct PVRSRV_SGX_CCB_INFO;

struct PVRSRV_SGXDEV_INFO {
	enum PVRSRV_DEVICE_TYPE eDeviceType;
	enum PVRSRV_DEVICE_CLASS eDeviceClass;

	u8 ui8VersionMajor;
	u8 ui8VersionMinor;
	u32 ui32CoreConfig;
	u32 ui32CoreFlags;

	void __iomem *pvRegsBaseKM;

	void *hRegMapping;

	struct IMG_SYS_PHYADDR sRegsPhysBase;

	u32 ui32RegSize;

	u32 ui32CoreClockSpeed;
	u32 ui32uKernelTimerClock;

	void *psStubPBDescListKM;

	struct IMG_DEV_PHYADDR sKernelPDDevPAddr;

	void *pvDeviceMemoryHeap;
	struct PVRSRV_KERNEL_MEM_INFO *psKernelCCBMemInfo;
	struct PVRSRV_SGX_KERNEL_CCB *psKernelCCB;
	struct PVRSRV_SGX_CCB_INFO *psKernelCCBInfo;
	struct PVRSRV_KERNEL_MEM_INFO *psKernelCCBCtlMemInfo;
	struct PVRSRV_SGX_CCB_CTL *psKernelCCBCtl;
	struct PVRSRV_KERNEL_MEM_INFO *psKernelCCBEventKickerMemInfo;
	u32 *pui32KernelCCBEventKicker;
	struct PVRSRV_KERNEL_MEM_INFO *psKernelSGXMiscMemInfo;
	u32 ui32HostKickAddress;
	u32 ui32GetMiscInfoAddress;
	u32 ui32KickTACounter;
	u32 ui32KickTARenderCounter;
	struct PVRSRV_KERNEL_MEM_INFO *psKernelHWPerfCBMemInfo;
	struct PVRSRV_SGXDEV_DIFF_INFO sDiffInfo;
	u32 ui32HWGroupRequested;
	u32 ui32HWReset;
#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
	/*!< Meminfo for EDM status buffer */
	struct PVRSRV_KERNEL_MEM_INFO *psKernelEDMStatusBufferMemInfo;
#endif

	u32 ui32ClientRefCount;

	u32 ui32CacheControl;

	void *pvMMUContextList;

	IMG_BOOL bForcePTOff;

	u32 ui32EDMTaskReg0;
	u32 ui32EDMTaskReg1;

	u32 ui32ClkGateStatusReg;
	u32 ui32ClkGateStatusMask;
	struct SGX_INIT_SCRIPTS sScripts;

	void *hBIFResetPDOSMemHandle;
	struct IMG_DEV_PHYADDR sBIFResetPDDevPAddr;
	struct IMG_DEV_PHYADDR sBIFResetPTDevPAddr;
	struct IMG_DEV_PHYADDR sBIFResetPageDevPAddr;
	u32 *pui32BIFResetPD;
	u32 *pui32BIFResetPT;

	void *hTimer;
	u32 ui32TimeStamp;
	u32 ui32NumResets;

	unsigned long long last_idle;
	unsigned long long burst_start;
	int burst_size;
	int burst_cnt;
	int power_down_delay;

	struct PVRSRV_KERNEL_MEM_INFO *psKernelSGXHostCtlMemInfo;
	struct SGXMKIF_HOST_CTL __iomem *psSGXHostCtl;

	struct PVRSRV_KERNEL_MEM_INFO *psKernelSGXTA3DCtlMemInfo;

	u32 ui32Flags;

#if defined(PDUMP)
	struct PVRSRV_SGX_PDUMP_CONTEXT sPDContext;
#endif


	u32 asSGXDevData[SGX_MAX_DEV_DATA];

};

struct SGX_TIMING_INFORMATION {
	u32 ui32CoreClockSpeed;
	u32 ui32HWRecoveryFreq;
	u32 ui32ActivePowManLatencyms;
	u32 ui32uKernelFreq;
};

struct SGX_DEVICE_MAP {
	u32 ui32Flags;

	struct IMG_SYS_PHYADDR sRegsSysPBase;
	struct IMG_CPU_PHYADDR sRegsCpuPBase;
	void __iomem *pvRegsCpuVBase;
	u32 ui32RegsSize;

	struct IMG_SYS_PHYADDR sLocalMemSysPBase;
	struct IMG_DEV_PHYADDR sLocalMemDevPBase;
	struct IMG_CPU_PHYADDR sLocalMemCpuPBase;
	u32 ui32LocalMemSize;

	u32 ui32IRQ;
};

struct PVRSRV_STUB_PBDESC;
struct PVRSRV_STUB_PBDESC {
	u32 ui32RefCount;
	u32 ui32TotalPBSize;
	struct PVRSRV_KERNEL_MEM_INFO *psSharedPBDescKernelMemInfo;
	struct PVRSRV_KERNEL_MEM_INFO *psHWPBDescKernelMemInfo;
	struct PVRSRV_KERNEL_MEM_INFO **ppsSubKernelMemInfos;
	u32 ui32SubKernelMemInfosCount;
	void *hDevCookie;
	struct PVRSRV_KERNEL_MEM_INFO *psBlockKernelMemInfo;
	struct PVRSRV_STUB_PBDESC *psNext;
};

struct PVRSRV_SGX_CCB_INFO {
	struct PVRSRV_KERNEL_MEM_INFO *psCCBMemInfo;
	struct PVRSRV_KERNEL_MEM_INFO *psCCBCtlMemInfo;
	struct SGXMKIF_COMMAND *psCommands;
	u32 *pui32WriteOffset;
	volatile u32 *pui32ReadOffset;
#if defined(PDUMP)
	u32 ui32CCBDumpWOff;
#endif
};

struct timer_work_data {
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct delayed_work work;
	struct workqueue_struct *work_queue;
	unsigned int interval;
	bool armed;
};

enum PVRSRV_ERROR SGXRegisterDevice(struct PVRSRV_DEVICE_NODE *psDeviceNode);
enum PVRSRV_ERROR SGXOSTimerEnable(struct timer_work_data *data);
enum PVRSRV_ERROR SGXOSTimerCancel(struct timer_work_data *data);
struct timer_work_data *
SGXOSTimerInit(struct PVRSRV_DEVICE_NODE *psDeviceNode);
void SGXOSTimerDeInit(struct timer_work_data *data);

void HWRecoveryResetSGX(struct PVRSRV_DEVICE_NODE *psDeviceNode);
void SGXReset(struct PVRSRV_SGXDEV_INFO *psDevInfo, u32 ui32PDUMPFlags);

enum PVRSRV_ERROR SGXInitialise(struct PVRSRV_SGXDEV_INFO *psDevInfo,
				IMG_BOOL bHardwareRecovery);
enum PVRSRV_ERROR SGXDeinitialise(void *hDevCookie);

void sgx_mark_new_command(struct PVRSRV_DEVICE_NODE *node);
void sgx_mark_power_down(struct PVRSRV_DEVICE_NODE *node);

void SGXStartTimer(struct PVRSRV_SGXDEV_INFO *psDevInfo,
		   IMG_BOOL bStartOSTimer);

enum PVRSRV_ERROR SGXPrePowerStateExt(void *hDevHandle,
				      enum PVR_POWER_STATE eNewPowerState,
				      enum PVR_POWER_STATE eCurrentPowerState);

enum PVRSRV_ERROR SGXPostPowerStateExt(void *hDevHandle,
				       enum PVR_POWER_STATE eNewPowerState,
				       enum PVR_POWER_STATE eCurrentPowerState);

enum PVRSRV_ERROR SGXPreClockSpeedChange(void *hDevHandle,
					 IMG_BOOL bIdleDevice,
					 enum PVR_POWER_STATE
					 eCurrentPowerState);

enum PVRSRV_ERROR SGXPostClockSpeedChange(void *hDevHandle,
					  IMG_BOOL bIdleDevice,
					  enum PVR_POWER_STATE
					  eCurrentPowerState);

enum PVRSRV_ERROR SGXDevInitCompatCheck(struct PVRSRV_DEVICE_NODE
					*psDeviceNode);

void SysGetSGXTimingInformation(struct SGX_TIMING_INFORMATION *psSGXTimingInfo);

#if defined(NO_HARDWARE)
static inline void NoHardwareGenerateEvent(struct PVRSRV_SGXDEV_INFO *psDevInfo,
					   u32 ui32StatusRegister,
					   u32 ui32StatusValue,
					   u32 ui32StatusMask)
{
	u32 ui32RegVal;

	ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, ui32StatusRegister);

	ui32RegVal &= ~ui32StatusMask;
	ui32RegVal |= (ui32StatusValue & ui32StatusMask);

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, ui32StatusRegister, ui32RegVal);
}
#endif

#endif
