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

#include <linux/version.h>
#include <asm/io.h>
#include <asm/page.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
#include <asm/exec.h>
#include <asm/cmpxchg.h>
#include <asm/auxvec.h>
#include <asm/switch_to.h>
#else
#include <asm/system.h>
#endif
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/pci.h>

#include <linux/string.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/hardirq.h>
#include <linux/timer.h>
#include <linux/capability.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "img_types.h"
#include "services_headers.h"
#include "mm.h"
#include "pvrmmap.h"
#include "mmap.h"
#include "env_data.h"
#include "proc.h"
#include "mutex.h"
#include "lock.h"

typedef struct PVRSRV_LINUX_EVENT_OBJECT_LIST_TAG
{
   rwlock_t 			   sLock;
   struct list_head        sList;

} PVRSRV_LINUX_EVENT_OBJECT_LIST;


typedef struct PVRSRV_LINUX_EVENT_OBJECT_TAG
{
	atomic_t	sTimeStamp;
	IMG_UINT32  ui32TimeStampPrevious;
#if defined(DEBUG)
	IMG_UINT	ui32Stats;
#endif
    wait_queue_head_t sWait;
	struct list_head        sList;
	IMG_HANDLE				hResItem;
	PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList;
} PVRSRV_LINUX_EVENT_OBJECT;

PVRSRV_ERROR LinuxEventObjectListCreate(IMG_HANDLE *phEventObjectList)
{
	PVRSRV_LINUX_EVENT_OBJECT_LIST *psEvenObjectList;

	if(OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP, sizeof(PVRSRV_LINUX_EVENT_OBJECT_LIST),
		(IMG_VOID **)&psEvenObjectList, IMG_NULL,
		"Linux Event Object List") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectCreate: failed to allocate memory for event list"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

    INIT_LIST_HEAD(&psEvenObjectList->sList);

	rwlock_init(&psEvenObjectList->sLock);

	*phEventObjectList = (IMG_HANDLE *) psEvenObjectList;

	return PVRSRV_OK;
}

PVRSRV_ERROR LinuxEventObjectListDestroy(IMG_HANDLE hEventObjectList)
{

	PVRSRV_LINUX_EVENT_OBJECT_LIST *psEvenObjectList = (PVRSRV_LINUX_EVENT_OBJECT_LIST *) hEventObjectList ;

	if(psEvenObjectList)
	{
		if (!list_empty(&psEvenObjectList->sList))
		{
			 PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectListDestroy: Event List is not empty"));
			 return PVRSRV_ERROR_GENERIC;
		}
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP, sizeof(PVRSRV_LINUX_EVENT_OBJECT_LIST), psEvenObjectList, IMG_NULL);

	}
	return PVRSRV_OK;
}


PVRSRV_ERROR LinuxEventObjectDelete(IMG_HANDLE hOSEventObjectList, IMG_HANDLE hOSEventObject)
{
	if(hOSEventObjectList)
	{
		if(hOSEventObject)
		{
			PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject = (PVRSRV_LINUX_EVENT_OBJECT *)hOSEventObject;
#if defined(DEBUG)
			PVR_DPF((PVR_DBG_MESSAGE, "LinuxEventObjectListDelete: Event object waits: %lu", psLinuxEventObject->ui32Stats));
#endif
			if(ResManFreeResByPtr(psLinuxEventObject->hResItem) != PVRSRV_OK)
			{
				return PVRSRV_ERROR_GENERIC;
			}

			return PVRSRV_OK;
		}
	}
	return PVRSRV_ERROR_GENERIC;

}

