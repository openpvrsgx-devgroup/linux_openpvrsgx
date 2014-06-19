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

#include <linux/version.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <asm/page.h>
#include <linux/vmalloc.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15))
#include <linux/mutex.h>
#else
#include <asm/semaphore.h>
#endif
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
#include "dbgdriv.h"

#if defined(MODULE) && defined(DEBUG) && !defined(SUPPORT_DRI_DRM)
IMG_UINT32	gPVRDebugLevel = (DBGPRIV_FATAL | DBGPRIV_ERROR | DBGPRIV_WARNING);

#define PVR_STRING_TERMINATOR		'\0'
#define PVR_IS_FILE_SEPARATOR(character) ( ((character) == '\\') || ((character) == '/') )

void PVRSRVDebugPrintf	(
						IMG_UINT32	ui32DebugLevel,
						const IMG_CHAR*	pszFileName,
						IMG_UINT32	ui32Line,
						const IMG_CHAR*	pszFormat,
						...
					)
{
	IMG_BOOL bTrace;
#if !defined(__sh__)
	IMG_CHAR *pszLeafName;

	pszLeafName = (char *)strrchr (pszFileName, '\\');

	if (pszLeafName)
	{
		pszFileName = pszLeafName;
	}
#endif 

	bTrace = (IMG_BOOL)(ui32DebugLevel & DBGPRIV_CALLTRACE) ? IMG_TRUE : IMG_FALSE;

	if (gPVRDebugLevel & ui32DebugLevel)
	{
		va_list vaArgs;
		char szBuffer[256];
		char *szBufferEnd = szBuffer;
		char *szBufferLimit = szBuffer + sizeof(szBuffer) - 1;

		
		*szBufferLimit = '\0';

		snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "PVR_K:");
		szBufferEnd += strlen(szBufferEnd);

		
		if (bTrace == IMG_FALSE)
		{
			switch(ui32DebugLevel)
			{
				case DBGPRIV_FATAL:
				{
					snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Fatal):");
					break;
				}
				case DBGPRIV_ERROR:
				{
					snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Error):");
					break;
				}
				case DBGPRIV_WARNING:
				{
					snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Warning):");
					break;
				}
				case DBGPRIV_MESSAGE:
				{
					snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Message):");
					break;
				}
				case DBGPRIV_VERBOSE:
				{
					snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Verbose):");
					break;
				}
				default:
				{
					snprintf(szBufferEnd, szBufferLimit - szBufferEnd, "(Unknown message level)");
					break;
				}
			}
			szBufferEnd += strlen(szBufferEnd);
		}
		snprintf(szBufferEnd, szBufferLimit - szBufferEnd, " ");
		szBufferEnd += strlen(szBufferEnd);

		va_start (vaArgs, pszFormat);
		vsnprintf(szBufferEnd, szBufferLimit - szBufferEnd, pszFormat, vaArgs);
		va_end (vaArgs);
		szBufferEnd += strlen(szBufferEnd);

 		
 		if (bTrace == IMG_FALSE)
		{
			snprintf(szBufferEnd, szBufferLimit - szBufferEnd, 
			         " [%d, %s]", (int)ui32Line, pszFileName);
			szBufferEnd += strlen(szBufferEnd);
		}

		printk(KERN_INFO "%s\r\n", szBuffer);
	}
}
#endif	

IMG_VOID HostMemSet(IMG_VOID *pvDest, IMG_UINT8 ui8Value, IMG_UINT32 ui32Size)
{
	memset(pvDest, (int) ui8Value, (size_t) ui32Size);
}

IMG_VOID HostMemCopy(IMG_VOID *pvDst, IMG_VOID *pvSrc, IMG_UINT32 ui32Size)
{
#if defined(USE_UNOPTIMISED_MEMCPY)
    unsigned char *src,*dst;
    int i;

    src=(unsigned char *)pvSrc;
    dst=(unsigned char *)pvDst;
    for(i=0;i<ui32Size;i++)
    {
        dst[i]=src[i];
    }
#else
    memcpy(pvDst, pvSrc, ui32Size);
#endif
}

IMG_UINT32 HostReadRegistryDWORDFromString(char *pcKey, char *pcValueName, IMG_UINT32 *pui32Data)
{
    
	return 0;
}

