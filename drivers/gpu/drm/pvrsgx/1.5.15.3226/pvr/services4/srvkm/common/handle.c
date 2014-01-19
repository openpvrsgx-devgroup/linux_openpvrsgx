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

#ifdef	PVR_SECURE_HANDLES
#include <stddef.h>

#include "services_headers.h"
#include "handle.h"

#ifdef	DEBUG
#define	HANDLE_BLOCK_SIZE	1
#else
#define	HANDLE_BLOCK_SIZE	256
#endif

#define	HANDLE_HASH_TAB_INIT_SIZE	32

#define	DEFAULT_MAX_INDEX_PLUS_ONE	0xfffffffful
#define	DEFAULT_MAX_HANDLE		DEFAULT_MAX_INDEX_PLUS_ONE

#define	INDEX_IS_VALID(psBase, i) ((i) < (psBase)->ui32TotalHandCount)

#define	INDEX_TO_HANDLE(psBase, idx) ((IMG_HANDLE)((idx) + 1))
#define	HANDLE_TO_INDEX(psBase, hand) ((IMG_UINT32)(hand) - 1)

#define INDEX_TO_HANDLE_PTR(psBase, i) (((psBase)->psHandleArray) + (i))
#define	HANDLE_TO_HANDLE_PTR(psBase, h) (INDEX_TO_HANDLE_PTR(psBase, HANDLE_TO_INDEX(psBase, h)))

#define	HANDLE_PTR_TO_INDEX(psBase, psHandle) (IMG_UINT32)((psHandle) - ((psBase)->psHandleArray))
#define	HANDLE_PTR_TO_HANDLE(psBase, psHandle) \
	INDEX_TO_HANDLE(psBase, HANDLE_PTR_TO_INDEX(psBase, psHandle))

#define	ROUND_UP_TO_MULTIPLE(a, b) ((((a) + (b) - 1) / (b)) * (b))

#define	HANDLES_BATCHED(psBase) ((psBase)->ui32HandBatchSize != 0)

#define	SET_FLAG(v, f) ((IMG_VOID)((v) |= (f)))
#define	CLEAR_FLAG(v, f) ((IMG_VOID)((v) &= ~(f)))
#define	TEST_FLAG(v, f) ((IMG_BOOL)(((v) & (f)) != 0))

#define	TEST_ALLOC_FLAG(psHandle, f) TEST_FLAG((psHandle)->eFlag, f)

#define	SET_INTERNAL_FLAG(psHandle, f) SET_FLAG((psHandle)->eInternalFlag, f)
#define	CLEAR_INTERNAL_FLAG(psHandle, f) CLEAR_FLAG((psHandle)->eInternalFlag, f)
#define	TEST_INTERNAL_FLAG(psHandle, f) TEST_FLAG((psHandle)->eInternalFlag, f)

#define	BATCHED_HANDLE(psHandle) TEST_INTERNAL_FLAG(psHandle, INTERNAL_HANDLE_FLAG_BATCHED)

#define	SET_BATCHED_HANDLE(psHandle) SET_INTERNAL_FLAG(psHandle, INTERNAL_HANDLE_FLAG_BATCHED)

#define	SET_UNBATCHED_HANDLE(psHandle) CLEAR_INTERNAL_FLAG(psHandle, INTERNAL_HANDLE_FLAG_BATCHED)

#define	BATCHED_HANDLE_PARTIALLY_FREE(psHandle) TEST_INTERNAL_FLAG(psHandle, INTERNAL_HANDLE_FLAG_BATCHED_PARTIALLY_FREE)

#define SET_BATCHED_HANDLE_PARTIALLY_FREE(psHandle) SET_INTERNAL_FLAG(psHandle, INTERNAL_HANDLE_FLAG_BATCHED_PARTIALLY_FREE)

#define	HANDLE_STRUCT_IS_FREE(psHandle) ((psHandle)->eType == PVRSRV_HANDLE_TYPE_NONE && (psHandle)->eInternalFlag == INTERNAL_HANDLE_FLAG_NONE)

#ifdef	MIN
#undef MIN
#endif

#define	MIN(x, y) (((x) < (y)) ? (x) : (y))

struct sHandleList
{
	IMG_UINT32 ui32Prev;
	IMG_UINT32 ui32Next;
	IMG_HANDLE hParent;
};

enum ePVRSRVInternalHandleFlag
{
	INTERNAL_HANDLE_FLAG_NONE = 0x00,
	INTERNAL_HANDLE_FLAG_BATCHED = 0x01,
	INTERNAL_HANDLE_FLAG_BATCHED_PARTIALLY_FREE = 0x02,
};

struct sHandle
{

	PVRSRV_HANDLE_TYPE eType;


	IMG_VOID *pvData;


	IMG_UINT32 ui32NextIndexPlusOne;


	enum ePVRSRVInternalHandleFlag eInternalFlag;


	PVRSRV_HANDLE_ALLOC_FLAG eFlag;


	IMG_UINT32 ui32Index;


	struct sHandleList sChildren;


	struct sHandleList sSiblings;
};

struct _PVRSRV_HANDLE_BASE_
{

	IMG_HANDLE hBaseBlockAlloc;


	IMG_HANDLE hHandBlockAlloc;


	struct sHandle *psHandleArray;


	HASH_TABLE *psHashTab;


	IMG_UINT32 ui32FreeHandCount;


	IMG_UINT32 ui32FirstFreeIndex;


	IMG_UINT32 ui32MaxIndexPlusOne;


	IMG_UINT32 ui32TotalHandCount;


	IMG_UINT32 ui32LastFreeIndexPlusOne;


	IMG_UINT32 ui32HandBatchSize;


	IMG_UINT32 ui32TotalHandCountPreBatch;


	IMG_UINT32 ui32FirstBatchIndexPlusOne;


	IMG_UINT32 ui32BatchHandAllocFailures;


	IMG_BOOL bPurgingEnabled;
};

enum eHandKey {
	HAND_KEY_DATA = 0,
	HAND_KEY_TYPE,
	HAND_KEY_PARENT,
	HAND_KEY_LEN
};

PVRSRV_HANDLE_BASE *gpsKernelHandleBase = IMG_NULL;

typedef IMG_UINTPTR_T HAND_KEY[HAND_KEY_LEN];

