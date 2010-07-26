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

#include <stddef.h>

#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include "sgxdefs.h"
#include "sgxmmu.h"
#include "services_headers.h"
#include "buffer_manager.h"
#include "sgxapi_km.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "sgxconfig.h"
#include "sysconfig.h"
#include "pvr_bridge_km.h"
#include "sgx_bridge_km.h"
#include "resman.h"

#include "pdump_km.h"
#include "ra.h"
#include "mmu.h"
#include "handle.h"
#include "perproc.h"

#include "sgxutils.h"
#include "pvrversion.h"
#include "sgx_options.h"

static IMG_BOOL SGX_ISRHandler(void *pvData);

static u32 gui32EventStatusServicesByISR;

static enum PVRSRV_ERROR SGXGetBuildInfoKM(struct PVRSRV_SGXDEV_INFO *psDevInfo,
				    struct PVRSRV_DEVICE_NODE *psDeviceNode);

static void SGXCommandComplete(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	BUG_ON(in_irq());

	SGXScheduleProcessQueuesKM(psDeviceNode);
}

static u32 DeinitDevInfo(struct PVRSRV_SGXDEV_INFO *psDevInfo)
{
	if (psDevInfo->psKernelCCBInfo != NULL)
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_SGX_CCB_INFO),
			  psDevInfo->psKernelCCBInfo, NULL);

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR InitDevInfo(struct PVRSRV_PER_PROCESS_DATA *psPerProc,
				struct PVRSRV_DEVICE_NODE *psDeviceNode,
				struct SGX_BRIDGE_INIT_INFO *psInitInfo)
{
	struct PVRSRV_SGXDEV_INFO *psDevInfo =
	    (struct PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;
	enum PVRSRV_ERROR eError;

	struct PVRSRV_SGX_CCB_INFO *psKernelCCBInfo = NULL;

	PVR_UNREFERENCED_PARAMETER(psPerProc);
	psDevInfo->sScripts = psInitInfo->sScripts;

	psDevInfo->psKernelCCBMemInfo =
	    (struct PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelCCBMemInfo;
	psDevInfo->psKernelCCB =
	    (struct PVRSRV_SGX_KERNEL_CCB *)psDevInfo->psKernelCCBMemInfo->
						    pvLinAddrKM;

	psDevInfo->psKernelCCBCtlMemInfo =
	    (struct PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelCCBCtlMemInfo;
	psDevInfo->psKernelCCBCtl =
	    (struct PVRSRV_SGX_CCB_CTL *)psDevInfo->psKernelCCBCtlMemInfo->
						    pvLinAddrKM;

	psDevInfo->psKernelCCBEventKickerMemInfo =
	    (struct PVRSRV_KERNEL_MEM_INFO *)
			    psInitInfo->hKernelCCBEventKickerMemInfo;
	psDevInfo->pui32KernelCCBEventKicker =
	    (u32 *)psDevInfo->psKernelCCBEventKickerMemInfo->pvLinAddrKM;

	psDevInfo->psKernelSGXHostCtlMemInfo =
	    (struct PVRSRV_KERNEL_MEM_INFO *)psInitInfo->
						    hKernelSGXHostCtlMemInfo;
	psDevInfo->psSGXHostCtl = (struct SGXMKIF_HOST_CTL __force __iomem *)
		psDevInfo->psKernelSGXHostCtlMemInfo->pvLinAddrKM;

	psDevInfo->psKernelSGXTA3DCtlMemInfo =
	    (struct PVRSRV_KERNEL_MEM_INFO *)psInitInfo->
						    hKernelSGXTA3DCtlMemInfo;

	psDevInfo->psKernelSGXMiscMemInfo =
	    (struct PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelSGXMiscMemInfo;

	psDevInfo->psKernelHWPerfCBMemInfo =
	    (struct PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelHWPerfCBMemInfo;
#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
	psDevInfo->psKernelEDMStatusBufferMemInfo =
	    (struct PVRSRV_KERNEL_MEM_INFO *)psInitInfo->
						  hKernelEDMStatusBufferMemInfo;
#endif

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			    sizeof(struct PVRSRV_SGX_CCB_INFO),
			    (void **)&psKernelCCBInfo, NULL);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "InitDevInfo: Failed to alloc memory");
		goto failed_allockernelccb;
	}

	OSMemSet(psKernelCCBInfo, 0, sizeof(struct PVRSRV_SGX_CCB_INFO));
	psKernelCCBInfo->psCCBMemInfo = psDevInfo->psKernelCCBMemInfo;
	psKernelCCBInfo->psCCBCtlMemInfo = psDevInfo->psKernelCCBCtlMemInfo;
	psKernelCCBInfo->psCommands = psDevInfo->psKernelCCB->asCommands;
	psKernelCCBInfo->pui32WriteOffset =
				&psDevInfo->psKernelCCBCtl->ui32WriteOffset;
	psKernelCCBInfo->pui32ReadOffset =
				&psDevInfo->psKernelCCBCtl->ui32ReadOffset;
	psDevInfo->psKernelCCBInfo = psKernelCCBInfo;

	psDevInfo->ui32HostKickAddress = psInitInfo->ui32HostKickAddress;

	psDevInfo->ui32GetMiscInfoAddress = psInitInfo->ui32GetMiscInfoAddress;

	psDevInfo->bForcePTOff = IMG_FALSE;

	psDevInfo->ui32CacheControl = psInitInfo->ui32CacheControl;

	psDevInfo->ui32EDMTaskReg0 = psInitInfo->ui32EDMTaskReg0;
	psDevInfo->ui32EDMTaskReg1 = psInitInfo->ui32EDMTaskReg1;
	psDevInfo->ui32ClkGateStatusReg = psInitInfo->ui32ClkGateStatusReg;
	psDevInfo->ui32ClkGateStatusMask = psInitInfo->ui32ClkGateStatusMask;

	OSMemCopy(&psDevInfo->asSGXDevData, &psInitInfo->asInitDevData,
		  sizeof(psDevInfo->asSGXDevData));

	return PVRSRV_OK;

failed_allockernelccb:
	DeinitDevInfo(psDevInfo);

	return eError;
}

static enum PVRSRV_ERROR SGXRunScript(struct PVRSRV_SGXDEV_INFO *psDevInfo,
				 union SGX_INIT_COMMAND *psScript,
				 u32 ui32NumInitCommands)
{
	u32 ui32PC;
	union SGX_INIT_COMMAND *psComm;

	for (ui32PC = 0, psComm = psScript;
	     ui32PC < ui32NumInitCommands; ui32PC++, psComm++) {
		switch (psComm->eOp) {
		case SGX_INIT_OP_WRITE_HW_REG:
			{
				OSWriteHWReg(psDevInfo->pvRegsBaseKM,
					     psComm->sWriteHWReg.ui32Offset,
					     psComm->sWriteHWReg.ui32Value);
				PDUMPREG(psComm->sWriteHWReg.ui32Offset,
					 psComm->sWriteHWReg.ui32Value);
				break;
			}
#if defined(PDUMP)
		case SGX_INIT_OP_PDUMP_HW_REG:
			{
				PDUMPREG(psComm->sPDumpHWReg.ui32Offset,
					 psComm->sPDumpHWReg.ui32Value);
				break;
			}
#endif
		case SGX_INIT_OP_HALT:
			{
				return PVRSRV_OK;
			}
		case SGX_INIT_OP_ILLEGAL:

		default:
			{
				PVR_DPF(PVR_DBG_ERROR,
				     "SGXRunScript: PC %d: Illegal command: %d",
				      ui32PC, psComm->eOp);
				return PVRSRV_ERROR_GENERIC;
			}
		}

	}

	return PVRSRV_ERROR_GENERIC;
}

enum PVRSRV_ERROR SGXInitialise(struct PVRSRV_SGXDEV_INFO *psDevInfo,
				  IMG_BOOL bHardwareRecovery)
{
	enum PVRSRV_ERROR eError;

	PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS,
			      "SGX initialisation script part 1\n");
	eError =
	    SGXRunScript(psDevInfo, psDevInfo->sScripts.asInitCommandsPart1,
			 SGX_MAX_INIT_COMMANDS);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SGXInitialise: SGXRunScript (part 1) failed (%d)",
			 eError);
		return PVRSRV_ERROR_GENERIC;
	}
	PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS,
			      "End of SGX initialisation script part 1\n");

	SGXReset(psDevInfo, PDUMP_FLAGS_CONTINUOUS);



	*psDevInfo->pui32KernelCCBEventKicker = 0;
#if defined(PDUMP)
	PDUMPMEM(NULL, psDevInfo->psKernelCCBEventKickerMemInfo, 0,
		 sizeof(*psDevInfo->pui32KernelCCBEventKicker),
		 PDUMP_FLAGS_CONTINUOUS,
		 MAKEUNIQUETAG(psDevInfo->psKernelCCBEventKickerMemInfo));
