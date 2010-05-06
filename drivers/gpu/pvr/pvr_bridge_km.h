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

#ifndef __PVR_BRIDGE_KM_H_
#define __PVR_BRIDGE_KM_H_

#include <linux/fs.h>			/* for struct file */
#include <linux/wait.h>
#include <linux/sched.h>

#include "pvr_bridge.h"
#include "perproc.h"

extern int pvr_dvfs_active;
extern struct mutex gPVRSRVLock;
extern wait_queue_head_t pvr_dvfs_wq;

void pvr_dvfs_wait_active(void);

static inline void pvr_dvfs_lock(void)
{
	mutex_lock(&gPVRSRVLock);
	pvr_dvfs_active = 1;
	mutex_unlock(&gPVRSRVLock);
}

static inline void pvr_dvfs_unlock(void)
{
	mutex_lock(&gPVRSRVLock);
	pvr_dvfs_active = 0;
	wake_up(&pvr_dvfs_wq);
	mutex_unlock(&gPVRSRVLock);
}

static inline void pvr_lock(void)
{
	mutex_lock(&gPVRSRVLock);
	if (pvr_dvfs_active)
		pvr_dvfs_wait_active();
}

static inline void pvr_unlock(void)
{
	mutex_unlock(&gPVRSRVLock);
}

static inline int pvr_is_locked(void)
{
	return mutex_is_locked(&gPVRSRVLock);
}

enum PVRSRV_ERROR LinuxBridgeInit(void);
void LinuxBridgeDeInit(void);

enum PVRSRV_ERROR PVRSRVEnumerateDevicesKM(u32 *pui32NumDevices,
		struct PVRSRV_DEVICE_IDENTIFIER *psDevIdList);

enum PVRSRV_ERROR PVRSRVAcquireDeviceDataKM(u32 uiDevIndex,
		enum PVRSRV_DEVICE_TYPE eDeviceType,
		void **phDevCookie);

enum PVRSRV_ERROR PVRSRVCreateCommandQueueKM(u32 ui32QueueSize,
		struct PVRSRV_QUEUE_INFO **ppsQueueInfo);

enum PVRSRV_ERROR PVRSRVDestroyCommandQueueKM(
					struct PVRSRV_QUEUE_INFO *psQueueInfo);

enum PVRSRV_ERROR PVRSRVGetDeviceMemHeapsKM(void *hDevCookie,
		struct PVRSRV_HEAP_INFO *psHeapInfo);

enum PVRSRV_ERROR PVRSRVCreateDeviceMemContextKM(void *hDevCookie,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc,
		void **phDevMemContext, u32 *pui32ClientHeapCount,
		struct PVRSRV_HEAP_INFO *psHeapInfo, IMG_BOOL *pbCreated,
		IMG_BOOL *pbShared);

enum PVRSRV_ERROR PVRSRVDestroyDeviceMemContextKM(void *hDevCookie,
		void *hDevMemContext, IMG_BOOL *pbCreated);

enum PVRSRV_ERROR PVRSRVGetDeviceMemHeapInfoKM(void *hDevCookie,
		void *hDevMemContext, u32 *pui32ClientHeapCount,
		struct PVRSRV_HEAP_INFO *psHeapInfo, IMG_BOOL *pbShared);

enum PVRSRV_ERROR PVRSRVAllocDeviceMemKM(void *hDevCookie,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc, void *hDevMemHeap,
		u32 ui32Flags, u32 ui32Size, u32 ui32Alignment,
		struct PVRSRV_KERNEL_MEM_INFO **ppsMemInfo);

enum PVRSRV_ERROR PVRSRVFreeDeviceMemKM(void *hDevCookie,
		struct PVRSRV_KERNEL_MEM_INFO *psMemInfo);

enum PVRSRV_ERROR PVRSRVDissociateDeviceMemKM(void *hDevCookie,
		struct PVRSRV_KERNEL_MEM_INFO *psMemInfo);

enum PVRSRV_ERROR PVRSRVReserveDeviceVirtualMemKM(void *hDevMemHeap,
		struct IMG_DEV_VIRTADDR *psDevVAddr, u32 ui32Size,
		u32 ui32Alignment, struct PVRSRV_KERNEL_MEM_INFO **ppsMemInfo);

enum PVRSRV_ERROR PVRSRVFreeDeviceVirtualMemKM(
		struct PVRSRV_KERNEL_MEM_INFO *psMemInfo);

enum PVRSRV_ERROR PVRSRVMapDeviceMemoryKM(
		struct PVRSRV_PER_PROCESS_DATA *psPerProc,
		struct PVRSRV_KERNEL_MEM_INFO *psSrcMemInfo,
		void *hDstDevMemHeap,
		struct PVRSRV_KERNEL_MEM_INFO **ppsDstMemInfo);

enum PVRSRV_ERROR PVRSRVUnmapDeviceMemoryKM(
		struct PVRSRV_KERNEL_MEM_INFO *psMemInfo);

enum PVRSRV_ERROR PVRSRVWrapExtMemoryKM(void *hDevCookie,
		struct PVRSRV_PER_PROCESS_DATA *psPerProc, void *hDevMemContext,
		u32 ui32ByteSize, u32 ui32PageOffset, IMG_BOOL bPhysContig,
		struct IMG_SYS_PHYADDR *psSysAddr, void *pvLinAddr,
		struct PVRSRV_KERNEL_MEM_INFO **ppsMemInfo);

enum PVRSRV_ERROR PVRSRVUnwrapExtMemoryKM(struct PVRSRV_KERNEL_MEM_INFO
					      *psMemInfo);