IMG_VOID * HostPageablePageAlloc(IMG_UINT32 ui32Pages)
{
    return (void*)vmalloc(ui32Pages * PAGE_SIZE);
}

IMG_VOID HostPageablePageFree(IMG_VOID * pvBase)
{
    vfree(pvBase);
}

IMG_VOID * HostNonPageablePageAlloc(IMG_UINT32 ui32Pages)
{
    return (void*)vmalloc(ui32Pages * PAGE_SIZE);
}

IMG_VOID HostNonPageablePageFree(IMG_VOID * pvBase)
{
    vfree(pvBase);
}

IMG_VOID * HostMapKrnBufIntoUser(IMG_VOID * pvKrnAddr, IMG_UINT32 ui32Size, IMG_VOID **ppvMdl)
{
    
	return IMG_NULL;
}

IMG_VOID HostUnMapKrnBufFromUser(IMG_VOID * pvUserAddr, IMG_VOID * pvMdl, IMG_VOID * pvProcess)
{
    
}

IMG_VOID HostCreateRegDeclStreams(IMG_VOID)
{
    
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
typedef	struct mutex		MUTEX;
#define	INIT_MUTEX(m)		mutex_init(m)
#define	DOWN_TRYLOCK(m)		(!mutex_trylock(m))
#define	DOWN(m)			mutex_lock(m)
#define UP(m)			mutex_unlock(m)
#else
typedef	struct semaphore	MUTEX;
#define	INIT_MUTEX(m)		init_MUTEX(m)
#define	DOWN_TRYLOCK(m)		down_trylock(m)
#define	DOWN(m)			down(m)
#define UP(m)			up(m)
#endif

IMG_VOID *HostCreateMutex(IMG_VOID)
{
	MUTEX *psMutex;

	psMutex = kmalloc(sizeof(*psMutex), GFP_KERNEL);
	if (psMutex)
	{
		INIT_MUTEX(psMutex);
	}

	return psMutex;
}

IMG_VOID HostAquireMutex(IMG_VOID * pvMutex)
{
	BUG_ON(in_interrupt());

#if defined(PVR_DEBUG_DBGDRV_DETECT_HOST_MUTEX_COLLISIONS)
	if (DOWN_TRYLOCK((MUTEX *)pvMutex))
	{
		printk(KERN_INFO "HostAquireMutex: Waiting for mutex\n");
		DOWN((MUTEX *)pvMutex);
	}
#else
	DOWN((MUTEX *)pvMutex);
#endif
}

IMG_VOID HostReleaseMutex(IMG_VOID * pvMutex)
{
	UP((MUTEX *)pvMutex);
}

IMG_VOID HostDestroyMutex(IMG_VOID * pvMutex)
{
	if (pvMutex)
	{
		kfree(pvMutex);
	}
}

#if defined(SUPPORT_DBGDRV_EVENT_OBJECTS)

#define	EVENT_WAIT_TIMEOUT_MS	500
#define	EVENT_WAIT_TIMEOUT_JIFFIES	(EVENT_WAIT_TIMEOUT_MS * HZ / 1000)

static int iStreamData;
static wait_queue_head_t sStreamDataEvent;

IMG_INT32 HostCreateEventObjects(IMG_VOID)
{
	init_waitqueue_head(&sStreamDataEvent);

	return 0;
}

IMG_VOID HostWaitForEvent(DBG_EVENT eEvent)
{
	switch(eEvent)
	{
		case DBG_EVENT_STREAM_DATA:
			
			wait_event_interruptible_timeout(sStreamDataEvent, iStreamData != 0, EVENT_WAIT_TIMEOUT_JIFFIES);
			iStreamData = 0;
			break;
		default:
			
			msleep_interruptible(EVENT_WAIT_TIMEOUT_MS);
			break;
	}
}

IMG_VOID HostSignalEvent(DBG_EVENT eEvent)
{
	switch(eEvent)
	{
		case DBG_EVENT_STREAM_DATA:
			iStreamData = 1;
			wake_up_interruptible(&sStreamDataEvent);
			break;
		default:
			break;
	}
}

IMG_VOID HostDestroyEventObjects(IMG_VOID)
{
}
#endif	