#endif

	PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS,
			      "SGX initialisation script part 2\n");
	eError =
	    SGXRunScript(psDevInfo, psDevInfo->sScripts.asInitCommandsPart2,
			 SGX_MAX_INIT_COMMANDS);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SGXInitialise: SGXRunScript (part 2) failed (%d)",
			 eError);
		return PVRSRV_ERROR_GENERIC;
	}
	PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS,
			      "End of SGX initialisation script part 2\n");

	SGXStartTimer(psDevInfo, (IMG_BOOL)!bHardwareRecovery);

	if (bHardwareRecovery) {
		struct SGXMKIF_HOST_CTL __iomem *psSGXHostCtl =
		    psDevInfo->psSGXHostCtl;

		if (PollForValueKM(&psSGXHostCtl->ui32InterruptClearFlags, 0,
		     PVRSRV_USSE_EDM_INTERRUPT_HWR,
		     MAX_HW_TIME_US / WAIT_TRY_COUNT, 1000) != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "SGXInitialise: "
					"Wait for uKernel HW Recovery failed");
			PVR_DBG_BREAK;
			return PVRSRV_ERROR_RETRY;
		}
	}

	PVR_ASSERT(psDevInfo->psKernelCCBCtl->ui32ReadOffset ==
		   psDevInfo->psKernelCCBCtl->ui32WriteOffset);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SGXDeinitialise(void *hDevCookie)
{
	struct PVRSRV_SGXDEV_INFO *psDevInfo = (struct PVRSRV_SGXDEV_INFO *)
								   hDevCookie;
	enum PVRSRV_ERROR eError;

	if (psDevInfo->pvRegsBaseKM == NULL)
		return PVRSRV_OK;

	eError = SGXRunScript(psDevInfo, psDevInfo->sScripts.asDeinitCommands,
			 SGX_MAX_DEINIT_COMMANDS);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SGXDeinitialise: SGXRunScript failed (%d)", eError);
		return PVRSRV_ERROR_GENERIC;
	}

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR DevInitSGXPart1(void *pvDeviceNode)
{
	struct PVRSRV_SGXDEV_INFO *psDevInfo;
	void *hKernelDevMemContext;
	struct IMG_DEV_PHYADDR sPDDevPAddr;
	u32 i;
	struct PVRSRV_DEVICE_NODE *psDeviceNode = (struct PVRSRV_DEVICE_NODE *)
								   pvDeviceNode;
	struct DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap =
	    psDeviceNode->sDevMemoryInfo.psDeviceMemoryHeap;
	enum PVRSRV_ERROR eError;

	PDUMPCOMMENT("SGX Initialisation Part 1");

	PDUMPCOMMENT("SGX Core Version Information: %s",
		     SGX_CORE_FRIENDLY_NAME);
	PDUMPCOMMENT("SGX Core Revision Information: multi rev support");

	if (OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
		       sizeof(struct PVRSRV_SGXDEV_INFO),
		       (void **)&psDevInfo, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			"DevInitSGXPart1 : Failed to alloc memory for DevInfo");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	OSMemSet(psDevInfo, 0, sizeof(struct PVRSRV_SGXDEV_INFO));

	psDevInfo->eDeviceType = DEV_DEVICE_TYPE;
	psDevInfo->eDeviceClass = DEV_DEVICE_CLASS;

	psDeviceNode->pvDevice = (void *) psDevInfo;

	psDevInfo->pvDeviceMemoryHeap = (void *) psDeviceMemoryHeap;

	hKernelDevMemContext = BM_CreateContext(psDeviceNode, &sPDDevPAddr,
						NULL, NULL);

	psDevInfo->sKernelPDDevPAddr = sPDDevPAddr;

	for (i = 0; i < psDeviceNode->sDevMemoryInfo.ui32HeapCount; i++) {
		void *hDevMemHeap;

		switch (psDeviceMemoryHeap[i].DevMemHeapType) {
		case DEVICE_MEMORY_HEAP_KERNEL:
		case DEVICE_MEMORY_HEAP_SHARED:
		case DEVICE_MEMORY_HEAP_SHARED_EXPORTED:
			{
				hDevMemHeap =
				    BM_CreateHeap(hKernelDevMemContext,
						  &psDeviceMemoryHeap[i]);

				psDeviceMemoryHeap[i].hDevMemHeap = hDevMemHeap;
				break;
			}
		}
	}

	eError = MMU_BIFResetPDAlloc(psDevInfo);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "DevInitSGX : Failed to alloc memory for BIF reset");
		return PVRSRV_ERROR_GENERIC;
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SGXGetInfoForSrvinitKM(void *hDevHandle,
				struct SGX_BRIDGE_INFO_FOR_SRVINIT *psInitInfo)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct PVRSRV_SGXDEV_INFO *psDevInfo;
	enum PVRSRV_ERROR eError;

	PDUMPCOMMENT("SGXGetInfoForSrvinit");

	psDeviceNode = (struct PVRSRV_DEVICE_NODE *)hDevHandle;
	psDevInfo = (struct PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;

	psInitInfo->sPDDevPAddr = psDevInfo->sKernelPDDevPAddr;

	eError =
	    PVRSRVGetDeviceMemHeapsKM(hDevHandle, &psInitInfo->asHeapInfo[0]);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "SGXGetInfoForSrvinit: "
				"PVRSRVGetDeviceMemHeapsKM failed (%d)",
			 eError);
		return PVRSRV_ERROR_GENERIC;
	}

	return eError;
}

enum PVRSRV_ERROR DevInitSGXPart2KM(struct PVRSRV_PER_PROCESS_DATA *psPerProc,
				   void *hDevHandle,
				   struct SGX_BRIDGE_INIT_INFO *psInitInfo)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct PVRSRV_SGXDEV_INFO *psDevInfo;
	enum PVRSRV_ERROR eError;
	struct SGX_DEVICE_MAP *psSGXDeviceMap;
	enum PVR_POWER_STATE eDefaultPowerState;
	u32 l;

	PDUMPCOMMENT("SGX Initialisation Part 2");

	psDeviceNode = (struct PVRSRV_DEVICE_NODE *)hDevHandle;
	psDevInfo = (struct PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;

	eError = InitDevInfo(psPerProc, psDeviceNode, psInitInfo);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "DevInitSGXPart2KM: "
					"Failed to load EDM program");
		goto failed_init_dev_info;
	}


	eError = SysGetDeviceMemoryMap(PVRSRV_DEVICE_TYPE_SGX,
				       (void **) &psSGXDeviceMap);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "DevInitSGXPart2KM: "
					"Failed to get device memory map!");
		return PVRSRV_ERROR_INIT_FAILURE;
	}

	if (psSGXDeviceMap->pvRegsCpuVBase) {
		psDevInfo->pvRegsBaseKM = psSGXDeviceMap->pvRegsCpuVBase;
	} else {
		psDevInfo->pvRegsBaseKM =
		    OSMapPhysToLin(psSGXDeviceMap->sRegsCpuPBase,
				   psSGXDeviceMap->ui32RegsSize,
				   PVRSRV_HAP_KERNEL_ONLY | PVRSRV_HAP_UNCACHED,
				   NULL);
		if (!psDevInfo->pvRegsBaseKM) {
			PVR_DPF(PVR_DBG_ERROR,
				 "DevInitSGXPart2KM: Failed to map in regs\n");
			return PVRSRV_ERROR_BAD_MAPPING;
		}
	}
	psDevInfo->ui32RegSize = psSGXDeviceMap->ui32RegsSize;
	psDevInfo->sRegsPhysBase = psSGXDeviceMap->sRegsSysPBase;

	psDeviceNode->pvISRData = psDeviceNode;

	PVR_ASSERT(psDeviceNode->pfnDeviceISR == SGX_ISRHandler);

	pvr_dev_lock();
	l = readl(&psDevInfo->psSGXHostCtl->ui32PowerStatus);
	l |= PVRSRV_USSE_EDM_POWMAN_NO_WORK;
	writel(l, &psDevInfo->psSGXHostCtl->ui32PowerStatus);
	pvr_dev_unlock();
	eDefaultPowerState = PVRSRV_POWER_STATE_D3;

	eError = PVRSRVRegisterPowerDevice(psDeviceNode->sDevId.ui32DeviceIndex,
					   SGXPrePowerStateExt,
					   SGXPostPowerStateExt,
					   SGXPreClockSpeedChange,
					   SGXPostClockSpeedChange,
					   (void *) psDeviceNode,
					   PVRSRV_POWER_STATE_D3,
					   eDefaultPowerState);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "DevInitSGXPart2KM: "
				"failed to register device with power manager");
		return eError;
	}

	OSMemSet(psDevInfo->psKernelCCB, 0,
		 sizeof(struct PVRSRV_SGX_KERNEL_CCB));
	OSMemSet(psDevInfo->psKernelCCBCtl, 0,
		 sizeof(struct PVRSRV_SGX_CCB_CTL));
	OSMemSet(psDevInfo->pui32KernelCCBEventKicker, 0,
		 sizeof(*psDevInfo->pui32KernelCCBEventKicker));
	PDUMPCOMMENT("Initialise Kernel CCB");
	PDUMPMEM(NULL, psDevInfo->psKernelCCBMemInfo, 0,
		 sizeof(struct PVRSRV_SGX_KERNEL_CCB), PDUMP_FLAGS_CONTINUOUS,
		 MAKEUNIQUETAG(psDevInfo->psKernelCCBMemInfo));
	PDUMPCOMMENT("Initialise Kernel CCB Control");
	PDUMPMEM(NULL, psDevInfo->psKernelCCBCtlMemInfo, 0,
		 sizeof(struct PVRSRV_SGX_CCB_CTL), PDUMP_FLAGS_CONTINUOUS,
		 MAKEUNIQUETAG(psDevInfo->psKernelCCBCtlMemInfo));
	PDUMPCOMMENT("Initialise Kernel CCB Event Kicker");
	PDUMPMEM(NULL, psDevInfo->psKernelCCBEventKickerMemInfo, 0,
		 sizeof(*psDevInfo->pui32KernelCCBEventKicker),
		 PDUMP_FLAGS_CONTINUOUS,
		 MAKEUNIQUETAG(psDevInfo->psKernelCCBEventKickerMemInfo));

	psDevInfo->hTimer = SGXOSTimerInit(psDeviceNode);
	if (!psDevInfo->hTimer)
		PVR_DPF(PVR_DBG_ERROR, "DevInitSGXPart2KM : "
			"Failed to initialize HW recovery timer");

	return PVRSRV_OK;

