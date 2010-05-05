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

#include <linux/version.h>
#include <linux/io.h>
#include <asm/page.h>
#include <asm/system.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/pci.h>

#include <linux/string.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/hardirq.h>
#include <linux/timer.h>
#include <linux/capability.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#include "img_types.h"
#include "services_headers.h"
#include "mm.h"
#include "pvrmmap.h"
#include "mmap.h"
#include "env_data.h"
#include "proc.h"
#include "event.h"
#include "pvr_bridge_km.h"

struct PVRSRV_LINUX_EVENT_OBJECT_LIST {
	rwlock_t sLock;
	struct list_head sList;

};

struct PVRSRV_LINUX_EVENT_OBJECT {
	atomic_t sTimeStamp;
	u32 ui32TimeStampPrevious;
#if defined(CONFIG_PVR_DEBUG_EXTRA)
	unsigned ui32Stats;
#endif
	wait_queue_head_t sWait;
	struct list_head sList;
	void *hResItem;
	struct PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList;
};

enum PVRSRV_ERROR LinuxEventObjectListCreate(void **phEventObjectList)
{
	struct PVRSRV_LINUX_EVENT_OBJECT_LIST *psEvenObjectList;

	if (OSAllocMem
	    (PVRSRV_OS_NON_PAGEABLE_HEAP,
	     sizeof(struct PVRSRV_LINUX_EVENT_OBJECT_LIST),
	     (void **) &psEvenObjectList, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR, "LinuxEventObjectCreate: "
				"failed to allocate memory for event list");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	INIT_LIST_HEAD(&psEvenObjectList->sList);

	rwlock_init(&psEvenObjectList->sLock);

	*phEventObjectList = (void **) psEvenObjectList;

	return PVRSRV_OK;
}

enum PVRSRV_ERROR LinuxEventObjectListDestroy(void *hEventObjectList)
{
	struct PVRSRV_LINUX_EVENT_OBJECT_LIST *psEvenObjectList =
	    (struct PVRSRV_LINUX_EVENT_OBJECT_LIST *)hEventObjectList;

	if (psEvenObjectList) {
		if (!list_empty(&psEvenObjectList->sList)) {
			PVR_DPF(PVR_DBG_ERROR,
			"LinuxEventObjectListDestroy: Event List is not empty");
			return PVRSRV_ERROR_GENERIC;
		}
		OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
			  sizeof(struct PVRSRV_LINUX_EVENT_OBJECT_LIST),
			  psEvenObjectList, NULL);
	}
	return PVRSRV_OK;
}

enum PVRSRV_ERROR LinuxEventObjectDelete(void *hOSEventObjectList,
				    void *hOSEventObject)
{
	if (hOSEventObjectList)
		if (hOSEventObject) {
			struct PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject =
			    (struct PVRSRV_LINUX_EVENT_OBJECT *)hOSEventObject;
#if defined(CONFIG_PVR_DEBUG_EXTRA)
			PVR_DPF(PVR_DBG_MESSAGE,
			"LinuxEventObjectListDelete: Event object waits: %lu",
				 psLinuxEventObject->ui32Stats);
#endif
			ResManFreeResByPtr(psLinuxEventObject->hResItem);

			return PVRSRV_OK;
		}
	return PVRSRV_ERROR_GENERIC;

}

static enum PVRSRV_ERROR LinuxEventObjectDeleteCallback(void *pvParam,
							u32 ui32Param)
{
	struct PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject = pvParam;
	struct PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList =
	    psLinuxEventObject->psLinuxEventObjectList;

	PVR_UNREFERENCED_PARAMETER(ui32Param);

	write_lock_bh(&psLinuxEventObjectList->sLock);
	list_del(&psLinuxEventObject->sList);
	write_unlock_bh(&psLinuxEventObjectList->sLock);

#if defined(CONFIG_PVR_DEBUG_EXTRA)
	PVR_DPF(PVR_DBG_MESSAGE,
		 "LinuxEventObjectDeleteCallback: Event object waits: %lu",
		 psLinuxEventObject->ui32Stats);
#endif

	OSFreeMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
		  sizeof(struct PVRSRV_LINUX_EVENT_OBJECT), psLinuxEventObject,
		  NULL);

	return PVRSRV_OK;
}