#ifdef INLINE_IS_PRAGMA
#pragma inline(HandleListInit)
#endif
static INLINE
IMG_VOID HandleListInit(IMG_UINT32 ui32Index, struct sHandleList *psList, IMG_HANDLE hParent)
{
	psList->ui32Next = ui32Index;
	psList->ui32Prev = ui32Index;
	psList->hParent = hParent;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(InitParentList)
#endif
static INLINE
IMG_VOID InitParentList(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psHandle)
{
	IMG_UINT32 ui32Parent = HANDLE_PTR_TO_INDEX(psBase, psHandle);

	HandleListInit(ui32Parent, &psHandle->sChildren, INDEX_TO_HANDLE(psBase, ui32Parent));
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(InitChildEntry)
#endif
static INLINE
IMG_VOID InitChildEntry(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psHandle)
{
	HandleListInit(HANDLE_PTR_TO_INDEX(psBase, psHandle), &psHandle->sSiblings, IMG_NULL);
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(HandleListIsEmpty)
#endif
static INLINE
IMG_BOOL HandleListIsEmpty(IMG_UINT32 ui32Index, struct sHandleList *psList)
{
	IMG_BOOL bIsEmpty;

	bIsEmpty = (IMG_BOOL)(psList->ui32Next == ui32Index);

#ifdef	DEBUG
	{
		IMG_BOOL bIsEmpty2;

		bIsEmpty2 = (IMG_BOOL)(psList->ui32Prev == ui32Index);
		PVR_ASSERT(bIsEmpty == bIsEmpty2);
	}
#endif

	return bIsEmpty;
}

#ifdef DEBUG
#ifdef INLINE_IS_PRAGMA
#pragma inline(NoChildren)
#endif
static INLINE
IMG_BOOL NoChildren(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psHandle)
{
	PVR_ASSERT(psHandle->sChildren.hParent == HANDLE_PTR_TO_HANDLE(psBase, psHandle));

	return HandleListIsEmpty(HANDLE_PTR_TO_INDEX(psBase, psHandle), &psHandle->sChildren);
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(NoParent)
#endif
static INLINE
IMG_BOOL NoParent(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psHandle)
{
	if (HandleListIsEmpty(HANDLE_PTR_TO_INDEX(psBase, psHandle), &psHandle->sSiblings))
	{
		PVR_ASSERT(psHandle->sSiblings.hParent == IMG_NULL);

		return IMG_TRUE;
	}
	else
	{
		PVR_ASSERT(psHandle->sSiblings.hParent != IMG_NULL);
	}
	return IMG_FALSE;
}
#endif
#ifdef INLINE_IS_PRAGMA
#pragma inline(ParentHandle)
#endif
static INLINE
IMG_HANDLE ParentHandle(struct sHandle *psHandle)
{
	return psHandle->sSiblings.hParent;
}

#define	LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, i, p, po, eo) \
		((struct sHandleList *)((IMG_CHAR *)(INDEX_TO_HANDLE_PTR(psBase, i)) + (((i) == (p)) ? (po) : (eo))))

#ifdef INLINE_IS_PRAGMA
#pragma inline(HandleListInsertBefore)
#endif
static INLINE
IMG_VOID HandleListInsertBefore(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32InsIndex, struct sHandleList *psIns, IMG_SIZE_T uiParentOffset, IMG_UINT32 ui32EntryIndex, struct sHandleList *psEntry, IMG_SIZE_T uiEntryOffset, IMG_UINT32 ui32ParentIndex)
{

	struct sHandleList *psPrevIns = LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, psIns->ui32Prev, ui32ParentIndex, uiParentOffset, uiEntryOffset);

	PVR_ASSERT(psEntry->hParent == IMG_NULL);
	PVR_ASSERT(ui32InsIndex == psPrevIns->ui32Next);
	PVR_ASSERT(LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, ui32ParentIndex, ui32ParentIndex, uiParentOffset, uiParentOffset)->hParent == INDEX_TO_HANDLE(psBase, ui32ParentIndex));

	psEntry->ui32Prev = psIns->ui32Prev;
	psIns->ui32Prev = ui32EntryIndex;
	psEntry->ui32Next = ui32InsIndex;
	psPrevIns->ui32Next = ui32EntryIndex;

	psEntry->hParent = INDEX_TO_HANDLE(psBase, ui32ParentIndex);
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(AdoptChild)
#endif
static INLINE
IMG_VOID AdoptChild(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psParent, struct sHandle *psChild)
{
	IMG_UINT32 ui32Parent = HANDLE_TO_INDEX(psBase, psParent->sChildren.hParent);

	PVR_ASSERT(ui32Parent == HANDLE_PTR_TO_INDEX(psBase, psParent));

	HandleListInsertBefore(psBase, ui32Parent, &psParent->sChildren, offsetof(struct sHandle, sChildren), HANDLE_PTR_TO_INDEX(psBase, psChild), &psChild->sSiblings, offsetof(struct sHandle, sSiblings), ui32Parent);

}

#ifdef INLINE_IS_PRAGMA
#pragma inline(HandleListRemove)
#endif
static INLINE
IMG_VOID HandleListRemove(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32EntryIndex, struct sHandleList *psEntry, IMG_SIZE_T uiEntryOffset, IMG_SIZE_T uiParentOffset)
{
	if (!HandleListIsEmpty(ui32EntryIndex, psEntry))
	{

		struct sHandleList *psPrev = LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, psEntry->ui32Prev, HANDLE_TO_INDEX(psBase, psEntry->hParent), uiParentOffset, uiEntryOffset);
		struct sHandleList *psNext = LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, psEntry->ui32Next, HANDLE_TO_INDEX(psBase, psEntry->hParent), uiParentOffset, uiEntryOffset);


		PVR_ASSERT(psEntry->hParent != IMG_NULL);

		psPrev->ui32Next = psEntry->ui32Next;
		psNext->ui32Prev = psEntry->ui32Prev;

		HandleListInit(ui32EntryIndex, psEntry, IMG_NULL);
	}
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(UnlinkFromParent)
#endif
static INLINE
IMG_VOID UnlinkFromParent(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psHandle)
{
	HandleListRemove(psBase, HANDLE_PTR_TO_INDEX(psBase, psHandle), &psHandle->sSiblings, offsetof(struct sHandle, sSiblings), offsetof(struct sHandle, sChildren));
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(HandleListIterate)
#endif
static INLINE
PVRSRV_ERROR HandleListIterate(PVRSRV_HANDLE_BASE *psBase, struct sHandleList *psHead, IMG_SIZE_T uiParentOffset, IMG_SIZE_T uiEntryOffset, PVRSRV_ERROR (*pfnIterFunc)(PVRSRV_HANDLE_BASE *, struct sHandle *))
{
	IMG_UINT32 ui32Index;
	IMG_UINT32 ui32Parent = HANDLE_TO_INDEX(psBase, psHead->hParent);

	PVR_ASSERT(psHead->hParent != IMG_NULL);


	for(ui32Index = psHead->ui32Next; ui32Index != ui32Parent; )
	{
		struct sHandle *psHandle = INDEX_TO_HANDLE_PTR(psBase, ui32Index);

		struct sHandleList *psEntry = LIST_PTR_FROM_INDEX_AND_OFFSET(psBase, ui32Index, ui32Parent, uiParentOffset, uiEntryOffset);
		PVRSRV_ERROR eError;

		PVR_ASSERT(psEntry->hParent == psHead->hParent);

		ui32Index = psEntry->ui32Next;

		eError = (*pfnIterFunc)(psBase, psHandle);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}
	}

	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(IterateOverChildren)
#endif
static INLINE
PVRSRV_ERROR IterateOverChildren(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psParent, PVRSRV_ERROR (*pfnIterFunc)(PVRSRV_HANDLE_BASE *, struct sHandle *))
{
	 return HandleListIterate(psBase, &psParent->sChildren, offsetof(struct sHandle, sChildren), offsetof(struct sHandle, sSiblings), pfnIterFunc);
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(GetHandleStructure)
#endif
static INLINE
PVRSRV_ERROR GetHandleStructure(PVRSRV_HANDLE_BASE *psBase, struct sHandle **ppsHandle, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType)
{
	IMG_UINT32 ui32Index = HANDLE_TO_INDEX(psBase, hHandle);
	struct sHandle *psHandle;


	if (!INDEX_IS_VALID(psBase, ui32Index))
	{
		PVR_DPF((PVR_DBG_ERROR, "GetHandleStructure: Handle index out of range (%u >= %u)", ui32Index, psBase->ui32TotalHandCount));
		return PVRSRV_ERROR_GENERIC;
	}

	psHandle =  INDEX_TO_HANDLE_PTR(psBase, ui32Index);
	if (psHandle->eType == PVRSRV_HANDLE_TYPE_NONE)
	{
		PVR_DPF((PVR_DBG_ERROR, "GetHandleStructure: Handle not allocated (index: %u)", ui32Index));
		return PVRSRV_ERROR_GENERIC;
	}


	if (eType != PVRSRV_HANDLE_TYPE_NONE && eType != psHandle->eType)
	{
		PVR_DPF((PVR_DBG_ERROR, "GetHandleStructure: Handle type mismatch (%d != %d)", eType, psHandle->eType));
		return PVRSRV_ERROR_GENERIC;
	}


	*ppsHandle = psHandle;

	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(ParentIfPrivate)
#endif
static INLINE
IMG_HANDLE ParentIfPrivate(struct sHandle *psHandle)
{
	return TEST_ALLOC_FLAG(psHandle, PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE) ?
			ParentHandle(psHandle) : IMG_NULL;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(InitKey)
#endif
static INLINE
IMG_VOID InitKey(HAND_KEY aKey, PVRSRV_HANDLE_BASE *psBase, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType, IMG_HANDLE hParent)
{
	PVR_UNREFERENCED_PARAMETER(psBase);

	aKey[HAND_KEY_DATA] = (IMG_UINTPTR_T)pvData;
	aKey[HAND_KEY_TYPE] = (IMG_UINTPTR_T)eType;
	aKey[HAND_KEY_PARENT] = (IMG_UINTPTR_T)hParent;
}

static PVRSRV_ERROR FreeHandleArray(PVRSRV_HANDLE_BASE *psBase)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (psBase->psHandleArray != IMG_NULL)
	{
		eError = OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
			psBase->ui32TotalHandCount * sizeof(struct sHandle),
			psBase->psHandleArray,
			psBase->hHandBlockAlloc);

		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "FreeHandleArray: Error freeing memory (%d)", eError));
		}
		else
		{
			psBase->psHandleArray = IMG_NULL;
		}
	}

	return eError;
}

static PVRSRV_ERROR FreeHandle(PVRSRV_HANDLE_BASE *psBase, struct sHandle *psHandle)
{
	HAND_KEY aKey;
	IMG_UINT32 ui32Index = HANDLE_PTR_TO_INDEX(psBase, psHandle);
	PVRSRV_ERROR eError;


	InitKey(aKey, psBase, psHandle->pvData, psHandle->eType, ParentIfPrivate(psHandle));

	if (!TEST_ALLOC_FLAG(psHandle, PVRSRV_HANDLE_ALLOC_FLAG_MULTI) && !BATCHED_HANDLE_PARTIALLY_FREE(psHandle))
	{
		IMG_HANDLE hHandle;
		hHandle = (IMG_HANDLE) HASH_Remove_Extended(psBase->psHashTab, aKey);

		PVR_ASSERT(hHandle != IMG_NULL);
		PVR_ASSERT(hHandle == INDEX_TO_HANDLE(psBase, ui32Index));
		PVR_UNREFERENCED_PARAMETER(hHandle);
	}


	UnlinkFromParent(psBase, psHandle);


	eError = IterateOverChildren(psBase, psHandle, FreeHandle);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FreeHandle: Error whilst freeing subhandles (%d)", eError));
		return eError;
	}


	psHandle->eType = PVRSRV_HANDLE_TYPE_NONE;

	if (BATCHED_HANDLE(psHandle) && !BATCHED_HANDLE_PARTIALLY_FREE(psHandle))
	{

		SET_BATCHED_HANDLE_PARTIALLY_FREE(psHandle);

		return PVRSRV_OK;
	}


	if (!psBase->bPurgingEnabled)
	{
		if (psBase->ui32FreeHandCount == 0)
		{
			PVR_ASSERT(psBase->ui32FirstFreeIndex == 0);
			PVR_ASSERT(psBase->ui32LastFreeIndexPlusOne == 0);

			psBase->ui32FirstFreeIndex =  ui32Index;
		}
		else
		{

			PVR_ASSERT(psBase->ui32LastFreeIndexPlusOne != 0);
			PVR_ASSERT(INDEX_TO_HANDLE_PTR(psBase, psBase->ui32LastFreeIndexPlusOne - 1)->ui32NextIndexPlusOne == 0);
			INDEX_TO_HANDLE_PTR(psBase, psBase->ui32LastFreeIndexPlusOne - 1)->ui32NextIndexPlusOne =  ui32Index + 1;
		}

		PVR_ASSERT(psHandle->ui32NextIndexPlusOne == 0);


		psBase->ui32LastFreeIndexPlusOne = ui32Index + 1;
	}

	psBase->ui32FreeHandCount++;

	return PVRSRV_OK;
}

