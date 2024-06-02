/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/* gdepaper_drm.h
 *
 * Copyright (c) 2019 Jan Sebastian Götte
 */

#ifndef _UAPI_GDEPAPER_DRM_H_
#define _UAPI_GDEPAPER_DRM_H_

#include "drm.h"

#if defined(__cplusplus)
extern "C" {
#endif

enum drm_gdepaper_vghl_lv {
	DRM_GDEP_PWR_VGHL_16V = 0,
	DRM_GDEP_PWR_VGHL_15V = 1,
	DRM_GDEP_PWR_VGHL_14V = 2,
	DRM_GDEP_PWR_VGHL_13V = 3,
};

struct gdepaper_refresh_params {
	enum drm_gdepaper_vghl_lv vg_lv; /* gate voltage level */
	__u32 vcom_sel; /* VCOM select bit according to datasheet */
	__s32 vdh_bw_mv; /* drive high level, b/w pixel (mV) */
	__s32 vdh_col_mv; /* drive high level, red/yellow pixel (mV) */
	__s32 vdl_mv; /* drive low level (mV) */
	__s32 vcom_dc_mv;
	__u32 vcom_data_ivl_hsync; /* data ivl len in hsync periods */
	__u32 border_data_sel; /* "vbd" in datasheet */
	__u32 data_polarity; /* "ddx" in datasheet */
	__u32 use_otp_luts_flag; /* Use OTP LUTs */
	__u8 lut_vcom_dc[44];
	__u8 lut_ww[42];
	__u8 lut_bw[42];
	__u8 lut_bb[42];
	__u8 lut_wb[42];
};

/* Force a full display refresh */
#define DRM_GDEPAPER_FORCE_FULL_REFRESH		0x00
#define DRM_GDEPAPER_GET_REFRESH_PARAMS		0x01
#define DRM_GDEPAPER_SET_REFRESH_PARAMS		0x02
#define DRM_GDEPAPER_SET_PARTIAL_UPDATE_EN	0x03

#define DRM_IOCTL_GDEPAPER_FORCE_FULL_REFRESH \
	DRM_IO(DRM_COMMAND_BASE + DRM_GDEPAPER_FORCE_FULL_REFRESH)
#define DRM_IOCTL_GDEPAPER_GET_REFRESH_PARAMS \
	DRM_IOR(DRM_COMMAND_BASE + DRM_GDEPAPER_GET_REFRESH_PARAMS, \
	struct gdepaper_refresh_params)
#define DRM_IOCTL_GDEPAPER_SET_REFRESH_PARAMS \
	DRM_IOR(DRM_COMMAND_BASE + DRM_GDEPAPER_SET_REFRESH_PARAMS, \
	struct gdepaper_refresh_params)
#define DRM_IOCTL_GDEPAPER_SET_PARTIAL_UPDATE_EN \
	DRM_IOR(DRM_COMMAND_BASE + DRM_GDEPAPER_SET_PARTIAL_UPDATE_EN, __u32)

#if defined(__cplusplus)
}
#endif

#endif /* _UAPI_GDEPAPER_DRM_H_ */
