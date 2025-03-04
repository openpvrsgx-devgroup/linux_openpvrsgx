/**************************************************************************
 * Copyright (c) 2011, Intel Corporation.
 * All Rights Reserved.
 * Copyright (c) 2008, Tungsten Graphics, Inc. Cedar Park, TX., USA.
 * All Rights Reserved.
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
 **************************************************************************/

#include <drm/drmP.h>
#include <drm/drm.h>
#include "psb_drm.h"
#include "psb_drv.h"
#include "psb_fb.h"
#include "psb_reg.h"
#include "psb_intel_reg.h"
#include "psb_intel_bios.h"
#include "psb_msvdx.h"
#include <drm/drm_pciids.h>
#include "pvr_drm_shared.h"
#include "psb_powermgmt.h"
#include "img_types.h"
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/spinlock.h>
#include <linux/pm_runtime.h>
#include <acpi/video.h>

#include "psb_intel_hdmi.h"

/*IMG headers*/
#include "pvr_drm_shared.h"
#include "img_types.h"
#include "pvr_bridge.h"
#include "linkage.h"

#include "bufferclass_video_linux.h"

int drm_psb_debug = 0;
static int drm_psb_trap_pagefaults;
int drm_msvdx_pmpolicy = PSB_PMPOLICY_POWERDOWN;
int drm_msvdx_delay = 250;
int drm_psb_ospm = 1;
extern bool gbdispstatus;

static int psb_probe(struct pci_dev *pdev, const struct pci_device_id *ent);

MODULE_PARM_DESC(debug, "Enable debug output");
MODULE_PARM_DESC(trap_pagefaults, "Error and reset on MMU pagefaults");
MODULE_PARM_DESC(ospm, "switch for ospm support");
MODULE_PARM_DESC(msvdx_pmpolicy, "msvdx power management policy btw frames");

module_param_named(debug, drm_psb_debug, int, 0600);
module_param_named(trap_pagefaults, drm_psb_trap_pagefaults, int, 0600);
module_param_named(msvdx_pmpolicy, drm_msvdx_pmpolicy, int, 0600);
module_param_named(msvdx_command_delay, drm_msvdx_delay, int, 0600);
module_param_named(ospm, drm_psb_ospm, int, 0600);
#ifndef MODULE
/* Make ospm configurable via cmdline firstly, and others can be enabled if needed. */
static int __init config_ospm(char *arg)
{
	/* ospm turn on/off control can be passed in as a cmdline parameter */
	/* to enable this feature add ospm=1 to cmdline */
	/* to disable this feature add ospm=0 to cmdline */
	if (!arg)
		return -EINVAL;

	if (!strcasecmp(arg, "0"))
		drm_psb_ospm = 0;
	else if (!strcasecmp (arg, "1"))
		drm_psb_ospm = 1;

	return 0;
}
early_param ("ospm", config_ospm);
#endif

static struct pci_device_id pciidlist[] = {
    {0x8086, 0x0be0, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0be1, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0be2, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0be3, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0be4, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0be5, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0be6, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0be7, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0be8, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0be9, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0bea, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0beb, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0bec, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0bed, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0bee, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0x8086, 0x0bef, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0, 0, 0}
};

MODULE_DEVICE_TABLE(pci, pciidlist);
/*
 * Standard IOCTLs.
 */

#define DRM_IOCTL_PSB_KMS_OFF	\
		DRM_IO(DRM_PSB_KMS_OFF + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_KMS_ON	\
		DRM_IO(DRM_PSB_KMS_ON + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_VT_LEAVE	\
		DRM_IO(DRM_PSB_VT_LEAVE + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_VT_ENTER	\
		DRM_IO(DRM_PSB_VT_ENTER + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_EXTENSION	\
		DRM_IOWR(DRM_PSB_EXTENSION + DRM_COMMAND_BASE, \
			 union drm_psb_extension_arg)
#define DRM_IOCTL_PSB_SIZES	\
		DRM_IOR(DRM_PSB_SIZES + DRM_COMMAND_BASE, \
			struct drm_psb_sizes_arg)
#define DRM_IOCTL_PSB_FUSE_REG	\
		DRM_IOWR(DRM_PSB_FUSE_REG + DRM_COMMAND_BASE, uint32_t)
#define DRM_IOCTL_PSB_VBT	\
		DRM_IOWR(DRM_PSB_VBT + DRM_COMMAND_BASE, \
			struct gct_ioctl_arg)
#define DRM_IOCTL_PSB_DC_STATE	\
		DRM_IOW(DRM_PSB_DC_STATE + DRM_COMMAND_BASE, \
			struct drm_psb_dc_state_arg)
#define DRM_IOCTL_PSB_ADB	\
		DRM_IOWR(DRM_PSB_ADB + DRM_COMMAND_BASE, uint32_t)
#define DRM_IOCTL_PSB_MODE_OPERATION	\
		DRM_IOWR(DRM_PSB_MODE_OPERATION + DRM_COMMAND_BASE, \
			 struct drm_psb_mode_operation_arg)
#define DRM_IOCTL_PSB_STOLEN_MEMORY	\
		DRM_IOWR(DRM_PSB_STOLEN_MEMORY + DRM_COMMAND_BASE, \
			 struct drm_psb_stolen_memory_arg)
#define DRM_IOCTL_PSB_REGISTER_RW	\
		DRM_IOWR(DRM_PSB_REGISTER_RW + DRM_COMMAND_BASE, \
			 struct drm_psb_register_rw_arg)
#define DRM_IOCTL_PSB_GTT_MAP	\
		DRM_IOWR(DRM_PSB_GTT_MAP + DRM_COMMAND_BASE, \
			 struct psb_gtt_mapping_arg)
#define DRM_IOCTL_PSB_GTT_UNMAP	\
		DRM_IOW(DRM_PSB_GTT_UNMAP + DRM_COMMAND_BASE, \
			struct psb_gtt_mapping_arg)
#define DRM_IOCTL_PSB_GETPAGEADDRS	\
		DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_GETPAGEADDRS,\
			 struct drm_psb_getpageaddrs_arg)
#define DRM_IOCTL_PSB_HIST_ENABLE	\
		DRM_IOWR(DRM_PSB_HIST_ENABLE + DRM_COMMAND_BASE, \
			 uint32_t)
#define DRM_IOCTL_PSB_HIST_STATUS	\
		DRM_IOWR(DRM_PSB_HIST_STATUS + DRM_COMMAND_BASE, \
			 struct drm_psb_hist_status_arg)
#define DRM_IOCTL_PSB_UPDATE_GUARD	\
		DRM_IOWR(DRM_PSB_UPDATE_GUARD + DRM_COMMAND_BASE, \
			 uint32_t)
#define DRM_IOCTL_PSB_INIT_COMM	\
		DRM_IOWR(DRM_PSB_INIT_COMM + DRM_COMMAND_BASE, \
			 uint32_t)
#define DRM_IOCTL_PSB_DPST	\
		DRM_IOWR(DRM_PSB_DPST + DRM_COMMAND_BASE, \
			 uint32_t)
#define DRM_IOCTL_PSB_GAMMA	\
		DRM_IOWR(DRM_PSB_GAMMA + DRM_COMMAND_BASE, \
			 struct drm_psb_dpst_lut_arg)
#define DRM_IOCTL_PSB_DPST_BL	\
		DRM_IOWR(DRM_PSB_DPST_BL + DRM_COMMAND_BASE, \
			 uint32_t)
#define DRM_IOCTL_PSB_GET_PIPE_FROM_CRTC_ID	\
		DRM_IOWR(DRM_PSB_GET_PIPE_FROM_CRTC_ID + DRM_COMMAND_BASE, \
			 struct drm_psb_get_pipe_from_crtc_id_arg)

/*pvr ioctls*/
#define PVR_DRM_SRVKM_IOCTL \
	DRM_IOW(DRM_COMMAND_BASE + PVR_DRM_SRVKM_CMD, \
		PVRSRV_BRIDGE_PACKAGE)
#define PVR_DRM_DISP_IOCTL \
	DRM_IO(DRM_COMMAND_BASE + PVR_DRM_DISP_CMD)
#define PVR_DRM_BC_IOCTL \
	DRM_IO(DRM_COMMAND_BASE + PVR_DRM_BC_CMD)
#define PVR_DRM_IS_MASTER_IOCTL \
	DRM_IO(DRM_COMMAND_BASE + PVR_DRM_IS_MASTER_CMD)
#define PVR_DRM_UNPRIV_IOCTL \
	DRM_IOWR(DRM_COMMAND_BASE + PVR_DRM_UNPRIV_CMD, \
		IMG_UINT32)
#if defined(PDUMP)
  #define PVR_DRM_DBGDRV_IOCTL \
	DRM_IOW(DRM_COMMAND_BASE + PVR_DRM_DBGDRV_CMD, IOCTL_PACKAGE)
#endif

/*DPU/DSR stuff*/
#define DRM_IOCRL_PSB_DPU_QUERY DRM_IOR(DRM_PSB_DPU_QUERY + DRM_COMMAND_BASE, IMG_UINT32)
#define DRM_IOCRL_PSB_DPU_DSR_ON DRM_IOW(DRM_PSB_DPU_DSR_ON + DRM_COMMAND_BASE, IMG_UINT32)
//#define DRM_IOCRL_PSB_DPU_DSR_OFF DRM_IOW(DRM_PSB_DPU_DSR_OFF + DRM_COMMAND_BASE, IMG_UINT32)
#define DRM_IOCRL_PSB_DPU_DSR_OFF DRM_IOW(DRM_PSB_DPU_DSR_OFF + DRM_COMMAND_BASE, struct drm_psb_drv_dsr_off_arg)

/*
 * TTM execbuf extension.
 */
#if defined(PDUMP)
#define DRM_PSB_CMDBUF		  (PVR_DRM_DBGDRV_CMD + 1)
#else
#define DRM_PSB_CMDBUF		  (DRM_PSB_DPU_DSR_OFF + 1)
/* #define DRM_PSB_CMDBUF		  (DRM_PSB_DPST_BL + 1) */
#endif

#define DRM_PSB_SCENE_UNREF	  (DRM_PSB_CMDBUF + 1)
#define DRM_IOCTL_PSB_CMDBUF	\
		DRM_IOW(DRM_PSB_CMDBUF + DRM_COMMAND_BASE,	\
			struct drm_psb_cmdbuf_arg)
#define DRM_IOCTL_PSB_SCENE_UNREF	\
		DRM_IOW(DRM_PSB_SCENE_UNREF + DRM_COMMAND_BASE, \
			struct drm_psb_scene)
#define DRM_IOCTL_PSB_KMS_OFF	  DRM_IO(DRM_PSB_KMS_OFF + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_KMS_ON	  DRM_IO(DRM_PSB_KMS_ON + DRM_COMMAND_BASE)
#define DRM_IOCTL_PSB_EXTENSION	\
		DRM_IOWR(DRM_PSB_EXTENSION + DRM_COMMAND_BASE, \
			 union drm_psb_extension_arg)
/*
 * TTM placement user extension.
 */

#define DRM_PSB_PLACEMENT_OFFSET   (DRM_PSB_SCENE_UNREF + 1)

#define DRM_PSB_TTM_PL_CREATE	 (TTM_PL_CREATE + DRM_PSB_PLACEMENT_OFFSET)
#define DRM_PSB_TTM_PL_REFERENCE (TTM_PL_REFERENCE + DRM_PSB_PLACEMENT_OFFSET)
#define DRM_PSB_TTM_PL_UNREF	 (TTM_PL_UNREF + DRM_PSB_PLACEMENT_OFFSET)
#define DRM_PSB_TTM_PL_SYNCCPU	 (TTM_PL_SYNCCPU + DRM_PSB_PLACEMENT_OFFSET)
#define DRM_PSB_TTM_PL_WAITIDLE  (TTM_PL_WAITIDLE + DRM_PSB_PLACEMENT_OFFSET)
#define DRM_PSB_TTM_PL_SETSTATUS (TTM_PL_SETSTATUS + DRM_PSB_PLACEMENT_OFFSET)
#define DRM_PSB_TTM_PL_CREATE_UB (TTM_PL_CREATE_UB + DRM_PSB_PLACEMENT_OFFSET)

/*
 * TTM fence extension.
 */

#define DRM_PSB_FENCE_OFFSET	   (DRM_PSB_TTM_PL_CREATE_UB + 1)
#define DRM_PSB_TTM_FENCE_SIGNALED (TTM_FENCE_SIGNALED + DRM_PSB_FENCE_OFFSET)
#define DRM_PSB_TTM_FENCE_FINISH   (TTM_FENCE_FINISH + DRM_PSB_FENCE_OFFSET)
#define DRM_PSB_TTM_FENCE_UNREF    (TTM_FENCE_UNREF + DRM_PSB_FENCE_OFFSET)

#define DRM_PSB_FLIP	   (DRM_PSB_TTM_FENCE_UNREF + 1)	/*20*/
/* PSB video extension */
#define DRM_LNC_VIDEO_GETPARAM		(DRM_PSB_FLIP + 1)

/*BC_VIDEO ioctl*/
#define DRM_BUFFER_CLASS_VIDEO      (DRM_LNC_VIDEO_GETPARAM + 1)    /*0x32*/

#define DRM_IOCTL_PSB_TTM_PL_CREATE    \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_CREATE,\
		 union ttm_pl_create_arg)
#define DRM_IOCTL_PSB_TTM_PL_REFERENCE \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_REFERENCE,\
		 union ttm_pl_reference_arg)
#define DRM_IOCTL_PSB_TTM_PL_UNREF    \
	DRM_IOW(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_UNREF,\
		struct ttm_pl_reference_req)
#define DRM_IOCTL_PSB_TTM_PL_SYNCCPU	\
	DRM_IOW(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_SYNCCPU,\
		struct ttm_pl_synccpu_arg)
#define DRM_IOCTL_PSB_TTM_PL_WAITIDLE	 \
	DRM_IOW(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_WAITIDLE,\
		struct ttm_pl_waitidle_arg)
#define DRM_IOCTL_PSB_TTM_PL_SETSTATUS \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_SETSTATUS,\
		 union ttm_pl_setstatus_arg)
