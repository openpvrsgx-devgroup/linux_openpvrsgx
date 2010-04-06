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

#include "services_headers.h"
#include "hash.h"
#include "ra.h"
#include "buffer_manager.h"
#include "osfunc.h"

#include <linux/kernel.h>
#include "proc.h"


#define MINIMUM_HASH_SIZE	64

struct BT {
	enum bt_type {
		btt_span,
		btt_free,
		btt_live
	} type;

	u32 base;
	size_t uSize;

	struct BT *pNextSegment;
	struct BT *pPrevSegment;

	struct BT *pNextFree;
	struct BT *pPrevFree;

	struct BM_MAPPING *psMapping;
};
struct BT;

struct RA_ARENA {
	char *name;
	u32 uQuantum;
	IMG_BOOL(*pImportAlloc)(void *, size_t uSize, size_t *pActualSize,
				struct BM_MAPPING **ppsMapping, u32 uFlags,
				u32 *pBase);
	void (*pImportFree)(void *, u32, struct BM_MAPPING *psMapping);
	void (*pBackingStoreFree)(void *, u32, u32, void *);
	void *pImportHandle;
#define FREE_TABLE_LIMIT	32
	struct BT *aHeadFree[FREE_TABLE_LIMIT];
	struct BT *pHeadSegment;
	struct BT *pTailSegment;
	struct HASH_TABLE *pSegmentHash;
#ifdef RA_STATS
	struct RA_STATISTICS sStatistics;
#endif
#if defined(CONFIG_PROC_FS) && defined(DEBUG)
#define PROC_NAME_SIZE		32
	char szProcInfoName[PROC_NAME_SIZE];
	char szProcSegsName[PROC_NAME_SIZE];
	IMG_BOOL bInitProcEntry;
#endif
};

#if defined(CONFIG_PROC_FS) && defined(DEBUG)
static int RA_DumpSegs(char *page, char **start, off_t off, int count, int *eof,
		       void *data);
static int RA_DumpInfo(char *page, char **start, off_t off, int count, int *eof,
		       void *data);
#endif

#if defined(CONFIG_PROC_FS) && defined(DEBUG)
static char *ReplaceSpaces(char *const pS)
{
	char *pT;

	for (pT = pS; *pT != 0; pT++)
		if (*pT == ' ' || *pT == '\t')
			*pT = '_';

	return pS;
}
#endif

static IMG_BOOL _RequestAllocFail(void *_h, size_t _uSize, size_t *_pActualSize,
				  struct BM_MAPPING **_ppsMapping,
				  u32 _uFlags, u32 *_pBase)
{
	PVR_UNREFERENCED_PARAMETER(_h);
	PVR_UNREFERENCED_PARAMETER(_uSize);
	PVR_UNREFERENCED_PARAMETER(_pActualSize);
	PVR_UNREFERENCED_PARAMETER(_ppsMapping);
	PVR_UNREFERENCED_PARAMETER(_uFlags);
	PVR_UNREFERENCED_PARAMETER(_pBase);

	return IMG_FALSE;
}

static u32 pvr_log2(size_t n)
{
	u32 l = 0;
	n >>= 1;
	while (n > 0) {
		n >>= 1;
		l++;
	}
	return l;
}

static enum PVRSRV_ERROR _SegmentListInsertAfter(struct RA_ARENA *pArena,
						 struct BT *pInsertionPoint,
						 struct BT *pBT)
{
	PVR_ASSERT(pArena != NULL);
	PVR_ASSERT(pInsertionPoint != NULL);

	if ((pInsertionPoint == NULL) || (pArena == NULL)) {
		PVR_DPF(PVR_DBG_ERROR,
			 "_SegmentListInsertAfter: invalid parameters");
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	pBT->pNextSegment = pInsertionPoint->pNextSegment;
	pBT->pPrevSegment = pInsertionPoint;
	if (pInsertionPoint->pNextSegment == NULL)
		pArena->pTailSegment = pBT;
	else
		pInsertionPoint->pNextSegment->pPrevSegment = pBT;
	pInsertionPoint->pNextSegment = pBT;

	return PVRSRV_OK;
}

static enum PVRSRV_ERROR _SegmentListInsert(struct RA_ARENA *pArena,
					    struct BT *pBT)
{
	enum PVRSRV_ERROR eError = PVRSRV_OK;

