/*************************************************************************/ /*!
@Title          Device specific utility routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Device specific functions
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

#include <stddef.h>

#include "sgxdefs.h"
#include "services_headers.h"
#include "buffer_manager.h"
#include "sgxapi_km.h"
#include "sgxinfo.h"
#include "sgx_mkif_km.h"
#include "sysconfig.h"
#include "pdump_km.h"
#include "mmu.h"
#include "pvr_bridge_km.h"
#include "osfunc.h"
#include "pvr_debug.h"
#include "sgxutils.h"

#ifdef __linux__
#include <linux/tty.h>
#else
#include <stdio.h>
#endif


#if defined(SYS_CUSTOM_POWERDOWN)
PVRSRV_ERROR SysPowerDownMISR(PVRSRV_DEVICE_NODE	* psDeviceNode, IMG_UINT32 ui32CallerID);
#endif



/*!
******************************************************************************

 @Function	SGXPostActivePowerEvent

 @Description

	 post power event functionality (e.g. restart)

 @Input	psDeviceNode : SGX Device Node
 @Input	ui32CallerID - KERNEL_ID or ISR_ID

 @Return   IMG_VOID :

******************************************************************************/
IMG_VOID SGXPostActivePowerEvent(PVRSRV_DEVICE_NODE	* psDeviceNode,
                                 IMG_UINT32           ui32CallerID)
{
	PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	SGXMKIF_HOST_CTL	*psSGXHostCtl = psDevInfo->psSGXHostCtl;

	/* Update the counter for stats. */
	psSGXHostCtl->ui32NumActivePowerEvents++;

	if ((psSGXHostCtl->ui32PowerStatus & PVRSRV_USSE_EDM_POWMAN_POWEROFF_RESTART_IMMEDIATE) != 0)
	{



		if (ui32CallerID == ISR_ID)
		{
			psDeviceNode->bReProcessDeviceCommandComplete = IMG_TRUE;
		}
		else
		{
			SGXScheduleProcessQueuesKM(psDeviceNode);
		}
	}
}


/*!
******************************************************************************

 @Function	SGXTestActivePowerEvent

 @Description

 Checks whether the microkernel has generated an active power event. If so,
 	perform the power transition.

 @Input	psDeviceNode : SGX Device Node
 @Input	ui32CallerID - KERNEL_ID or ISR_ID

 @Return   IMG_VOID :

******************************************************************************/
IMG_VOID SGXTestActivePowerEvent (PVRSRV_DEVICE_NODE	*psDeviceNode,
								  IMG_UINT32			ui32CallerID)
{
	PVRSRV_ERROR		eError = PVRSRV_OK;
	PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	SGXMKIF_HOST_CTL	*psSGXHostCtl = psDevInfo->psSGXHostCtl;

	if (((psSGXHostCtl->ui32InterruptFlags & PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER) != 0) &&
		((psSGXHostCtl->ui32InterruptClearFlags & PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER) == 0))
	{

		psSGXHostCtl->ui32InterruptClearFlags |= PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER;

		/* Suspend pdumping. */
		PDUMPSUSPEND();

#if defined(SYS_CUSTOM_POWERDOWN)
		/*
		 	Some power down code cannot be executed inside an MISR on
		 	some platforms that use mutexes inside the power code.
		 */
		eError = SysPowerDownMISR(psDeviceNode, ui32CallerID);
#else
		eError = PVRSRVSetDevicePowerStateKM(psDeviceNode->sDevId.ui32DeviceIndex,
											 PVRSRV_DEV_POWER_STATE_OFF,
											 ui32CallerID, IMG_FALSE);
		if (eError == PVRSRV_OK)
		{
			SGXPostActivePowerEvent(psDeviceNode, ui32CallerID);
		}
#endif
		if (eError == PVRSRV_ERROR_RETRY)
		{


			psSGXHostCtl->ui32InterruptClearFlags &= ~PVRSRV_USSE_EDM_INTERRUPT_ACTIVE_POWER;
			eError = PVRSRV_OK;
		}


		PDUMPRESUME();
	}

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXTestActivePowerEvent error:%lu", eError));
	}
}


/******************************************************************************
 FUNCTION	: SGXAcquireKernelCCBSlot

 PURPOSE	: Attempts to obtain a slot in the Kernel CCB

 PARAMETERS	: psCCB - the CCB

 RETURNS	: Address of space if available, IMG_NULL otherwise
******************************************************************************/
#ifdef INLINE_IS_PRAGMA
#pragma inline(SGXAcquireKernelCCBSlot)
#endif
static INLINE SGXMKIF_COMMAND * SGXAcquireKernelCCBSlot(PVRSRV_SGX_CCB_INFO *psCCB)
{
	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US)
	{
		if(((*psCCB->pui32WriteOffset + 1) & 255) != *psCCB->pui32ReadOffset)
		{
			return &psCCB->psCommands[*psCCB->pui32WriteOffset];
		}

		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT();

	/* Time out on waiting for CCB space */
	return IMG_NULL;
}

