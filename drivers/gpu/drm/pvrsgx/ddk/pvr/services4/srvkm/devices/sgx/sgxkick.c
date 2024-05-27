/*************************************************************************/ /*!
@Title          Device specific kickTA routines
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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

#include <stddef.h> /* For the macro offsetof() */
#include "services_headers.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#if defined (PDUMP)
#include "sgxapi_km.h"
#include "pdump_km.h"
#endif
#include "sgx_bridge_km.h"
#include "osfunc.h"
#include "pvr_debug.h"
#include "sgxutils.h"

/*!
******************************************************************************

 @Function	SGXDoKickKM

 @Description

 Really kicks the TA

 @Input hDevHandle - Device handle

 @Return ui32Error - success or failure

******************************************************************************/
IMG_EXPORT
PVRSRV_ERROR SGXDoKickKM(IMG_HANDLE hDevHandle, SGX_CCB_KICK *psCCBKick)
{
	PVRSRV_ERROR eError;
	PVRSRV_KERNEL_SYNC_INFO	*psSyncInfo;
	PVRSRV_KERNEL_MEM_INFO	*psCCBMemInfo = (PVRSRV_KERNEL_MEM_INFO *) psCCBKick->hCCBKernelMemInfo;
	SGXMKIF_CMDTA_SHARED *psTACmd;
	IMG_UINT32 i;
#if defined(SUPPORT_SGX_HWPERF)
	PVRSRV_DEVICE_NODE      *psDeviceNode;
	PVRSRV_SGXDEV_INFO      *psDevInfo;

	psDeviceNode = (PVRSRV_DEVICE_NODE *)hDevHandle;
	psDevInfo = (PVRSRV_SGXDEV_INFO *)psDeviceNode->pvDevice;
#endif

#if defined(SUPPORT_SGX_HWPERF)
	if (psCCBKick->bKickRender)
	{
		++psDevInfo->ui32KickTARenderCounter;
	}
	++psDevInfo->ui32KickTACounter;
#endif

	if (!CCB_OFFSET_IS_VALID(SGXMKIF_CMDTA_SHARED, psCCBMemInfo, psCCBKick, ui32CCBOffset))
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXDoKickKM: Invalid CCB offset"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}


	psTACmd = CCB_DATA_FROM_OFFSET(SGXMKIF_CMDTA_SHARED, psCCBMemInfo, psCCBKick, ui32CCBOffset);


	if (psCCBKick->hTA3DSyncInfo)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->hTA3DSyncInfo;
		psTACmd->sTA3DDependency.sWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;

		psTACmd->sTA3DDependency.ui32WriteOpsPendingVal   = psSyncInfo->psSyncData->ui32WriteOpsPending;

		if (psCCBKick->bTADependency)
		{
			psSyncInfo->psSyncData->ui32WriteOpsPending++;
		}
	}

	if (psCCBKick->hTASyncInfo != IMG_NULL)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->hTASyncInfo;

		psTACmd->sTATQSyncReadOpsCompleteDevVAddr  = psSyncInfo->sReadOpsCompleteDevVAddr;
		psTACmd->sTATQSyncWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;

		psTACmd->ui32TATQSyncReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending++;
		psTACmd->ui32TATQSyncWriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending;
	}

	if (psCCBKick->h3DSyncInfo != IMG_NULL)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->h3DSyncInfo;

		psTACmd->s3DTQSyncReadOpsCompleteDevVAddr  = psSyncInfo->sReadOpsCompleteDevVAddr;
		psTACmd->s3DTQSyncWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;

		psTACmd->ui323DTQSyncReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending++;
		psTACmd->ui323DTQSyncWriteOpsPendingVal  = psSyncInfo->psSyncData->ui32WriteOpsPending;
	}

	psTACmd->ui32NumTAStatusVals = psCCBKick->ui32NumTAStatusVals;
	if (psCCBKick->ui32NumTAStatusVals != 0)
	{
		/* Copy status vals over */
		for (i = 0; i < psCCBKick->ui32NumTAStatusVals; i++)
		{
#if defined(SUPPORT_SGX_NEW_STATUS_VALS)
			psTACmd->sCtlTAStatusInfo[i] = psCCBKick->asTAStatusUpdate[i].sCtlStatus;
#else
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->ahTAStatusSyncInfo[i];
			psTACmd->sCtlTAStatusInfo[i].sStatusDevAddr = psSyncInfo->sReadOpsCompleteDevVAddr;
			psTACmd->sCtlTAStatusInfo[i].ui32StatusValue = psSyncInfo->psSyncData->ui32ReadOpsPending;
#endif
		}
	}

	psTACmd->ui32Num3DStatusVals = psCCBKick->ui32Num3DStatusVals;
	if (psCCBKick->ui32Num3DStatusVals != 0)
	{
		/* Copy status vals over */
		for (i = 0; i < psCCBKick->ui32Num3DStatusVals; i++)
		{
#if defined(SUPPORT_SGX_NEW_STATUS_VALS)
			psTACmd->sCtl3DStatusInfo[i] = psCCBKick->as3DStatusUpdate[i].sCtlStatus;
#else
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->ah3DStatusSyncInfo[i];
			psTACmd->sCtl3DStatusInfo[i].sStatusDevAddr = psSyncInfo->sReadOpsCompleteDevVAddr;
			psTACmd->sCtl3DStatusInfo[i].ui32StatusValue = psSyncInfo->psSyncData->ui32ReadOpsPending;
#endif
		}
	}


