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

#include <stddef.h>

#include "sgxdefs.h"
#include "sgxmmu.h"
#include "services_headers.h"
#include "buffer_manager.h"
#include "sgxapi_km.h"
#include "sgxinfo.h"
#include "sgx_mkif_km.h"
#include "sgxconfig.h"
#include "sysconfig.h"
#include "pvr_bridge_km.h"

#include "sgx_bridge_km.h"

#include "pdump_km.h"
#include "ra.h"
#include "mmu.h"
#include "handle.h"
#include "perproc.h"

#include "sgxutils.h"
#include "pvrversion.h"
#include "sgx_options.h"

#include "lists.h"
#include "srvkm.h"

DECLARE_LIST_ANY_VA(PVRSRV_POWER_DEV);

#if defined(SUPPORT_SGX_HWPERF)
IMG_VOID* MatchPowerDeviceIndex_AnyVaCb(PVRSRV_POWER_DEV *psPowerDev, va_list va);
#endif

#define VAR(x) #x

#define CHECK_SIZE(NAME) \
{	\
	if (psSGXStructSizes->ui32Sizeof_##NAME != psDevInfo->sSGXStructSizes.ui32Sizeof_##NAME) \
	{	\
		PVR_DPF((PVR_DBG_ERROR, "SGXDevInitCompatCheck: Size check failed for SGXMKIF_%s (client) = %d bytes, (ukernel) = %d bytes\n", \
			VAR(NAME), \
			psDevInfo->sSGXStructSizes.ui32Sizeof_##NAME, \
			psSGXStructSizes->ui32Sizeof_##NAME )); \
		bStructSizesFailed = IMG_TRUE; \
	}	\
}

#if defined (SYS_USING_INTERRUPTS)
IMG_BOOL SGX_ISRHandler(IMG_VOID *pvData);
#endif

IMG_UINT32 gui32EventStatusServicesByISR = 0;


static
PVRSRV_ERROR SGXGetMiscInfoUkernel(PVRSRV_SGXDEV_INFO	*psDevInfo,
								   PVRSRV_DEVICE_NODE 	*psDeviceNode);


static IMG_VOID SGXCommandComplete(PVRSRV_DEVICE_NODE *psDeviceNode)
{
#if defined(OS_SUPPORTS_IN_LISR)
	if (OSInLISR(psDeviceNode->psSysData))
	{

		psDeviceNode->bReProcessDeviceCommandComplete = IMG_TRUE;
	}
	else
	{
		SGXScheduleProcessQueuesKM(psDeviceNode);
	}
#else
	SGXScheduleProcessQueuesKM(psDeviceNode);
#endif
}

static IMG_UINT32 DeinitDevInfo(PVRSRV_SGXDEV_INFO *psDevInfo)
{
	if (psDevInfo->psKernelCCBInfo != IMG_NULL)
	{


		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(PVRSRV_SGX_CCB_INFO), psDevInfo->psKernelCCBInfo, IMG_NULL);
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR InitDevInfo(PVRSRV_PER_PROCESS_DATA *psPerProc,
								PVRSRV_DEVICE_NODE *psDeviceNode,
								SGX_BRIDGE_INIT_INFO *psInitInfo)
{
	PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;
	PVRSRV_ERROR		eError;

	PVRSRV_SGX_CCB_INFO	*psKernelCCBInfo = IMG_NULL;

	PVR_UNREFERENCED_PARAMETER(psPerProc);
	psDevInfo->sScripts = psInitInfo->sScripts;

	psDevInfo->psKernelCCBMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelCCBMemInfo;
	psDevInfo->psKernelCCB = (PVRSRV_SGX_KERNEL_CCB *) psDevInfo->psKernelCCBMemInfo->pvLinAddrKM;

	psDevInfo->psKernelCCBCtlMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelCCBCtlMemInfo;
	psDevInfo->psKernelCCBCtl = (PVRSRV_SGX_CCB_CTL *) psDevInfo->psKernelCCBCtlMemInfo->pvLinAddrKM;

	psDevInfo->psKernelCCBEventKickerMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelCCBEventKickerMemInfo;
	psDevInfo->pui32KernelCCBEventKicker = (IMG_UINT32 *)psDevInfo->psKernelCCBEventKickerMemInfo->pvLinAddrKM;

	psDevInfo->psKernelSGXHostCtlMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelSGXHostCtlMemInfo;
	psDevInfo->psSGXHostCtl = (SGXMKIF_HOST_CTL *)psDevInfo->psKernelSGXHostCtlMemInfo->pvLinAddrKM;

	psDevInfo->psKernelSGXTA3DCtlMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelSGXTA3DCtlMemInfo;

	psDevInfo->psKernelSGXMiscMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelSGXMiscMemInfo;

#if defined(SGX_SUPPORT_HWPROFILING)
	psDevInfo->psKernelHWProfilingMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelHWProfilingMemInfo;
#endif
#if defined(SUPPORT_SGX_HWPERF)
	psDevInfo->psKernelHWPerfCBMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelHWPerfCBMemInfo;
#endif
#ifdef PVRSRV_USSE_EDM_STATUS_DEBUG
	psDevInfo->psKernelEDMStatusBufferMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelEDMStatusBufferMemInfo;
#endif
#if defined(SGX_FEATURE_OVERLAPPED_SPM)
	psDevInfo->psKernelTmpRgnHeaderMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelTmpRgnHeaderMemInfo;
#endif
#if defined(SGX_FEATURE_SPM_MODE_0)
	psDevInfo->psKernelTmpDPMStateMemInfo = (PVRSRV_KERNEL_MEM_INFO *)psInitInfo->hKernelTmpDPMStateMemInfo;
#endif

	psDevInfo->ui32ClientBuildOptions = psInitInfo->ui32ClientBuildOptions;


	psDevInfo->sSGXStructSizes = psInitInfo->sSGXStructSizes;



	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
						sizeof(PVRSRV_SGX_CCB_INFO),
						(IMG_VOID **)&psKernelCCBInfo, 0,
						"SGX Circular Command Buffer Info");
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"InitDevInfo: Failed to alloc memory"));
		goto failed_allockernelccb;
	}


	OSMemSet(psKernelCCBInfo, 0, sizeof(PVRSRV_SGX_CCB_INFO));
	psKernelCCBInfo->psCCBMemInfo		= psDevInfo->psKernelCCBMemInfo;
	psKernelCCBInfo->psCCBCtlMemInfo	= psDevInfo->psKernelCCBCtlMemInfo;
	psKernelCCBInfo->psCommands			= psDevInfo->psKernelCCB->asCommands;
	psKernelCCBInfo->pui32WriteOffset	= &psDevInfo->psKernelCCBCtl->ui32WriteOffset;
	psKernelCCBInfo->pui32ReadOffset	= &psDevInfo->psKernelCCBCtl->ui32ReadOffset;
	psDevInfo->psKernelCCBInfo = psKernelCCBInfo;



	OSMemCopy(psDevInfo->aui32HostKickAddr, psInitInfo->aui32HostKickAddr,
			  SGXMKIF_CMD_MAX * sizeof(psDevInfo->aui32HostKickAddr[0]));

	psDevInfo->bForcePTOff = IMG_FALSE;

	psDevInfo->ui32CacheControl = psInitInfo->ui32CacheControl;

	psDevInfo->ui32EDMTaskReg0 = psInitInfo->ui32EDMTaskReg0;
	psDevInfo->ui32EDMTaskReg1 = psInitInfo->ui32EDMTaskReg1;
	psDevInfo->ui32ClkGateStatusReg = psInitInfo->ui32ClkGateStatusReg;
	psDevInfo->ui32ClkGateStatusMask = psInitInfo->ui32ClkGateStatusMask;
#if defined(SGX_FEATURE_MP)
	psDevInfo->ui32MasterClkGateStatusReg = psInitInfo->ui32MasterClkGateStatusReg;
	psDevInfo->ui32MasterClkGateStatusMask = psInitInfo->ui32MasterClkGateStatusMask;
#endif



	OSMemCopy(&psDevInfo->asSGXDevData,  &psInitInfo->asInitDevData, sizeof(psDevInfo->asSGXDevData));

	return PVRSRV_OK;

failed_allockernelccb:
	DeinitDevInfo(psDevInfo);

	return eError;
}




static PVRSRV_ERROR SGXRunScript(PVRSRV_SGXDEV_INFO *psDevInfo, SGX_INIT_COMMAND *psScript, IMG_UINT32 ui32NumInitCommands)
{
	IMG_UINT32 ui32PC;
	SGX_INIT_COMMAND *psComm;

	for (ui32PC = 0, psComm = psScript;
		ui32PC < ui32NumInitCommands;
		ui32PC++, psComm++)
	{
		switch (psComm->eOp)
		{
			case SGX_INIT_OP_WRITE_HW_REG:
			{
				OSWriteHWReg(psDevInfo->pvRegsBaseKM, psComm->sWriteHWReg.ui32Offset, psComm->sWriteHWReg.ui32Value);
				PDUMPREG(psComm->sWriteHWReg.ui32Offset, psComm->sWriteHWReg.ui32Value);
				break;
			}
#if defined(PDUMP)
			case SGX_INIT_OP_PDUMP_HW_REG:
			{
				PDUMPREG(psComm->sPDumpHWReg.ui32Offset, psComm->sPDumpHWReg.ui32Value);
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
				PVR_DPF((PVR_DBG_ERROR,"SGXRunScript: PC %d: Illegal command: %d", ui32PC, psComm->eOp));
				return PVRSRV_ERROR_GENERIC;
			}
		}

	}

	return PVRSRV_ERROR_GENERIC;
}

PVRSRV_ERROR SGXInitialise(PVRSRV_SGXDEV_INFO	*psDevInfo)
{
	PVRSRV_ERROR			eError;
	PVRSRV_KERNEL_MEM_INFO	*psSGXHostCtlMemInfo = psDevInfo->psKernelSGXHostCtlMemInfo;
	SGXMKIF_HOST_CTL		*psSGXHostCtl = psSGXHostCtlMemInfo->pvLinAddrKM;
#if defined(PDUMP)
	static IMG_BOOL			bFirstTime = IMG_TRUE;
#endif



	PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS, "SGX initialisation script part 1\n");
	eError = SGXRunScript(psDevInfo, psDevInfo->sScripts.asInitCommandsPart1, SGX_MAX_INIT_COMMANDS);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXInitialise: SGXRunScript (part 1) failed (%d)", eError));
		return (PVRSRV_ERROR_GENERIC);
	}
	PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS, "End of SGX initialisation script part 1\n");


	SGXReset(psDevInfo, PDUMP_FLAGS_CONTINUOUS);

#if defined(EUR_CR_POWER)
#if defined(SGX531)





	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_POWER, 1);
	PDUMPREG(EUR_CR_POWER, 1);
#else

	OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_POWER, 0);
	PDUMPREG(EUR_CR_POWER, 0);