failed_init_dev_info:
	return eError;
}

static enum PVRSRV_ERROR DevDeInitSGX(void *pvDeviceNode)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode =
		(struct PVRSRV_DEVICE_NODE *)pvDeviceNode;
	struct PVRSRV_SGXDEV_INFO *psDevInfo =
		(struct PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;
	enum PVRSRV_ERROR eError;
	u32 ui32Heap;
	struct DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap;
	struct SGX_DEVICE_MAP *psSGXDeviceMap;

	if (!psDevInfo) {
		PVR_DPF(PVR_DBG_ERROR, "DevDeInitSGX: Null DevInfo");
		return PVRSRV_OK;
	}
	if (psDevInfo->hTimer) {
		SGXOSTimerCancel(psDevInfo->hTimer);
		SGXOSTimerDeInit(psDevInfo->hTimer);
		psDevInfo->hTimer = NULL;
	}

	MMU_BIFResetPDFree(psDevInfo);

	DeinitDevInfo(psDevInfo);


	psDeviceMemoryHeap =
	    (struct DEVICE_MEMORY_HEAP_INFO *)psDevInfo->pvDeviceMemoryHeap;
	for (ui32Heap = 0;
	     ui32Heap < psDeviceNode->sDevMemoryInfo.ui32HeapCount;
	     ui32Heap++) {
		switch (psDeviceMemoryHeap[ui32Heap].DevMemHeapType) {
		case DEVICE_MEMORY_HEAP_KERNEL:
		case DEVICE_MEMORY_HEAP_SHARED:
		case DEVICE_MEMORY_HEAP_SHARED_EXPORTED:
			{
				if (psDeviceMemoryHeap[ui32Heap].hDevMemHeap !=
				    NULL)
					BM_DestroyHeap(psDeviceMemoryHeap
						       [ui32Heap].hDevMemHeap);
				break;
			}
		}
	}

	if (!pvr_put_ctx(psDeviceNode->sDevMemoryInfo.pBMKernelContext))
		pr_err("%s: kernel context still in use, can't free it",
			__func__);

	eError = PVRSRVRemovePowerDevice(
				((struct PVRSRV_DEVICE_NODE *)pvDeviceNode)->
				  sDevId.ui32DeviceIndex);
	if (eError != PVRSRV_OK)
		return eError;

	eError = SysGetDeviceMemoryMap(PVRSRV_DEVICE_TYPE_SGX,
				       (void **)&psSGXDeviceMap);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "DevDeInitSGX: Failed to get device memory map!");
		return eError;
	}

	if (!psSGXDeviceMap->pvRegsCpuVBase)
		if (psDevInfo->pvRegsBaseKM != NULL)
			OSUnMapPhysToLin(psDevInfo->pvRegsBaseKM,
					 psDevInfo->ui32RegSize,
					 PVRSRV_HAP_KERNEL_ONLY |
						 PVRSRV_HAP_UNCACHED,
					 NULL);

	OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
		  sizeof(struct PVRSRV_SGXDEV_INFO), psDevInfo, NULL);

	psDeviceNode->pvDevice = NULL;

	if (psDeviceMemoryHeap != NULL)
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			  sizeof(struct DEVICE_MEMORY_HEAP_INFO) *
			  psDeviceNode->sDevMemoryInfo.ui32HeapCount,
			  psDeviceMemoryHeap, NULL);

	return PVRSRV_OK;
}

#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG

#define SGXMK_TRACE_BUFFER_SIZE		512

static void dump_edm(struct PVRSRV_SGXDEV_INFO *psDevInfo)
{
	u32 *trace_buffer =
		psDevInfo->psKernelEDMStatusBufferMemInfo->pvLinAddrKM;
	u32 last_code, write_offset;
	int i;

	last_code = *trace_buffer;
	trace_buffer++;
	write_offset = *trace_buffer;

	pr_err("Last SGX microkernel status code: 0x%x\n", last_code);

	trace_buffer++;
	/* Dump the status values */

	for (i = 0; i < SGXMK_TRACE_BUFFER_SIZE; i++) {
		u32     *buf;
		buf = trace_buffer + (((write_offset + i) %
					SGXMK_TRACE_BUFFER_SIZE) * 4);
		pr_err("(MKT%u) %8.8X %8.8X %8.8X %8.8X\n", i,
				buf[2], buf[3], buf[1], buf[0]);
	}
}
#else
static void dump_edm(struct PVRSRV_SGXDEV_INFO *psDevInfo) {}
#endif

static void dump_process_info(struct PVRSRV_DEVICE_NODE *dev)
{
	struct PVRSRV_SGXDEV_INFO *dev_info = dev->pvDevice;
	u32 page_dir = readl(dev_info->pvRegsBaseKM +
				EUR_CR_BIF_DIR_LIST_BASE0);
	struct BM_CONTEXT *bm_ctx;
	struct RESMAN_CONTEXT *res_ctx = NULL;

	bm_ctx = bm_find_context(dev->sDevMemoryInfo.pBMContext, page_dir);
	if (bm_ctx)
		res_ctx = pvr_get_resman_ctx(bm_ctx);

	if (res_ctx) {
		struct task_struct *tsk;
		struct PVRSRV_PER_PROCESS_DATA *proc;
		int pid;

		proc = pvr_get_proc_by_ctx(res_ctx);
		pid = proc->ui32PID;
		rcu_read_lock();
		tsk = pid_task(find_vpid(pid), PIDTYPE_PID);
		pr_err("PID = %d, process name = %s\n", pid, tsk->comm);
		rcu_read_unlock();
	}
}

static void dump_sgx_registers(struct PVRSRV_SGXDEV_INFO *psDevInfo)
{
	pr_err("EVENT_STATUS =     0x%08X\n"
		"EVENT_STATUS2 =    0x%08X\n"
		"BIF_CTRL =         0x%08X\n"
		"BIF_INT_STAT =     0x%08X\n"
		"BIF_MEM_REQ_STAT = 0x%08X\n"
		"BIF_FAULT  =       0x%08X\n"
		"CLKGATECTL =       0x%08X\n",
		readl(psDevInfo->pvRegsBaseKM + EUR_CR_EVENT_STATUS),
		readl(psDevInfo->pvRegsBaseKM + EUR_CR_EVENT_STATUS2),
		readl(psDevInfo->pvRegsBaseKM + EUR_CR_BIF_CTRL),
		readl(psDevInfo->pvRegsBaseKM + EUR_CR_BIF_INT_STAT),
		readl(psDevInfo->pvRegsBaseKM + EUR_CR_BIF_MEM_REQ_STAT),
		readl(psDevInfo->pvRegsBaseKM + EUR_CR_BIF_FAULT),
		readl(psDevInfo->pvRegsBaseKM + EUR_CR_CLKGATECTL));
}