static PVRSRV_ERROR FreeAllHandles(PVRSRV_HANDLE_BASE *psBase)
{
	IMG_UINT32 i;
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (psBase->ui32FreeHandCount == psBase->ui32TotalHandCount)
	{
		return eError;
	}

	for (i = 0; i < psBase->ui32TotalHandCount; i++)
	{
		struct sHandle *psHandle;

		psHandle = INDEX_TO_HANDLE_PTR(psBase, i);

		if (psHandle->eType != PVRSRV_HANDLE_TYPE_NONE)
		{
			eError = FreeHandle(psBase, psHandle);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "FreeAllHandles: FreeHandle failed (%d)", eError));
				break;
			}


			if (psBase->ui32FreeHandCount == psBase->ui32TotalHandCount)
			{
				break;
			}
		}
	}

	return eError;
}

static PVRSRV_ERROR FreeHandleBase(PVRSRV_HANDLE_BASE *psBase)
{
	PVRSRV_ERROR eError;

	if (HANDLES_BATCHED(psBase))
	{
		PVR_DPF((PVR_DBG_WARNING, "FreeHandleBase: Uncommitted/Unreleased handle batch"));
		PVRSRVReleaseHandleBatch(psBase);
	}


	eError = FreeAllHandles(psBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FreeHandleBase: Couldn't free handles (%d)", eError));
		return eError;
	}


	eError = FreeHandleArray(psBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FreeHandleBase: Couldn't free handle array (%d)", eError));
		return eError;
	}

	if (psBase->psHashTab != IMG_NULL)
	{

		HASH_Delete(psBase->psHashTab);
	}

	eError = OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
		sizeof(*psBase),
		psBase,
		psBase->hBaseBlockAlloc);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "FreeHandleBase: Couldn't free handle base (%d)", eError));
		return eError;
	}

	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(FindHandle)
