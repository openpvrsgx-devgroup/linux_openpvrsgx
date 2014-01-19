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

#ifndef _RA_H_
#define _RA_H_

#include "img_types.h"
#include "hash.h"
#include "osfunc.h"

typedef struct _RA_ARENA_ RA_ARENA;
typedef struct _BM_MAPPING_ BM_MAPPING;



#define RA_STATS


struct _RA_STATISTICS_
{

    IMG_SIZE_T uSpanCount;


    IMG_SIZE_T uLiveSegmentCount;


    IMG_SIZE_T uFreeSegmentCount;


    IMG_SIZE_T uTotalResourceCount;


    IMG_SIZE_T uFreeResourceCount;


    IMG_SIZE_T uCumulativeAllocs;


    IMG_SIZE_T uCumulativeFrees;


    IMG_SIZE_T uImportCount;


    IMG_SIZE_T uExportCount;
};
typedef struct _RA_STATISTICS_ RA_STATISTICS;

struct _RA_SEGMENT_DETAILS_
{
	IMG_SIZE_T      uiSize;
	IMG_CPU_PHYADDR sCpuPhyAddr;
	IMG_HANDLE      hSegment;
};
typedef struct _RA_SEGMENT_DETAILS_ RA_SEGMENT_DETAILS;

RA_ARENA *
RA_Create (IMG_CHAR *name,
           IMG_UINTPTR_T base,
           IMG_SIZE_T uSize,
           BM_MAPPING *psMapping,
           IMG_SIZE_T uQuantum,
           IMG_BOOL (*imp_alloc)(IMG_VOID *_h,
                                IMG_SIZE_T uSize,
                                IMG_SIZE_T *pActualSize,
                                BM_MAPPING **ppsMapping,
                                IMG_UINT32 uFlags,
                                IMG_UINTPTR_T *pBase),
           IMG_VOID (*imp_free) (IMG_VOID *,
                                IMG_UINTPTR_T,
                                BM_MAPPING *),
           IMG_VOID (*backingstore_free) (IMG_VOID *,
                                          IMG_SIZE_T,
                                          IMG_SIZE_T,
                                          IMG_HANDLE),
           IMG_VOID *import_handle);

IMG_VOID
RA_Delete (RA_ARENA *pArena);

IMG_BOOL
RA_TestDelete (RA_ARENA *pArena);

IMG_BOOL
RA_Add (RA_ARENA *pArena, IMG_UINTPTR_T base, IMG_SIZE_T uSize);

IMG_BOOL
RA_Alloc (RA_ARENA *pArena,
          IMG_SIZE_T uSize,
          IMG_SIZE_T *pActualSize,
          BM_MAPPING **ppsMapping,
          IMG_UINT32 uFlags,
          IMG_UINT32 uAlignment,
		  IMG_UINT32 uAlignmentOffset,
          IMG_UINTPTR_T *pBase);

IMG_VOID
RA_Free (RA_ARENA *pArena, IMG_UINTPTR_T base, IMG_BOOL bFreeBackingStore);


#ifdef RA_STATS

#define CHECK_SPACE(total)					\
{											\
	if(total<100) 							\
		return PVRSRV_ERROR_INVALID_PARAMS;	\
}

#define UPDATE_SPACE(str, count, total)		\
{											\
	if(count == -1)					 		\
		return PVRSRV_ERROR_INVALID_PARAMS;	\
	else									\
	{										\
		str += count;						\
		total -= count;						\
	}										\
}


IMG_BOOL RA_GetNextLiveSegment(IMG_HANDLE hArena, RA_SEGMENT_DETAILS *psSegDetails);


PVRSRV_ERROR RA_GetStats(RA_ARENA *pArena,
							IMG_CHAR **ppszStr,
							IMG_UINT32 *pui32StrLen);

#endif

#endif