/* Should be called with pvr_lock held */
void HWRecoveryResetSGX(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	enum PVRSRV_ERROR eError;
	struct PVRSRV_SGXDEV_INFO *psDevInfo =
	    (struct PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;
	struct SGXMKIF_HOST_CTL __iomem *psSGXHostCtl =
					psDevInfo->psSGXHostCtl;
	u32 l;
	int max_retries = 10;

	BUG_ON(!pvr_is_locked());

	pvr_dev_lock();

	l = readl(&psSGXHostCtl->ui32InterruptClearFlags);
	l |= PVRSRV_USSE_EDM_INTERRUPT_HWR;
	writel(l, &psSGXHostCtl->ui32InterruptClearFlags);

	pr_err("%s: SGX Hardware Recovery triggered\n", __func__);

	dump_process_info(psDeviceNode);
	dump_sgx_registers(psDevInfo);
	dump_edm(psDevInfo);

	PDUMPSUSPEND();

	do {
		eError = SGXInitialise(psDevInfo, IMG_TRUE);
		if (eError != PVRSRV_ERROR_RETRY)
			break;
	} while (max_retries--);

	if (eError != PVRSRV_OK) {
		pr_err("%s: recovery failed (%d). Disabling the driver",
			__func__, eError);
		pvr_disable();

		PDUMPRESUME();
		pvr_dev_unlock();

		return;
	}

	PDUMPRESUME();

	SGXScheduleProcessQueues(psDeviceNode);

	pvr_dev_unlock();

	PVRSRVProcessQueues(IMG_TRUE);
}

static unsigned long sgx_reset_forced;

static void SGXOSTimer(struct work_struct *work)
{
	struct timer_work_data *data = container_of(work,
						    struct timer_work_data,
						    work.work);
	struct PVRSRV_DEVICE_NODE *psDeviceNode = data->psDeviceNode;
	struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	static u32 ui32EDMTasks;
	static u32 ui32LockupCounter;
	static u32 ui32NumResets;
	u32 ui32CurrentEDMTasks;
	IMG_BOOL bLockup = IMG_FALSE;
	IMG_BOOL bPoweredDown;

	pvr_lock();

	if (!data->armed || pvr_is_disabled()) {
		pvr_unlock();
		return;
	}

	psDevInfo->ui32TimeStamp++;

#if defined(NO_HARDWARE)
	bPoweredDown = IMG_TRUE;
#else
	bPoweredDown = (IMG_BOOL) !SGXIsDevicePowered(psDeviceNode);
#endif

	if (bPoweredDown) {
		ui32LockupCounter = 0;
	} else {
		pvr_dev_lock();
		ui32CurrentEDMTasks = OSReadHWReg(psDevInfo->pvRegsBaseKM,
						psDevInfo->ui32EDMTaskReg0);
		if (psDevInfo->ui32EDMTaskReg1 != 0)
			ui32CurrentEDMTasks ^=
			    OSReadHWReg(psDevInfo->pvRegsBaseKM,
					psDevInfo->ui32EDMTaskReg1);
		if ((ui32CurrentEDMTasks == ui32EDMTasks) &&
		    (psDevInfo->ui32NumResets == ui32NumResets)) {
			ui32LockupCounter++;
			if (ui32LockupCounter == 3) {
				ui32LockupCounter = 0;
				PVR_DPF(PVR_DBG_ERROR, "SGXOSTimer() "
					"detected SGX lockup (0x%x tasks)",
					 ui32EDMTasks);

				bLockup = IMG_TRUE;
			}
		} else {
			ui32LockupCounter = 0;
			ui32EDMTasks = ui32CurrentEDMTasks;
			ui32NumResets = psDevInfo->ui32NumResets;
		}
		pvr_dev_unlock();
	}

	bLockup |= cmpxchg(&sgx_reset_forced, 1, 0);

	if (bLockup) {
		struct SGXMKIF_HOST_CTL __iomem *psSGXHostCtl =
						psDevInfo->psSGXHostCtl;
		u32 l;

		pvr_dev_lock();
		l = readl(&psSGXHostCtl->ui32HostDetectedLockups);
		l++;
		writel(l, &psSGXHostCtl->ui32HostDetectedLockups);
		pvr_dev_unlock();

		HWRecoveryResetSGX(psDeviceNode);
	}

	queue_delayed_work(data->work_queue, &data->work,
			   msecs_to_jiffies(data->interval));

	pvr_unlock();
}

struct timer_work_data *
SGXOSTimerInit(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	struct timer_work_data *data;

	data = kzalloc(sizeof(struct timer_work_data), GFP_KERNEL);
	if (!data)
		return NULL;

	data->work_queue = create_workqueue("SGXOSTimer");
	if (!data->work_queue) {
		kfree(data);
		return NULL;
	}

	data->interval = 150;
	data->psDeviceNode = psDeviceNode;
	INIT_DELAYED_WORK(&data->work, SGXOSTimer);

	return data;
}

void SGXOSTimerDeInit(struct timer_work_data *data)
{
	data->armed = false;
	destroy_workqueue(data->work_queue);
	kfree(data);
}

enum PVRSRV_ERROR SGXOSTimerEnable(struct timer_work_data *data)
{
	if (!data)
		return PVRSRV_ERROR_GENERIC;

	if (queue_delayed_work(data->work_queue, &data->work,
			       msecs_to_jiffies(data->interval))) {
		data->armed = true;
		return PVRSRV_OK;
	}

	return PVRSRV_ERROR_GENERIC;
}

enum PVRSRV_ERROR SGXOSTimerCancel(struct timer_work_data *data)
{
	if (!data)
		return PVRSRV_ERROR_GENERIC;

	data->armed = false;
	cancel_delayed_work(&data->work);

	return PVRSRV_OK;
}

int sgx_force_reset(void)
{
	return !cmpxchg(&sgx_reset_forced, 0, 1);
}

static IMG_BOOL SGX_ISRHandler(void *pvData)
{
	IMG_BOOL bInterruptProcessed = IMG_FALSE;

	{
		u32 ui32EventStatus, ui32EventEnable;
		u32 ui32EventClear = 0;
		struct PVRSRV_DEVICE_NODE *psDeviceNode;
		struct PVRSRV_SGXDEV_INFO *psDevInfo;

		if (pvData == NULL) {
			PVR_DPF(PVR_DBG_ERROR,
				 "SGX_ISRHandler: Invalid params\n");
			return bInterruptProcessed;
		}

		psDeviceNode = (struct PVRSRV_DEVICE_NODE *)pvData;
		psDevInfo = (struct PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;

		ui32EventStatus =
		    OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_STATUS);
		ui32EventEnable = OSReadHWReg(psDevInfo->pvRegsBaseKM,
						EUR_CR_EVENT_HOST_ENABLE);

		gui32EventStatusServicesByISR = ui32EventStatus;

		ui32EventStatus &= ui32EventEnable;

		if (ui32EventStatus & EUR_CR_EVENT_STATUS_SW_EVENT_MASK)
			ui32EventClear |= EUR_CR_EVENT_HOST_CLEAR_SW_EVENT_MASK;

		if (ui32EventClear) {
			bInterruptProcessed = IMG_TRUE;

			ui32EventClear |=
			    EUR_CR_EVENT_HOST_CLEAR_MASTER_INTERRUPT_MASK;

			OSWriteHWReg(psDevInfo->pvRegsBaseKM,
				     EUR_CR_EVENT_HOST_CLEAR, ui32EventClear);
		}
	}

	return bInterruptProcessed;
}

static void SGX_MISRHandler(void *pvData)
{
	struct PVRSRV_DEVICE_NODE *psDeviceNode =
			(struct PVRSRV_DEVICE_NODE *)pvData;
	struct PVRSRV_SGXDEV_INFO *psDevInfo =
			(struct PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;
	struct SGXMKIF_HOST_CTL __iomem *psSGXHostCtl =
			psDevInfo->psSGXHostCtl;
	u32 l1, l2;
	int dev_idx;
	enum PVRSRV_ERROR err;

	dev_idx = psDeviceNode->sDevId.ui32DeviceIndex;

	pvr_dev_lock();

	err = PVRSRVSetDevicePowerStateKM(dev_idx, PVRSRV_POWER_STATE_D0);
	BUG_ON(err != PVRSRV_OK);

	l1 = readl(&psSGXHostCtl->ui32InterruptFlags);
	l2 = readl(&psSGXHostCtl->ui32InterruptClearFlags);

	if ((l1 & PVRSRV_USSE_EDM_INTERRUPT_HWR) &&
	    !(l2 & PVRSRV_USSE_EDM_INTERRUPT_HWR))
		HWRecoveryResetSGX(psDeviceNode);

	if (psDeviceNode->bReProcessDeviceCommandComplete)
		SGXScheduleProcessQueues(psDeviceNode);

	SGXTestActivePowerEvent(psDeviceNode);

	pvr_dev_unlock();
}

enum PVRSRV_ERROR SGXRegisterDevice(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	struct DEVICE_MEMORY_INFO *psDevMemoryInfo;
	struct DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap;

	psDeviceNode->sDevId.eDeviceType = DEV_DEVICE_TYPE;
	psDeviceNode->sDevId.eDeviceClass = DEV_DEVICE_CLASS;

	psDeviceNode->pfnInitDevice = DevInitSGXPart1;
	psDeviceNode->pfnDeInitDevice = DevDeInitSGX;

	psDeviceNode->pfnInitDeviceCompatCheck = SGXDevInitCompatCheck;

	psDeviceNode->pfnMMUInitialise = MMU_Initialise;
	psDeviceNode->pfnMMUFinalise = MMU_Finalise;
	psDeviceNode->pfnMMUInsertHeap = MMU_InsertHeap;
	psDeviceNode->pfnMMUCreate = MMU_Create;
	psDeviceNode->pfnMMUDelete = MMU_Delete;
	psDeviceNode->pfnMMUAlloc = MMU_Alloc;
	psDeviceNode->pfnMMUFree = MMU_Free;
	psDeviceNode->pfnMMUMapPages = MMU_MapPages;
	psDeviceNode->pfnMMUMapShadow = MMU_MapShadow;
	psDeviceNode->pfnMMUUnmapPages = MMU_UnmapPages;
	psDeviceNode->pfnMMUMapScatter = MMU_MapScatter;
	psDeviceNode->pfnMMUGetPhysPageAddr = MMU_GetPhysPageAddr;
	psDeviceNode->pfnMMUGetPDDevPAddr = MMU_GetPDDevPAddr;

	psDeviceNode->pfnDeviceISR = SGX_ISRHandler;
	psDeviceNode->pfnDeviceMISR = SGX_MISRHandler;

	psDeviceNode->pfnDeviceCommandComplete = SGXCommandComplete;

	psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;

	psDevMemoryInfo->ui32AddressSpaceSizeLog2 =
	    SGX_FEATURE_ADDRESS_SPACE_SIZE;

	psDevMemoryInfo->ui32Flags = 0;
	psDevMemoryInfo->ui32HeapCount = SGX_MAX_HEAP_ID;
	psDevMemoryInfo->ui32SyncHeapID = SGX_SYNCINFO_HEAP_ID;

	psDevMemoryInfo->ui32MappingHeapID = SGX_GENERAL_HEAP_ID;

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(struct DEVICE_MEMORY_HEAP_INFO) *
		       psDevMemoryInfo->ui32HeapCount,
		       (void **) &psDevMemoryInfo->psDeviceMemoryHeap,
		       NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "SGXRegisterDevice : "
				"Failed to alloc memory for "
				"struct DEVICE_MEMORY_HEAP_INFO");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}
	OSMemSet(psDevMemoryInfo->psDeviceMemoryHeap, 0,
		 sizeof(struct DEVICE_MEMORY_HEAP_INFO) *
		 psDevMemoryInfo->ui32HeapCount);

	psDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;

	psDeviceMemoryHeap[SGX_GENERAL_HEAP_ID].ui32HeapID =
	    HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_GENERAL_HEAP_ID);
	psDeviceMemoryHeap[SGX_GENERAL_HEAP_ID].sDevVAddrBase.uiAddr =
	    SGX_GENERAL_HEAP_BASE;
	psDeviceMemoryHeap[SGX_GENERAL_HEAP_ID].ui32HeapSize =
	    SGX_GENERAL_HEAP_SIZE;
	psDeviceMemoryHeap[SGX_GENERAL_HEAP_ID].ui32Attribs =
	    PVRSRV_HAP_WRITECOMBINE | PVRSRV_MEM_RAM_BACKED_ALLOCATION |
	    PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap[SGX_GENERAL_HEAP_ID].pszName = "General";
	psDeviceMemoryHeap[SGX_GENERAL_HEAP_ID].pszBSName = "General BS";
	psDeviceMemoryHeap[SGX_GENERAL_HEAP_ID].DevMemHeapType =
	    DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap[SGX_GENERAL_HEAP_ID].ui32DataPageSize =
	    SGX_MMU_PAGE_SIZE;

	psDeviceMemoryHeap[SGX_TADATA_HEAP_ID].ui32HeapID =
	    HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_TADATA_HEAP_ID);
	psDeviceMemoryHeap[SGX_TADATA_HEAP_ID].sDevVAddrBase.uiAddr =
	    SGX_TADATA_HEAP_BASE;
	psDeviceMemoryHeap[SGX_TADATA_HEAP_ID].ui32HeapSize =
	    SGX_TADATA_HEAP_SIZE;
	psDeviceMemoryHeap[SGX_TADATA_HEAP_ID].ui32Attribs =
	    PVRSRV_HAP_WRITECOMBINE | PVRSRV_MEM_RAM_BACKED_ALLOCATION
	    | PVRSRV_HAP_MULTI_PROCESS;
	psDeviceMemoryHeap[SGX_TADATA_HEAP_ID].pszName = "TA Data";
	psDeviceMemoryHeap[SGX_TADATA_HEAP_ID].pszBSName = "TA Data BS";
	psDeviceMemoryHeap[SGX_TADATA_HEAP_ID].DevMemHeapType =
	    DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap[SGX_TADATA_HEAP_ID].ui32DataPageSize =
	    SGX_MMU_PAGE_SIZE;

	psDeviceMemoryHeap[SGX_KERNEL_CODE_HEAP_ID].ui32HeapID =
	    HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_KERNEL_CODE_HEAP_ID);
	psDeviceMemoryHeap[SGX_KERNEL_CODE_HEAP_ID].sDevVAddrBase.uiAddr =
	    SGX_KERNEL_CODE_HEAP_BASE;
	psDeviceMemoryHeap[SGX_KERNEL_CODE_HEAP_ID].ui32HeapSize =
	    SGX_KERNEL_CODE_HEAP_SIZE;
	psDeviceMemoryHeap[SGX_KERNEL_CODE_HEAP_ID].ui32Attribs =
	    PVRSRV_HAP_WRITECOMBINE | PVRSRV_MEM_RAM_BACKED_ALLOCATION |
	    PVRSRV_HAP_MULTI_PROCESS;
	psDeviceMemoryHeap[SGX_KERNEL_CODE_HEAP_ID].pszName = "Kernel Code";
	psDeviceMemoryHeap[SGX_KERNEL_CODE_HEAP_ID].pszBSName =
	    "Kernel Code BS";
	psDeviceMemoryHeap[SGX_KERNEL_CODE_HEAP_ID].DevMemHeapType =
	    DEVICE_MEMORY_HEAP_SHARED_EXPORTED;

	psDeviceMemoryHeap[SGX_KERNEL_CODE_HEAP_ID].ui32DataPageSize =
	    SGX_MMU_PAGE_SIZE;

	psDeviceMemoryHeap[SGX_KERNEL_DATA_HEAP_ID].ui32HeapID =
	    HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_KERNEL_DATA_HEAP_ID);
	psDeviceMemoryHeap[SGX_KERNEL_DATA_HEAP_ID].sDevVAddrBase.uiAddr =
	    SGX_KERNEL_DATA_HEAP_BASE;
	psDeviceMemoryHeap[SGX_KERNEL_DATA_HEAP_ID].ui32HeapSize =
	    SGX_KERNEL_DATA_HEAP_SIZE;
	psDeviceMemoryHeap[SGX_KERNEL_DATA_HEAP_ID].ui32Attribs =
	    PVRSRV_HAP_WRITECOMBINE | PVRSRV_MEM_RAM_BACKED_ALLOCATION |
	    PVRSRV_HAP_MULTI_PROCESS;
	psDeviceMemoryHeap[SGX_KERNEL_DATA_HEAP_ID].pszName = "KernelData";
	psDeviceMemoryHeap[SGX_KERNEL_DATA_HEAP_ID].pszBSName = "KernelData BS";
	psDeviceMemoryHeap[SGX_KERNEL_DATA_HEAP_ID].DevMemHeapType =
	    DEVICE_MEMORY_HEAP_SHARED_EXPORTED;

	psDeviceMemoryHeap[SGX_KERNEL_DATA_HEAP_ID].ui32DataPageSize =
	    SGX_MMU_PAGE_SIZE;

	psDeviceMemoryHeap[SGX_PIXELSHADER_HEAP_ID].ui32HeapID =
	    HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_PIXELSHADER_HEAP_ID);
	psDeviceMemoryHeap[SGX_PIXELSHADER_HEAP_ID].sDevVAddrBase.uiAddr =
	    SGX_PIXELSHADER_HEAP_BASE;
	psDeviceMemoryHeap[SGX_PIXELSHADER_HEAP_ID].ui32HeapSize =
	    SGX_PIXELSHADER_HEAP_SIZE;
	psDeviceMemoryHeap[SGX_PIXELSHADER_HEAP_ID].ui32Attribs =
	    PVRSRV_HAP_WRITECOMBINE | PVRSRV_MEM_RAM_BACKED_ALLOCATION |
	    PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap[SGX_PIXELSHADER_HEAP_ID].pszName = "PixelShaderUSSE";
	psDeviceMemoryHeap[SGX_PIXELSHADER_HEAP_ID].pszBSName =
	    "PixelShaderUSSE BS";
	psDeviceMemoryHeap[SGX_PIXELSHADER_HEAP_ID].DevMemHeapType =
	    DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap[SGX_PIXELSHADER_HEAP_ID].ui32DataPageSize =
	    SGX_MMU_PAGE_SIZE;

	psDeviceMemoryHeap[SGX_VERTEXSHADER_HEAP_ID].ui32HeapID =
	    HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_VERTEXSHADER_HEAP_ID);
	psDeviceMemoryHeap[SGX_VERTEXSHADER_HEAP_ID].sDevVAddrBase.uiAddr =
	    SGX_VERTEXSHADER_HEAP_BASE;
	psDeviceMemoryHeap[SGX_VERTEXSHADER_HEAP_ID].ui32HeapSize =
	    SGX_VERTEXSHADER_HEAP_SIZE;
	psDeviceMemoryHeap[SGX_VERTEXSHADER_HEAP_ID].ui32Attribs =
	    PVRSRV_HAP_WRITECOMBINE | PVRSRV_MEM_RAM_BACKED_ALLOCATION |
	    PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap[SGX_VERTEXSHADER_HEAP_ID].pszName =
	    "VertexShaderUSSE";
	psDeviceMemoryHeap[SGX_VERTEXSHADER_HEAP_ID].pszBSName =
	    "VertexShaderUSSE BS";
	psDeviceMemoryHeap[SGX_VERTEXSHADER_HEAP_ID].DevMemHeapType =
	    DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap[SGX_VERTEXSHADER_HEAP_ID].ui32DataPageSize =
	    SGX_MMU_PAGE_SIZE;

	psDeviceMemoryHeap[SGX_PDSPIXEL_CODEDATA_HEAP_ID].ui32HeapID =
	    HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_PDSPIXEL_CODEDATA_HEAP_ID);
	psDeviceMemoryHeap[SGX_PDSPIXEL_CODEDATA_HEAP_ID].sDevVAddrBase.uiAddr =
	    SGX_PDSPIXEL_CODEDATA_HEAP_BASE;
	psDeviceMemoryHeap[SGX_PDSPIXEL_CODEDATA_HEAP_ID].ui32HeapSize =
	    SGX_PDSPIXEL_CODEDATA_HEAP_SIZE;
	psDeviceMemoryHeap[SGX_PDSPIXEL_CODEDATA_HEAP_ID].ui32Attribs =
	    PVRSRV_HAP_WRITECOMBINE | PVRSRV_MEM_RAM_BACKED_ALLOCATION |
	    PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap[SGX_PDSPIXEL_CODEDATA_HEAP_ID].pszName =
	    "PDSPixelCodeData";
	psDeviceMemoryHeap[SGX_PDSPIXEL_CODEDATA_HEAP_ID].pszBSName =
	    "PDSPixelCodeData BS";
	psDeviceMemoryHeap[SGX_PDSPIXEL_CODEDATA_HEAP_ID].DevMemHeapType =
	    DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap[SGX_PDSPIXEL_CODEDATA_HEAP_ID].ui32DataPageSize =
	    SGX_MMU_PAGE_SIZE;

	psDeviceMemoryHeap[SGX_PDSVERTEX_CODEDATA_HEAP_ID].ui32HeapID =
	    HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_PDSVERTEX_CODEDATA_HEAP_ID);
	psDeviceMemoryHeap[SGX_PDSVERTEX_CODEDATA_HEAP_ID].sDevVAddrBase.
	    uiAddr = SGX_PDSVERTEX_CODEDATA_HEAP_BASE;
	psDeviceMemoryHeap[SGX_PDSVERTEX_CODEDATA_HEAP_ID].ui32HeapSize =
	    SGX_PDSVERTEX_CODEDATA_HEAP_SIZE;
	psDeviceMemoryHeap[SGX_PDSVERTEX_CODEDATA_HEAP_ID].ui32Attribs =
	    PVRSRV_HAP_WRITECOMBINE | PVRSRV_MEM_RAM_BACKED_ALLOCATION |
	    PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap[SGX_PDSVERTEX_CODEDATA_HEAP_ID].pszName =
	    "PDSVertexCodeData";
	psDeviceMemoryHeap[SGX_PDSVERTEX_CODEDATA_HEAP_ID].pszBSName =
	    "PDSVertexCodeData BS";
	psDeviceMemoryHeap[SGX_PDSVERTEX_CODEDATA_HEAP_ID].DevMemHeapType =
	    DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap[SGX_PDSVERTEX_CODEDATA_HEAP_ID].ui32DataPageSize =
	    SGX_MMU_PAGE_SIZE;

	psDeviceMemoryHeap[SGX_SYNCINFO_HEAP_ID].ui32HeapID =
	    HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_SYNCINFO_HEAP_ID);
	psDeviceMemoryHeap[SGX_SYNCINFO_HEAP_ID].sDevVAddrBase.uiAddr =
	    SGX_SYNCINFO_HEAP_BASE;
	psDeviceMemoryHeap[SGX_SYNCINFO_HEAP_ID].ui32HeapSize =
	    SGX_SYNCINFO_HEAP_SIZE;

	psDeviceMemoryHeap[SGX_SYNCINFO_HEAP_ID].ui32Attribs =
	    PVRSRV_HAP_WRITECOMBINE | PVRSRV_MEM_RAM_BACKED_ALLOCATION |
	    PVRSRV_HAP_MULTI_PROCESS;
	psDeviceMemoryHeap[SGX_SYNCINFO_HEAP_ID].pszName = "CacheCoherent";
	psDeviceMemoryHeap[SGX_SYNCINFO_HEAP_ID].pszBSName = "CacheCoherent BS";

	psDeviceMemoryHeap[SGX_SYNCINFO_HEAP_ID].DevMemHeapType =
	    DEVICE_MEMORY_HEAP_SHARED_EXPORTED;

	psDeviceMemoryHeap[SGX_SYNCINFO_HEAP_ID].ui32DataPageSize =
	    SGX_MMU_PAGE_SIZE;

	psDeviceMemoryHeap[SGX_3DPARAMETERS_HEAP_ID].ui32HeapID =
	    HEAP_ID(PVRSRV_DEVICE_TYPE_SGX, SGX_3DPARAMETERS_HEAP_ID);
	psDeviceMemoryHeap[SGX_3DPARAMETERS_HEAP_ID].sDevVAddrBase.uiAddr =
	    SGX_3DPARAMETERS_HEAP_BASE;
	psDeviceMemoryHeap[SGX_3DPARAMETERS_HEAP_ID].ui32HeapSize =
	    SGX_3DPARAMETERS_HEAP_SIZE;
	psDeviceMemoryHeap[SGX_3DPARAMETERS_HEAP_ID].pszName = "3DParameters";
	psDeviceMemoryHeap[SGX_3DPARAMETERS_HEAP_ID].pszBSName =
	    "3DParameters BS";
	psDeviceMemoryHeap[SGX_3DPARAMETERS_HEAP_ID].ui32Attribs =
	    PVRSRV_HAP_WRITECOMBINE | PVRSRV_MEM_RAM_BACKED_ALLOCATION |
	    PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap[SGX_3DPARAMETERS_HEAP_ID].DevMemHeapType =
	    DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap[SGX_3DPARAMETERS_HEAP_ID].ui32DataPageSize =
	    SGX_MMU_PAGE_SIZE;

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SGXGetClientInfoKM(void *hDevCookie,
					 struct SGX_CLIENT_INFO *psClientInfo)
{
	struct PVRSRV_SGXDEV_INFO *psDevInfo =
	    (struct PVRSRV_SGXDEV_INFO *)
			((struct PVRSRV_DEVICE_NODE *)hDevCookie)->pvDevice;