/*!
******************************************************************************

 @Function	SGXScheduleCCBCommand

 @Description - Submits a CCB command and kicks the ukernel (without
 				power management)

 @Input psDevInfo - pointer to device info
 @Input eCmdType - see SGXMKIF_CMD_*
 @Input psCommandData - kernel CCB command
 @Input ui32CallerID - KERNEL_ID or ISR_ID
 @Input ui32PDumpFlags

 @Return ui32Error - success or failure

******************************************************************************/
PVRSRV_ERROR SGXScheduleCCBCommand(PVRSRV_SGXDEV_INFO 	*psDevInfo,
								   SGXMKIF_CMD_TYPE		eCmdType,
								   SGXMKIF_COMMAND		*psCommandData,
								   IMG_UINT32			ui32CallerID,
								   IMG_UINT32			ui32PDumpFlags)
{
	PVRSRV_SGX_CCB_INFO *psKernelCCB;
	PVRSRV_ERROR eError = PVRSRV_OK;
	SGXMKIF_COMMAND *psSGXCommand;
#if defined(PDUMP)
	IMG_VOID *pvDumpCommand;
	IMG_BOOL bPDumpIsSuspended = PDumpIsSuspended();
#else
	PVR_UNREFERENCED_PARAMETER(ui32CallerID);
	PVR_UNREFERENCED_PARAMETER(ui32PDumpFlags);
#endif

#if defined(FIX_HW_BRN_28889)




	if ( (eCmdType != SGXMKIF_CMD_PROCESS_QUEUES) &&
		 ((psDevInfo->ui32CacheControl & SGXMKIF_CC_INVAL_DATA) != 0) &&
		 ((psDevInfo->ui32CacheControl & (SGXMKIF_CC_INVAL_BIF_PT | SGXMKIF_CC_INVAL_BIF_PD)) != 0))
	{
	#if defined(PDUMP)
		PVRSRV_KERNEL_MEM_INFO	*psSGXHostCtlMemInfo = psDevInfo->psKernelSGXHostCtlMemInfo;
	#endif
		SGXMKIF_HOST_CTL	*psSGXHostCtl = psDevInfo->psSGXHostCtl;
		SGXMKIF_COMMAND		sCacheCommand = {0};

		eError = SGXScheduleCCBCommand(psDevInfo,
									   SGXMKIF_CMD_PROCESS_QUEUES,
									   &sCacheCommand,
									   ui32CallerID,
									   ui32PDumpFlags);
		if (eError != PVRSRV_OK)
		{
			goto Exit;
		}

		/* Wait for the invalidate to happen */
		#if !defined(NO_HARDWARE)
		if(PollForValueKM(&psSGXHostCtl->ui32InvalStatus,
						  PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE,
						  PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE,
						  2 * MAX_HW_TIME_US/WAIT_TRY_COUNT,
						  WAIT_TRY_COUNT) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXScheduleCCBCommand: Wait for uKernel to Invalidate BIF cache failed"));
			PVR_DBG_BREAK;
		}
		#endif

		psSGXHostCtl->ui32InvalStatus &= ~(PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE);

		#if defined(PDUMP)
		if ((ui32CallerID != ISR_ID) && (bPDumpIsSuspended == IMG_FALSE))
		{

			PDUMPCOMMENTWITHFLAGS(0, "Host Control - Poll for BIF cache invalidate request to complete");
			PDUMPMEMPOL(psSGXHostCtlMemInfo,
						offsetof(SGXMKIF_HOST_CTL, ui32InvalStatus),
						PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE,
						PVRSRV_USSE_EDM_BIF_INVAL_COMPLETE,
						PDUMP_POLL_OPERATOR_EQUAL,
						0,
						MAKEUNIQUETAG(psSGXHostCtlMemInfo));
			PDUMPMEM(IMG_NULL, psSGXHostCtlMemInfo, offsetof(SGXMKIF_HOST_CTL, ui32CleanupStatus), sizeof(IMG_UINT32), 0, MAKEUNIQUETAG(psSGXHostCtlMemInfo));
		}
		#endif
	}
#endif

	psKernelCCB = psDevInfo->psKernelCCBInfo;

	psSGXCommand = SGXAcquireKernelCCBSlot(psKernelCCB);


	if(!psSGXCommand)
	{
		eError = PVRSRV_ERROR_TIMEOUT;
		goto Exit;
	}

	/* embed cache control word */
	psCommandData->ui32CacheControl = psDevInfo->ui32CacheControl;

#if defined(PDUMP)
	/* Accumulate any cache invalidates that may have happened */
	psDevInfo->sPDContext.ui32CacheControl |= psDevInfo->ui32CacheControl;
#endif

	/* and clear it */
	psDevInfo->ui32CacheControl = 0;

	/* Copy command data over */
	*psSGXCommand = *psCommandData;

	if (eCmdType >= SGXMKIF_CMD_MAX)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXScheduleCCBCommandKM: Unknown command type: %d", eCmdType)) ;
		eError = PVRSRV_ERROR_GENERIC;
		goto Exit;
	}

#if defined(SUPPORT_CPU_CACHED_BUFFERS)
	{
		SYS_DATA *psSysData;

		SysAcquireData(&psSysData);

		if (psSysData->bFlushAll)
		{
			OSFlushCPUCacheKM();

			psSysData->bFlushAll = IMG_FALSE;
		}
	}
#endif

	psSGXCommand->ui32ServiceAddress = psDevInfo->aui32HostKickAddr[eCmdType];