enum PVRSRV_ERROR PVRSRVEnumerateDCKM(enum PVRSRV_DEVICE_CLASS DeviceClass,
		u32 *pui32DevCount, u32 *pui32DevID);

enum PVRSRV_ERROR PVRSRVOpenDCDeviceKM(
		struct PVRSRV_PER_PROCESS_DATA *psPerProc,
		u32 ui32DeviceID, void *hDevCookie, void **phDeviceKM);

enum PVRSRV_ERROR PVRSRVCloseDCDeviceKM(void *hDeviceKM,
		IMG_BOOL bResManCallback);

enum PVRSRV_ERROR PVRSRVEnumDCFormatsKM(void *hDeviceKM, u32 *pui32Count,
		struct DISPLAY_FORMAT *psFormat);

enum PVRSRV_ERROR PVRSRVEnumDCDimsKM(void *hDeviceKM,
		struct DISPLAY_FORMAT *psFormat, u32 *pui32Count,
		struct DISPLAY_DIMS *psDim);

enum PVRSRV_ERROR PVRSRVGetDCSystemBufferKM(void *hDeviceKM, void **phBuffer);

enum PVRSRV_ERROR PVRSRVGetDCInfoKM(void *hDeviceKM,
		struct DISPLAY_INFO *psDisplayInfo);

enum PVRSRV_ERROR PVRSRVCreateDCSwapChainKM(
		struct PVRSRV_PER_PROCESS_DATA *psPerProc, void *hDeviceKM,
		u32 ui32Flags, struct DISPLAY_SURF_ATTRIBUTES *psDstSurfAttrib,
		struct DISPLAY_SURF_ATTRIBUTES *psSrcSurfAttrib,
		u32 ui32BufferCount, u32 ui32OEMFlags, void **phSwapChain,
		u32 *pui32SwapChainID);

enum PVRSRV_ERROR PVRSRVDestroyDCSwapChainKM(void *hSwapChain);
enum PVRSRV_ERROR PVRSRVSetDCDstRectKM(void *hDeviceKM, void *hSwapChain,
		struct IMG_RECT *psRect);
enum PVRSRV_ERROR PVRSRVSetDCSrcRectKM(void *hDeviceKM, void *hSwapChain,
		struct IMG_RECT *psRect);
enum PVRSRV_ERROR PVRSRVSetDCDstColourKeyKM(void *hDeviceKM, void *hSwapChain,
		u32 ui32CKColour);
enum PVRSRV_ERROR PVRSRVSetDCSrcColourKeyKM(void *hDeviceKM, void *hSwapChain,
		u32 ui32CKColour);
enum PVRSRV_ERROR PVRSRVGetDCBuffersKM(void *hDeviceKM, void *hSwapChain,
		u32 *pui32BufferCount, void **phBuffer);
enum PVRSRV_ERROR PVRSRVSwapToDCBufferKM(void *hDeviceKM, void *hBuffer,
		u32 ui32SwapInterval, void *hPrivateTag,
		u32 ui32ClipRectCount, struct IMG_RECT *psClipRect);
enum PVRSRV_ERROR PVRSRVSwapToDCSystemKM(void *hDeviceKM, void *hSwapChain);

enum PVRSRV_ERROR PVRSRVOpenBCDeviceKM(
		struct PVRSRV_PER_PROCESS_DATA *psPerProc,
		u32 ui32DeviceID, void *hDevCookie, void **phDeviceKM);
enum PVRSRV_ERROR PVRSRVCloseBCDeviceKM(void *hDeviceKM,
		IMG_BOOL bResManCallback);

enum PVRSRV_ERROR PVRSRVGetBCInfoKM(void *hDeviceKM,
		struct BUFFER_INFO *psBufferInfo);
enum PVRSRV_ERROR PVRSRVGetBCBufferKM(void *hDeviceKM,
		u32 ui32BufferIndex, void **phBuffer);

enum PVRSRV_ERROR PVRSRVMapDeviceClassMemoryKM(
		struct PVRSRV_PER_PROCESS_DATA *psPerProc, void *hDevMemContext,
		void *hDeviceClassBuffer,
		struct PVRSRV_KERNEL_MEM_INFO **ppsMemInfo, void **phOSMapInfo);

enum PVRSRV_ERROR PVRSRVUnmapDeviceClassMemoryKM(
		struct PVRSRV_KERNEL_MEM_INFO *psMemInfo);

enum PVRSRV_ERROR PVRSRVGetFreeDeviceMemKM(u32 ui32Flags, u32 *pui32Total,
		u32 *pui32Free, u32 *pui32LargestBlock);
enum PVRSRV_ERROR PVRSRVAllocSyncInfoKM(void *hDevCookie, void *hDevMemContext,
		struct PVRSRV_KERNEL_SYNC_INFO **ppsKernelSyncInfo);
enum PVRSRV_ERROR PVRSRVFreeSyncInfoKM(
		struct PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo);

enum PVRSRV_ERROR PVRSRVGetMiscInfoKM(struct PVRSRV_MISC_INFO *psMiscInfo);

enum PVRSRV_ERROR PVRSRVGetFBStatsKM(u32 *pui32Total, u32 *pui32Available);

enum PVRSRV_ERROR PVRSRVAllocSharedSysMemoryKM(
		struct PVRSRV_PER_PROCESS_DATA *psPerProc, u32 ui32Flags,
		u32 ui32Size, struct PVRSRV_KERNEL_MEM_INFO **ppsKernelMemInfo);

enum PVRSRV_ERROR PVRSRVFreeSharedSysMemoryKM(
		struct PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo);

enum PVRSRV_ERROR PVRSRVDissociateMemFromResmanKM(
		struct PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo);

long PVRSRV_BridgeDispatchKM(struct file *, unsigned, unsigned long);

#endif
