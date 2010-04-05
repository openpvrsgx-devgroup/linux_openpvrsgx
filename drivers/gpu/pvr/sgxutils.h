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

#include "perproc.h"

#define CCB_OFFSET_IS_VALID(type, psCCBMemInfo, psCCBKick, offset)	\
	((sizeof(type) <= (psCCBMemInfo)->ui32AllocSize) &&		\
	((psCCBKick)->offset <= (psCCBMemInfo)->ui32AllocSize - sizeof(type)))

#define	CCB_DATA_FROM_OFFSET(type, psCCBMemInfo, psCCBKick, offset)	\
	((type *)(((char *)(psCCBMemInfo)->pvLinAddrKM) +		\
		(psCCBKick)->offset))

void SGXTestActivePowerEvent(struct PVRSRV_DEVICE_NODE *psDeviceNode);

enum PVRSRV_ERROR SGXScheduleCCBCommand(
				struct PVRSRV_SGXDEV_INFO *psDevInfo,
				enum SGXMKIF_COMMAND_TYPE eCommandType,
				struct SGXMKIF_COMMAND *psCommandData,
				u32 ui32CallerID, u32 ui32PDumpFlags);
enum PVRSRV_ERROR SGXScheduleCCBCommandKM(
				struct PVRSRV_DEVICE_NODE *psDeviceNode,
				enum SGXMKIF_COMMAND_TYPE eCommandType,
				struct SGXMKIF_COMMAND *psCommandData,
				u32 ui32CallerID, u32 ui32PDumpFlags);

enum PVRSRV_ERROR SGXScheduleProcessQueuesKM(
				struct PVRSRV_DEVICE_NODE *psDeviceNode);

IMG_BOOL SGXIsDevicePowered(struct PVRSRV_DEVICE_NODE *psDeviceNode);

void *SGXRegisterHWRenderContextKM(void *psDeviceNode,
			 struct IMG_DEV_VIRTADDR *psHWRenderContextDevVAddr,
			 struct PVRSRV_PER_PROCESS_DATA *psPerProc);

void *SGXRegisterHWTransferContextKM(void *psDeviceNode,
			 struct IMG_DEV_VIRTADDR *psHWTransferContextDevVAddr,
			 struct PVRSRV_PER_PROCESS_DATA *psPerProc);

void SGXFlushHWRenderTargetKM(void *psSGXDevInfo,
			 struct IMG_DEV_VIRTADDR psHWRTDataSetDevVAddr);

enum PVRSRV_ERROR SGXUnregisterHWRenderContextKM(void *hHWRenderContext);

enum PVRSRV_ERROR SGXUnregisterHWTransferContextKM(void *hHWTransferContext);

u32 SGXConvertTimeStamp(struct PVRSRV_SGXDEV_INFO *psDevInfo,
			u32 ui32TimeWraps, u32 ui32Time);

void SGXCleanupRequest(struct PVRSRV_DEVICE_NODE *psDeviceNode,
		       struct IMG_DEV_VIRTADDR *psHWDataDevVAddr,
		       u32 ui32ResManRequestFlag);
