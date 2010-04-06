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

#if !defined(__KERNELDISPLAY_H__)
#define __KERNELDISPLAY_H__

#include <linux/module.h>

#define DC_FLIP_COMMAND				0

#define DC_STATE_NO_FLUSH_COMMANDS		0
#define DC_STATE_FLUSH_COMMANDS			1

struct PVRSRV_DC_SRV2DISP_KMJTABLE {
	struct module *owner;
	u32 ui32TableSize;
	enum PVRSRV_ERROR (*pfnOpenDCDevice)(u32, void **,
					struct PVRSRV_SYNC_DATA *);
	enum PVRSRV_ERROR (*pfnCloseDCDevice)(void *);
	enum PVRSRV_ERROR (*pfnEnumDCFormats)(void *, u32 *,
					 struct DISPLAY_FORMAT *);
	enum PVRSRV_ERROR (*pfnEnumDCDims)(void *, struct DISPLAY_FORMAT *,
				      u32 *, struct DISPLAY_DIMS *);
	enum PVRSRV_ERROR (*pfnGetDCSystemBuffer)(void *, void **);
	enum PVRSRV_ERROR (*pfnGetDCInfo)(void *, struct DISPLAY_INFO *);
	enum PVRSRV_ERROR (*pfnGetBufferAddr)(void *, void *,
					 struct IMG_SYS_PHYADDR **, u32 *,
					 void __iomem **, void **, IMG_BOOL *);
	enum PVRSRV_ERROR (*pfnCreateDCSwapChain)(void *, u32,
					     struct DISPLAY_SURF_ATTRIBUTES *,
					     struct DISPLAY_SURF_ATTRIBUTES *,
					     u32, struct PVRSRV_SYNC_DATA **,
					     u32, void **, u32 *);
	enum PVRSRV_ERROR (*pfnDestroyDCSwapChain)(void *, void *);
	enum PVRSRV_ERROR (*pfnSetDCDstRect)(void *, void *, struct IMG_RECT *);
	enum PVRSRV_ERROR (*pfnSetDCSrcRect)(void *, void *, struct IMG_RECT *);
	enum PVRSRV_ERROR (*pfnSetDCDstColourKey)(void *, void *, u32);
	enum PVRSRV_ERROR (*pfnSetDCSrcColourKey)(void *, void *, u32);
	enum PVRSRV_ERROR (*pfnGetDCBuffers)(void *, void *, u32 *, void **);
	void (*pfnSetDCState)(void *, u32);
};

struct PVRSRV_DC_DISP2SRV_KMJTABLE {
	u32 ui32TableSize;
	enum PVRSRV_ERROR (*pfnPVRSRVRegisterDCDevice)(
				struct PVRSRV_DC_SRV2DISP_KMJTABLE*, u32 *);
	enum PVRSRV_ERROR (*pfnPVRSRVRemoveDCDevice)(u32);
	enum PVRSRV_ERROR (*pfnPVRSRVOEMFunction)(u32, void *, u32, void *,
				u32);
	enum PVRSRV_ERROR (*pfnPVRSRVRegisterCmdProcList)(u32,
				IMG_BOOL (**)(void *, u32, void *), u32[][2],
				u32);
	enum PVRSRV_ERROR (*pfnPVRSRVRemoveCmdProcList)(u32, u32);
	void (*pfnPVRSRVCmdComplete)(void *, IMG_BOOL);
	enum PVRSRV_ERROR (*pfnPVRSRVRegisterSystemISRHandler)(
				IMG_BOOL (*)(void *), void *, u32, u32);
	enum PVRSRV_ERROR (*pfnPVRSRVRegisterPowerDevice)(u32,
			enum PVRSRV_ERROR (*)(void *, enum PVR_POWER_STATE,
					 enum PVR_POWER_STATE),
			enum PVRSRV_ERROR (*)(void *, enum PVR_POWER_STATE,
					 enum PVR_POWER_STATE),
			enum PVRSRV_ERROR (*)(void *, IMG_BOOL,
					 enum PVR_POWER_STATE),
			enum PVRSRV_ERROR (*)(void *, IMG_BOOL,
					 enum PVR_POWER_STATE),
			void *, enum PVR_POWER_STATE, enum PVR_POWER_STATE);
};

struct DISPLAYCLASS_FLIP_COMMAND {
	void *hExtDevice;
	void *hExtSwapChain;
	void *hExtBuffer;
	void *hPrivateTag;
	u32 ui32ClipRectCount;
	struct IMG_RECT *psClipRect;
	u32 ui32SwapInterval;
};

IMG_BOOL PVRGetDisplayClassJTable(struct PVRSRV_DC_DISP2SRV_KMJTABLE *psJTable);

#endif
