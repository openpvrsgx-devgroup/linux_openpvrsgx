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

#include <linux/version.h>
#include <linux/sched.h>
#include <linux/hardirq.h>

#include <linux/semaphore.h>

#include "services_headers.h"
#include "resman.h"

static DECLARE_MUTEX(lock);

#define ACQUIRE_SYNC_OBJ  do {						   \
		if (in_interrupt()) {					   \
			printk(KERN_ERR "ISR cannot take RESMAN mutex\n"); \
			BUG();						   \
	} else								\
			down(&lock);					   \
} while (0)
#define RELEASE_SYNC_OBJ up(&lock)


#define RESMAN_SIGNATURE 0x12345678

struct RESMAN_ITEM {
#ifdef DEBUG
	u32 ui32Signature;
#endif
	struct RESMAN_ITEM **ppsThis;
	struct RESMAN_ITEM *psNext;

	u32 ui32Flags;
	u32 ui32ResType;

	void *pvParam;
	u32 ui32Param;

	enum PVRSRV_ERROR (*pfnFreeResource)(void *pvParam, u32 ui32Param);
};

struct RESMAN_CONTEXT {
#ifdef DEBUG
	u32 ui32Signature;
#endif
	struct RESMAN_CONTEXT **ppsThis;
	struct RESMAN_CONTEXT *psNext;
	struct PVRSRV_PER_PROCESS_DATA *psPerProc;
	struct RESMAN_ITEM *psResItemList;

};

struct RESMAN_LIST {
	struct RESMAN_CONTEXT *psContextList;
};

static struct RESMAN_LIST *gpsResList;

#define PRINT_RESLIST(x, y, z)

static void FreeResourceByPtr(struct RESMAN_ITEM *psItem,
					   IMG_BOOL bExecuteCallback);

static int FreeResourceByCriteria(struct RESMAN_CONTEXT *psContext,
					   u32 ui32SearchCriteria,
					   u32 ui32ResType, void *pvParam,
					   u32 ui32Param,
					   IMG_BOOL bExecuteCallback);

#ifdef DEBUG
static void ValidateResList(struct RESMAN_LIST *psResList);
#define VALIDATERESLIST() ValidateResList(gpsResList)
#else
#define VALIDATERESLIST()
#endif

enum PVRSRV_ERROR ResManInit(void)
{
	if (gpsResList == NULL) {

		if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			       sizeof(*gpsResList),
			       (void **) &gpsResList,
			       NULL) != PVRSRV_OK)
			return PVRSRV_ERROR_OUT_OF_MEMORY;

		gpsResList->psContextList = NULL;

		VALIDATERESLIST();
	}

	return PVRSRV_OK;
}

void ResManDeInit(void)
{
	if (gpsResList != NULL)
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*gpsResList),
			  gpsResList, NULL);
}

enum PVRSRV_ERROR PVRSRVResManConnect(void *hPerProc,
				 struct RESMAN_CONTEXT **phResManContext)
{
	enum PVRSRV_ERROR eError;
	struct RESMAN_CONTEXT *psResManContext;

	ACQUIRE_SYNC_OBJ;

	VALIDATERESLIST();

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(*psResManContext),
			    (void **) &psResManContext, NULL);
	if (eError != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "PVRSRVResManConnect: "
				"ERROR allocating new RESMAN context struct");

		VALIDATERESLIST();
		RELEASE_SYNC_OBJ;

		return eError;
	}
#ifdef DEBUG
	psResManContext->ui32Signature = RESMAN_SIGNATURE;
#endif
	psResManContext->psResItemList = NULL;
	psResManContext->psPerProc = hPerProc;

	psResManContext->psNext = gpsResList->psContextList;
	psResManContext->ppsThis = &gpsResList->psContextList;
	gpsResList->psContextList = psResManContext;
	if (psResManContext->psNext)
		psResManContext->psNext->ppsThis = &(psResManContext->psNext);

	VALIDATERESLIST();

	RELEASE_SYNC_OBJ;

	*phResManContext = psResManContext;

	return PVRSRV_OK;
}

static inline bool warn_unfreed_res(void)
{
	return !(current->flags & PF_SIGNALED);
}

static int free_one_res(struct RESMAN_CONTEXT *ctx, u32 restype)
{
	int freed;

	freed = FreeResourceByCriteria(ctx, RESMAN_CRITERIA_RESTYPE, restype,
					NULL, 0, IMG_TRUE);
	if (freed && warn_unfreed_res())
		PVR_DPF(DBGPRIV_WARNING, "pvr: %s: cleaning up %d "
			   "unfreed resource of type %d\n",
			   current->comm, freed, restype);

	return freed;
}