static PVRSRV_ERROR LinuxEventObjectDeleteCallback(IMG_PVOID pvParam, IMG_UINT32 ui32Param)
{
	PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject = pvParam;
	PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList = psLinuxEventObject->psLinuxEventObjectList;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	write_lock_bh(&psLinuxEventObjectList->sLock);
	list_del(&psLinuxEventObject->sList);
	write_unlock_bh(&psLinuxEventObjectList->sLock);

#if defined(DEBUG)
	PVR_DPF((PVR_DBG_MESSAGE, "LinuxEventObjectDeleteCallback: Event object waits: %lu", psLinuxEventObject->ui32Stats));
#endif

	OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP, sizeof(PVRSRV_LINUX_EVENT_OBJECT), psLinuxEventObject, IMG_NULL);


	return PVRSRV_OK;
}
PVRSRV_ERROR LinuxEventObjectAdd(IMG_HANDLE hOSEventObjectList, IMG_HANDLE *phOSEventObject)
 {
	PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject;
	PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList = (PVRSRV_LINUX_EVENT_OBJECT_LIST*)hOSEventObjectList;
	IMG_UINT32 ui32PID = OSGetCurrentProcessIDKM();
	PVRSRV_PER_PROCESS_DATA *psPerProc;

	psPerProc = PVRSRVPerProcessData(ui32PID);
	if (psPerProc == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectAdd: Couldn't find per-process data"));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}


	if(OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP, sizeof(PVRSRV_LINUX_EVENT_OBJECT),
		(IMG_VOID **)&psLinuxEventObject, IMG_NULL,
		"Linux Event Object") != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "LinuxEventObjectAdd: failed to allocate memory "));
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	INIT_LIST_HEAD(&psLinuxEventObject->sList);

	atomic_set(&psLinuxEventObject->sTimeStamp, 0);
	psLinuxEventObject->ui32TimeStampPrevious = 0;

#if defined(DEBUG)
	psLinuxEventObject->ui32Stats = 0;
#endif
    init_waitqueue_head(&psLinuxEventObject->sWait);

	psLinuxEventObject->psLinuxEventObjectList = psLinuxEventObjectList;

	psLinuxEventObject->hResItem = ResManRegisterRes(psPerProc->hResManContext,
													 RESMAN_TYPE_EVENT_OBJECT,
													 psLinuxEventObject,
													 0,
													 &LinuxEventObjectDeleteCallback);

	write_lock_bh(&psLinuxEventObjectList->sLock);
	list_add(&psLinuxEventObject->sList, &psLinuxEventObjectList->sList);
    write_unlock_bh(&psLinuxEventObjectList->sLock);

	*phOSEventObject = psLinuxEventObject;

	return PVRSRV_OK;
}

PVRSRV_ERROR LinuxEventObjectSignal(IMG_HANDLE hOSEventObjectList)
{
	PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject;
	PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList = (PVRSRV_LINUX_EVENT_OBJECT_LIST*)hOSEventObjectList;
	struct list_head *psListEntry, *psListEntryTemp, *psList;
	psList = &psLinuxEventObjectList->sList;

	read_lock(&psLinuxEventObjectList->sLock);
	list_for_each_safe(psListEntry, psListEntryTemp, psList)
	{

		psLinuxEventObject = (PVRSRV_LINUX_EVENT_OBJECT *)list_entry(psListEntry, PVRSRV_LINUX_EVENT_OBJECT, sList);

		atomic_inc(&psLinuxEventObject->sTimeStamp);
	 	wake_up_interruptible(&psLinuxEventObject->sWait);
	}
	read_unlock(&psLinuxEventObjectList->sLock);

	return 	PVRSRV_OK;

}

PVRSRV_ERROR LinuxEventObjectWait(IMG_HANDLE hOSEventObject, IMG_UINT32 ui32MSTimeout)
{
	IMG_UINT32 ui32TimeStamp;
	DEFINE_WAIT(sWait);

	PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject = (PVRSRV_LINUX_EVENT_OBJECT *) hOSEventObject;

	IMG_UINT32 ui32TimeOutJiffies = msecs_to_jiffies(ui32MSTimeout);

	do
	{
		prepare_to_wait(&psLinuxEventObject->sWait, &sWait, TASK_INTERRUPTIBLE);
		ui32TimeStamp = atomic_read(&psLinuxEventObject->sTimeStamp);

		if(psLinuxEventObject->ui32TimeStampPrevious != ui32TimeStamp)
		{
			break;
		}

		mutex_unlock(&gPVRSRVLock);

		ui32TimeOutJiffies = (IMG_UINT32)schedule_timeout((IMG_INT32)ui32TimeOutJiffies);

		mutex_lock(&gPVRSRVLock);
#if defined(DEBUG)
		psLinuxEventObject->ui32Stats++;
#endif


	} while (ui32TimeOutJiffies);

	finish_wait(&psLinuxEventObject->sWait, &sWait);

	psLinuxEventObject->ui32TimeStampPrevious = ui32TimeStamp;

	return ui32TimeOutJiffies ? PVRSRV_OK : PVRSRV_ERROR_TIMEOUT;

}