#if defined(PDUMP)
	if ((ui32CallerID != ISR_ID) && (bPDumpIsSuspended == IMG_FALSE))
	{

		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Poll for space in the Kernel CCB\r\n");
		PDUMPMEMPOL(psKernelCCB->psCCBCtlMemInfo,
					offsetof(PVRSRV_SGX_CCB_CTL, ui32ReadOffset),
					(psKernelCCB->ui32CCBDumpWOff + 1) & 0xff,
					0xff,
					PDUMP_POLL_OPERATOR_NOTEQUAL,
					ui32PDumpFlags,
					MAKEUNIQUETAG(psKernelCCB->psCCBCtlMemInfo));

		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Kernel CCB command\r\n");
		pvDumpCommand = (IMG_VOID *)((IMG_UINT8 *)psKernelCCB->psCCBMemInfo->pvLinAddrKM + (*psKernelCCB->pui32WriteOffset * sizeof(SGXMKIF_COMMAND)));

		PDUMPMEM(pvDumpCommand,
					psKernelCCB->psCCBMemInfo,
					psKernelCCB->ui32CCBDumpWOff * sizeof(SGXMKIF_COMMAND),
					sizeof(SGXMKIF_COMMAND),
					ui32PDumpFlags,
					MAKEUNIQUETAG(psKernelCCB->psCCBMemInfo));

		/* Overwrite cache control with pdump shadow */
		PDUMPMEM(&psDevInfo->sPDContext.ui32CacheControl,
					psKernelCCB->psCCBMemInfo,
					psKernelCCB->ui32CCBDumpWOff * sizeof(SGXMKIF_COMMAND) +
					offsetof(SGXMKIF_COMMAND, ui32CacheControl),
					sizeof(IMG_UINT32),
					ui32PDumpFlags,
					MAKEUNIQUETAG(psKernelCCB->psCCBMemInfo));

		if (PDumpIsCaptureFrameKM()
		|| ((ui32PDumpFlags & PDUMP_FLAGS_CONTINUOUS) != 0))
		{
			/* Clear cache invalidate shadow */
			psDevInfo->sPDContext.ui32CacheControl = 0;
		}
	}
#endif

#if defined(FIX_HW_BRN_26620) && defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_BYPASS_SYSTEM_CACHE)

	eError = PollForValueKM (psKernelCCB->pui32ReadOffset,
								*psKernelCCB->pui32WriteOffset,
								0xFF,
								MAX_HW_TIME_US/WAIT_TRY_COUNT,
								WAIT_TRY_COUNT);
	if (eError != PVRSRV_OK)
	{
		eError = PVRSRV_ERROR_TIMEOUT;
		goto Exit;
	}
#endif

	/*
		Increment the write offset
	*/
	*psKernelCCB->pui32WriteOffset = (*psKernelCCB->pui32WriteOffset + 1) & 255;

#if defined(PDUMP)
	if ((ui32CallerID != ISR_ID) && (bPDumpIsSuspended == IMG_FALSE))
	{
	#if defined(FIX_HW_BRN_26620) && defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_BYPASS_SYSTEM_CACHE)
		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Poll for previous Kernel CCB CMD to be read\r\n");
		PDUMPMEMPOL(psKernelCCB->psCCBCtlMemInfo,
					offsetof(PVRSRV_SGX_CCB_CTL, ui32ReadOffset),
					(psKernelCCB->ui32CCBDumpWOff),
					0xFF,
					PDUMP_POLL_OPERATOR_EQUAL,
					ui32PDumpFlags,
					MAKEUNIQUETAG(psKernelCCB->psCCBCtlMemInfo));
	#endif

		if (PDumpIsCaptureFrameKM()
		|| ((ui32PDumpFlags & PDUMP_FLAGS_CONTINUOUS) != 0))
		{
			psKernelCCB->ui32CCBDumpWOff = (psKernelCCB->ui32CCBDumpWOff + 1) & 0xFF;
			psDevInfo->ui32KernelCCBEventKickerDumpVal = (psDevInfo->ui32KernelCCBEventKickerDumpVal + 1) & 0xFF;
		}

		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Kernel CCB write offset\r\n");
		PDUMPMEM(&psKernelCCB->ui32CCBDumpWOff,
				 psKernelCCB->psCCBCtlMemInfo,
				 offsetof(PVRSRV_SGX_CCB_CTL, ui32WriteOffset),
				 sizeof(IMG_UINT32),
				 ui32PDumpFlags,
				 MAKEUNIQUETAG(psKernelCCB->psCCBCtlMemInfo));
		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Kernel CCB event kicker\r\n");
		PDUMPMEM(&psDevInfo->ui32KernelCCBEventKickerDumpVal,
				 psDevInfo->psKernelCCBEventKickerMemInfo,
				 0,
				 sizeof(IMG_UINT32),
				 ui32PDumpFlags,
				 MAKEUNIQUETAG(psDevInfo->psKernelCCBEventKickerMemInfo));
		PDUMPCOMMENTWITHFLAGS(ui32PDumpFlags, "Kick the SGX microkernel\r\n");
	#if defined(FIX_HW_BRN_26620) && defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_BYPASS_SYSTEM_CACHE)
		PDUMPREGWITHFLAGS(SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK2, 0), EUR_CR_EVENT_KICK2_NOW_MASK, ui32PDumpFlags);
	#else
		PDUMPREGWITHFLAGS(SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK, 0), EUR_CR_EVENT_KICK_NOW_MASK, ui32PDumpFlags);
	#endif
	}
#endif

	*psDevInfo->pui32KernelCCBEventKicker = (*psDevInfo->pui32KernelCCBEventKicker + 1) & 0xFF;
#if defined(FIX_HW_BRN_26620) && defined(SGX_FEATURE_SYSTEM_CACHE) && !defined(SGX_BYPASS_SYSTEM_CACHE)
	OSWriteHWReg(psDevInfo->pvRegsBaseKM,
				SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK2, 0),
				EUR_CR_EVENT_KICK2_NOW_MASK);
#else
	OSWriteHWReg(psDevInfo->pvRegsBaseKM,
				SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK, 0),
				EUR_CR_EVENT_KICK_NOW_MASK);