#define DRM_IOCTL_PSB_TTM_PL_CREATE_UB    \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_TTM_PL_CREATE_UB,\
		 union ttm_pl_create_ub_arg)
#define DRM_IOCTL_PSB_TTM_FENCE_SIGNALED \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_TTM_FENCE_SIGNALED,	\
		  union ttm_fence_signaled_arg)
#define DRM_IOCTL_PSB_TTM_FENCE_FINISH \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_TTM_FENCE_FINISH,	\
		 union ttm_fence_finish_arg)
#define DRM_IOCTL_PSB_TTM_FENCE_UNREF \
	DRM_IOW(DRM_COMMAND_BASE + DRM_PSB_TTM_FENCE_UNREF,	\
		 struct ttm_fence_unref_arg)
#define DRM_IOCTL_PSB_FLIP \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_PSB_FLIP, \
		 struct drm_psb_pageflip_arg)
#define DRM_IOCTL_LNC_VIDEO_GETPARAM \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_LNC_VIDEO_GETPARAM, \
		 struct drm_lnc_video_getparam_arg)

/*bc_video ioctl*/
#define DRM_IOCTL_BUFFER_CLASS_VIDEO \
        DRM_IOWR(DRM_COMMAND_BASE + DRM_BUFFER_CLASS_VIDEO, \
             BC_Video_ioctl_package)

static int psb_vt_leave_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv);
static int psb_vt_enter_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv);
static int psb_sizes_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv);
static int psb_fuse_reg_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv);
static int psb_vbt_ioctl(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
static int psb_dc_state_ioctl(struct drm_device *dev, void * data,
			      struct drm_file *file_priv);
static int psb_adb_ioctl(struct drm_device *dev, void *data,
			 struct drm_file *file_priv);
static int psb_mode_operation_ioctl(struct drm_device *dev, void *data,
				    struct drm_file *file_priv);
static int psb_stolen_memory_ioctl(struct drm_device *dev, void *data,
				   struct drm_file *file_priv);
static int psb_register_rw_ioctl(struct drm_device *dev, void *data,
				 struct drm_file *file_priv);
static int psb_hist_enable_ioctl(struct drm_device *dev, void *data,
				 struct drm_file *file_priv);
static int psb_hist_status_ioctl(struct drm_device *dev, void *data,
				 struct drm_file *file_priv);
static int psb_update_guard_ioctl(struct drm_device *dev, void *data,
			       struct drm_file *file_priv);
static int psb_init_comm_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv);
static int psb_dpst_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file_priv);
static int psb_gamma_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv);
static int psb_dpst_bl_ioctl(struct drm_device *dev, void *data,
			     struct drm_file *file_priv);

static int psb_ioctl_not_support(struct drm_device *dev, void *data,
				 struct drm_file *file_priv)
{
	return -EOPNOTSUPP;
}

#define PSB_IOCTL_DEF(ioctl, func, flags) \
	[DRM_IOCTL_NR(ioctl) - DRM_COMMAND_BASE] = {ioctl, flags, func}

static struct drm_ioctl_desc psb_ioctls[] = {
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_KMS_OFF, psbfb_kms_off_ioctl,
		      DRM_ROOT_ONLY),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_KMS_ON,
			psbfb_kms_on_ioctl,
			DRM_ROOT_ONLY),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_VT_LEAVE, psb_vt_leave_ioctl,
		      DRM_ROOT_ONLY),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_VT_ENTER,
			psb_vt_enter_ioctl,
			DRM_ROOT_ONLY),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_EXTENSION, psb_extension_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_SIZES, psb_sizes_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_FUSE_REG, psb_fuse_reg_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_VBT, psb_vbt_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_DC_STATE, psb_dc_state_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_ADB, psb_adb_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_MODE_OPERATION, psb_mode_operation_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_STOLEN_MEMORY, psb_stolen_memory_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_REGISTER_RW, psb_register_rw_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_GTT_MAP,
			psb_gtt_map_meminfo_ioctl,
			DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_GTT_UNMAP,
			psb_gtt_unmap_meminfo_ioctl,
			DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_GETPAGEADDRS,
			psb_getpageaddrs_ioctl,
			DRM_AUTH),
	PSB_IOCTL_DEF(PVR_DRM_SRVKM_IOCTL, PVRSRV_BridgeDispatchKM, 0),
	PSB_IOCTL_DEF(PVR_DRM_DISP_IOCTL, PVRDRM_Dummy_ioctl, 0),
	PSB_IOCTL_DEF(PVR_DRM_BC_IOCTL, PVRDRM_Dummy_ioctl, 0),
	PSB_IOCTL_DEF(PVR_DRM_IS_MASTER_IOCTL, PVRDRMIsMaster, DRM_MASTER),
	PSB_IOCTL_DEF(PVR_DRM_UNPRIV_IOCTL, PVRDRMUnprivCmd, 0),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_HIST_ENABLE,
			psb_hist_enable_ioctl,
			DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_HIST_STATUS,
			psb_hist_status_ioctl,
			DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_UPDATE_GUARD, psb_update_guard_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_INIT_COMM, psb_init_comm_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_DPST, psb_dpst_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_GAMMA, psb_gamma_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_DPST_BL, psb_dpst_bl_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_GET_PIPE_FROM_CRTC_ID, psb_intel_get_pipe_from_crtc_id, 0),
