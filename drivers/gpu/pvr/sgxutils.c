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

#include "sgxdefs.h"
#include "services_headers.h"
#include "buffer_manager.h"
#include "sgxapi_km.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "sysconfig.h"
#include "pdump_km.h"
#include "mmu.h"
#include "pvr_bridge_km.h"
#include "sgx_bridge_km.h"
#include "osfunc.h"
#include "pvr_debug.h"
#include "sgxutils.h"

#include <linux/tty.h>
#include <linux/io.h>

static void SGXPostActivePowerEvent(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	struct SGXMKIF_HOST_CTL __iomem *psSGXHostCtl =
					     psDevInfo->psSGXHostCtl;
	u32 l;

	l = readl(&psSGXHostCtl->ui32NumActivePowerEvents);
	l++;
	writel(l, &psSGXHostCtl->ui32NumActivePowerEvents);

	l = readl(&psSGXHostCtl->ui32PowerStatus);
	if (l & PVRSRV_USSE_EDM_POWMAN_POWEROFF_RESTART_IMMEDIATE)
		SGXScheduleProcessQueues(psDeviceNode);
}

void SGXTestActivePowerEvent(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	enum PVRSRV_ERROR eError = PVRSRV_OK;
	struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	struct SGXMKIF_HOST_CTL __iomem *psSGXHostCtl = psDevInfo->psSGXHostCtl;
	u32 l;

	l = readl(&psSGXHostCtl->ui32InterruptFlags);
	if (!(l & PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER))
		return;

	l = readl(&psSGXHostCtl->ui32InterruptClearFlags);
	if (l & PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER)
		return;

	/* Microkernel is idle and is requesting to be powered down. */
	l = readl(&psSGXHostCtl->ui32InterruptClearFlags);
	l |= PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER;
	writel(l, &psSGXHostCtl->ui32InterruptClearFlags);

	PDUMPSUSPEND();

	eError = PVRSRVSetDevicePowerStateKM(
					psDeviceNode->sDevId.ui32DeviceIndex,
					PVRSRV_POWER_STATE_D3);
	if (eError == PVRSRV_OK)
		SGXPostActivePowerEvent(psDeviceNode);

	PDUMPRESUME();

	if (eError != PVRSRV_OK)
		PVR_DPF(PVR_DBG_ERROR, "SGXTestActivePowerEvent error:%lu",
					eError);
}

static inline struct SGXMKIF_COMMAND *SGXAcquireKernelCCBSlot(
					struct PVRSRV_SGX_CCB_INFO *psCCB)
{
	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US) {
		if (((*psCCB->pui32WriteOffset + 1) & 255) !=
		    *psCCB->pui32ReadOffset) {
			return &psCCB->psCommands[*psCCB->pui32WriteOffset];
		}
	}
	END_LOOP_UNTIL_TIMEOUT();

	return NULL;
}