#endif
#endif


	*psDevInfo->pui32KernelCCBEventKicker = 0;
#if defined(PDUMP)
	if (bFirstTime)
	{
		psDevInfo->ui32KernelCCBEventKickerDumpVal = 0;
		PDUMPMEM(&psDevInfo->ui32KernelCCBEventKickerDumpVal,
				 psDevInfo->psKernelCCBEventKickerMemInfo, 0,
				 sizeof(*psDevInfo->pui32KernelCCBEventKicker), PDUMP_FLAGS_CONTINUOUS,
				 MAKEUNIQUETAG(psDevInfo->psKernelCCBEventKickerMemInfo));
	}
#endif



	PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS, "SGX initialisation script part 2\n");
	eError = SGXRunScript(psDevInfo, psDevInfo->sScripts.asInitCommandsPart2, SGX_MAX_INIT_COMMANDS);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXInitialise: SGXRunScript (part 2) failed (%d)", eError));
		return (PVRSRV_ERROR_GENERIC);
	}
	PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS, "End of SGX initialisation script part 2\n");


	psSGXHostCtl->ui32InitStatus = 0;
#if defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS,
						  "Reset the SGX microkernel initialisation status\n");
	PDUMPMEM(IMG_NULL, psSGXHostCtlMemInfo,
			 offsetof(SGXMKIF_HOST_CTL, ui32InitStatus),
			 sizeof(IMG_UINT32), PDUMP_FLAGS_CONTINUOUS,
			 MAKEUNIQUETAG(psSGXHostCtlMemInfo));
#endif

#if defined(SGX_FEATURE_MULTI_EVENT_KICK)
	OSWriteHWReg(psDevInfo->pvRegsBaseKM,
				 SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK2, 0),
				 EUR_CR_EVENT_KICK2_NOW_MASK);
#else
	*psDevInfo->pui32KernelCCBEventKicker = (*psDevInfo->pui32KernelCCBEventKicker + 1) & 0xFF;
	OSWriteHWReg(psDevInfo->pvRegsBaseKM,
				 SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK, 0),
				 EUR_CR_EVENT_KICK_NOW_MASK);
#endif

#if defined(PDUMP)






	if (bFirstTime)
	{
#if defined(SGX_FEATURE_MULTI_EVENT_KICK)
		PDUMPREG(SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK2, 0), EUR_CR_EVENT_KICK2_NOW_MASK);
#else
		psDevInfo->ui32KernelCCBEventKickerDumpVal = 1;
		PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS,
							  "First increment of the SGX event kicker value\n");
		PDUMPMEM(&psDevInfo->ui32KernelCCBEventKickerDumpVal,
				 psDevInfo->psKernelCCBEventKickerMemInfo,
				 0,
				 sizeof(IMG_UINT32),
				 PDUMP_FLAGS_CONTINUOUS,
				 MAKEUNIQUETAG(psDevInfo->psKernelCCBEventKickerMemInfo));
		PDUMPREG(SGX_MP_CORE_SELECT(EUR_CR_EVENT_KICK, 0), EUR_CR_EVENT_KICK_NOW_MASK);
#endif
		bFirstTime = IMG_FALSE;
	}
#endif

#if !defined(NO_HARDWARE)


	if (PollForValueKM(&psSGXHostCtl->ui32InitStatus,
					   PVRSRV_USSE_EDM_INIT_COMPLETE,
					   PVRSRV_USSE_EDM_INIT_COMPLETE,
					   MAX_HW_TIME_US/WAIT_TRY_COUNT,
					   WAIT_TRY_COUNT) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXInitialise: Wait for uKernel initialisation failed"));
		PVR_DBG_BREAK;
		return PVRSRV_ERROR_RETRY;
	}
#endif

#if defined(PDUMP)
	PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS,
						  "Wait for the SGX microkernel initialisation to complete");
	PDUMPMEMPOL(psSGXHostCtlMemInfo,
				offsetof(SGXMKIF_HOST_CTL, ui32InitStatus),
				PVRSRV_USSE_EDM_INIT_COMPLETE,
				PVRSRV_USSE_EDM_INIT_COMPLETE,
				PDUMP_POLL_OPERATOR_EQUAL,
				PDUMP_FLAGS_CONTINUOUS,
				MAKEUNIQUETAG(psSGXHostCtlMemInfo));
#endif

#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030) && defined(SGX_FEATURE_HOST_PORT)



	WorkaroundBRN22997ReadHostPort(psDevInfo);
#endif

	PVR_ASSERT(psDevInfo->psKernelCCBCtl->ui32ReadOffset == psDevInfo->psKernelCCBCtl->ui32WriteOffset);

	return PVRSRV_OK;
}

PVRSRV_ERROR SGXDeinitialise(IMG_HANDLE hDevCookie)

{
	PVRSRV_SGXDEV_INFO	*psDevInfo = (PVRSRV_SGXDEV_INFO *) hDevCookie;
	PVRSRV_ERROR		eError;


	if (psDevInfo->pvRegsBaseKM == IMG_NULL)
	{
		return PVRSRV_OK;
	}

	eError = SGXRunScript(psDevInfo, psDevInfo->sScripts.asDeinitCommands, SGX_MAX_DEINIT_COMMANDS);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXDeinitialise: SGXRunScript failed (%d)", eError));
		return (PVRSRV_ERROR_GENERIC);
	}

	return PVRSRV_OK;
}


static PVRSRV_ERROR DevInitSGXPart1 (IMG_VOID *pvDeviceNode)
{
	PVRSRV_SGXDEV_INFO	*psDevInfo;
	IMG_HANDLE		hKernelDevMemContext;
	IMG_DEV_PHYADDR		sPDDevPAddr;
	IMG_UINT32		i;
	PVRSRV_DEVICE_NODE  *psDeviceNode = (PVRSRV_DEVICE_NODE *)pvDeviceNode;
	DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap = psDeviceNode->sDevMemoryInfo.psDeviceMemoryHeap;
	PVRSRV_ERROR		eError;

	PDUMPCOMMENT("SGX Initialisation Part 1");


	PDUMPCOMMENT("SGX Core Version Information: %s", SGX_CORE_FRIENDLY_NAME);
#ifdef SGX_CORE_REV
	PDUMPCOMMENT("SGX Core Revision Information: %d", SGX_CORE_REV);
#else
	PDUMPCOMMENT("SGX Core Revision Information: head rtl");
#endif

	#if defined(SGX_FEATURE_SYSTEM_CACHE)
	PDUMPCOMMENT("SGX System Level Cache is present\r\n");
	#if defined(SGX_BYPASS_SYSTEM_CACHE)
	PDUMPCOMMENT("SGX System Level Cache is bypassed\r\n");
	#endif
	#endif


	if(OSAllocMem( PVRSRV_OS_NON_PAGEABLE_HEAP,
					 sizeof(PVRSRV_SGXDEV_INFO),
					 (IMG_VOID **)&psDevInfo, IMG_NULL,
					 "SGX Device Info") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"DevInitSGXPart1 : Failed to alloc memory for DevInfo"));
		return (PVRSRV_ERROR_OUT_OF_MEMORY);
	}
	OSMemSet (psDevInfo, 0, sizeof(PVRSRV_SGXDEV_INFO));


	psDevInfo->eDeviceType 		= DEV_DEVICE_TYPE;
	psDevInfo->eDeviceClass 	= DEV_DEVICE_CLASS;


	psDeviceNode->pvDevice = (IMG_PVOID)psDevInfo;


	psDevInfo->pvDeviceMemoryHeap = (IMG_VOID*)psDeviceMemoryHeap;


	hKernelDevMemContext = BM_CreateContext(psDeviceNode,
											&sPDDevPAddr,
											IMG_NULL,
											IMG_NULL);
	if (hKernelDevMemContext == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"DevInitSGXPart1: Failed BM_CreateContext"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	psDevInfo->sKernelPDDevPAddr = sPDDevPAddr;


	for(i=0; i<psDeviceNode->sDevMemoryInfo.ui32HeapCount; i++)
	{
		IMG_HANDLE hDevMemHeap;

		switch(psDeviceMemoryHeap[i].DevMemHeapType)
		{
			case DEVICE_MEMORY_HEAP_KERNEL:
			case DEVICE_MEMORY_HEAP_SHARED:
			case DEVICE_MEMORY_HEAP_SHARED_EXPORTED:
			{
				hDevMemHeap = BM_CreateHeap (hKernelDevMemContext,
												&psDeviceMemoryHeap[i]);



				psDeviceMemoryHeap[i].hDevMemHeap = hDevMemHeap;
				break;
			}
		}
	}

	eError = MMU_BIFResetPDAlloc(psDevInfo);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"DevInitSGX : Failed to alloc memory for BIF reset"));
		return PVRSRV_ERROR_GENERIC;
	}

	return PVRSRV_OK;
}

IMG_EXPORT
PVRSRV_ERROR SGXGetInfoForSrvinitKM(IMG_HANDLE hDevHandle, SGX_BRIDGE_INFO_FOR_SRVINIT *psInitInfo)
{
	PVRSRV_DEVICE_NODE	*psDeviceNode;
	PVRSRV_SGXDEV_INFO	*psDevInfo;
	PVRSRV_ERROR		eError;

	PDUMPCOMMENT("SGXGetInfoForSrvinit");

	psDeviceNode = (PVRSRV_DEVICE_NODE *)hDevHandle;
	psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;

	psInitInfo->sPDDevPAddr = psDevInfo->sKernelPDDevPAddr;

	eError = PVRSRVGetDeviceMemHeapsKM(hDevHandle, &psInitInfo->asHeapInfo[0]);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXGetInfoForSrvinit: PVRSRVGetDeviceMemHeapsKM failed (%d)", eError));
		return PVRSRV_ERROR_GENERIC;
	}

	return eError;
}

