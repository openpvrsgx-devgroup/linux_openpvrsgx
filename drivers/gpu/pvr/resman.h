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

#ifndef __RESMAN_H__
#define __RESMAN_H__

enum {
	RESMAN_TYPE_SHARED_PB_DESC = 1,
	RESMAN_TYPE_SHARED_PB_DESC_CREATE_LOCK,
	RESMAN_TYPE_HW_RENDER_CONTEXT,
	RESMAN_TYPE_HW_TRANSFER_CONTEXT,
	RESMAN_TYPE_HW_2D_CONTEXT,
	RESMAN_TYPE_TRANSFER_CONTEXT,

	RESMAN_TYPE_DISPLAYCLASS_SWAPCHAIN,
	RESMAN_TYPE_DISPLAYCLASS_DEVICE,

	RESMAN_TYPE_BUFFERCLASS_DEVICE,

	RESMAN_TYPE_OS_USERMODE_MAPPING,

	RESMAN_TYPE_DEVICEMEM_CONTEXT,
	RESMAN_TYPE_DEVICECLASSMEM_MAPPING,
	RESMAN_TYPE_DEVICEMEM_MAPPING,
	RESMAN_TYPE_DEVICEMEM_WRAP,
	RESMAN_TYPE_DEVICEMEM_ALLOCATION,
	RESMAN_TYPE_EVENT_OBJECT,
	RESMAN_TYPE_SHARED_MEM_INFO,

	RESMAN_TYPE_KERNEL_DEVICEMEM_ALLOCATION
};

#define RESMAN_CRITERIA_ALL				0x00000000
#define RESMAN_CRITERIA_RESTYPE				0x00000001
#define RESMAN_CRITERIA_PVOID_PARAM			0x00000002
#define RESMAN_CRITERIA_UI32_PARAM			0x00000004

struct RESMAN_ITEM;
struct RESMAN_CONTEXT;

enum PVRSRV_ERROR ResManInit(void);
void ResManDeInit(void);

struct RESMAN_ITEM *ResManRegisterRes(struct RESMAN_CONTEXT *hResManContext,
				      u32 ui32ResType, void *pvParam,
				      u32 ui32Param,
				      enum PVRSRV_ERROR (*pfnFreeResource)
						(void *pvParam, u32 ui32Param));

void ResManFreeResByPtr(struct RESMAN_ITEM *psResItem);

void ResManFreeResByCriteria(struct RESMAN_CONTEXT *hResManContext,
		u32 ui32SearchCriteria, u32 ui32ResType, void *pvParam,
		u32 ui32Param);

enum PVRSRV_ERROR ResManDissociateRes(struct RESMAN_ITEM *psResItem,
				      struct RESMAN_CONTEXT
				      *psNewResManContext);

enum PVRSRV_ERROR ResManFindResourceByPtr(struct RESMAN_CONTEXT *hResManContext,
		struct RESMAN_ITEM *psItem);

struct RESMAN_CONTEXT *pvr_get_resman_ctx(void *resource);
struct PVRSRV_PER_PROCESS_DATA *pvr_get_proc_by_ctx(struct RESMAN_CONTEXT *ctx);

enum PVRSRV_ERROR PVRSRVResManConnect(void *hPerProc,
		struct RESMAN_CONTEXT **phResManContext);

void PVRSRVResManDisconnect(struct RESMAN_CONTEXT *hResManContext,
		IMG_BOOL bKernelContext);

#endif
