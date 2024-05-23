/**************************************************************************
 * Copyright (c) 2011, Intel Corporation.
 * All Rights Reserved.
 * Copyright (c) 2008, Tungsten Graphics Inc.  Cedar Park, TX., USA.
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

#ifndef _PSB_DRM_H_
#define _PSB_DRM_H_

#if defined(__linux__) && !defined(__KERNEL__)
#include<stdint.h>
#include <linux/types.h>
#include "drm_mode.h"
#endif

#include "psb_ttm_fence_user.h"
#include "psb_ttm_placement_user.h"

/*
 * Menlow/MRST graphics driver package version
 * a.b.c.xxxx
 * a - Product Family: 5 - Linux
 * b - Major Release Version: 0 - non-Gallium (Unbuntu);
 *                            1 - Gallium (Moblin2)
 * c - Hotfix Release
 * xxxx - Graphics internal build #
 */
#define PSB_PACKAGE_VERSION "5.3.0.32L.0039"

#define DRM_PSB_SAREA_MAJOR 0
#define DRM_PSB_SAREA_MINOR 2
#define PSB_FIXED_SHIFT 16

#define PSB_NUM_PIPE 3

/*
 * Public memory types.
 */

#define DRM_PSB_MEM_MMU TTM_PL_PRIV1
#define DRM_PSB_FLAG_MEM_MMU TTM_PL_FLAG_PRIV1

#define TTM_PL_CI               TTM_PL_PRIV0
#define TTM_PL_FLAG_CI          TTM_PL_FLAG_PRIV0       

#define TTM_PL_RAR               TTM_PL_PRIV2
#define TTM_PL_FLAG_RAR          TTM_PL_FLAG_PRIV2       

typedef int32_t psb_fixed;
typedef uint32_t psb_ufixed;

static inline int32_t psb_int_to_fixed(int a)
{
	return a * (1 << PSB_FIXED_SHIFT);
}

static inline uint32_t psb_unsigned_to_ufixed(unsigned int a)
{
	return a << PSB_FIXED_SHIFT;
}

/*Status of the command sent to the gfx device.*/
typedef enum {
	DRM_CMD_SUCCESS,
	DRM_CMD_FAILED,
	DRM_CMD_HANG
} drm_cmd_status_t;

struct drm_psb_scanout {
	uint32_t buffer_id;	/* DRM buffer object ID */
	uint32_t rotation;	/* Rotation as in RR_rotation definitions */
	uint32_t stride;	/* Buffer stride in bytes */
	uint32_t depth;		/* Buffer depth in bits (NOT) bpp */
	uint32_t width;		/* Buffer width in pixels */
	uint32_t height;	/* Buffer height in lines */
	int32_t transform[3][3];	/* Buffer composite transform */
	/* (scaling, rot, reflect) */
};

#define DRM_PSB_SAREA_OWNERS 16
#define DRM_PSB_SAREA_OWNER_2D 0
#define DRM_PSB_SAREA_OWNER_3D 1

#define DRM_PSB_SAREA_SCANOUTS 3

struct drm_psb_sarea {
	/* Track changes of this data structure */

	uint32_t major;
	uint32_t minor;

	/* Last context to touch part of hw */
	uint32_t ctx_owners[DRM_PSB_SAREA_OWNERS];

	/* Definition of front- and rotated buffers */
	uint32_t num_scanouts;
	struct drm_psb_scanout scanouts[DRM_PSB_SAREA_SCANOUTS];

	int planeA_x;
	int planeA_y;
	int planeA_w;
	int planeA_h;
	int planeB_x;
	int planeB_y;
	int planeB_w;
	int planeB_h;
	/* Number of active scanouts */
	uint32_t num_active_scanouts;
};

#define PSB_RELOC_MAGIC         0x67676767
#define PSB_RELOC_SHIFT_MASK    0x0000FFFF
#define PSB_RELOC_SHIFT_SHIFT   0
#define PSB_RELOC_ALSHIFT_MASK  0xFFFF0000
#define PSB_RELOC_ALSHIFT_SHIFT 16

#define PSB_RELOC_OP_OFFSET     0	/* Offset of the indicated
					 * buffer
					 */

struct drm_psb_reloc {
	uint32_t reloc_op;
	uint32_t where;		/* offset in destination buffer */
	uint32_t buffer;	/* Buffer reloc applies to */
	uint32_t mask;		/* Destination format: */
	uint32_t shift;		/* Destination format: */
	uint32_t pre_add;	/* Destination format: */
	uint32_t background;	/* Destination add */
	uint32_t dst_buffer;	/* Destination buffer. Index into buffer_list */
	uint32_t arg0;		/* Reloc-op dependant */
	uint32_t arg1;
};


