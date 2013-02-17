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

#include "perproc.h"
#include "sgxinfokm.h"

 
#define CCB_OFFSET_IS_VALID(type, psCCBMemInfo, psCCBKick, offset) \
	((sizeof(type) <= (psCCBMemInfo)->uAllocSize) && \
	((psCCBKick)->offset <= (psCCBMemInfo)->uAllocSize - sizeof(type)))

#define	CCB_DATA_FROM_OFFSET(type, psCCBMemInfo, psCCBKick, offset) \
	((type *)(((IMG_CHAR *)(psCCBMemInfo)->pvLinAddrKM) + \
		(psCCBKick)->offset))


IMG_IMPORT
IMG_VOID SGXTestActivePowerEvent(PVRSRV_DEVICE_NODE	*psDeviceNode,
								 IMG_UINT32			ui32CallerID);

IMG_IMPORT
PVRSRV_ERROR SGXScheduleCCBCommand(PVRSRV_DEVICE_NODE	*psDeviceNode,
								   SGXMKIF_CMD_TYPE		eCommandType,
								   SGXMKIF_COMMAND		*psCommandData,
								   IMG_UINT32			ui32CallerID,
								   IMG_UINT32			ui32PDumpFlags,
								   IMG_HANDLE			hDevMemContext,
								   IMG_BOOL				bLastInScene);
IMG_IMPORT
PVRSRV_ERROR SGXScheduleCCBCommandKM(PVRSRV_DEVICE_NODE		*psDeviceNode,
									 SGXMKIF_CMD_TYPE		eCommandType,
									 SGXMKIF_COMMAND		*psCommandData,
									 IMG_UINT32				ui32CallerID,
									 IMG_UINT32				ui32PDumpFlags,
									 IMG_HANDLE				hDevMemContext,
									 IMG_BOOL				bLastInScene);

IMG_IMPORT
PVRSRV_ERROR SGXScheduleProcessQueuesKM(PVRSRV_DEVICE_NODE *psDeviceNode);

IMG_IMPORT
IMG_BOOL SGXIsDevicePowered(PVRSRV_DEVICE_NODE *psDeviceNode);

IMG_IMPORT
IMG_HANDLE SGXRegisterHWRenderContextKM(IMG_HANDLE				psDeviceNode,
                                        IMG_CPU_VIRTADDR        *psHWRenderContextCpuVAddr,
                                        IMG_UINT32              ui32HWRenderContextSize,
                                        IMG_UINT32              ui32OffsetToPDDevPAddr,
                                        IMG_HANDLE              hDevMemContext,
                                        IMG_DEV_VIRTADDR        *psHWRenderContextDevVAddr,
										PVRSRV_PER_PROCESS_DATA *psPerProc);

IMG_IMPORT
IMG_HANDLE SGXRegisterHWTransferContextKM(IMG_HANDLE		      psDeviceNode,
                                          IMG_CPU_VIRTADDR        *psHWTransferContextCpuVAddr,
                                          IMG_UINT32              ui32HWTransferContextSize,
                                          IMG_UINT32              ui32OffsetToPDDevPAddr,
                                          IMG_HANDLE              hDevMemContext,
                                          IMG_DEV_VIRTADDR        *psHWTransferContextDevVAddr,
										  PVRSRV_PER_PROCESS_DATA *psPerProc);

IMG_IMPORT
PVRSRV_ERROR SGXFlushHWRenderTargetKM(IMG_HANDLE psSGXDevInfo,
									  IMG_DEV_VIRTADDR psHWRTDataSetDevVAddr,
									  IMG_BOOL bForceCleanup);

IMG_IMPORT
PVRSRV_ERROR SGXUnregisterHWRenderContextKM(IMG_HANDLE hHWRenderContext, IMG_BOOL bForceCleanup);

IMG_IMPORT
PVRSRV_ERROR SGXUnregisterHWTransferContextKM(IMG_HANDLE hHWTransferContext, IMG_BOOL bForceCleanup);

IMG_IMPORT
PVRSRV_ERROR SGXSetRenderContextPriorityKM(IMG_HANDLE       hDeviceNode,
                                           IMG_HANDLE       hHWRenderContext,
                                           IMG_UINT32       ui32Priority,
                                           IMG_UINT32       ui32OffsetOfPriorityField);

IMG_IMPORT
PVRSRV_ERROR SGXSetTransferContextPriorityKM(IMG_HANDLE       hDeviceNode,
                                             IMG_HANDLE       hHWTransferContext,
                                             IMG_UINT32       ui32Priority,
                                             IMG_UINT32       ui32OffsetOfPriorityField);

#if defined(SGX_FEATURE_2D_HARDWARE)
IMG_IMPORT
IMG_HANDLE SGXRegisterHW2DContextKM(IMG_HANDLE				psDeviceNode,
                                    IMG_CPU_VIRTADDR        *psHW2DContextCpuVAddr,
                                    IMG_UINT32              ui32HW2DContextSize,
                                    IMG_UINT32              ui32OffsetToPDDevPAddr,
                                    IMG_HANDLE              hDevMemContext,
                                    IMG_DEV_VIRTADDR        *psHW2DContextDevVAddr,
									PVRSRV_PER_PROCESS_DATA *psPerProc);

IMG_IMPORT
PVRSRV_ERROR SGXUnregisterHW2DContextKM(IMG_HANDLE hHW2DContext, IMG_BOOL bForceCleanup);
#endif

IMG_UINT32 SGXConvertTimeStamp(PVRSRV_SGXDEV_INFO	*psDevInfo,
							   IMG_UINT32			ui32TimeWraps,
							   IMG_UINT32			ui32Time);

IMG_VOID SGXWaitClocks(PVRSRV_SGXDEV_INFO	*psDevInfo,
					   IMG_UINT32			ui32SGXClocks);

PVRSRV_ERROR SGXCleanupRequest(PVRSRV_DEVICE_NODE	*psDeviceNode,
							IMG_DEV_VIRTADDR	*psHWDataDevVAddr,
							IMG_UINT32			ui32CleanupType,
							IMG_BOOL			bForceCleanup);

IMG_IMPORT
PVRSRV_ERROR PVRSRVGetSGXRevDataKM(PVRSRV_DEVICE_NODE* psDeviceNode, IMG_UINT32 *pui32SGXCoreRev,
				IMG_UINT32 *pui32SGXCoreID);

PVRSRV_ERROR SGXContextSuspend(PVRSRV_DEVICE_NODE	*psDeviceNode,
							   IMG_DEV_VIRTADDR		*psHWContextDevVAddr,
							   IMG_BOOL				bResume);