void PVRSRVResManDisconnect(struct RESMAN_CONTEXT *ctx, IMG_BOOL bKernelContext)
{

	ACQUIRE_SYNC_OBJ;

	VALIDATERESLIST();

	PRINT_RESLIST(gpsResList, ctx, IMG_TRUE);

	if (!bKernelContext) {
		int i = 0;

		i += free_one_res(ctx, RESMAN_TYPE_OS_USERMODE_MAPPING);
		i += free_one_res(ctx, RESMAN_TYPE_EVENT_OBJECT);
		i += free_one_res(ctx, RESMAN_TYPE_HW_RENDER_CONTEXT);
		i += free_one_res(ctx, RESMAN_TYPE_HW_TRANSFER_CONTEXT);
		i += free_one_res(ctx, RESMAN_TYPE_HW_2D_CONTEXT);
		i += free_one_res(ctx, RESMAN_TYPE_TRANSFER_CONTEXT);
		i += free_one_res(ctx, RESMAN_TYPE_SHARED_PB_DESC_CREATE_LOCK);
		i += free_one_res(ctx, RESMAN_TYPE_SHARED_PB_DESC);
		i += free_one_res(ctx, RESMAN_TYPE_DISPLAYCLASS_SWAPCHAIN);
		i += free_one_res(ctx, RESMAN_TYPE_DISPLAYCLASS_DEVICE);
		i += free_one_res(ctx, RESMAN_TYPE_BUFFERCLASS_DEVICE);
		i += free_one_res(ctx, RESMAN_TYPE_DEVICECLASSMEM_MAPPING);
		i += free_one_res(ctx, RESMAN_TYPE_DEVICEMEM_WRAP);
		i += free_one_res(ctx, RESMAN_TYPE_DEVICEMEM_MAPPING);
		i += free_one_res(ctx, RESMAN_TYPE_KERNEL_DEVICEMEM_ALLOCATION);
		i += free_one_res(ctx, RESMAN_TYPE_DEVICEMEM_ALLOCATION);
		i += free_one_res(ctx, RESMAN_TYPE_DEVICEMEM_CONTEXT);

		if (i && warn_unfreed_res())
			pr_warning("pvr: %s: cleaning up %d "
				   "unfreed resources\n",
				   current->comm, i);
	}

	PVR_ASSERT(ctx->psResItemList == NULL);

	*(ctx->ppsThis) = ctx->psNext;
	if (ctx->psNext)
		ctx->psNext->ppsThis = ctx->ppsThis;

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct RESMAN_CONTEXT),
		  ctx, NULL);

	VALIDATERESLIST();

	PRINT_RESLIST(gpsResList, ctx, IMG_FALSE);

	RELEASE_SYNC_OBJ;
}

struct RESMAN_ITEM *ResManRegisterRes(struct RESMAN_CONTEXT *psResManContext,
				      u32 ui32ResType, void *pvParam,
				      u32 ui32Param,
				      enum PVRSRV_ERROR (*pfnFreeResource)
						(void *pvParam, u32 ui32Param))
{
	struct RESMAN_ITEM *psNewResItem;

	PVR_ASSERT(psResManContext != NULL);
	PVR_ASSERT(ui32ResType != 0);

	if (psResManContext == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "ResManRegisterRes: "
					"invalid parameter - psResManContext");
		return (struct RESMAN_ITEM *)NULL;
	}

	ACQUIRE_SYNC_OBJ;

	VALIDATERESLIST();

	PVR_DPF(PVR_DBG_MESSAGE, "ResManRegisterRes: register resource "
		 "Context 0x%x, ResType 0x%x, pvParam 0x%x, ui32Param 0x%x, "
		 "FreeFunc %08X",
		 psResManContext, ui32ResType, (u32) pvParam,
		 ui32Param, pfnFreeResource);

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(struct RESMAN_ITEM), (void **) &psNewResItem,
		       NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "ResManRegisterRes: "
			 "ERROR allocating new resource item");

		RELEASE_SYNC_OBJ;

		return (struct RESMAN_ITEM *)NULL;
	}

#ifdef DEBUG
	psNewResItem->ui32Signature = RESMAN_SIGNATURE;
#endif
	psNewResItem->ui32ResType = ui32ResType;
	psNewResItem->pvParam = pvParam;
	psNewResItem->ui32Param = ui32Param;
	psNewResItem->pfnFreeResource = pfnFreeResource;
	psNewResItem->ui32Flags = 0;

	psNewResItem->ppsThis = &psResManContext->psResItemList;
	psNewResItem->psNext = psResManContext->psResItemList;
	psResManContext->psResItemList = psNewResItem;
	if (psNewResItem->psNext)
		psNewResItem->psNext->ppsThis = &psNewResItem->psNext;

	VALIDATERESLIST();

	RELEASE_SYNC_OBJ;

	return psNewResItem;
}