enum PVRSRV_ERROR SGXScheduleCCBCommand(struct PVRSRV_SGXDEV_INFO *psDevInfo,
					enum SGXMKIF_COMMAND_TYPE eCommandType,
					struct SGXMKIF_COMMAND *psCommandData,
					u32 ui32CallerID, u32 ui32PDumpFlags)
{
	struct PVRSRV_SGX_CCB_INFO *psKernelCCB;
	enum PVRSRV_ERROR eError = PVRSRV_OK;
	struct SGXMKIF_COMMAND *psSGXCommand;
#if defined(PDUMP)
	void *pvDumpCommand;
#else
	PVR_UNREFERENCED_PARAMETER(ui32CallerID);
	PVR_UNREFERENCED_PARAMETER(ui32PDumpFlags);
#endif

	psKernelCCB = psDevInfo->psKernelCCBInfo;

	psSGXCommand = SGXAcquireKernelCCBSlot(psKernelCCB);

	if (!psSGXCommand) {
		eError = PVRSRV_ERROR_TIMEOUT;
		goto Exit;
	}

	psCommandData->ui32Data[2] = psDevInfo->ui32CacheControl;

#if defined(PDUMP)

	psDevInfo->sPDContext.ui32CacheControl |= psDevInfo->ui32CacheControl;
#endif

	psDevInfo->ui32CacheControl = 0;

	*psSGXCommand = *psCommandData;

	switch (eCommandType) {
	case SGXMKIF_COMMAND_EDM_KICK:
		psSGXCommand->ui32ServiceAddress =
		    psDevInfo->ui32HostKickAddress;
		break;
	case SGXMKIF_COMMAND_REQUEST_SGXMISCINFO:
		psSGXCommand->ui32ServiceAddress =
		    psDevInfo->ui32GetMiscInfoAddress;
		break;
	case SGXMKIF_COMMAND_VIDEO_KICK:
	default:
		PVR_DPF(PVR_DBG_ERROR,
			 "SGXScheduleCCBCommandKM: Unknown command type: %d",
			 eCommandType);
		eError = PVRSRV_ERROR_GENERIC;
		goto Exit;
	}

#if defined(PDUMP)
	if (ui32CallerID != ISR_ID) {
		PDUMPCOMMENTWITHFLAGS(0,
				      "Poll for space in the Kernel CCB\r\n");
		PDUMPMEMPOL(psKernelCCB->psCCBCtlMemInfo,
			    offsetof(struct PVRSRV_SGX_CCB_CTL, ui32ReadOffset),
			    (psKernelCCB->ui32CCBDumpWOff + 1) & 0xff, 0xff,
			    PDUMP_POLL_OPERATOR_NOTEQUAL, IMG_FALSE, IMG_FALSE,
			    MAKEUNIQUETAG(psKernelCCB->psCCBCtlMemInfo));

		PDUMPCOMMENTWITHFLAGS(0, "Kernel CCB command\r\n");
		pvDumpCommand =
		    (void *)((u8 *)psKernelCCB->psCCBMemInfo->
				  pvLinAddrKM +
				  (*psKernelCCB->pui32WriteOffset *
			      sizeof(struct SGXMKIF_COMMAND)));

		PDUMPMEM(pvDumpCommand,
			 psKernelCCB->psCCBMemInfo,
			 psKernelCCB->ui32CCBDumpWOff *
			 sizeof(struct SGXMKIF_COMMAND),
			 sizeof(struct SGXMKIF_COMMAND), ui32PDumpFlags,
			 MAKEUNIQUETAG(psKernelCCB->psCCBMemInfo));

		PDUMPMEM(&psDevInfo->sPDContext.ui32CacheControl,
			 psKernelCCB->psCCBMemInfo,
			 psKernelCCB->ui32CCBDumpWOff *
			 sizeof(struct SGXMKIF_COMMAND) +
			 offsetof(struct SGXMKIF_COMMAND, ui32Data[2]),
			 sizeof(u32), ui32PDumpFlags,
			 MAKEUNIQUETAG(psKernelCCB->psCCBMemInfo));

		if (PDumpIsCaptureFrameKM() ||
		    ((ui32PDumpFlags & PDUMP_FLAGS_CONTINUOUS) != 0))
			psDevInfo->sPDContext.ui32CacheControl = 0;
	}
#endif
	*psKernelCCB->pui32WriteOffset =
	    (*psKernelCCB->pui32WriteOffset + 1) & 255;

#if defined(PDUMP)
	if (ui32CallerID != ISR_ID) {
		if (PDumpIsCaptureFrameKM() ||
		    ((ui32PDumpFlags & PDUMP_FLAGS_CONTINUOUS) != 0))
			psKernelCCB->ui32CCBDumpWOff =
			    (psKernelCCB->ui32CCBDumpWOff + 1) & 0xFF;

		PDUMPCOMMENTWITHFLAGS(0, "Kernel CCB write offset\r\n");
		PDUMPMEM(&psKernelCCB->ui32CCBDumpWOff,
			 psKernelCCB->psCCBCtlMemInfo,
			 offsetof(struct PVRSRV_SGX_CCB_CTL, ui32WriteOffset),
			 sizeof(u32), ui32PDumpFlags,
			 MAKEUNIQUETAG(psKernelCCB->psCCBCtlMemInfo));
		PDUMPCOMMENTWITHFLAGS(0, "Kernel CCB event kicker\r\n");
		PDUMPMEM(&psKernelCCB->ui32CCBDumpWOff,
			 psDevInfo->psKernelCCBEventKickerMemInfo, 0,
			 sizeof(u32), ui32PDumpFlags,
			 MAKEUNIQUETAG(psDevInfo->
				       psKernelCCBEventKickerMemInfo));
		PDUMPCOMMENTWITHFLAGS(0, "Event kick\r\n");
		PDUMPREGWITHFLAGS(SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK, 0),
				  EUR_CR_EVENT_KICK_NOW_MASK, 0);
	}