#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
	/* SRC and DST sync dependencies */
	psTACmd->ui32NumTASrcSyncs = psCCBKick->ui32NumTASrcSyncs;
	for (i=0; i<psCCBKick->ui32NumTASrcSyncs; i++)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahTASrcKernelSyncInfo[i];

		psTACmd->asTASrcSyncs[i].sWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
		psTACmd->asTASrcSyncs[i].sReadOpsCompleteDevVAddr = psSyncInfo->sReadOpsCompleteDevVAddr;

		/* Get ui32ReadOpsPending snapshot and copy into the CCB - before incrementing. */
		psTACmd->asTASrcSyncs[i].ui32ReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending++;
		/* Copy ui32WriteOpsPending snapshot into the CCB. */
		psTACmd->asTASrcSyncs[i].ui32WriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending;
	}

	psTACmd->ui32NumTADstSyncs = psCCBKick->ui32NumTADstSyncs;
	for (i=0; i<psCCBKick->ui32NumTADstSyncs; i++)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahTADstKernelSyncInfo[i];

		psTACmd->asTADstSyncs[i].sWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
		psTACmd->asTADstSyncs[i].sReadOpsCompleteDevVAddr = psSyncInfo->sReadOpsCompleteDevVAddr;

		/* Get ui32ReadOpsPending snapshot and copy into the CCB */
		psTACmd->asTADstSyncs[i].ui32ReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending;
		/* Copy ui32WriteOpsPending snapshot into the CCB - before incrementing */
		psTACmd->asTADstSyncs[i].ui32WriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending++;
	}

	psTACmd->ui32Num3DSrcSyncs = psCCBKick->ui32Num3DSrcSyncs;
	for (i=0; i<psCCBKick->ui32Num3DSrcSyncs; i++)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ah3DSrcKernelSyncInfo[i];

		psTACmd->as3DSrcSyncs[i].sWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
		psTACmd->as3DSrcSyncs[i].sReadOpsCompleteDevVAddr = psSyncInfo->sReadOpsCompleteDevVAddr;

		/* Get ui32ReadOpsPending snapshot and copy into the CCB - before incrementing. */
		psTACmd->as3DSrcSyncs[i].ui32ReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending++;
		/* Copy ui32WriteOpsPending snapshot into the CCB. */
		psTACmd->as3DSrcSyncs[i].ui32WriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending;
	}
#else /* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */
	/* texture dependencies */
	psTACmd->ui32NumSrcSyncs = psCCBKick->ui32NumSrcSyncs;
	for (i=0; i<psCCBKick->ui32NumSrcSyncs; i++)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahSrcKernelSyncInfo[i];

		psTACmd->asSrcSyncs[i].sWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
		psTACmd->asSrcSyncs[i].sReadOpsCompleteDevVAddr = psSyncInfo->sReadOpsCompleteDevVAddr;

		/* Get ui32ReadOpsPending snapshot and copy into the CCB - before incrementing. */
		psTACmd->asSrcSyncs[i].ui32ReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending++;
		/* Copy ui32WriteOpsPending snapshot into the CCB. */
		psTACmd->asSrcSyncs[i].ui32WriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending;
	}
