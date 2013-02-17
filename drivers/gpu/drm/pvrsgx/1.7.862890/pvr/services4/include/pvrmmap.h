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
 ******************************************************************************/

#ifndef __PVRMMAP_H__
#define __PVRMMAP_H__

#if defined (SUPPORT_SID_INTERFACE)
PVRSRV_ERROR PVRPMapKMem(IMG_HANDLE hModule, IMG_VOID **ppvLinAddr, IMG_VOID *pvLinAddrKM, IMG_SID *phMappingInfo, IMG_SID hMHandle);
#else
PVRSRV_ERROR PVRPMapKMem(IMG_HANDLE hModule, IMG_VOID **ppvLinAddr, IMG_VOID *pvLinAddrKM, IMG_HANDLE *phMappingInfo, IMG_HANDLE hMHandle);
#endif


#if defined (SUPPORT_SID_INTERFACE)
IMG_BOOL PVRUnMapKMem(IMG_HANDLE hModule, IMG_SID hMappingInfo, IMG_SID hMHandle);
#else
IMG_BOOL PVRUnMapKMem(IMG_HANDLE hModule, IMG_HANDLE hMappingInfo, IMG_HANDLE hMHandle);
#endif

#endif 