#if defined(PDUMP)
	PSB_IOCTL_DEF(PVR_DRM_DBGDRV_IOCTL, SYSPVRDBGDrivIoctl, 0),
#endif
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_CMDBUF, psb_cmdbuf_ioctl, DRM_AUTH),
	/*to be removed later*/
	/*PSB_IOCTL_DEF(DRM_IOCTL_PSB_SCENE_UNREF, drm_psb_scene_unref_ioctl,
		      DRM_AUTH),*/

	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_CREATE, psb_pl_create_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_REFERENCE, psb_pl_reference_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_UNREF, psb_pl_unref_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_SYNCCPU, psb_pl_synccpu_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_WAITIDLE, psb_pl_waitidle_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_SETSTATUS, psb_pl_setstatus_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_PL_CREATE_UB, psb_pl_ub_create_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_FENCE_SIGNALED,
		      psb_fence_signaled_ioctl, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_FENCE_FINISH, psb_fence_finish_ioctl,
		      DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_PSB_TTM_FENCE_UNREF, psb_fence_unref_ioctl,
		      DRM_AUTH),
	/*to be removed later */
	/*PSB_IOCTL_DEF(DRM_IOCTL_PSB_FLIP, psb_page_flip, DRM_AUTH),*/
	PSB_IOCTL_DEF(DRM_IOCTL_LNC_VIDEO_GETPARAM,
			lnc_video_getparam, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCTL_BUFFER_CLASS_VIDEO, BC_Video_Bridge, DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCRL_PSB_DPU_QUERY, psb_ioctl_not_support,
	             DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCRL_PSB_DPU_DSR_ON, psb_ioctl_not_support,
	             DRM_AUTH),
	PSB_IOCTL_DEF(DRM_IOCRL_PSB_DPU_DSR_OFF, psb_ioctl_not_support,
	              DRM_AUTH)
};


static void psb_lastclose(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;

	return;

	if (!dev->dev_private)
		return;

	mutex_lock(&dev_priv->cmdbuf_mutex);
	if (dev_priv->context.buffers) {
		vfree(dev_priv->context.buffers);
		dev_priv->context.buffers = NULL;
	}
	mutex_unlock(&dev_priv->cmdbuf_mutex);
}

static void psb_do_takedown(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct ttm_bo_device *bdev = &dev_priv->bdev;


	if (dev_priv->have_mem_mmu) {
		ttm_bo_clean_mm(bdev, DRM_PSB_MEM_MMU);
		dev_priv->have_mem_mmu = 0;
	}

	if (dev_priv->have_tt) {
		ttm_bo_clean_mm(bdev, TTM_PL_TT);
		dev_priv->have_tt = 0;
	}

	if (dev_priv->have_camera) {
		ttm_bo_clean_mm(bdev, TTM_PL_CI);
		dev_priv->have_camera = 0;
	}
	if (dev_priv->have_rar) {
		ttm_bo_clean_mm(bdev, TTM_PL_RAR);
		dev_priv->have_rar = 0;
	}

	psb_msvdx_uninit(dev);
}

static void psb_get_core_freq(struct drm_device *dev)
{
	uint32_t clock;
	struct drm_psb_private *dev_priv = (struct drm_psb_private *) dev->dev_private;

	clock = PSB_RVDC32(INTEL_CDV_DISP_CLK_FREQ);

	switch ((clock >> 4) & 0x0F) {
	case 0:
		dev_priv->core_freq = 160;
		break;
	case 1:
		dev_priv->core_freq = 200;
		break;
	case 2:
		dev_priv->core_freq = 267;
		break;
	case 3:
		dev_priv->core_freq = 320;
		break;
	case 4:
		dev_priv->core_freq = 356;
		break;
	case 5:
		dev_priv->core_freq = 400;
		break;
	default:
		dev_priv->core_freq = 0;
	}
}

#define FB_REG06 0xD0810600
#define FB_TOPAZ_DISABLE BIT0
#define FB_MIPI_DISABLE  BIT11
#define FB_REG09 0xD0810900
#define FB_SKU_MASK  (BIT12|BIT13|BIT14)
#define FB_SKU_SHIFT 12
#define FB_SKU_100 0
#define FB_SKU_100L 1
#define FB_SKU_83 2
#if 1 /* FIXME remove it after PO */
#define FB_GFX_CLK_DIVIDE_MASK	(BIT20|BIT21|BIT22)
#define FB_GFX_CLK_DIVIDE_SHIFT 20
#define FB_VED_CLK_DIVIDE_MASK	(BIT23|BIT24)
#define FB_VED_CLK_DIVIDE_SHIFT 23
#define FB_VEC_CLK_DIVIDE_MASK	(BIT25|BIT26)
#define FB_VEC_CLK_DIVIDE_SHIFT 25
#endif	/* FIXME remove it after PO */

static int psb_do_init(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;
	struct ttm_bo_device *bdev = &dev_priv->bdev;
	struct psb_gtt *pg = dev_priv->pg;

	uint32_t stolen_gtt;
	uint32_t tt_start;
	uint32_t tt_pages;

	int ret = -ENOMEM;


	/*
	 * Initialize sequence numbers for the different command
	 * submission mechanisms.
	 */

	dev_priv->sequence[PSB_ENGINE_2D] = 1;
	dev_priv->sequence[PSB_ENGINE_VIDEO] = 1;

	if (pg->mmu_gatt_start & 0x0FFFFFFF) {
		DRM_ERROR("Gatt must be 256M aligned. This is a bug.\n");
		ret = -EINVAL;
		goto out_err;
	}

	stolen_gtt = (pg->stolen_size >> PAGE_SHIFT) * 4;
	stolen_gtt = (stolen_gtt + PAGE_SIZE - 1) >> PAGE_SHIFT;
	stolen_gtt = (stolen_gtt < pg->gtt_pages) ? stolen_gtt : pg->gtt_pages;

	dev_priv->gatt_free_offset = pg->mmu_gatt_start +
				     (stolen_gtt << PAGE_SHIFT) * 1024;

	if (1 || drm_debug) {
		uint32_t core_id = PSB_RSGX32(PSB_CR_CORE_ID);
		uint32_t core_rev = PSB_RSGX32(PSB_CR_CORE_REVISION);

		DRM_DEBUG_KMS("SGX core id = 0x%08x\n", core_id);
		DRM_DEBUG_KMS("SGX core rev major = 0x%02x, minor = 0x%02x\n",
			 (core_rev & _PSB_CC_REVISION_MAJOR_MASK) >>
			 _PSB_CC_REVISION_MAJOR_SHIFT,
			 (core_rev & _PSB_CC_REVISION_MINOR_MASK) >>
			 _PSB_CC_REVISION_MINOR_SHIFT);
		DRM_DEBUG_KMS("SGX core rev maintenance = 0x%02x, designer = 0x%02x\n",
			  (core_rev & _PSB_CC_REVISION_MAINTENANCE_MASK) >>
			  _PSB_CC_REVISION_MAINTENANCE_SHIFT,
			  (core_rev & _PSB_CC_REVISION_DESIGNER_MASK) >>
			  _PSB_CC_REVISION_DESIGNER_SHIFT);
	}

	spin_lock_init(&dev_priv->irqmask_lock);

	tt_pages = (pg->gatt_pages < PSB_TT_PRIV0_PLIMIT) ?
	    pg->gatt_pages : PSB_TT_PRIV0_PLIMIT;
	tt_start = dev_priv->gatt_free_offset - pg->mmu_gatt_start;
	tt_pages -= tt_start >> PAGE_SHIFT;
	dev_priv->sizes.ta_mem_size = 0;


	/* TT region managed by TTM. */
	if (!ttm_bo_init_mm(bdev, TTM_PL_TT,
			pg->gatt_pages -
			(pg->ci_start >> PAGE_SHIFT) -
			((dev_priv->ci_region_size + dev_priv->rar_region_size)
			 >> PAGE_SHIFT))) {

		dev_priv->have_tt = 1;
		dev_priv->sizes.tt_size =
			(tt_pages << PAGE_SHIFT) / (1024 * 1024) / 2;
	}

	DRM_DEBUG_KMS(" TT region size is %d\n",
			pg->gatt_pages - (pg->ci_start >> PAGE_SHIFT) - ((dev_priv->ci_region_size +
				dev_priv->rar_region_size) >> PAGE_SHIFT));
	if (!ttm_bo_init_mm(bdev,
			DRM_PSB_MEM_MMU,
			PSB_MEM_TT_START >> PAGE_SHIFT)) {
		dev_priv->have_mem_mmu = 1;
		dev_priv->sizes.mmu_size =
			PSB_MEM_TT_START / (1024*1024);
	}


	PSB_DEBUG_INIT("Init MSVDX\n");
	if (psb_msvdx_init(dev)) {
		ret = -EIO;
		DRM_ERROR("Failure in MSVDX Initialization\n");
		goto out_err;
	}

	return 0;
out_err:
	psb_do_takedown(dev);
	return ret;
}