#endif
	*psDevInfo->pui32KernelCCBEventKicker =
	    (*psDevInfo->pui32KernelCCBEventKicker + 1) & 0xFF;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM,
		     SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK, 0),
		     EUR_CR_EVENT_KICK_NOW_MASK);

#if defined(NO_HARDWARE)

	*psKernelCCB->pui32ReadOffset =
	    (*psKernelCCB->pui32ReadOffset + 1) & 255;
#endif

Exit:
	return eError;
}

enum PVRSRV_ERROR SGXScheduleCCBCommandKM(
			struct PVRSRV_DEVICE_NODE *psDeviceNode,
			enum SGXMKIF_COMMAND_TYPE eCommandType,
			struct SGXMKIF_COMMAND *psCommandData,
			u32 ui32CallerID, u32 ui32PDumpFlags)
{
	enum PVRSRV_ERROR eError;
	struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

	PDUMPSUSPEND();

	eError =
	    PVRSRVSetDevicePowerStateKM(psDeviceNode->sDevId.ui32DeviceIndex,
					PVRSRV_POWER_STATE_D0);

	PDUMPRESUME();

	if (eError == PVRSRV_OK) {
		psDeviceNode->bReProcessDeviceCommandComplete = IMG_FALSE;
	} else {
		PVR_DPF(PVR_DBG_ERROR, "%s: can't power on device (%d)",
					__func__, eError);
		return eError;
	}

	eError = SGXScheduleCCBCommand(psDevInfo, eCommandType, psCommandData,
				  ui32CallerID, ui32PDumpFlags);

	if (ui32CallerID != ISR_ID)
		SGXTestActivePowerEvent(psDeviceNode);

	return eError;
}

enum PVRSRV_ERROR SGXScheduleProcessQueues(struct PVRSRV_DEVICE_NODE
					   *psDeviceNode)
{
	enum PVRSRV_ERROR eError;
	struct PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	struct SGXMKIF_HOST_CTL *psHostCtl =
	    psDevInfo->psKernelSGXHostCtlMemInfo->pvLinAddrKM;
	u32 ui32PowerStatus;
	struct SGXMKIF_COMMAND sCommand = { 0 };

	ui32PowerStatus = psHostCtl->ui32PowerStatus;
	if ((ui32PowerStatus & PVRSRV_USSE_EDM_POWMAN_NO_WORK) != 0)
		return PVRSRV_OK;

	eError =
	    PVRSRVSetDevicePowerStateKM(psDeviceNode->sDevId.ui32DeviceIndex,
					PVRSRV_POWER_STATE_D0);
	if (eError != PVRSRV_OK)
		return eError;

	sCommand.ui32Data[0] = PVRSRV_CCBFLAGS_PROCESS_QUEUESCMD;
	eError = SGXScheduleCCBCommand(psDeviceNode->pvDevice,
				       SGXMKIF_COMMAND_EDM_KICK, &sCommand,
				       ISR_ID, 0);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "%s failed to schedule CCB command: %lu",
			 __func__, eError);
		return PVRSRV_ERROR_GENERIC;
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR SGXScheduleProcessQueuesKM(struct PVRSRV_DEVICE_NODE
					   *psDeviceNode)
{
	enum PVRSRV_ERROR eError;

	eError = SGXScheduleProcessQueues(psDeviceNode);

	return eError;
}

IMG_BOOL SGXIsDevicePowered(struct PVRSRV_DEVICE_NODE *psDeviceNode)
{
	return PVRSRVIsDevicePowered(psDeviceNode->sDevId.ui32DeviceIndex);
}

enum PVRSRV_ERROR SGXGetInternalDevInfoKM(void *hDevCookie,
					      struct SGX_INTERNAL_DEVINFO
					      *psSGXInternalDevInfo)
{
	struct PVRSRV_SGXDEV_INFO *psDevInfo = (struct PVRSRV_SGXDEV_INFO *)
			((struct PVRSRV_DEVICE_NODE *)hDevCookie)->pvDevice;

	psSGXInternalDevInfo->ui32Flags = psDevInfo->ui32Flags;
	psSGXInternalDevInfo->bForcePTOff = (IMG_BOOL)psDevInfo->bForcePTOff;

	psSGXInternalDevInfo->hHostCtlKernelMemInfoHandle =
	    (void *)psDevInfo->psKernelSGXHostCtlMemInfo;

	return PVRSRV_OK;
}