#endif
static INLINE
IMG_HANDLE FindHandle(PVRSRV_HANDLE_BASE *psBase, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType, IMG_HANDLE hParent)
{
	HAND_KEY aKey;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	InitKey(aKey, psBase, pvData, eType, hParent);

	return (IMG_HANDLE) HASH_Retrieve_Extended(psBase->psHashTab, aKey);
}

static PVRSRV_ERROR ReallocMem(IMG_PVOID *ppvMem, IMG_HANDLE *phBlockAlloc, IMG_UINT32 ui32NewSize, IMG_UINT32 ui32OldSize)
{
	IMG_VOID *pvOldMem = *ppvMem;
	IMG_HANDLE hOldBlockAlloc = *phBlockAlloc;
	IMG_UINT32 ui32CopySize = MIN(ui32NewSize, ui32OldSize);
	IMG_VOID *pvNewMem = IMG_NULL;
	IMG_HANDLE hNewBlockAlloc = IMG_NULL;
	PVRSRV_ERROR eError;

	if (ui32NewSize == ui32OldSize)
	{
		return (PVRSRV_OK);
	}

	if (ui32NewSize != 0)
	{

		eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
			ui32NewSize,
			&pvNewMem,
			&hNewBlockAlloc,
			"Memory Area");
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "ReallocMem: Couldn't allocate new memory area (%d)", eError));
			return eError;
		}
	}

	if (ui32CopySize != 0)
	{

		OSMemCopy(pvNewMem, pvOldMem, ui32CopySize);
	}

	if (ui32OldSize != 0)
	{

		eError = OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP,
				ui32OldSize,
				pvOldMem,
				hOldBlockAlloc);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "ReallocMem: Couldn't free old memory area (%d)", eError));
		}
	}

	*ppvMem = pvNewMem;
	*phBlockAlloc = hNewBlockAlloc;

	return PVRSRV_OK;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(ReallocHandleArray)
#endif
static INLINE
PVRSRV_ERROR ReallocHandleArray(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32NewCount, IMG_UINT32 ui32OldCount)
{
	return ReallocMem((IMG_PVOID *)&psBase->psHandleArray,
				&psBase->hHandBlockAlloc,
				ui32NewCount * sizeof(struct sHandle),
				ui32OldCount * sizeof(struct sHandle));
}

static PVRSRV_ERROR IncreaseHandleArraySize(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32Delta)
{
	PVRSRV_ERROR eError;
	struct sHandle *psHandle;
	IMG_UINT32 ui32DeltaAdjusted = ROUND_UP_TO_MULTIPLE(ui32Delta, HANDLE_BLOCK_SIZE);
	IMG_UINT32 ui32NewTotalHandCount = psBase->ui32TotalHandCount + ui32DeltaAdjusted;
;

	PVR_ASSERT(ui32Delta != 0);


	if (ui32NewTotalHandCount > psBase->ui32MaxIndexPlusOne || ui32NewTotalHandCount <= psBase->ui32TotalHandCount)
	{
		ui32NewTotalHandCount = psBase->ui32MaxIndexPlusOne;

		ui32DeltaAdjusted = ui32NewTotalHandCount - psBase->ui32TotalHandCount;

		if (ui32DeltaAdjusted < ui32Delta)
		{
			PVR_DPF((PVR_DBG_ERROR, "IncreaseHandleArraySize: Maximum handle limit reached (%d)", psBase->ui32MaxIndexPlusOne));
			return PVRSRV_ERROR_OUT_OF_MEMORY;
		}
	}

	PVR_ASSERT(ui32DeltaAdjusted >= ui32Delta);


	eError = ReallocHandleArray(psBase, ui32NewTotalHandCount, psBase->ui32TotalHandCount);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "IncreaseHandleArraySize: ReallocHandleArray failed (%d)", eError));
		return eError;
	}


	for(psHandle = psBase->psHandleArray + psBase->ui32TotalHandCount;
		psHandle < psBase->psHandleArray + ui32NewTotalHandCount;
		psHandle++)
	{
		psHandle->eType = PVRSRV_HANDLE_TYPE_NONE;
		psHandle->eInternalFlag = INTERNAL_HANDLE_FLAG_NONE;
		psHandle->ui32NextIndexPlusOne  = 0;
	}


	psBase->ui32FreeHandCount += ui32DeltaAdjusted;

	if (psBase->ui32FirstFreeIndex == 0)
	{
		PVR_ASSERT(psBase->ui32LastFreeIndexPlusOne == 0);

		psBase->ui32FirstFreeIndex = psBase->ui32TotalHandCount;
	}
	else
	{
		if (!psBase->bPurgingEnabled)
		{
			PVR_ASSERT(psBase->ui32LastFreeIndexPlusOne != 0)
			PVR_ASSERT(INDEX_TO_HANDLE_PTR(psBase, psBase->ui32LastFreeIndexPlusOne - 1)->ui32NextIndexPlusOne == 0);

			INDEX_TO_HANDLE_PTR(psBase, psBase->ui32LastFreeIndexPlusOne - 1)->ui32NextIndexPlusOne = psBase->ui32TotalHandCount + 1;
		}
	}

	if (!psBase->bPurgingEnabled)
	{
		psBase->ui32LastFreeIndexPlusOne = ui32NewTotalHandCount;
	}

	psBase->ui32TotalHandCount = ui32NewTotalHandCount;

	return PVRSRV_OK;
}