	if (pArena->pHeadSegment == NULL) {
		pArena->pHeadSegment = pArena->pTailSegment = pBT;
		pBT->pNextSegment = pBT->pPrevSegment = NULL;
	} else {
		struct BT *pBTScan;
		if (pBT->base < pArena->pHeadSegment->base) {
			pBT->pNextSegment = pArena->pHeadSegment;
			pArena->pHeadSegment->pPrevSegment = pBT;
			pArena->pHeadSegment = pBT;
			pBT->pPrevSegment = NULL;
		} else {
			pBTScan = pArena->pHeadSegment;

			while ((pBTScan->pNextSegment != NULL) &&
			       (pBT->base >= pBTScan->pNextSegment->base))
				pBTScan = pBTScan->pNextSegment;

			eError = _SegmentListInsertAfter(pArena, pBTScan, pBT);
			if (eError != PVRSRV_OK)
				return eError;
		}
	}
	return eError;
}

static void _SegmentListRemove(struct RA_ARENA *pArena, struct BT *pBT)
{
	if (pBT->pPrevSegment == NULL)
		pArena->pHeadSegment = pBT->pNextSegment;
	else
		pBT->pPrevSegment->pNextSegment = pBT->pNextSegment;

	if (pBT->pNextSegment == NULL)
		pArena->pTailSegment = pBT->pPrevSegment;
	else
		pBT->pNextSegment->pPrevSegment = pBT->pPrevSegment;
}

static struct BT *_SegmentSplit(struct RA_ARENA *pArena, struct BT *pBT,
				size_t uSize)
{
	struct BT *pNeighbour;

	PVR_ASSERT(pArena != NULL);

	if (pArena == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "_SegmentSplit: invalid parameter - pArena");
		return NULL;
	}

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(struct BT),
		       (void **) &pNeighbour, NULL) != PVRSRV_OK)
		return NULL;

	pNeighbour->pPrevSegment = pBT;
	pNeighbour->pNextSegment = pBT->pNextSegment;
	if (pBT->pNextSegment == NULL)
		pArena->pTailSegment = pNeighbour;
	else
		pBT->pNextSegment->pPrevSegment = pNeighbour;
	pBT->pNextSegment = pNeighbour;

	pNeighbour->type = btt_free;
	pNeighbour->uSize = pBT->uSize - uSize;
	pNeighbour->base = pBT->base + uSize;
	pNeighbour->psMapping = pBT->psMapping;
	pBT->uSize = uSize;
	return pNeighbour;
}

static void _FreeListInsert(struct RA_ARENA *pArena, struct BT *pBT)
{
	u32 uIndex;
	uIndex = pvr_log2(pBT->uSize);
	pBT->type = btt_free;
	pBT->pNextFree = pArena->aHeadFree[uIndex];
	pBT->pPrevFree = NULL;
	if (pArena->aHeadFree[uIndex] != NULL)
		pArena->aHeadFree[uIndex]->pPrevFree = pBT;
	pArena->aHeadFree[uIndex] = pBT;
}

static void _FreeListRemove(struct RA_ARENA *pArena, struct BT *pBT)
{
	u32 uIndex;
	uIndex = pvr_log2(pBT->uSize);
	if (pBT->pNextFree != NULL)
		pBT->pNextFree->pPrevFree = pBT->pPrevFree;
	if (pBT->pPrevFree == NULL)
		pArena->aHeadFree[uIndex] = pBT->pNextFree;
	else
		pBT->pPrevFree->pNextFree = pBT->pNextFree;
}

static struct BT *_BuildSpanMarker(u32 base, size_t uSize)
{
	struct BT *pBT;

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(struct BT),
		       (void **) &pBT, NULL) != PVRSRV_OK)
		return NULL;

	pBT->type = btt_span;
	pBT->base = base;
	pBT->uSize = uSize;
	pBT->psMapping = NULL;

	return pBT;
}

static struct BT *_BuildBT(u32 base, size_t uSize)
{
	struct BT *pBT;

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(struct BT),
		       (void **) &pBT, NULL) != PVRSRV_OK)
		return NULL;

	pBT->type = btt_free;
	pBT->base = base;
	pBT->uSize = uSize;

	return pBT;
}

static struct BT *_InsertResource(struct RA_ARENA *pArena, u32 base,
			   size_t uSize)
{
	struct BT *pBT;
	PVR_ASSERT(pArena != NULL);
	if (pArena == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "_InsertResource: invalid parameter - pArena");
		return NULL;
	}

	pBT = _BuildBT(base, uSize);
	if (pBT != NULL) {
		if (_SegmentListInsert(pArena, pBT) != PVRSRV_OK) {
			PVR_DPF(PVR_DBG_ERROR,
			"_InsertResource: call to _SegmentListInsert failed");
			return NULL;
		}
		_FreeListInsert(pArena, pBT);
#ifdef RA_STATS
		pArena->sStatistics.uTotalResourceCount += uSize;
		pArena->sStatistics.uFreeResourceCount += uSize;
		pArena->sStatistics.uSpanCount++;
#endif
	}
	return pBT;
}

