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

#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/hardirq.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/tty.h>
#include <linux/debugfs.h>
#include <stdarg.h>
#include "img_types.h"
#include "servicesext.h"
#include "pvr_debug.h"
#include "proc.h"
#include "syscommon.h"
#include "sgxinfokm.h"
#include "sgxutils.h"
#include "pvr_bridge_km.h"

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
			 unsigned long count, void *data)
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

int PVRDebugProcGetLevel(char *page, char **start, off_t off, int count,
			 int *eof, void *data)
{
	if (off == 0) {
		*start = (char *)1;
		return printAppend(page, count, 0, "%u\n", gPVRDebugLevel);
	}
	*eof = 1;
	return 0;
}

#endif

#ifdef CONFIG_DEBUG_FS

static struct dentry *debugfs_dir;
static u32 pvr_reset;

static struct PVRSRV_DEVICE_NODE *get_sgx_node(void)
{
	struct SYS_DATA *sysdata;
	struct PVRSRV_DEVICE_NODE *node;

	if (SysAcquireData(&sysdata) != PVRSRV_OK)
		return NULL;

	for (node = sysdata->psDeviceNodeList; node; node = node->psNext)
		if (node->sDevId.eDeviceType == PVRSRV_DEVICE_TYPE_SGX)
			break;

	return node;
}

static int pvr_dbg_reset(void *data, u64 val)
{
	struct PVRSRV_DEVICE_NODE *node;
	enum PVRSRV_ERROR err;
	int r = 0;

	if (val != 1)
		return 0;

	pvr_lock();

	if (pvr_is_disabled()) {
		r = -ENODEV;
		goto exit;
	}

	node = get_sgx_node();
	if (!node) {
		r =  -ENODEV;
		goto exit;
	}

	err = PVRSRVSetDevicePowerStateKM(node->sDevId.ui32DeviceIndex,
					     PVRSRV_POWER_STATE_D0);
	if (err != PVRSRV_OK) {
		r = -EIO;
		goto exit;
	}

	HWRecoveryResetSGX(node);

	SGXTestActivePowerEvent(node);
exit:
	pvr_unlock();

	return r;
}

static int pvr_dbg_set(void *data, u64 val)
{
	u32 *var = data;

	if (var == &pvr_reset)
		return pvr_dbg_reset(data, val);

	BUG();
}

DEFINE_SIMPLE_ATTRIBUTE(pvr_dbg_fops, NULL, pvr_dbg_set, "%llu\n");

static int pvr_init_debugfs(void)
{
	debugfs_dir = debugfs_create_dir("pvr", NULL);
	if (!debugfs_dir)
		return -ENODEV;

	if (!debugfs_create_file("reset_sgx", S_IWUGO, debugfs_dir, &pvr_reset,
				 &pvr_dbg_fops)) {
		debugfs_remove(debugfs_dir);
		return -ENODEV;
	}

	return 0;
}

static void pvr_cleanup_debugfs(void)
{
	debugfs_remove_recursive(debugfs_dir);
}

#else		/* !CONFIG_DEBUG_FS */

static int pvr_init_debugfs(void)
{
	return 0;
}

static void pvr_cleanup_debugfs(void) { }

#endif

void pvr_dbg_init(void)
{
#if defined(CONFIG_PVR_DEBUG) || defined(TIMING)
	mutex_init(&gsDebugMutexNonIRQ);
#endif
	pvr_init_debugfs();
}

void pvr_dbg_cleanup(void)
{
	pvr_cleanup_debugfs();
}