static PVRSRV_ERROR EnsureFreeHandles(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32Free)
{
	PVRSRV_ERROR eError;

	if (ui32Free > psBase->ui32FreeHandCount)
	{
		IMG_UINT32 ui32FreeHandDelta = ui32Free - psBase->ui32FreeHandCount;
		eError = IncreaseHandleArraySize(psBase, ui32FreeHandDelta);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "EnsureFreeHandles: Couldn't allocate %u handles to ensure %u free handles (IncreaseHandleArraySize failed with error %d)", ui32FreeHandDelta, ui32Free, eError));

			return eError;
		}
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR AllocHandle(PVRSRV_HANDLE_BASE *psBase, IMG_HANDLE *phHandle, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType, PVRSRV_HANDLE_ALLOC_FLAG eFlag, IMG_HANDLE hParent)
{
	IMG_UINT32 ui32NewIndex;
	struct sHandle *psNewHandle = IMG_NULL;
	IMG_HANDLE hHandle;
	HAND_KEY aKey;
	PVRSRV_ERROR eError;


	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	PVR_ASSERT(psBase->psHashTab != IMG_NULL);

	if (!TEST_FLAG(eFlag, PVRSRV_HANDLE_ALLOC_FLAG_MULTI))
	{

		PVR_ASSERT(FindHandle(psBase, pvData, eType, hParent) == IMG_NULL);
	}

	if (psBase->ui32FreeHandCount == 0 && HANDLES_BATCHED(psBase))
	{
		 PVR_DPF((PVR_DBG_WARNING, "AllocHandle: Handle batch size (%u) was too small, allocating additional space", psBase->ui32HandBatchSize));
	}


	eError = EnsureFreeHandles(psBase, 1);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "AllocHandle: EnsureFreeHandles failed (%d)", eError));
		return eError;
	}
	PVR_ASSERT(psBase->ui32FreeHandCount != 0)

	if (!psBase->bPurgingEnabled)
	{

		ui32NewIndex = psBase->ui32FirstFreeIndex;


		psNewHandle = INDEX_TO_HANDLE_PTR(psBase, ui32NewIndex);
	}
	else
	{

		for(ui32NewIndex = psBase->ui32FirstFreeIndex; ui32NewIndex < psBase->ui32TotalHandCount; ui32NewIndex++)
		{
			psNewHandle = INDEX_TO_HANDLE_PTR(psBase, ui32NewIndex);
			if (HANDLE_STRUCT_IS_FREE(psNewHandle))
			{
				break;
			}

		}
		psBase->ui32FirstFreeIndex = 0;
		PVR_ASSERT(ui32NewIndex < psBase->ui32TotalHandCount);
	}
	PVR_ASSERT(psNewHandle != IMG_NULL);


	hHandle = INDEX_TO_HANDLE(psBase, ui32NewIndex);


	if (!TEST_FLAG(eFlag, PVRSRV_HANDLE_ALLOC_FLAG_MULTI))
	{

		InitKey(aKey, psBase, pvData, eType, hParent);


		if (!HASH_Insert_Extended(psBase->psHashTab, aKey, (IMG_UINTPTR_T)hHandle))
		{
			PVR_DPF((PVR_DBG_ERROR, "AllocHandle: Couldn't add handle to hash table"));

			return PVRSRV_ERROR_GENERIC;
		}
	}

	psBase->ui32FreeHandCount--;


	if (!psBase->bPurgingEnabled)
	{

		if (psBase->ui32FreeHandCount == 0)
		{
			PVR_ASSERT(psBase->ui32FirstFreeIndex == ui32NewIndex);
			PVR_ASSERT(psBase->ui32LastFreeIndexPlusOne == (ui32NewIndex + 1));

			psBase->ui32LastFreeIndexPlusOne = 0;
			psBase->ui32FirstFreeIndex = 0;
		}
		else
		{

			psBase->ui32FirstFreeIndex = (psNewHandle->ui32NextIndexPlusOne == 0) ?
				ui32NewIndex + 1 :
				psNewHandle->ui32NextIndexPlusOne - 1;
		}
	}


	psNewHandle->eType = eType;
	psNewHandle->pvData = pvData;
	psNewHandle->eInternalFlag = INTERNAL_HANDLE_FLAG_NONE;
	psNewHandle->eFlag = eFlag;
	psNewHandle->ui32Index = ui32NewIndex;

	InitParentList(psBase, psNewHandle);
#if defined(DEBUG)
	PVR_ASSERT(NoChildren(psBase, psNewHandle));
#endif

	InitChildEntry(psBase, psNewHandle);
#if defined(DEBUG)
	PVR_ASSERT(NoParent(psBase, psNewHandle));
