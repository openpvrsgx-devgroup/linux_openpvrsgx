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

#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/hardirq.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/seq_file.h>

#include "img_types.h"
#include "servicesext.h"
#include "pvr_debug.h"
#include "proc.h"

#ifdef CONFIG_PVR_DEBUG

u32 gPVRDebugLevel = DBGPRIV_WARNING;

#define	PVR_MAX_MSG_LEN PVR_MAX_DEBUG_MESSAGE_LEN

static char gszBufferNonIRQ[PVR_MAX_MSG_LEN + 1];

static char gszBufferIRQ[PVR_MAX_MSG_LEN + 1];
static struct mutex gsDebugMutexNonIRQ;
static DEFINE_SPINLOCK(gsDebugLockIRQ);
#define	USE_SPIN_LOCK (in_interrupt() || !preemptible())
static inline void GetBufferLock(unsigned long *pulLockFlags)
{
	if (USE_SPIN_LOCK)
		spin_lock_irqsave(&gsDebugLockIRQ, *pulLockFlags);
	else
		mutex_lock(&gsDebugMutexNonIRQ);
}

static inline void ReleaseBufferLock(unsigned long ulLockFlags)
{
	if (USE_SPIN_LOCK)
		spin_unlock_irqrestore(&gsDebugLockIRQ, ulLockFlags);
	else
		mutex_unlock(&gsDebugMutexNonIRQ);
}

static inline void SelectBuffer(char **ppszBuf, u32 *pui32BufSiz)
{
	if (USE_SPIN_LOCK) {
		*ppszBuf = gszBufferIRQ;
		*pui32BufSiz = sizeof(gszBufferIRQ);
	} else {
		*ppszBuf = gszBufferNonIRQ;
		*pui32BufSiz = sizeof(gszBufferNonIRQ);
	}
}

static IMG_BOOL VBAppend(char *pszBuf, u32 ui32BufSiz, const char *pszFormat,
			 va_list VArgs)
{
	u32 ui32Used;
	u32 ui32Space;
	s32 i32Len;
	ui32Used = strlen(pszBuf);
	BUG_ON(ui32Used >= ui32BufSiz);
	ui32Space = ui32BufSiz - ui32Used;
	i32Len = vsnprintf(&pszBuf[ui32Used], ui32Space, pszFormat, VArgs);
	pszBuf[ui32BufSiz - 1] = 0;
	return i32Len < 0 || i32Len >= ui32Space;
}

static IMG_BOOL BAppend(char *pszBuf, u32 ui32BufSiz, const char *pszFormat,
			...)
{
	va_list VArgs;
	IMG_BOOL bTrunc;

	va_start(VArgs, pszFormat);

	bTrunc = VBAppend(pszBuf, ui32BufSiz, pszFormat, VArgs);

	va_end(VArgs);

	return bTrunc;
}

void PVRSRVDebugPrintf(u32 ui32DebugLevel,
		       const char *pszFileName,
		       u32 ui32Line, const char *pszFormat, ...)
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
		unsigned long ulLockFlags = 0;	/* suppress gc warning */
		char *pszBuf;
		u32 ui32BufSiz;

		SelectBuffer(&pszBuf, &ui32BufSiz);
		va_start(vaArgs, pszFormat);

		GetBufferLock(&ulLockFlags);

		if (bDebug) {
			switch (ui32DebugLevel) {
			case DBGPRIV_FATAL:
				strncpy(pszBuf, "PVR_K:(Fatal): ",
					(ui32BufSiz - 1));
				break;
			case DBGPRIV_ERROR:
				strncpy(pszBuf, "PVR_K:(Error): ",
						(ui32BufSiz - 1));
				break;
			case DBGPRIV_WARNING:
				strncpy(pszBuf, "PVR_K:(Warning): ",
						(ui32BufSiz - 1));
				break;
			case DBGPRIV_MESSAGE:
				strncpy(pszBuf, "PVR_K:(Message): ",
						(ui32BufSiz - 1));
				break;
			case DBGPRIV_VERBOSE:
				strncpy(pszBuf, "PVR_K:(Verbose): ",
						(ui32BufSiz - 1));
				break;
			default:
				strncpy(pszBuf,
						"PVR_K:(Unknown message level)",
						(ui32BufSiz - 1));
				break;
			}
		} else {
			strncpy(pszBuf, "PVR_K: ", (ui32BufSiz - 1));
		}

		if (VBAppend(pszBuf, ui32BufSiz, pszFormat, vaArgs)) {
			printk(KERN_INFO "PVR_K:(Message Truncated): %s\n",
			       pszBuf);
		} else {
			if (!bTrace) {
				if (BAppend
				    (pszBuf, ui32BufSiz, " [%lu, %s]", ui32Line,
				     pszFileName))
					printk(KERN_INFO
					      "PVR_K:(Message Truncated): %s\n",
					       pszBuf);
				else
					printk(KERN_INFO "%s\n", pszBuf);
			}
		}

		ReleaseBufferLock(ulLockFlags);
		va_end(vaArgs);
	}
}