#if defined(PDUMP) && !defined(EDM_USSE_HWDEBUG)
#define PDUMP_SGX_CLEANUP
#endif

void SGXCleanupRequest(struct PVRSRV_DEVICE_NODE *psDeviceNode,
				  struct IMG_DEV_VIRTADDR *psHWDataDevVAddr,
				  u32 ui32ResManRequestFlag)
{
	struct PVRSRV_SGXDEV_INFO *psSGXDevInfo =
	    (struct PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;
	struct PVRSRV_KERNEL_MEM_INFO *psSGXHostCtlMemInfo =
	    psSGXDevInfo->psKernelSGXHostCtlMemInfo;
	struct SGXMKIF_HOST_CTL __iomem *psSGXHostCtl =
	    (struct SGXMKIF_HOST_CTL __iomem __force *)
					psSGXHostCtlMemInfo->pvLinAddrKM;
#if defined(PDUMP_SGX_CLEANUP)
	void *hUniqueTag = MAKEUNIQUETAG(psSGXHostCtlMemInfo);
#endif
	u32 l;

	if (readl(&psSGXHostCtl->ui32PowerStatus) &
	     PVRSRV_USSE_EDM_POWMAN_NO_WORK) {
		;
	} else {
		if (psSGXDevInfo->ui32CacheControl &
		    SGX_BIF_INVALIDATE_PDCACHE) {
			l = readl(&psSGXHostCtl->ui32ResManFlags);
			l |= PVRSRV_USSE_EDM_RESMAN_CLEANUP_INVALPD;
			writel(l, &psSGXHostCtl->ui32ResManFlags);

			psSGXDevInfo->ui32CacheControl ^=
			    SGX_BIF_INVALIDATE_PDCACHE;
		}
		if (psSGXDevInfo->ui32CacheControl &
		    SGX_BIF_INVALIDATE_PTCACHE) {
			l = readl(&psSGXHostCtl->ui32ResManFlags);
			l |= PVRSRV_USSE_EDM_RESMAN_CLEANUP_INVALPT;
			writel(l, &psSGXHostCtl->ui32ResManFlags);

			psSGXDevInfo->ui32CacheControl ^=
			    SGX_BIF_INVALIDATE_PTCACHE;
		}

		if (psHWDataDevVAddr == NULL)
			writel(0, &psSGXHostCtl->sResManCleanupData.uiAddr);
		else
			writel(psHWDataDevVAddr->uiAddr,
				&psSGXHostCtl->sResManCleanupData.uiAddr);

		l = readl(&psSGXHostCtl->ui32ResManFlags);
		l |= ui32ResManRequestFlag;
		writel(l, &psSGXHostCtl->ui32ResManFlags);

#if defined(PDUMP_SGX_CLEANUP)

		PDUMPCOMMENTWITHFLAGS(0,
		    "TA/3D CCB Control - Request clean-up event on uKernel...");
		PDUMPMEM(NULL, psSGXHostCtlMemInfo,
			 offsetof(struct SGXMKIF_HOST_CTL,
				  sResManCleanupData.uiAddr), sizeof(u32), 0,
			 hUniqueTag);
		PDUMPMEM(&ui32ResManRequestFlag, psSGXHostCtlMemInfo,
			 offsetof(struct SGXMKIF_HOST_CTL, ui32ResManFlags),
			 sizeof(u32), 0, hUniqueTag);
#else
		PDUMPCOMMENTWITHFLAGS(0, "Clean-up event on uKernel disabled");
#endif

		SGXScheduleProcessQueues(psDeviceNode);

#if !defined(NO_HARDWARE)
		if (PollForValueKM(&psSGXHostCtl->ui32ResManFlags,
		     PVRSRV_USSE_EDM_RESMAN_CLEANUP_COMPLETE,
		     PVRSRV_USSE_EDM_RESMAN_CLEANUP_COMPLETE,
		     MAX_HW_TIME_US / WAIT_TRY_COUNT,
		     WAIT_TRY_COUNT) != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR, "SGXCleanupRequest: "
					 "Wait for uKernel to clean up failed");
			PVR_DBG_BREAK;
		}
#endif

#if defined(PDUMP_SGX_CLEANUP)

		PDUMPCOMMENTWITHFLAGS(0, "TA/3D CCB Control - "
				   "Wait for clean-up request to complete...");
		PDUMPMEMPOL(psSGXHostCtlMemInfo,
			    offsetof(struct SGXMKIF_HOST_CTL, ui32ResManFlags),
			    PVRSRV_USSE_EDM_RESMAN_CLEANUP_COMPLETE,
			    PVRSRV_USSE_EDM_RESMAN_CLEANUP_COMPLETE,
			    PDUMP_POLL_OPERATOR_EQUAL, IMG_FALSE, IMG_FALSE,
			    hUniqueTag);
#endif

		l = readl(&psSGXHostCtl->ui32ResManFlags);
		l &= ~ui32ResManRequestFlag;
		writel(l, &psSGXHostCtl->ui32ResManFlags);

		l = readl(&psSGXHostCtl->ui32ResManFlags);
		l &= ~PVRSRV_USSE_EDM_RESMAN_CLEANUP_COMPLETE;
		writel(l, &psSGXHostCtl->ui32ResManFlags);

#if defined(PDUMP_SGX_CLEANUP)
		PDUMPMEM(NULL, psSGXHostCtlMemInfo,
			 offsetof(struct SGXMKIF_HOST_CTL, ui32ResManFlags),
			 sizeof(u32), 0, hUniqueTag);
#endif
	}
}