#endif/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */

	if (psCCBKick->bFirstKickOrResume && psCCBKick->ui32NumDstSyncObjects > 0)
	{
		PVRSRV_KERNEL_MEM_INFO	*psHWDstSyncListMemInfo =
								(PVRSRV_KERNEL_MEM_INFO *)psCCBKick->hKernelHWSyncListMemInfo;
		SGXMKIF_HWDEVICE_SYNC_LIST *psHWDeviceSyncList = psHWDstSyncListMemInfo->pvLinAddrKM;
		IMG_UINT32	ui32NumDstSyncs = psCCBKick->ui32NumDstSyncObjects;

		PVR_ASSERT(((PVRSRV_KERNEL_MEM_INFO *)psCCBKick->hKernelHWSyncListMemInfo)->ui32AllocSize >= (sizeof(SGXMKIF_HWDEVICE_SYNC_LIST) +
								(sizeof(PVRSRV_DEVICE_SYNC_OBJECT) * ui32NumDstSyncs)));

		psHWDeviceSyncList->ui32NumSyncObjects = ui32NumDstSyncs;
#if defined(PDUMP)
		if (PDumpIsCaptureFrameKM())
		{
			PDUMPCOMMENT("HWDeviceSyncList for TACmd\r\n");
			PDUMPMEM(IMG_NULL,
					 psHWDstSyncListMemInfo,
					 0,
					 sizeof(SGXMKIF_HWDEVICE_SYNC_LIST),
					 0,
					 MAKEUNIQUETAG(psHWDstSyncListMemInfo));
		}
#endif

		for (i=0; i<ui32NumDstSyncs; i++)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->pahDstSyncHandles[i];

			if (psSyncInfo)
			{
				psHWDeviceSyncList->asSyncData[i].sWriteOpsCompleteDevVAddr = psSyncInfo->sWriteOpsCompleteDevVAddr;
				psHWDeviceSyncList->asSyncData[i].sReadOpsCompleteDevVAddr = psSyncInfo->sReadOpsCompleteDevVAddr;

				psHWDeviceSyncList->asSyncData[i].ui32ReadOpsPendingVal = psSyncInfo->psSyncData->ui32ReadOpsPending;
				psHWDeviceSyncList->asSyncData[i].ui32WriteOpsPendingVal = psSyncInfo->psSyncData->ui32WriteOpsPending++;

	#if defined(PDUMP)
				if (PDumpIsCaptureFrameKM())
				{
					IMG_UINT32 ui32ModifiedValue;
					IMG_UINT32 ui32SyncOffset = offsetof(SGXMKIF_HWDEVICE_SYNC_LIST, asSyncData)
												+ (i * sizeof(PVRSRV_DEVICE_SYNC_OBJECT));
					IMG_UINT32 ui32WOpsOffset = ui32SyncOffset
												+ offsetof(PVRSRV_DEVICE_SYNC_OBJECT, ui32WriteOpsPendingVal);
					IMG_UINT32 ui32ROpsOffset = ui32SyncOffset
												+ offsetof(PVRSRV_DEVICE_SYNC_OBJECT, ui32ReadOpsPendingVal);

					PDUMPCOMMENT("HWDeviceSyncObject for RT: %i\r\n", i);

					PDUMPMEM(IMG_NULL,
							 psHWDstSyncListMemInfo,
							 ui32SyncOffset,
							 sizeof(PVRSRV_DEVICE_SYNC_OBJECT),
							 0,
							 MAKEUNIQUETAG(psHWDstSyncListMemInfo));

					if ((psSyncInfo->psSyncData->ui32LastOpDumpVal == 0) &&
						(psSyncInfo->psSyncData->ui32LastReadOpDumpVal == 0))
					{
						/*
						* Init the ROpsComplete value to 0.
						*/
						PDUMPCOMMENT("Init RT ROpsComplete\r\n", i);
						PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
							psSyncInfo->psSyncDataMemInfoKM,
							offsetof(PVRSRV_SYNC_DATA, ui32ReadOpsComplete),
							sizeof(psSyncInfo->psSyncData->ui32ReadOpsComplete),
							0,
							MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));
						/*
						* Init the WOpsComplete value to 0.
						*/
						PDUMPCOMMENT("Init RT WOpsComplete\r\n");
							PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
								psSyncInfo->psSyncDataMemInfoKM,
								offsetof(PVRSRV_SYNC_DATA, ui32WriteOpsComplete),
								sizeof(psSyncInfo->psSyncData->ui32WriteOpsComplete),
								0,
								MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));
					}

					psSyncInfo->psSyncData->ui32LastOpDumpVal++;

					ui32ModifiedValue = psSyncInfo->psSyncData->ui32LastOpDumpVal - 1;

					PDUMPCOMMENT("Modify RT %d WOpPendingVal in HWDevSyncList\r\n", i);

					PDUMPMEM(&ui32ModifiedValue,
						psHWDstSyncListMemInfo,
						ui32WOpsOffset,
						sizeof(IMG_UINT32),
						0,
						MAKEUNIQUETAG(psHWDstSyncListMemInfo));

					ui32ModifiedValue = 0;
					PDUMPCOMMENT("Modify RT %d ROpsPendingVal in HWDevSyncList\r\n", i);

					PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
						 psHWDstSyncListMemInfo,
						 ui32ROpsOffset,
						 sizeof(IMG_UINT32),
						 0,
						MAKEUNIQUETAG(psHWDstSyncListMemInfo));
				}
	#endif	/* defined(PDUMP) */
			}
			else
			{
				psHWDeviceSyncList->asSyncData[i].sWriteOpsCompleteDevVAddr.uiAddr = 0;
				psHWDeviceSyncList->asSyncData[i].sReadOpsCompleteDevVAddr.uiAddr = 0;

				psHWDeviceSyncList->asSyncData[i].ui32ReadOpsPendingVal = 0;
				psHWDeviceSyncList->asSyncData[i].ui32WriteOpsPendingVal = 0;
			}
		}
	}
	
	/* 
		NOTE: THIS MUST BE THE LAST THING WRITTEN TO THE TA COMMAND!
		Set the ready for so the uKernel will process the command.
	*/
	psTACmd->ui32CtrlFlags |= SGXMKIF_CMDTA_CTRLFLAGS_READY;

