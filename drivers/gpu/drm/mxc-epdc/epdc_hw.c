// SPDX-License-Identifier: GPL-2.0+
// Copyright (C) 2020 Andreas Kemnade
//
/*
 * based on the EPDC framebuffer driver
 * Copyright (C) 2010-2016 Freescale Semiconductor, Inc.
 * Copyright 2017 NXP
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>

#include "mxc_epdc.h"
#include "epdc_regs.h"
#include "epdc_hw.h"
/* TODO: should probably be untangled */
#include "epdc_update.h"
#include "epdc_waveform.h"

void mxc_epdc_powerup(struct mxc_epdc *priv)
{
	int ret = 0;

	mutex_lock(&priv->power_mutex);

	/*
	 * If power down request is pending, clear
	 * powering_down to cancel the request.
	 */
	if (priv->powering_down)
		priv->powering_down = false;

	if (priv->powered) {
		mutex_unlock(&priv->power_mutex);
		return;
	}

	dev_dbg(priv->drm.dev, "EPDC Powerup\n");

	priv->updates_active = true;

	/* Enable the v3p3 regulator */
	ret = regulator_enable(priv->v3p3_regulator);
	if (IS_ERR((void *)ret)) {
		dev_err(priv->drm.dev,
			"Unable to enable V3P3 regulator. err = 0x%x\n",
			ret);
		mutex_unlock(&priv->power_mutex);
		return;
	}

	usleep_range(1000, 2000);

	pm_runtime_get_sync(priv->drm.dev);

	/* Enable clocks to EPDC */
	clk_prepare_enable(priv->epdc_clk_axi);
	clk_prepare_enable(priv->epdc_clk_pix);

	epdc_write(priv, EPDC_CTRL_CLEAR, EPDC_CTRL_CLKGATE);

	/* Enable power to the EPD panel */
	ret = regulator_enable(priv->display_regulator);
	if (IS_ERR((void *)ret)) {
		dev_err(priv->drm.dev,
			"Unable to enable DISPLAY regulator. err = 0x%x\n",
			ret);
		mutex_unlock(&priv->power_mutex);
		return;
	}
	ret = regulator_enable(priv->vcom_regulator);
	if (IS_ERR((void *)ret)) {
		dev_err(priv->drm.dev,
			"Unable to enable VCOM regulator. err = 0x%x\n",
			ret);
		mutex_unlock(&priv->power_mutex);
		return;
	}

	priv->powered = true;

	mutex_unlock(&priv->power_mutex);
}

void mxc_epdc_powerdown(struct mxc_epdc *priv)
{
	mutex_lock(&priv->power_mutex);

	/* If powering_down has been cleared, a powerup
	 * request is pre-empting this powerdown request.
	 */
	if (!priv->powering_down
		|| (!priv->powered)) {
		mutex_unlock(&priv->power_mutex);
		return;
	}

	dev_dbg(priv->drm.dev, "EPDC Powerdown\n");

	/* Disable power to the EPD panel */
	regulator_disable(priv->vcom_regulator);
	regulator_disable(priv->display_regulator);

	/* Disable clocks to EPDC */
	epdc_write(priv, EPDC_CTRL_SET, EPDC_CTRL_CLKGATE);
	clk_disable_unprepare(priv->epdc_clk_pix);
	clk_disable_unprepare(priv->epdc_clk_axi);

	pm_runtime_put_sync_suspend(priv->drm.dev);

	/* turn off the V3p3 */
	regulator_disable(priv->v3p3_regulator);

	priv->powered = false;
	priv->powering_down = false;

	if (priv->wait_for_powerdown) {
		priv->wait_for_powerdown = false;
		complete(&priv->powerdown_compl);
	}

	mutex_unlock(&priv->power_mutex);
}

static void epdc_set_horizontal_timing(struct mxc_epdc *priv, u32 horiz_start,
				       u32 horiz_end,
				       u32 hsync_width, u32 hsync_line_length)
{
	u32 reg_val =
	    ((hsync_width << EPDC_TCE_HSCAN1_LINE_SYNC_WIDTH_OFFSET) &
	     EPDC_TCE_HSCAN1_LINE_SYNC_WIDTH_MASK)
	    | ((hsync_line_length << EPDC_TCE_HSCAN1_LINE_SYNC_OFFSET) &
	       EPDC_TCE_HSCAN1_LINE_SYNC_MASK);
	epdc_write(priv, EPDC_TCE_HSCAN1, reg_val);

	reg_val =
	    ((horiz_start << EPDC_TCE_HSCAN2_LINE_BEGIN_OFFSET) &
	     EPDC_TCE_HSCAN2_LINE_BEGIN_MASK)
	    | ((horiz_end << EPDC_TCE_HSCAN2_LINE_END_OFFSET) &
	       EPDC_TCE_HSCAN2_LINE_END_MASK);
	epdc_write(priv, EPDC_TCE_HSCAN2, reg_val);
}

