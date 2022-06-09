/*************************************************************************/ /*!
@Title          Services Internal Header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    services internal details
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#if !defined (__SERVICESINT_H__)
#define __SERVICESINT_H__

#if defined (__cplusplus)
extern "C" {
#endif

#include "services.h"
#include "sysinfo.h"

#define HWREC_DEFAULT_TIMEOUT	(500)

#define DRIVERNAME_MAXLENGTH	(100)

typedef enum _PVRSRV_MEMTYPE_
{
	PVRSRV_MEMTYPE_UNKNOWN		= 0,
	PVRSRV_MEMTYPE_DEVICE		= 1,
	PVRSRV_MEMTYPE_DEVICECLASS	= 2,
	PVRSRV_MEMTYPE_WRAPPED		= 3,
	PVRSRV_MEMTYPE_MAPPED		= 4,
}
PVRSRV_MEMTYPE;

/*
	Kernel Memory Information structure
*/
typedef struct _PVRSRV_KERNEL_MEM_INFO_
{

	IMG_PVOID				pvLinAddrKM;


	IMG_DEV_VIRTADDR		sDevVAddr;


	IMG_UINT32				ui32Flags;


	IMG_SIZE_T				ui32AllocSize;


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

	PVRSRV_MEMTYPE			memType;

} PVRSRV_KERNEL_MEM_INFO;


/*
	Kernel Sync Info structure
*/
typedef struct _PVRSRV_KERNEL_SYNC_INFO_
{

	PVRSRV_SYNC_DATA		*psSyncData;


	IMG_DEV_VIRTADDR		sWriteOpsCompleteDevVAddr;


	IMG_DEV_VIRTADDR		sReadOpsCompleteDevVAddr;


	/* meminfo for sync data */
	PVRSRV_KERNEL_MEM_INFO	*psSyncDataMemInfoKM;


	IMG_HANDLE				hResItem;



        IMG_UINT32              ui32RefCount;

} PVRSRV_KERNEL_SYNC_INFO;

/*!
 *****************************************************************************
 *	This is a device addressable version of a pvrsrv_sync_oject
 *	- any hw cmd may have an unlimited number of these
 ****************************************************************************/
typedef struct _PVRSRV_DEVICE_SYNC_OBJECT_
{

	IMG_UINT32			ui32ReadOpsPendingVal;
	IMG_DEV_VIRTADDR	sReadOpsCompleteDevVAddr;
	IMG_UINT32			ui32WriteOpsPendingVal;
	IMG_DEV_VIRTADDR	sWriteOpsCompleteDevVAddr;
} PVRSRV_DEVICE_SYNC_OBJECT;

/*!
 *****************************************************************************
 *	encapsulates a single sync object
 *	- any cmd may have an unlimited number of these
 ****************************************************************************/
typedef struct _PVRSRV_SYNC_OBJECT
{
	PVRSRV_KERNEL_SYNC_INFO *psKernelSyncInfoKM;
	IMG_UINT32				ui32WriteOpsPending;
	IMG_UINT32				ui32ReadOpsPending;

}PVRSRV_SYNC_OBJECT, *PPVRSRV_SYNC_OBJECT;

/*!
 *****************************************************************************
 * The `one size fits all' generic command.
 ****************************************************************************/
typedef struct _PVRSRV_COMMAND
{
	IMG_SIZE_T			ui32CmdSize;
	IMG_UINT32			ui32DevIndex;
	IMG_UINT32			CommandType;
	IMG_UINT32			ui32DstSyncCount;
	IMG_UINT32			ui32SrcSyncCount;
	PVRSRV_SYNC_OBJECT	*psDstSync;
	PVRSRV_SYNC_OBJECT	*psSrcSync;
	IMG_SIZE_T			ui32DataSize;
	IMG_UINT32			ui32ProcessID;
	IMG_VOID			*pvData;
}PVRSRV_COMMAND, *PPVRSRV_COMMAND;


/*!
 *****************************************************************************
 * Circular command buffer structure forming the queue of pending commands.
 *
 * Queues are implemented as circular comamnd buffers (CCBs).
 * The buffer is allocated as a specified size, plus the size of the largest supported command.
 * The extra size allows commands to be added without worrying about wrapping around at the end.
 *
 * Commands are added to the CCB by client processes and consumed within
 * kernel mode code running from within  an L/MISR typically.
 *
 * The process of adding a command to a queue is as follows:-
 * 	- A `lock' is acquired to prevent other processes from adding commands to a queue
 * 	- Data representing the command to be executed, along with it's PVRSRV_SYNC_INFO
 * 	  dependencies is written to the buffer representing the queue at the queues
 * 	  current WriteOffset.
 * 	- The PVRSRV_SYNC_INFO that the command depends on are updated to reflect
 * 	  the addition of the new command.
 * 	- The WriteOffset is incremented by the size of the command added.
 * 	- If the WriteOffset now lies beyound the declared buffer size, it is
 * 	  reset to zero.
 * 	- The semaphore is released.
 *
 *****************************************************************************/
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