IMG_EXPORT
PVRSRV_ERROR DevInitSGXPart2KM (PVRSRV_PER_PROCESS_DATA *psPerProc,
                                IMG_HANDLE hDevHandle,
                                SGX_BRIDGE_INIT_INFO *psInitInfo)
{
	PVRSRV_DEVICE_NODE		*psDeviceNode;
	PVRSRV_SGXDEV_INFO		*psDevInfo;
	PVRSRV_ERROR			eError;
	SGX_DEVICE_MAP			*psSGXDeviceMap;
	PVRSRV_DEV_POWER_STATE	eDefaultPowerState;

	PDUMPCOMMENT("SGX Initialisation Part 2");

	psDeviceNode = (PVRSRV_DEVICE_NODE *)hDevHandle;
	psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;



	eError = InitDevInfo(psPerProc, psDeviceNode, psInitInfo);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"DevInitSGXPart2KM: Failed to load EDM program"));
		goto failed_init_dev_info;
	}


	eError = SysGetDeviceMemoryMap(PVRSRV_DEVICE_TYPE_SGX,
									(IMG_VOID**)&psSGXDeviceMap);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"DevInitSGXPart2KM: Failed to get device memory map!"));
		return PVRSRV_ERROR_INIT_FAILURE;
	}


	if (psSGXDeviceMap->pvRegsCpuVBase)
	{
		psDevInfo->pvRegsBaseKM = psSGXDeviceMap->pvRegsCpuVBase;
	}
	else
	{

		psDevInfo->pvRegsBaseKM = OSMapPhysToLin(psSGXDeviceMap->sRegsCpuPBase,
											   psSGXDeviceMap->ui32RegsSize,
											   PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
											   IMG_NULL);
		if (!psDevInfo->pvRegsBaseKM)
		{
			PVR_DPF((PVR_DBG_ERROR,"DevInitSGXPart2KM: Failed to map in regs\n"));
			return PVRSRV_ERROR_BAD_MAPPING;
		}
	}
	psDevInfo->ui32RegSize = psSGXDeviceMap->ui32RegsSize;
	psDevInfo->sRegsPhysBase = psSGXDeviceMap->sRegsSysPBase;


#if defined(SGX_FEATURE_HOST_PORT)
	if (psSGXDeviceMap->ui32Flags & SGX_HOSTPORT_PRESENT)
	{

		psDevInfo->pvHostPortBaseKM = OSMapPhysToLin(psSGXDeviceMap->sHPCpuPBase,
									  	           psSGXDeviceMap->ui32HPSize,
									  	           PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
									  	           IMG_NULL);
		if (!psDevInfo->pvHostPortBaseKM)
		{
			PVR_DPF((PVR_DBG_ERROR,"DevInitSGXPart2KM: Failed to map in host port\n"));
			return PVRSRV_ERROR_BAD_MAPPING;
		}
		psDevInfo->ui32HPSize = psSGXDeviceMap->ui32HPSize;
		psDevInfo->sHPSysPAddr = psSGXDeviceMap->sHPSysPBase;
	}
#endif

#if defined (SYS_USING_INTERRUPTS)


	psDeviceNode->pvISRData = psDeviceNode;

	PVR_ASSERT(psDeviceNode->pfnDeviceISR == SGX_ISRHandler);

#endif


	psDevInfo->psSGXHostCtl->ui32PowerStatus |= PVRSRV_USSE_EDM_POWMAN_NO_WORK;
	eDefaultPowerState = PVRSRV_DEV_POWER_STATE_OFF;

	eError = PVRSRVRegisterPowerDevice (psDeviceNode->sDevId.ui32DeviceIndex,
										SGXPrePowerState, SGXPostPowerState,
										SGXPreClockSpeedChange, SGXPostClockSpeedChange,
										(IMG_HANDLE)psDeviceNode,
										PVRSRV_DEV_POWER_STATE_OFF,
										eDefaultPowerState);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"DevInitSGXPart2KM: failed to register device with power manager"));
		return eError;
	}

#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030) && defined(SGX_FEATURE_HOST_PORT)
	eError = WorkaroundBRN22997Alloc(psDevInfo);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXInitialise : Failed to alloc memory for BRN22997 workaround"));
		return eError;
	}
#endif

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)

	psDevInfo->ui32ExtSysCacheRegsSize = psSGXDeviceMap->ui32ExtSysCacheRegsSize;
	psDevInfo->sExtSysCacheRegsDevPBase = psSGXDeviceMap->sExtSysCacheRegsDevPBase;
	eError = MMU_MapExtSystemCacheRegs(psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXInitialise : Failed to map external system cache registers"));
		return eError;
	}
#endif



	OSMemSet(psDevInfo->psKernelCCB, 0, sizeof(PVRSRV_SGX_KERNEL_CCB));
	OSMemSet(psDevInfo->psKernelCCBCtl, 0, sizeof(PVRSRV_SGX_CCB_CTL));
	OSMemSet(psDevInfo->pui32KernelCCBEventKicker, 0, sizeof(*psDevInfo->pui32KernelCCBEventKicker));
	PDUMPCOMMENT("Initialise Kernel CCB");
	PDUMPMEM(IMG_NULL, psDevInfo->psKernelCCBMemInfo, 0, sizeof(PVRSRV_SGX_KERNEL_CCB), PDUMP_FLAGS_CONTINUOUS, MAKEUNIQUETAG(psDevInfo->psKernelCCBMemInfo));
	PDUMPCOMMENT("Initialise Kernel CCB Control");
	PDUMPMEM(IMG_NULL, psDevInfo->psKernelCCBCtlMemInfo, 0, sizeof(PVRSRV_SGX_CCB_CTL), PDUMP_FLAGS_CONTINUOUS, MAKEUNIQUETAG(psDevInfo->psKernelCCBCtlMemInfo));
	PDUMPCOMMENT("Initialise Kernel CCB Event Kicker");
	PDUMPMEM(IMG_NULL, psDevInfo->psKernelCCBEventKickerMemInfo, 0, sizeof(*psDevInfo->pui32KernelCCBEventKicker), PDUMP_FLAGS_CONTINUOUS, MAKEUNIQUETAG(psDevInfo->psKernelCCBEventKickerMemInfo));

	return PVRSRV_OK;

failed_init_dev_info:
	return eError;
}

static PVRSRV_ERROR DevDeInitSGX (IMG_VOID *pvDeviceNode)
{
	PVRSRV_DEVICE_NODE			*psDeviceNode = (PVRSRV_DEVICE_NODE *)pvDeviceNode;
	PVRSRV_SGXDEV_INFO			*psDevInfo = (PVRSRV_SGXDEV_INFO*)psDeviceNode->pvDevice;
	PVRSRV_ERROR				eError;
	IMG_UINT32					ui32Heap;
	DEVICE_MEMORY_HEAP_INFO		*psDeviceMemoryHeap;
	SGX_DEVICE_MAP				*psSGXDeviceMap;

	if (!psDevInfo)
	{

		PVR_DPF((PVR_DBG_ERROR,"DevDeInitSGX: Null DevInfo"));
		return PVRSRV_OK;
	}

#if defined(SUPPORT_HW_RECOVERY)
	if (psDevInfo->hTimer)
	{
		eError = OSRemoveTimer(psDevInfo->hTimer);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR,"DevDeInitSGX: Failed to remove timer"));
			return 	eError;
		}
		psDevInfo->hTimer = IMG_NULL;
	}
#endif

#if defined(SUPPORT_EXTERNAL_SYSTEM_CACHE)

	eError = MMU_UnmapExtSystemCacheRegs(psDeviceNode);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"DevDeInitSGX: Failed to unmap ext system cache registers"));
		return eError;
	}
#endif

#if defined(FIX_HW_BRN_22997) && defined(FIX_HW_BRN_23030) && defined(SGX_FEATURE_HOST_PORT)
	WorkaroundBRN22997Free(psDevInfo);
#endif

	MMU_BIFResetPDFree(psDevInfo);




	DeinitDevInfo(psDevInfo);


	psDeviceMemoryHeap = (DEVICE_MEMORY_HEAP_INFO *)psDevInfo->pvDeviceMemoryHeap;
	for(ui32Heap=0; ui32Heap<psDeviceNode->sDevMemoryInfo.ui32HeapCount; ui32Heap++)
	{
		switch(psDeviceMemoryHeap[ui32Heap].DevMemHeapType)
		{
			case DEVICE_MEMORY_HEAP_KERNEL:
			case DEVICE_MEMORY_HEAP_SHARED:
			case DEVICE_MEMORY_HEAP_SHARED_EXPORTED:
			{
				if (psDeviceMemoryHeap[ui32Heap].hDevMemHeap != IMG_NULL)
				{
					BM_DestroyHeap(psDeviceMemoryHeap[ui32Heap].hDevMemHeap);
				}
				break;
			}
		}
	}


	eError = BM_DestroyContext(psDeviceNode->sDevMemoryInfo.pBMKernelContext, IMG_NULL);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"DevDeInitSGX : Failed to destroy kernel context"));
		return eError;
	}


	eError = PVRSRVRemovePowerDevice (((PVRSRV_DEVICE_NODE*)pvDeviceNode)->sDevId.ui32DeviceIndex);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	eError = SysGetDeviceMemoryMap(PVRSRV_DEVICE_TYPE_SGX,
									(IMG_VOID**)&psSGXDeviceMap);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"DevDeInitSGX: Failed to get device memory map!"));
		return eError;
	}


	if (!psSGXDeviceMap->pvRegsCpuVBase)
	{

		if (psDevInfo->pvRegsBaseKM != IMG_NULL)
		{
			OSUnMapPhysToLin(psDevInfo->pvRegsBaseKM,
							 psDevInfo->ui32RegSize,
							 PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
							 IMG_NULL);
		}
	}

#if defined(SGX_FEATURE_HOST_PORT)
	if (psSGXDeviceMap->ui32Flags & SGX_HOSTPORT_PRESENT)
	{

		if (psDevInfo->pvHostPortBaseKM != IMG_NULL)
		{
			OSUnMapPhysToLin(psDevInfo->pvHostPortBaseKM,
						   psDevInfo->ui32HPSize,
						   PVRSRV_HAP_KERNEL_ONLY|PVRSRV_HAP_UNCACHED,
						   IMG_NULL);
		}
	}
#endif



	OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				sizeof(PVRSRV_SGXDEV_INFO),
				psDevInfo,
				0);

	psDeviceNode->pvDevice = IMG_NULL;

	if (psDeviceMemoryHeap != IMG_NULL)
	{

		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
				sizeof(DEVICE_MEMORY_HEAP_INFO) * SGX_MAX_HEAP_ID,
				psDeviceMemoryHeap,
				0);
	}

	return PVRSRV_OK;
}


