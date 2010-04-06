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
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/hardirq.h>

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#endif

#include "img_types.h"
#include "pvr_debug.h"

#include "dbgdrvif.h"
#include "hostfunc.h"

#define PVR_STRING_TERMINATOR		'\0'
#define PVR_IS_FILE_SEPARATOR(character) \
	(((character) == '\\') || ((character) == '/'))

static u32 gPVRDebugLevel = DBGPRIV_WARNING;

void PVRSRVDebugPrintf(u32 ui32DebugLevel,
		       const char *pszFileName,
		       u32 ui32Line, const char *pszFormat, ...
    )
{
	IMG_BOOL bTrace, bDebug;
	char *pszLeafName;

	pszLeafName = (char *)strrchr(pszFileName, '\\');

	if (pszLeafName)
		pszFileName = pszLeafName;

	bTrace = gPVRDebugLevel & ui32DebugLevel & DBGPRIV_CALLTRACE;
	bDebug = ((gPVRDebugLevel & DBGPRIV_ALLLEVELS) >= ui32DebugLevel);

	if (bTrace || bDebug) {
		va_list vaArgs;
		static char szBuffer[256];

		va_start(vaArgs, pszFormat);

		if (bDebug) {
			switch (ui32DebugLevel) {
			case DBGPRIV_FATAL:
				{
					strcpy(szBuffer, "PVR_K:(Fatal): ");
					break;
				}
			case DBGPRIV_ERROR:
				{
					strcpy(szBuffer, "PVR_K:(Error): ");
					break;
				}
			case DBGPRIV_WARNING:
				{
					strcpy(szBuffer, "PVR_K:(Warning): ");
					break;
				}
			case DBGPRIV_MESSAGE:
				{
					strcpy(szBuffer, "PVR_K:(Message): ");
					break;
				}
			case DBGPRIV_VERBOSE:
				{
					strcpy(szBuffer, "PVR_K:(Verbose): ");
					break;
				}
			default:
				{
					strcpy(szBuffer,
					       "PVR_K:(Unknown message level)");
					break;
				}
			}
		} else {
			strcpy(szBuffer, "PVR_K: ");
		}

		vsprintf(&szBuffer[strlen(szBuffer)], pszFormat, vaArgs);

		if (!bTrace)
			sprintf(&szBuffer[strlen(szBuffer)], " [%d, %s]",
				(int)ui32Line, pszFileName);

		printk(KERN_INFO "%s\r\n", szBuffer);

		va_end(vaArgs);
	}
}

void HostMemSet(void *pvDest, u8 ui8Value, u32 ui32Size)
{
	memset(pvDest, (int)ui8Value, (size_t) ui32Size);
}

void HostMemCopy(void *pvDst, void *pvSrc, u32 ui32Size)
{
	memcpy(pvDst, pvSrc, ui32Size);
}

u32 HostReadRegistryDWORDFromString(char *pcKey, char *pcValueName,
					   u32 *pui32Data)
{

	return 0;
}

void *HostPageablePageAlloc(u32 ui32Pages)
{
	return (void *)vmalloc(ui32Pages * PAGE_SIZE);
}

void HostPageablePageFree(void *pvBase)
{
	vfree(pvBase);
}

void *HostNonPageablePageAlloc(u32 ui32Pages)
{
	return (void *)vmalloc(ui32Pages * PAGE_SIZE);
}

void HostNonPageablePageFree(void *pvBase)
{
	vfree(pvBase);
}

void *HostMapKrnBufIntoUser(void *pvKrnAddr, u32 ui32Size,
				void **ppvMdl)
{

	return NULL;
}

void HostUnMapKrnBufFromUser(void *pvUserAddr, void *pvMdl,
				 void *pvProcess)
{

}

void HostCreateRegDeclStreams(void)
{

}

void *HostCreateMutex(void)
{
	struct mutex *psSem;

	psSem = kmalloc(sizeof(*psSem), GFP_KERNEL);
	if (psSem)
		mutex_init(psSem);

	return psSem;
}

void HostAquireMutex(void *pvMutex)
{
	BUG_ON(in_interrupt());

#if defined(PVR_DEBUG_DBGDRV_DETECT_HOST_MUTEX_COLLISIONS)
	if (mutex_trylock((struct mutex *)pvMutex)) {
		printk(KERN_INFO "HostAquireMutex: Waiting for mutex\n");
		mutex_lock((struct mutex *)pvMutex);
	}
#else
	mutex_lock((struct mutex *)pvMutex);
#endif
}

void HostReleaseMutex(void *pvMutex)
{
	mutex_unlock((struct mutex *)pvMutex);
}

void HostDestroyMutex(void *pvMutex)
{
	kfree(pvMutex);
}

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)

#define	EVENT_WAIT_TIMEOUT_MS	500
#define	EVENT_WAIT_TIMEOUT_JIFFIES	(EVENT_WAIT_TIMEOUT_MS * HZ / 1000)

static int iStreamData;
static wait_queue_head_t sStreamDataEvent;

s32 HostCreateEventObjects(void)
{
	init_waitqueue_head(&sStreamDataEvent);

	return 0;
}

void HostWaitForEvent(enum DBG_EVENT eEvent)
{
	switch (eEvent) {
	case DBG_EVENT_STREAM_DATA:

		wait_event_interruptible_timeout(sStreamDataEvent,
						 iStreamData != 0,
						 EVENT_WAIT_TIMEOUT_JIFFIES);
		iStreamData = 0;
		break;
	default:

		msleep_interruptible(EVENT_WAIT_TIMEOUT_MS);
		break;
	}
}

void HostSignalEvent(enum DBG_EVENT eEvent)
{
	switch (eEvent) {
	case DBG_EVENT_STREAM_DATA:
		iStreamData = 1;
		wake_up_interruptible(&sStreamDataEvent);
		break;
	default:
		break;
	}
}

void HostDestroyEventObjects(void)
{
}
#endif