struct SGX_HW_RENDER_CONTEXT_CLEANUP {
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct IMG_DEV_VIRTADDR sHWRenderContextDevVAddr;
	void *hBlockAlloc;
	struct RESMAN_ITEM *psResItem;
};

static enum PVRSRV_ERROR SGXCleanupHWRenderContextCallback(void *pvParam,
						      u32 ui32Param)
{
	struct SGX_HW_RENDER_CONTEXT_CLEANUP *psCleanup = pvParam;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	SGXCleanupRequest(psCleanup->psDeviceNode,
			  &psCleanup->sHWRenderContextDevVAddr,
			  PVRSRV_USSE_EDM_RESMAN_CLEANUP_RC_REQUEST);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
		  sizeof(struct SGX_HW_RENDER_CONTEXT_CLEANUP),
		  psCleanup, psCleanup->hBlockAlloc);

	return PVRSRV_OK;
}

struct SGX_HW_TRANSFER_CONTEXT_CLEANUP {
	struct PVRSRV_DEVICE_NODE *psDeviceNode;
	struct IMG_DEV_VIRTADDR sHWTransferContextDevVAddr;
	void *hBlockAlloc;
	struct RESMAN_ITEM *psResItem;
};

static enum PVRSRV_ERROR SGXCleanupHWTransferContextCallback(void *pvParam,
							u32 ui32Param)
{
	struct SGX_HW_TRANSFER_CONTEXT_CLEANUP *psCleanup =
	    (struct SGX_HW_TRANSFER_CONTEXT_CLEANUP *)pvParam;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	SGXCleanupRequest(psCleanup->psDeviceNode,
			  &psCleanup->sHWTransferContextDevVAddr,
			  PVRSRV_USSE_EDM_RESMAN_CLEANUP_TC_REQUEST);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
		  sizeof(struct SGX_HW_TRANSFER_CONTEXT_CLEANUP),
		  psCleanup, psCleanup->hBlockAlloc);

	return PVRSRV_OK;
}