IMG_VOID SGXDumpDebugInfo (PVRSRV_DEVICE_NODE *psDeviceNode,
						   IMG_BOOL			  bDumpSGXRegs)
{
	IMG_UINT			ui32RegVal;
	PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;

	if (bDumpSGXRegs)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGX Register Base Address (Linear):   0x%08X", psDevInfo->pvRegsBaseKM));
		PVR_DPF((PVR_DBG_ERROR,"SGX Register Base Address (Physical): 0x%08X", psDevInfo->sRegsPhysBase));




		ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_STATUS);
		if (ui32RegVal & (EUR_CR_EVENT_STATUS_DPM_OUT_OF_MEMORY_GBL_MASK | EUR_CR_EVENT_STATUS_DPM_OUT_OF_MEMORY_MT_MASK))
		{
			PVR_LOG(("DPM out of memory!!"));
		}
		PVR_LOG(("EUR_CR_EVENT_STATUS:     %x", ui32RegVal));

		ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_STATUS2);
		PVR_LOG(("EUR_CR_EVENT_STATUS2:    %x", ui32RegVal));

		ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_CTRL);
		PVR_LOG(("EUR_CR_BIF_CTRL:         %x", ui32RegVal));

		#if defined(EUR_CR_BIF_BANK0)
		ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_BANK0);
		PVR_LOG(("EUR_CR_BIF_BANK0:        %x", ui32RegVal));
		#endif

		ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_INT_STAT);
		PVR_LOG(("EUR_CR_BIF_INT_STAT:     %x", ui32RegVal));

		ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_FAULT);
		PVR_LOG(("EUR_CR_BIF_FAULT:        %x", ui32RegVal));

		ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_BIF_MEM_REQ_STAT);
		PVR_LOG(("EUR_CR_BIF_MEM_REQ_STAT: %x", ui32RegVal));

		ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_CLKGATECTL);
		PVR_LOG(("EUR_CR_CLKGATECTL:       %x", ui32RegVal));

		#if defined(EUR_CR_PDS_PC_BASE)
		ui32RegVal = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_PDS_PC_BASE);
		PVR_LOG(("EUR_CR_PDS_PC_BASE:      %x", ui32RegVal));
		#endif


	}

	#if defined(PVRSRV_USSE_EDM_STATUS_DEBUG)
	{
		IMG_UINT32	*pui32MKTraceBuffer = psDevInfo->psKernelEDMStatusBufferMemInfo->pvLinAddrKM;
		IMG_UINT32	ui32LastStatusCode, ui32WriteOffset;

		ui32LastStatusCode = *pui32MKTraceBuffer;
		pui32MKTraceBuffer++;
		ui32WriteOffset = *pui32MKTraceBuffer;
		pui32MKTraceBuffer++;

		PVR_LOG(("Last SGX microkernel status code: 0x%x", ui32LastStatusCode));

		#if defined(PVRSRV_DUMP_MK_TRACE)


		{
			IMG_UINT32	ui32LoopCounter;

			for (ui32LoopCounter = 0;
				 ui32LoopCounter < SGXMK_TRACE_BUFFER_SIZE;
				 ui32LoopCounter++)
			{
				IMG_UINT32	*pui32BufPtr;
				pui32BufPtr = pui32MKTraceBuffer +
								(((ui32WriteOffset + ui32LoopCounter) % SGXMK_TRACE_BUFFER_SIZE) * 4);
				PVR_LOG(("(MKT%u) %08X %08X %08X %08X", ui32LoopCounter,
						 pui32BufPtr[2], pui32BufPtr[3], pui32BufPtr[1], pui32BufPtr[0]));
			}
		}
		#endif
	}
	#endif

	{


		IMG_UINT32	*pui32HostCtlBuffer = (IMG_UINT32 *)psDevInfo->psSGXHostCtl;
		IMG_UINT32	ui32LoopCounter;

		PVR_LOG(("SGX Host control:"));

		for (ui32LoopCounter = 0;
			 ui32LoopCounter < sizeof(*psDevInfo->psSGXHostCtl) / sizeof(*pui32HostCtlBuffer);
			 ui32LoopCounter += 4)
		{
			PVR_LOG(("\t0x%X: 0x%08X 0x%08X 0x%08X 0x%08X", ui32LoopCounter * sizeof(*pui32HostCtlBuffer),
					pui32HostCtlBuffer[ui32LoopCounter + 0], pui32HostCtlBuffer[ui32LoopCounter + 1],
					pui32HostCtlBuffer[ui32LoopCounter + 2], pui32HostCtlBuffer[ui32LoopCounter + 3]));
		}
	}

	{


		IMG_UINT32	*pui32TA3DCtlBuffer = psDevInfo->psKernelSGXTA3DCtlMemInfo->pvLinAddrKM;
		IMG_UINT32	ui32LoopCounter;

		PVR_LOG(("SGX TA/3D control:"));

		for (ui32LoopCounter = 0;
			 ui32LoopCounter < psDevInfo->psKernelSGXTA3DCtlMemInfo->ui32AllocSize / sizeof(*pui32TA3DCtlBuffer);
			 ui32LoopCounter += 4)
		{
			PVR_LOG(("\t0x%X: 0x%08X 0x%08X 0x%08X 0x%08X", ui32LoopCounter * sizeof(*pui32TA3DCtlBuffer),
					pui32TA3DCtlBuffer[ui32LoopCounter + 0], pui32TA3DCtlBuffer[ui32LoopCounter + 1],
					pui32TA3DCtlBuffer[ui32LoopCounter + 2], pui32TA3DCtlBuffer[ui32LoopCounter + 3]));
		}
	}

	QueueDumpDebugInfo();
}


#if defined(SYS_USING_INTERRUPTS) || defined(SUPPORT_HW_RECOVERY)
static
IMG_VOID HWRecoveryResetSGX (PVRSRV_DEVICE_NODE *psDeviceNode,
							 IMG_UINT32 		ui32Component,
							 IMG_UINT32			ui32CallerID)
{
	PVRSRV_ERROR		eError;
	PVRSRV_SGXDEV_INFO	*psDevInfo = (PVRSRV_SGXDEV_INFO*)psDeviceNode->pvDevice;
	SGXMKIF_HOST_CTL	*psSGXHostCtl = (SGXMKIF_HOST_CTL *)psDevInfo->psSGXHostCtl;

	PVR_UNREFERENCED_PARAMETER(ui32Component);



	eError = PVRSRVPowerLock(ui32CallerID, IMG_FALSE);
	if(eError != PVRSRV_OK)
	{



		PVR_DPF((PVR_DBG_WARNING,"HWRecoveryResetSGX: Power transition in progress"));
		return;
	}

	psSGXHostCtl->ui32InterruptClearFlags |= PVRSRV_USSE_EDM_INTERRUPT_HWR;

	PVR_LOG(("HWRecoveryResetSGX: SGX Hardware Recovery triggered"));

	SGXDumpDebugInfo(psDeviceNode, IMG_TRUE);


	PDUMPSUSPEND();


	eError = SGXInitialise(psDevInfo);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"HWRecoveryResetSGX: SGXInitialise failed (%d)", eError));
	}


	PDUMPRESUME();

	PVRSRVPowerUnlock(ui32CallerID);


	SGXScheduleProcessQueuesKM(psDeviceNode);



	PVRSRVProcessQueues(ui32CallerID, IMG_TRUE);
}
#endif


#if defined(SUPPORT_HW_RECOVERY)
IMG_VOID SGXOSTimer(IMG_VOID *pvData)
{
	PVRSRV_DEVICE_NODE *psDeviceNode = pvData;
	PVRSRV_SGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
	static IMG_UINT32	ui32EDMTasks = 0;
	static IMG_UINT32	ui32LockupCounter = 0;
	static IMG_UINT32	ui32NumResets = 0;
	IMG_UINT32		ui32CurrentEDMTasks;
	IMG_BOOL		bLockup = IMG_FALSE;
	IMG_BOOL		bPoweredDown;


	psDevInfo->ui32TimeStamp++;

#if defined(NO_HARDWARE)
	bPoweredDown = IMG_TRUE;
#else
	bPoweredDown = SGXIsDevicePowered(psDeviceNode) ? IMG_FALSE : IMG_TRUE;
#endif



	if (bPoweredDown)
	{
		ui32LockupCounter = 0;
	}
	else
	{

		ui32CurrentEDMTasks = OSReadHWReg(psDevInfo->pvRegsBaseKM, psDevInfo->ui32EDMTaskReg0);
		if (psDevInfo->ui32EDMTaskReg1 != 0)
		{
			ui32CurrentEDMTasks ^= OSReadHWReg(psDevInfo->pvRegsBaseKM, psDevInfo->ui32EDMTaskReg1);
		}
		if ((ui32CurrentEDMTasks == ui32EDMTasks) &&
			(psDevInfo->ui32NumResets == ui32NumResets))
		{
			ui32LockupCounter++;
			if (ui32LockupCounter == 3)
			{
				ui32LockupCounter = 0;
				PVR_DPF((PVR_DBG_ERROR, "SGXOSTimer() detected SGX lockup (0x%x tasks)", ui32EDMTasks));

				bLockup = IMG_TRUE;
			}
		}
		else
		{
			ui32LockupCounter = 0;
			ui32EDMTasks = ui32CurrentEDMTasks;
			ui32NumResets = psDevInfo->ui32NumResets;
		}
	}

	if (bLockup)
	{
		SGXMKIF_HOST_CTL	*psSGXHostCtl = (SGXMKIF_HOST_CTL *)psDevInfo->psSGXHostCtl;


		psSGXHostCtl->ui32HostDetectedLockups ++;

		PVR_LOG(("HWRecoveryResetSGX: From Kernel"));

		HWRecoveryResetSGX(psDeviceNode, 0, KERNEL_ID);
	}
}
#endif


#if defined(SYS_USING_INTERRUPTS)

IMG_BOOL SGX_ISRHandler (IMG_VOID *pvData)
{
	IMG_BOOL bInterruptProcessed = IMG_FALSE;



	{
		IMG_UINT32 ui32EventStatus, ui32EventEnable;
		IMG_UINT32 ui32EventClear = 0;
		PVRSRV_DEVICE_NODE *psDeviceNode;
		PVRSRV_SGXDEV_INFO *psDevInfo;


		if(pvData == IMG_NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGX_ISRHandler: Invalid params\n"));
			return bInterruptProcessed;
		}

		psDeviceNode = (PVRSRV_DEVICE_NODE *)pvData;
		psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;

		ui32EventStatus = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_STATUS);
		ui32EventEnable = OSReadHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_HOST_ENABLE);



		gui32EventStatusServicesByISR = ui32EventStatus;


		ui32EventStatus &= ui32EventEnable;

		if (ui32EventStatus & EUR_CR_EVENT_STATUS_SW_EVENT_MASK)
		{
			ui32EventClear |= EUR_CR_EVENT_HOST_CLEAR_SW_EVENT_MASK;
		}

		if (ui32EventClear)
		{
			bInterruptProcessed = IMG_TRUE;


			ui32EventClear |= EUR_CR_EVENT_HOST_CLEAR_MASTER_INTERRUPT_MASK;


			OSWriteHWReg(psDevInfo->pvRegsBaseKM, EUR_CR_EVENT_HOST_CLEAR, ui32EventClear);
		}
	}

	return bInterruptProcessed;
}


IMG_VOID SGX_MISRHandler (IMG_VOID *pvData)
{
	PVRSRV_DEVICE_NODE	*psDeviceNode = (PVRSRV_DEVICE_NODE *)pvData;
	PVRSRV_SGXDEV_INFO	*psDevInfo = (PVRSRV_SGXDEV_INFO*)psDeviceNode->pvDevice;
	SGXMKIF_HOST_CTL	*psSGXHostCtl = (SGXMKIF_HOST_CTL *)psDevInfo->psSGXHostCtl;

	if (((psSGXHostCtl->ui32InterruptFlags & PVRSRV_USSE_EDM_INTERRUPT_HWR) != 0UL) &&
		((psSGXHostCtl->ui32InterruptClearFlags & PVRSRV_USSE_EDM_INTERRUPT_HWR) == 0UL))
	{
		PVR_LOG(("HWRecoveryResetSGX: From ISR"));
		HWRecoveryResetSGX(psDeviceNode, 0, ISR_ID);
	}

#if defined(OS_SUPPORTS_IN_LISR)
	if (psDeviceNode->bReProcessDeviceCommandComplete)
	{
		SGXScheduleProcessQueuesKM(psDeviceNode);
	}
#endif

	SGXTestActivePowerEvent(psDeviceNode, ISR_ID);
}
#endif