	psDevInfo->ui32ClientRefCount++;
#ifdef PDUMP
	if (psDevInfo->ui32ClientRefCount == 1)
		psDevInfo->psKernelCCBInfo->ui32CCBDumpWOff = 0;
#endif
	psClientInfo->ui32ProcessID = OSGetCurrentProcessIDKM();

	OSMemCopy(&psClientInfo->asDevData, &psDevInfo->asSGXDevData,
		  sizeof(psClientInfo->asDevData));

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SGXDevInitCompatCheck(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	struct PVRSRV_SGXDEV_INFO *psDevInfo;
	struct PVRSRV_KERNEL_MEM_INFO *psMemInfo;
	enum PVRSRV_ERROR eError;
#if !defined(NO_HARDWARE)
	u32 ui32BuildOptions, ui32BuildOptionsMismatch;
	struct PVRSRV_SGX_MISCINFO_FEATURES *psSGXFeatures;
#endif

	if (psDeviceNode->sDevId.eDeviceType != PVRSRV_DEVICE_TYPE_SGX) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SGXDevInitCompatCheck: Device not of type SGX");
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto exit;
	}
	psDevInfo = psDeviceNode->pvDevice;
	psMemInfo = psDevInfo->psKernelSGXMiscMemInfo;

#if !defined(NO_HARDWARE)