static void epdc_set_vertical_timing(struct mxc_epdc *priv,
				     u32 vert_start,
				     u32 vert_end,
				     u32 vsync_width)
{
	u32 reg_val =
	    ((vert_start << EPDC_TCE_VSCAN_FRAME_BEGIN_OFFSET) &
	     EPDC_TCE_VSCAN_FRAME_BEGIN_MASK)
	    | ((vert_end << EPDC_TCE_VSCAN_FRAME_END_OFFSET) &
	       EPDC_TCE_VSCAN_FRAME_END_MASK)
	    | ((vsync_width << EPDC_TCE_VSCAN_FRAME_SYNC_OFFSET) &
	       EPDC_TCE_VSCAN_FRAME_SYNC_MASK);
	epdc_write(priv, EPDC_TCE_VSCAN, reg_val);
}

static inline void epdc_set_screen_res(struct mxc_epdc *priv,
				       u32 width, u32 height)
{
	u32 val = (height << EPDC_RES_VERTICAL_OFFSET) | width;

	epdc_write(priv, EPDC_RES, val);
}


void epdc_init_settings(struct mxc_epdc *priv, struct drm_display_mode *m)
{
	u32 reg_val;
	int num_ce;
	int i;

	/* Enable clocks to access EPDC regs */
	clk_prepare_enable(priv->epdc_clk_axi);
	clk_prepare_enable(priv->epdc_clk_pix);

	/* Reset */
	epdc_write(priv, EPDC_CTRL_SET, EPDC_CTRL_SFTRST);
	while (!(epdc_read(priv, EPDC_CTRL) & EPDC_CTRL_CLKGATE))
		;
	epdc_write(priv, EPDC_CTRL_CLEAR, EPDC_CTRL_SFTRST);

	/* Enable clock gating (clear to enable) */
	epdc_write(priv, EPDC_CTRL_CLEAR, EPDC_CTRL_CLKGATE);
	while (epdc_read(priv, EPDC_CTRL) & (EPDC_CTRL_SFTRST | EPDC_CTRL_CLKGATE))
		;

	/* EPDC_CTRL */
	reg_val = epdc_read(priv, EPDC_CTRL);
	reg_val &= ~EPDC_CTRL_UPD_DATA_SWIZZLE_MASK;
	reg_val |= EPDC_CTRL_UPD_DATA_SWIZZLE_NO_SWAP;
	reg_val &= ~EPDC_CTRL_LUT_DATA_SWIZZLE_MASK;
	reg_val |= EPDC_CTRL_LUT_DATA_SWIZZLE_NO_SWAP;
	epdc_write(priv, EPDC_CTRL_SET, reg_val);

	/* EPDC_FORMAT - 2bit TFT and buf_pix_fmt Buf pixel format */
	reg_val = EPDC_FORMAT_TFT_PIXEL_FORMAT_2BIT
		| priv->buf_pix_fmt
	    | ((0x0 << EPDC_FORMAT_DEFAULT_TFT_PIXEL_OFFSET) &
	       EPDC_FORMAT_DEFAULT_TFT_PIXEL_MASK);
	epdc_write(priv, EPDC_FORMAT, reg_val);
	if (priv->rev >= 30) {
		if (priv->buf_pix_fmt == EPDC_FORMAT_BUF_PIXEL_FORMAT_P5N) {
			/* 110 00101 0101 0100*/
			epdc_write(priv, EPDC_WB_FIELD2, 0xc554);
			/* 101 00000 0000 0100*/
			epdc_write(priv, EPDC_WB_FIELD1, 0xa004);
		} else {
			/* 110 00101 0101 0100*/
			epdc_write(priv, EPDC_WB_FIELD2, 0xc443);
			/* 101 00000 0000 0100*/
			epdc_write(priv, EPDC_WB_FIELD1, 0xa003);
		}
	}

	/* EPDC_FIFOCTRL (disabled) */
	reg_val =
	    ((100 << EPDC_FIFOCTRL_FIFO_INIT_LEVEL_OFFSET) &
	     EPDC_FIFOCTRL_FIFO_INIT_LEVEL_MASK)
	    | ((200 << EPDC_FIFOCTRL_FIFO_H_LEVEL_OFFSET) &
	       EPDC_FIFOCTRL_FIFO_H_LEVEL_MASK)
	    | ((100 << EPDC_FIFOCTRL_FIFO_L_LEVEL_OFFSET) &
	       EPDC_FIFOCTRL_FIFO_L_LEVEL_MASK);
	epdc_write(priv, EPDC_FIFOCTRL, reg_val);

	/* EPDC_TEMP - Use default temp to get index */
	epdc_write(priv, EPDC_TEMP,
		   mxc_epdc_fb_get_temp_index(priv, TEMP_USE_AMBIENT));

	/* EPDC_RES */
	epdc_set_screen_res(priv, m->hdisplay, m->vdisplay);

	/* EPDC_AUTOWV_LUT */
	/* Initialize all auto-wavefrom look-up values to 2 - GC16 */
	for (i = 0; i < 8; i++)
		epdc_write(priv, EPDC_AUTOWV_LUT,
			(2 << EPDC_AUTOWV_LUT_DATA_OFFSET) |
			(i << EPDC_AUTOWV_LUT_ADDR_OFFSET));

	/*
	 * EPDC_TCE_CTRL
	 * VSCAN_HOLDOFF = 4
	 * VCOM_MODE = MANUAL
	 * VCOM_VAL = 0
	 * DDR_MODE = DISABLED
	 * LVDS_MODE_CE = DISABLED
	 * LVDS_MODE = DISABLED
	 * DUAL_SCAN = DISABLED
	 * SDDO_WIDTH = 8bit
	 * PIXELS_PER_SDCLK = 4
	 */
	reg_val =
	    ((priv->imx_mode.vscan_holdoff << EPDC_TCE_CTRL_VSCAN_HOLDOFF_OFFSET) &
	     EPDC_TCE_CTRL_VSCAN_HOLDOFF_MASK)
	    | EPDC_TCE_CTRL_PIXELS_PER_SDCLK_4;
	epdc_write(priv, EPDC_TCE_CTRL, reg_val);

	/* EPDC_TCE_HSCAN */
	epdc_set_horizontal_timing(priv, m->hsync_start - m->hdisplay,
				   m->htotal - m->hsync_end,
				   m->hsync_end - m->hsync_start,
				   m->hsync_end - m->hsync_start);

	/* EPDC_TCE_VSCAN */
	epdc_set_vertical_timing(priv, m->vsync_start - m->vdisplay,
				 m->vtotal - m->vsync_end,
				 m->vsync_end - m->vsync_start);

	/* EPDC_TCE_OE */
	reg_val =
	    ((priv->imx_mode.sdoed_width << EPDC_TCE_OE_SDOED_WIDTH_OFFSET) &
	     EPDC_TCE_OE_SDOED_WIDTH_MASK)
	    | ((priv->imx_mode.sdoed_delay << EPDC_TCE_OE_SDOED_DLY_OFFSET) &
	       EPDC_TCE_OE_SDOED_DLY_MASK)
	    | ((priv->imx_mode.sdoez_width << EPDC_TCE_OE_SDOEZ_WIDTH_OFFSET) &
	       EPDC_TCE_OE_SDOEZ_WIDTH_MASK)
	    | ((priv->imx_mode.sdoez_delay << EPDC_TCE_OE_SDOEZ_DLY_OFFSET) &
	       EPDC_TCE_OE_SDOEZ_DLY_MASK);
	epdc_write(priv, EPDC_TCE_OE, reg_val);

	/* EPDC_TCE_TIMING1 */
	epdc_write(priv, EPDC_TCE_TIMING1, 0x0);

	/* EPDC_TCE_TIMING2 */
	reg_val =
	    ((priv->imx_mode.gdclk_hp_offs << EPDC_TCE_TIMING2_GDCLK_HP_OFFSET) &
	     EPDC_TCE_TIMING2_GDCLK_HP_MASK)
	    | ((priv->imx_mode.gdsp_offs << EPDC_TCE_TIMING2_GDSP_OFFSET_OFFSET) &
	       EPDC_TCE_TIMING2_GDSP_OFFSET_MASK);
	epdc_write(priv, EPDC_TCE_TIMING2, reg_val);

	/* EPDC_TCE_TIMING3 */
	reg_val =
	    ((priv->imx_mode.gdoe_offs << EPDC_TCE_TIMING3_GDOE_OFFSET_OFFSET) &
	     EPDC_TCE_TIMING3_GDOE_OFFSET_MASK)
	    | ((priv->imx_mode.gdclk_offs << EPDC_TCE_TIMING3_GDCLK_OFFSET_OFFSET) &
	       EPDC_TCE_TIMING3_GDCLK_OFFSET_MASK);
	epdc_write(priv, EPDC_TCE_TIMING3, reg_val);

	/*
	 * EPDC_TCE_SDCFG
	 * SDCLK_HOLD = 1
	 * SDSHR = 1
	 * NUM_CE = 1
	 * SDDO_REFORMAT = FLIP_PIXELS
	 * SDDO_INVERT = DISABLED
	 * PIXELS_PER_CE = display horizontal resolution
	 */
	num_ce = priv->imx_mode.num_ce;
	if (num_ce == 0)
		num_ce = 1;
	reg_val = EPDC_TCE_SDCFG_SDCLK_HOLD | EPDC_TCE_SDCFG_SDSHR
	    | ((num_ce << EPDC_TCE_SDCFG_NUM_CE_OFFSET) &
	       EPDC_TCE_SDCFG_NUM_CE_MASK)
	    | EPDC_TCE_SDCFG_SDDO_REFORMAT_FLIP_PIXELS
	    | ((priv->epdc_mem_width/num_ce << EPDC_TCE_SDCFG_PIXELS_PER_CE_OFFSET) &
	       EPDC_TCE_SDCFG_PIXELS_PER_CE_MASK);
	epdc_write(priv, EPDC_TCE_SDCFG, reg_val);

	/*
	 * EPDC_TCE_GDCFG
	 * GDRL = 1
	 * GDOE_MODE = 0;
	 * GDSP_MODE = 0;
	 */
	reg_val = EPDC_TCE_SDCFG_GDRL;
	epdc_write(priv, EPDC_TCE_GDCFG, reg_val);

	/*
	 * EPDC_TCE_POLARITY
	 * SDCE_POL = ACTIVE LOW
	 * SDLE_POL = ACTIVE HIGH
	 * SDOE_POL = ACTIVE HIGH
	 * GDOE_POL = ACTIVE HIGH
	 * GDSP_POL = ACTIVE LOW
	 */
	reg_val = EPDC_TCE_POLARITY_SDLE_POL_ACTIVE_HIGH
	    | EPDC_TCE_POLARITY_SDOE_POL_ACTIVE_HIGH
	    | EPDC_TCE_POLARITY_GDOE_POL_ACTIVE_HIGH;
	epdc_write(priv, EPDC_TCE_POLARITY, reg_val);

	/* EPDC_IRQ_MASK */
	epdc_write(priv, EPDC_IRQ_MASK, EPDC_IRQ_TCE_UNDERRUN_IRQ);

	/*
	 * EPDC_GPIO
	 * PWRCOM = ?
	 * PWRCTRL = ?
	 * BDR = ?
	 */
	reg_val = ((0 << EPDC_GPIO_PWRCTRL_OFFSET) & EPDC_GPIO_PWRCTRL_MASK)
	    | ((0 << EPDC_GPIO_BDR_OFFSET) & EPDC_GPIO_BDR_MASK);
	epdc_write(priv, EPDC_GPIO, reg_val);

	epdc_write(priv, EPDC_WVADDR, priv->waveform_buffer_phys);
	epdc_write(priv, EPDC_WB_ADDR, priv->working_buffer_phys);
	if (priv->rev >= 30)
		epdc_write(priv, EPDC_WB_ADDR_TCE_V3,
			   priv->working_buffer_phys);
	else
		epdc_write(priv, EPDC_WB_ADDR_TCE,
			   priv->working_buffer_phys);

	/* Disable clock */
	clk_disable_unprepare(priv->epdc_clk_axi);
	clk_disable_unprepare(priv->epdc_clk_pix);
}

