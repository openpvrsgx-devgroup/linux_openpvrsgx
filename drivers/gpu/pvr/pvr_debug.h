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

#ifndef __PVR_DEBUG_H__
#define __PVR_DEBUG_H__

#include <linux/fs.h>

#include "img_types.h"

#define PVR_MAX_DEBUG_MESSAGE_LEN	512

#define DBGPRIV_FATAL			0x01UL
#define DBGPRIV_ERROR			0x02UL
#define DBGPRIV_WARNING			0x04UL
#define DBGPRIV_MESSAGE			0x08UL
#define DBGPRIV_VERBOSE			0x10UL
#define DBGPRIV_CALLTRACE		0x20UL
#define DBGPRIV_ALLOC			0x40UL
#define DBGPRIV_ALLLEVELS		(DBGPRIV_FATAL | DBGPRIV_ERROR |     \
					 DBGPRIV_WARNING | DBGPRIV_MESSAGE | \
					 DBGPRIV_VERBOSE)

#define PVR_DBG_FATAL			DBGPRIV_FATAL
#define PVR_DBG_ERROR			DBGPRIV_ERROR
#define PVR_DBG_WARNING			DBGPRIV_WARNING
#define PVR_DBG_MESSAGE			DBGPRIV_MESSAGE
#define PVR_DBG_VERBOSE			DBGPRIV_VERBOSE
#define PVR_DBG_CALLTRACE		DBGPRIV_CALLTRACE
#define PVR_DBG_ALLOC			DBGPRIV_ALLOC

#if defined(CONFIG_PVR_DEBUG)

#define PVR_ASSERT(EXPR)						     \
	do {								     \
		if (!(EXPR))						     \
			PVRSRVDebugAssertFail(__FILE__, __LINE__);	     \
	} while (0)

#define PVR_DPF(level, fmt, ...)					     \
	PVRSRVDebugPrintf(level, __FILE__, __LINE__, fmt, ## __VA_ARGS__)

#define PVR_TRACE(fmt, ...)						     \
	PVRSRVTrace(fmt, ##__VA_ARGS__)

void PVRSRVDebugAssertFail(const char *pszFile, u32 ui32Line);
void PVRSRVDebugPrintf(u32 ui32DebugLevel, const char *pszFileName,
		       u32 ui32Line, const char *pszFormat, ...);
void PVRSRVTrace(const char *pszFormat, ...);

int PVRDebugProcSetLevel(struct file *file, const char __user *buffer,
			 unsigned long count, void *data);
int PVRDebugProcGetLevel(char *page, char **start, off_t off, int count,
			 int *eof, void *data);
void PVRDebugSetLevel(u32 uDebugLevel);

void pvr_dbg_init(void);
void pvr_dbg_cleanup(void);

#define PVR_DBG_BREAK

#else

#if defined(TIMING)

#define PVR_ASSERT(EXPR)		do { } while (0)
#define PVR_DPF(level, fmt, ...)	do { } while (0)
#define PVR_TRACE(fmt, ...)	\
	PVRSRVTrace(fmt, ##__VA_ARGS__)
#define PVR_DBG_BREAK			do { } while (0)

void PVRSRVTrace(const char *pszFormat, ...);

#else

#define PVR_ASSERT(EXPR)		do { } while (0)
#define PVR_DPF(level, fmt, ...)	do { } while (0)
#define PVR_TRACE(fmt, ...)		do { } while (0)
#define PVR_DBG_BREAK			do { } while (0)

static inline void pvr_dbg_init(void) {};
static inline void pvr_dbg_cleanup(void) {};

#endif
#endif

#endif