static int psb_driver_unload(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv =
	    (struct drm_psb_private *) dev->dev_private;

	/*Fristly, unload pvr driver*/
	PVRSRVDrmUnload(dev);

	psb_intel_opregion_fini(dev);

		psb_modeset_cleanup(dev);

	if (dev_priv) {

		/* psb_watchdog_takedown(dev_priv); */
		psb_do_takedown(dev);

		if (dev_priv->pf_pd) {
			psb_mmu_free_pagedir(dev_priv->pf_pd);
			dev_priv->pf_pd = NULL;
		}
		if (dev_priv->mmu) {
			struct psb_gtt *pg = dev_priv->pg;

			down_read(&pg->sem);
			psb_mmu_remove_pfn_sequence(
					psb_mmu_get_default_pd
					(dev_priv->mmu),
					pg->mmu_gatt_start,
					pg->vram_stolen_size >> PAGE_SHIFT);
			if (pg->ci_stolen_size != 0)
				psb_mmu_remove_pfn_sequence(
					psb_mmu_get_default_pd
					(dev_priv->mmu),
					pg->ci_start,
					pg->ci_stolen_size >> PAGE_SHIFT);
			if (pg->rar_stolen_size != 0)
				psb_mmu_remove_pfn_sequence(
					psb_mmu_get_default_pd
					(dev_priv->mmu),
					pg->rar_start,
					pg->rar_stolen_size >> PAGE_SHIFT);
			up_read(&pg->sem);
			psb_mmu_driver_takedown(dev_priv->mmu);
			dev_priv->mmu = NULL;
		}
		psb_gtt_takedown(dev_priv->pg, 1);
		if (dev_priv->scratch_page) {
			__free_page(dev_priv->scratch_page);
			dev_priv->scratch_page = NULL;
		}
		if (dev_priv->has_bo_device) {
			ttm_bo_device_release(&dev_priv->bdev);
			dev_priv->has_bo_device = 0;
		}
		if (dev_priv->has_fence_device) {
			ttm_fence_device_release(&dev_priv->fdev);
			dev_priv->has_fence_device = 0;
		}
		if (dev_priv->vdc_reg) {
			iounmap(dev_priv->vdc_reg);
			dev_priv->vdc_reg = NULL;
		}
		if (dev_priv->sgx_reg) {
			iounmap(dev_priv->sgx_reg);
			dev_priv->sgx_reg = NULL;
		}

		if (dev_priv->msvdx_reg) {
			iounmap(dev_priv->msvdx_reg);
			dev_priv->msvdx_reg = NULL;
		}

		if (dev_priv->tdev)
			ttm_object_device_release(&dev_priv->tdev);

		if (dev_priv->has_global)
			psb_ttm_global_release(dev_priv);

		kfree(dev_priv);
		dev->dev_private = NULL;

		psb_intel_destory_bios(dev);
	}

	ospm_power_uninit();

	return 0;
}


static int psb_driver_load(struct drm_device *dev, unsigned long chipset)
{
	struct drm_psb_private *dev_priv;
	struct ttm_bo_device *bdev;
	unsigned long resource_start;
	struct psb_gtt *pg;
	unsigned long irqflags;
	int ret = -ENOMEM;
	uint32_t tt_pages;

	DRM_DEBUG_KMS("psb - %s\n", PSB_PACKAGE_VERSION);

#if defined(MODULE) && defined(CONFIG_NET)
	psb_kobject_uevent_init();
#endif

	ret = SYSPVRInit();
	if (ret)
		return ret;

	if (IS_CDV(dev))
		DRM_DEBUG_KMS("Run drivers on Cedartrail platform!\n");

	dev_priv = kzalloc(sizeof(*dev_priv), GFP_KERNEL);
	if (dev_priv == NULL)
		return -ENOMEM;
	INIT_LIST_HEAD(&dev_priv->video_ctx);
	dev_priv->num_pipe = 2;

	/*init DPST umcomm to NULL*/
	dev_priv->psb_dpst_state = NULL;
	dev_priv->psb_hotplug_state = NULL;

	dev_priv->dev = dev;
	bdev = &dev_priv->bdev;

	ret = psb_ttm_global_init(dev_priv);
	if (unlikely(ret != 0))
		goto out_err;
	dev_priv->has_global = 1;

	dev_priv->tdev = ttm_object_device_init(dev_priv->mem_global_ref.object,
					   	PSB_OBJECT_HASH_ORDER);
	if (unlikely(dev_priv->tdev == NULL))
		goto out_err;

	mutex_init(&dev_priv->temp_mem);
	mutex_init(&dev_priv->cmdbuf_mutex);
	mutex_init(&dev_priv->reset_mutex);
	INIT_LIST_HEAD(&dev_priv->context.validate_list);
	INIT_LIST_HEAD(&dev_priv->context.kern_validate_list);

	spin_lock_init(&dev_priv->reloc_lock);

	DRM_INIT_WAITQUEUE(&dev_priv->rel_mapped_queue);

	dev->dev_private = (void *) dev_priv;
	dev_priv->chipset = chipset;

	PSB_DEBUG_GENERAL("Init watchdog and scheduler\n");
	/* psb_watchdog_init(dev_priv); */
	psb_scheduler_init(dev, &dev_priv->scheduler);


	PSB_DEBUG_INIT("Mapping MMIO\n");
	resource_start = pci_resource_start(dev->pdev, PSB_MMIO_RESOURCE);

	if (IS_MSVDX(dev)) /* Work around for medfield by Li */
		dev_priv->msvdx_reg = ioremap(resource_start + MRST_MSVDX_OFFSET,
					      PSB_MSVDX_SIZE);
	else
		dev_priv->msvdx_reg = ioremap(resource_start + PSB_MSVDX_OFFSET,
					      PSB_MSVDX_SIZE);

	if (!dev_priv->msvdx_reg)
		goto out_err;

	dev_priv->vdc_reg = ioremap(resource_start + PSB_VDC_OFFSET,
				    PSB_VDC_SIZE);
	if (!dev_priv->vdc_reg)
		goto out_err;

	if (IS_MID(dev))
		dev_priv->sgx_reg = ioremap(resource_start + MRST_SGX_OFFSET,
					    PSB_SGX_SIZE);
	else
		dev_priv->sgx_reg = ioremap(resource_start + PSB_SGX_OFFSET,
					    PSB_SGX_SIZE);

	if (!dev_priv->sgx_reg)
		goto out_err;

	psb_get_core_freq(dev);

	psb_intel_opregion_setup(dev);
	psb_intel_init_bios(dev);

	PSB_DEBUG_INIT("Init TTM fence and BO driver\n");

	/* Init OSPM support */
	ospm_power_init(dev);

	ret = psb_ttm_fence_device_init(&dev_priv->fdev);
	if (unlikely(ret != 0))
		goto out_err;

	/* For VXD385 DE2.x firmware support 16bit fence value */
	if(IS_CDV(dev) && IS_FW_UPDATED) {
		DRM_DEBUG_KMS("Setting up the video fences\n");
		dev_priv->fdev.fence_class[PSB_ENGINE_VIDEO].wrap_diff = (1 << 14);
		dev_priv->fdev.fence_class[PSB_ENGINE_VIDEO].flush_diff = ( 1 << 13);
		dev_priv->fdev.fence_class[PSB_ENGINE_VIDEO].sequence_mask = 0x0000ffff;
		dev_priv->sequence[PSB_ENGINE_VIDEO] = 1;
	}
	dev_priv->has_fence_device = 1;
	ret = ttm_bo_device_init(bdev,
				 dev_priv->bo_global_ref.ref.object,
				 &psb_ttm_bo_driver,
				 DRM_PSB_FILE_PAGE_OFFSET, false);
	if (unlikely(ret != 0))
		goto out_err;
	dev_priv->has_bo_device = 1;
	ttm_lock_init(&dev_priv->ttm_lock);

	ret = -ENOMEM;

	dev_priv->scratch_page = alloc_page(GFP_DMA32 | __GFP_ZERO);
	if (!dev_priv->scratch_page)
		goto out_err;

	set_pages_uc(dev_priv->scratch_page, 1);

	dev_priv->pg = psb_gtt_alloc(dev);
	if (!dev_priv->pg)
		goto out_err;

	ret = psb_gtt_init(dev_priv->pg, 0);
	if (ret)
		goto out_err;

	ret = psb_gtt_mm_init(dev_priv->pg);
	if (ret)
		goto out_err;

	dev_priv->mmu = psb_mmu_driver_init((void *)0,
					drm_psb_trap_pagefaults, 0,
					dev_priv);
	if (!dev_priv->mmu)
		goto out_err;

	pg = dev_priv->pg;

	tt_pages = (pg->gatt_pages < PSB_TT_PRIV0_PLIMIT) ?  (pg->gatt_pages) :
	       					             PSB_TT_PRIV0_PLIMIT;

	/* CI/RAR use the lower half of TT. */
	pg->ci_start = ((tt_pages *3)/ 4) << PAGE_SHIFT;
	pg->rar_start = pg->ci_start + pg->ci_stolen_size;


	/*
	 * Make MSVDX/TOPAZ MMU aware of the CI stolen memory area.
	 */
	if (dev_priv->pg->ci_stolen_size != 0) {
		down_read(&pg->sem);
		ret = psb_mmu_insert_pfn_sequence(psb_mmu_get_default_pd
				(dev_priv->mmu),
				dev_priv->ci_region_start >> PAGE_SHIFT,
				pg->mmu_gatt_start + pg->ci_start,
				pg->ci_stolen_size >> PAGE_SHIFT, 0);
		up_read(&pg->sem);
		if (ret)
			goto out_err;
	}

	/*
	 * Make MSVDX/TOPAZ MMU aware of the rar stolen memory area.
	 */
	if (dev_priv->pg->rar_stolen_size != 0) {
		down_read(&pg->sem);
		ret = psb_mmu_insert_pfn_sequence(
				psb_mmu_get_default_pd(dev_priv->mmu),
				dev_priv->rar_region_start >> PAGE_SHIFT,
				pg->mmu_gatt_start + pg->rar_start,
				pg->rar_stolen_size >> PAGE_SHIFT, 0);
		up_read(&pg->sem);
		if (ret)
			goto out_err;
	}

	dev_priv->pf_pd = psb_mmu_alloc_pd(dev_priv->mmu, 1, 0);
	if (!dev_priv->pf_pd)
		goto out_err;

	psb_mmu_set_pd_context(psb_mmu_get_default_pd(dev_priv->mmu), 0);
	psb_mmu_set_pd_context(dev_priv->pf_pd, 1);

	spin_lock_init(&dev_priv->sequence_lock);

	PSB_DEBUG_INIT("Begin to init MSVDX/Topaz\n");

	ret = psb_do_init(dev);
	if (ret)
		return ret;

	/*initialize the MSI*/
	if (IS_MID(dev)) {
		if (pci_enable_msi(dev->pdev)) {
			DRM_ERROR("Enable MSI failed!\n");
		} else {
			PSB_DEBUG_INIT("Enabled MSI IRQ (%d)\n",
				       dev->pdev->irq);
			/* pci_write_config_word(pdev, 0x04, 0x07); */
		}
	}

	ret = drm_vblank_init(dev, dev_priv->num_pipe);
	if (ret)
	    goto out_err;

	/*
	 * Install interrupt handlers prior to powering off SGX or else we will
	 * crash.
	 */
	dev_priv->vdc_irq_mask = 0;
	dev_priv->pipestat[0] = 0;
	dev_priv->pipestat[1] = 0;
	dev_priv->pipestat[2] = 0;
	spin_lock_irqsave(&dev_priv->irqmask_lock, irqflags);
	PSB_WVDC32(0x00000000, PSB_INT_ENABLE_R);
	PSB_WVDC32(0xFFFFFFFF, PSB_INT_MASK_R);
	spin_unlock_irqrestore(&dev_priv->irqmask_lock, irqflags);
	if (drm_core_check_feature(dev, DRIVER_MODESET))
		drm_irq_install(dev);

	dev->vblank_disable_allowed = 1;

	dev->max_vblank_count = 0xffffff; /* only 24 bits of frame count */

	dev->driver->get_vblank_counter = psb_get_vblank_counter;

	psb_intel_opregion_init(dev);
	acpi_video_register();

	{
		psb_modeset_init(dev);
		psb_fbdev_init(dev);
		drm_kms_helper_poll_init(dev);
	}

	/* initialize HDMI Hotplug interrupt forwarding
	* notifications for user mode
	*/

	/*Intel drm driver load is done, continue doing pvr load*/
	DRM_DEBUG_KMS("Pvr driver load\n");

	ret = PVRSRVDrmLoad(dev, chipset);
	if (ret)
		return ret;

	/*init for bc_video*/
	return BC_Video_ModInit();

out_err:
	psb_driver_unload(dev);
	return ret;
}