void mxc_epdc_init_sequence(struct mxc_epdc *priv, struct drm_display_mode *m)
{
	/* Initialize EPDC, passing pointer to EPDC registers */
	struct clk *epdc_parent;
	unsigned long rounded_parent_rate, epdc_pix_rate,
			rounded_pix_clk, target_pix_clk;

	/* Enable pix clk for EPDC */
	clk_prepare_enable(priv->epdc_clk_axi);

	target_pix_clk = m->clock * 1000;
	rounded_pix_clk = clk_round_rate(priv->epdc_clk_pix, target_pix_clk);

	if (((rounded_pix_clk >= target_pix_clk + target_pix_clk/100) ||
		(rounded_pix_clk <= target_pix_clk - target_pix_clk/100))) {
		/* Can't get close enough without changing parent clk */
		epdc_parent = clk_get_parent(priv->epdc_clk_pix);
		rounded_parent_rate = clk_round_rate(epdc_parent, target_pix_clk);

		epdc_pix_rate = target_pix_clk;
		while (epdc_pix_rate < rounded_parent_rate)
			epdc_pix_rate *= 2;
		clk_set_rate(epdc_parent, epdc_pix_rate);

		rounded_pix_clk = clk_round_rate(priv->epdc_clk_pix, target_pix_clk);
		if (((rounded_pix_clk >= target_pix_clk + target_pix_clk/100) ||
			(rounded_pix_clk <= target_pix_clk - target_pix_clk/100)))
			/* Still can't get a good clock, provide warning */
			dev_err(priv->drm.dev, "Unable to get an accurate EPDC pix clk"
				"desired = %lu, actual = %lu\n", target_pix_clk,
				rounded_pix_clk);
	}

	clk_set_rate(priv->epdc_clk_pix, rounded_pix_clk);
	clk_prepare_enable(priv->epdc_clk_pix);

	epdc_init_settings(priv, m);

	priv->in_init = true;
	mxc_epdc_powerup(priv);
	mxc_epdc_draw_mode0(priv);
	/* Force power down event */
	priv->powering_down = true;
	mxc_epdc_powerdown(priv);
	priv->updates_active = false;

	/* Disable clocks */
	clk_disable_unprepare(priv->epdc_clk_axi);
	clk_disable_unprepare(priv->epdc_clk_pix);
	priv->hw_ready = true;
	priv->hw_initializing = false;

}