#endif

	if (HANDLES_BATCHED(psBase))
	{

		psNewHandle->ui32NextIndexPlusOne = psBase->ui32FirstBatchIndexPlusOne;

		psBase->ui32FirstBatchIndexPlusOne = ui32NewIndex + 1;


		SET_BATCHED_HANDLE(psNewHandle);
	}
	else
	{
		psNewHandle->ui32NextIndexPlusOne = 0;
	}


	*phHandle = hHandle;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVAllocHandle(PVRSRV_HANDLE_BASE *psBase, IMG_HANDLE *phHandle, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType, PVRSRV_HANDLE_ALLOC_FLAG eFlag)
{
	IMG_HANDLE hHandle;
	PVRSRV_ERROR eError;

	*phHandle = IMG_NULL;

	if (HANDLES_BATCHED(psBase))
	{

		psBase->ui32BatchHandAllocFailures++;
	}


	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	if (!TEST_FLAG(eFlag, PVRSRV_HANDLE_ALLOC_FLAG_MULTI))
	{

		hHandle = FindHandle(psBase, pvData, eType, IMG_NULL);
		if (hHandle != IMG_NULL)
		{
			struct sHandle *psHandle;

			eError = GetHandleStructure(psBase, &psHandle, hHandle, eType);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocHandle: Lookup of existing handle failed"));
				return eError;
			}


			if (TEST_FLAG(psHandle->eFlag & eFlag, PVRSRV_HANDLE_ALLOC_FLAG_SHARED))
			{
				*phHandle = hHandle;
				eError = PVRSRV_OK;
				goto exit_ok;
			}
			return PVRSRV_ERROR_GENERIC;
		}
	}

	eError = AllocHandle(psBase, phHandle, pvData, eType, eFlag, IMG_NULL);

exit_ok:
	if (HANDLES_BATCHED(psBase) && (eError == PVRSRV_OK))
	{
		psBase->ui32BatchHandAllocFailures--;
	}

	return eError;
}

PVRSRV_ERROR PVRSRVAllocSubHandle(PVRSRV_HANDLE_BASE *psBase, IMG_HANDLE *phHandle, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType, PVRSRV_HANDLE_ALLOC_FLAG eFlag, IMG_HANDLE hParent)
{
	struct sHandle *psPHand;
	struct sHandle *psCHand;
	PVRSRV_ERROR eError;
	IMG_HANDLE hParentKey;
	IMG_HANDLE hHandle;

	*phHandle = IMG_NULL;

	if (HANDLES_BATCHED(psBase))
	{

		psBase->ui32BatchHandAllocFailures++;
	}


	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	hParentKey = TEST_FLAG(eFlag, PVRSRV_HANDLE_ALLOC_FLAG_PRIVATE) ?
			hParent : IMG_NULL;


	eError = GetHandleStructure(psBase, &psPHand, hParent, PVRSRV_HANDLE_TYPE_NONE);
	if (eError != PVRSRV_OK)
	{
		return PVRSRV_ERROR_GENERIC;
	}

	if (!TEST_FLAG(eFlag, PVRSRV_HANDLE_ALLOC_FLAG_MULTI))
	{

		hHandle = FindHandle(psBase, pvData, eType, hParentKey);
		if (hHandle != IMG_NULL)
		{
			struct sHandle *psCHandle;
			PVRSRV_ERROR eErr;

			eErr = GetHandleStructure(psBase, &psCHandle, hHandle, eType);
			if (eErr != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocSubHandle: Lookup of existing handle failed"));
				return eErr;
			}

			PVR_ASSERT(hParentKey != IMG_NULL && ParentHandle(HANDLE_TO_HANDLE_PTR(psBase, hHandle)) == hParent);


			if (TEST_FLAG(psCHandle->eFlag & eFlag, PVRSRV_HANDLE_ALLOC_FLAG_SHARED) && ParentHandle(HANDLE_TO_HANDLE_PTR(psBase, hHandle)) == hParent)
			{
				*phHandle = hHandle;
				goto exit_ok;
			}
			return PVRSRV_ERROR_GENERIC;
		}
	}

	eError = AllocHandle(psBase, &hHandle, pvData, eType, eFlag, hParentKey);
	if (eError != PVRSRV_OK)
	{
		return eError;
	}


	psPHand = HANDLE_TO_HANDLE_PTR(psBase, hParent);

	psCHand = HANDLE_TO_HANDLE_PTR(psBase, hHandle);

	AdoptChild(psBase, psPHand, psCHand);

	*phHandle = hHandle;

exit_ok:
	if (HANDLES_BATCHED(psBase))
	{
		psBase->ui32BatchHandAllocFailures--;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVFindHandle(PVRSRV_HANDLE_BASE *psBase, IMG_HANDLE *phHandle, IMG_VOID *pvData, PVRSRV_HANDLE_TYPE eType)
{
	IMG_HANDLE hHandle;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);


	hHandle = (IMG_HANDLE) FindHandle(psBase, pvData, eType, IMG_NULL);
	if (hHandle == IMG_NULL)
	{
		return PVRSRV_ERROR_GENERIC;
	}

	*phHandle = hHandle;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVLookupHandleAnyType(PVRSRV_HANDLE_BASE *psBase, IMG_PVOID *ppvData, PVRSRV_HANDLE_TYPE *peType, IMG_HANDLE hHandle)
{
	struct sHandle *psHandle;
	PVRSRV_ERROR eError;

	eError = GetHandleStructure(psBase, &psHandle, hHandle, PVRSRV_HANDLE_TYPE_NONE);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVLookupHandleAnyType: Error looking up handle (%d)", eError));
		return eError;
	}

	*ppvData = psHandle->pvData;
	*peType = psHandle->eType;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVLookupHandle(PVRSRV_HANDLE_BASE *psBase, IMG_PVOID *ppvData, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType)
{
	struct sHandle *psHandle;
	PVRSRV_ERROR eError;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	eError = GetHandleStructure(psBase, &psHandle, hHandle, eType);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVLookupHandle: Error looking up handle (%d)", eError));
		return eError;
	}

	*ppvData = psHandle->pvData;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVLookupSubHandle(PVRSRV_HANDLE_BASE *psBase, IMG_PVOID *ppvData, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType, IMG_HANDLE hAncestor)
{
	struct sHandle *psPHand;
	struct sHandle *psCHand;
	PVRSRV_ERROR eError;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	eError = GetHandleStructure(psBase, &psCHand, hHandle, eType);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVLookupSubHandle: Error looking up subhandle (%d)", eError));
		return eError;
	}


	for (psPHand = psCHand; ParentHandle(psPHand) != hAncestor; )
	{
		eError = GetHandleStructure(psBase, &psPHand, ParentHandle(psPHand), PVRSRV_HANDLE_TYPE_NONE);
		if (eError != PVRSRV_OK)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVLookupSubHandle: Subhandle doesn't belong to given ancestor"));
			return PVRSRV_ERROR_GENERIC;
		}
	}

	*ppvData = psCHand->pvData;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVGetParentHandle(PVRSRV_HANDLE_BASE *psBase, IMG_PVOID *phParent, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType)
{
	struct sHandle *psHandle;
	PVRSRV_ERROR eError;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	eError = GetHandleStructure(psBase, &psHandle, hHandle, eType);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVGetParentHandle: Error looking up subhandle (%d)", eError));
		return eError;
	}

	*phParent = ParentHandle(psHandle);

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVLookupAndReleaseHandle(PVRSRV_HANDLE_BASE *psBase, IMG_PVOID *ppvData, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType)
{
	struct sHandle *psHandle;
	PVRSRV_ERROR eError;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	eError = GetHandleStructure(psBase, &psHandle, hHandle, eType);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVLookupAndReleaseHandle: Error looking up handle (%d)", eError));
		return eError;
	}

	*ppvData = psHandle->pvData;

	eError = FreeHandle(psBase, psHandle);

	return eError;
}