static struct BT *_InsertResourceSpan(struct RA_ARENA *pArena, u32 base,
			       size_t uSize)
{
	enum PVRSRV_ERROR eError;
	struct BT *pSpanStart;
	struct BT *pSpanEnd;
	struct BT *pBT;

	PVR_ASSERT(pArena != NULL);
	if (pArena == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "_InsertResourceSpan: invalid parameter - pArena");
		return NULL;
	}

	PVR_DPF(PVR_DBG_MESSAGE,
		 "RA_InsertResourceSpan: arena='%s', base=0x%x, size=0x%x",
		 pArena->name, base, uSize);

	pSpanStart = _BuildSpanMarker(base, uSize);
	if (pSpanStart == NULL)
		goto fail_start;
	pSpanEnd = _BuildSpanMarker(base + uSize, 0);
	if (pSpanEnd == NULL)
		goto fail_end;

	pBT = _BuildBT(base, uSize);
	if (pBT == NULL)
		goto fail_bt;

	eError = _SegmentListInsert(pArena, pSpanStart);
	if (eError != PVRSRV_OK)
		goto fail_SegListInsert;

	eError = _SegmentListInsertAfter(pArena, pSpanStart, pBT);
	if (eError != PVRSRV_OK)
		goto fail_SegListInsert;

	_FreeListInsert(pArena, pBT);

	eError = _SegmentListInsertAfter(pArena, pBT, pSpanEnd);
	if (eError != PVRSRV_OK)
		goto fail_SegListInsert;

#ifdef RA_STATS
	pArena->sStatistics.uTotalResourceCount += uSize;
#endif
	return pBT;

fail_SegListInsert:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BT), pBT, NULL);
fail_bt:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BT), pSpanEnd, NULL);
fail_end:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BT), pSpanStart, NULL);
fail_start:
	return NULL;
}

static void _FreeBT(struct RA_ARENA *pArena, struct BT *pBT,
		    IMG_BOOL bFreeBackingStore)
{
	struct BT *pNeighbour;
	u32 uOrigBase;
	size_t uOrigSize;

	PVR_ASSERT(pArena != NULL);
	PVR_ASSERT(pBT != NULL);

	if ((pArena == NULL) || (pBT == NULL)) {
		PVR_DPF(PVR_DBG_ERROR, "_FreeBT: invalid parameter");
		return;
	}
#ifdef RA_STATS
	pArena->sStatistics.uLiveSegmentCount--;
	pArena->sStatistics.uFreeSegmentCount++;
	pArena->sStatistics.uFreeResourceCount += pBT->uSize;
#endif

	uOrigBase = pBT->base;
	uOrigSize = pBT->uSize;

	pNeighbour = pBT->pPrevSegment;
	if (pNeighbour != NULL && pNeighbour->type == btt_free &&
	    pNeighbour->base + pNeighbour->uSize == pBT->base) {
		_FreeListRemove(pArena, pNeighbour);
		_SegmentListRemove(pArena, pNeighbour);
		pBT->base = pNeighbour->base;
		pBT->uSize += pNeighbour->uSize;
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BT),
			  pNeighbour, NULL);
#ifdef RA_STATS
		pArena->sStatistics.uFreeSegmentCount--;
#endif
	}

	pNeighbour = pBT->pNextSegment;
	if (pNeighbour != NULL && pNeighbour->type == btt_free &&
	    pBT->base + pBT->uSize == pNeighbour->base) {
		_FreeListRemove(pArena, pNeighbour);
		_SegmentListRemove(pArena, pNeighbour);
		pBT->uSize += pNeighbour->uSize;
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BT),
			  pNeighbour, NULL);
#ifdef RA_STATS
		pArena->sStatistics.uFreeSegmentCount--;
#endif
	}

	if (pArena->pBackingStoreFree != NULL && bFreeBackingStore) {
		u32 uRoundedStart, uRoundedEnd;

		uRoundedStart = (uOrigBase / pArena->uQuantum) *
							pArena->uQuantum;

		if (uRoundedStart < pBT->base)
			uRoundedStart += pArena->uQuantum;

		uRoundedEnd = ((uOrigBase + uOrigSize + pArena->uQuantum -
		      1) / pArena->uQuantum) * pArena->uQuantum;

		if (uRoundedEnd > (pBT->base + pBT->uSize))
			uRoundedEnd -= pArena->uQuantum;

		if (uRoundedStart < uRoundedEnd)
			pArena->pBackingStoreFree(pArena->pImportHandle,
						  uRoundedStart, uRoundedEnd,
						  (void *) 0);
	}

	if (pBT->pNextSegment != NULL && pBT->pNextSegment->type == btt_span &&
	    pBT->pPrevSegment != NULL && pBT->pPrevSegment->type == btt_span) {
		struct BT *next = pBT->pNextSegment;
		struct BT *prev = pBT->pPrevSegment;
		_SegmentListRemove(pArena, next);
		_SegmentListRemove(pArena, prev);
		_SegmentListRemove(pArena, pBT);
		pArena->pImportFree(pArena->pImportHandle, pBT->base,
				    pBT->psMapping);
#ifdef RA_STATS
		pArena->sStatistics.uSpanCount--;
		pArena->sStatistics.uExportCount++;
		pArena->sStatistics.uFreeSegmentCount--;
		pArena->sStatistics.uFreeResourceCount -= pBT->uSize;
		pArena->sStatistics.uTotalResourceCount -= pBT->uSize;
#endif
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BT), next,
			  NULL);
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BT), prev,
			  NULL);
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BT), pBT,
			  NULL);
	} else
		_FreeListInsert(pArena, pBT);
}