#endif

#if defined(NO_HARDWARE)

	*psKernelCCB->pui32ReadOffset = (*psKernelCCB->pui32ReadOffset + 1) & 255;
#endif

Exit:
	return eError;
}


/*!
******************************************************************************

 @Function	SGXScheduleCCBCommandKM

 @Description - Submits a CCB command and kicks the ukernel

 @Input psDeviceNode - pointer to SGX device node
 @Input eCmdType - see SGXMKIF_CMD_*
 @Input psCommandData - kernel CCB command
 @Input ui32CallerID - KERNEL_ID or ISR_ID
 @Input ui32PDumpFlags

 @Return ui32Error - success or failure

******************************************************************************/
PVRSRV_ERROR SGXScheduleCCBCommandKM(PVRSRV_DEVICE_NODE		*psDeviceNode,
									 SGXMKIF_CMD_TYPE		eCmdType,
									 SGXMKIF_COMMAND		*psCommandData,
									 IMG_UINT32				ui32CallerID,
									 IMG_UINT32				ui32PDumpFlags)
{
	PVRSRV_ERROR		eError;
	PVRSRV_SGXDEV_INFO 	*psDevInfo = psDeviceNode->pvDevice;


	PDUMPSUSPEND();

	/* Ensure that SGX is powered up before kicking the ukernel. */
	eError = PVRSRVSetDevicePowerStateKM(psDeviceNode->sDevId.ui32DeviceIndex,
										 PVRSRV_DEV_POWER_STATE_ON,
										 ui32CallerID,
										 IMG_TRUE);

	PDUMPRESUME();

	if (eError == PVRSRV_OK)
	{
		psDeviceNode->bReProcessDeviceCommandComplete = IMG_FALSE;
	}
	else
	{
		if (eError == PVRSRV_ERROR_RETRY)
		{
			if (ui32CallerID == ISR_ID)
			{



				psDeviceNode->bReProcessDeviceCommandComplete = IMG_TRUE;
				eError = PVRSRV_OK;
			}
			else
			{


			}
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXScheduleCCBCommandKM failed to acquire lock - "
					 "ui32CallerID:%ld eError:%lu", ui32CallerID, eError));
		}

		return eError;
	}

	eError = SGXScheduleCCBCommand(psDevInfo, eCmdType, psCommandData, ui32CallerID, ui32PDumpFlags);

	PVRSRVPowerUnlock(ui32CallerID);

	if (ui32CallerID != ISR_ID)
	{



		SGXTestActivePowerEvent(psDeviceNode, ui32CallerID);
	}

	return eError;
}


#define print_error(function, variable)\
{\
	printk(KERN_ERR "ERROR!\nERROR!\nERROR!\n");\
	printk(KERN_ERR "ERROR: %s() about to dereference a NULL pointer!\n", function);\
	printk(KERN_ERR "  %s is NULL\n", variable);\
	printk(KERN_ERR "  The client-side program needs to call SrvInit()\n");\
	printk(KERN_ERR "  Working-around error by exiting the function early\n");\
}


/*!
******************************************************************************

 @Function	SGXScheduleProcessQueuesKM

 @Description - Software command complete handler

 @Input psDeviceNode - SGX device node

******************************************************************************/
PVRSRV_ERROR SGXScheduleProcessQueuesKM(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	/* Note: If the client-side code doesn't call SrvInit(), the normal
	 * implementation of this function will chase a NULL pointer.  The
	 * workaround is to use an extra set of curly braces, and then to check the
	 * pointers normally used during the variable initialization code.  if a
	 * problem is found, exit early and provide a helpful error message in the
	 * kernel log file.
	 */
	if (NULL == psDeviceNode) {
		print_error(__FUNCTION__, "psDeviceNode");
		return PVRSRV_OK;
	} else {
		PVRSRV_SGXDEV_INFO 	*psDevInfo = psDeviceNode->pvDevice;
		if (NULL == psDevInfo) {
			print_error(__FUNCTION__, "psDevInfo");
			return PVRSRV_OK;
		}
		if (NULL == psDevInfo->psKernelSGXHostCtlMemInfo) {
			print_error(__FUNCTION__, "psDevInfo->psKernelSGXHostCtlMemInfo");
			return PVRSRV_OK;
		}
		if (NULL == psDevInfo->psKernelSGXHostCtlMemInfo->pvLinAddrKM) {
			print_error(__FUNCTION__,
				"psDevInfo->psKernelSGXHostCtlMemInfo->pvLinAddrKM");
			return PVRSRV_OK;
		}
	}
	/* Below starts the extra set of curly-braces, and the original function: */
{
	PVRSRV_ERROR 		eError;
	PVRSRV_SGXDEV_INFO 	*psDevInfo = psDeviceNode->pvDevice;
	SGXMKIF_HOST_CTL	*psHostCtl = psDevInfo->psKernelSGXHostCtlMemInfo->pvLinAddrKM;
	IMG_UINT32		ui32PowerStatus;
	SGXMKIF_COMMAND		sCommand = {0};

	ui32PowerStatus = psHostCtl->ui32PowerStatus;
	if ((ui32PowerStatus & PVRSRV_USSE_EDM_POWMAN_NO_WORK) != 0)
	{
		/* The ukernel has no work to do so don't waste power. */
		return PVRSRV_OK;
	}

	eError = SGXScheduleCCBCommandKM(psDeviceNode, SGXMKIF_CMD_PROCESS_QUEUES, &sCommand, ISR_ID, 0);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXScheduleProcessQueuesKM failed to schedule CCB command: %lu", eError));
		return PVRSRV_ERROR_GENERIC;
	}

	return PVRSRV_OK;
}
}


