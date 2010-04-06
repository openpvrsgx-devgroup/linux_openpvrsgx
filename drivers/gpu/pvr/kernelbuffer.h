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

#if !defined(__KERNELBUFFER_H__)
#define __KERNELBUFFER_H__

#include "servicesext.h"

struct PVRSRV_BC_SRV2BUFFER_KMJTABLE {
	u32 ui32TableSize;
	enum PVRSRV_ERROR (*pfnOpenBCDevice)(void **);
	enum PVRSRV_ERROR (*pfnCloseBCDevice)(void *);
	enum PVRSRV_ERROR (*pfnGetBCInfo)(void *, struct BUFFER_INFO *);
	enum PVRSRV_ERROR (*pfnGetBCBuffer)(void *, u32,
				       struct PVRSRV_SYNC_DATA *, void **);
	enum PVRSRV_ERROR (*pfnGetBufferAddr)(void *, void *,
					 struct IMG_SYS_PHYADDR **, u32 *,
					 void __iomem **, void **, IMG_BOOL *);
};

struct PVRSRV_BC_BUFFER2SRV_KMJTABLE {
	u32 ui32TableSize;
	enum PVRSRV_ERROR (*pfnPVRSRVRegisterBCDevice)(
			struct PVRSRV_BC_SRV2BUFFER_KMJTABLE *, u32 *);
	enum PVRSRV_ERROR (*pfnPVRSRVRemoveBCDevice)(u32);
};

IMG_BOOL PVRGetBufferClassJTable(
		struct PVRSRV_BC_BUFFER2SRV_KMJTABLE *psJTable);


#endif
