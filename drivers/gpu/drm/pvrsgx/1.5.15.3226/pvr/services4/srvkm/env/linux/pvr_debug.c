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


#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/hardirq.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/tty.h>
#include <stdarg.h>
#include "img_types.h"
#include "servicesext.h"
#include "pvr_debug.h"
#include "proc.h"
#include "mutex.h"
#include "linkage.h"

#if defined(PVRSRV_NEED_PVR_DPF)

#define PVR_MAX_FILEPATH_LEN 256

static IMG_UINT32 gPVRDebugLevel = DBGPRIV_WARNING;

#endif

#define	PVR_MAX_MSG_LEN PVR_MAX_DEBUG_MESSAGE_LEN

static IMG_CHAR gszBufferNonIRQ[PVR_MAX_MSG_LEN + 1];

static IMG_CHAR gszBufferIRQ[PVR_MAX_MSG_LEN + 1];

static struct mutex gsDebugMutexNonIRQ;

DEFINE_SPINLOCK(gsDebugLockIRQ);

#define	USE_SPIN_LOCK (in_interrupt() || !preemptible())

static inline void GetBufferLock(unsigned long *pulLockFlags)
{
	if (USE_SPIN_LOCK)
	{
		spin_lock_irqsave(&gsDebugLockIRQ, *pulLockFlags);
	}
	else
	{
		mutex_lock(&gsDebugMutexNonIRQ);
	}
}

static inline void ReleaseBufferLock(unsigned long ulLockFlags)
{
	if (USE_SPIN_LOCK)
	{
		spin_unlock_irqrestore(&gsDebugLockIRQ, ulLockFlags);
	}
	else
	{
		mutex_unlock(&gsDebugMutexNonIRQ);
	}
}

static inline void SelectBuffer(IMG_CHAR **ppszBuf, IMG_UINT32 *pui32BufSiz)
{
	if (USE_SPIN_LOCK)
	{
		*ppszBuf = gszBufferIRQ;
		*pui32BufSiz = sizeof(gszBufferIRQ);
	}
	else
	{
		*ppszBuf = gszBufferNonIRQ;
		*pui32BufSiz = sizeof(gszBufferNonIRQ);
	}
}

static IMG_BOOL VBAppend(IMG_CHAR *pszBuf, IMG_UINT32 ui32BufSiz, const IMG_CHAR* pszFormat, va_list VArgs)
{
	IMG_UINT32 ui32Used;
	IMG_UINT32 ui32Space;
	IMG_INT32 i32Len;

	ui32Used = strlen(pszBuf);
	BUG_ON(ui32Used >= ui32BufSiz);
	ui32Space = ui32BufSiz - ui32Used;

	i32Len = vsnprintf(&pszBuf[ui32Used], ui32Space, pszFormat, VArgs);
	pszBuf[ui32BufSiz - 1] = 0;


	return (i32Len < 0 || i32Len >= ui32Space);
}

IMG_VOID PVRDPFInit(IMG_VOID)
{
    mutex_init(&gsDebugMutexNonIRQ);
}

IMG_VOID PVRSRVReleasePrintf(const IMG_CHAR *pszFormat, ...)
{
	va_list vaArgs;
	unsigned long ulLockFlags = 0;
	IMG_CHAR *pszBuf;
	IMG_UINT32 ui32BufSiz;

	SelectBuffer(&pszBuf, &ui32BufSiz);

	va_start(vaArgs, pszFormat);

	GetBufferLock(&ulLockFlags);
	strncpy (pszBuf, "PVR_K: ", (ui32BufSiz -1));

	if (VBAppend(pszBuf, ui32BufSiz, pszFormat, vaArgs))
	{
		printk(KERN_INFO "PVR_K:(Message Truncated): %s\n", pszBuf);
	}
	else
	{
		printk(KERN_INFO "%s\n", pszBuf);
	}

	ReleaseBufferLock(ulLockFlags);
	va_end(vaArgs);

}

#if defined(PVRSRV_NEED_PVR_ASSERT)

IMG_VOID PVRSRVDebugAssertFail(const IMG_CHAR* pszFile, IMG_UINT32 uLine)
{
	PVRSRVDebugPrintf(DBGPRIV_FATAL, pszFile, uLine, "Debug assertion failed!");
	BUG();
}