#if defined(PDUMP)
	if (PDumpIsCaptureFrameKM())
	{
		PDUMPCOMMENT("Shared part of TA command\r\n");

		PDUMPMEM(psTACmd,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff,
				 sizeof(SGXMKIF_CMDTA_SHARED),
				 0,
				 MAKEUNIQUETAG(psCCBMemInfo));

#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
		for (i=0; i<psCCBKick->ui32NumTASrcSyncs; i++)
		{
			IMG_UINT32 	ui32ModifiedValue;
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahTASrcKernelSyncInfo[i];

			if ((psSyncInfo->psSyncData->ui32LastOpDumpVal == 0) &&
				(psSyncInfo->psSyncData->ui32LastReadOpDumpVal == 0))
			{
				/*
				* Init the ROpsComplete value to 0.
				*/
				PDUMPCOMMENT("Init RT TA-SRC ROpsComplete\r\n", i);
				PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
					psSyncInfo->psSyncDataMemInfoKM,
					offsetof(PVRSRV_SYNC_DATA, ui32ReadOpsComplete),
					sizeof(psSyncInfo->psSyncData->ui32ReadOpsComplete),
					0,
					MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));
				/*
				* Init the WOpsComplete value to 0.
				*/
				PDUMPCOMMENT("Init RT TA-SRC WOpsComplete\r\n");
					PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
						psSyncInfo->psSyncDataMemInfoKM,
						offsetof(PVRSRV_SYNC_DATA, ui32WriteOpsComplete),
						sizeof(psSyncInfo->psSyncData->ui32WriteOpsComplete),
						0,
						MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));
			}

			psSyncInfo->psSyncData->ui32LastReadOpDumpVal++;

			ui32ModifiedValue = psSyncInfo->psSyncData->ui32LastReadOpDumpVal - 1;

			PDUMPCOMMENT("Modify TA SrcSync %d ROpsPendingVal\r\n", i);

			PDUMPMEM(&ui32ModifiedValue,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff + offsetof(SGXMKIF_CMDTA_SHARED, asTASrcSyncs) +
					(i * sizeof(PVRSRV_DEVICE_SYNC_OBJECT)) + offsetof(PVRSRV_DEVICE_SYNC_OBJECT, ui32ReadOpsPendingVal),
				 sizeof(IMG_UINT32),
				 0,
				MAKEUNIQUETAG(psCCBMemInfo));

			PDUMPCOMMENT("Modify TA SrcSync %d WOpPendingVal\r\n", i);

			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
				psCCBMemInfo,
				psCCBKick->ui32CCBDumpWOff + offsetof(SGXMKIF_CMDTA_SHARED, asTASrcSyncs) +
					(i * sizeof(PVRSRV_DEVICE_SYNC_OBJECT)) + offsetof(PVRSRV_DEVICE_SYNC_OBJECT, ui32WriteOpsPendingVal),
				sizeof(IMG_UINT32),
				0,
				MAKEUNIQUETAG(psCCBMemInfo));
		}

		for (i=0; i<psCCBKick->ui32NumTADstSyncs; i++)
		{
			IMG_UINT32 	ui32ModifiedValue;
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahTADstKernelSyncInfo[i];

			if ((psSyncInfo->psSyncData->ui32LastOpDumpVal == 0) &&
				(psSyncInfo->psSyncData->ui32LastReadOpDumpVal == 0))
			{
				/*
				 * Init the ROpsComplete value to 0.
				 */
				PDUMPCOMMENT("Init RT TA-DST ROpsComplete\r\n", i);
				PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
					psSyncInfo->psSyncDataMemInfoKM,
					offsetof(PVRSRV_SYNC_DATA, ui32ReadOpsComplete),
					sizeof(psSyncInfo->psSyncData->ui32ReadOpsComplete),
					0,
					MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));
				/*
				 * Init the WOpsComplete value to 0.
				 */
				PDUMPCOMMENT("Init RT TA-DST WOpsComplete\r\n");
					PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
						psSyncInfo->psSyncDataMemInfoKM,
						offsetof(PVRSRV_SYNC_DATA, ui32WriteOpsComplete),
						sizeof(psSyncInfo->psSyncData->ui32WriteOpsComplete),
						0,
						MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));
			}

			psSyncInfo->psSyncData->ui32LastOpDumpVal++;

			ui32ModifiedValue = psSyncInfo->psSyncData->ui32LastOpDumpVal - 1;

			PDUMPCOMMENT("Modify TA DstSync %d WOpPendingVal\r\n", i);

			PDUMPMEM(&ui32ModifiedValue,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff + offsetof(SGXMKIF_CMDTA_SHARED, asTADstSyncs) +
					(i * sizeof(PVRSRV_DEVICE_SYNC_OBJECT)) + offsetof(PVRSRV_DEVICE_SYNC_OBJECT, ui32WriteOpsPendingVal),
				 sizeof(IMG_UINT32),
				 0,
				MAKEUNIQUETAG(psCCBMemInfo));

			PDUMPCOMMENT("Modify TA DstSync %d ROpsPendingVal\r\n", i);

			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
				psCCBMemInfo,
				psCCBKick->ui32CCBDumpWOff + offsetof(SGXMKIF_CMDTA_SHARED, asTADstSyncs) +
					(i * sizeof(PVRSRV_DEVICE_SYNC_OBJECT)) + offsetof(PVRSRV_DEVICE_SYNC_OBJECT, ui32ReadOpsPendingVal),
				sizeof(IMG_UINT32),
				0,
				MAKEUNIQUETAG(psCCBMemInfo));
		}

		for (i=0; i<psCCBKick->ui32Num3DSrcSyncs; i++)
		{
			IMG_UINT32 	ui32ModifiedValue;
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ah3DSrcKernelSyncInfo[i];

			if ((psSyncInfo->psSyncData->ui32LastOpDumpVal == 0) &&
				(psSyncInfo->psSyncData->ui32LastReadOpDumpVal == 0))
			{
				/*
				* Init the ROpsComplete value to 0.
				*/
				PDUMPCOMMENT("Init RT 3D-SRC ROpsComplete\r\n", i);
				PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
					psSyncInfo->psSyncDataMemInfoKM,
					offsetof(PVRSRV_SYNC_DATA, ui32ReadOpsComplete),
					sizeof(psSyncInfo->psSyncData->ui32ReadOpsComplete),
					0,
					MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));
				/*
				* Init the WOpsComplete value to 0.
				*/
				PDUMPCOMMENT("Init RT 3D-SRC WOpsComplete\r\n");
					PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
						psSyncInfo->psSyncDataMemInfoKM,
						offsetof(PVRSRV_SYNC_DATA, ui32WriteOpsComplete),
						sizeof(psSyncInfo->psSyncData->ui32WriteOpsComplete),
						0,
						MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));
			}

			psSyncInfo->psSyncData->ui32LastReadOpDumpVal++;

			ui32ModifiedValue = psSyncInfo->psSyncData->ui32LastReadOpDumpVal - 1;

			PDUMPCOMMENT("Modify 3D SrcSync %d ROpsPendingVal\r\n", i);

			PDUMPMEM(&ui32ModifiedValue,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff + offsetof(SGXMKIF_CMDTA_SHARED, as3DSrcSyncs) +
					(i * sizeof(PVRSRV_DEVICE_SYNC_OBJECT)) + offsetof(PVRSRV_DEVICE_SYNC_OBJECT, ui32ReadOpsPendingVal),
				 sizeof(IMG_UINT32),
				 0,
				MAKEUNIQUETAG(psCCBMemInfo));

			PDUMPCOMMENT("Modify 3D SrcSync %d WOpPendingVal\r\n", i);

			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
				psCCBMemInfo,
				psCCBKick->ui32CCBDumpWOff + offsetof(SGXMKIF_CMDTA_SHARED, as3DSrcSyncs) +
					(i * sizeof(PVRSRV_DEVICE_SYNC_OBJECT)) + offsetof(PVRSRV_DEVICE_SYNC_OBJECT, ui32WriteOpsPendingVal),
				sizeof(IMG_UINT32),
				0,
				MAKEUNIQUETAG(psCCBMemInfo));
		}