/*!
******************************************************************************

 @Function	SGXIsDevicePowered

 @Description

	Whether the device is powered, for the purposes of lockup detection.

 @Input psDeviceNode - pointer to device node

 @Return   IMG_BOOL  : Whether device is powered

******************************************************************************/
IMG_BOOL SGXIsDevicePowered(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	return PVRSRVIsDevicePowered(psDeviceNode->sDevId.ui32DeviceIndex);
}

/*!
*******************************************************************************

 @Function	SGXGetInternalDevInfoKM

 @Description
	Gets device information that is not intended to be passed
	on beyond the srvclient libs.

 @Input hDevCookie

 @Output psSGXInternalDevInfo

 @Return   PVRSRV_ERROR :

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR SGXGetInternalDevInfoKM(IMG_HANDLE hDevCookie,
									SGX_INTERNAL_DEVINFO *psSGXInternalDevInfo)
{
	PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)((PVRSRV_DEVICE_NODE *)hDevCookie)->pvDevice;

	psSGXInternalDevInfo->ui32Flags = psDevInfo->ui32Flags;
	psSGXInternalDevInfo->bForcePTOff = (IMG_BOOL)psDevInfo->bForcePTOff;

	/* This should be patched up by OS bridge code */
	psSGXInternalDevInfo->hHostCtlKernelMemInfoHandle =
		(IMG_HANDLE)psDevInfo->psKernelSGXHostCtlMemInfo;

	return PVRSRV_OK;
}


IMG_VOID SGXCleanupRequest(PVRSRV_DEVICE_NODE	*psDeviceNode,
						   IMG_DEV_VIRTADDR		*psHWDataDevVAddr,
						   IMG_UINT32			ui32CleanupType)
{
	/* Note: If the client-side code doesn't call SrvInit(), the normal
	 * implementation of this function will chase a NULL pointer.  The
	 * workaround is to use an extra set of curly braces, and then to check the
	 * pointers normally used during the variable initialization code.  if a
	 * problem is found, exit early and provide a helpful error message in the
	 * kernel log file.
	 */
	if (NULL == psDeviceNode) {
		print_error(__FUNCTION__, "psDeviceNode");
		return;
	} else {
		PVRSRV_SGXDEV_INFO 	*psSGXDevInfo = psDeviceNode->pvDevice;
		if (NULL == psSGXDevInfo) {
			print_error(__FUNCTION__, "psSGXDevInfo");
			return;
		}
		if (NULL == psSGXDevInfo->psKernelSGXHostCtlMemInfo) {
			print_error(__FUNCTION__,"psSGXDevInfo->psKernelSGXHostCtlMemInfo");
			return;
		} else {
			PVRSRV_KERNEL_MEM_INFO	*psSGXHostCtlMemInfo =
				psSGXDevInfo->psKernelSGXHostCtlMemInfo;
			if (NULL == psSGXHostCtlMemInfo->pvLinAddrKM) {
				print_error(__FUNCTION__, "psSGXHostCtlMemInfo->pvLinAddrKM");
				return;
			}
		}
	}
	/* Below starts the extra set of curly-braces, and the original function: */
{
	PVRSRV_ERROR			eError;
	PVRSRV_SGXDEV_INFO		*psSGXDevInfo = psDeviceNode->pvDevice;
	PVRSRV_KERNEL_MEM_INFO	*psSGXHostCtlMemInfo = psSGXDevInfo->psKernelSGXHostCtlMemInfo;
	SGXMKIF_HOST_CTL		*psSGXHostCtl = psSGXHostCtlMemInfo->pvLinAddrKM;

	if ((psSGXHostCtl->ui32PowerStatus & PVRSRV_USSE_EDM_POWMAN_NO_WORK) != 0)
	{

	}
	else
	{
		SGXMKIF_COMMAND		sCommand = {0};

		PDUMPCOMMENTWITHFLAGS(0, "Request ukernel resouce clean-up");
		sCommand.ui32Data[0] = ui32CleanupType;
		sCommand.ui32Data[1] = (psHWDataDevVAddr == IMG_NULL) ? 0 : psHWDataDevVAddr->uiAddr;

		eError = SGXScheduleCCBCommandKM(psDeviceNode, SGXMKIF_CMD_CLEANUP, &sCommand, KERNEL_ID, 0);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXCleanupRequest: Failed to submit clean-up command"));
			PVR_DBG_BREAK;
		}


		#if !defined(NO_HARDWARE)
		if(PollForValueKM(&psSGXHostCtl->ui32CleanupStatus,
						  PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE,
						  PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE,
						  MAX_HW_TIME_US/WAIT_TRY_COUNT,
						  WAIT_TRY_COUNT) != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"SGXCleanupRequest: Wait for uKernel to clean up failed"));
			PVR_DBG_BREAK;
		}
		#endif

		#if defined(PDUMP)

		PDUMPCOMMENTWITHFLAGS(0, "Host Control - Poll for clean-up request to complete");
		PDUMPMEMPOL(psSGXHostCtlMemInfo,
					offsetof(SGXMKIF_HOST_CTL, ui32CleanupStatus),
					PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE,
					PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE,
					PDUMP_POLL_OPERATOR_EQUAL,
					0,
					MAKEUNIQUETAG(psSGXHostCtlMemInfo));
		#endif

		psSGXHostCtl->ui32CleanupStatus &= ~(PVRSRV_USSE_EDM_CLEANUPCMD_COMPLETE);
		PDUMPMEM(IMG_NULL, psSGXHostCtlMemInfo, offsetof(SGXMKIF_HOST_CTL, ui32CleanupStatus), sizeof(IMG_UINT32), 0, MAKEUNIQUETAG(psSGXHostCtlMemInfo));

	#if defined(SGX_FEATURE_SYSTEM_CACHE)
		psSGXDevInfo->ui32CacheControl |= (SGXMKIF_CC_INVAL_BIF_SL | SGXMKIF_CC_INVAL_DATA);
	#else
		psSGXDevInfo->ui32CacheControl |= SGXMKIF_CC_INVAL_DATA;
	#endif
	}
}
}