#endif

#if defined(PVRSRV_NEED_PVR_TRACE)

IMG_VOID PVRSRVTrace(const IMG_CHAR* pszFormat, ...)
{
	va_list VArgs;
	unsigned long ulLockFlags = 0;
	IMG_CHAR *pszBuf;
	IMG_UINT32 ui32BufSiz;

	SelectBuffer(&pszBuf, &ui32BufSiz);

	va_start(VArgs, pszFormat);

	GetBufferLock(&ulLockFlags);

	strncpy(pszBuf, "PVR: ", (ui32BufSiz -1));

	if (VBAppend(pszBuf, ui32BufSiz, pszFormat, VArgs))
	{
		printk(KERN_INFO "PVR_K:(Message Truncated): %s\n", pszBuf);
	}
	else
	{
		printk(KERN_INFO "%s\n", pszBuf);
	}

	ReleaseBufferLock(ulLockFlags);

	va_end(VArgs);
}

#endif

#if defined(PVRSRV_NEED_PVR_DPF)

static IMG_BOOL BAppend(IMG_CHAR *pszBuf, IMG_UINT32 ui32BufSiz, const IMG_CHAR *pszFormat, ...)
{
		va_list VArgs;
		IMG_BOOL bTrunc;

		va_start (VArgs, pszFormat);

		bTrunc = VBAppend(pszBuf, ui32BufSiz, pszFormat, VArgs);

		va_end (VArgs);

		return bTrunc;
}

IMG_VOID PVRSRVDebugPrintf	(
						IMG_UINT32	ui32DebugLevel,
						const IMG_CHAR*	pszFullFileName,
						IMG_UINT32	ui32Line,
						const IMG_CHAR*	pszFormat,
						...
					)
{
	IMG_BOOL bTrace, bDebug;
	const IMG_CHAR *pszFileName = pszFullFileName;
	IMG_CHAR *pszLeafName;

	bTrace = gPVRDebugLevel & ui32DebugLevel & DBGPRIV_CALLTRACE;
	bDebug = ((gPVRDebugLevel & DBGPRIV_ALLLEVELS) >= ui32DebugLevel);

	if (bTrace || bDebug)
	{
		va_list vaArgs;
		unsigned long ulLockFlags = 0;
		IMG_CHAR *pszBuf;
		IMG_UINT32 ui32BufSiz;

		SelectBuffer(&pszBuf, &ui32BufSiz);

		va_start(vaArgs, pszFormat);

		GetBufferLock(&ulLockFlags);


		if (bDebug)
		{
			switch(ui32DebugLevel)
			{
				case DBGPRIV_FATAL:
				{
					strncpy (pszBuf, "PVR_K:(Fatal): ", (ui32BufSiz -1));
					break;
				}
				case DBGPRIV_ERROR:
				{
					strncpy (pszBuf, "PVR_K:(Error): ", (ui32BufSiz -1));
					break;
				}
				case DBGPRIV_WARNING:
				{
					strncpy (pszBuf, "PVR_K:(Warning): ", (ui32BufSiz -1));
					break;
				}
				case DBGPRIV_MESSAGE:
				{
					strncpy (pszBuf, "PVR_K:(Message): ", (ui32BufSiz -1));
					break;
				}
				case DBGPRIV_VERBOSE:
				{
					strncpy (pszBuf, "PVR_K:(Verbose): ", (ui32BufSiz -1));
					break;
				}
				default:
				{
					strncpy (pszBuf, "PVR_K:(Unknown message level)", (ui32BufSiz -1));
					break;
				}
			}
		}
		else
		{
			strncpy (pszBuf, "PVR_K: ", (ui32BufSiz -1));
		}

		if (VBAppend(pszBuf, ui32BufSiz, pszFormat, vaArgs))
		{
			printk(KERN_INFO "PVR_K:(Message Truncated): %s\n", pszBuf);
		}
		else
		{

			if (!bTrace)
			{
#ifdef DEBUG_LOG_PATH_TRUNCATE

				static IMG_CHAR szFileNameRewrite[PVR_MAX_FILEPATH_LEN];

				IMG_CHAR* pszTruncIter;
				IMG_CHAR* pszTruncBackInter;


				pszFileName = pszFullFileName + strlen(DEBUG_LOG_PATH_TRUNCATE)+1;


				strncpy(szFileNameRewrite, pszFileName,PVR_MAX_FILEPATH_LEN);

				if(strlen(szFileNameRewrite) == PVR_MAX_FILEPATH_LEN-1) {
					IMG_CHAR szTruncateMassage[] = "FILENAME TRUNCATED";
					strcpy(szFileNameRewrite + (PVR_MAX_FILEPATH_LEN - 1 - strlen(szTruncateMassage)), szTruncateMassage);
				}

				pszTruncIter = szFileNameRewrite;
				while(*pszTruncIter++ != 0)
				{
					IMG_CHAR* pszNextStartPoint;

					if(
					   !( ( *pszTruncIter == '/' && (pszTruncIter-4 >= szFileNameRewrite) ) &&
						 ( *(pszTruncIter-1) == '.') &&
						 ( *(pszTruncIter-2) == '.') &&
						 ( *(pszTruncIter-3) == '/') )
					   ) continue;


					pszTruncBackInter = pszTruncIter - 3;
					while(*(--pszTruncBackInter) != '/')
					{
						if(pszTruncBackInter <= szFileNameRewrite) break;
					}
					pszNextStartPoint = pszTruncBackInter;


					while(*pszTruncIter != 0)
					{
						*pszTruncBackInter++ = *pszTruncIter++;
					}
					*pszTruncBackInter = 0;


					pszTruncIter = pszNextStartPoint;
				}

				pszFileName = szFileNameRewrite;

				if(*pszFileName == '/') pszFileName++;
#endif

#if !defined(__sh__)
				pszLeafName = (IMG_CHAR *)strrchr (pszFileName, '\\');

				if (pszLeafName)
				{
					pszFileName = pszLeafName;
		       	}
#endif

				if (BAppend(pszBuf, ui32BufSiz, " [%lu, %s]", ui32Line, pszFileName))
				{
					printk(KERN_INFO "PVR_K:(Message Truncated): %s\n", pszBuf);
				}
				else
				{
					printk(KERN_INFO "%s\n", pszBuf);
				}
			}
			else
			{
				printk(KERN_INFO "%s\n", pszBuf);
			}
		}

		ReleaseBufferLock(ulLockFlags);

		va_end (vaArgs);
	}
}

