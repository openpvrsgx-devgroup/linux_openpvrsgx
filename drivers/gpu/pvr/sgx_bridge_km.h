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

#if !defined(__SGX_BRIDGE_KM_H__)
#define __SGX_BRIDGE_KM_H__

#include "sgxapi_km.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"
#include "sgx_bridge.h"
#include "pvr_bridge.h"
#include "perproc.h"


enum PVRSRV_ERROR SGXSubmitTransferKM(void *hDevHandle,
		struct PVRSRV_TRANSFER_SGX_KICK *psKick);


enum PVRSRV_ERROR SGXDoKickKM(void *hDevHandle, struct SGX_CCB_KICK *psCCBKick,
			      int max_3dstat_val);

enum PVRSRV_ERROR SGXGetPhysPageAddrKM(void *hDevMemHeap,
		struct IMG_DEV_VIRTADDR sDevVAddr,
		struct IMG_DEV_PHYADDR *pDevPAddr,
		struct IMG_CPU_PHYADDR *pCpuPAddr);

enum PVRSRV_ERROR SGXGetMMUPDAddrKM(void *hDevCookie,
		void *hDevMemContext, struct IMG_DEV_PHYADDR *psPDDevPAddr);

enum PVRSRV_ERROR SGXGetClientInfoKM(void *hDevCookie,
					 struct SGX_CLIENT_INFO *psClientInfo);

enum PVRSRV_ERROR SGXGetMiscInfoKM(struct PVRSRV_SGXDEV_INFO *psDevInfo,
				       struct SGX_MISC_INFO *psMiscInfo,
				       struct PVRSRV_DEVICE_NODE *psDeviceNode);

enum PVRSRV_ERROR SGXReadDiffCountersKM(void *hDevHandle, u32 ui32Reg,
			    u32 *pui32Old, IMG_BOOL bNew, u32 ui32New,
			    u32 ui32NewReset, u32 ui32CountersReg,
			    u32 *pui32Time, IMG_BOOL *pbActive,
			    struct PVRSRV_SGXDEV_DIFF_INFO *psDiffs);
enum PVRSRV_ERROR SGXReadHWPerfCBKM(void *hDevHandle, u32 ui32ArraySize,
			    struct PVRSRV_SGX_HWPERF_CB_ENTRY *psHWPerfCBData,
			    u32 *pui32DataCount, u32 *pui32ClockSpeed,
			    u32 *pui32HostTimeStamp);

enum PVRSRV_ERROR SGX2DQueryBlitsCompleteKM(
			    struct PVRSRV_SGXDEV_INFO *psDevInfo,
			    struct PVRSRV_KERNEL_SYNC_INFO *psSyncInfo,
			    IMG_BOOL bWaitForComplete);

enum PVRSRV_ERROR SGXGetInfoForSrvinitKM(void *hDevHandle,
			    struct SGX_BRIDGE_INFO_FOR_SRVINIT *psInitInfo);

enum PVRSRV_ERROR DevInitSGXPart2KM(struct PVRSRV_PER_PROCESS_DATA *psPerProc,
			    void *hDevHandle,
			    struct SGX_BRIDGE_INIT_INFO *psInitInfo);

enum PVRSRV_ERROR SGXFindSharedPBDescKM(
	struct PVRSRV_PER_PROCESS_DATA *psPerProc,
	void *hDevCookie, IMG_BOOL bLockOnFailure, u32 ui32TotalPBSize,
	void **phSharedPBDesc,
	struct PVRSRV_KERNEL_MEM_INFO **ppsSharedPBDescKernelMemInfo,
	struct PVRSRV_KERNEL_MEM_INFO **ppsHWPBDescKernelMemInfo,
	struct PVRSRV_KERNEL_MEM_INFO **ppsBlockKernelMemInfo,
	struct PVRSRV_KERNEL_MEM_INFO ***pppsSharedPBDescSubKernelMemInfos,
	u32 *ui32SharedPBDescSubKernelMemInfosCount);

enum PVRSRV_ERROR SGXUnrefSharedPBDescKM(void *hSharedPBDesc);

enum PVRSRV_ERROR SGXAddSharedPBDescKM(
		struct PVRSRV_PER_PROCESS_DATA *psPerProc,
		void *hDevCookie,
		struct PVRSRV_KERNEL_MEM_INFO *psSharedPBDescKernelMemInfo,
		struct PVRSRV_KERNEL_MEM_INFO *psHWPBDescKernelMemInfo,
		struct PVRSRV_KERNEL_MEM_INFO *psBlockKernelMemInfo,
		u32 ui32TotalPBSize, void **phSharedPBDesc,
		struct PVRSRV_KERNEL_MEM_INFO **psSharedPBDescSubKernelMemInfos,
		u32 ui32SharedPBDescSubKernelMemInfosCount);

enum PVRSRV_ERROR SGXGetInternalDevInfoKM(void *hDevCookie,
			struct SGX_INTERNAL_DEVINFO *psSGXInternalDevInfo);

int sgx_force_reset(void);

#endif