static int alloc_from_bt(struct RA_ARENA *arena, struct BT *bt, u32 start,
			 size_t size, u32 align,
			 struct BM_MAPPING **new_mapping, u32 *new_base)
{
	_FreeListRemove(arena, bt);
	PVR_ASSERT(bt->type == btt_free);
#ifdef RA_STATS
	arena->sStatistics.uLiveSegmentCount++;
	arena->sStatistics.uFreeSegmentCount--;
	arena->sStatistics.uFreeResourceCount -= bt->uSize;
#endif
	if (start > bt->base) {
		struct BT *next_bt;

		next_bt = _SegmentSplit(arena, bt, start - bt->base);

		if (!next_bt) {
			PVR_DPF(PVR_DBG_ERROR, "_AttemptAllocAligned: "
					"Front split failed");

			_FreeListInsert(arena, bt);
			return -1;
		}

		_FreeListInsert(arena, bt);
#ifdef RA_STATS
		arena->sStatistics.uFreeSegmentCount++;
		arena->sStatistics.uFreeResourceCount += bt->uSize;
#endif
		bt = next_bt;
	}

	if (bt->uSize > size) {
		struct BT *next_bt;
		next_bt = _SegmentSplit(arena, bt, size);

		if (!next_bt) {
			PVR_DPF(PVR_DBG_ERROR, "_AttemptAllocAligned: "
					"Back split failed");

			_FreeListInsert(arena, bt);
			return -1;
		}

		_FreeListInsert(arena, next_bt);
#ifdef RA_STATS
		arena->sStatistics.uFreeSegmentCount++;
		arena->sStatistics.uFreeResourceCount += next_bt->uSize;
#endif
	}

	bt->type = btt_live;

	if (!HASH_Insert(arena->pSegmentHash, bt->base, (u32)bt)) {
		_FreeBT(arena, bt, IMG_FALSE);
		return -1;
	}

	if (new_mapping)
		*new_mapping = bt->psMapping;

	*new_base = bt->base;

	return 0;
}

static IMG_BOOL _AttemptAllocAligned(struct RA_ARENA *pArena, size_t uSize,
		     struct BM_MAPPING **ppsMapping, u32 uFlags, u32 uAlignment,
		     u32 *base)
{
	u32 uIndex;
	PVR_ASSERT(pArena != NULL);
	if (pArena == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "_AttemptAllocAligned: invalid parameter - pArena");
		return IMG_FALSE;
	}

	uIndex = pvr_log2(uSize);

	while (uIndex < FREE_TABLE_LIMIT && pArena->aHeadFree[uIndex] == NULL)
		uIndex++;

	for (; uIndex < FREE_TABLE_LIMIT; uIndex++) {
		struct BT *pBT;

		pBT = pArena->aHeadFree[uIndex];
		if (!pBT)
			continue;

		for (; pBT != NULL; pBT = pBT->pNextFree) {
			u32 aligned_base;

			if (uAlignment > 1)
				aligned_base = (pBT->base + uAlignment -
					  1) / uAlignment * uAlignment;
			else
				aligned_base = pBT->base;
			PVR_DPF(PVR_DBG_MESSAGE,
			   "RA_AttemptAllocAligned: pBT-base=0x%x "
			   "pBT-size=0x%x alignedbase=0x%x size=0x%x",
			   pBT->base, pBT->uSize, aligned_base, uSize);

			if (pBT->base + pBT->uSize < aligned_base + uSize)
				continue;

			if (pBT->psMapping && pBT->psMapping->ui32Flags !=
			    uFlags) {
				PVR_DPF(PVR_DBG_MESSAGE,
					"AttemptAllocAligned: mismatch in "
					"flags. Import has %x, request was %x",
					 pBT->psMapping->ui32Flags, uFlags);
				continue;
			}

			if (alloc_from_bt(pArena, pBT, aligned_base, uSize,
					  uFlags, ppsMapping, base) < 0)
				return IMG_FALSE;

			return IMG_TRUE;
		}
	}

	return IMG_FALSE;
}

