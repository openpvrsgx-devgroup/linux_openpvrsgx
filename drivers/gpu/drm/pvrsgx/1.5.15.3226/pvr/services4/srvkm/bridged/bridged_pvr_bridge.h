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

#ifndef __BRIDGED_PVR_BRIDGE_H__
#define __BRIDGED_PVR_BRIDGE_H__

#include "pvr_bridge.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__linux__)
#define PVRSRV_GET_BRIDGE_ID(X)	_IOC_NR(X)
#else
#define PVRSRV_GET_BRIDGE_ID(X)	(X - PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST))
#endif

#ifndef ENOMEM
#define ENOMEM	12
#endif
#ifndef EFAULT
#define EFAULT	14
#endif
#ifndef ENOTTY
#define ENOTTY	25
#endif

#if defined(DEBUG_BRIDGE_KM)
PVRSRV_ERROR
CopyFromUserWrapper(PVRSRV_PER_PROCESS_DATA *pProcData,
					IMG_UINT32 ui32BridgeID,
					IMG_VOID *pvDest,
					IMG_VOID *pvSrc,
					IMG_UINT32 ui32Size);
PVRSRV_ERROR
CopyToUserWrapper(PVRSRV_PER_PROCESS_DATA *pProcData,
				  IMG_UINT32 ui32BridgeID,
				  IMG_VOID *pvDest,
				  IMG_VOID *pvSrc,
				  IMG_UINT32 ui32Size);
#else
#define CopyFromUserWrapper(pProcData, ui32BridgeID, pvDest, pvSrc, ui32Size) \
	OSCopyFromUser(pProcData, pvDest, pvSrc, ui32Size)
#define CopyToUserWrapper(pProcData, ui32BridgeID, pvDest, pvSrc, ui32Size) \
	OSCopyToUser(pProcData, pvDest, pvSrc, ui32Size)
#endif


#define ASSIGN_AND_RETURN_ON_ERROR(error, src, res)		\
	do							\
	{							\
		(error) = (src);				\
		if ((error) != PVRSRV_OK) 			\
		{						\
			return (res);				\
		}						\
	} while (error != PVRSRV_OK)

#define ASSIGN_AND_EXIT_ON_ERROR(error, src)		\
	ASSIGN_AND_RETURN_ON_ERROR(error, src, 0)

#if defined (PVR_SECURE_HANDLES)
#ifdef INLINE_IS_PRAGMA
#pragma inline(NewHandleBatch)
#endif
static INLINE PVRSRV_ERROR
NewHandleBatch(PVRSRV_PER_PROCESS_DATA *psPerProc,
					IMG_UINT32 ui32BatchSize)
{
	PVRSRV_ERROR eError;

	PVR_ASSERT(!psPerProc->bHandlesBatched);

	eError = PVRSRVNewHandleBatch(psPerProc->psHandleBase, ui32BatchSize);

	if (eError == PVRSRV_OK)
	{
		psPerProc->bHandlesBatched = IMG_TRUE;
	}

	return eError;
}

#define NEW_HANDLE_BATCH_OR_ERROR(error, psPerProc, ui32BatchSize)	\
	ASSIGN_AND_EXIT_ON_ERROR(error, NewHandleBatch(psPerProc, ui32BatchSize))

#ifdef INLINE_IS_PRAGMA
#pragma inline(CommitHandleBatch)
#endif
static INLINE PVRSRV_ERROR
CommitHandleBatch(PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVR_ASSERT(psPerProc->bHandlesBatched);

	psPerProc->bHandlesBatched = IMG_FALSE;

	return PVRSRVCommitHandleBatch(psPerProc->psHandleBase);
}


#define COMMIT_HANDLE_BATCH_OR_ERROR(error, psPerProc) 			\
	ASSIGN_AND_EXIT_ON_ERROR(error, CommitHandleBatch(psPerProc))

#ifdef INLINE_IS_PRAGMA
#pragma inline(ReleaseHandleBatch)
#endif
static INLINE IMG_VOID
ReleaseHandleBatch(PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	if (psPerProc->bHandlesBatched)
	{
		psPerProc->bHandlesBatched = IMG_FALSE;

		PVRSRVReleaseHandleBatch(psPerProc->psHandleBase);
	}
}
#else
#define NEW_HANDLE_BATCH_OR_ERROR(error, psPerProc, ui32BatchSize)
#define COMMIT_HANDLE_BATCH_OR_ERROR(error, psPerProc)
#define ReleaseHandleBatch(psPerProc)
#endif

