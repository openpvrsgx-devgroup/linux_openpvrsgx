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
 *****************************************************************************/

#include "ion.h"

#include "services.h"
#include "servicesint.h"
#include "mutex.h"
#include "lock.h"
#include "mm.h"
#include "handle.h"
#include "perproc.h"
#include "env_perproc.h"
#include "private_data.h"
#include "pvr_debug.h"

#include <linux/module.h>
#include <linux/file.h>
#include <linux/fs.h>

#define MAX_HANDLES_PER_FD 2
extern struct ion_client *gpsIONClient;

void PVRSRVExportFDToIONHandles(int fd, struct ion_client **client,
								struct ion_handle **handles,
								unsigned int *num_handles)
{
	PVRSRV_FILE_PRIVATE_DATA *psPrivateData;
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;
	LinuxMemArea *psLinuxMemArea;
	PVRSRV_ERROR eError;
	struct file *psFile;
	int i;
	unsigned int ui32NumHandles = *num_handles;

	/* Take the bridge mutex so the handle won't be freed underneath us */
	LinuxLockMutex(&gPVRSRVLock);

	psFile = fget(fd);
	if(!psFile)
		goto err_unlock;

	psPrivateData = psFile->private_data;
	if(!psPrivateData)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: struct file* has no private_data; "
								"invalid export handle", __func__));
		goto err_fput;
	}

	eError = PVRSRVLookupHandle(KERNEL_HANDLE_BASE,
								(IMG_PVOID *)&psKernelMemInfo,
								psPrivateData->hKernelMemInfo,
								PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to look up MEM_INFO handle",
								__func__));
		goto err_fput;
	}

	psLinuxMemArea = (LinuxMemArea *)psKernelMemInfo->sMemBlk.hOSMemHandle;
	BUG_ON(psLinuxMemArea == IMG_NULL);

	if(psLinuxMemArea->eAreaType != LINUX_MEM_AREA_ION)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Valid handle, but not an ION buffer",
								__func__));
		goto err_fput;
	}

	for (i = 0; (i < ui32NumHandles) && (i < MAX_HANDLES_PER_FD); i++)
		handles[i] = psLinuxMemArea->uData.sIONTilerAlloc.psIONHandle[i];

	*num_handles = i;

	if(client)
		*client = gpsIONClient;

err_fput:
	fput(psFile);
err_unlock:
	/* Allow PVRSRV clients to communicate with srvkm again */
	LinuxUnLockMutex(&gPVRSRVLock);
}

struct ion_handle *
PVRSRVExportFDToIONHandle(int fd, struct ion_client **client)
{
	unsigned int num_handles = 1;
	struct ion_handle *psHandle = IMG_NULL;
	PVRSRVExportFDToIONHandles(fd, client, &psHandle, &num_handles);
	return psHandle;
}

EXPORT_SYMBOL(PVRSRVExportFDToIONHandles);
EXPORT_SYMBOL(PVRSRVExportFDToIONHandle);