struct RA_ARENA *RA_Create(char *name, u32 base, size_t uSize,
			   struct BM_MAPPING *psMapping, size_t uQuantum,
			   IMG_BOOL(*imp_alloc) (void *, size_t uSize,
						 size_t *pActualSize,
						 struct BM_MAPPING **ppsMapping,
						 u32 _flags, u32 *pBase),
			   void (*imp_free) (void *, u32, struct BM_MAPPING *),
			   void(*backingstore_free) (void *, u32, u32, void *),
			   void *pImportHandle)
{
	struct RA_ARENA *pArena;
	struct BT *pBT;
	int i;

	PVR_DPF(PVR_DBG_MESSAGE, "RA_Create: "
		 "name='%s', base=0x%x, uSize=0x%x, alloc=0x%x, free=0x%x",
		 name, base, uSize, imp_alloc, imp_free);

	if (OSAllocMem(PVRSRV_OS_PAGEABLE_HEAP,
		       sizeof(*pArena),
		       (void **) &pArena, NULL) != PVRSRV_OK)
		goto arena_fail;

	pArena->name = name;
	pArena->pImportAlloc =
	    (imp_alloc != NULL) ? imp_alloc : _RequestAllocFail;
	pArena->pImportFree = imp_free;
	pArena->pBackingStoreFree = backingstore_free;
	pArena->pImportHandle = pImportHandle;
	for (i = 0; i < FREE_TABLE_LIMIT; i++)
		pArena->aHeadFree[i] = NULL;
	pArena->pHeadSegment = NULL;
	pArena->pTailSegment = NULL;
	pArena->uQuantum = uQuantum;

#ifdef RA_STATS
	pArena->sStatistics.uSpanCount = 0;
	pArena->sStatistics.uLiveSegmentCount = 0;
	pArena->sStatistics.uFreeSegmentCount = 0;
	pArena->sStatistics.uFreeResourceCount = 0;
	pArena->sStatistics.uTotalResourceCount = 0;
	pArena->sStatistics.uCumulativeAllocs = 0;
	pArena->sStatistics.uCumulativeFrees = 0;
	pArena->sStatistics.uImportCount = 0;
	pArena->sStatistics.uExportCount = 0;
#endif

#if defined(CONFIG_PROC_FS) && defined(DEBUG)
	if (strcmp(pArena->name, "") != 0) {
		int ret;
		int (*pfnCreateProcEntry) (const char *, read_proc_t,
					   write_proc_t, void *);

		pArena->bInitProcEntry =
		    !PVRSRVGetInitServerState(PVRSRV_INIT_SERVER_SUCCESSFUL);

		pfnCreateProcEntry = pArena->bInitProcEntry ? CreateProcEntry :
					    CreatePerProcessProcEntry;

		ret = snprintf(pArena->szProcInfoName,
			     sizeof(pArena->szProcInfoName), "ra_info_%s",
			     pArena->name);
		if (ret > 0 && ret < sizeof(pArena->szProcInfoName)) {
			(void)pfnCreateProcEntry(ReplaceSpaces
					       (pArena->szProcInfoName),
					       RA_DumpInfo, NULL, pArena);
		} else {
			pArena->szProcInfoName[0] = 0;
			PVR_DPF(PVR_DBG_ERROR, "RA_Create: "
			      "couldn't create ra_info proc entry for arena %s",
				 pArena->name);
		}

		ret = snprintf(pArena->szProcSegsName,
			     sizeof(pArena->szProcSegsName), "ra_segs_%s",
			     pArena->name);
		if (ret > 0 && ret < sizeof(pArena->szProcInfoName)) {
			(void)pfnCreateProcEntry(ReplaceSpaces
					       (pArena->szProcSegsName),
					       RA_DumpSegs, NULL, pArena);
		} else {
			pArena->szProcSegsName[0] = 0;
			PVR_DPF(PVR_DBG_ERROR, "RA_Create: "
			      "couldn't create ra_segs proc entry for arena %s",
				 pArena->name);
		}
	}
#endif

	pArena->pSegmentHash = HASH_Create(MINIMUM_HASH_SIZE);
	if (pArena->pSegmentHash == NULL)
		goto hash_fail;
	if (uSize > 0) {
		uSize = (uSize + uQuantum - 1) / uQuantum * uQuantum;
		pBT = _InsertResource(pArena, base, uSize);
		if (pBT == NULL)
			goto insert_fail;
		pBT->psMapping = psMapping;

	}
	return pArena;

insert_fail:
	HASH_Delete(pArena->pSegmentHash);
hash_fail:
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct RA_ARENA), pArena,
		  NULL);
arena_fail:
	return NULL;
}

void RA_Delete(struct RA_ARENA *pArena)
{
	u32 uIndex;

	PVR_ASSERT(pArena != NULL);

	if (pArena == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "RA_Delete: invalid parameter - pArena");
		return;
	}

	PVR_DPF(PVR_DBG_MESSAGE, "RA_Delete: name='%s'", pArena->name);

	for (uIndex = 0; uIndex < FREE_TABLE_LIMIT; uIndex++)
		pArena->aHeadFree[uIndex] = NULL;

	while (pArena->pHeadSegment != NULL) {
		struct BT *pBT = pArena->pHeadSegment;
		PVR_ASSERT(pBT->type == btt_free);
		_SegmentListRemove(pArena, pBT);
		OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct BT), pBT,
			  NULL);