	eError = SGXGetBuildInfoKM(psDevInfo, psDeviceNode);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "SGXDevInitCompatCheck: "
				"Unable to validate device DDK version");
		goto exit;
	}
	psSGXFeatures =
	    &((struct PVRSRV_SGX_MISCINFO_INFO *)(psMemInfo->pvLinAddrKM))->
							    sSGXFeatures;
	if ((psSGXFeatures->ui32DDKVersion !=
	     ((PVRVERSION_MAJ << 16) | (PVRVERSION_MIN << 8) |
	      PVRVERSION_BRANCH)) ||
	     (psSGXFeatures->ui32DDKBuild != PVRVERSION_BUILD)) {
		PVR_DPF(PVR_DBG_ERROR, "SGXDevInitCompatCheck: "
			"Incompatible driver DDK revision (%ld)"
			"/device DDK revision (%ld).",
			 PVRVERSION_BUILD, psSGXFeatures->ui32DDKBuild);
		eError = PVRSRV_ERROR_DDK_VERSION_MISMATCH;
		goto exit;
	} else {
		PVR_DPF(PVR_DBG_WARNING, "(Success) SGXInit: "
				"driver DDK (%ld) and device DDK (%ld) match",
			 PVRVERSION_BUILD, psSGXFeatures->ui32DDKBuild);
	}

	ui32BuildOptions = psSGXFeatures->ui32BuildOptions;
	if (ui32BuildOptions != (SGX_BUILD_OPTIONS)) {
		ui32BuildOptionsMismatch =
		    ui32BuildOptions ^ (SGX_BUILD_OPTIONS);
		if (((SGX_BUILD_OPTIONS) & ui32BuildOptionsMismatch) != 0)
			PVR_DPF(PVR_DBG_ERROR, "SGXInit: "
				"Mismatch in driver and microkernel build "
				"options; extra options present in driver: "
				"(0x%lx)",
				 (SGX_BUILD_OPTIONS) &
				 ui32BuildOptionsMismatch);

		if ((ui32BuildOptions & ui32BuildOptionsMismatch) != 0)
			PVR_DPF(PVR_DBG_ERROR, "SGXInit: "
				"Mismatch in driver and microkernel build "
				"options; extra options present in "
				"microkernel: (0x%lx)",
				 ui32BuildOptions & ui32BuildOptionsMismatch);
		eError = PVRSRV_ERROR_BUILD_MISMATCH;
		goto exit;
	} else {
		PVR_DPF(PVR_DBG_WARNING, "(Success) SGXInit: "
				"Driver and microkernel build options match.");
	}

