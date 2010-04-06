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

#ifndef _RA_H_
#define _RA_H_

#include "img_types.h"
#include "hash.h"
#include "osfunc.h"

struct RA_ARENA;
struct BM_MAPPING;

#define RA_STATS

struct RA_STATISTICS {
	u32 uSpanCount;
	u32 uLiveSegmentCount;
	u32 uFreeSegmentCount;
	u32 uTotalResourceCount;
	u32 uFreeResourceCount;
	u32 uCumulativeAllocs;
	u32 uCumulativeFrees;
	u32 uImportCount;
	u32 uExportCount;
};
struct RA_STATISTICS;

struct RA_SEGMENT_DETAILS {
	u32 uiSize;
	struct IMG_CPU_PHYADDR sCpuPhyAddr;
	void *hSegment;
};
struct RA_SEGMENT_DETAILS;

struct RA_ARENA *RA_Create(char *name, u32 base, size_t uSize,
			   struct BM_MAPPING *psMapping, size_t uQuantum,
			   IMG_BOOL(*imp_alloc)(void *_h, size_t uSize,
						size_t *pActualSize,
						struct BM_MAPPING **ppsMapping,
						u32 uFlags, u32 *pBase),
			   void (*imp_free)(void *, u32, struct BM_MAPPING *),
			   void (*backingstore_free)(void *, u32, u32, void *),
			   void *import_handle);

void RA_Delete(struct RA_ARENA *pArena);

IMG_BOOL RA_TestDelete(struct RA_ARENA *pArena);

IMG_BOOL RA_Add(struct RA_ARENA *pArena, u32 base, size_t uSize);

IMG_BOOL RA_Alloc(struct RA_ARENA *pArena, size_t uSize,
	 struct BM_MAPPING **ppsMapping, u32 uFlags, u32 uAlignment,
	 u32 *pBase);

void RA_Free(struct RA_ARENA *pArena, u32 base, IMG_BOOL bFreeBackingStore);

#ifdef RA_STATS

#define CHECK_SPACE(total)				\
{							\
	if (total < 100)				\
		return PVRSRV_ERROR_INVALID_PARAMS;	\
}

#define UPDATE_SPACE(str, count, total)			\
{							\
	if (count == -1)				\
		return PVRSRV_ERROR_INVALID_PARAMS;	\
	else {						\
		str += count;				\
		total -= count;				\
	}						\
}

IMG_BOOL RA_GetNextLiveSegment(void *hArena,
			       struct RA_SEGMENT_DETAILS *psSegDetails);

enum PVRSRV_ERROR RA_GetStats(struct RA_ARENA *pArena, char **ppszStr,
				u32 *pui32StrLen);

#endif

#endif