int psb_driver_device_is_agp(struct drm_device *dev)
{
	return 0;
}

int psb_extension_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	union drm_psb_extension_arg *arg = data;
	struct drm_psb_extension_rep *rep = &arg->rep;

	if (strcmp(arg->extension, "psb_ttm_placement_alphadrop") == 0) {
		rep->exists = 1;
		rep->driver_ioctl_offset = DRM_PSB_PLACEMENT_OFFSET;
		rep->sarea_offset = 0;
		rep->major = 1;
		rep->minor = 0;
		rep->pl = 0;
		return 0;
	}
	if (strcmp(arg->extension, "psb_ttm_fence_alphadrop") == 0) {
		rep->exists = 1;
		rep->driver_ioctl_offset = DRM_PSB_FENCE_OFFSET;
		rep->sarea_offset = 0;
		rep->major = 1;
		rep->minor = 0;
		rep->pl = 0;
		return 0;
	}
	if (strcmp(arg->extension, "psb_ttm_execbuf_alphadrop") == 0) {
		rep->exists = 1;
		rep->driver_ioctl_offset = DRM_PSB_CMDBUF;
		rep->sarea_offset = 0;
		rep->major = 1;
		rep->minor = 0;
		rep->pl = 0;
		return 0;
	}

	/*return the page flipping ioctl offset*/
	if (strcmp(arg->extension, "psb_page_flipping_alphadrop") == 0) {
		rep->exists = 1;
		rep->driver_ioctl_offset = DRM_PSB_FLIP;
		rep->sarea_offset = 0;
		rep->major = 1;
		rep->minor = 0;
		rep->pl = 0;
		return 0;
	}

	/* return the video rar offset */
	if (strcmp(arg->extension, "lnc_video_getparam") == 0) {
		rep->exists = 1;
		rep->driver_ioctl_offset = DRM_LNC_VIDEO_GETPARAM;
		rep->sarea_offset = 0;
		rep->major = 1;
		rep->minor = 0;
		rep->pl = 0;
		return 0;
	}

	rep->exists = 0;
	return 0;
}

static int psb_vt_leave_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct ttm_bo_device *bdev = &dev_priv->bdev;
	int ret;

	ret = ttm_vt_lock(&dev_priv->ttm_lock, 1,
			     psb_fpriv(file_priv)->tfile);
	if (unlikely(ret != 0))
		return ret;

	ret = ttm_bo_evict_mm(&dev_priv->bdev, TTM_PL_TT);
	if (unlikely(ret != 0))
		goto out_unlock;

	ret = ttm_bo_clean_mm(bdev, TTM_PL_TT);
	if (unlikely(ret != 0))
		DRM_INFO("Warning: GATT was not clean after VT switch.\n");

	ttm_bo_swapout_all(&dev_priv->bdev);

	return 0;
out_unlock:
	(void) ttm_vt_unlock(&dev_priv->ttm_lock);
	return ret;
}

static int psb_vt_enter_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	return ttm_vt_unlock(&dev_priv->ttm_lock);
}

static int psb_sizes_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct drm_psb_sizes_arg *arg =
		(struct drm_psb_sizes_arg *) data;

	*arg = dev_priv->sizes;
	return 0;
}

static int psb_fuse_reg_ioctl(struct drm_device *dev, void *data,
			      struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	uint32_t *arg = data;

	*arg = dev_priv->fuse_reg_value;
	return 0;
}
static int psb_vbt_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct gct_ioctl_arg *pGCT = data;

	memcpy(pGCT, &dev_priv->gct_data, sizeof(*pGCT));

	return 0;
}

static int psb_dc_state_ioctl(struct drm_device *dev, void * data,
				struct drm_file *file_priv)
{
	uint32_t flags;
	uint32_t obj_id;
	struct drm_mode_object *obj;
	struct drm_connector *connector;
	struct drm_crtc *crtc;
	struct drm_psb_dc_state_arg *arg =
		(struct drm_psb_dc_state_arg *)data;

	if (IS_MID(dev))
		return 0;

	flags = arg->flags;
	obj_id = arg->obj_id;

	if (flags & PSB_DC_CRTC_MASK) {
		obj = drm_mode_object_find(dev, obj_id,
				DRM_MODE_OBJECT_CRTC);
		if (!obj) {
			DRM_DEBUG_KMS("Invalid CRTC object.\n");
			return -EINVAL;
		}

		crtc = obj_to_crtc(obj);

		mutex_lock(&dev->mode_config.mutex);
		if (drm_helper_crtc_in_use(crtc)) {
			if (flags & PSB_DC_CRTC_SAVE)
				crtc->funcs->save(crtc);
			else
				crtc->funcs->restore(crtc);
		}
		mutex_unlock(&dev->mode_config.mutex);

		return 0;
	} else if (flags & PSB_DC_OUTPUT_MASK) {
		obj = drm_mode_object_find(dev, obj_id,
				DRM_MODE_OBJECT_CONNECTOR);
		if (!obj) {
			DRM_DEBUG_KMS("Invalid connector id.\n");
			return -EINVAL;
		}

		connector = obj_to_connector(obj);
		if (flags & PSB_DC_OUTPUT_SAVE)
			connector->funcs->save(connector);
		else
			connector->funcs->restore(connector);

		return 0;
	}

	DRM_DEBUG_KMS("Bad flags 0x%x\n", flags);
	return -EINVAL;
}

static int psb_dpst_bl_ioctl(struct drm_device *dev, void *data,
		       struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	uint32_t *arg = data;
	struct backlight_device bd;
	dev_priv->blc_adj2 = *arg;

#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
	bd.props.brightness = psb_get_brightness(&bd);
	psb_set_brightness(&bd);
#endif
	return 0;
}

static int psb_adb_ioctl(struct drm_device *dev, void *data,
			struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	uint32_t *arg = data;
	struct backlight_device bd;
	dev_priv->blc_adj1 = *arg;

#ifdef CONFIG_BACKLIGHT_CLASS_DEVICE
	bd.props.brightness = psb_get_brightness(&bd);
	psb_set_brightness(&bd);
#endif
	return 0;
}