enum PVRSRV_ERROR LinuxEventObjectAdd(void *hOSEventObjectList,
				 void **phOSEventObject)
{
	struct PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject;
	struct PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList =
	    (struct PVRSRV_LINUX_EVENT_OBJECT_LIST *)hOSEventObjectList;
	u32 ui32PID = OSGetCurrentProcessIDKM();
	struct PVRSRV_PER_PROCESS_DATA *psPerProc;

	psPerProc = PVRSRVPerProcessData(ui32PID);
	if (psPerProc == NULL) {
		PVR_DPF(PVR_DBG_ERROR,
			 "LinuxEventObjectAdd: Couldn't find per-process data");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	if (OSAllocMem(PVRSRV_OS_NON_PAGEABLE_HEAP,
	     sizeof(struct PVRSRV_LINUX_EVENT_OBJECT),
	     (void **) &psLinuxEventObject, NULL) != PVRSRV_OK) {
		PVR_DPF(PVR_DBG_ERROR,
			 "LinuxEventObjectAdd: failed to allocate memory ");
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	INIT_LIST_HEAD(&psLinuxEventObject->sList);

	atomic_set(&psLinuxEventObject->sTimeStamp, 0);
	psLinuxEventObject->ui32TimeStampPrevious = 0;

#if defined(CONFIG_PVR_DEBUG_EXTRA)
	psLinuxEventObject->ui32Stats = 0;
#endif
	init_waitqueue_head(&psLinuxEventObject->sWait);

	psLinuxEventObject->psLinuxEventObjectList = psLinuxEventObjectList;

	psLinuxEventObject->hResItem =
	    ResManRegisterRes(psPerProc->hResManContext,
			      RESMAN_TYPE_EVENT_OBJECT, psLinuxEventObject, 0,
			      &LinuxEventObjectDeleteCallback);

	write_lock_bh(&psLinuxEventObjectList->sLock);
	list_add(&psLinuxEventObject->sList, &psLinuxEventObjectList->sList);
	write_unlock_bh(&psLinuxEventObjectList->sLock);

	*phOSEventObject = psLinuxEventObject;

	return PVRSRV_OK;
}

enum PVRSRV_ERROR LinuxEventObjectSignal(void *hOSEventObjectList)
{
	struct PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject;
	struct PVRSRV_LINUX_EVENT_OBJECT_LIST *psLinuxEventObjectList =
	    (struct PVRSRV_LINUX_EVENT_OBJECT_LIST *)hOSEventObjectList;
	struct list_head *psListEntry, *psListEntryTemp, *psList;
	psList = &psLinuxEventObjectList->sList;

	list_for_each_safe(psListEntry, psListEntryTemp, psList) {

		psLinuxEventObject =
		    (struct PVRSRV_LINUX_EVENT_OBJECT *)list_entry(psListEntry,
					struct PVRSRV_LINUX_EVENT_OBJECT,
					sList);

		atomic_inc(&psLinuxEventObject->sTimeStamp);
		wake_up_interruptible(&psLinuxEventObject->sWait);
	}

	return PVRSRV_OK;

}

enum PVRSRV_ERROR LinuxEventObjectWait(void *hOSEventObject, u32 ui32MSTimeout)
{
	u32 ui32TimeStamp;
	DEFINE_WAIT(sWait);

	struct PVRSRV_LINUX_EVENT_OBJECT *psLinuxEventObject =
	    (struct PVRSRV_LINUX_EVENT_OBJECT *)hOSEventObject;

	u32 ui32TimeOutJiffies = msecs_to_jiffies(ui32MSTimeout);

	do {
		prepare_to_wait(&psLinuxEventObject->sWait, &sWait,
				TASK_INTERRUPTIBLE);
		ui32TimeStamp = atomic_read(&psLinuxEventObject->sTimeStamp);

		if (psLinuxEventObject->ui32TimeStampPrevious != ui32TimeStamp)
			break;

		pvr_unlock();

		ui32TimeOutJiffies =
		    (u32) schedule_timeout((s32) ui32TimeOutJiffies);

		pvr_lock();

		if (pvr_is_disabled()) {
			ui32TimeOutJiffies = 1;
			break;
		}
#if defined(CONFIG_PVR_DEBUG_EXTRA)
		psLinuxEventObject->ui32Stats++;
#endif

	} while (ui32TimeOutJiffies);

	finish_wait(&psLinuxEventObject->sWait, &sWait);

	psLinuxEventObject->ui32TimeStampPrevious = ui32TimeStamp;

	return ui32TimeOutJiffies ? PVRSRV_OK : PVRSRV_ERROR_TIMEOUT;

}