PVRSRV_ERROR SGXRegisterDevice (PVRSRV_DEVICE_NODE *psDeviceNode)
{
	DEVICE_MEMORY_INFO *psDevMemoryInfo;
	DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap;


	psDeviceNode->sDevId.eDeviceType	= DEV_DEVICE_TYPE;
	psDeviceNode->sDevId.eDeviceClass	= DEV_DEVICE_CLASS;

	psDeviceNode->pfnInitDevice		= DevInitSGXPart1;
	psDeviceNode->pfnDeInitDevice		= DevDeInitSGX;

	psDeviceNode->pfnInitDeviceCompatCheck	= SGXDevInitCompatCheck;



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

#if defined (SYS_USING_INTERRUPTS)


	psDeviceNode->pfnDeviceISR = SGX_ISRHandler;
	psDeviceNode->pfnDeviceMISR = SGX_MISRHandler;
#endif



	psDeviceNode->pfnDeviceCommandComplete = SGXCommandComplete;



	psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;

	psDevMemoryInfo->ui32AddressSpaceSizeLog2 = SGX_FEATURE_ADDRESS_SPACE_SIZE;


	psDevMemoryInfo->ui32Flags = 0;


	if(OSAllocMem( PVRSRV_OS_PAGEABLE_HEAP,
					 sizeof(DEVICE_MEMORY_HEAP_INFO) * SGX_MAX_HEAP_ID,
					 (IMG_VOID **)&psDevMemoryInfo->psDeviceMemoryHeap, 0,
					 "Array of Device Memory Heap Info") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SGXRegisterDevice : Failed to alloc memory for DEVICE_MEMORY_HEAP_INFO"));
		return (PVRSRV_ERROR_OUT_OF_MEMORY);
	}
	OSMemSet(psDevMemoryInfo->psDeviceMemoryHeap, 0, sizeof(DEVICE_MEMORY_HEAP_INFO) * SGX_MAX_HEAP_ID);

	psDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;





	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_GENERAL_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_GENERAL_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_GENERAL_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
														| PVRSRV_MEM_RAM_BACKED_ALLOCATION
														| PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap->pszName = "General";
	psDeviceMemoryHeap->pszBSName = "General BS";
	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
#if !defined(SUPPORT_SGX_GENERAL_MAPPING_HEAP)

	psDevMemoryInfo->ui32MappingHeapID = (IMG_UINT32)(psDeviceMemoryHeap - psDevMemoryInfo->psDeviceMemoryHeap);
#endif
	psDeviceMemoryHeap++;



	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_TADATA_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_TADATA_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_TADATA_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
														| PVRSRV_MEM_RAM_BACKED_ALLOCATION
														| PVRSRV_HAP_MULTI_PROCESS;
	psDeviceMemoryHeap->pszName = "TA Data";
	psDeviceMemoryHeap->pszBSName = "TA Data BS";
	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
	psDeviceMemoryHeap++;



	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_KERNEL_CODE_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_KERNEL_CODE_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_KERNEL_CODE_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
															| PVRSRV_MEM_RAM_BACKED_ALLOCATION
															| PVRSRV_HAP_MULTI_PROCESS;
	psDeviceMemoryHeap->pszName = "Kernel Code";
	psDeviceMemoryHeap->pszBSName = "Kernel Code BS";
	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_SHARED_EXPORTED;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
	psDeviceMemoryHeap++;



	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_KERNEL_DATA_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_KERNEL_DATA_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_KERNEL_DATA_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
																| PVRSRV_MEM_RAM_BACKED_ALLOCATION
																| PVRSRV_HAP_MULTI_PROCESS;
	psDeviceMemoryHeap->pszName = "KernelData";
	psDeviceMemoryHeap->pszBSName = "KernelData BS";
	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_SHARED_EXPORTED;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
	psDeviceMemoryHeap++;



	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_PIXELSHADER_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_PIXELSHADER_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_PIXELSHADER_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
																| PVRSRV_MEM_RAM_BACKED_ALLOCATION
																| PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap->pszName = "PixelShaderUSSE";
	psDeviceMemoryHeap->pszBSName = "PixelShaderUSSE BS";
	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
	psDeviceMemoryHeap++;



	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_VERTEXSHADER_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_VERTEXSHADER_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_VERTEXSHADER_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
																| PVRSRV_MEM_RAM_BACKED_ALLOCATION
																| PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap->pszName = "VertexShaderUSSE";
	psDeviceMemoryHeap->pszBSName = "VertexShaderUSSE BS";
	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
	psDeviceMemoryHeap++;



	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_PDSPIXEL_CODEDATA_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_PDSPIXEL_CODEDATA_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_PDSPIXEL_CODEDATA_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
																| PVRSRV_MEM_RAM_BACKED_ALLOCATION
																| PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap->pszName = "PDSPixelCodeData";
	psDeviceMemoryHeap->pszBSName = "PDSPixelCodeData BS";
	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
	psDeviceMemoryHeap++;



	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_PDSVERTEX_CODEDATA_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_PDSVERTEX_CODEDATA_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_PDSVERTEX_CODEDATA_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
																| PVRSRV_MEM_RAM_BACKED_ALLOCATION
																| PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap->pszName = "PDSVertexCodeData";
	psDeviceMemoryHeap->pszBSName = "PDSVertexCodeData BS";
	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
	psDeviceMemoryHeap++;



	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_SYNCINFO_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_SYNCINFO_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_SYNCINFO_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
														| PVRSRV_MEM_RAM_BACKED_ALLOCATION
														| PVRSRV_HAP_MULTI_PROCESS;
	psDeviceMemoryHeap->pszName = "CacheCoherent";
	psDeviceMemoryHeap->pszBSName = "CacheCoherent BS";
	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_SHARED_EXPORTED;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;

	psDevMemoryInfo->ui32SyncHeapID = (IMG_UINT32)(psDeviceMemoryHeap - psDevMemoryInfo->psDeviceMemoryHeap);
	psDeviceMemoryHeap++;



	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_3DPARAMETERS_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_3DPARAMETERS_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_3DPARAMETERS_HEAP_SIZE;
	psDeviceMemoryHeap->pszName = "3DParameters";
	psDeviceMemoryHeap->pszBSName = "3DParameters BS";
#if defined(SUPPORT_PERCONTEXT_PB)
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
															| PVRSRV_MEM_RAM_BACKED_ALLOCATION
															| PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_PERCONTEXT;
#else
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
													| PVRSRV_MEM_RAM_BACKED_ALLOCATION
													| PVRSRV_HAP_MULTI_PROCESS;
	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_SHARED_EXPORTED;
#endif

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
	psDeviceMemoryHeap++;


#if defined(SUPPORT_SGX_GENERAL_MAPPING_HEAP)

	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_GENERAL_MAPPING_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_GENERAL_MAPPING_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_GENERAL_MAPPING_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE | PVRSRV_HAP_MULTI_PROCESS;
	psDeviceMemoryHeap->pszName = "GeneralMapping";
	psDeviceMemoryHeap->pszBSName = "GeneralMapping BS";
	#if defined(SGX_FEATURE_MULTIPLE_MEM_CONTEXTS) && defined(FIX_HW_BRN_23410)







		psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_SHARED_EXPORTED;
#else
		psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_PERCONTEXT;
#endif

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;

	psDevMemoryInfo->ui32MappingHeapID = (IMG_UINT32)(psDeviceMemoryHeap - psDevMemoryInfo->psDeviceMemoryHeap);
	psDeviceMemoryHeap++;
#endif


#if defined(SGX_FEATURE_2D_HARDWARE)

	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_2D_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_2D_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_2D_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
														| PVRSRV_MEM_RAM_BACKED_ALLOCATION
														| PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap->pszName = "2D";
	psDeviceMemoryHeap->pszBSName = "2D BS";

	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_SHARED_EXPORTED;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
	psDeviceMemoryHeap++;
#endif


#if defined(FIX_HW_BRN_26915)


	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_CGBUFFER_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_CGBUFFER_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_CGBUFFER_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
														| PVRSRV_MEM_RAM_BACKED_ALLOCATION
														| PVRSRV_HAP_SINGLE_PROCESS;
	psDeviceMemoryHeap->pszName = "CGBuffer";
	psDeviceMemoryHeap->pszBSName = "CGBuffer BS";

	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_PERCONTEXT;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
	psDeviceMemoryHeap++;
#endif

#if defined(SUPPORT_SGX_VIDEO_HEAP)

	psDeviceMemoryHeap->ui32HeapID = HEAP_ID( PVRSRV_DEVICE_TYPE_SGX, SGX_VIDEO_HEAP_ID);
	psDeviceMemoryHeap->sDevVAddrBase.uiAddr = SGX_VIDEO_HEAP_BASE;
	psDeviceMemoryHeap->ui32HeapSize = SGX_VIDEO_HEAP_SIZE;
	psDeviceMemoryHeap->ui32Attribs = PVRSRV_HAP_WRITECOMBINE
										| PVRSRV_MEM_RAM_BACKED_ALLOCATION
										| PVRSRV_HAP_MULTI_PROCESS;
	psDeviceMemoryHeap->pszName = "Video";
	psDeviceMemoryHeap->pszBSName = "Video BS";

	psDeviceMemoryHeap->DevMemHeapType = DEVICE_MEMORY_HEAP_SHARED_EXPORTED;

	psDeviceMemoryHeap->ui32DataPageSize = SGX_MMU_PAGE_SIZE;
	psDeviceMemoryHeap++;
#endif


	psDevMemoryInfo->ui32HeapCount = (IMG_UINT32)(psDeviceMemoryHeap - psDevMemoryInfo->psDeviceMemoryHeap);

	return PVRSRV_OK;
}

IMG_EXPORT
PVRSRV_ERROR SGXGetClientInfoKM(IMG_HANDLE					hDevCookie,
								SGX_CLIENT_INFO*		psClientInfo)
{
	PVRSRV_SGXDEV_INFO *psDevInfo = (PVRSRV_SGXDEV_INFO *)((PVRSRV_DEVICE_NODE *)hDevCookie)->pvDevice;



	psDevInfo->ui32ClientRefCount++;

#if defined(PDUMP)

	psDevInfo->psKernelCCBInfo->ui32CCBDumpWOff = 0;
#endif


	psClientInfo->ui32ProcessID = OSGetCurrentProcessIDKM();



	OSMemCopy(&psClientInfo->asDevData, &psDevInfo->asSGXDevData, sizeof(psClientInfo->asDevData));


	return PVRSRV_OK;
}