void ResManFreeResByPtr(struct RESMAN_ITEM *psResItem)
{
	BUG_ON(!psResItem);

	PVR_DPF(PVR_DBG_MESSAGE,
		 "ResManFreeResByPtr: freeing resource at %08X", psResItem);

	ACQUIRE_SYNC_OBJ;

	VALIDATERESLIST();

	FreeResourceByPtr(psResItem, IMG_TRUE);

	VALIDATERESLIST();

	RELEASE_SYNC_OBJ;
}

void ResManFreeResByCriteria(struct RESMAN_CONTEXT *psResManContext,
				u32 ui32SearchCriteria, u32 ui32ResType,
				void *pvParam, u32 ui32Param)
{
	PVR_ASSERT(psResManContext != NULL);

	ACQUIRE_SYNC_OBJ;

	VALIDATERESLIST();

	PVR_DPF(PVR_DBG_MESSAGE, "ResManFreeResByCriteria: "
		"Context 0x%x, Criteria 0x%x, Type 0x%x, Addr 0x%x, Param 0x%x",
		 psResManContext, ui32SearchCriteria, ui32ResType,
		 (u32) pvParam, ui32Param);

	(void)FreeResourceByCriteria(psResManContext, ui32SearchCriteria,
					ui32ResType, pvParam, ui32Param,
					IMG_TRUE);

	VALIDATERESLIST();

	RELEASE_SYNC_OBJ;
}

enum PVRSRV_ERROR ResManDissociateRes(struct RESMAN_ITEM *psResItem,
			     struct RESMAN_CONTEXT *psNewResManContext)
{
	PVR_ASSERT(psResItem != NULL);

	if (psResItem == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "ResManDissociateRes: invalid parameter - psResItem");
		PVR_DBG_BREAK;
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
#ifdef DEBUG
	PVR_ASSERT(psResItem->ui32Signature == RESMAN_SIGNATURE);
#endif

	if (psNewResManContext != NULL) {
		if (psResItem->psNext)
			psResItem->psNext->ppsThis = psResItem->ppsThis;
		*psResItem->ppsThis = psResItem->psNext;

		psResItem->ppsThis = &psNewResManContext->psResItemList;
		psResItem->psNext = psNewResManContext->psResItemList;
		psNewResManContext->psResItemList = psResItem;
		if (psResItem->psNext)
			psResItem->psNext->ppsThis = &psResItem->psNext;
	} else {
		FreeResourceByPtr(psResItem, IMG_FALSE);
	}

	return PVRSRV_OK;
}

enum PVRSRV_ERROR ResManFindResourceByPtr(
					struct RESMAN_CONTEXT *psResManContext,
					struct RESMAN_ITEM *psItem)
{
	struct RESMAN_ITEM *psCurItem;

	PVR_ASSERT(psResManContext != NULL);
	PVR_ASSERT(psItem != NULL);

	if ((psItem == NULL) || (psResManContext == NULL)) {
		PVR_DPF(PVR_DBG_ERROR,
			 "ResManFindResourceByPtr: invalid parameter");
		PVR_DBG_BREAK;
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
#ifdef DEBUG
	PVR_ASSERT(psItem->ui32Signature == RESMAN_SIGNATURE);
#endif

	ACQUIRE_SYNC_OBJ;

	PVR_DPF(PVR_DBG_MESSAGE,
		 "FindResourceByPtr: psItem=%08X, psItem->psNext=%08X",
		 psItem, psItem->psNext);

	PVR_DPF(PVR_DBG_MESSAGE,
		 "FindResourceByPtr: Resource Ctx 0x%x, Type 0x%x, Addr 0x%x, "
		 "Param 0x%x, FnCall %08X, Flags 0x%x",
		 psResManContext,
		 psItem->ui32ResType, (u32) psItem->pvParam,
		 psItem->ui32Param, psItem->pfnFreeResource,
		 psItem->ui32Flags);

	psCurItem = psResManContext->psResItemList;

	while (psCurItem != NULL) {
		if (psCurItem != psItem) {
			psCurItem = psCurItem->psNext;
		} else {
			RELEASE_SYNC_OBJ;
			return PVRSRV_OK;
		}
	}

	RELEASE_SYNC_OBJ;

	return PVRSRV_ERROR_NOT_OWNER;
}

static void FreeResourceByPtr(struct RESMAN_ITEM *psItem,
				      IMG_BOOL bExecuteCallback)
{
	PVR_ASSERT(psItem->ui32Signature == RESMAN_SIGNATURE);

	PVR_DPF(PVR_DBG_MESSAGE,
		 "FreeResourceByPtr: psItem=%08X, psItem->psNext=%08X",
		 psItem, psItem->psNext);

	PVR_DPF(PVR_DBG_MESSAGE,
		 "FreeResourceByPtr: Type 0x%x, Addr 0x%x, "
		 "Param 0x%x, FnCall %08X, Flags 0x%x",
		 psItem->ui32ResType, (u32) psItem->pvParam,
		 psItem->ui32Param, psItem->pfnFreeResource,
		 psItem->ui32Flags);

	if (psItem->psNext)
		psItem->psNext->ppsThis = psItem->ppsThis;
	*psItem->ppsThis = psItem->psNext;

	RELEASE_SYNC_OBJ;

	if (bExecuteCallback &&
	    psItem->pfnFreeResource(psItem->pvParam, psItem->ui32Param) !=
			PVRSRV_OK)
		PVR_DPF(PVR_DBG_ERROR, "FreeResourceByPtr: "
					"ERROR calling FreeResource function");

	ACQUIRE_SYNC_OBJ;

	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct RESMAN_ITEM), psItem,
		  NULL);
}

