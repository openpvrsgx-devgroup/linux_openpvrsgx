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

#ifndef __PERPROC_H__
#define __PERPROC_H__

#include "img_types.h"
#include "resman.h"

#include "handle.h"

struct PVRSRV_PER_PROCESS_DATA {
	u32 ui32PID;
	void *hBlockAlloc;
	struct RESMAN_CONTEXT *hResManContext;
	void *hPerProcData;
	struct PVRSRV_HANDLE_BASE *psHandleBase;

	IMG_BOOL bHandlesBatched;
	u32 ui32RefCount;

	IMG_BOOL bInitProcess;

	void *hOsPrivateData;
};

struct PVRSRV_PER_PROCESS_DATA *PVRSRVPerProcessData(u32 ui32PID);

enum PVRSRV_ERROR PVRSRVPerProcessDataConnect(u32 ui32PID);
void PVRSRVPerProcessDataDisconnect(u32 ui32PID);

enum PVRSRV_ERROR PVRSRVPerProcessDataInit(void);
enum PVRSRV_ERROR PVRSRVPerProcessDataDeInit(void);

static inline struct PVRSRV_PER_PROCESS_DATA *PVRSRVFindPerProcessData(void)
{
	return PVRSRVPerProcessData(OSGetCurrentProcessIDKM());
}

static inline void *PVRSRVProcessPrivateData(struct PVRSRV_PER_PROCESS_DATA
					     *psPerProc)
{
	return (psPerProc != NULL) ? psPerProc->hOsPrivateData : NULL;
}

static inline void *PVRSRVPerProcessPrivateData(u32 ui32PID)
{
	return PVRSRVProcessPrivateData(PVRSRVPerProcessData(ui32PID));
}

static inline void *PVRSRVFindPerProcessPrivateData(void)
{
	return PVRSRVProcessPrivateData(PVRSRVFindPerProcessData());
}

#endif