IMG_VOID SGXPanic(PVRSRV_DEVICE_NODE	*psDeviceNode)
{
	PVR_LOG(("SGX panic"));
	SGXDumpDebugInfo(psDeviceNode, IMG_FALSE);
	OSPanic();
}


PVRSRV_ERROR SGXDevInitCompatCheck(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVRSRV_ERROR	eError;
	PVRSRV_SGXDEV_INFO 				*psDevInfo;
	IMG_UINT32 			ui32BuildOptions, ui32BuildOptionsMismatch;
#if !defined(NO_HARDWARE)
	PPVRSRV_KERNEL_MEM_INFO			psMemInfo;
	PVRSRV_SGX_MISCINFO_INFO		*psSGXMiscInfoInt;
	PVRSRV_SGX_MISCINFO_FEATURES	*psSGXFeatures;
	SGX_MISCINFO_STRUCT_SIZES		*psSGXStructSizes;
	IMG_BOOL						bStructSizesFailed;


	IMG_BOOL	bCheckCoreRev;
	const IMG_UINT32	aui32CoreRevExceptions[] =
		{
			0x10100, 0x10101
		};
	const IMG_UINT32	ui32NumCoreExceptions = sizeof(aui32CoreRevExceptions) / (2*sizeof(IMG_UINT32));
	IMG_UINT	i;
#endif


	if(psDeviceNode->sDevId.eDeviceType != PVRSRV_DEVICE_TYPE_SGX)
	{
		PVR_LOG(("(FAIL) SGXInit: Device not of type SGX"));
		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto chk_exit;
	}

	psDevInfo = psDeviceNode->pvDevice;



	ui32BuildOptions = (SGX_BUILD_OPTIONS);
#if defined(DEBUG) || defined (INTERNAL_TEST)
	/* Workaround: During development, the DEBUG bit can get out of sync, so
	 * ignore that bit:
	 */
	if (0x1 & ui32BuildOptions) {
		psDevInfo->ui32ClientBuildOptions =
			psDevInfo->ui32ClientBuildOptions | 0x1;
	} else {
		psDevInfo->ui32ClientBuildOptions =
			psDevInfo->ui32ClientBuildOptions & 0xfffffffe;
	}
#endif
	if (ui32BuildOptions != psDevInfo->ui32ClientBuildOptions)
	{
		ui32BuildOptionsMismatch = ui32BuildOptions ^ psDevInfo->ui32ClientBuildOptions;
		if ( (psDevInfo->ui32ClientBuildOptions & ui32BuildOptionsMismatch) != 0)
		{
			PVR_LOG(("(FAIL) SGXInit: Mismatch in client-side and KM driver build options; "
				"extra options present in client-side driver: (0x%lx). Please check sgx_options.h",
				psDevInfo->ui32ClientBuildOptions & ui32BuildOptionsMismatch ));
		}

		if ( (ui32BuildOptions & ui32BuildOptionsMismatch) != 0)
		{
			PVR_LOG(("(FAIL) SGXInit: Mismatch in client-side and KM driver build options; "
				"extra options present in KM: (0x%lx). Please check sgx_options.h",
				ui32BuildOptions & ui32BuildOptionsMismatch ));
		}
		eError = PVRSRV_ERROR_BUILD_MISMATCH;
		goto chk_exit;
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "SGXInit: Client-side and KM driver build options match. [ OK ]"));
	}

#if !defined (NO_HARDWARE)
	psMemInfo = psDevInfo->psKernelSGXMiscMemInfo;


	psSGXMiscInfoInt = psMemInfo->pvLinAddrKM;
	psSGXMiscInfoInt->ui32MiscInfoFlags = 0;
	psSGXMiscInfoInt->ui32MiscInfoFlags |= PVRSRV_USSE_MISCINFO_GET_STRUCT_SIZES;
	eError = SGXGetMiscInfoUkernel(psDevInfo, psDeviceNode);


	if(eError != PVRSRV_OK)
	{
		PVR_LOG(("(FAIL) SGXInit: Unable to validate device DDK version"));
		goto chk_exit;
	}
	psSGXFeatures = &((PVRSRV_SGX_MISCINFO_INFO*)(psMemInfo->pvLinAddrKM))->sSGXFeatures;
	if( (psSGXFeatures->ui32DDKVersion !=
		((PVRVERSION_MAJ << 16) |
		 (PVRVERSION_MIN << 8) |
		  PVRVERSION_BRANCH) ) ||
		(psSGXFeatures->ui32DDKBuild != PVRVERSION_BUILD) )
	{
		PVR_LOG(("(FAIL) SGXInit: Incompatible driver DDK revision (%ld)/device DDK revision (%ld).",
				PVRVERSION_BUILD, psSGXFeatures->ui32DDKBuild));
		eError = PVRSRV_ERROR_DDK_VERSION_MISMATCH;
		PVR_DBG_BREAK;
		goto chk_exit;
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "SGXInit: driver DDK (%ld) and device DDK (%ld) match. [ OK ]",
				PVRVERSION_BUILD, psSGXFeatures->ui32DDKBuild));
	}


	if (psSGXFeatures->ui32CoreRevSW == 0)
	{


		PVR_LOG(("SGXInit: HW core rev (%lx) check skipped.",
				psSGXFeatures->ui32CoreRev));
	}
	else
	{

		bCheckCoreRev = IMG_TRUE;
		for(i=0; i<ui32NumCoreExceptions; i+=2)
		{
			if( (psSGXFeatures->ui32CoreRev==aui32CoreRevExceptions[i]) &&
				(psSGXFeatures->ui32CoreRevSW==aui32CoreRevExceptions[i+1])	)
			{
				PVR_LOG(("SGXInit: HW core rev (%lx), SW core rev (%lx) check skipped.",
						psSGXFeatures->ui32CoreRev,
						psSGXFeatures->ui32CoreRevSW));
				bCheckCoreRev = IMG_FALSE;
			}
		}

		if (bCheckCoreRev)
		{
			if (psSGXFeatures->ui32CoreRev != psSGXFeatures->ui32CoreRevSW)
			{
				PVR_LOG(("(FAIL) SGXInit: Incompatible HW core rev (%lx) and SW core rev (%lx).",
						psSGXFeatures->ui32CoreRev, psSGXFeatures->ui32CoreRevSW));
						eError = PVRSRV_ERROR_BUILD_MISMATCH;
						goto chk_exit;
			}
			else
			{
				PVR_DPF((PVR_DBG_MESSAGE, "SGXInit: HW core rev (%lx) and SW core rev (%lx) match. [ OK ]",
						psSGXFeatures->ui32CoreRev, psSGXFeatures->ui32CoreRevSW));
			}
		}
	}


	psSGXStructSizes = &((PVRSRV_SGX_MISCINFO_INFO*)(psMemInfo->pvLinAddrKM))->sSGXStructSizes;

	bStructSizesFailed = IMG_FALSE;

	CHECK_SIZE(HOST_CTL);
	CHECK_SIZE(COMMAND);
#if defined(SGX_FEATURE_2D_HARDWARE)
	CHECK_SIZE(2DCMD);
	CHECK_SIZE(2DCMD_SHARED);
#endif
	CHECK_SIZE(CMDTA);
	CHECK_SIZE(CMDTA_SHARED);
	CHECK_SIZE(TRANSFERCMD);
	CHECK_SIZE(TRANSFERCMD_SHARED);

	CHECK_SIZE(3DREGISTERS);
	CHECK_SIZE(HWPBDESC);
	CHECK_SIZE(HWRENDERCONTEXT);
	CHECK_SIZE(HWRENDERDETAILS);
	CHECK_SIZE(HWRTDATA);
	CHECK_SIZE(HWRTDATASET);
	CHECK_SIZE(HWTRANSFERCONTEXT);

	if (bStructSizesFailed == IMG_TRUE)
	{
		PVR_LOG(("(FAIL) SGXInit: Mismatch in SGXMKIF structure sizes."));
		eError = PVRSRV_ERROR_BUILD_MISMATCH;
		goto chk_exit;
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "SGXInit: SGXMKIF structure sizes match. [ OK ]"));
	}


#if defined(DEBUG) || defined (INTERNAL_TEST)
	/* Workaround: During development, the DEBUG bit can get out of sync, so
	 * ignore that bit:
	 */
	if (0x1 & (SGX_BUILD_OPTIONS)) {
		ui32BuildOptions = psSGXFeatures->ui32BuildOptions | 0x1;
	} else {
		ui32BuildOptions = psSGXFeatures->ui32BuildOptions & 0xfffffffe;
	}
#else
	ui32BuildOptions = psSGXFeatures->ui32BuildOptions;
#endif
	if (ui32BuildOptions != (SGX_BUILD_OPTIONS))
	{
		ui32BuildOptionsMismatch = ui32BuildOptions ^ (SGX_BUILD_OPTIONS);
		if ( ((SGX_BUILD_OPTIONS) & ui32BuildOptionsMismatch) != 0)
		{
			PVR_LOG(("(FAIL) SGXInit: Mismatch in driver and microkernel build options; "
				"extra options present in driver: (0x%lx). Please check sgx_options.h",
				(SGX_BUILD_OPTIONS) & ui32BuildOptionsMismatch ));
		}

		if ( (ui32BuildOptions & ui32BuildOptionsMismatch) != 0)
		{
			PVR_LOG(("(FAIL) SGXInit: Mismatch in driver and microkernel build options; "
				"extra options present in microkernel: (0x%lx). Please check sgx_options.h",
				ui32BuildOptions & ui32BuildOptionsMismatch ));
		}
		eError = PVRSRV_ERROR_BUILD_MISMATCH;
		goto chk_exit;
	}
	else
	{
		PVR_DPF((PVR_DBG_MESSAGE, "SGXInit: Driver and microkernel build options match. [ OK ]"));
	}
#endif

	eError = PVRSRV_OK;
chk_exit:
#if defined(IGNORE_SGX_INIT_COMPATIBILITY_CHECK)
	return PVRSRV_OK;
#else
	return eError;
#endif
}