static int psb_hist_enable_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv)
{
	u32 irqCtrl = 0;
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct dpst_guardband guardband_reg;
	struct dpst_ie_histogram_control ie_hist_cont_reg;
	uint32_t *enable = data;

	if (*enable == 1) {
		ie_hist_cont_reg.data = PSB_RVDC32(HISTOGRAM_LOGIC_CONTROL);
		ie_hist_cont_reg.ie_pipe_assignment = 0;
		ie_hist_cont_reg.histogram_mode_select = DPST_YUV_LUMA_MODE;
		ie_hist_cont_reg.ie_histogram_enable = 1;
		PSB_WVDC32(ie_hist_cont_reg.data, HISTOGRAM_LOGIC_CONTROL);

		guardband_reg.data = PSB_RVDC32(HISTOGRAM_INT_CONTROL);
		guardband_reg.interrupt_enable = 1;
		guardband_reg.interrupt_status = 1;
		PSB_WVDC32(guardband_reg.data, HISTOGRAM_INT_CONTROL);

		irqCtrl = PSB_RVDC32(PIPEASTAT);
		PSB_WVDC32(irqCtrl | PIPE_DPST_EVENT_ENABLE, PIPEASTAT);
		/* Wait for two vblanks */
	} else {
		guardband_reg.data = PSB_RVDC32(HISTOGRAM_INT_CONTROL);
		guardband_reg.interrupt_enable = 0;
		guardband_reg.interrupt_status = 1;
		PSB_WVDC32(guardband_reg.data, HISTOGRAM_INT_CONTROL);

		ie_hist_cont_reg.data = PSB_RVDC32(HISTOGRAM_LOGIC_CONTROL);
		ie_hist_cont_reg.ie_histogram_enable = 0;
		PSB_WVDC32(ie_hist_cont_reg.data, HISTOGRAM_LOGIC_CONTROL);

		irqCtrl = PSB_RVDC32(PIPEASTAT);
		irqCtrl &= ~PIPE_DPST_EVENT_ENABLE;
		PSB_WVDC32(irqCtrl, PIPEASTAT);
	}

	return 0;
}

static int psb_hist_status_ioctl(struct drm_device *dev, void *data,
				 struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv		= psb_priv(dev);
	struct drm_psb_hist_status_arg *hist_status	= data;
	uint32_t *arg					= hist_status->buf;
	u32 iedbr_reg_data				= 0;
	struct dpst_ie_histogram_control ie_hist_cont_reg;
	u32 i;
	int dpst3_bin_threshold_count	= 0;
	uint32_t blm_hist_ctl		= HISTOGRAM_LOGIC_CONTROL;
	uint32_t iebdr_reg		= HISTOGRAM_BIN_DATA;
	uint32_t segvalue_max_22_bit	= 0x3fffff;
	uint32_t iedbr_busy_bit		= 0x80000000;
	int dpst3_bin_count		= 32;

	ie_hist_cont_reg.data			= PSB_RVDC32(blm_hist_ctl);
	ie_hist_cont_reg.bin_reg_func_select	= dpst3_bin_threshold_count;
	ie_hist_cont_reg.bin_reg_index		= 0;

	PSB_WVDC32(ie_hist_cont_reg.data, blm_hist_ctl);

	for (i = 0; i < dpst3_bin_count; i++) {
		iedbr_reg_data = PSB_RVDC32(iebdr_reg);

		if (!(iedbr_reg_data & iedbr_busy_bit)) {
			arg[i] = iedbr_reg_data & segvalue_max_22_bit;
		} else {
			i = 0;
			ie_hist_cont_reg.data = PSB_RVDC32(blm_hist_ctl);
			ie_hist_cont_reg.bin_reg_index = 0;
			PSB_WVDC32(ie_hist_cont_reg.data, blm_hist_ctl);
		}
	}

	return 0;
}

static int psb_init_comm_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct pci_dev *pdev = NULL;
	struct device *ddev = NULL;
	struct kobject *kobj = NULL;
	uint32_t *arg = data;

	if (*arg == 1) {
		/*find handle to drm kboject*/
		pdev = dev->pdev;
		ddev = &pdev->dev;
		kobj = &ddev->kobj;

		if (dev_priv->psb_dpst_state == NULL) {
			/*init dpst kmum comms*/
			dev_priv->psb_dpst_state = psb_dpst_init(kobj);
		} else {
			DRM_INFO("DPST already initialized\n");
		}
		if (dev_priv->psb_dpst_state == NULL) {
			DRM_ERROR("DPST is not initialized correctly\n");
			return -EINVAL;
		}
		psb_irq_enable_dpst(dev);
		psb_dpst_notify_change_um(DPST_EVENT_INIT_COMPLETE,
					  dev_priv->psb_dpst_state);
	} else {
		if (dev_priv->psb_dpst_state == NULL) {
			DRM_ERROR("DPST doesn't exit\n");
			return -EINVAL;
		}
		/*hotplug and dpst destroy examples*/
		psb_irq_disable_dpst(dev);
		psb_dpst_notify_change_um(DPST_EVENT_TERMINATE,
					  dev_priv->psb_dpst_state);
		psb_dpst_device_pool_destroy(dev_priv->psb_dpst_state);
		dev_priv->psb_dpst_state = NULL;
	}
	return 0;
}

/* return the current mode to the dpst module */
static int psb_dpst_ioctl(struct drm_device *dev, void *data,
			  struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	uint32_t *arg = data;
	uint32_t x;
	uint32_t y;
	uint32_t reg;

	reg = PSB_RVDC32(PIPEASRC);

	/* horizontal is the left 16 bits */
	x = reg >> 16;
	/* vertical is the right 16 bits */
	y = reg & 0x0000ffff;

	/* the values are the image size minus one */
	x+=1;
	y+=1;

	*arg = (x << 16) | y;

	return 0;
}

static int psb_gamma_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv)
{
	struct drm_psb_dpst_lut_arg *lut_arg = data;
	struct drm_mode_object *obj;
	struct drm_crtc *crtc;
	struct drm_connector *connector;
	struct psb_intel_crtc *psb_intel_crtc;
	int i = 0;
	int32_t obj_id;

	obj_id = lut_arg->output_id;
	obj = drm_mode_object_find(dev, obj_id, DRM_MODE_OBJECT_CONNECTOR);
	if (!obj) {
		DRM_DEBUG_KMS("Invalid Connector object.\n");
		return -EINVAL;
	}

	connector = obj_to_connector(obj);
	crtc = connector->encoder->crtc;
	psb_intel_crtc = to_psb_intel_crtc(crtc);

	for (i = 0; i < 256; i++)
		psb_intel_crtc->lut_adj[i] = lut_arg->lut[i];

	psb_intel_crtc_load_lut(crtc);

	return 0;
}

static int psb_update_guard_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct dpst_guardband* input = (struct dpst_guardband*) data;
	struct dpst_guardband reg_data;

	reg_data.data = PSB_RVDC32(HISTOGRAM_INT_CONTROL);
	reg_data.guardband = input->guardband;
	reg_data.guardband_interrupt_delay = input->guardband_interrupt_delay;
	/* printk(KERN_ALERT "guardband = %u\ninterrupt delay = %u\n", 
		reg_data.guardband, reg_data.guardband_interrupt_delay); */
	PSB_WVDC32(reg_data.data, HISTOGRAM_INT_CONTROL);
	
	return 0;
}

static int psb_mode_operation_ioctl(struct drm_device *dev, void *data,
				struct drm_file *file_priv)
{
	uint32_t obj_id;
	uint16_t op;
	struct drm_mode_modeinfo *umode;
	struct drm_display_mode *mode = NULL;
	struct drm_psb_mode_operation_arg *arg;
	struct drm_mode_object *obj;
	struct drm_connector *connector;
	struct drm_framebuffer * drm_fb;
	struct psb_framebuffer * psb_fb;
	struct drm_connector_helper_funcs *connector_funcs;
	int ret = 0;
	int resp = MODE_OK;

	arg = (struct drm_psb_mode_operation_arg *)data;
        obj_id = arg->obj_id;
	op = arg->operation;

	switch(op) {
	case PSB_MODE_OPERATION_SET_DC_BASE:
		obj = drm_mode_object_find(dev, obj_id, DRM_MODE_OBJECT_FB);
		if(!obj) {
			DRM_ERROR("Invalid FB id %d\n", obj_id);
			return -EINVAL;
		}

		drm_fb = obj_to_fb(obj);
		psb_fb = to_psb_fb(drm_fb);
			
		REG_WRITE(DSPASURF, psb_fb->offset);
		REG_READ(DSPASURF);
		REG_WRITE(DSPBSURF, psb_fb->offset);
		REG_READ(DSPBSURF);
		
		return 0;
	case PSB_MODE_OPERATION_MODE_VALID:
		umode = &arg->mode;

		mutex_lock(&dev->mode_config.mutex);

		obj = drm_mode_object_find(dev, obj_id, DRM_MODE_OBJECT_CONNECTOR);
		if (!obj) {
			ret = -EINVAL;
			goto mode_op_out;
		}

		connector = obj_to_connector(obj);

		mode = drm_mode_create(dev);
		if (!mode) {
			ret = -ENOMEM;
			goto mode_op_out;
		}

		/* drm_crtc_convert_umode(mode, umode); */
		{
			mode->clock = umode->clock;
			mode->hdisplay = umode->hdisplay;
			mode->hsync_start = umode->hsync_start;
			mode->hsync_end = umode->hsync_end;
			mode->htotal = umode->htotal;
			mode->hskew = umode->hskew;
			mode->vdisplay = umode->vdisplay;
			mode->vsync_start = umode->vsync_start;
			mode->vsync_end = umode->vsync_end;
			mode->vtotal = umode->vtotal;
			mode->vscan = umode->vscan;
			mode->vrefresh = umode->vrefresh;
			mode->flags = umode->flags;
			mode->type = umode->type;
			strncpy(mode->name, umode->name, DRM_DISPLAY_MODE_LEN);
			mode->name[DRM_DISPLAY_MODE_LEN-1] = 0;
		}

		connector_funcs = (struct drm_connector_helper_funcs *)
				   connector->helper_private;

		if (connector_funcs->mode_valid) {
			resp = connector_funcs->mode_valid(connector, mode);
			arg->data = (void *)resp;
		}

		/*do some clean up work*/
        	if(mode) {
                	drm_mode_destroy(dev, mode);
        	}
mode_op_out:
        	mutex_unlock(&dev->mode_config.mutex);
        	return ret;

	default:
		DRM_DEBUG_KMS("Unsupported psb mode operation");
		return -EOPNOTSUPP;
	}