typedef struct _SGX_HW_RENDER_CONTEXT_CLEANUP_
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
	IMG_DEV_VIRTADDR sHWRenderContextDevVAddr;
	IMG_HANDLE hBlockAlloc;
	PRESMAN_ITEM psResItem;
} SGX_HW_RENDER_CONTEXT_CLEANUP;


static PVRSRV_ERROR SGXCleanupHWRenderContextCallback(IMG_PVOID		pvParam,
													  IMG_UINT32	ui32Param)
{
	SGX_HW_RENDER_CONTEXT_CLEANUP *psCleanup = pvParam;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	SGXCleanupRequest(psCleanup->psDeviceNode,
					  &psCleanup->sHWRenderContextDevVAddr,
					  PVRSRV_CLEANUPCMD_RC);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(SGX_HW_RENDER_CONTEXT_CLEANUP),
			  psCleanup,
			  psCleanup->hBlockAlloc);


	return PVRSRV_OK;
}

typedef struct _SGX_HW_TRANSFER_CONTEXT_CLEANUP_
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
	IMG_DEV_VIRTADDR sHWTransferContextDevVAddr;
	IMG_HANDLE hBlockAlloc;
	PRESMAN_ITEM psResItem;
} SGX_HW_TRANSFER_CONTEXT_CLEANUP;


static PVRSRV_ERROR SGXCleanupHWTransferContextCallback(IMG_PVOID	pvParam,
														IMG_UINT32	ui32Param)
{
	SGX_HW_TRANSFER_CONTEXT_CLEANUP *psCleanup = (SGX_HW_TRANSFER_CONTEXT_CLEANUP *)pvParam;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	SGXCleanupRequest(psCleanup->psDeviceNode,
					  &psCleanup->sHWTransferContextDevVAddr,
					  PVRSRV_CLEANUPCMD_TC);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(SGX_HW_TRANSFER_CONTEXT_CLEANUP),
			  psCleanup,
			  psCleanup->hBlockAlloc);


	return PVRSRV_OK;
}

IMG_EXPORT
IMG_HANDLE SGXRegisterHWRenderContextKM(IMG_HANDLE				psDeviceNode,
										IMG_DEV_VIRTADDR		*psHWRenderContextDevVAddr,
										PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_ERROR eError;
	IMG_HANDLE hBlockAlloc;
	SGX_HW_RENDER_CONTEXT_CLEANUP *psCleanup;
	PRESMAN_ITEM psResItem;

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof(SGX_HW_RENDER_CONTEXT_CLEANUP),
						(IMG_VOID **)&psCleanup,
						&hBlockAlloc,
						"SGX Hardware Render Context Cleanup");

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWRenderContextKM: Couldn't allocate memory for SGX_HW_RENDER_CONTEXT_CLEANUP structure"));
		return IMG_NULL;
	}

	psCleanup->hBlockAlloc = hBlockAlloc;
	psCleanup->psDeviceNode = psDeviceNode;
	psCleanup->sHWRenderContextDevVAddr = *psHWRenderContextDevVAddr;

	psResItem = ResManRegisterRes(psPerProc->hResManContext,
								  RESMAN_TYPE_HW_RENDER_CONTEXT,
								  (IMG_VOID *)psCleanup,
								  0,
								  &SGXCleanupHWRenderContextCallback);

	if (psResItem == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWRenderContextKM: ResManRegisterRes failed"));
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(SGX_HW_RENDER_CONTEXT_CLEANUP),
				  psCleanup,
				  psCleanup->hBlockAlloc);


		return IMG_NULL;
	}

	psCleanup->psResItem = psResItem;

	return (IMG_HANDLE)psCleanup;
}