#define PSB_GPU_ACCESS_READ         (1ULL << 32)
#define PSB_GPU_ACCESS_WRITE        (1ULL << 33)
#define PSB_GPU_ACCESS_MASK         (PSB_GPU_ACCESS_READ | PSB_GPU_ACCESS_WRITE)

#define PSB_BO_FLAG_COMMAND         (1ULL << 52)

#define PSB_ENGINE_2D 0
#define PSB_ENGINE_VIDEO 1

/*
 * For this fence class we have a couple of
 * fence types.
 */

#define _PSB_FENCE_EXE_SHIFT           0
#define _PSB_FENCE_FEEDBACK_SHIFT      4

#define _PSB_FENCE_TYPE_EXE         (1 << _PSB_FENCE_EXE_SHIFT)
#define _PSB_FENCE_TYPE_FEEDBACK    (1 << _PSB_FENCE_FEEDBACK_SHIFT)

#define PSB_NUM_ENGINES 6


#define PSB_FEEDBACK_OP_VISTEST (1 << 0)

struct drm_psb_extension_rep {
	int32_t exists;
	uint32_t driver_ioctl_offset;
	uint32_t sarea_offset;
	uint32_t major;
	uint32_t minor;
	uint32_t pl;
};

#define DRM_PSB_EXT_NAME_LEN 128

union drm_psb_extension_arg {
	char extension[DRM_PSB_EXT_NAME_LEN];
	struct drm_psb_extension_rep rep;
};

struct psb_validate_req {
	uint64_t set_flags;
	uint64_t clear_flags;
	uint64_t next;
	uint64_t presumed_gpu_offset;
	uint32_t buffer_handle;
	uint32_t presumed_flags;
	uint32_t group;
	uint32_t pad64;
};

struct psb_validate_rep {
	uint64_t gpu_offset;
	uint32_t placement;
	uint32_t fence_type_mask;
};

#define PSB_USE_PRESUMED     (1 << 0)

struct psb_validate_arg {
	int handled;
	int ret;
	union {
		struct psb_validate_req req;
		struct psb_validate_rep rep;
	} d;
};


#define DRM_PSB_FENCE_NO_USER        (1 << 0)

struct psb_ttm_fence_rep {
	uint32_t handle;
	uint32_t fence_class;
	uint32_t fence_type;
	uint32_t signaled_types;
	uint32_t error;
};

typedef struct drm_psb_cmdbuf_arg {
	uint64_t buffer_list;	/* List of buffers to validate */
	uint64_t clip_rects;	/* See i915 counterpart */
	uint64_t scene_arg;
	uint64_t fence_arg;

	uint32_t ta_flags;

	uint32_t ta_handle;	/* TA reg-value pairs */
	uint32_t ta_offset;
	uint32_t ta_size;

	uint32_t oom_handle;
	uint32_t oom_offset;
	uint32_t oom_size;

	uint32_t cmdbuf_handle;	/* 2D Command buffer object or, */
	uint32_t cmdbuf_offset;	/* rasterizer reg-value pairs */
	uint32_t cmdbuf_size;

	uint32_t reloc_handle;	/* Reloc buffer object */
	uint32_t reloc_offset;
	uint32_t num_relocs;

	int32_t damage;		/* Damage front buffer with cliprects */
	/* Not implemented yet */
	uint32_t fence_flags;
	uint32_t engine;

	/*
	 * Feedback;
	 */

	uint32_t feedback_ops;
	uint32_t feedback_handle;
	uint32_t feedback_offset;
	uint32_t feedback_breakpoints;
	uint32_t feedback_size;
} drm_psb_cmdbuf_arg_t;

typedef struct drm_psb_pageflip_arg {
	uint32_t flip_offset;
	uint32_t stride;
} drm_psb_pageflip_arg_t;

typedef enum {
	LNC_VIDEO_DEVICE_INFO,
	LNC_VIDEO_GETPARAM_RAR_INFO,
	LNC_VIDEO_GETPARAM_CI_INFO,
	LNC_VIDEO_GETPARAM_RAR_HANDLER_OFFSET,
	LNC_VIDEO_FRAME_SKIP,
	IMG_VIDEO_DECODE_STATUS,
	IMG_VIDEO_NEW_CONTEXT,
	IMG_VIDEO_RM_CONTEXT,
	IMG_VIDEO_MB_ERROR
} lnc_getparam_key_t;

struct drm_lnc_video_getparam_arg {
	lnc_getparam_key_t key;
	uint64_t arg;	/* argument pointer */
	uint64_t value;	/* feed back pointer */
};


/*
 * Feedback components:
 */

/*
 * Vistest component. The number of these in the feedback buffer
 * equals the number of vistest breakpoints + 1.
 * This is currently the only feedback component.
 */

struct drm_psb_vistest {
	uint32_t vt[8];
};