	return 0;
}

static int psb_stolen_memory_ioctl(struct drm_device *dev, void *data,
				   struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct drm_psb_stolen_memory_arg *arg = data;

	arg->base = dev_priv->pg->stolen_base;
	arg->size = dev_priv->pg->vram_stolen_size;

	return 0;
}

#define CRTC_PIPEA	(1 << 0)
#define CRTC_PIPEB	(1 << 1)

static int psb_register_rw_ioctl(struct drm_device *dev, void *data,
				 struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = psb_priv(dev);
	struct drm_psb_register_rw_arg *arg = data;
	unsigned int iep_ble_status;
	unsigned long iep_timeout;

	if (arg->display_write_mask != 0) {
		if (arg->display_write_mask & REGRWBITS_PFIT_CONTROLS)
			PSB_WVDC32(arg->display.pfit_controls,
				   PFIT_CONTROL);
		if (arg->display_write_mask & REGRWBITS_PFIT_AUTOSCALE_RATIOS)
			PSB_WVDC32(arg->display.pfit_autoscale_ratios,
				   PFIT_AUTO_RATIOS);
		if (arg->display_write_mask & REGRWBITS_PFIT_PROGRAMMED_SCALE_RATIOS)
			PSB_WVDC32(arg->display.pfit_programmed_scale_ratios,
				   PFIT_PGM_RATIOS);
		if (arg->display_write_mask & REGRWBITS_PIPEASRC)
			PSB_WVDC32(arg->display.pipeasrc, PIPEASRC);
		if (arg->display_write_mask & REGRWBITS_PIPEBSRC)
			PSB_WVDC32(arg->display.pipebsrc, PIPEBSRC);
		if (arg->display_write_mask & REGRWBITS_VTOTAL_A)
			PSB_WVDC32(arg->display.vtotal_a, VTOTAL_A);
		if (arg->display_write_mask & REGRWBITS_VTOTAL_B)
			PSB_WVDC32(arg->display.vtotal_b, VTOTAL_B);
	}

	if (arg->display_read_mask != 0) {
		if (arg->display_read_mask & REGRWBITS_PFIT_CONTROLS)
			arg->display.pfit_controls = PSB_RVDC32(PFIT_CONTROL);
		if (arg->display_read_mask & REGRWBITS_PFIT_AUTOSCALE_RATIOS)
			arg->display.pfit_autoscale_ratios = PSB_RVDC32(PFIT_AUTO_RATIOS);
		if (arg->display_read_mask & REGRWBITS_PFIT_PROGRAMMED_SCALE_RATIOS)
			arg->display.pfit_programmed_scale_ratios = PSB_RVDC32(PFIT_PGM_RATIOS);
		if (arg->display_read_mask & REGRWBITS_PIPEASRC)
			arg->display.pipeasrc = PSB_RVDC32(PIPEASRC);
		if (arg->display_read_mask & REGRWBITS_PIPEBSRC)
			arg->display.pipebsrc = PSB_RVDC32(PIPEBSRC);
		if (arg->display_read_mask & REGRWBITS_VTOTAL_A)
			arg->display.vtotal_a = PSB_RVDC32(VTOTAL_A);
		if (arg->display_read_mask & REGRWBITS_VTOTAL_B)
			arg->display.vtotal_b = PSB_RVDC32(VTOTAL_B);
		if (arg->display_read_mask & REGRBITS_PIPECONF) {
				arg->display.pipe_enabled = 0;
			if ((PSB_RVDC32(PIPEACONF) & PIPEACONF_ENABLE))
				arg->display.pipe_enabled |= CRTC_PIPEA;
			if ((PSB_RVDC32(PIPEBCONF) & PIPEACONF_ENABLE))
				arg->display.pipe_enabled |= CRTC_PIPEB;
		}
	}

	if (arg->overlay_write_mask != 0) {
		if (arg->overlay_write_mask & OV_REGRWBITS_OGAM_ALL) {
			PSB_WVDC32(arg->overlay.OGAMC5, OV_OGAMC5);
			PSB_WVDC32(arg->overlay.OGAMC4, OV_OGAMC4);
			PSB_WVDC32(arg->overlay.OGAMC3, OV_OGAMC3);
			PSB_WVDC32(arg->overlay.OGAMC2, OV_OGAMC2);
			PSB_WVDC32(arg->overlay.OGAMC1, OV_OGAMC1);
			PSB_WVDC32(arg->overlay.OGAMC0, OV_OGAMC0);
		}
		if (arg->overlay_write_mask & OVC_REGRWBITS_OGAM_ALL) {
			PSB_WVDC32(arg->overlay.OGAMC5, OVC_OGAMC5);
			PSB_WVDC32(arg->overlay.OGAMC4, OVC_OGAMC4);
			PSB_WVDC32(arg->overlay.OGAMC3, OVC_OGAMC3);
			PSB_WVDC32(arg->overlay.OGAMC2, OVC_OGAMC2);
			PSB_WVDC32(arg->overlay.OGAMC1, OVC_OGAMC1);
			PSB_WVDC32(arg->overlay.OGAMC0, OVC_OGAMC0);
		}

		if (arg->overlay_write_mask & OV_REGRWBITS_OVADD)
		{
			PSB_WVDC32(arg->overlay.OVADD, OV_OVADD);

			if (arg->overlay.b_wait_vblank) {
				/*Wait for 20ms.*/
				unsigned long vblank_timeout = jiffies + HZ/50;
				uint32_t temp;
				while (time_before_eq(jiffies, vblank_timeout)) {
					temp = PSB_RVDC32(OV_DOVASTA);
					if ((temp & (0x1 << 31)) != 0) {
						break;
					}
					cpu_relax();
				}
			}

			if (IS_CDV(dev)) {

				if (arg->overlay.IEP_ENABLED) {	
					/* VBLANK period */
					iep_timeout = jiffies + HZ / 10;
					do{
						iep_ble_status = PSB_RVDC32(0x31800);
						if (time_after_eq(jiffies, iep_timeout)) {
							DRM_ERROR("IEP Lite timeout\n");
							break;
						}
						cpu_relax();
					}while((iep_ble_status>>1) != 1);

					arg->overlay.IEP_BLE_MINMAX    = PSB_RVDC32(0x31804);
					arg->overlay.IEP_BSSCC_CONTROL = PSB_RVDC32(0x32000);
				}
			}
		}
		if (arg->overlay_write_mask & OVC_REGRWBITS_OVADD) {
			PSB_WVDC32(arg->overlay.OVADD, OVC_OVADD);
			if (arg->overlay.b_wait_vblank) {
				/*Wait for 20ms.*/
				unsigned long vblank_timeout = jiffies + HZ/50;
				uint32_t temp;
				while (time_before_eq(jiffies, vblank_timeout)) {
					temp = PSB_RVDC32(OVC_DOVCSTA);
					if ((temp & (0x1 << 31)) != 0) {
						break;
					}
					cpu_relax();
				}
			}
		}
	}

	if (arg->overlay_read_mask != 0) {
		if (arg->overlay_read_mask & OV_REGRWBITS_OGAM_ALL) {
			arg->overlay.OGAMC5 = PSB_RVDC32(OV_OGAMC5);
			arg->overlay.OGAMC4 = PSB_RVDC32(OV_OGAMC4);
			arg->overlay.OGAMC3 = PSB_RVDC32(OV_OGAMC3);
			arg->overlay.OGAMC2 = PSB_RVDC32(OV_OGAMC2);
			arg->overlay.OGAMC1 = PSB_RVDC32(OV_OGAMC1);
			arg->overlay.OGAMC0 = PSB_RVDC32(OV_OGAMC0);
		}
		if (arg->overlay_read_mask & OVC_REGRWBITS_OGAM_ALL) {
			arg->overlay.OGAMC5 = PSB_RVDC32(OVC_OGAMC5);
			arg->overlay.OGAMC4 = PSB_RVDC32(OVC_OGAMC4);
			arg->overlay.OGAMC3 = PSB_RVDC32(OVC_OGAMC3);
			arg->overlay.OGAMC2 = PSB_RVDC32(OVC_OGAMC2);
			arg->overlay.OGAMC1 = PSB_RVDC32(OVC_OGAMC1);
			arg->overlay.OGAMC0 = PSB_RVDC32(OVC_OGAMC0);
		}
		if (arg->overlay_read_mask & OV_REGRWBITS_OVADD)
			arg->overlay.OVADD = PSB_RVDC32(OV_OVADD);
		if (arg->overlay_read_mask & OVC_REGRWBITS_OVADD)
			arg->overlay.OVADD = PSB_RVDC32(OVC_OVADD);
	}

	if (arg->sprite_enable_mask != 0) {
		PSB_WVDC32(0x1F3E, DSPARB);
		PSB_WVDC32(arg->sprite.dspa_control | PSB_RVDC32(DSPACNTR), DSPACNTR);
		PSB_WVDC32(arg->sprite.dspa_key_value, DSPAKEYVAL);
		PSB_WVDC32(arg->sprite.dspa_key_mask, DSPAKEYMASK);
		PSB_WVDC32(PSB_RVDC32(DSPASURF), DSPASURF);
		PSB_RVDC32(DSPASURF);
		PSB_WVDC32(arg->sprite.dspc_control, DSPCCNTR);
		PSB_WVDC32(arg->sprite.dspc_stride, DSPCSTRIDE);
		PSB_WVDC32(arg->sprite.dspc_position, DSPCPOS);
		PSB_WVDC32(arg->sprite.dspc_linear_offset, DSPCLINOFF);
		PSB_WVDC32(arg->sprite.dspc_size, DSPCSIZE);
		PSB_WVDC32(arg->sprite.dspc_surface, DSPCSURF);
		PSB_RVDC32(DSPCSURF);
	}

	if (arg->sprite_disable_mask != 0) {
		PSB_WVDC32(0x3F3E, DSPARB);
		PSB_WVDC32(0x0, DSPCCNTR);
		PSB_WVDC32(arg->sprite.dspc_surface, DSPCSURF);
		PSB_RVDC32(DSPCSURF);
	}

	if (arg->subpicture_enable_mask != 0) {
		uint32_t temp;
		if ( arg->subpicture_enable_mask & REGRWBITS_DSPACNTR){
			temp =  PSB_RVDC32(DSPACNTR);
			temp &= ~DISPPLANE_PIXFORMAT_MASK;			
			temp &= ~DISPPLANE_BOTTOM;
			temp |= DISPPLANE_32BPP;
			PSB_WVDC32(temp, DSPACNTR);

			temp =  PSB_RVDC32(DSPABASE);
			PSB_WVDC32(temp, DSPABASE);
			PSB_RVDC32(DSPABASE);
			temp =  PSB_RVDC32(DSPASURF);
			PSB_WVDC32(temp, DSPASURF);
			PSB_RVDC32(DSPASURF);
		}
		if ( arg->subpicture_enable_mask & REGRWBITS_DSPBCNTR){
			temp =  PSB_RVDC32(DSPBCNTR);
			temp &= ~DISPPLANE_PIXFORMAT_MASK;			
			temp &= ~DISPPLANE_BOTTOM;
			temp |= DISPPLANE_32BPP;
			PSB_WVDC32(temp, DSPBCNTR);

			temp =  PSB_RVDC32(DSPBBASE);
			PSB_WVDC32(temp, DSPBBASE);
			PSB_RVDC32(DSPBBASE);
			temp =  PSB_RVDC32(DSPBSURF);
			PSB_WVDC32(temp, DSPBSURF);
			PSB_RVDC32(DSPBSURF);
		}
		if ( arg->subpicture_enable_mask & REGRWBITS_DSPCCNTR){
			temp =  PSB_RVDC32(DSPCCNTR);
			temp &= ~DISPPLANE_PIXFORMAT_MASK;			
			temp &= ~DISPPLANE_BOTTOM;
			temp |= DISPPLANE_32BPP;
			PSB_WVDC32(temp, DSPCCNTR);

			temp =  PSB_RVDC32(DSPCBASE);
			PSB_WVDC32(temp, DSPCBASE);
			PSB_RVDC32(DSPCBASE);
			temp =  PSB_RVDC32(DSPCSURF);
			PSB_WVDC32(temp, DSPCSURF);
			PSB_RVDC32(DSPCSURF);
		}
	}
	
	if (arg->subpicture_disable_mask != 0) {
		uint32_t temp;
		if ( arg->subpicture_disable_mask & REGRWBITS_DSPACNTR){
			temp =  PSB_RVDC32(DSPACNTR);
			temp &= ~DISPPLANE_PIXFORMAT_MASK;			
			temp |= DISPPLANE_32BPP_NO_ALPHA;
			PSB_WVDC32(temp, DSPACNTR);

			temp =  PSB_RVDC32(DSPABASE);
			PSB_WVDC32(temp, DSPABASE);
			PSB_RVDC32(DSPABASE);
			temp =  PSB_RVDC32(DSPASURF);
			PSB_WVDC32(temp, DSPASURF);
			PSB_RVDC32(DSPASURF);
		}
		if ( arg->subpicture_disable_mask & REGRWBITS_DSPBCNTR){
			temp =  PSB_RVDC32(DSPBCNTR);
			temp &= ~DISPPLANE_PIXFORMAT_MASK;			
			temp |= DISPPLANE_32BPP_NO_ALPHA;
			PSB_WVDC32(temp, DSPBCNTR);

			temp =  PSB_RVDC32(DSPBBASE);
			PSB_WVDC32(temp, DSPBBASE);
			PSB_RVDC32(DSPBBASE);
			temp =  PSB_RVDC32(DSPBSURF);
			PSB_WVDC32(temp, DSPBSURF);
			PSB_RVDC32(DSPBSURF);
		}
		if ( arg->subpicture_disable_mask & REGRWBITS_DSPCCNTR){
			temp =  PSB_RVDC32(DSPCCNTR);
			temp &= ~DISPPLANE_PIXFORMAT_MASK;			
			temp |= DISPPLANE_32BPP_NO_ALPHA;
			PSB_WVDC32(temp, DSPCCNTR);

			temp =  PSB_RVDC32(DSPCBASE);
			PSB_WVDC32(temp, DSPCBASE);
			PSB_RVDC32(DSPCBASE);
			temp =  PSB_RVDC32(DSPCSURF);
			PSB_WVDC32(temp, DSPCSURF);
			PSB_RVDC32(DSPCSURF);
		}
	}

	return 0;
}