static int FreeResourceByCriteria(struct RESMAN_CONTEXT *psResManContext,
				u32 ui32SearchCriteria, u32 ui32ResType,
				void *pvParam, u32 ui32Param,
				IMG_BOOL bExecuteCallback)
{
	struct RESMAN_ITEM *psCurItem;
	bool bMatch;
	int freed = 0;

	psCurItem = psResManContext->psResItemList;

	while (psCurItem != NULL) {
		bMatch = IMG_TRUE;

		if (((ui32SearchCriteria & RESMAN_CRITERIA_RESTYPE) != 0UL) &&
		    (psCurItem->ui32ResType != ui32ResType))
			bMatch = IMG_FALSE;
		else if (((ui32SearchCriteria & RESMAN_CRITERIA_PVOID_PARAM) !=
			  0UL) && (psCurItem->pvParam != pvParam))
			bMatch = IMG_FALSE;
		else if (((ui32SearchCriteria & RESMAN_CRITERIA_UI32_PARAM) !=
			  0UL) && (psCurItem->ui32Param != ui32Param))
			bMatch = IMG_FALSE;

		if (!bMatch) {
			psCurItem = psCurItem->psNext;
		} else {
			FreeResourceByPtr(psCurItem, bExecuteCallback);
			psCurItem = psResManContext->psResItemList;
			freed++;
		}
	}

	return freed;
}

#ifdef DEBUG
static void ValidateResList(struct RESMAN_LIST *psResList)
{
	struct RESMAN_ITEM *psCurItem, **ppsThisItem;
	struct RESMAN_CONTEXT *psCurContext, **ppsThisContext;

	if (psResList == NULL) {
		PVR_DPF(PVR_DBG_MESSAGE,
			 "ValidateResList: resman not initialised yet");
		return;
	}

	psCurContext = psResList->psContextList;
	ppsThisContext = &psResList->psContextList;

	while (psCurContext != NULL) {
		PVR_ASSERT(psCurContext->ui32Signature == RESMAN_SIGNATURE);
		if (psCurContext->ppsThis != ppsThisContext) {
			PVR_DPF(PVR_DBG_WARNING, "psCC=%08X "
			     "psCC->ppsThis=%08X psCC->psNext=%08X ppsTC=%08X",
				 psCurContext, psCurContext->ppsThis,
				 psCurContext->psNext, ppsThisContext);
			PVR_ASSERT(psCurContext->ppsThis == ppsThisContext);
		}

		psCurItem = psCurContext->psResItemList;
		ppsThisItem = &psCurContext->psResItemList;
		while (psCurItem != NULL) {
			PVR_ASSERT(psCurItem->ui32Signature ==
				   RESMAN_SIGNATURE);
			if (psCurItem->ppsThis != ppsThisItem) {
				PVR_DPF(PVR_DBG_WARNING, "psCurItem=%08X "
					"psCurItem->ppsThis=%08X "
					"psCurItem->psNext=%08X "
					"ppsThisItem=%08X",
					 psCurItem, psCurItem->ppsThis,
					 psCurItem->psNext, ppsThisItem);
				PVR_ASSERT(psCurItem->ppsThis == ppsThisItem);
			}

			ppsThisItem = &psCurItem->psNext;
			psCurItem = psCurItem->psNext;
		}

		ppsThisContext = &psCurContext->psNext;
		psCurContext = psCurContext->psNext;
	}
}
#endif