struct drm_psb_sizes_arg {
	uint32_t ta_mem_size;
	uint32_t mmu_size;
	uint32_t pds_size;
	uint32_t rastgeom_size;
	uint32_t tt_size;
	uint32_t vram_size;
};

struct drm_psb_hist_status_arg {
	uint32_t buf[32];
};

struct drm_psb_dpst_lut_arg {
	uint8_t lut[256];
	int output_id;
};

struct mrst_timing_info {
	uint16_t pixel_clock;
	uint8_t hactive_lo;
	uint8_t hblank_lo;
	uint8_t hblank_hi:4;
	uint8_t hactive_hi:4;
	uint8_t vactive_lo;
	uint8_t vblank_lo;
	uint8_t vblank_hi:4;
	uint8_t vactive_hi:4;
	uint8_t hsync_offset_lo;
	uint8_t hsync_pulse_width_lo;
	uint8_t vsync_pulse_width_lo:4;
	uint8_t vsync_offset_lo:4;
	uint8_t vsync_pulse_width_hi:2;
	uint8_t vsync_offset_hi:2;
	uint8_t hsync_pulse_width_hi:2;
	uint8_t hsync_offset_hi:2;
	uint8_t width_mm_lo;
	uint8_t height_mm_lo;
	uint8_t height_mm_hi:4;
	uint8_t width_mm_hi:4;
	uint8_t hborder;
	uint8_t vborder;
	uint8_t unknown0:1;
	uint8_t hsync_positive:1;
	uint8_t vsync_positive:1;
	uint8_t separate_sync:2;
	uint8_t stereo:1;
	uint8_t unknown6:1;
	uint8_t interlaced:1;
} __attribute__((packed));

struct gct_ioctl_arg{
	uint8_t bpi; /* boot panel index, number of panel used during boot */
	uint8_t pt; /* panel type, 4 bit field, 0=lvds, 1=mipi */
	struct mrst_timing_info DTD; /* timing info for the selected panel */
	uint32_t Panel_Port_Control;
	uint32_t PP_On_Sequencing;/*1 dword,Register 0x61208,*/
	uint32_t PP_Off_Sequencing;/*1 dword,Register 0x6120C,*/
	uint32_t PP_Cycle_Delay;
	uint16_t Panel_Backlight_Inverter_Descriptor;
	uint16_t Panel_MIPI_Display_Descriptor;
} __attribute__ ((packed));

#define PSB_DC_CRTC_SAVE 0x01
#define PSB_DC_CRTC_RESTORE 0x02
#define PSB_DC_OUTPUT_SAVE 0x04
#define PSB_DC_OUTPUT_RESTORE 0x08
#define PSB_DC_CRTC_MASK 0x03
#define PSB_DC_OUTPUT_MASK 0x0C

struct drm_psb_dc_state_arg {
	uint32_t flags;
	uint32_t obj_id;
};

struct drm_psb_mode_operation_arg {
	uint32_t obj_id;
	uint16_t operation;
	struct drm_mode_modeinfo mode;
	void *data;
};

struct drm_psb_stolen_memory_arg {
	uint32_t base;
	uint32_t size;
};

/*Display Register Bits*/
#define REGRWBITS_PFIT_CONTROLS			(1 << 0)
#define REGRWBITS_PFIT_AUTOSCALE_RATIOS		(1 << 1)
#define REGRWBITS_PFIT_PROGRAMMED_SCALE_RATIOS	(1 << 2)
#define REGRWBITS_PIPEASRC			(1 << 3)
#define REGRWBITS_PIPEBSRC			(1 << 4)
#define REGRWBITS_VTOTAL_A			(1 << 5)
#define REGRWBITS_VTOTAL_B			(1 << 6)
#define REGRWBITS_DSPACNTR	(1 << 8)
#define REGRWBITS_DSPBCNTR	(1 << 9)
#define REGRWBITS_DSPCCNTR	(1 << 10)
#define REGRBITS_PIPECONF	(1 << 11)


/*Overlay Register Bits*/
#define OV_REGRWBITS_OVADD			(1 << 0)
#define OV_REGRWBITS_OGAM_ALL			(1 << 1)

#define OVC_REGRWBITS_OVADD                  (1 << 2)
#define OVC_REGRWBITS_OGAM_ALL			(1 << 3)

struct drm_psb_register_rw_arg {
	uint32_t b_force_hw_on;

	uint32_t display_read_mask;
	uint32_t display_write_mask;

	struct {
		uint32_t pfit_controls;
		uint32_t pfit_autoscale_ratios;
		uint32_t pfit_programmed_scale_ratios;
		uint32_t pipeasrc;
		uint32_t pipebsrc;
		uint32_t vtotal_a;
		uint32_t vtotal_b;
		uint32_t pipe_enabled;
	} display;

