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

#ifndef _IOCTL_
#define _IOCTL_

u32 DBGDIOCDrivCreateStream(void *, void *);
u32 DBGDIOCDrivDestroyStream(void *, void *);
u32 DBGDIOCDrivGetStream(void *, void *);
u32 DBGDIOCDrivWriteString(void *, void *);
u32 DBGDIOCDrivReadString(void *, void *);
u32 DBGDIOCDrivWrite(void *, void *);
u32 DBGDIOCDrivWrite2(void *, void *);
u32 DBGDIOCDrivRead(void *, void *);
u32 DBGDIOCDrivSetCaptureMode(void *, void *);
u32 DBGDIOCDrivSetOutMode(void *, void *);
u32 DBGDIOCDrivSetDebugLevel(void *, void *);
u32 DBGDIOCDrivSetFrame(void *, void *);
u32 DBGDIOCDrivGetFrame(void *, void *);
u32 DBGDIOCDrivOverrideMode(void *, void *);
u32 DBGDIOCDrivDefaultMode(void *, void *);
u32 DBGDIOCDrivGetServiceTable(void *, void *);
u32 DBGDIOCDrivWriteStringCM(void *, void *);
u32 DBGDIOCDrivWriteCM(void *, void *);
u32 DBGDIOCDrivSetMarker(void *, void *);
u32 DBGDIOCDrivGetMarker(void *, void *);
u32 DBGDIOCDrivIsCaptureFrame(void *, void *);
u32 DBGDIOCDrivWriteLF(void *, void *);
u32 DBGDIOCDrivReadLF(void *, void *);
u32 DBGDIOCDrivWaitForEvent(void *, void *);

u32(*g_DBGDrivProc[])(void *, void *) = {
DBGDIOCDrivCreateStream,
	    DBGDIOCDrivDestroyStream,
	    DBGDIOCDrivGetStream,
	    DBGDIOCDrivWriteString,
	    DBGDIOCDrivReadString,
	    DBGDIOCDrivWrite,
	    DBGDIOCDrivRead,
	    DBGDIOCDrivSetCaptureMode,
	    DBGDIOCDrivSetOutMode,
	    DBGDIOCDrivSetDebugLevel,
	    DBGDIOCDrivSetFrame,
	    DBGDIOCDrivGetFrame,
	    DBGDIOCDrivOverrideMode,
	    DBGDIOCDrivDefaultMode,
	    DBGDIOCDrivGetServiceTable,
	    DBGDIOCDrivWrite2,
	    DBGDIOCDrivWriteStringCM,
	    DBGDIOCDrivWriteCM,
	    DBGDIOCDrivSetMarker,
	    DBGDIOCDrivGetMarker,
	    DBGDIOCDrivIsCaptureFrame,
	    DBGDIOCDrivWriteLF, DBGDIOCDrivReadLF, DBGDIOCDrivWaitForEvent};

#define MAX_DBGVXD_W32_API (sizeof(g_DBGDrivProc)/sizeof(u32))

#endif