/* always available as we are SIGIO'd */
static unsigned int psb_poll(struct file *filp,
			     struct poll_table_struct *wait)
{
	return POLLIN | POLLRDNORM;
}

static int psb_driver_open(struct drm_device *dev, struct drm_file *priv)
{
	DRM_DEBUG_KMS("\n");
	return PVRSRVOpen(dev, priv);
}

/* When a client dies:
 *    - Check for and clean up flipped page state
 */
void psb_driver_preclose(struct drm_device *dev, struct drm_file *priv)
{
}

static void psb_remove(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);
	drm_put_dev(dev);
}

static const struct dev_pm_ops psb_pm_ops = {
        .runtime_suspend = psb_runtime_suspend,
        .runtime_resume = psb_runtime_resume,
        .runtime_idle = psb_runtime_idle,
};

static struct drm_driver driver = {
	.driver_features = DRIVER_HAVE_IRQ | DRIVER_IRQ_SHARED | \
			   DRIVER_IRQ_VBL | DRIVER_MODESET,
	.load = psb_driver_load,
	.unload = psb_driver_unload,

	.ioctls = psb_ioctls,
	.num_ioctls = DRM_ARRAY_SIZE(psb_ioctls),
	.device_is_agp = psb_driver_device_is_agp,
	.irq_preinstall = psb_irq_preinstall,
	.irq_postinstall = psb_irq_postinstall,
	.irq_uninstall = psb_irq_uninstall,
	.irq_handler = psb_irq_handler,
	.enable_vblank = psb_enable_vblank,
	.disable_vblank = psb_disable_vblank,
	.get_vblank_counter = psb_get_vblank_counter,
	.firstopen = NULL,
	.lastclose = psb_lastclose,
	.open = psb_driver_open,
	.postclose = PVRSRVDrmPostClose,
	.suspend = PVRSRVDriverSuspend,
	.resume = PVRSRVDriverResume,
	.preclose = psb_driver_preclose,
	.fops = {
		 .owner = THIS_MODULE,
		 .open = psb_open,
		 .release = psb_release,
		 .unlocked_ioctl = drm_ioctl,
		 .mmap = psb_mmap,
		 .poll = psb_poll,
		 .fasync = drm_fasync,
		 .read = drm_read,
		 },
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.major = PSB_DRM_DRIVER_MAJOR,
	.minor = PSB_DRM_DRIVER_MINOR,
	.patchlevel = PSB_DRM_DRIVER_PATCHLEVEL
};

static struct pci_driver psb_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = pciidlist,
	.resume = ospm_power_resume,
	.suspend = ospm_power_suspend,
	.probe = psb_probe,
	.remove = psb_remove,
#if 0
#ifdef CONFIG_PM
	.driver.pm = &psb_pm_ops,
#endif
#endif
};

static int psb_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	return drm_get_pci_dev(pdev, ent, &driver);
}

static int __init psb_init(void)
{
	return drm_pci_init(&driver, &psb_pci_driver);
}

static void __exit psb_exit(void)
{
    int ret;

    /*cleanup for bc_video*/
    ret = BC_Video_ModCleanup();
    if (ret != 0)
    {
        return;
    }

    drm_pci_exit(&driver, &psb_pci_driver);
}

late_initcall(psb_init);
module_exit(psb_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