static
PVRSRV_ERROR SGXGetMiscInfoUkernel(PVRSRV_SGXDEV_INFO	*psDevInfo,
								   PVRSRV_DEVICE_NODE 	*psDeviceNode)
{
	PVRSRV_ERROR		eError;
	SGXMKIF_COMMAND		sCommandData;
	PVRSRV_SGX_MISCINFO_INFO			*psSGXMiscInfoInt;
	PVRSRV_SGX_MISCINFO_FEATURES		*psSGXFeatures;
	SGX_MISCINFO_STRUCT_SIZES			*psSGXStructSizes;

	PPVRSRV_KERNEL_MEM_INFO	psMemInfo = psDevInfo->psKernelSGXMiscMemInfo;

	if (! psMemInfo->pvLinAddrKM)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetMiscInfoUkernel: Invalid address."));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	psSGXMiscInfoInt = psMemInfo->pvLinAddrKM;
	psSGXFeatures = &psSGXMiscInfoInt->sSGXFeatures;
	psSGXStructSizes = &psSGXMiscInfoInt->sSGXStructSizes;

	psSGXMiscInfoInt->ui32MiscInfoFlags &= ~PVRSRV_USSE_MISCINFO_READY;


	OSMemSet(psSGXFeatures, 0, sizeof(*psSGXFeatures));
	OSMemSet(psSGXStructSizes, 0, sizeof(*psSGXStructSizes));


	sCommandData.ui32Data[1] = psMemInfo->sDevVAddr.uiAddr;

	eError = SGXScheduleCCBCommandKM(psDeviceNode,
									 SGXMKIF_CMD_GETMISCINFO,
									 &sCommandData,
									 KERNEL_ID,
									 0);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXGetMiscInfoUkernel: SGXScheduleCCBCommandKM failed."));
		return eError;
	}


#if !defined(NO_HARDWARE)
	{
		IMG_BOOL bExit;

		bExit = IMG_FALSE;
		LOOP_UNTIL_TIMEOUT(MAX_HW_TIME_US)
		{
			if ((psSGXMiscInfoInt->ui32MiscInfoFlags & PVRSRV_USSE_MISCINFO_READY) != 0)
			{
				bExit = IMG_TRUE;
				break;
			}
		} END_LOOP_UNTIL_TIMEOUT();


		if (!bExit)
		{
			PVR_DPF((PVR_DBG_ERROR, "SGXGetMiscInfoUkernel: Timeout occurred waiting for misc info."));
			return PVRSRV_ERROR_TIMEOUT;
		}
	}
#endif

	return PVRSRV_OK;
}



IMG_EXPORT
PVRSRV_ERROR SGXGetMiscInfoKM(PVRSRV_SGXDEV_INFO	*psDevInfo,
							  SGX_MISC_INFO			*psMiscInfo,
							  PVRSRV_DEVICE_NODE 	*psDeviceNode,
							  IMG_HANDLE 			 hDevMemContext)
{
	PPVRSRV_KERNEL_MEM_INFO	psMemInfo = psDevInfo->psKernelSGXMiscMemInfo;
	IMG_UINT32	*pui32MiscInfoFlags = &((PVRSRV_SGX_MISCINFO_INFO*)(psMemInfo->pvLinAddrKM))->ui32MiscInfoFlags;


	*pui32MiscInfoFlags = 0;

#if !defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
	PVR_UNREFERENCED_PARAMETER(hDevMemContext);
#endif

	switch(psMiscInfo->eRequest)
	{
#if defined(SGX_FEATURE_DATA_BREAKPOINTS)
		case SGX_MISC_INFO_REQUEST_SET_BREAKPOINT:
		{
			IMG_UINT32 ui32RegOffset;
			IMG_UINT32 ui32RegVal;
			IMG_UINT32 ui32BaseRegOffset;
			IMG_UINT32 ui32BaseRegVal;
			IMG_UINT32 ui32MaskRegOffset;
			IMG_UINT32 ui32MaskRegVal;

			switch(psMiscInfo->uData.sSGXBreakpointInfo.ui32BPIndex)
			{
				case 0:
					ui32RegOffset = EUR_CR_BREAKPOINT0;
					ui32BaseRegOffset = EUR_CR_BREAKPOINT0_BASE;
					ui32MaskRegOffset = EUR_CR_BREAKPOINT0_MASK;
					break;
				case 1:
					ui32RegOffset = EUR_CR_BREAKPOINT1;
					ui32BaseRegOffset = EUR_CR_BREAKPOINT1_BASE;
					ui32MaskRegOffset = EUR_CR_BREAKPOINT1_MASK;
					break;
				case 2:
					ui32RegOffset = EUR_CR_BREAKPOINT2;
					ui32BaseRegOffset = EUR_CR_BREAKPOINT2_BASE;
					ui32MaskRegOffset = EUR_CR_BREAKPOINT2_MASK;
					break;
				case 3:
					ui32RegOffset = EUR_CR_BREAKPOINT3;
					ui32BaseRegOffset = EUR_CR_BREAKPOINT3_BASE;
					ui32MaskRegOffset = EUR_CR_BREAKPOINT3_MASK;
					break;
				default:
					PVR_DPF((PVR_DBG_ERROR,"SGXGetMiscInfoKM: SGX_MISC_INFO_REQUEST_SET_BREAKPOINT invalid BP idx %d", psMiscInfo->uData.sSGXBreakpointInfo.ui32BPIndex));
					return PVRSRV_ERROR_INVALID_PARAMS;
			}


			if(psMiscInfo->uData.sSGXBreakpointInfo.bBPEnable)
			{

				IMG_DEV_VIRTADDR sBPDevVAddr = psMiscInfo->uData.sSGXBreakpointInfo.sBPDevVAddr;


				ui32MaskRegVal = EUR_CR_BREAKPOINT0_MASK_REGION_MASK | EUR_CR_BREAKPOINT0_MASK_DM_MASK;


				ui32BaseRegVal = sBPDevVAddr.uiAddr & EUR_CR_BREAKPOINT0_BASE_ADDRESS_MASK;


				ui32RegVal =	EUR_CR_BREAKPOINT0_CTRL_WENABLE_MASK
							|	EUR_CR_BREAKPOINT0_CTRL_WENABLE_MASK
							|	EUR_CR_BREAKPOINT0_CTRL_TRAPENABLE_MASK;
			}
			else
			{

				ui32RegVal = ui32BaseRegVal = ui32MaskRegVal = 0;
			}










			return PVRSRV_OK;
		}
#endif

		case SGX_MISC_INFO_REQUEST_CLOCKSPEED:
		{
			psMiscInfo->uData.ui32SGXClockSpeed = psDevInfo->ui32CoreClockSpeed;
			return PVRSRV_OK;
		}

		case SGX_MISC_INFO_REQUEST_SGXREV:
		{
			PVRSRV_ERROR eError;
			PVRSRV_SGX_MISCINFO_FEATURES		*psSGXFeatures;

			eError = SGXGetMiscInfoUkernel(psDevInfo, psDeviceNode);
			if(eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "An error occurred in SGXGetMiscInfoUkernel: %d\n",
						eError));
				return eError;
			}
			psSGXFeatures = &((PVRSRV_SGX_MISCINFO_INFO*)(psMemInfo->pvLinAddrKM))->sSGXFeatures;


			psMiscInfo->uData.sSGXFeatures = *psSGXFeatures;


			PVR_DPF((PVR_DBG_MESSAGE, "SGXGetMiscInfoKM: Core 0x%lx, sw ID 0x%lx, sw Rev 0x%lx\n",
					psSGXFeatures->ui32CoreRev,
					psSGXFeatures->ui32CoreIdSW,
					psSGXFeatures->ui32CoreRevSW));
			PVR_DPF((PVR_DBG_MESSAGE, "SGXGetMiscInfoKM: DDK version 0x%lx, DDK build 0x%lx\n",
					psSGXFeatures->ui32DDKVersion,
					psSGXFeatures->ui32DDKBuild));


			return PVRSRV_OK;
		}

		case SGX_MISC_INFO_REQUEST_DRIVER_SGXREV:
		{
			PVRSRV_SGX_MISCINFO_FEATURES		*psSGXFeatures;

			psSGXFeatures = &((PVRSRV_SGX_MISCINFO_INFO*)(psMemInfo->pvLinAddrKM))->sSGXFeatures;


			OSMemSet(psMemInfo->pvLinAddrKM, 0,
					sizeof(PVRSRV_SGX_MISCINFO_INFO));

			psSGXFeatures->ui32DDKVersion =
				(PVRVERSION_MAJ << 16) |
				(PVRVERSION_MIN << 8) |
				PVRVERSION_BRANCH;
			psSGXFeatures->ui32DDKBuild = PVRVERSION_BUILD;


			psSGXFeatures->ui32BuildOptions = (SGX_BUILD_OPTIONS);


			psMiscInfo->uData.sSGXFeatures = *psSGXFeatures;
			return PVRSRV_OK;
		}

#if defined(SUPPORT_SGX_EDM_MEMORY_DEBUG)
		case SGX_MISC_INFO_REQUEST_MEMREAD:
		{
			PVRSRV_ERROR eError;
			PPVRSRV_KERNEL_MEM_INFO	psMemInfo = psDevInfo->psKernelSGXMiscMemInfo;
			PVRSRV_SGX_MISCINFO_FEATURES		*psSGXFeatures;
			PVRSRV_SGX_MISCINFO_MEMREAD			*psSGXMemReadData;

			psSGXMemReadData = &((PVRSRV_SGX_MISCINFO_INFO*)(psMemInfo->pvLinAddrKM))->sSGXMemReadData;


			*pui32MiscInfoFlags |= PVRSRV_USSE_MISCINFO_MEMREAD;


			if(psMiscInfo->hDevMemContext != IMG_NULL)
			{
				SGXGetMMUPDAddrKM( (IMG_HANDLE)psDeviceNode, hDevMemContext, &psSGXMemReadData->sPDDevPAddr);
			}
			else
			{
				return PVRSRV_ERROR_INVALID_PARAMS;
			}


			if(psMiscInfo->sDevVAddr.uiAddr != 0)
			{
				psSGXMemReadData->sDevVAddr = psMiscInfo->sDevVAddr;
			}
			else
			{
				return PVRSRV_ERROR_INVALID_PARAMS;
			}


			eError = SGXGetMiscInfoUkernel(psDevInfo, psDeviceNode);
			if(eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "An error occurred in SGXGetMiscInfoUkernel: %d\n",
						eError));
				return eError;
			}
			psSGXFeatures = &((PVRSRV_SGX_MISCINFO_INFO*)(psMemInfo->pvLinAddrKM))->sSGXFeatures;

#if !defined SGX_FEATURE_MULTIPLE_MEM_CONTEXTS
			if(*pui32MiscInfoFlags & PVRSRV_USSE_MISCINFO_MEMREAD_FAIL)
			{
				return PVRSRV_ERROR_GENERIC;
			}
#endif

			psMiscInfo->uData.sSGXFeatures = *psSGXFeatures;
			return PVRSRV_OK;
		}
#endif