#else/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */
		for (i=0; i<psCCBKick->ui32NumSrcSyncs; i++)
		{
			IMG_UINT32 	ui32ModifiedValue;
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahSrcKernelSyncInfo[i];

			if ((psSyncInfo->psSyncData->ui32LastOpDumpVal == 0) &&
				(psSyncInfo->psSyncData->ui32LastReadOpDumpVal == 0))
			{
				/*
				* Init the ROpsComplete value to 0.
				*/
				PDUMPCOMMENT("Init RT ROpsComplete\r\n");
				PDUMPMEM(&psSyncInfo->psSyncData->ui32LastReadOpDumpVal,
					psSyncInfo->psSyncDataMemInfoKM,
					offsetof(PVRSRV_SYNC_DATA, ui32ReadOpsComplete),
					sizeof(psSyncInfo->psSyncData->ui32ReadOpsComplete),
					0,
					MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));
				/*
				* Init the WOpsComplete value to 0.
				*/
				PDUMPCOMMENT("Init RT WOpsComplete\r\n");
					PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
						psSyncInfo->psSyncDataMemInfoKM,
						offsetof(PVRSRV_SYNC_DATA, ui32WriteOpsComplete),
						sizeof(psSyncInfo->psSyncData->ui32WriteOpsComplete),
						0,
						MAKEUNIQUETAG(psSyncInfo->psSyncDataMemInfoKM));
			}

			psSyncInfo->psSyncData->ui32LastReadOpDumpVal++;

			ui32ModifiedValue = psSyncInfo->psSyncData->ui32LastReadOpDumpVal - 1;

			PDUMPCOMMENT("Modify SrcSync %d ROpsPendingVal\r\n", i);

			PDUMPMEM(&ui32ModifiedValue,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff + offsetof(SGXMKIF_CMDTA_SHARED, asSrcSyncs) +
					(i * sizeof(PVRSRV_DEVICE_SYNC_OBJECT)) + offsetof(PVRSRV_DEVICE_SYNC_OBJECT, ui32ReadOpsPendingVal),
				 sizeof(IMG_UINT32),
				 0,
				MAKEUNIQUETAG(psCCBMemInfo));

			PDUMPCOMMENT("Modify SrcSync %d WOpPendingVal\r\n", i);

			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
				psCCBMemInfo,
				psCCBKick->ui32CCBDumpWOff + offsetof(SGXMKIF_CMDTA_SHARED, asSrcSyncs) +
					(i * sizeof(PVRSRV_DEVICE_SYNC_OBJECT)) + offsetof(PVRSRV_DEVICE_SYNC_OBJECT, ui32WriteOpsPendingVal),
				sizeof(IMG_UINT32),
				0,
				MAKEUNIQUETAG(psCCBMemInfo));
		}