PVRSRV_ERROR PVRSRVReleaseHandle(PVRSRV_HANDLE_BASE *psBase, IMG_HANDLE hHandle, PVRSRV_HANDLE_TYPE eType)
{
	struct sHandle *psHandle;
	PVRSRV_ERROR eError;

	PVR_ASSERT(eType != PVRSRV_HANDLE_TYPE_NONE);

	eError = GetHandleStructure(psBase, &psHandle, hHandle, eType);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVReleaseHandle: Error looking up handle (%d)", eError));
		return eError;
	}

	eError = FreeHandle(psBase, psHandle);

	return eError;
}

PVRSRV_ERROR PVRSRVNewHandleBatch(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32BatchSize)
{
	PVRSRV_ERROR eError;

	if (HANDLES_BATCHED(psBase))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVNewHandleBatch: There is a handle batch already in use (size %u)", psBase->ui32HandBatchSize));
		return  PVRSRV_ERROR_GENERIC;
	}

	if (ui32BatchSize == 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVNewHandleBatch: Invalid batch size (%u)", ui32BatchSize));
		return  PVRSRV_ERROR_INVALID_PARAMS;
	}

	eError = EnsureFreeHandles(psBase, ui32BatchSize);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVNewHandleBatch: EnsureFreeHandles failed (error %d)", eError));
		return eError;
	}

	psBase->ui32HandBatchSize = ui32BatchSize;


	psBase->ui32TotalHandCountPreBatch = psBase->ui32TotalHandCount;

	PVR_ASSERT(psBase->ui32BatchHandAllocFailures == 0);

	PVR_ASSERT(psBase->ui32FirstBatchIndexPlusOne == 0);

	PVR_ASSERT(HANDLES_BATCHED(psBase));

	return PVRSRV_OK;
}

static PVRSRV_ERROR PVRSRVHandleBatchCommitOrRelease(PVRSRV_HANDLE_BASE *psBase, IMG_BOOL bCommit)
{

	IMG_UINT32 ui32IndexPlusOne;
	IMG_BOOL bCommitBatch = bCommit;

	if (!HANDLES_BATCHED(psBase))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleBatchCommitOrRelease: There is no handle batch"));
		return PVRSRV_ERROR_INVALID_PARAMS;

	}

	if (psBase->ui32BatchHandAllocFailures != 0)
	{
		if (bCommit)
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleBatchCommitOrRelease: Attempting to commit batch with handle allocation failures."));
		}
		bCommitBatch = IMG_FALSE;
	}

	PVR_ASSERT(psBase->ui32BatchHandAllocFailures == 0 || !bCommit);

	ui32IndexPlusOne = psBase->ui32FirstBatchIndexPlusOne;
	while(ui32IndexPlusOne != 0)
	{
		struct sHandle *psHandle = INDEX_TO_HANDLE_PTR(psBase, ui32IndexPlusOne - 1);
		IMG_UINT32 ui32NextIndexPlusOne = psHandle->ui32NextIndexPlusOne;
		PVR_ASSERT(BATCHED_HANDLE(psHandle));

		psHandle->ui32NextIndexPlusOne = 0;

		if (!bCommitBatch || BATCHED_HANDLE_PARTIALLY_FREE(psHandle))
		{
			PVRSRV_ERROR eError;


			if (!BATCHED_HANDLE_PARTIALLY_FREE(psHandle))
			{
				SET_UNBATCHED_HANDLE(psHandle);
			}

			eError = FreeHandle(psBase, psHandle);
			if (eError != PVRSRV_OK)
			{
				 PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleBatchCommitOrRelease: Error freeing handle (%d)", eError));
			}
			PVR_ASSERT(eError == PVRSRV_OK);
		}
		else
		{
			SET_UNBATCHED_HANDLE(psHandle);
		}

		ui32IndexPlusOne = ui32NextIndexPlusOne;
	}

#ifdef DEBUG
	if (psBase->ui32TotalHandCountPreBatch != psBase->ui32TotalHandCount)
	{
		IMG_UINT32 ui32Delta = psBase->ui32TotalHandCount - psBase->ui32TotalHandCountPreBatch;

		PVR_ASSERT(psBase->ui32TotalHandCount > psBase->ui32TotalHandCountPreBatch);

		PVR_DPF((PVR_DBG_WARNING, "PVRSRVHandleBatchCommitOrRelease: The batch size was too small.  Batch size was %u, but needs to be %u", psBase->ui32HandBatchSize,  psBase->ui32HandBatchSize + ui32Delta));

	}