#ifdef RA_STATS
		pArena->sStatistics.uSpanCount--;
#endif
	}
#if defined(CONFIG_PROC_FS) && defined(DEBUG)
	{
		void (*pfnRemoveProcEntry) (const char *);

		pfnRemoveProcEntry =
		    pArena->
		    bInitProcEntry ? RemoveProcEntry :
		    RemovePerProcessProcEntry;

		if (pArena->szProcInfoName[0] != 0)
			pfnRemoveProcEntry(pArena->szProcInfoName);

		if (pArena->szProcSegsName[0] != 0)
			pfnRemoveProcEntry(pArena->szProcSegsName);
	}
#endif
	HASH_Delete(pArena->pSegmentHash);
	OSFreeMem(PVRSRV_OS_PAGEABLE_HEAP, sizeof(struct RA_ARENA), pArena,
		  NULL);
}

IMG_BOOL RA_TestDelete(struct RA_ARENA *pArena)
{
	PVR_ASSERT(pArena != NULL);

	if (pArena != NULL)
		while (pArena->pHeadSegment != NULL) {
			struct BT *pBT = pArena->pHeadSegment;
			if (pBT->type != btt_free)
				return IMG_FALSE;
		}

	return IMG_TRUE;
}

IMG_BOOL RA_Add(struct RA_ARENA *pArena, u32 base, size_t uSize)
{
	PVR_ASSERT(pArena != NULL);

	if (pArena == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "RA_Add: invalid parameter - pArena");
		return IMG_FALSE;
	}

	PVR_DPF(PVR_DBG_MESSAGE,
		 "RA_Add: name='%s', base=0x%x, size=0x%x", pArena->name, base,
		 uSize);

	uSize = (uSize + pArena->uQuantum - 1) /
		pArena->uQuantum * pArena->uQuantum;
	return (IMG_BOOL)(_InsertResource(pArena, base, uSize) != NULL);
}

IMG_BOOL RA_Alloc(struct RA_ARENA *pArena, size_t uRequestSize,
		  struct BM_MAPPING **ppsMapping, u32 uFlags, u32 uAlignment,
		  u32 *base)
{
	IMG_BOOL bResult;
	size_t uSize = uRequestSize;

	PVR_ASSERT(pArena != NULL);

	if (pArena == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "RA_Alloc: invalid parameter - pArena");
		return IMG_FALSE;
	}

	PVR_DPF(PVR_DBG_MESSAGE, "RA_Alloc: "
		 "arena='%s', size=0x%x(0x%x), alignment=0x%x",
		 pArena->name, uSize, uRequestSize, uAlignment);

	bResult = _AttemptAllocAligned(pArena, uSize, ppsMapping, uFlags,
				       uAlignment, base);
	if (!bResult) {
		struct BM_MAPPING *psImportMapping;
		u32 import_base;
		size_t uImportSize = uSize;

		if (uAlignment > pArena->uQuantum)
			uImportSize += (uAlignment - 1);

		uImportSize =
		    ((uImportSize + pArena->uQuantum - 1) /
		     pArena->uQuantum) * pArena->uQuantum;

		bResult =
		    pArena->pImportAlloc(pArena->pImportHandle, uImportSize,
					 &uImportSize, &psImportMapping, uFlags,
					 &import_base);
		if (bResult) {
			struct BT *pBT;
			pBT = _InsertResourceSpan(pArena, import_base,
						uImportSize);

			if (pBT == NULL) {
				pArena->pImportFree(pArena->pImportHandle,
						    import_base,
						    psImportMapping);
				PVR_DPF(PVR_DBG_MESSAGE, "RA_Alloc: "
					 "name='%s', size=0x%x failed!",
					 pArena->name, uSize);

				return IMG_FALSE;
			}
			pBT->psMapping = psImportMapping;
#ifdef RA_STATS
			pArena->sStatistics.uFreeSegmentCount++;
			pArena->sStatistics.uFreeResourceCount += uImportSize;
			pArena->sStatistics.uImportCount++;
			pArena->sStatistics.uSpanCount++;
#endif
			bResult = _AttemptAllocAligned(pArena, uSize,
						 ppsMapping, uFlags, uAlignment,
						 base);
			if (!bResult)
				PVR_DPF(PVR_DBG_MESSAGE, "RA_Alloc: "
					 "name='%s' uAlignment failed!",
					 pArena->name);
		}
	}
#ifdef RA_STATS
	if (bResult)
		pArena->sStatistics.uCumulativeAllocs++;
#endif

	PVR_DPF(PVR_DBG_MESSAGE,
		 "RA_Alloc: name='%s', size=0x%x, *base=0x%x = %d",
		 pArena->name, uSize, *base, bResult);

	return bResult;
}