#endif

		for (i = 0; i < psCCBKick->ui32NumTAStatusVals; i++)
		{
#if defined(SUPPORT_SGX_NEW_STATUS_VALS)
			PDUMPCOMMENT("Modify TA status value in TA cmd\r\n");
			PDUMPMEM(&psCCBKick->asTAStatusUpdate[i].ui32LastStatusUpdateDumpVal,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff + offsetof(SGXMKIF_CMDTA_SHARED, sCtlTAStatusInfo[i].ui32StatusValue),
				 sizeof(IMG_UINT32),
				 0,
				MAKEUNIQUETAG(psCCBMemInfo));
#else
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->ahTAStatusSyncInfo[i];
			PDUMPCOMMENT("Modify TA status value in TA cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff + offsetof(SGXMKIF_CMDTA_SHARED, sCtlTAStatusInfo[i].ui32StatusValue),
				 sizeof(IMG_UINT32),
				 0,
				MAKEUNIQUETAG(psCCBMemInfo));
#endif
		}

		for (i = 0; i < psCCBKick->ui32Num3DStatusVals; i++)
		{
#if !defined(SUPPORT_SGX_NEW_STATUS_VALS)
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->ah3DStatusSyncInfo[i];
			PDUMPCOMMENT("Modify 3D status value in TA cmd\r\n");
			PDUMPMEM(&psSyncInfo->psSyncData->ui32LastOpDumpVal,
				 psCCBMemInfo,
				 psCCBKick->ui32CCBDumpWOff + offsetof(SGXMKIF_CMDTA_SHARED, sCtl3DStatusInfo[i].ui32StatusValue),
				 sizeof(IMG_UINT32),
				 0,
				MAKEUNIQUETAG(psCCBMemInfo));
#endif
		}
	}