IMG_EXPORT
PVRSRV_ERROR SGXUnregisterHWRenderContextKM(IMG_HANDLE hHWRenderContext)
{
	PVRSRV_ERROR eError;
	SGX_HW_RENDER_CONTEXT_CLEANUP *psCleanup;

	PVR_ASSERT(hHWRenderContext != IMG_NULL);

	psCleanup = (SGX_HW_RENDER_CONTEXT_CLEANUP *)hHWRenderContext;

	if (psCleanup == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnregisterHWRenderContextKM: invalid parameter"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = ResManFreeResByPtr(psCleanup->psResItem);

	return eError;
}


IMG_EXPORT
IMG_HANDLE SGXRegisterHWTransferContextKM(IMG_HANDLE				psDeviceNode,
										  IMG_DEV_VIRTADDR			*psHWTransferContextDevVAddr,
										  PVRSRV_PER_PROCESS_DATA	*psPerProc)
{
	PVRSRV_ERROR eError;
	IMG_HANDLE hBlockAlloc;
	SGX_HW_TRANSFER_CONTEXT_CLEANUP *psCleanup;
	PRESMAN_ITEM psResItem;

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof(SGX_HW_TRANSFER_CONTEXT_CLEANUP),
						(IMG_VOID **)&psCleanup,
						&hBlockAlloc,
						"SGX Hardware Transfer Context Cleanup");

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWTransferContextKM: Couldn't allocate memory for SGX_HW_TRANSFER_CONTEXT_CLEANUP structure"));
		return IMG_NULL;
	}

	psCleanup->hBlockAlloc = hBlockAlloc;
	psCleanup->psDeviceNode = psDeviceNode;
	psCleanup->sHWTransferContextDevVAddr = *psHWTransferContextDevVAddr;

	psResItem = ResManRegisterRes(psPerProc->hResManContext,
								  RESMAN_TYPE_HW_TRANSFER_CONTEXT,
								  psCleanup,
								  0,
								  &SGXCleanupHWTransferContextCallback);

	if (psResItem == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHWTransferContextKM: ResManRegisterRes failed"));
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(SGX_HW_TRANSFER_CONTEXT_CLEANUP),
				  psCleanup,
				  psCleanup->hBlockAlloc);


		return IMG_NULL;
	}

	psCleanup->psResItem = psResItem;

	return (IMG_HANDLE)psCleanup;
}

IMG_EXPORT
PVRSRV_ERROR SGXUnregisterHWTransferContextKM(IMG_HANDLE hHWTransferContext)
{
	PVRSRV_ERROR eError;
	SGX_HW_TRANSFER_CONTEXT_CLEANUP *psCleanup;

	PVR_ASSERT(hHWTransferContext != IMG_NULL);

	psCleanup = (SGX_HW_TRANSFER_CONTEXT_CLEANUP *)hHWTransferContext;

	if (psCleanup == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXUnregisterHWTransferContextKM: invalid parameter"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = ResManFreeResByPtr(psCleanup->psResItem);

	return eError;
}

#if defined(SGX_FEATURE_2D_HARDWARE)
typedef struct _SGX_HW_2D_CONTEXT_CLEANUP_
{
	PVRSRV_DEVICE_NODE *psDeviceNode;
	IMG_DEV_VIRTADDR sHW2DContextDevVAddr;
	IMG_HANDLE hBlockAlloc;
	PRESMAN_ITEM psResItem;
} SGX_HW_2D_CONTEXT_CLEANUP;

static PVRSRV_ERROR SGXCleanupHW2DContextCallback(IMG_PVOID pvParam, IMG_UINT32 ui32Param)
{
	SGX_HW_2D_CONTEXT_CLEANUP *psCleanup = (SGX_HW_2D_CONTEXT_CLEANUP *)pvParam;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	SGXCleanupRequest(psCleanup->psDeviceNode,
					  &psCleanup->sHW2DContextDevVAddr,
					  PVRSRV_CLEANUPCMD_2DC);

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			  sizeof(SGX_HW_2D_CONTEXT_CLEANUP),
			  psCleanup,
			  psCleanup->hBlockAlloc);


	return PVRSRV_OK;
}

IMG_EXPORT
IMG_HANDLE SGXRegisterHW2DContextKM(IMG_HANDLE				psDeviceNode,
									IMG_DEV_VIRTADDR		*psHW2DContextDevVAddr,
									PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVRSRV_ERROR eError;
	IMG_HANDLE hBlockAlloc;
	SGX_HW_2D_CONTEXT_CLEANUP *psCleanup;
	PRESMAN_ITEM psResItem;

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof(SGX_HW_2D_CONTEXT_CLEANUP),
						(IMG_VOID **)&psCleanup,
						&hBlockAlloc,
						"SGX Hardware 2D Context Cleanup");

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHW2DContextKM: Couldn't allocate memory for SGX_HW_2D_CONTEXT_CLEANUP structure"));
		return IMG_NULL;
	}

	psCleanup->hBlockAlloc = hBlockAlloc;
	psCleanup->psDeviceNode = psDeviceNode;
	psCleanup->sHW2DContextDevVAddr = *psHW2DContextDevVAddr;

	psResItem = ResManRegisterRes(psPerProc->hResManContext,
								  RESMAN_TYPE_HW_2D_CONTEXT,
								  psCleanup,
								  0,
								  &SGXCleanupHW2DContextCallback);

	if (psResItem == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXRegisterHW2DContextKM: ResManRegisterRes failed"));
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				  sizeof(SGX_HW_2D_CONTEXT_CLEANUP),
				  psCleanup,
				  psCleanup->hBlockAlloc);


		return IMG_NULL;
	}

	psCleanup->psResItem = psResItem;

	return (IMG_HANDLE)psCleanup;
}

IMG_EXPORT
PVRSRV_ERROR SGXUnregisterHW2DContextKM(IMG_HANDLE hHW2DContext)
{
	PVRSRV_ERROR eError;
	SGX_HW_2D_CONTEXT_CLEANUP *psCleanup;

	PVR_ASSERT(hHW2DContext != IMG_NULL);

	if (hHW2DContext == IMG_NULL)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psCleanup = (SGX_HW_2D_CONTEXT_CLEANUP *)hHW2DContext;

	eError = ResManFreeResByPtr(psCleanup->psResItem);

	return eError;
}
#endif