int mxc_epdc_init_hw(struct mxc_epdc *priv)
{
	struct pinctrl *pinctrl;
	const char *thermal = NULL;
	u32 val;

	/* get pmic regulators */
	priv->display_regulator = devm_regulator_get(priv->drm.dev, "DISPLAY");
	if (IS_ERR(priv->display_regulator))
		return dev_err_probe(priv->drm.dev, PTR_ERR(priv->display_regulator),
				     "Unable to get display PMIC regulator\n");

	priv->vcom_regulator = devm_regulator_get(priv->drm.dev, "VCOM");
	if (IS_ERR(priv->vcom_regulator))
		return dev_err_probe(priv->drm.dev, PTR_ERR(priv->vcom_regulator),
				     "Unable to get VCOM regulator\n");

	priv->v3p3_regulator = devm_regulator_get(priv->drm.dev, "V3P3");
	if (IS_ERR(priv->v3p3_regulator))
		return dev_err_probe(priv->drm.dev, PTR_ERR(priv->vcom_regulator),
				     "Unable to get V3P3 regulator\n");

	of_property_read_string(priv->drm.dev->of_node,
				"epd-thermal-zone", &thermal);
	if (thermal) {
		priv->thermal = thermal_zone_get_zone_by_name(thermal);
		if (IS_ERR(priv->thermal))
			return dev_err_probe(priv->drm.dev, PTR_ERR(priv->thermal),
					     "unable to get thermal");
	}
	priv->iobase = devm_platform_get_and_ioremap_resource(to_platform_device(priv->drm.dev),
							      0, NULL);
	if (priv->iobase == NULL)
		return -ENOMEM;

//	priv->auto_mode = AUTO_UPDATE_MODE_REGION_MODE;

	priv->epdc_clk_axi = devm_clk_get(priv->drm.dev, "epdc_axi");
	if (IS_ERR(priv->epdc_clk_axi))
		return dev_err_probe(priv->drm.dev, PTR_ERR(priv->epdc_clk_axi),
				     "Unable to get EPDC AXI clk\n");

	priv->epdc_clk_pix = devm_clk_get(priv->drm.dev, "epdc_pix");
	if (IS_ERR(priv->epdc_clk_pix))
		return dev_err_probe(priv->drm.dev, PTR_ERR(priv->epdc_clk_pix),
				     "Unable to get EPDC pix clk\n");

	clk_prepare_enable(priv->epdc_clk_axi);
	val = epdc_read(priv, EPDC_VERSION);
	clk_disable_unprepare(priv->epdc_clk_axi);
	priv->rev = ((val & EPDC_VERSION_MAJOR_MASK) >>
				EPDC_VERSION_MAJOR_OFFSET) * 10
			+ ((val & EPDC_VERSION_MINOR_MASK) >>
				EPDC_VERSION_MINOR_OFFSET);
	dev_dbg(priv->drm.dev, "EPDC version = %d\n", priv->rev);

	/* Initialize EPDC pins */
	pinctrl = devm_pinctrl_get_select_default(priv->drm.dev);
	if (IS_ERR(pinctrl)) {
		dev_err(priv->drm.dev, "can't get/select pinctrl\n");
		return PTR_ERR(pinctrl);
	}

	mutex_init(&priv->power_mutex);

	return 0;
}