#endif

#if defined(DEBUG)

IMG_VOID PVRDebugSetLevel(IMG_UINT32 uDebugLevel)
{
	printk(KERN_INFO "PVR: Setting Debug Level = 0x%x\n",(IMG_UINT)uDebugLevel);

	gPVRDebugLevel = uDebugLevel;
}

IMG_INT PVRDebugProcSetLevel(struct file *file, const IMG_CHAR *buffer, IMG_UINT32 count, IMG_VOID *data)
{
#define	_PROC_SET_BUFFER_SZ		2
	IMG_CHAR data_buffer[_PROC_SET_BUFFER_SZ];

	if (count != _PROC_SET_BUFFER_SZ)
	{
		return -EINVAL;
	}
	else
	{
		if (copy_from_user(data_buffer, buffer, count))
			return -EINVAL;
		if (data_buffer[count - 1] != '\n')
			return -EINVAL;
		PVRDebugSetLevel(data_buffer[0] - '0');
	}
	return (count);
}

#ifdef PVR_PROC_USE_SEQ_FILE
void ProcSeqShowDebugLevel(struct seq_file *sfile,void* el)
{
	seq_printf(sfile, "%lu\n", gPVRDebugLevel);
}

#else
IMG_INT PVRDebugProcGetLevel(IMG_CHAR *page, IMG_CHAR **start, off_t off, IMG_INT count, IMG_INT *eof, IMG_VOID *data)
{
	if (off == 0) {
		*start = (IMG_CHAR *)1;
		return printAppend(page, count, 0, "%lu\n", gPVRDebugLevel);
	}
	*eof = 1;
	return 0;
}
#endif

#endif