void PVRSRVDebugAssertFail(const char *pszFile, u32 uLine)
{
	PVRSRVDebugPrintf(DBGPRIV_FATAL, pszFile, uLine,
			  "Debug assertion failed!");
	BUG();
}

void PVRSRVTrace(const char *pszFormat, ...)
{
	va_list VArgs;
	unsigned long ulLockFlags = 0;		/* suppress gcc warning */
	char *pszBuf;
	u32 ui32BufSiz;

	SelectBuffer(&pszBuf, &ui32BufSiz);

	va_start(VArgs, pszFormat);
	GetBufferLock(&ulLockFlags);
	strncpy(pszBuf, "PVR: ", (ui32BufSiz - 1));
	if (VBAppend(pszBuf, ui32BufSiz, pszFormat, VArgs))
		printk(KERN_INFO "PVR_K:(Message Truncated): %s\n", pszBuf);
	else
		printk(KERN_INFO "%s\n", pszBuf);
	ReleaseBufferLock(ulLockFlags);
	va_end(VArgs);
}

void PVRDebugSetLevel(u32 uDebugLevel)
{
	printk(KERN_INFO "PVR: Setting Debug Level = 0x%x\n",
	       (unsigned)uDebugLevel);

	gPVRDebugLevel = uDebugLevel;
}

int PVRDebugProcSetLevel(struct file *file, const char __user *buffer,
			 size_t count, loff_t *ppos)
{
#define	_PROC_SET_BUFFER_SZ		2
	char data_buffer[_PROC_SET_BUFFER_SZ];

	if (count != _PROC_SET_BUFFER_SZ) {
		return -EINVAL;
	} else {
		if (copy_from_user(data_buffer, buffer, count))
			return -EINVAL;
		if (data_buffer[count - 1] != '\n')
			return -EINVAL;
		PVRDebugSetLevel(data_buffer[0] - '0');
	}
	return count;
}

static int ProcDebugLevelSeqShow(struct seq_file *s, void *v)
{
	seq_printf(s, "%u\n", gPVRDebugLevel);
	return 0;
}

static void *ProcDebugLevelSeqStart(struct seq_file *s, loff_t *pos)
{
	return NULL + (*pos == 0);
}

static void *ProcDebugLevelSeqNext(struct seq_file *s, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void ProcDebugLevelSeqStop(struct seq_file *s, void *v)
{
	/* Nothing to do */
}

struct seq_operations pvr_proc_debug_level_ops = {
	.start = ProcDebugLevelSeqStart,
	.next  = ProcDebugLevelSeqNext,
	.stop  = ProcDebugLevelSeqStop,
	.show  = ProcDebugLevelSeqShow,
};

#endif

void pvr_dbg_init(void)
{
#if defined(CONFIG_PVR_DEBUG) || defined(TIMING)
	mutex_init(&gsDebugMutexNonIRQ);
#endif
}

void pvr_dbg_cleanup(void)
{

}