void *SGXRegisterHWRenderContextKM(void *psDeviceNode,
			    struct IMG_DEV_VIRTADDR *psHWRenderContextDevVAddr,
			    struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	enum PVRSRV_ERROR eError;
	void *hBlockAlloc;
	struct SGX_HW_RENDER_CONTEXT_CLEANUP *psCleanup;
	struct RESMAN_ITEM *psResItem;

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			    sizeof(struct SGX_HW_RENDER_CONTEXT_CLEANUP),
			    (void **) &psCleanup, &hBlockAlloc);

	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			"SGXRegisterHWRenderContextKM: "
			"Couldn't allocate memory for struct "
			"SGX_HW_RENDER_CONTEXT_CLEANUP structure");
		return NULL;
	}

	psCleanup->hBlockAlloc = hBlockAlloc;
	psCleanup->psDeviceNode = psDeviceNode;
	psCleanup->sHWRenderContextDevVAddr = *psHWRenderContextDevVAddr;

	psResItem = ResManRegisterRes(psPerProc->hResManContext,
				      RESMAN_TYPE_HW_RENDER_CONTEXT,
				      (void *) psCleanup,
				      0, &SGXCleanupHWRenderContextCallback);

	if (psResItem == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "SGXRegisterHWRenderContextKM: "
					"ResManRegisterRes failed");
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct SGX_HW_RENDER_CONTEXT_CLEANUP),
			  psCleanup, psCleanup->hBlockAlloc);

		return NULL;
	}

	psCleanup->psResItem = psResItem;

	return (void *)psCleanup;
}

enum PVRSRV_ERROR SGXUnregisterHWRenderContextKM(void *hHWRenderContext)
{
	struct SGX_HW_RENDER_CONTEXT_CLEANUP *psCleanup;

	PVR_ASSERT(hHWRenderContext != NULL);

	psCleanup = (struct SGX_HW_RENDER_CONTEXT_CLEANUP *)hHWRenderContext;

	if (psCleanup == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "SGXUnregisterHWRenderContextKM: invalid parameter");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	ResManFreeResByPtr(psCleanup->psResItem);

	return PVRSRV_OK;
}

void *SGXRegisterHWTransferContextKM(void *psDeviceNode,
		      struct IMG_DEV_VIRTADDR *psHWTransferContextDevVAddr,
		      struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	enum PVRSRV_ERROR eError;
	void *hBlockAlloc;
	struct SGX_HW_TRANSFER_CONTEXT_CLEANUP *psCleanup;
	struct RESMAN_ITEM *psResItem;

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			    sizeof(struct SGX_HW_TRANSFER_CONTEXT_CLEANUP),
			    (void **) &psCleanup, &hBlockAlloc);

	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			"SGXRegisterHWTransferContextKM: "
			"Couldn't allocate memory for struct "
			"SGX_HW_TRANSFER_CONTEXT_CLEANUP structure");
		return NULL;
	}

	psCleanup->hBlockAlloc = hBlockAlloc;
	psCleanup->psDeviceNode = psDeviceNode;
	psCleanup->sHWTransferContextDevVAddr = *psHWTransferContextDevVAddr;

	psResItem = ResManRegisterRes(psPerProc->hResManContext,
				      RESMAN_TYPE_HW_TRANSFER_CONTEXT,
				      psCleanup,
				      0, &SGXCleanupHWTransferContextCallback);

	if (psResItem == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "SGXRegisterHWTransferContextKM: "
						"ResManRegisterRes failed");
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(struct SGX_HW_TRANSFER_CONTEXT_CLEANUP),
			  psCleanup, psCleanup->hBlockAlloc);

		return NULL;
	}

	psCleanup->psResItem = psResItem;

	return (void *)psCleanup;
}

enum PVRSRV_ERROR SGXUnregisterHWTransferContextKM(void *hHWTransferContext)
{
	struct SGX_HW_TRANSFER_CONTEXT_CLEANUP *psCleanup;

	PVR_ASSERT(hHWTransferContext != NULL);

	psCleanup =
	    (struct SGX_HW_TRANSFER_CONTEXT_CLEANUP *)hHWTransferContext;

	if (psCleanup == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "SGXUnregisterHWTransferContextKM: "
						"invalid parameter");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	ResManFreeResByPtr(psCleanup->psResItem);

	return PVRSRV_OK;
}

static inline IMG_BOOL SGX2DQuerySyncOpsComplete(
				struct PVRSRV_KERNEL_SYNC_INFO *psSyncInfo,
				u32 ui32ReadOpsPending, u32 ui32WriteOpsPending)
{
	struct PVRSRV_SYNC_DATA *psSyncData = psSyncInfo->psSyncData;

	return (IMG_BOOL)((psSyncData->ui32ReadOpsComplete >=
				ui32ReadOpsPending) &&
			   (psSyncData->ui32WriteOpsComplete >=
				ui32WriteOpsPending));
}

