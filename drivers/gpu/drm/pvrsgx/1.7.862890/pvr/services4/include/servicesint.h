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
 ******************************************************************************/

#if !defined (__SERVICESINT_H__)
#define __SERVICESINT_H__

#if defined (__cplusplus)
extern "C" {
#endif

#include "services.h"
#include "sysinfo.h"

#define HWREC_DEFAULT_TIMEOUT	(500)

#define DRIVERNAME_MAXLENGTH	(100)

#define	ALIGNSIZE(size, alignshift)	(((size) + ((1UL << (alignshift))-1)) & ~((1UL << (alignshift))-1))

#ifndef MAX
#define MAX(a,b) 					(((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) 					(((a) < (b)) ? (a) : (b))
#endif

typedef enum _PVRSRV_MEMTYPE_
{
	PVRSRV_MEMTYPE_UNKNOWN		= 0,
	PVRSRV_MEMTYPE_DEVICE		= 1,
	PVRSRV_MEMTYPE_DEVICECLASS	= 2,
	PVRSRV_MEMTYPE_WRAPPED		= 3,
	PVRSRV_MEMTYPE_MAPPED		= 4,
} PVRSRV_MEMTYPE;

typedef struct _PVRSRV_KERNEL_MEM_INFO_
{
	
	IMG_PVOID				pvLinAddrKM;

	
	IMG_DEV_VIRTADDR		sDevVAddr;

	
	IMG_UINT32				ui32Flags;

	
	IMG_SIZE_T				uAllocSize;

	
	PVRSRV_MEMBLK			sMemBlk;

	
	IMG_PVOID				pvSysBackupBuffer;

	
	IMG_UINT32				ui32RefCount;

	
	IMG_BOOL				bPendingFree;


#if defined(SUPPORT_MEMINFO_IDS)
	#if !defined(USE_CODE)
	
	IMG_UINT64				ui64Stamp;
	#else 
	IMG_UINT32				dummy1;
	IMG_UINT32				dummy2;
	#endif 
#endif 

	
	struct _PVRSRV_KERNEL_SYNC_INFO_	*psKernelSyncInfo;

	PVRSRV_MEMTYPE				memType;

    






	struct {
        

		IMG_BOOL bInUse;

        
		IMG_HANDLE hDevCookieInt;

        
		IMG_UINT32 ui32ShareIndex;

        

		IMG_UINT32 ui32OrigReqAttribs;
		IMG_UINT32 ui32OrigReqSize;
		IMG_UINT32 ui32OrigReqAlignment;
	} sShareMemWorkaround;
} PVRSRV_KERNEL_MEM_INFO;


typedef struct _PVRSRV_KERNEL_SYNC_INFO_
{
	
	PVRSRV_SYNC_DATA		*psSyncData;

	
	IMG_DEV_VIRTADDR		sWriteOpsCompleteDevVAddr;

	
	IMG_DEV_VIRTADDR		sReadOpsCompleteDevVAddr;

	
	PVRSRV_KERNEL_MEM_INFO	*psSyncDataMemInfoKM;

	
	
	IMG_UINT32              ui32RefCount;

	
	IMG_HANDLE hResItem;

	
	IMG_UINT32		ui32UID;
} PVRSRV_KERNEL_SYNC_INFO;

typedef struct _PVRSRV_DEVICE_SYNC_OBJECT_
{
	
	IMG_UINT32			ui32ReadOpsPendingVal;
	IMG_DEV_VIRTADDR	sReadOpsCompleteDevVAddr;
	IMG_UINT32			ui32WriteOpsPendingVal;
	IMG_DEV_VIRTADDR	sWriteOpsCompleteDevVAddr;
} PVRSRV_DEVICE_SYNC_OBJECT;

typedef struct _PVRSRV_SYNC_OBJECT
{
	PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfoKM;
	IMG_UINT32				ui32WriteOpsPending;
	IMG_UINT32				ui32ReadOpsPending;

}PVRSRV_SYNC_OBJECT, *PPVRSRV_SYNC_OBJECT;

typedef struct _PVRSRV_COMMAND
{
	IMG_SIZE_T			uCmdSize;		
	IMG_UINT32			ui32DevIndex;		
	IMG_UINT32			CommandType;		
	IMG_UINT32			ui32DstSyncCount;	
	IMG_UINT32			ui32SrcSyncCount;	
	PVRSRV_SYNC_OBJECT	*psDstSync;			
	PVRSRV_SYNC_OBJECT	*psSrcSync;			
	IMG_SIZE_T			uDataSize;		
	IMG_UINT32			ui32ProcessID;		
	IMG_VOID			*pvData;			
}PVRSRV_COMMAND, *PPVRSRV_COMMAND;


typedef struct _PVRSRV_QUEUE_INFO_
{
	IMG_VOID			*pvLinQueueKM;			
	IMG_VOID			*pvLinQueueUM;			
	volatile IMG_SIZE_T	ui32ReadOffset;			
	volatile IMG_SIZE_T	ui32WriteOffset;		
	IMG_UINT32			*pui32KickerAddrKM;		
	IMG_UINT32			*pui32KickerAddrUM;		
	IMG_SIZE_T			ui32QueueSize;			

	IMG_UINT32			ui32ProcessID;			

	IMG_HANDLE			hMemBlock[2];

	struct _PVRSRV_QUEUE_INFO_ *psNextKM;		
}PVRSRV_QUEUE_INFO;


typedef struct _PVRSRV_HEAP_INFO_KM_
{
	IMG_UINT32			ui32HeapID;
	IMG_DEV_VIRTADDR	sDevVAddrBase;

	IMG_HANDLE 			hDevMemHeap;
	IMG_UINT32			ui32HeapByteSize;
	IMG_UINT32			ui32Attribs;
	IMG_UINT32			ui32XTileStride;
}PVRSRV_HEAP_INFO_KM;


typedef struct _PVRSRV_EVENTOBJECT_KM_
{
	
	IMG_CHAR	szName[EVENTOBJNAME_MAXLENGTH];
	
	IMG_HANDLE	hOSEventKM;

} PVRSRV_EVENTOBJECT_KM;


typedef struct _PVRSRV_MISC_INFO_KM_
{
	IMG_UINT32	ui32StateRequest;		
	IMG_UINT32	ui32StatePresent;		

	
	IMG_VOID	*pvSOCTimerRegisterKM;
	IMG_VOID	*pvSOCTimerRegisterUM;
	IMG_HANDLE	hSOCTimerRegisterOSMemHandle;
	IMG_HANDLE	hSOCTimerRegisterMappingInfo;

	
	IMG_VOID	*pvSOCClockGateRegs;
	IMG_UINT32	ui32SOCClockGateRegsSize;

	
	IMG_CHAR	*pszMemoryStr;
	IMG_UINT32	ui32MemoryStrLen;

	
	PVRSRV_EVENTOBJECT_KM	sGlobalEventObject;
	IMG_HANDLE				hOSGlobalEvent;

	
	IMG_UINT32	aui32DDKVersion[4];

	
	struct
	{
		
		IMG_BOOL bDeferOp;

		
		PVRSRV_MISC_INFO_CPUCACHEOP_TYPE eCacheOpType;

		PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;

		
		IMG_VOID *pvBaseVAddr;

		
		IMG_UINT32	ui32Length;
	} sCacheOpCtl;
} PVRSRV_MISC_INFO_KM;


typedef PVRSRV_ERROR (*PFN_INSERT_CMD) (PVRSRV_QUEUE_INFO*,
										PVRSRV_COMMAND**,
										IMG_UINT32,
										IMG_UINT16,
										IMG_UINT32,
										PVRSRV_KERNEL_SYNC_INFO*[],
										IMG_UINT32,
										PVRSRV_KERNEL_SYNC_INFO*[],
										IMG_UINT32);
typedef PVRSRV_ERROR (*PFN_SUBMIT_CMD) (PVRSRV_QUEUE_INFO*, PVRSRV_COMMAND*, IMG_BOOL);


typedef struct PVRSRV_DEVICECLASS_BUFFER_TAG
{
	PFN_GET_BUFFER_ADDR		pfnGetBufferAddr;
	IMG_HANDLE				hDevMemContext;
	IMG_HANDLE				hExtDevice;
	IMG_HANDLE				hExtBuffer;
	PVRSRV_KERNEL_SYNC_INFO	*psKernelSyncInfo;
	IMG_UINT32				ui32MemMapRefCount;
} PVRSRV_DEVICECLASS_BUFFER;


typedef struct PVRSRV_CLIENT_DEVICECLASS_INFO_TAG
{
#if defined (SUPPORT_SID_INTERFACE)
	IMG_SID     hDeviceKM;
#else
	IMG_HANDLE hDeviceKM;
#endif
	IMG_HANDLE	hServices;
} PVRSRV_CLIENT_DEVICECLASS_INFO;


#ifdef INLINE_IS_PRAGMA
#pragma inline(PVRSRVGetWriteOpsPending)
#endif
static INLINE
IMG_UINT32 PVRSRVGetWriteOpsPending(PVRSRV_KERNEL_SYNC_INFO *psSyncInfo, IMG_BOOL bIsReadOp)
{
	IMG_UINT32 ui32WriteOpsPending;

	if(bIsReadOp)
	{
		ui32WriteOpsPending = psSyncInfo->psSyncData->ui32WriteOpsPending;
	}
	else
	{
		


		ui32WriteOpsPending = psSyncInfo->psSyncData->ui32WriteOpsPending++;
	}

	return ui32WriteOpsPending;
}

#ifdef INLINE_IS_PRAGMA
#pragma inline(PVRSRVGetReadOpsPending)
#endif
static INLINE
IMG_UINT32 PVRSRVGetReadOpsPending(PVRSRV_KERNEL_SYNC_INFO *psSyncInfo, IMG_BOOL bIsReadOp)
{
	IMG_UINT32 ui32ReadOpsPending;

	if(bIsReadOp)
	{
		ui32ReadOpsPending = psSyncInfo->psSyncData->ui32ReadOpsPending++;
	}
	else
	{
		ui32ReadOpsPending = psSyncInfo->psSyncData->ui32ReadOpsPending;
	}

	return ui32ReadOpsPending;
}

IMG_IMPORT
PVRSRV_ERROR PVRSRVQueueCommand(IMG_HANDLE hQueueInfo,
								PVRSRV_COMMAND *psCommand);


IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV
PVRSRVAllocSharedSysMem(const PVRSRV_CONNECTION *psConnection,
						IMG_UINT32 ui32Flags,
						IMG_SIZE_T ui32Size,
						PVRSRV_CLIENT_MEM_INFO **ppsClientMemInfo);

IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV
PVRSRVFreeSharedSysMem(const PVRSRV_CONNECTION *psConnection,
					   PVRSRV_CLIENT_MEM_INFO *psClientMemInfo);

IMG_IMPORT PVRSRV_ERROR
PVRSRVUnrefSharedSysMem(const PVRSRV_CONNECTION *psConnection,
                        PVRSRV_CLIENT_MEM_INFO *psClientMemInfo);

IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV
PVRSRVMapMemInfoMem(const PVRSRV_CONNECTION *psConnection,
#if defined (SUPPORT_SID_INTERFACE)
                    IMG_SID hKernelMemInfo,
#else
                    IMG_HANDLE hKernelMemInfo,
#endif
                    PVRSRV_CLIENT_MEM_INFO **ppsClientMemInfo);


#if defined (__cplusplus)
}
#endif
#endif 