#endif	/* defined(PDUMP) */

	eError = SGXScheduleCCBCommandKM(hDevHandle, SGXMKIF_CMD_TA, &psCCBKick->sCommand, KERNEL_ID, 0);
	if (eError == PVRSRV_ERROR_RETRY)
	{
		if (psCCBKick->bFirstKickOrResume && psCCBKick->ui32NumDstSyncObjects > 0)
		{
			for (i=0; i < psCCBKick->ui32NumDstSyncObjects; i++)
			{

				psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->pahDstSyncHandles[i];

				if (psSyncInfo)
				{
					psSyncInfo->psSyncData->ui32WriteOpsPending--;
#if defined(PDUMP)
					if (PDumpIsCaptureFrameKM())
					{
						psSyncInfo->psSyncData->ui32LastOpDumpVal--;
					}
#endif
				}
			}
		}

#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
		for (i=0; i<psCCBKick->ui32NumTASrcSyncs; i++)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahTASrcKernelSyncInfo[i];
			psSyncInfo->psSyncData->ui32ReadOpsPending--;
		}
		for (i=0; i<psCCBKick->ui32NumTADstSyncs; i++)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahTADstKernelSyncInfo[i];
			psSyncInfo->psSyncData->ui32WriteOpsPending--;
		}
		for (i=0; i<psCCBKick->ui32Num3DSrcSyncs; i++)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ah3DSrcKernelSyncInfo[i];
			psSyncInfo->psSyncData->ui32ReadOpsPending--;
		}
#else/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */
		for (i=0; i<psCCBKick->ui32NumSrcSyncs; i++)
		{
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahSrcKernelSyncInfo[i];
			psSyncInfo->psSyncData->ui32ReadOpsPending--;
		}
#endif/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */

		return eError;
	}
	else if (PVRSRV_OK != eError)
	{
		PVR_DPF((PVR_DBG_ERROR, "SGXDoKickKM: SGXScheduleCCBCommandKM failed."));
		return eError;
	}


#if defined(NO_HARDWARE)


	/* TA/3D dependency */
	if (psCCBKick->hTA3DSyncInfo)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->hTA3DSyncInfo;

		if (psCCBKick->bTADependency)
		{
			psSyncInfo->psSyncData->ui32WriteOpsComplete = psSyncInfo->psSyncData->ui32WriteOpsPending;
		}
	}

	if (psCCBKick->hTASyncInfo != IMG_NULL)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->hTASyncInfo;

		psSyncInfo->psSyncData->ui32ReadOpsComplete =  psSyncInfo->psSyncData->ui32ReadOpsPending;
	}

	if (psCCBKick->h3DSyncInfo != IMG_NULL)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->h3DSyncInfo;

		psSyncInfo->psSyncData->ui32ReadOpsComplete =  psSyncInfo->psSyncData->ui32ReadOpsPending;
	}

	/* Copy status vals over */
	for (i = 0; i < psCCBKick->ui32NumTAStatusVals; i++)
	{
#if defined(SUPPORT_SGX_NEW_STATUS_VALS)
		PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo = (PVRSRV_KERNEL_MEM_INFO*)psCCBKick->asTAStatusUpdate[i].hKernelMemInfo;
		/* derive offset into meminfo and write the status value */
		*(IMG_UINT32*)((IMG_UINTPTR_T)psKernelMemInfo->pvLinAddrKM
						+ (psTACmd->sCtlTAStatusInfo[i].sStatusDevAddr.uiAddr
						- psKernelMemInfo->sDevVAddr.uiAddr)) = psTACmd->sCtlTAStatusInfo[i].ui32StatusValue;
#else
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->ahTAStatusSyncInfo[i];
		psSyncInfo->psSyncData->ui32ReadOpsComplete = psTACmd->sCtlTAStatusInfo[i].ui32StatusValue;
#endif
	}