#endif
	eError = PVRSRV_OK;
exit:
	return eError;
}

static
enum PVRSRV_ERROR SGXGetBuildInfoKM(struct PVRSRV_SGXDEV_INFO *psDevInfo,
				    struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	enum PVRSRV_ERROR eError;
	struct SGXMKIF_COMMAND sCommandData;
	struct PVRSRV_SGX_MISCINFO_INFO *psSGXMiscInfoInt;
	struct PVRSRV_SGX_MISCINFO_FEATURES *psSGXFeatures;

	struct PVRSRV_KERNEL_MEM_INFO *psMemInfo =
	    psDevInfo->psKernelSGXMiscMemInfo;

	if (!psMemInfo->pvLinAddrKM) {
		PVR_DPF(PVR_DBG_ERROR, "SGXGetMiscInfoKM: Invalid address.");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	psSGXMiscInfoInt = psMemInfo->pvLinAddrKM;
	psSGXMiscInfoInt->ui32MiscInfoFlags &= ~PVRSRV_USSE_MISCINFO_READY;
	psSGXFeatures = &psSGXMiscInfoInt->sSGXFeatures;

	OSMemSet(psMemInfo->pvLinAddrKM, 0,
		 sizeof(struct PVRSRV_SGX_MISCINFO_INFO));

	sCommandData.ui32Data[1] = psMemInfo->sDevVAddr.uiAddr;

	OSMemSet(psSGXFeatures, 0, sizeof(*psSGXFeatures));

	mb();

	eError = SGXScheduleCCBCommandKM(psDeviceNode,
					 SGXMKIF_COMMAND_REQUEST_SGXMISCINFO,
					 &sCommandData, KERNEL_ID, 0);

	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SGXGetMiscInfoKM: SGXScheduleCCBCommandKM failed.");
		return eError;
	}

#if !defined(NO_HARDWARE)
	{
		IMG_BOOL bTimeout = IMG_TRUE;

		LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US) {
			if ((psSGXMiscInfoInt->
			     ui32MiscInfoFlags & PVRSRV_USSE_MISCINFO_READY) !=
			    0) {
				bTimeout = IMG_FALSE;
				break;
			}
		}
		END_LOOP_UNTIL_TIMEOUT();

		if (bTimeout)
			return PVRSRV_ERROR_TIMEOUT;
	}
#endif

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SGXGetMiscInfoKM(struct PVRSRV_SGXDEV_INFO *psDevInfo,
				       struct SGX_MISC_INFO *psMiscInfo,
				       struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	switch (psMiscInfo->eRequest) {
	case SGX_MISC_INFO_REQUEST_CLOCKSPEED:
		{
			psMiscInfo->uData.ui32SGXClockSpeed =
			    psDevInfo->ui32CoreClockSpeed;
			return PVRSRV_OK;
		}

	case SGX_MISC_INFO_REQUEST_SGXREV:
		{
			struct PVRSRV_SGX_MISCINFO_FEATURES *psSGXFeatures;
			struct PVRSRV_KERNEL_MEM_INFO *psMemInfo =
			    psDevInfo->psKernelSGXMiscMemInfo;

			SGXGetBuildInfoKM(psDevInfo, psDeviceNode);
			psSGXFeatures =
			    &((struct PVRSRV_SGX_MISCINFO_INFO *)(psMemInfo->
						  pvLinAddrKM))->sSGXFeatures;

			psMiscInfo->uData.sSGXFeatures = *psSGXFeatures;

			PVR_DPF(PVR_DBG_MESSAGE, "SGXGetMiscInfoKM: "
					"Core 0x%lx, sw ID 0x%lx, "
					"sw Rev 0x%lx\n",
				 psSGXFeatures->ui32CoreRev,
				 psSGXFeatures->ui32CoreIdSW,
				 psSGXFeatures->ui32CoreRevSW);
			PVR_DPF(PVR_DBG_MESSAGE, "SGXGetMiscInfoKM: "
					"DDK version 0x%lx, DDK build 0x%lx\n",
				 psSGXFeatures->ui32DDKVersion,
				 psSGXFeatures->ui32DDKBuild);

			return PVRSRV_OK;
		}

	case SGX_MISC_INFO_REQUEST_DRIVER_SGXREV:
		{
			struct PVRSRV_KERNEL_MEM_INFO *psMemInfo =
			    psDevInfo->psKernelSGXMiscMemInfo;
			struct PVRSRV_SGX_MISCINFO_FEATURES *psSGXFeatures;

			psSGXFeatures = &((struct PVRSRV_SGX_MISCINFO_INFO *)(
					psMemInfo->pvLinAddrKM))->sSGXFeatures;

			OSMemSet(psMemInfo->pvLinAddrKM, 0,
				 sizeof(struct PVRSRV_SGX_MISCINFO_INFO));

			psSGXFeatures->ui32DDKVersion =
			    (PVRVERSION_MAJ << 16) |
			    (PVRVERSION_MIN << 8) | PVRVERSION_BRANCH;
			psSGXFeatures->ui32DDKBuild = PVRVERSION_BUILD;

			psMiscInfo->uData.sSGXFeatures = *psSGXFeatures;
			return PVRSRV_OK;
		}

	case SGX_MISC_INFO_REQUEST_SET_HWPERF_STATUS:
		{
			struct SGXMKIF_HWPERF_CB *psHWPerfCB =
			    psDevInfo->psKernelHWPerfCBMemInfo->pvLinAddrKM;
			unsigned ui32MatchingFlags;

			if ((psMiscInfo->uData.ui32NewHWPerfStatus &
			     ~(PVRSRV_SGX_HWPERF_GRAPHICS_ON |
			       PVRSRV_SGX_HWPERF_MK_EXECUTION_ON)) != 0) {
				return PVRSRV_ERROR_INVALID_PARAMS;
			}

			pvr_dev_lock();
			ui32MatchingFlags = readl(&psDevInfo->
						 psSGXHostCtl->ui32HWPerfFlags);
			ui32MatchingFlags &=
				psMiscInfo->uData.ui32NewHWPerfStatus;
			if ((ui32MatchingFlags & PVRSRV_SGX_HWPERF_GRAPHICS_ON)
			    == 0UL) {
				psHWPerfCB->ui32OrdinalGRAPHICS = 0xffffffff;
			}
			if ((ui32MatchingFlags &
			     PVRSRV_SGX_HWPERF_MK_EXECUTION_ON) == 0UL) {
				psHWPerfCB->ui32OrdinalMK_EXECUTION =
				    0xffffffffUL;
			}


			writel(psMiscInfo->uData.ui32NewHWPerfStatus,
				&psDevInfo->psSGXHostCtl->ui32HWPerfFlags);
			pvr_dev_unlock();
#if defined(PDUMP)
			PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS,
					      "SGX ukernel HWPerf status %u\n",
					      readl(&psDevInfo->psSGXHostCtl->
							      ui32HWPerfFlags));
			PDUMPMEM(NULL, psDevInfo->psKernelSGXHostCtlMemInfo,
				 offsetof(struct SGXMKIF_HOST_CTL,
					  ui32HWPerfFlags),
				 sizeof(psDevInfo->psSGXHostCtl->
					ui32HWPerfFlags),
				 PDUMP_FLAGS_CONTINUOUS,
				 MAKEUNIQUETAG(psDevInfo->
					       psKernelSGXHostCtlMemInfo));
#endif

			return PVRSRV_OK;
		}
	case SGX_MISC_INFO_REQUEST_HWPERF_CB_ON:
		{

			struct SGXMKIF_HWPERF_CB *psHWPerfCB =
			    psDevInfo->psKernelHWPerfCBMemInfo->pvLinAddrKM;
			u32 l;

			psHWPerfCB->ui32OrdinalGRAPHICS = 0xffffffffUL;

			pvr_dev_lock();;
			l = readl(&psDevInfo->psSGXHostCtl->ui32HWPerfFlags);
			l |= PVRSRV_SGX_HWPERF_GRAPHICS_ON;
			writel(l, &psDevInfo->psSGXHostCtl->ui32HWPerfFlags);
			pvr_dev_unlock();

			return PVRSRV_OK;
		}
	case SGX_MISC_INFO_REQUEST_HWPERF_CB_OFF:
		{
			pvr_dev_lock();
			writel(0, &psDevInfo->psSGXHostCtl->ui32HWPerfFlags);
			pvr_dev_unlock();

			return PVRSRV_OK;
		}
	case SGX_MISC_INFO_REQUEST_HWPERF_RETRIEVE_CB:
		{
			struct SGX_MISC_INFO_HWPERF_RETRIEVE_CB *psRetrieve =
			    &psMiscInfo->uData.sRetrieveCB;
			struct SGXMKIF_HWPERF_CB *psHWPerfCB =
			    psDevInfo->psKernelHWPerfCBMemInfo->pvLinAddrKM;
			unsigned i;

			for (i = 0;
			     psHWPerfCB->ui32Woff != psHWPerfCB->ui32Roff
			     && i < psRetrieve->ui32ArraySize; i++) {
				struct SGXMKIF_HWPERF_CB_ENTRY *psData =
				    &psHWPerfCB->psHWPerfCBData[psHWPerfCB->
								ui32Roff];

				psRetrieve->psHWPerfData[i].ui32FrameNo =
				    psData->ui32FrameNo;
				psRetrieve->psHWPerfData[i].ui32Type =
				    (psData->ui32Type &
				     PVRSRV_SGX_HWPERF_TYPE_OP_MASK);
				psRetrieve->psHWPerfData[i].ui32StartTime =
				    psData->ui32Time;
				psRetrieve->psHWPerfData[i].ui32StartTimeWraps =
				    psData->ui32TimeWraps;
				psRetrieve->psHWPerfData[i].ui32EndTime =
				    psData->ui32Time;
				psRetrieve->psHWPerfData[i].ui32EndTimeWraps =
				    psData->ui32TimeWraps;
				psRetrieve->psHWPerfData[i].ui32ClockSpeed =
				    psDevInfo->ui32CoreClockSpeed;
				psRetrieve->psHWPerfData[i].ui32TimeMax =
				    psDevInfo->ui32uKernelTimerClock;
				psHWPerfCB->ui32Roff =
				    (psHWPerfCB->ui32Roff + 1) &
				    (SGXMKIF_HWPERF_CB_SIZE - 1);
			}
			psRetrieve->ui32DataCount = i;
			psRetrieve->ui32Time = OSClockus();
			return PVRSRV_OK;
		}
	default:
		{
			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}
}