#ifdef SUPPORT_SGX_HWPERF
		case SGX_MISC_INFO_REQUEST_SET_HWPERF_STATUS:
		{
			SGXMKIF_HWPERF_CB *psHWPerfCB = psDevInfo->psKernelHWPerfCBMemInfo->pvLinAddrKM;
			IMG_UINT ui32MatchingFlags;


			if ((psMiscInfo->uData.ui32NewHWPerfStatus & ~(PVRSRV_SGX_HWPERF_GRAPHICS_ON | PVRSRV_SGX_HWPERF_MK_EXECUTION_ON)) != 0)
			{
				return PVRSRV_ERROR_INVALID_PARAMS;
			}


			ui32MatchingFlags = psMiscInfo->uData.ui32NewHWPerfStatus & psDevInfo->psSGXHostCtl->ui32HWPerfFlags;
			if((ui32MatchingFlags & PVRSRV_SGX_HWPERF_GRAPHICS_ON) == 0UL)
			{
				psHWPerfCB->ui32OrdinalGRAPHICS = 0xffffffff;
			}
			if((ui32MatchingFlags & PVRSRV_SGX_HWPERF_MK_EXECUTION_ON) == 0UL)
			{
				psHWPerfCB->ui32OrdinalMK_EXECUTION = 0xffffffffUL;
			}


			psDevInfo->psSGXHostCtl->ui32HWPerfFlags = psMiscInfo->uData.ui32NewHWPerfStatus;
			#if defined(PDUMP)
			PDUMPCOMMENTWITHFLAGS(PDUMP_FLAGS_CONTINUOUS, "SGX ukernel HWPerf status %lu\n",
								  psDevInfo->psSGXHostCtl->ui32HWPerfFlags);
			PDUMPMEM(IMG_NULL, psDevInfo->psKernelSGXHostCtlMemInfo,
					 offsetof(SGXMKIF_HOST_CTL, ui32HWPerfFlags),
					 sizeof(psDevInfo->psSGXHostCtl->ui32HWPerfFlags), PDUMP_FLAGS_CONTINUOUS,
					 MAKEUNIQUETAG(psDevInfo->psKernelSGXHostCtlMemInfo));
			#endif

			return PVRSRV_OK;
		}
		case SGX_MISC_INFO_REQUEST_HWPERF_CB_ON:
		{

			SGXMKIF_HWPERF_CB *psHWPerfCB = psDevInfo->psKernelHWPerfCBMemInfo->pvLinAddrKM;
			psHWPerfCB->ui32OrdinalGRAPHICS = 0xffffffffUL;

			psDevInfo->psSGXHostCtl->ui32HWPerfFlags |= PVRSRV_SGX_HWPERF_GRAPHICS_ON;
			return PVRSRV_OK;
		}
		case SGX_MISC_INFO_REQUEST_HWPERF_CB_OFF:
		{

			psDevInfo->psSGXHostCtl->ui32HWPerfFlags = 0;
			return PVRSRV_OK;
		}
		case SGX_MISC_INFO_REQUEST_HWPERF_RETRIEVE_CB:
		{

			SGX_MISC_INFO_HWPERF_RETRIEVE_CB *psRetrieve = &psMiscInfo->uData.sRetrieveCB;
			SGXMKIF_HWPERF_CB *psHWPerfCB = psDevInfo->psKernelHWPerfCBMemInfo->pvLinAddrKM;
			IMG_UINT i;

			for (i = 0; psHWPerfCB->ui32Woff != psHWPerfCB->ui32Roff && i < psRetrieve->ui32ArraySize; i++)
			{
				SGXMKIF_HWPERF_CB_ENTRY *psData = &psHWPerfCB->psHWPerfCBData[psHWPerfCB->ui32Roff];



				psRetrieve->psHWPerfData[i].ui32FrameNo = psData->ui32FrameNo;
				psRetrieve->psHWPerfData[i].ui32Type = (psData->ui32Type & PVRSRV_SGX_HWPERF_TYPE_OP_MASK);
				psRetrieve->psHWPerfData[i].ui32StartTime = psData->ui32Time;
				psRetrieve->psHWPerfData[i].ui32StartTimeWraps = psData->ui32TimeWraps;
				psRetrieve->psHWPerfData[i].ui32EndTime = psData->ui32Time;
				psRetrieve->psHWPerfData[i].ui32EndTimeWraps = psData->ui32TimeWraps;
				psRetrieve->psHWPerfData[i].ui32ClockSpeed = psDevInfo->ui32CoreClockSpeed;
				psRetrieve->psHWPerfData[i].ui32TimeMax = psDevInfo->ui32uKernelTimerClock;
				psHWPerfCB->ui32Roff = (psHWPerfCB->ui32Roff + 1) & (SGXMKIF_HWPERF_CB_SIZE - 1);
			}
			psRetrieve->ui32DataCount = i;
			psRetrieve->ui32Time = OSClockus();
			return PVRSRV_OK;
		}
#endif
		case SGX_MISC_INFO_DUMP_DEBUG_INFO:
		{
			PVR_LOG(("User requested SGX debug info"));


			SGXDumpDebugInfo(psDeviceNode, IMG_FALSE);

			return PVRSRV_OK;
		}

		case SGX_MISC_INFO_PANIC:
		{
			PVR_LOG(("User requested SGX panic"));

			SGXPanic(psDeviceNode);

			return PVRSRV_OK;
		}

		default:
		{

			return PVRSRV_ERROR_INVALID_PARAMS;
		}
	}
}

#if defined(SUPPORT_SGX_HWPERF)
IMG_EXPORT
PVRSRV_ERROR SGXReadDiffCountersKM(IMG_HANDLE				hDevHandle,
								   IMG_UINT32				ui32Reg,
								   IMG_UINT32				*pui32Old,
								   IMG_BOOL					bNew,
								   IMG_UINT32				ui32New,
								   IMG_UINT32				ui32NewReset,
								   IMG_UINT32				ui32CountersReg,
								   IMG_UINT32				ui32Reg2,
								   IMG_BOOL					*pbActive,
								   PVRSRV_SGXDEV_DIFF_INFO	*psDiffs)
{
	PVRSRV_ERROR    	eError;
	SYS_DATA			*psSysData;
	PVRSRV_POWER_DEV	*psPowerDevice;
	IMG_BOOL			bPowered = IMG_FALSE;
	PVRSRV_DEVICE_NODE	*psDeviceNode = hDevHandle;
	PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;


	if(bNew)
	{
		psDevInfo->ui32HWGroupRequested = ui32New;
	}
	psDevInfo->ui32HWReset |= ui32NewReset;


	eError = PVRSRVPowerLock(KERNEL_ID, IMG_FALSE);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}

	SysAcquireData(&psSysData);


	psPowerDevice = (PVRSRV_POWER_DEV*)
				List_PVRSRV_POWER_DEV_Any_va(psSysData->psPowerDeviceList,
											MatchPowerDeviceIndex_AnyVaCb,
											psDeviceNode->sDevId.ui32DeviceIndex);

	if (psPowerDevice)
	{
		bPowered = (IMG_BOOL)(psPowerDevice->eCurrentPowerState == PVRSRV_DEV_POWER_STATE_ON);
	}



	*pbActive = bPowered;



	{
		IMG_UINT32 ui32rval = 0;


		if(bPowered)
		{
			IMG_UINT32 i;


			*pui32Old = OSReadHWReg(psDevInfo->pvRegsBaseKM, ui32Reg);

			for (i = 0; i < PVRSRV_SGX_DIFF_NUM_COUNTERS; ++i)
			{
				psDiffs->aui32Counters[i] = OSReadHWReg(psDevInfo->pvRegsBaseKM, ui32CountersReg + (i * 4));
			}

			if(ui32Reg2)
			{
				ui32rval = OSReadHWReg(psDevInfo->pvRegsBaseKM, ui32Reg2);
			}



			if (psDevInfo->ui32HWGroupRequested != *pui32Old)
			{

				if(psDevInfo->ui32HWReset != 0)
				{
					OSWriteHWReg(psDevInfo->pvRegsBaseKM, ui32Reg, psDevInfo->ui32HWGroupRequested | psDevInfo->ui32HWReset);
					psDevInfo->ui32HWReset = 0;
				}

				OSWriteHWReg(psDevInfo->pvRegsBaseKM, ui32Reg, psDevInfo->ui32HWGroupRequested);
			}
		}

		psDiffs->ui32Time[0] = OSClockus();
		psDiffs->ui32Time[1] = psDevInfo->psSGXHostCtl->ui32TimeWraps;
		psDiffs->ui32Time[2] = ui32rval;

		psDiffs->ui32Marker[0] = psDevInfo->ui32KickTACounter;
		psDiffs->ui32Marker[1] = psDevInfo->ui32KickTARenderCounter;
	}


	PVRSRVPowerUnlock(KERNEL_ID);

	SGXTestActivePowerEvent(psDeviceNode, KERNEL_ID);

	return eError;
}


IMG_EXPORT
PVRSRV_ERROR SGXReadHWPerfCBKM(IMG_HANDLE					hDevHandle,
							   IMG_UINT32					ui32ArraySize,
							   PVRSRV_SGX_HWPERF_CB_ENTRY	*psClientHWPerfEntry,
							   IMG_UINT32					*pui32DataCount,
							   IMG_UINT32					*pui32ClockSpeed,
							   IMG_UINT32					*pui32HostTimeStamp)
{
	PVRSRV_ERROR    	eError = PVRSRV_OK;
	PVRSRV_DEVICE_NODE	*psDeviceNode = hDevHandle;
	PVRSRV_SGXDEV_INFO	*psDevInfo = psDeviceNode->pvDevice;
	SGXMKIF_HWPERF_CB	*psHWPerfCB = psDevInfo->psKernelHWPerfCBMemInfo->pvLinAddrKM;
	IMG_UINT			i;

	for (i = 0;
		 psHWPerfCB->ui32Woff != psHWPerfCB->ui32Roff && i < ui32ArraySize;
		 i++)
	{
		SGXMKIF_HWPERF_CB_ENTRY *psMKPerfEntry = &psHWPerfCB->psHWPerfCBData[psHWPerfCB->ui32Roff];

		psClientHWPerfEntry[i].ui32FrameNo = psMKPerfEntry->ui32FrameNo;
		psClientHWPerfEntry[i].ui32Type = psMKPerfEntry->ui32Type;
		psClientHWPerfEntry[i].ui32Ordinal	= psMKPerfEntry->ui32Ordinal;
		psClientHWPerfEntry[i].ui32Clocksx16 = SGXConvertTimeStamp(psDevInfo,
													psMKPerfEntry->ui32TimeWraps,
													psMKPerfEntry->ui32Time);
		OSMemCopy(&psClientHWPerfEntry[i].ui32Counters[0],
				  &psMKPerfEntry->ui32Counters[0],
				  sizeof(psMKPerfEntry->ui32Counters));

		psHWPerfCB->ui32Roff = (psHWPerfCB->ui32Roff + 1) & (SGXMKIF_HWPERF_CB_SIZE - 1);
	}

	*pui32DataCount = i;
	*pui32ClockSpeed = psDevInfo->ui32CoreClockSpeed;
	*pui32HostTimeStamp = OSClockus();

	return eError;
}
#else
#endif


