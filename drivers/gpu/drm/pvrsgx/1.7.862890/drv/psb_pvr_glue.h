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

#include "psb_drv.h"
#include "services_headers.h"

extern int psb_get_meminfo_by_handle(IMG_HANDLE hKernelMemInfo,
				PVRSRV_KERNEL_MEM_INFO **ppsKernelMemInfo);
extern IMG_UINT32 psb_get_tgid(void);
extern int psb_get_pages_by_mem_handle(IMG_HANDLE hOSMemHandle,
					struct page ***pages);