/*!****************************************************************************
 @Function	SGX2DQuerySyncOpsCompleteKM

 @Input		psSyncInfo : Sync object to be queried

 @Return	IMG_TRUE - ops complete, IMG_FALSE - ops pending

******************************************************************************/
#ifdef INLINE_IS_PRAGMA
#pragma inline(SGX2DQuerySyncOpsComplete)
#endif
static INLINE
IMG_BOOL SGX2DQuerySyncOpsComplete(PVRSRV_KERNEL_SYNC_INFO	*psSyncInfo,
								   IMG_UINT32				ui32ReadOpsPending,
								   IMG_UINT32				ui32WriteOpsPending)
{
	PVRSRV_SYNC_DATA *psSyncData = psSyncInfo->psSyncData;

	return (IMG_BOOL)(
					  (psSyncData->ui32ReadOpsComplete >= ui32ReadOpsPending) &&
					  (psSyncData->ui32WriteOpsComplete >= ui32WriteOpsPending)
					 );
}

/*!****************************************************************************
 @Function	SGX2DQueryBlitsCompleteKM

 @Input		psDevInfo : SGX device info structure

 @Input		psSyncInfo : Sync object to be queried

 @Return	PVRSRV_ERROR

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR SGX2DQueryBlitsCompleteKM(PVRSRV_SGXDEV_INFO	*psDevInfo,
									   PVRSRV_KERNEL_SYNC_INFO *psSyncInfo,
									   IMG_BOOL bWaitForComplete)
{
	IMG_UINT32	ui32ReadOpsPending, ui32WriteOpsPending;

	PVR_UNREFERENCED_PARAMETER(psDevInfo);

	PVR_DPF((PVR_DBG_CALLTRACE, "SGX2DQueryBlitsCompleteKM: Start"));

	ui32ReadOpsPending = psSyncInfo->psSyncData->ui32ReadOpsPending;
	ui32WriteOpsPending = psSyncInfo->psSyncData->ui32WriteOpsPending;

	if(SGX2DQuerySyncOpsComplete(psSyncInfo, ui32ReadOpsPending, ui32WriteOpsPending))
	{
		/* Instant success */
		PVR_DPF((PVR_DBG_CALLTRACE, "SGX2DQueryBlitsCompleteKM: No wait. Blits complete."));
		return PVRSRV_OK;
	}

	/* Not complete yet */
	if (!bWaitForComplete)
	{
		/* Just report not complete */
		PVR_DPF((PVR_DBG_CALLTRACE, "SGX2DQueryBlitsCompleteKM: No wait. Ops pending."));
		return PVRSRV_ERROR_CMD_NOT_PROCESSED;
	}

	/* Start polling */
	PVR_DPF((PVR_DBG_MESSAGE, "SGX2DQueryBlitsCompleteKM: Ops pending. Start polling."));

	LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US)
	{
		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);

		if(SGX2DQuerySyncOpsComplete(psSyncInfo, ui32ReadOpsPending, ui32WriteOpsPending))
		{
			/* Success */
			PVR_DPF((PVR_DBG_CALLTRACE, "SGX2DQueryBlitsCompleteKM: Wait over.  Blits complete."));
			return PVRSRV_OK;
		}

		OSWaitus(MAX_HW_TIME_US/WAIT_TRY_COUNT);
	} END_LOOP_UNTIL_TIMEOUT();

	/* Timed out */
	PVR_DPF((PVR_DBG_ERROR,"SGX2DQueryBlitsCompleteKM: Timed out. Ops pending."));

#if defined(DEBUG)
	{
		PVRSRV_SYNC_DATA *psSyncData = psSyncInfo->psSyncData;

		PVR_TRACE(("SGX2DQueryBlitsCompleteKM: Syncinfo: %p, Syncdata: %p", psSyncInfo, psSyncData));

		PVR_TRACE(("SGX2DQueryBlitsCompleteKM: Read ops complete: %d, Read ops pending: %d", psSyncData->ui32ReadOpsComplete, psSyncData->ui32ReadOpsPending));
		PVR_TRACE(("SGX2DQueryBlitsCompleteKM: Write ops complete: %d, Write ops pending: %d", psSyncData->ui32WriteOpsComplete, psSyncData->ui32WriteOpsPending));

	}
#endif

	return PVRSRV_ERROR_TIMEOUT;
}


IMG_EXPORT
IMG_VOID SGXFlushHWRenderTargetKM(IMG_HANDLE psDeviceNode, IMG_DEV_VIRTADDR sHWRTDataSetDevVAddr)
{
	PVR_ASSERT(sHWRTDataSetDevVAddr.uiAddr != IMG_NULL);

	SGXCleanupRequest(psDeviceNode,
					  &sHWRTDataSetDevVAddr,
					  PVRSRV_CLEANUPCMD_RT);
}


IMG_UINT32 SGXConvertTimeStamp(PVRSRV_SGXDEV_INFO	*psDevInfo,
							   IMG_UINT32			ui32TimeWraps,
							   IMG_UINT32			ui32Time)
{
#if defined(EUR_CR_TIMER)
	PVR_UNREFERENCED_PARAMETER(psDevInfo);
	PVR_UNREFERENCED_PARAMETER(ui32TimeWraps);
	return ui32Time;
#else
	IMG_UINT64	ui64Clocks;
	IMG_UINT32	ui32Clocksx16;

	ui64Clocks = ((IMG_UINT64)ui32TimeWraps * psDevInfo->ui32uKernelTimerClock) +
					(psDevInfo->ui32uKernelTimerClock - (ui32Time & EUR_CR_EVENT_TIMER_VALUE_MASK));
	ui32Clocksx16 = (IMG_UINT32)(ui64Clocks / 16);

	return ui32Clocksx16;
#endif /* EUR_CR_TIMER */
}





/******************************************************************************
 End of file (sgxutils.c)
******************************************************************************/