void RA_Free(struct RA_ARENA *pArena, u32 base, IMG_BOOL bFreeBackingStore)
{
	struct BT *pBT;

	PVR_ASSERT(pArena != NULL);

	if (pArena == NULL) {
		PVR_DPF(PVR_DBG_ERROR, "RA_Free: invalid parameter - pArena");
		return;
	}

	PVR_DPF(PVR_DBG_MESSAGE,
		 "RA_Free: name='%s', base=0x%x", pArena->name, base);

	pBT = (struct BT *)HASH_Remove(pArena->pSegmentHash, base);
	PVR_ASSERT(pBT != NULL);

	if (pBT) {
		PVR_ASSERT(pBT->base == base);

#ifdef RA_STATS
		pArena->sStatistics.uCumulativeFrees++;
#endif

		_FreeBT(pArena, pBT, bFreeBackingStore);
	}
}

IMG_BOOL RA_GetNextLiveSegment(void *hArena,
			       struct RA_SEGMENT_DETAILS *psSegDetails)
{
	struct BT *pBT;

	if (psSegDetails->hSegment) {
		pBT = (struct BT *)psSegDetails->hSegment;
	} else {
		struct RA_ARENA *pArena = (struct RA_ARENA *)hArena;
		pBT = pArena->pHeadSegment;
	}

	while (pBT != NULL) {
		if (pBT->type == btt_live) {
			psSegDetails->uiSize = pBT->uSize;
			psSegDetails->sCpuPhyAddr.uiAddr = pBT->base;
			psSegDetails->hSegment = (void *) pBT->pNextSegment;

			return IMG_TRUE;
		}

		pBT = pBT->pNextSegment;
	}

	psSegDetails->uiSize = 0;
	psSegDetails->sCpuPhyAddr.uiAddr = 0;
	psSegDetails->hSegment = (void *) -1;

	return IMG_FALSE;
}

#if (defined(CONFIG_PROC_FS) && defined(DEBUG)) || defined(RA_STATS)
static char *_BTType(int eType)
{
	switch (eType) {
	case btt_span:
		return "span";
	case btt_free:
		return "free";
	case btt_live:
		return "live";
	}
	return "junk";
}
#endif

#if defined(ENABLE_RA_DUMP)
void RA_Dump(struct RA_ARENA *pArena)
{
	struct BT *pBT;
	PVR_ASSERT(pArena != NULL);
	PVR_DPF(PVR_DBG_MESSAGE, "Arena '%s':", pArena->name);
	PVR_DPF(PVR_DBG_MESSAGE,
		 "  alloc=%08X free=%08X handle=%08X quantum=%d",
		 pArena->pImportAlloc, pArena->pImportFree,
		 pArena->pImportHandle, pArena->uQuantum);
	PVR_DPF(PVR_DBG_MESSAGE, "  segment Chain:");
	if (pArena->pHeadSegment != NULL &&
	    pArena->pHeadSegment->pPrevSegment != NULL)
		PVR_DPF(PVR_DBG_MESSAGE,
			 "  error: head boundary tag has invalid pPrevSegment");
	if (pArena->pTailSegment != NULL &&
	    pArena->pTailSegment->pNextSegment != NULL)
		PVR_DPF(PVR_DBG_MESSAGE,
			 "  error: tail boundary tag has invalid pNextSegment");

	for (pBT = pArena->pHeadSegment; pBT != NULL; pBT = pBT->pNextSegment)
		PVR_DPF(PVR_DBG_MESSAGE,
			 "\tbase=0x%x size=0x%x type=%s ref=%08X",
			 (u32) pBT->base, pBT->uSize, _BTType(pBT->type),
			 pBT->pRef);

}
#endif

#if defined(CONFIG_PROC_FS) && defined(DEBUG)
static int RA_DumpSegs(char *page, char **start, off_t off, int count, int *eof,
		       void *data)
{
	struct BT *pBT = NULL;
	int len = 0;
	struct RA_ARENA *pArena = (struct RA_ARENA *)data;

	if (count < 80) {
		*start = (char *)0;
		return 0;
	}
	*eof = 0;
	*start = (char *)1;
	if (off == 0)
		return printAppend(page, count, 0,
			"Arena \"%s\"\nBase         Size Type Ref\n",
			pArena->name);
	for (pBT = pArena->pHeadSegment; --off && pBT;
	     pBT = pBT->pNextSegment)
		;
	if (pBT)
		len = printAppend(page, count, 0, "%08x %8x %4s %08x\n",
				  (unsigned)pBT->base, (unsigned)pBT->uSize,
				  _BTType(pBT->type), (unsigned)pBT->psMapping);
	else
		*eof = 1;
	return len;
}