#endif

	psBase->ui32HandBatchSize = 0;
	psBase->ui32FirstBatchIndexPlusOne = 0;
	psBase->ui32TotalHandCountPreBatch = 0;
	psBase->ui32BatchHandAllocFailures = 0;

	if (psBase->ui32BatchHandAllocFailures != 0 && bCommit)
	{
		PVR_ASSERT(!bCommitBatch);

		return PVRSRV_ERROR_GENERIC;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVCommitHandleBatch(PVRSRV_HANDLE_BASE *psBase)
{
	return PVRSRVHandleBatchCommitOrRelease(psBase, IMG_TRUE);
}

IMG_VOID PVRSRVReleaseHandleBatch(PVRSRV_HANDLE_BASE *psBase)
{
	(IMG_VOID) PVRSRVHandleBatchCommitOrRelease(psBase, IMG_FALSE);
}

PVRSRV_ERROR PVRSRVSetMaxHandle(PVRSRV_HANDLE_BASE *psBase, IMG_UINT32 ui32MaxHandle)
{
	if (HANDLES_BATCHED(psBase))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetMaxHandle: Limit cannot be set whilst in batch mode"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}


	if (ui32MaxHandle == 0 || ui32MaxHandle >= DEFAULT_MAX_HANDLE)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetMaxHandle: Limit must be between %u and %u, inclusive", 0, DEFAULT_MAX_HANDLE));

		return PVRSRV_ERROR_INVALID_PARAMS;
	}


	if (psBase->ui32TotalHandCount != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVSetMaxHandle: Limit cannot be set becuase handles have already been allocated"));

		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psBase->ui32MaxIndexPlusOne = ui32MaxHandle;

	return PVRSRV_OK;
}

IMG_UINT32 PVRSRVGetMaxHandle(PVRSRV_HANDLE_BASE *psBase)
{
	return psBase->ui32MaxIndexPlusOne;
}

PVRSRV_ERROR PVRSRVEnableHandlePurging(PVRSRV_HANDLE_BASE *psBase)
{
	if (psBase->bPurgingEnabled)
	{
		PVR_DPF((PVR_DBG_WARNING, "PVRSRVEnableHandlePurging: Purging already enabled"));
		return PVRSRV_OK;
	}


	if (psBase->ui32TotalHandCount != 0)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVEnableHandlePurging: Handles have already been allocated"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psBase->bPurgingEnabled = IMG_TRUE;

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVPurgeHandles(PVRSRV_HANDLE_BASE *psBase)
{
	IMG_UINT32 ui32Handle;
	IMG_UINT32 ui32NewHandCount;

	if (!psBase->bPurgingEnabled)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPurgeHandles: Purging not enabled for this handle base"));
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	if (HANDLES_BATCHED(psBase))
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVPurgeHandles: Purging not allowed whilst in batch mode"));
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	for (ui32Handle = psBase->ui32TotalHandCount; ui32Handle != 0; ui32Handle--)
	{
		struct sHandle *psHandle = HANDLE_TO_HANDLE_PTR(psBase, ui32Handle);
		if (!HANDLE_STRUCT_IS_FREE(psHandle))
		{
			break;
		}
	}

	ui32NewHandCount = ROUND_UP_TO_MULTIPLE(ui32Handle, HANDLE_BLOCK_SIZE);


	if (ui32NewHandCount >= ui32Handle && ui32NewHandCount <= (psBase->ui32TotalHandCount/2))
	{
		IMG_UINT32 ui32Delta = psBase->ui32TotalHandCount - ui32NewHandCount;
		PVRSRV_ERROR eError;



		eError = ReallocHandleArray(psBase, ui32NewHandCount, psBase->ui32TotalHandCount);
		if (eError != PVRSRV_OK)
		{
			return eError;
		}


		psBase->ui32TotalHandCount = ui32NewHandCount;
		psBase->ui32FreeHandCount -= ui32Delta;
		psBase->ui32FirstFreeIndex = 0;
	}

	return PVRSRV_OK;
}

PVRSRV_ERROR PVRSRVAllocHandleBase(PVRSRV_HANDLE_BASE **ppsBase)
{
	PVRSRV_HANDLE_BASE *psBase;
	IMG_HANDLE hBlockAlloc;
	PVRSRV_ERROR eError;

	eError = OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		sizeof(*psBase),
		(IMG_PVOID *)&psBase,
		&hBlockAlloc,
		"Handle Base");
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocHandleBase: Couldn't allocate handle base (%d)", eError));
		return eError;
	}
	OSMemSet(psBase, 0, sizeof(*psBase));


	psBase->psHashTab = HASH_Create_Extended(HANDLE_HASH_TAB_INIT_SIZE, sizeof(HAND_KEY), HASH_Func_Default, HASH_Key_Comp_Default);
	if (psBase->psHashTab == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVAllocHandleBase: Couldn't create data pointer hash table\n"));
		goto failure;
	}

	psBase->hBaseBlockAlloc = hBlockAlloc;

	psBase->ui32MaxIndexPlusOne = DEFAULT_MAX_INDEX_PLUS_ONE;

	*ppsBase = psBase;

	return PVRSRV_OK;
failure:
	(IMG_VOID)PVRSRVFreeHandleBase(psBase);
	return PVRSRV_ERROR_GENERIC;
}

PVRSRV_ERROR PVRSRVFreeHandleBase(PVRSRV_HANDLE_BASE *psBase)
{
	PVRSRV_ERROR eError;

	PVR_ASSERT(psBase != gpsKernelHandleBase);

	eError = FreeHandleBase(psBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVFreeHandleBase: FreeHandleBase failed (%d)", eError));
	}

	return eError;
}

PVRSRV_ERROR PVRSRVHandleInit(IMG_VOID)
{
	PVRSRV_ERROR eError;

	PVR_ASSERT(gpsKernelHandleBase == IMG_NULL);

	eError = PVRSRVAllocHandleBase(&gpsKernelHandleBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleInit: PVRSRVAllocHandleBase failed (%d)", eError));
		goto error;
	}

	eError = PVRSRVEnableHandlePurging(gpsKernelHandleBase);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleInit: PVRSRVEnableHandlePurging failed (%d)", eError));
		goto error;
	}

	return PVRSRV_OK;
error:
	(IMG_VOID) PVRSRVHandleDeInit();
	return eError;
}

PVRSRV_ERROR PVRSRVHandleDeInit(IMG_VOID)
{
	PVRSRV_ERROR eError = PVRSRV_OK;

	if (gpsKernelHandleBase != IMG_NULL)
	{
		eError = FreeHandleBase(gpsKernelHandleBase);
		if (eError == PVRSRV_OK)
		{
			gpsKernelHandleBase = IMG_NULL;
		}
		else
		{
			PVR_DPF((PVR_DBG_ERROR, "PVRSRVHandleDeInit: FreeHandleBase failed (%d)", eError));
		}
	}

	return eError;
}
#else
#endif