enum PVRSRV_ERROR SGXReadDiffCountersKM(void *hDevHandle, u32 ui32Reg,
				   u32 *pui32Old, IMG_BOOL bNew, u32 ui32New,
				   u32 ui32NewReset, u32 ui32CountersReg,
				   u32 *pui32Time, IMG_BOOL *pbActive,
				   struct PVRSRV_SGXDEV_DIFF_INFO *psDiffs)
{
	struct SYS_DATA *psSysData;
	struct PVRSRV_POWER_DEV *psPowerDevice;
	IMG_BOOL bPowered = IMG_FALSE;
	struct PVRSRV_DEVICE_NODE *psDeviceNode = hDevHandle;
	struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	if (bNew)
		psDevInfo->ui32HWGroupRequested = ui32New;
	psDevInfo->ui32HWReset |= ui32NewReset;

	SysAcquireData(&psSysData);

	psPowerDevice = psSysData->psPowerDeviceList;
	while (psPowerDevice) {
		if (psPowerDevice->ui32DeviceIndex ==
		    psDeviceNode->sDevId.ui32DeviceIndex) {
			bPowered =
			    (IMG_BOOL)(psPowerDevice->eCurrentPowerState ==
					PVRSRV_POWER_STATE_D0);
			break;
		}

		psPowerDevice = psPowerDevice->psNext;
	}

	*pbActive = bPowered;

	pvr_dev_lock();

	{
		struct PVRSRV_SGXDEV_DIFF_INFO sNew,
					       *psPrev = &psDevInfo->sDiffInfo;
		u32 i;

		sNew.ui32Time[0] = OSClockus();
		*pui32Time = sNew.ui32Time[0];
		if (sNew.ui32Time[0] != psPrev->ui32Time[0] && bPowered) {

			*pui32Old =
			    OSReadHWReg(psDevInfo->pvRegsBaseKM, ui32Reg);

			for (i = 0; i < PVRSRV_SGX_DIFF_NUM_COUNTERS; ++i) {
				sNew.aui32Counters[i] =
				    OSReadHWReg(psDevInfo->pvRegsBaseKM,
						ui32CountersReg + (i * 4));
			}

			if (psDevInfo->ui32HWGroupRequested != *pui32Old) {
				if (psDevInfo->ui32HWReset != 0) {
					OSWriteHWReg(psDevInfo->pvRegsBaseKM,
						     ui32Reg,
						     psDevInfo->
						     ui32HWGroupRequested |
						     psDevInfo->ui32HWReset);
					psDevInfo->ui32HWReset = 0;
				}
				OSWriteHWReg(psDevInfo->pvRegsBaseKM, ui32Reg,
					     psDevInfo->ui32HWGroupRequested);
			}

			sNew.ui32Marker[0] = psDevInfo->ui32KickTACounter;
			sNew.ui32Marker[1] = psDevInfo->ui32KickTARenderCounter;

			sNew.ui32Time[1] = readl(
				&psDevInfo->psSGXHostCtl->ui32TimeWraps);

			for (i = 0; i < PVRSRV_SGX_DIFF_NUM_COUNTERS; ++i) {
				psDiffs->aui32Counters[i] =
				    sNew.aui32Counters[i] -
				    psPrev->aui32Counters[i];
			}

			psDiffs->ui32Marker[0] =
			    sNew.ui32Marker[0] - psPrev->ui32Marker[0];
			psDiffs->ui32Marker[1] =
			    sNew.ui32Marker[1] - psPrev->ui32Marker[1];

			psDiffs->ui32Time[0] =
			    sNew.ui32Time[0] - psPrev->ui32Time[0];
			psDiffs->ui32Time[1] =
			    sNew.ui32Time[1] - psPrev->ui32Time[1];

			*psPrev = sNew;
		} else {
			for (i = 0; i < PVRSRV_SGX_DIFF_NUM_COUNTERS; ++i)
				psDiffs->aui32Counters[i] = 0;

			psDiffs->ui32Marker[0] = 0;
			psDiffs->ui32Marker[1] = 0;

			psDiffs->ui32Time[0] = 0;
			psDiffs->ui32Time[1] = 0;
		}
	}

	SGXTestActivePowerEvent(psDeviceNode);

	pvr_dev_unlock();

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SGXReadHWPerfCBKM(void *hDevHandle, u32 ui32ArraySize,
			struct PVRSRV_SGX_HWPERF_CB_ENTRY *psClientHWPerfEntry,
			u32 *pui32DataCount, u32 *pui32ClockSpeed,
			u32 *pui32HostTimeStamp)
{
	enum PVRSRV_ERROR eError = PVRSRV_OK;
	struct PVRSRV_DEVICE_NODE *psDeviceNode = hDevHandle;
	struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	struct SGXMKIF_HWPERF_CB *psHWPerfCB =
	    psDevInfo->psKernelHWPerfCBMemInfo->pvLinAddrKM;
	unsigned i;

	for (i = 0;
	     psHWPerfCB->ui32Woff != psHWPerfCB->ui32Roff && i < ui32ArraySize;
	     i++) {
		struct SGXMKIF_HWPERF_CB_ENTRY *psMKPerfEntry =
		    &psHWPerfCB->psHWPerfCBData[psHWPerfCB->ui32Roff];

		psClientHWPerfEntry[i].ui32FrameNo = psMKPerfEntry->ui32FrameNo;
		psClientHWPerfEntry[i].ui32Type = psMKPerfEntry->ui32Type;
		psClientHWPerfEntry[i].ui32Ordinal = psMKPerfEntry->ui32Ordinal;
		psClientHWPerfEntry[i].ui32Clocksx16 =
		    SGXConvertTimeStamp(psDevInfo, psMKPerfEntry->ui32TimeWraps,
					psMKPerfEntry->ui32Time);
		OSMemCopy(&psClientHWPerfEntry[i].ui32Counters[0],
			  &psMKPerfEntry->ui32Counters[0],
			  sizeof(psMKPerfEntry->ui32Counters));

		psHWPerfCB->ui32Roff =
		    (psHWPerfCB->ui32Roff + 1) & (SGXMKIF_HWPERF_CB_SIZE - 1);
	}

	*pui32DataCount = i;
	*pui32ClockSpeed = psDevInfo->ui32CoreClockSpeed;
	*pui32HostTimeStamp = OSClockus();

	return eError;
}