IMG_INT
DummyBW(IMG_UINT32 ui32BridgeID,
		IMG_VOID *psBridgeIn,
		IMG_VOID *psBridgeOut,
		PVRSRV_PER_PROCESS_DATA *psPerProc);

typedef IMG_INT (*BridgeWrapperFunction)(IMG_UINT32 ui32BridgeID,
									 IMG_VOID *psBridgeIn,
									 IMG_VOID *psBridgeOut,
									 PVRSRV_PER_PROCESS_DATA *psPerProc);

typedef struct _PVRSRV_BRIDGE_DISPATCH_TABLE_ENTRY
{
	BridgeWrapperFunction pfFunction;
#if defined(DEBUG_BRIDGE_KM)
	const IMG_CHAR *pszIOCName;
	const IMG_CHAR *pszFunctionName;
	IMG_UINT32 ui32CallCount;
	IMG_UINT32 ui32CopyFromUserTotalBytes;
	IMG_UINT32 ui32CopyToUserTotalBytes;
#endif
}PVRSRV_BRIDGE_DISPATCH_TABLE_ENTRY;

#if defined(SUPPORT_VGX) || defined(SUPPORT_MSVDX)
	#if defined(SUPPORT_VGX)
		#define BRIDGE_DISPATCH_TABLE_ENTRY_COUNT (PVRSRV_BRIDGE_LAST_VGX_CMD+1)
		#define PVRSRV_BRIDGE_LAST_DEVICE_CMD	   PVRSRV_BRIDGE_LAST_VGX_CMD
	#else
		#define BRIDGE_DISPATCH_TABLE_ENTRY_COUNT (PVRSRV_BRIDGE_LAST_MSVDX_CMD+1)
		#define PVRSRV_BRIDGE_LAST_DEVICE_CMD	   PVRSRV_BRIDGE_LAST_MSVDX_CMD
	#endif
#else
	#if defined(SUPPORT_SGX)
		#define BRIDGE_DISPATCH_TABLE_ENTRY_COUNT (PVRSRV_BRIDGE_LAST_SGX_CMD+1)
		#define PVRSRV_BRIDGE_LAST_DEVICE_CMD	   PVRSRV_BRIDGE_LAST_SGX_CMD
	#else
		#define BRIDGE_DISPATCH_TABLE_ENTRY_COUNT (PVRSRV_BRIDGE_LAST_NON_DEVICE_CMD+1)
		#define PVRSRV_BRIDGE_LAST_DEVICE_CMD	   PVRSRV_BRIDGE_LAST_NON_DEVICE_CMD
	#endif
#endif

extern PVRSRV_BRIDGE_DISPATCH_TABLE_ENTRY g_BridgeDispatchTable[BRIDGE_DISPATCH_TABLE_ENTRY_COUNT];

IMG_VOID
_SetDispatchTableEntry(IMG_UINT32 ui32Index,
					   const IMG_CHAR *pszIOCName,
					   BridgeWrapperFunction pfFunction,
					   const IMG_CHAR *pszFunctionName);


#define SetDispatchTableEntry(ui32Index, pfFunction) \
	_SetDispatchTableEntry(PVRSRV_GET_BRIDGE_ID(ui32Index), #ui32Index, (BridgeWrapperFunction)pfFunction, #pfFunction)

#define DISPATCH_TABLE_GAP_THRESHOLD 5

#if defined(DEBUG)
#define PVRSRV_BRIDGE_ASSERT_CMD(X, Y) PVR_ASSERT(X == PVRSRV_GET_BRIDGE_ID(Y))
#else
#define PVRSRV_BRIDGE_ASSERT_CMD(X, Y) PVR_UNREFERENCED_PARAMETER(X)
#endif


#if defined(DEBUG_BRIDGE_KM)
typedef struct _PVRSRV_BRIDGE_GLOBAL_STATS
{
	IMG_UINT32 ui32IOCTLCount;
	IMG_UINT32 ui32TotalCopyFromUserBytes;
	IMG_UINT32 ui32TotalCopyToUserBytes;
}PVRSRV_BRIDGE_GLOBAL_STATS;

extern PVRSRV_BRIDGE_GLOBAL_STATS g_BridgeGlobalStats;
#endif


PVRSRV_ERROR CommonBridgeInit(IMG_VOID);

IMG_INT BridgedDispatchKM(PVRSRV_PER_PROCESS_DATA * psPerProc,
					  PVRSRV_BRIDGE_PACKAGE   * psBridgePackageKM);

#if defined (__cplusplus)
}
#endif

#endif

