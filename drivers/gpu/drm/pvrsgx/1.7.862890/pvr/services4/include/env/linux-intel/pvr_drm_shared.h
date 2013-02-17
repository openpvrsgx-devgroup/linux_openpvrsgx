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

#if !defined(__PVR_DRM_SHARED_H__)
#define __PVR_DRM_SHARED_H__

#define PVR_DRM_SRVKM_CMD	0x12
#define	PVR_DRM_DISP_CMD	0x13
#define	PVR_DRM_BC_CMD		0x14
#define PVR_DRM_IS_MASTER_CMD	0x15
#define PVR_DRM_UNPRIV_CMD	0x16
#define PVR_DRM_DBGDRV_CMD	0x1E

#define	PVR_DRM_UNPRIV_INIT_SUCCESFUL	0
#define	PVR_DRM_UNPRIV_BUSID_TYPE	1
#define	PVR_DRM_UNPRIV_BUSID_FIELD	2

#define	PVR_DRM_BUS_TYPE_PCI		0

#define	PVR_DRM_PCI_DOMAIN		0
#define	PVR_DRM_PCI_BUS			1
#define	PVR_DRM_PCI_DEV			2
#define	PVR_DRM_PCI_FUNC		3

#endif