typedef PVRSRV_ERROR (*PFN_INSERT_CMD) (PVRSRV_QUEUE_INFO*,
										PVRSRV_COMMAND**,
										IMG_UINT32,
										IMG_UINT16,
										IMG_UINT32,
										PVRSRV_KERNEL_SYNC_INFO*[],
										IMG_UINT32,
										PVRSRV_KERNEL_SYNC_INFO*[],
										IMG_UINT32);
/* submit command function pointer */
typedef PVRSRV_ERROR (*PFN_SUBMIT_CMD) (PVRSRV_QUEUE_INFO*, PVRSRV_COMMAND*, IMG_BOOL);


typedef struct PVRSRV_DEVICECLASS_BUFFER_TAG
{
	PFN_GET_BUFFER_ADDR		pfnGetBufferAddr;
	IMG_HANDLE				hDevMemContext;
	IMG_HANDLE				hExtDevice;
	IMG_HANDLE				hExtBuffer;
	PVRSRV_KERNEL_SYNC_INFO	*psKernelSyncInfo;

} PVRSRV_DEVICECLASS_BUFFER;


typedef struct PVRSRV_CLIENT_DEVICECLASS_INFO_TAG
{
	IMG_HANDLE hDeviceKM;
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
PVRSRVGetMMUContextPDDevPAddr(const PVRSRV_CONNECTION *psConnection,
                              IMG_HANDLE hDevMemContext,
                              IMG_DEV_PHYADDR *sPDDevPAddr);

IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV
PVRSRVAllocSharedSysMem(const PVRSRV_CONNECTION *psConnection,
						IMG_UINT32 ui32Flags,
						IMG_SIZE_T ui32Size,
						PVRSRV_CLIENT_MEM_INFO **ppsClientMemInfo);

/*!
 * *****************************************************************************
 * @Description Frees memory allocated via PVRSRVAllocSharedMemory (Note you must
 *        be sure any additional kernel references you created have been
 *        removed before freeing the memory)
 *
 * @Input psConnection
 * @Input psClientMemInfo
 *
 * @Return PVRSRV_ERROR
 ********************************************************************************/
IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV
PVRSRVFreeSharedSysMem(const PVRSRV_CONNECTION *psConnection,
					   PVRSRV_CLIENT_MEM_INFO *psClientMemInfo);

/*!
 * *****************************************************************************
 * @Description Removes any userspace reference to the shared system memory, except
 *        that the memory will remain registered with the services resource
 *        manager so if the process dies/exits the actuall shared memory will
 *        still be freed.
 *        If you need to move ownership of shared memory from userspace
 *        to kernel space then before unrefing a shared piece of memory you can
 *        take a copy of psClientMemInfo->hKernelMemInfo; call
 *        PVRSRVUnrefSharedSysMem; then use some mechanism (specialised bridge
 *        function) to request that the kernel remove any resource manager
 *        reference to the shared memory and assume responsaility for the meminfo
 *        in one atomic operation. (Note to aid with such a kernel space bridge
 *        function see PVRSRVDissociateSharedSysMemoryKM)
 *
 * @Input psConnection
 * @Input psClientMemInfo
 *
 * @Return PVRSRV_ERROR
 ********************************************************************************/
IMG_IMPORT PVRSRV_ERROR
PVRSRVUnrefSharedSysMem(const PVRSRV_CONNECTION *psConnection,
                        PVRSRV_CLIENT_MEM_INFO *psClientMemInfo);

/*!
 * *****************************************************************************
 * @Description For shared system or device memory that is owned by the kernel, you can
 *              use this function to map the underlying memory into a client using a
 *              handle for the KernelMemInfo.
 *
 * @Input psConnection
 * @Input hKernelMemInfo
 * @Output ppsClientMemInfo
 *
 * @Return PVRSRV_ERROR
 ********************************************************************************/
IMG_IMPORT PVRSRV_ERROR IMG_CALLCONV
PVRSRVMapMemInfoMem(const PVRSRV_CONNECTION *psConnection,
                    IMG_HANDLE hKernelMemInfo,
                    PVRSRV_CLIENT_MEM_INFO **ppsClientMemInfo);


#if defined (__cplusplus)
}
#endif
#endif /* __SERVICESINT_H__ */

/*****************************************************************************
 End of file (servicesint.h)
*****************************************************************************/
