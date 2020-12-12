/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (C) 2020 Andreas Kemnade */

#include <video/display_timing.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>

#include <drm/drm_drv.h>
#include <drm/drm_connector.h>
#include <drm/drm_simple_kms_helper.h>
#include <linux/thermal.h>
#include "epdc_regs.h"

struct mxcfb_waveform_modes {
	int mode_init;
	int mode_du;
	int mode_gc4;
	int mode_gc8;
	int mode_gc16;
	int mode_gc32;
};

struct imx_epdc_fb_mode {
	u32 vscan_holdoff;
	u32 sdoed_width;
	u32 sdoed_delay;
	u32 sdoez_width;
	u32 sdoez_delay;
	u32 gdclk_hp_offs;
	u32 gdsp_offs;
	u32 gdoe_offs;
	u32 gdclk_offs;
	u32 num_ce;
};

struct clk;
struct regulator;
struct mxc_epdc {
	struct drm_device drm;
	struct drm_simple_display_pipe pipe;
	struct drm_connector connector;
	struct display_timing timing;
	struct imx_epdc_fb_mode imx_mode;
	void __iomem *iobase;
	struct completion powerdown_compl;
	struct clk *epdc_clk_axi;
	struct clk *epdc_clk_pix;
	struct regulator *display_regulator;
	struct regulator *vcom_regulator;
	struct regulator *v3p3_regulator;
	struct thermal_zone_device *thermal;
	int rev;

	dma_addr_t epdc_mem_phys;
	void *epdc_mem_virt;
	int epdc_mem_width;
	int epdc_mem_height;
	u32 *working_buffer_virt;
	dma_addr_t working_buffer_phys;
	u32 working_buffer_size;

	/* waveform related stuff */
	int trt_entries;
	int temp_index;
	u8 *temp_range_bounds;
	int buf_pix_fmt;
	struct mxcfb_waveform_modes wv_modes;
	bool wv_modes_update;
	u32 *waveform_buffer_virt;
	dma_addr_t waveform_buffer_phys;
	u32 waveform_buffer_size;

	struct mutex power_mutex;
	bool powered;
	bool powering_down;
	bool updates_active;
	int wait_for_powerdown;
	int pwrdown_delay;

	/* elements related to EPDC updates */
	int num_luts;
	int max_num_updates;
#warning take of init of in_init hw_ready, hw_initializing
	bool in_init;
	bool hw_ready;
	bool hw_initializing;
	bool waiting_for_idle;

};

static inline u32 epdc_read(struct mxc_epdc *priv, u32 reg)
{
	return readl(priv->iobase + reg);
}

static inline void epdc_write(struct mxc_epdc *priv, u32 reg, u32 data)
{
	writel(data, priv->iobase + reg);
}

