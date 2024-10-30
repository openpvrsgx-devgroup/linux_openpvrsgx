/*
 * Copyright (c) 2011, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "psb_pvr_glue.h"

/**
 * FIXME: should NOT use these file under env/linux directly
 */
#include "mm.h"

int psb_get_meminfo_by_handle(IMG_HANDLE hKernelMemInfo,
				PVRSRV_KERNEL_MEM_INFO **ppsKernelMemInfo)
{
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo = IMG_NULL;
	PVRSRV_PER_PROCESS_DATA *psPerProc = IMG_NULL;
	PVRSRV_ERROR eError;

	psPerProc = PVRSRVPerProcessData(OSGetCurrentProcessIDKM());
	eError = PVRSRVLookupHandle(psPerProc->psHandleBase,
				    (IMG_VOID *)&psKernelMemInfo,
				    hKernelMemInfo,
				    PVRSRV_HANDLE_TYPE_MEM_INFO);
	if (eError != PVRSRV_OK) {
		DRM_ERROR("Cannot find kernel meminfo for handle 0x%x\n",
			  (IMG_UINT32)hKernelMemInfo);
		return -EINVAL;
	}

	*ppsKernelMemInfo = psKernelMemInfo;

	DRM_DEBUG_KMS("Got Kernel MemInfo for handle 0x%x\n",
		  (IMG_UINT32)hKernelMemInfo);
	return 0;
}

IMG_UINT32 psb_get_tgid(void)
{
	return OSGetCurrentProcessIDKM();
}

int psb_get_pages_by_mem_handle(IMG_HANDLE hOSMemHandle, struct page ***pages)
{
	LinuxMemArea *psLinuxMemArea = (LinuxMemArea *)hOSMemHandle;
	struct page **page_list;

	if (psLinuxMemArea->eAreaType != LINUX_MEM_AREA_ALLOC_PAGES) {
		DRM_ERROR("MemArea type is not LINUX_MEM_AREA_ALLOC_PAGES\n");
		return -EINVAL;
	}

	page_list = psLinuxMemArea->uData.sPageList.pvPageList;
	if (!page_list) {
		DRM_DEBUG_KMS("Page List is NULL\n");
		return -ENOMEM;
	}

	*pages = page_list;
	return 0;
}