#if defined(SUPPORT_SGX_GENERALISED_SYNCOBJECTS)
	/* SRC and DST dependencies */
	for (i=0; i<psCCBKick->ui32NumTASrcSyncs; i++)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahTASrcKernelSyncInfo[i];
		psSyncInfo->psSyncData->ui32ReadOpsComplete =  psSyncInfo->psSyncData->ui32ReadOpsPending;
	}
	for (i=0; i<psCCBKick->ui32NumTADstSyncs; i++)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahTADstKernelSyncInfo[i];
		psSyncInfo->psSyncData->ui32WriteOpsComplete =  psSyncInfo->psSyncData->ui32WriteOpsPending;
	}
	for (i=0; i<psCCBKick->ui32Num3DSrcSyncs; i++)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ah3DSrcKernelSyncInfo[i];
		psSyncInfo->psSyncData->ui32ReadOpsComplete =  psSyncInfo->psSyncData->ui32ReadOpsPending;
	}
#else/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */
	/* texture dependencies */
	for (i=0; i<psCCBKick->ui32NumSrcSyncs; i++)
	{
		psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *) psCCBKick->ahSrcKernelSyncInfo[i];
		psSyncInfo->psSyncData->ui32ReadOpsComplete =  psSyncInfo->psSyncData->ui32ReadOpsPending;
	}
#endif/* SUPPORT_SGX_GENERALISED_SYNCOBJECTS */

	if (psCCBKick->bTerminateOrAbort)
	{
		if (psCCBKick->ui32NumDstSyncObjects > 0)
		{
			PVRSRV_KERNEL_MEM_INFO	*psHWDstSyncListMemInfo =
								(PVRSRV_KERNEL_MEM_INFO *)psCCBKick->hKernelHWSyncListMemInfo;
			SGXMKIF_HWDEVICE_SYNC_LIST *psHWDeviceSyncList = psHWDstSyncListMemInfo->pvLinAddrKM;

			for (i=0; i<psCCBKick->ui32NumDstSyncObjects; i++)
			{
				psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->pahDstSyncHandles[i];
				if (psSyncInfo)
					psSyncInfo->psSyncData->ui32WriteOpsComplete = psHWDeviceSyncList->asSyncData[i].ui32WriteOpsPendingVal+1;
			}
		}

		/* Copy status vals over */
		for (i = 0; i < psCCBKick->ui32Num3DStatusVals; i++)
		{
#if defined(SUPPORT_SGX_NEW_STATUS_VALS)
			PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo = (PVRSRV_KERNEL_MEM_INFO*)psCCBKick->as3DStatusUpdate[i].hKernelMemInfo;
			/* derive offset into meminfo and write the status value */
			*(IMG_UINT32*)((IMG_UINTPTR_T)psKernelMemInfo->pvLinAddrKM
							+ (psTACmd->sCtl3DStatusInfo[i].sStatusDevAddr.uiAddr
							- psKernelMemInfo->sDevVAddr.uiAddr)) = psTACmd->sCtl3DStatusInfo[i].ui32StatusValue;
#else
			psSyncInfo = (PVRSRV_KERNEL_SYNC_INFO *)psCCBKick->ah3DStatusSyncInfo[i];
			psSyncInfo->psSyncData->ui32ReadOpsComplete = psTACmd->sCtl3DStatusInfo[i].ui32StatusValue;
#endif
		}
	}
#endif

	return eError;
}

/******************************************************************************
 End of file (sgxkick.c)
******************************************************************************/
