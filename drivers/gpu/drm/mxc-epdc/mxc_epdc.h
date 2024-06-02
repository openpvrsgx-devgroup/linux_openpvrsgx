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

#define GRAYSCALE_8BIT				0x1
#define GRAYSCALE_8BIT_INVERTED			0x2
#define GRAYSCALE_4BIT			  0x3
#define GRAYSCALE_4BIT_INVERTED		 0x4

#define AUTO_UPDATE_MODE_REGION_MODE		0
#define AUTO_UPDATE_MODE_AUTOMATIC_MODE		1

#define UPDATE_SCHEME_SNAPSHOT			0
#define UPDATE_SCHEME_QUEUE			1
#define UPDATE_SCHEME_QUEUE_AND_MERGE		2

#define UPDATE_MODE_PARTIAL			0x0
#define UPDATE_MODE_FULL			0x1

#define WAVEFORM_MODE_GLR16			4
#define WAVEFORM_MODE_GLD16			5
#define WAVEFORM_MODE_AUTO			257

#define TEMP_USE_AMBIENT			0x1000

#define FB_POWERDOWN_DISABLE			-1

struct mxcfb_update_data {
	struct drm_rect update_region;
	u32 waveform_mode;
	u32 update_mode;
	int temp;
	int dither_mode;
	int quant_bit;
};

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

	int order_cnt;
	struct list_head upd_pending_list;
	struct list_head upd_buf_queue;
	struct list_head upd_buf_free_list;
	struct list_head upd_buf_collision_list;
	struct update_data_list *cur_update;
	struct mutex queue_mutex;
	int epdc_irq;
	struct list_head full_marker_list;
	u32 *lut_update_order;
	u64 epdc_colliding_luts;
	u64 luts_complete_wb;
	struct completion updates_done;
	struct delayed_work epdc_done_work;
	struct workqueue_struct *epdc_submit_workqueue;
	struct work_struct epdc_submit_work;
	struct workqueue_struct *epdc_intr_workqueue;
	struct work_struct epdc_intr_work;
	bool waiting_for_wb;
	bool waiting_for_lut;
	struct completion update_res_free;
};

static inline u32 epdc_read(struct mxc_epdc *priv, u32 reg)
{
	return readl(priv->iobase + reg);
}

static inline void epdc_write(struct mxc_epdc *priv, u32 reg, u32 data)
{
	writel(data, priv->iobase + reg);
}