static int RA_DumpInfo(char *page, char **start, off_t off, int count, int *eof,
	    void *data)
{
	int len = 0;
	struct RA_ARENA *pArena = (struct RA_ARENA *)data;

	if (count < 80) {
		*start = (char *)0;
		return 0;
	}
	*eof = 0;
	switch (off) {
	case 0:
		len = printAppend(page, count, 0, "quantum\t\t\t%u\n",
				  pArena->uQuantum);
		break;
	case 1:
		len = printAppend(page, count, 0, "import_handle\t\t%08X\n",
				(unsigned)pArena->pImportHandle);
		break;
#ifdef RA_STATS
	case 2:
		len = printAppend(page, count, 0, "span count\t\t%u\n",
				pArena->sStatistics.uSpanCount);
		break;
	case 3:
		len = printAppend(page, count, 0, "live segment count\t%u\n",
				pArena->sStatistics.uLiveSegmentCount);
		break;
	case 4:
		len = printAppend(page, count, 0, "free segment count\t%u\n",
				pArena->sStatistics.uFreeSegmentCount);
		break;
	case 5:
		len = printAppend(page, count, 0,
				"free resource count\t%u (0x%x)\n",
				pArena->sStatistics.uFreeResourceCount,
				(unsigned)pArena->sStatistics.
				uFreeResourceCount);
		break;
	case 6:
		len = printAppend(page, count, 0, "total allocs\t\t%u\n",
				pArena->sStatistics.uCumulativeAllocs);
		break;
	case 7:
		len = printAppend(page, count, 0, "total frees\t\t%u\n",
				pArena->sStatistics.uCumulativeFrees);
		break;
	case 8:
		len = printAppend(page, count, 0, "import count\t\t%u\n",
				pArena->sStatistics.uImportCount);
		break;
	case 9:
		len = printAppend(page, count, 0, "export count\t\t%u\n",
				pArena->sStatistics.uExportCount);
		break;
#endif

	default:
		*eof = 1;
	}
	*start = (char *)1;
	return len;
}
#endif

#ifdef RA_STATS
enum PVRSRV_ERROR RA_GetStats(struct RA_ARENA *pArena, char **ppszStr,
			      u32 *pui32StrLen)
{
	char *pszStr = *ppszStr;
	u32 ui32StrLen = *pui32StrLen;
	s32 i32Count;
	struct BT *pBT;

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "\nArena '%s':\n", pArena->name);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100,
		       "  allocCB=%08X freeCB=%08X handle=%08X quantum=%d\n",
		       pArena->pImportAlloc, pArena->pImportFree,
		       pArena->pImportHandle, pArena->uQuantum);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "span count\t\t%lu\n",
		       pArena->sStatistics.uSpanCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "live segment count\t%lu\n",
		       pArena->sStatistics.uLiveSegmentCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "free segment count\t%lu\n",
		       pArena->sStatistics.uFreeSegmentCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "free resource count\t%lu (0x%x)\n",
			      pArena->sStatistics.uFreeResourceCount,
			      (unsigned)pArena->sStatistics.uFreeResourceCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "total allocs\t\t%lu\n",
		       pArena->sStatistics.uCumulativeAllocs);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "total frees\t\t%lu\n",
		       pArena->sStatistics.uCumulativeFrees);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "import count\t\t%lu\n",
		       pArena->sStatistics.uImportCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "export count\t\t%lu\n",
		       pArena->sStatistics.uExportCount);
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	CHECK_SPACE(ui32StrLen);
	i32Count = OSSNPrintf(pszStr, 100, "  segment Chain:\n");
	UPDATE_SPACE(pszStr, i32Count, ui32StrLen);

	if (pArena->pHeadSegment != NULL &&
	    pArena->pHeadSegment->pPrevSegment != NULL) {
		CHECK_SPACE(ui32StrLen);
		i32Count = OSSNPrintf(pszStr, 100,
		       "  error: head boundary tag has invalid pPrevSegment\n");
		UPDATE_SPACE(pszStr, i32Count, ui32StrLen);
	}

	if (pArena->pTailSegment != NULL &&
	    pArena->pTailSegment->pNextSegment != NULL) {
		CHECK_SPACE(ui32StrLen);
		i32Count = OSSNPrintf(pszStr, 100,
		       "  error: tail boundary tag has invalid pNextSegment\n");
		UPDATE_SPACE(pszStr, i32Count, ui32StrLen);
	}

	for (pBT = pArena->pHeadSegment; pBT != NULL;
	     pBT = pBT->pNextSegment) {
		CHECK_SPACE(ui32StrLen);
		i32Count = OSSNPrintf(pszStr, 100,
			       "\tbase=0x%x size=0x%x type=%s ref=%08X\n",
			       (u32) pBT->base, pBT->uSize, _BTType(pBT->type),
			       pBT->psMapping);
		UPDATE_SPACE(pszStr, i32Count, ui32StrLen);
	}

	*ppszStr = pszStr;
	*pui32StrLen = ui32StrLen;

	return PVRSRV_OK;
}
#endif
