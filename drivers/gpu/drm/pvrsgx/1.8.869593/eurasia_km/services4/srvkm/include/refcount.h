/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
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
*/ /**************************************************************************/

#ifndef __REFCOUNT_H__
#define __REFCOUNT_H__

#include "pvr_bridge_km.h"

#if defined(PVRSRV_REFCOUNT_DEBUG)

void PVRSRVDumpRefCountCCB(void);

#define PVRSRVKernelSyncInfoIncRef(x...) \
	PVRSRVKernelSyncInfoIncRef2(__FILE__, __LINE__, x)
#define PVRSRVKernelSyncInfoDecRef(x...) \
	PVRSRVKernelSyncInfoDecRef2(__FILE__, __LINE__, x)
#define PVRSRVKernelMemInfoIncRef(x...) \
	PVRSRVKernelMemInfoIncRef2(__FILE__, __LINE__, x)
#define PVRSRVKernelMemInfoDecRef(x...) \
	PVRSRVKernelMemInfoDecRef2(__FILE__, __LINE__, x)
#define PVRSRVBMBufIncRef(x...) \
	PVRSRVBMBufIncRef2(__FILE__, __LINE__, x)
#define PVRSRVBMBufDecRef(x...) \
	PVRSRVBMBufDecRef2(__FILE__, __LINE__, x)
#define PVRSRVBMBufIncExport(x...) \
	PVRSRVBMBufIncExport2(__FILE__, __LINE__, x)
#define PVRSRVBMBufDecExport(x...) \
	PVRSRVBMBufDecExport2(__FILE__, __LINE__, x)

void PVRSRVKernelSyncInfoIncRef2(const IMG_CHAR *pszFile, IMG_INT iLine,
								 PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo,
								 PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo);
void PVRSRVKernelSyncInfoDecRef2(const IMG_CHAR *pszFile, IMG_INT iLine,
								 PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo,
								 PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo);
void PVRSRVKernelMemInfoIncRef2(const IMG_CHAR *pszFile, IMG_INT iLine,
								PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo);
void PVRSRVKernelMemInfoDecRef2(const IMG_CHAR *pszFile, IMG_INT iLine,
								PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo);
void PVRSRVBMBufIncRef2(const IMG_CHAR *pszFile,
						IMG_INT iLine, BM_BUF *pBuf);
void PVRSRVBMBufDecRef2(const IMG_CHAR *pszFile,
						IMG_INT iLine, BM_BUF *pBuf);
void PVRSRVBMBufIncExport2(const IMG_CHAR *pszFile,
						   IMG_INT iLine, BM_BUF *pBuf);
void PVRSRVBMBufDecExport2(const IMG_CHAR *pszFile,
						   IMG_INT iLine, BM_BUF *pBuf);

#if defined(__linux__)

/* mmap refcounting is Linux specific */
#include "mmap.h"

#define PVRSRVOffsetStructIncRef(x...) \
	PVRSRVOffsetStructIncRef2(__FILE__, __LINE__, x)
#define PVRSRVOffsetStructDecRef(x...) \
	PVRSRVOffsetStructDecRef2(__FILE__, __LINE__, x)
#define PVRSRVOffsetStructIncMapped(x...) \
	PVRSRVOffsetStructIncMapped2(__FILE__, __LINE__, x)
#define PVRSRVOffsetStructDecMapped(x...) \
	PVRSRVOffsetStructDecMapped2(__FILE__, __LINE__, x)

void PVRSRVOffsetStructIncRef2(const IMG_CHAR *pszFile, IMG_INT iLine,
							   PKV_OFFSET_STRUCT psOffsetStruct);
void PVRSRVOffsetStructDecRef2(const IMG_CHAR *pszFile, IMG_INT iLine,
							   PKV_OFFSET_STRUCT psOffsetStruct);
void PVRSRVOffsetStructIncMapped2(const IMG_CHAR *pszFile, IMG_INT iLine,
								  PKV_OFFSET_STRUCT psOffsetStruct);
void PVRSRVOffsetStructDecMapped2(const IMG_CHAR *pszFile, IMG_INT iLine,
								  PKV_OFFSET_STRUCT psOffsetStruct);

#endif /* defined(__linux__) */

#else /* defined(PVRSRV_REFCOUNT_DEBUG) */

static INLINE void PVRSRVDumpRefCountCCB(void) { }

static INLINE void PVRSRVKernelSyncInfoIncRef(PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo,
								PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo)
{
	PVR_UNREFERENCED_PARAMETER(psKernelMemInfo);
	PVRSRVAcquireSyncInfoKM(psKernelSyncInfo);
}

static INLINE void PVRSRVKernelSyncInfoDecRef(PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfo,
								PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo)
{
	PVR_UNREFERENCED_PARAMETER(psKernelMemInfo);
	PVRSRVReleaseSyncInfoKM(psKernelSyncInfo);
}

static INLINE void PVRSRVKernelMemInfoIncRef(PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo)
{
	psKernelMemInfo->ui32RefCount++;
}

static INLINE void PVRSRVKernelMemInfoDecRef(PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo)
{
	psKernelMemInfo->ui32RefCount--;
}

static INLINE void PVRSRVBMBufIncRef(BM_BUF *pBuf)
{
	pBuf->ui32RefCount++;
}

static INLINE void PVRSRVBMBufDecRef(BM_BUF *pBuf)
{
	pBuf->ui32RefCount--;
}

static INLINE void PVRSRVBMBufIncExport(BM_BUF *pBuf)
{
	pBuf->ui32ExportCount++;
}

static INLINE void PVRSRVBMBufDecExport(BM_BUF *pBuf)
{
	pBuf->ui32ExportCount--;
}

#if defined(__linux__)

/* mmap refcounting is Linux specific */
#include "mmap.h"

static INLINE void PVRSRVOffsetStructIncRef(PKV_OFFSET_STRUCT psOffsetStruct)
{
	psOffsetStruct->ui32RefCount++;
}

static INLINE void PVRSRVOffsetStructDecRef(PKV_OFFSET_STRUCT psOffsetStruct)
{
	psOffsetStruct->ui32RefCount--;
}

static INLINE void PVRSRVOffsetStructIncMapped(PKV_OFFSET_STRUCT psOffsetStruct)
{
	psOffsetStruct->ui32Mapped++;
}

static INLINE void PVRSRVOffsetStructDecMapped(PKV_OFFSET_STRUCT psOffsetStruct)
{
	psOffsetStruct->ui32Mapped--;
}

#endif /* defined(__linux__) */

#endif /* defined(PVRSRV_REFCOUNT_DEBUG) */

#endif /* __REFCOUNT_H__ */
