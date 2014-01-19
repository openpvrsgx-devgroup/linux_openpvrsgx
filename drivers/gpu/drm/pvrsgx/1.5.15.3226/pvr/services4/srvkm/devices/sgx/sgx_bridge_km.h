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

#if !defined(__SGX_BRIDGE_KM_H__)
#define __SGX_BRIDGE_KM_H__

#include "sgxapi_km.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "sgx_bridge.h"
#include "pvr_bridge.h"
#include "perproc.h"

#if defined (__cplusplus)
extern "C" {
#endif

IMG_IMPORT
PVRSRV_ERROR SGXSubmitTransferKM(IMG_HANDLE hDevHandle, PVRSRV_TRANSFER_SGX_KICK *psKick);

#if defined(SGX_FEATURE_2D_HARDWARE)
IMG_IMPORT
PVRSRV_ERROR SGXSubmit2DKM(IMG_HANDLE hDevHandle, PVRSRV_2D_SGX_KICK *psKick);
#endif

IMG_IMPORT
PVRSRV_ERROR SGXDoKickKM(IMG_HANDLE hDevHandle,
						 SGX_CCB_KICK *psCCBKick);

IMG_IMPORT
PVRSRV_ERROR SGXGetPhysPageAddrKM(IMG_HANDLE hDevMemHeap,
								  IMG_DEV_VIRTADDR sDevVAddr,
								  IMG_DEV_PHYADDR *pDevPAddr,
								  IMG_CPU_PHYADDR *pCpuPAddr);

IMG_IMPORT
PVRSRV_ERROR IMG_CALLCONV SGXGetMMUPDAddrKM(IMG_HANDLE		hDevCookie,
											IMG_HANDLE		hDevMemContext,
											IMG_DEV_PHYADDR	*psPDDevPAddr);

IMG_IMPORT
PVRSRV_ERROR SGXGetClientInfoKM(IMG_HANDLE				hDevCookie,
								SGX_CLIENT_INFO*	psClientInfo);

IMG_IMPORT
PVRSRV_ERROR SGXGetMiscInfoKM(PVRSRV_SGXDEV_INFO	*psDevInfo,
							  SGX_MISC_INFO			*psMiscInfo,
							  PVRSRV_DEVICE_NODE 	*psDeviceNode,
							  IMG_HANDLE 			 hDevMemContext);

#if defined(SUPPORT_SGX_HWPERF)
IMG_IMPORT
PVRSRV_ERROR SGXReadDiffCountersKM(IMG_HANDLE				hDevHandle,
								   IMG_UINT32				ui32Reg,
								   IMG_UINT32				*pui32Old,
								   IMG_BOOL					bNew,
								   IMG_UINT32				ui32New,
								   IMG_UINT32				ui32NewReset,
								   IMG_UINT32				ui32CountersReg,
								   IMG_UINT32				ui32Reg2,
								   IMG_BOOL					*pbActive,
								   PVRSRV_SGXDEV_DIFF_INFO	*psDiffs);
IMG_IMPORT
PVRSRV_ERROR SGXReadHWPerfCBKM(IMG_HANDLE					hDevHandle,
							   IMG_UINT32					ui32ArraySize,
							   PVRSRV_SGX_HWPERF_CB_ENTRY	*psHWPerfCBData,
							   IMG_UINT32					*pui32DataCount,
							   IMG_UINT32					*pui32ClockSpeed,
							   IMG_UINT32					*pui32HostTimeStamp);
#endif

IMG_IMPORT
PVRSRV_ERROR SGX2DQueryBlitsCompleteKM(PVRSRV_SGXDEV_INFO		*psDevInfo,
									   PVRSRV_KERNEL_SYNC_INFO	*psSyncInfo,
									   IMG_BOOL bWaitForComplete);

IMG_IMPORT
PVRSRV_ERROR SGXGetInfoForSrvinitKM(IMG_HANDLE hDevHandle,
									SGX_BRIDGE_INFO_FOR_SRVINIT *psInitInfo);

IMG_IMPORT
PVRSRV_ERROR DevInitSGXPart2KM(PVRSRV_PER_PROCESS_DATA *psPerProc,
							   IMG_HANDLE hDevHandle,
							   SGX_BRIDGE_INIT_INFO *psInitInfo);

IMG_IMPORT PVRSRV_ERROR
SGXFindSharedPBDescKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
					  IMG_HANDLE				hDevCookie,
					  IMG_BOOL				bLockOnFailure,
					  IMG_UINT32				ui32TotalPBSize,
					  IMG_HANDLE				*phSharedPBDesc,
					  PVRSRV_KERNEL_MEM_INFO	**ppsSharedPBDescKernelMemInfo,
					  PVRSRV_KERNEL_MEM_INFO	**ppsHWPBDescKernelMemInfo,
					  PVRSRV_KERNEL_MEM_INFO	**ppsBlockKernelMemInfo,
					  PVRSRV_KERNEL_MEM_INFO	**ppsHWBlockKernelMemInfo,
					  PVRSRV_KERNEL_MEM_INFO	***pppsSharedPBDescSubKernelMemInfos,
					  IMG_UINT32				*ui32SharedPBDescSubKernelMemInfosCount);

IMG_IMPORT PVRSRV_ERROR
SGXUnrefSharedPBDescKM(IMG_HANDLE hSharedPBDesc);

IMG_IMPORT PVRSRV_ERROR
SGXAddSharedPBDescKM(PVRSRV_PER_PROCESS_DATA	*psPerProc,
					 IMG_HANDLE 				hDevCookie,
					 PVRSRV_KERNEL_MEM_INFO		*psSharedPBDescKernelMemInfo,
					 PVRSRV_KERNEL_MEM_INFO		*psHWPBDescKernelMemInfo,
					 PVRSRV_KERNEL_MEM_INFO		*psBlockKernelMemInfo,
					 PVRSRV_KERNEL_MEM_INFO		*psHWBlockKernelMemInfo,
					 IMG_UINT32					ui32TotalPBSize,
					 IMG_HANDLE					*phSharedPBDesc,
					 PVRSRV_KERNEL_MEM_INFO		**psSharedPBDescSubKernelMemInfos,
					 IMG_UINT32					ui32SharedPBDescSubKernelMemInfosCount);


IMG_IMPORT PVRSRV_ERROR
SGXGetInternalDevInfoKM(IMG_HANDLE hDevCookie,
						SGX_INTERNAL_DEVINFO *psSGXInternalDevInfo);

#if defined (__cplusplus)
}
#endif

#endif

