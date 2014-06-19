/*
 * Copyright (C) 2011 Texas Instruments, Inc
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef GC_BVMAPPING_H
#define GC_BVMAPPING_H

#include "services_headers.h"

void gc_bvunmap_meminfo(PVRSRV_KERNEL_MEM_INFO *psMemInfo);

void gc_bvmap_meminfo(PVRSRV_KERNEL_MEM_INFO *psMemInfo);

IMG_VOID *gc_meminfo_to_hndl(PVRSRV_KERNEL_MEM_INFO *psMemInfo);

#endif