enum PVRSRV_ERROR SGX2DQueryBlitsCompleteKM(
				    struct PVRSRV_SGXDEV_INFO *psDevInfo,
				    struct PVRSRV_KERNEL_SYNC_INFO *psSyncInfo,
				    IMG_BOOL bWaitForComplete)
{
	u32 ui32ReadOpsPending, ui32WriteOpsPending;

	PVR_UNREFERENCED_PARAMETER(psDevInfo);

	PVR_DPF(PVR_DBG_CALLTRACE, "SGX2DQueryBlitsCompleteKM: Start");

	ui32ReadOpsPending = psSyncInfo->psSyncData->ui32ReadOpsPending;
	ui32WriteOpsPending = psSyncInfo->psSyncData->ui32WriteOpsPending;

	if (SGX2DQuerySyncOpsComplete
	    (psSyncInfo, ui32ReadOpsPending, ui32WriteOpsPending)) {

		PVR_DPF(PVR_DBG_CALLTRACE,
			 "SGX2DQueryBlitsCompleteKM: No wait. Blits complete.");
		return PVRSRV_OK;
	}

	if (!bWaitForComplete) {

		PVR_DPF(PVR_DBG_CALLTRACE,
			 "SGX2DQueryBlitsCompleteKM: No wait. Ops pending.");
		return PVRSRV_ERROR_CMD_NOT_PROCESSED;
	}

	PVR_DPF(PVR_DBG_MESSAGE,
		 "SGX2DQueryBlitsCompleteKM: Ops pending. Start polling.");
	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US) {
		OSWaitus(MAX_HW_TIME_US / WAIT_TRY_COUNT);

		if (SGX2DQuerySyncOpsComplete
		    (psSyncInfo, ui32ReadOpsPending, ui32WriteOpsPending)) {

			PVR_DPF(PVR_DBG_CALLTRACE,
					"SGX2DQueryBlitsCompleteKM: "
					"Wait over.  Blits complete.");
			return PVRSRV_OK;
		}
	}
	END_LOOP_UNTIL_TIMEOUT();

	PVR_DPF(PVR_DBG_ERROR,
		 "SGX2DQueryBlitsCompleteKM: Timed out. Ops pending.");

#if defined(CONFIG_PVR_DEBUG_EXTRA)
	{
		struct PVRSRV_SYNC_DATA *psSyncData = psSyncInfo->psSyncData;

		PVR_TRACE("SGX2DQueryBlitsCompleteKM: "
			   "Syncinfo: %p, Syncdata: %p",
			   psSyncInfo, psSyncData);

		PVR_TRACE("SGX2DQueryBlitsCompleteKM: "
			   "Read ops complete: %d, Read ops pending: %d",
			   psSyncData->ui32ReadOpsComplete,
			   psSyncData->ui32ReadOpsPending);
		PVR_TRACE("SGX2DQueryBlitsCompleteKM: "
			  "Write ops complete: %d, Write ops pending: %d",
			  psSyncData->ui32WriteOpsComplete,
			  psSyncData->ui32WriteOpsPending);

	}
#endif
	return PVRSRV_ERROR_TIMEOUT;
}

void SGXFlushHWRenderTargetKM(void *psDeviceNode,
			      struct IMG_DEV_VIRTADDR sHWRTDataSetDevVAddr)
{
	PVR_ASSERT(sHWRTDataSetDevVAddr.uiAddr);

	SGXCleanupRequest((struct PVRSRV_DEVICE_NODE *)psDeviceNode,
			  &sHWRTDataSetDevVAddr,
			  PVRSRV_USSE_EDM_RESMAN_CLEANUP_RT_REQUEST);
}

u32 SGXConvertTimeStamp(struct PVRSRV_SGXDEV_INFO *psDevInfo, u32 ui32TimeWraps,
			u32 ui32Time)
{
	u64 ui64Clocks;
	u32 ui32Clocksx16;

	ui64Clocks = ((u64) ui32TimeWraps * psDevInfo->ui32uKernelTimerClock) +
	    (psDevInfo->ui32uKernelTimerClock -
	     (ui32Time & EUR_CR_EVENT_TIMER_VALUE_MASK));
	ui32Clocksx16 = (u32) (ui64Clocks / 16);

	return ui32Clocksx16;
}
