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

#ifndef __BRIDGED_PVR_BRIDGE_H__
#define __BRIDGED_PVR_BRIDGE_H__

#include "pvr_bridge.h"

#define PVRSRV_GET_BRIDGE_ID(X)	_IOC_NR(X)

#if defined(DEBUG_BRIDGE_KM)
enum PVRSRV_ERROR CopyFromUserWrapper(struct PVRSRV_PER_PROCESS_DATA *pProcData,
				      u32 ui32BridgeID, void *pvDest,
				      void __user *pvSrc, u32 ui32Size);
enum PVRSRV_ERROR CopyToUserWrapper(struct PVRSRV_PER_PROCESS_DATA *pProcData,
				    u32 ui32BridgeID, void __user *pvDest,
				    void *pvSrc, u32 ui32Size);
#else
#define CopyFromUserWrapper(pProcData, ui32BridgeID, pvDest, pvSrc, ui32Size) \
	OSCopyFromUser(pProcData, pvDest, pvSrc, ui32Size)
#define CopyToUserWrapper(pProcData, ui32BridgeID, pvDest, pvSrc, ui32Size) \
	OSCopyToUser(pProcData, pvDest, pvSrc, ui32Size)
#endif

#define ASSIGN_AND_RETURN_ON_ERROR(error, src, res)		\
	do {							\
		(error) = (src);				\
		if ((error) != PVRSRV_OK)			\
			return res;				\
	} while (error != PVRSRV_OK)

#define ASSIGN_AND_EXIT_ON_ERROR(error, src)		\
	ASSIGN_AND_RETURN_ON_ERROR(error, src, 0)

static inline enum PVRSRV_ERROR NewHandleBatch(
		struct PVRSRV_PER_PROCESS_DATA *psPerProc, u32 ui32BatchSize)
{
	enum PVRSRV_ERROR eError;

	PVR_ASSERT(!psPerProc->bHandlesBatched);

	eError = PVRSRVNewHandleBatch(psPerProc->psHandleBase, ui32BatchSize);

	if (eError == PVRSRV_OK)
		psPerProc->bHandlesBatched = IMG_TRUE;

	return eError;
}

#define NEW_HANDLE_BATCH_OR_ERROR(error, psPerProc, ui32BatchSize)	\
	ASSIGN_AND_EXIT_ON_ERROR(error, NewHandleBatch(psPerProc,	\
				 ui32BatchSize))

static inline enum PVRSRV_ERROR
CommitHandleBatch(struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	PVR_ASSERT(psPerProc->bHandlesBatched);

	psPerProc->bHandlesBatched = IMG_FALSE;

	return PVRSRVCommitHandleBatch(psPerProc->psHandleBase);
}

#define COMMIT_HANDLE_BATCH_OR_ERROR(error, psPerProc)			\
	ASSIGN_AND_EXIT_ON_ERROR(error, CommitHandleBatch(psPerProc))

static inline void ReleaseHandleBatch(struct PVRSRV_PER_PROCESS_DATA *psPerProc)
{
	if (psPerProc->bHandlesBatched) {
		psPerProc->bHandlesBatched = IMG_FALSE;

		PVRSRVReleaseHandleBatch(psPerProc->psHandleBase);
	}
}

int DummyBW(u32 ui32BridgeID, void *psBridgeIn, void *psBridgeOut,
	    struct PVRSRV_PER_PROCESS_DATA *psPerProc);

struct PVRSRV_BRIDGE_DISPATCH_TABLE_ENTRY {
	int (*pfFunction)(u32 ui32BridgeID, void *psBridgeIn, void *psBridgeOut,
			  struct PVRSRV_PER_PROCESS_DATA *psPerProc);
#if defined(DEBUG_BRIDGE_KM)
	const char *pszIOCName;
	const char *pszFunctionName;
	u32 ui32CallCount;
	u32 ui32CopyFromUserTotalBytes;
	u32 ui32CopyToUserTotalBytes;
#endif
};

#define BRIDGE_DISPATCH_TABLE_ENTRY_COUNT (PVRSRV_BRIDGE_LAST_SGX_CMD+1)
#define PVRSRV_BRIDGE_LAST_DEVICE_CMD	   PVRSRV_BRIDGE_LAST_SGX_CMD

extern struct PVRSRV_BRIDGE_DISPATCH_TABLE_ENTRY
	    g_BridgeDispatchTable[BRIDGE_DISPATCH_TABLE_ENTRY_COUNT];

void _SetDispatchTableEntry(u32 ui32Index,
			    const char *pszIOCName,
			    int (*pfFunction) (u32 ui32BridgeID,
					       void *psBridgeIn,
					       void *psBridgeOut,
					       struct PVRSRV_PER_PROCESS_DATA *
					       psPerProc),
			    const char *pszFunctionName);

#define SetDispatchTableEntry(ui32Index, pfFunction) \
	_SetDispatchTableEntry(PVRSRV_GET_BRIDGE_ID(ui32Index), #ui32Index, \
	   (int (*)(u32 ui32BridgeID, void *psBridgeIn, void *psBridgeOut,  \
	   struct PVRSRV_PER_PROCESS_DATA *psPerProc))pfFunction, #pfFunction)

#define DISPATCH_TABLE_GAP_THRESHOLD 5

#if defined(DEBUG)
#define PVRSRV_BRIDGE_ASSERT_CMD(X, Y) PVR_ASSERT(X == PVRSRV_GET_BRIDGE_ID(Y))
#else
#define PVRSRV_BRIDGE_ASSERT_CMD(X, Y) PVR_UNREFERENCED_PARAMETER(X)
#endif

#if defined(DEBUG_BRIDGE_KM)
struct PVRSRV_BRIDGE_GLOBAL_STATS {
	u32 ui32IOCTLCount;
	u32 ui32TotalCopyFromUserBytes;
	u32 ui32TotalCopyToUserBytes;
};

extern struct PVRSRV_BRIDGE_GLOBAL_STATS g_BridgeGlobalStats;
#endif

enum PVRSRV_ERROR CommonBridgeInit(void);

int BridgedDispatchKM(struct PVRSRV_PER_PROCESS_DATA *psPerProc,
		      struct PVRSRV_BRIDGE_PACKAGE *psBridgePackageKM);

#endif