	uint32_t overlay_read_mask;
	uint32_t overlay_write_mask;

	struct {
		uint32_t OVADD;
		uint32_t OGAMC0;
		uint32_t OGAMC1;
		uint32_t OGAMC2;
		uint32_t OGAMC3;
		uint32_t OGAMC4;
		uint32_t OGAMC5;
        	uint32_t IEP_ENABLED;
        	uint32_t IEP_BLE_MINMAX;
        	uint32_t IEP_BSSCC_CONTROL;
                uint32_t b_wait_vblank;
	} overlay;

	uint32_t sprite_enable_mask;
	uint32_t sprite_disable_mask;

	struct {
		uint32_t dspa_control;
		uint32_t dspa_key_value;
		uint32_t dspa_key_mask;
		uint32_t dspc_control;
		uint32_t dspc_stride;
		uint32_t dspc_position;
		uint32_t dspc_linear_offset;
		uint32_t dspc_size;
		uint32_t dspc_surface;
	} sprite;

	uint32_t subpicture_enable_mask;
	uint32_t subpicture_disable_mask;
};

struct psb_gtt_mapping_arg {
	void *hKernelMemInfo;
	uint32_t offset_pages;
};

struct drm_psb_getpageaddrs_arg {
	uint32_t handle;
	unsigned long *page_addrs;
	unsigned long gtt_offset;
};


#define MAX_SLICES_PER_PICTURE 72
typedef struct drm_psb_msvdx_decode_status {
	uint32_t fw_status;
	uint32_t num_error_slice;
	int32_t start_error_mb_list[MAX_SLICES_PER_PICTURE];
	int32_t end_error_mb_list[MAX_SLICES_PER_PICTURE];
	int32_t slice_missing_or_error[MAX_SLICES_PER_PICTURE];
} drm_psb_msvdx_decode_status_t;

/* Controlling the kernel modesetting buffers */

#define DRM_PSB_KMS_OFF		0x00
#define DRM_PSB_KMS_ON		0x01
#define DRM_PSB_VT_LEAVE        0x02
#define DRM_PSB_VT_ENTER        0x03
#define DRM_PSB_EXTENSION       0x06
#define DRM_PSB_SIZES           0x07
#define DRM_PSB_FUSE_REG	0x08
#define DRM_PSB_VBT		0x09
#define DRM_PSB_DC_STATE	0x0A
#define DRM_PSB_ADB		0x0B
#define DRM_PSB_MODE_OPERATION	0x0C
#define DRM_PSB_STOLEN_MEMORY	0x0D
#define DRM_PSB_REGISTER_RW	0x0E
#define DRM_PSB_GTT_MAP         0x0F
#define DRM_PSB_GTT_UNMAP       0x10
#define DRM_PSB_GETPAGEADDRS	0x11
/**
 * NOTE: Add new commands here, but increment
 * the values below and increment their
 * corresponding defines where they're
 * defined elsewhere.
 */
#define DRM_PVR_RESERVED1	0x12
#define DRM_PVR_RESERVED2	0x13
#define DRM_PVR_RESERVED3	0x14
#define DRM_PVR_RESERVED4	0x15
#define DRM_PVR_RESERVED5	0x16

#define DRM_PSB_HIST_ENABLE	0x17
#define DRM_PSB_HIST_STATUS	0x18
#define DRM_PSB_UPDATE_GUARD	0x19
#define DRM_PSB_INIT_COMM	0x1A
#define DRM_PSB_DPST		0x1B
#define DRM_PSB_GAMMA		0x1C
#define DRM_PSB_DPST_BL		0x1D

#define DRM_PVR_RESERVED6	0x1E

#define DRM_PSB_GET_PIPE_FROM_CRTC_ID 0x1F
#define DRM_PSB_DPU_QUERY 0x20
#define DRM_PSB_DPU_DSR_ON 0x21
#define DRM_PSB_DPU_DSR_OFF 0x22

#define DRM_PSB_DSR_ENABLE	0xfffffffe
#define DRM_PSB_DSR_DISABLE	0xffffffff

struct psb_drm_dpu_rect {  
    int x, y;             
    int width, height;    
};  

struct drm_psb_drv_dsr_off_arg {
	int screen;
	struct psb_drm_dpu_rect damage_rect;
};


struct drm_psb_dev_info_arg {
	uint32_t num_use_attribute_registers;
};
#define DRM_PSB_DEVINFO         0x01

#define PSB_MODE_OPERATION_MODE_VALID	0x01
#define PSB_MODE_OPERATION_SET_DC_BASE  0x02

struct drm_psb_get_pipe_from_crtc_id_arg {
	/** ID of CRTC being requested **/
	uint32_t crtc_id;

	/** pipe of requested CRTC **/
	uint32_t pipe;
};

#endif
