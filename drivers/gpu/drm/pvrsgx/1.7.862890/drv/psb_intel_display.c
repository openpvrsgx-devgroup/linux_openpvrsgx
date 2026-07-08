/*
 * Copyright Â© 2011 Intel Corporation
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
 * Authors:
 *	Eric Anholt <eric@anholt.net>
 */

#include <linux/i2c.h>
#include <linux/pm_runtime.h>

#include <drm/drmP.h>
#include "psb_fb.h"
#include "psb_drv.h"
#include "psb_intel_drv.h"
#include "psb_intel_reg.h"
#include "psb_intel_display.h"
#include "psb_powermgmt.h"


struct psb_intel_range_t {
	int min, max;
};

struct psb_intel_p2_t {
	int dot_limit;
	int p2_slow, p2_fast;
};

#define INTEL_P2_NUM		      2

struct psb_intel_limit_t {
	struct psb_intel_range_t dot, vco, n, m, m1, m2, p, p1;
	struct psb_intel_p2_t p2;
	bool (* find_pll)(const struct psb_intel_limit_t *, struct drm_crtc *,
				int, int, struct psb_intel_clock_t *);
};

static bool cdv_intel_find_best_PLL(const struct psb_intel_limit_t *limit,
					struct drm_crtc *crtc, int target,
					int refclk,
					struct psb_intel_clock_t *best_clock);
static bool cdv_intel_find_dp_pll(const struct psb_intel_limit_t *limit, struct drm_crtc *crtc, int target,
				int refclk,
				struct psb_intel_clock_t *best_clock);

#define I8XX_DOT_MIN		  25000
#define I8XX_DOT_MAX		 350000
#define I8XX_VCO_MIN		 930000
#define I8XX_VCO_MAX		1400000
#define I8XX_N_MIN		      3
#define I8XX_N_MAX		     16
#define I8XX_M_MIN		     96
#define I8XX_M_MAX		    140
#define I8XX_M1_MIN		     18
#define I8XX_M1_MAX		     26
#define I8XX_M2_MIN		      6
#define I8XX_M2_MAX		     16
#define I8XX_P_MIN		      4
#define I8XX_P_MAX		    128
#define I8XX_P1_MIN		      2
#define I8XX_P1_MAX		     33
#define I8XX_P1_LVDS_MIN	      1
#define I8XX_P1_LVDS_MAX	      6
#define I8XX_P2_SLOW		      4
#define I8XX_P2_FAST		      2
#define I8XX_P2_LVDS_SLOW	      14
#define I8XX_P2_LVDS_FAST	      14	/* No fast option */
#define I8XX_P2_SLOW_LIMIT	 165000

#define I9XX_DOT_MIN		  20000
#define I9XX_DOT_MAX		 400000
#define I9XX_VCO_MIN		1400000
#define I9XX_VCO_MAX		2800000
#define I9XX_N_MIN		      3
#define I9XX_N_MAX		      8
#define I9XX_M_MIN		     70
#define I9XX_M_MAX		    120
#define I9XX_M1_MIN		     10
#define I9XX_M1_MAX		     20
#define I9XX_M2_MIN		      5
#define I9XX_M2_MAX		      9
#define I9XX_P_SDVO_DAC_MIN	      5
#define I9XX_P_SDVO_DAC_MAX	     80
#define I9XX_P_LVDS_MIN		      7
#define I9XX_P_LVDS_MAX		     98
#define I9XX_P1_MIN		      1
#define I9XX_P1_MAX		      8
#define I9XX_P2_SDVO_DAC_SLOW		     10
#define I9XX_P2_SDVO_DAC_FAST		      5
#define I9XX_P2_SDVO_DAC_SLOW_LIMIT	 200000
#define I9XX_P2_LVDS_SLOW		     14
#define I9XX_P2_LVDS_FAST		      7
#define I9XX_P2_LVDS_SLOW_LIMIT		 112000

#define INTEL_LIMIT_I8XX_DVO_DAC    0
#define INTEL_LIMIT_I8XX_LVDS	    1
#define INTEL_LIMIT_I9XX_SDVO_DAC   2
#define INTEL_LIMIT_I9XX_LVDS	    3

#define CDV_LIMIT_SINGLE_LVDS_96	0
#define CDV_LIMIT_SINGLE_LVDS_100	1
#define CDV_LIMIT_DAC_HDMI_27		2
#define CDV_LIMIT_DAC_HDMI_96		3
#define CDV_LIMIT_DP_27			4
#define CDV_LIMIT_DP_100		5

static const struct psb_intel_limit_t cdv_intel_limits[] = {
	{			/* CDV_SIGNLE_LVDS_96MHz */
	 .dot = {.min = 20000, .max = 115500},
	 .vco = {.min = 1800000, .max = 3600000},
	 .n = {.min = 2, .max = 6},
	 .m = {.min = 60, .max = 160},
	 .m1 = {.min = 0, .max = 0},
	 .m2 = {.min = 58, .max = 158},
	 .p = {.min = 28, .max = 140},
	 .p1 = {.min = 2, .max = 10},
	 .p2 = {.dot_limit = 200000,
		.p2_slow = 14, .p2_fast = 14},
	 .find_pll = cdv_intel_find_best_PLL,
	 },
	{			/* CDV_SINGLE_LVDS_100MHz */
	 .dot = {.min = 20000, .max = 115500},
	 .vco = {.min = 1800000, .max = 3600000},
	 .n = {.min = 2, .max = 6},
	 .m = {.min = 60, .max = 160},
	 .m1 = {.min = 0, .max = 0},
	 .m2 = {.min = 58, .max = 158},
	 .p = {.min = 28, .max = 140},
	 .p1 = {.min = 2, .max = 10},
	 /* The single-channel range is 25-112Mhz, and dual-channel
	  * is 80-224Mhz.  Prefer single channel as much as possible.
	  */
	 .p2 = {.dot_limit = 200000, .p2_slow = 14, .p2_fast = 14},
	 .find_pll = cdv_intel_find_best_PLL,
	 },
	{			/* CDV_DAC_HDMI_27MHz */
	 .dot = {.min = 20000, .max = 400000},
	 .vco = {.min = 1809000, .max = 3564000},
	 .n = {.min = 1, .max = 1},
	 .m = {.min = 67, .max = 132},
	 .m1 = {.min = 0, .max = 0},
	 .m2 = {.min = 65, .max = 130},
	 .p = {.min = 5, .max = 90},
	 .p1 = {.min = 1, .max = 9},
	 .p2 = {.dot_limit = 225000, .p2_slow = 10, .p2_fast = 5},
	 .find_pll = cdv_intel_find_best_PLL,
	 },
	{			/* CDV_DAC_HDMI_96MHz */
	 .dot = {.min = 20000, .max = 400000},
	 .vco = {.min = 1800000, .max = 3600000},
	 .n = {.min = 2, .max = 6},
	 .m = {.min = 60, .max = 160},
	 .m1 = {.min = 0, .max = 0},
	 .m2 = {.min = 58, .max = 158},
	 .p = {.min = 5, .max = 100},
	 .p1 = {.min = 1, .max = 10},
	 .p2 = {.dot_limit = 225000, .p2_slow = 10, .p2_fast = 5},
	 .find_pll = cdv_intel_find_best_PLL,
	 },
	{			/* CDV_DP_27MHz */
	 .dot = {.min = 160000, .max = 272000},
	 .vco = {.min = 1809000, .max = 3564000},
	 .n = {.min = 1, .max = 1},
	 .m = {.min = 67, .max = 132},
	 .m1 = {.min = 0, .max = 0},
	 .m2 = {.min = 65, .max = 130},
	 .p = {.min = 5, .max = 90},
	 .p1 = {.min = 1, .max = 9},
	 .p2 = {.dot_limit = 225000, .p2_slow = 10, .p2_fast = 10},
	 .find_pll = cdv_intel_find_dp_pll,
	 },
	{			/* CDV_DP_100MHz */
	 .dot = {.min = 160000, .max = 272000},
	 .vco = {.min = 1800000, .max = 3600000},
	 .n = {.min = 2, .max = 6},
	 .m = {.min = 60, .max = 164},
	 .m1 = {.min = 0, .max = 0},
	 .m2 = {.min = 58, .max = 162},
	 .p = {.min = 5, .max = 100},
	 .p1 = {.min = 1, .max = 10},
	 .p2 = {.dot_limit = 225000, .p2_slow = 10, .p2_fast = 10},
	 .find_pll = cdv_intel_find_dp_pll,
	 }	
};

static const struct psb_intel_limit_t *cdv_intel_limit(struct drm_crtc *crtc, int refclk)
{
	const struct psb_intel_limit_t *limit;

	DRM_DEBUG_KMS("ref clk is %d\n", refclk);

	if (psb_intel_pipe_has_type(crtc, INTEL_OUTPUT_LVDS)) {
		/*
 		 * Now only single-channel LVDS is supported on CDV. If it is
 		 * incorrect, please add the dual-channel LVDS.
 		 */
		if (refclk == 96000)
			limit = &cdv_intel_limits[CDV_LIMIT_SINGLE_LVDS_96];
		else 
			limit = &cdv_intel_limits[CDV_LIMIT_SINGLE_LVDS_100];
	} else if (psb_intel_pipe_has_type(crtc, INTEL_OUTPUT_DISPLAYPORT) ||
			psb_intel_pipe_has_type(crtc, INTEL_OUTPUT_EDP)) {
		if (refclk == 27000)
			limit = &cdv_intel_limits[CDV_LIMIT_DP_27];
		else
			limit = &cdv_intel_limits[CDV_LIMIT_DP_100];
	} else {
		if (refclk == 27000)
			limit = &cdv_intel_limits[CDV_LIMIT_DAC_HDMI_27];
		else
			limit = &cdv_intel_limits[CDV_LIMIT_DAC_HDMI_96];
	}
	return limit;
}
/* On the CDV the M is written, which is similar to that on Pineview */
/*static bool single_m_multiplier(struct drm_device *dev)
{
	return IS_CDV(dev);
}
*/

/** Derive the pixel clock for the given refclk and divisors for 8xx chips. */
static void i8xx_clock(int refclk, struct psb_intel_clock_t *clock)
{
	clock->m = 5 * (clock->m1 + 2) + (clock->m2 + 2);
	clock->p = clock->p1 * clock->p2;
	clock->vco = refclk * clock->m / (clock->n + 2);
	clock->dot = clock->vco / clock->p;
}


/* m1 is reserved as 0 in CDV, n is a ring counter */
static void cdv_intel_clock(struct drm_device *dev, 
			int refclk, struct psb_intel_clock_t *clock)
{
	clock->m = clock->m2 + 2;
	clock->p = clock->p1 * clock->p2;
	clock->vco = (refclk * clock->m) / clock->n;
	clock->dot = clock->vco / clock->p;
}

/**
 * Returns whether any output on the specified pipe is of the specified type
 */
bool psb_intel_pipe_has_type(struct drm_crtc *crtc, int type)
{
	struct drm_device *dev = crtc->dev;
	struct drm_mode_config *mode_config = &dev->mode_config;
	struct drm_connector *l_entry;

	list_for_each_entry(l_entry, &mode_config->connector_list, head) {
		if (l_entry->encoder && l_entry->encoder->crtc == crtc) {
			struct psb_intel_output *psb_intel_output =
			    to_psb_intel_output(l_entry);
			if (psb_intel_output->type == type)
				return true;
		}
	}
	return false;
}

#define INTELPllInvalid(s)   { /* ErrorF (s) */; return false; }
/**
 * Returns whether the given set of divisors are valid for a given refclk with
 * the given connectors.
 */

static bool cdv_intel_PLL_is_valid(struct drm_crtc *crtc,
				const struct psb_intel_limit_t *limit,
			       struct psb_intel_clock_t *clock)
{
	if (clock->p1 < limit->p1.min || limit->p1.max < clock->p1)
		INTELPllInvalid("p1 out of range\n");
	if (clock->p < limit->p.min || limit->p.max < clock->p)
		INTELPllInvalid("p out of range\n");
	/* unnecessary to check the range of m(m1/M2)/n again */
	/*
	if (clock->m2 < limit->m2.min || limit->m2.max < clock->m2)
		INTELPllInvalid("m2 out of range\n");
	if (clock->m < limit->m.min || limit->m.max < clock->m)
		INTELPllInvalid("m out of range\n");
	if (clock->n < limit->n.min || limit->n.max < clock->n)
		INTELPllInvalid("n out of range\n");
	*/
	if (clock->vco < limit->vco.min || limit->vco.max < clock->vco)
		INTELPllInvalid("vco out of range\n");
	/* XXX: We may need to be checking "Dot clock"
	 * depending on the multiplier, connector, etc.,
	 * rather than just a single range.
	 */
	if (clock->dot < limit->dot.min || limit->dot.max < clock->dot)
		INTELPllInvalid("dot out of range\n");

	return true;
}

static bool cdv_intel_find_best_PLL(const struct psb_intel_limit_t *limit, struct drm_crtc *crtc, int target,
				int refclk,
				struct psb_intel_clock_t *best_clock)
{
	struct drm_device *dev = crtc->dev;
	struct psb_intel_clock_t clock;
	int err = target;


	if (psb_intel_pipe_has_type(crtc, INTEL_OUTPUT_LVDS) &&
	    (REG_READ(LVDS) & LVDS_PORT_EN) != 0) {
		/*
		 * For LVDS, if the panel is on, just rely on its current
		 * settings for dual-channel.  We haven't figured out how to
		 * reliably set up different single/dual channel state, if we
		 * even can.
		 */
		if ((REG_READ(LVDS) & LVDS_CLKB_POWER_MASK) ==
		    LVDS_CLKB_POWER_UP)
			clock.p2 = limit->p2.p2_fast;
		else
			clock.p2 = limit->p2.p2_slow;
	} else {
		if (target < limit->p2.dot_limit)
			clock.p2 = limit->p2.p2_slow;
		else
			clock.p2 = limit->p2.p2_fast;
	}

	memset(best_clock, 0, sizeof(*best_clock));
	clock.m1 = 0;
	/* m1 is reserved as 0 in CDV, n is a ring counter. So skip the m1 loop */
	for (clock.n = limit->n.min; clock.n <= limit->n.max; clock.n++) {
		for (clock.m2 = limit->m2.min; clock.m2 <= limit->m2.max;
					     clock.m2++) {
			for (clock.p1 = limit->p1.min; clock.p1 <= limit->p1.max;
				     clock.p1++) {
				int this_err;

				cdv_intel_clock(dev, refclk, &clock);

				if (!cdv_intel_PLL_is_valid(crtc, limit, &clock))
						continue;
				
				this_err = abs(clock.dot - target);
				if (this_err < err) {
					*best_clock = clock;
					err = this_err;
				}
			}
		}
	}

	return err != target;
}

static bool cdv_intel_find_dp_pll(const struct psb_intel_limit_t *limit, struct drm_crtc *crtc, int target,
				int refclk,
				struct psb_intel_clock_t *best_clock)
{
	struct psb_intel_clock_t clock;
	if (refclk == 27000) {
		if (target < 200000) {
			clock.p1 = 2;
			clock.p2 = 10;
			clock.n = 1;
			clock.m1 = 0;
			clock.m2 = 118;
		} else {
			clock.p1 = 1;
			clock.p2 = 10;
			clock.n = 1;
			clock.m1 = 0;
			clock.m2 = 98;
		}
	} else if (refclk == 100000) {
		if (target < 200000) {
			clock.p1 = 2;
			clock.p2 = 10;
			clock.n = 5;
			clock.m1 = 0;
			clock.m2 = 160;
		} else {
			clock.p1 = 1;
			clock.p2 = 10;
			clock.n = 5;
			clock.m1 = 0;
			clock.m2 = 133;
		}
	} else
		return false;
	clock.m = clock.m2 + 2;
	clock.p = clock.p1 * clock.p2;
	clock.vco = (refclk * clock.m) / clock.n;
	clock.dot = clock.vco / clock.p;
	memcpy(best_clock, &clock, sizeof(struct psb_intel_clock_t));
	return true;
		
}

void psb_intel_wait_for_vblank(struct drm_device *dev)
{
	/* Wait for 20ms, i.e. one cycle at 50hz. */
	mdelay(20);
}

static bool psb_intel_pipe_enabled(struct drm_device *dev, int pipe)
{
	struct drm_crtc *crtc;
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_intel_crtc *psb_intel_crtc = NULL;

	crtc = dev_priv->pipe_to_crtc_mapping[pipe];
	psb_intel_crtc = to_psb_intel_crtc(crtc);

	if (crtc->fb == NULL || !psb_intel_crtc->active) {
		return false;
	}
	return true;
}

#define		FIFO_PIPEA		(1 << 0)
#define		FIFO_PIPEB		(1 << 1)

static bool psb_intel_single_pipe_active (struct drm_device *dev)
{
	uint32_t pipe_enabled = 0;

	if (psb_intel_pipe_enabled(dev, 0)) {
		pipe_enabled |= FIFO_PIPEA;
	}
	if (psb_intel_pipe_enabled(dev, 1)) {
		pipe_enabled |= FIFO_PIPEB;
	}

	DRM_DEBUG_KMS("pipe enabled %x\n", pipe_enabled);

	if ((pipe_enabled == FIFO_PIPEA) || (pipe_enabled == FIFO_PIPEB))
		return true;
	else
		return false;
}

static bool is_pipeb_lvds(struct drm_device *dev, struct drm_crtc *crtc)
{
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
	struct drm_mode_config *mode_config = &dev->mode_config;
	struct drm_connector *connector;

	if (psb_intel_crtc->pipe != 1)
		return false;

	list_for_each_entry(connector, &mode_config->connector_list, head) {
		struct psb_intel_output *psb_intel_output =
		    to_psb_intel_output(connector);

		if (!connector->encoder
		    || connector->encoder->crtc != crtc)
			continue;

		if (psb_intel_output->type == INTEL_OUTPUT_LVDS)
			return true;
	}

	return false;
}

static void psb_intel_disable_self_refresh (struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;

	if (REG_READ(FW_BLC_SELF) & FW_BLC_SELF_EN) {

		/* Disable self-refresh before adjust WM */
		REG_WRITE(FW_BLC_SELF, (REG_READ(FW_BLC_SELF) & ~FW_BLC_SELF_EN));
		REG_READ(FW_BLC_SELF);

		psb_intel_wait_for_vblank(dev);

		/* Cedarview workaround to write ovelay plane, which force to leave
		 * MAX_FIFO state.
		 */
		REG_WRITE(OV_OVADD, dev_priv->ovl_offset);
		REG_READ(OV_OVADD);

		psb_intel_wait_for_vblank(dev);
	}

}

static void psb_intel_update_watermark (struct drm_device *dev, struct drm_crtc *crtc)
{

	if (psb_intel_single_pipe_active(dev)) {
		u32 fw;

		fw = REG_READ(DSPFW1);
		fw &= ~DSP_FIFO_SR_WM_MASK;
		fw |= (0x7e << DSP_FIFO_SR_WM_SHIFT);
		fw &= ~CURSOR_B_FIFO_WM_MASK;
		fw |= (0x4 << CURSOR_B_FIFO_WM_SHIFT);
		REG_WRITE(DSPFW1, fw);

		fw = REG_READ(DSPFW2);
		fw &= ~CURSOR_A_FIFO_WM_MASK;
		fw |= (0x6 << CURSOR_A_FIFO_WM_SHIFT);
		fw &= ~DSP_PLANE_C_FIFO_WM_MASK;
		fw |= (0x8 << DSP_PLANE_C_FIFO_WM_SHIFT);
		REG_WRITE(DSPFW2, fw);

		REG_WRITE(DSPFW3, 0x36000000);

		/* ignore FW4 */

		if (is_pipeb_lvds(dev, crtc)) {
			REG_WRITE(DSPFW5, 0x00040330);
		} else {
			fw = (3 << DSP_PLANE_B_FIFO_WM1_SHIFT) |
			     (4 << DSP_PLANE_A_FIFO_WM1_SHIFT) |
			     (3 << CURSOR_B_FIFO_WM1_SHIFT) |
			     (4 << CURSOR_FIFO_SR_WM1_SHIFT);
			REG_WRITE(DSPFW5, fw);
		}

		REG_WRITE(DSPFW6, 0x10);

		psb_intel_wait_for_vblank(dev);

		/* enable self-refresh for single pipe active */
		REG_WRITE(FW_BLC_SELF, FW_BLC_SELF_EN);
		REG_READ(FW_BLC_SELF);
		psb_intel_wait_for_vblank(dev);

	} else {

		/* HW team suggested values... */
		REG_WRITE(DSPFW1, 0x3f880808);
		REG_WRITE(DSPFW2, 0x0b020202);
		REG_WRITE(DSPFW3, 0x24000000);
		REG_WRITE(DSPFW4, 0x08030202);
		REG_WRITE(DSPFW5, 0x01010101);
		REG_WRITE(DSPFW6, 0x1d0);

		psb_intel_wait_for_vblank(dev);

		psb_intel_disable_self_refresh(dev);
	
	}
}

int psb_intel_pipe_set_base(struct drm_crtc *crtc,
			    int x, int y, struct drm_framebuffer *old_fb)
{
	struct drm_device *dev = crtc->dev;
	/* struct drm_i915_master_private *master_priv; */
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
	struct psb_framebuffer *psbfb = to_psb_fb(crtc->fb);
	struct psb_intel_mode_device *mode_dev = psb_intel_crtc->mode_dev;
	int pipe = psb_intel_crtc->pipe;
	unsigned long Start, Offset;
	int dspbase = (pipe == 0 ? DSPABASE : DSPBBASE);
	int dspsurf = (pipe == 0 ? DSPASURF : DSPBSURF);
	int dspstride = (pipe == 0) ? DSPASTRIDE : DSPBSTRIDE;
	int dspcntr_reg = (pipe == 0) ? DSPACNTR : DSPBCNTR;
	u32 dspcntr;
	int ret = 0;

	PSB_DEBUG_ENTRY("\n");

	/* no fb bound */
	if (!crtc->fb) {
		DRM_DEBUG_KMS("No FB bound\n");
		return 0;
	}

	Start = mode_dev->bo_offset(dev, psbfb);
	Offset = y * crtc->fb->pitch + x * (crtc->fb->bits_per_pixel / 8);

	REG_WRITE(dspstride, crtc->fb->pitch);

	dspcntr = REG_READ(dspcntr_reg);
	dspcntr &= ~DISPPLANE_PIXFORMAT_MASK;

	switch (crtc->fb->bits_per_pixel) {
	case 8:
		dspcntr |= DISPPLANE_8BPP;
		break;
	case 16:
		if (crtc->fb->depth == 15)
			dspcntr |= DISPPLANE_15_16BPP;
		else
			dspcntr |= DISPPLANE_16BPP;
		break;
	case 24:
	case 32:
		dspcntr |= DISPPLANE_32BPP_NO_ALPHA;
		break;
	default:
		DRM_ERROR("Unknown color depth\n");
		ret = -EINVAL;
		goto psb_intel_pipe_set_base_exit;
	}
	REG_WRITE(dspcntr_reg, dspcntr);

	DRM_DEBUG_KMS("Writing base %08lX %08lX %d %d for FB ID %d\n", Start, Offset, x, y, psbfb->base.base.id);
	if (IS_CDV(dev)) {
		REG_WRITE(dspbase, Offset);
		REG_READ(dspbase);
		REG_WRITE(dspsurf, Start);
		REG_READ(dspsurf);
	} else {
		REG_WRITE(dspbase, Start + Offset);
		REG_READ(dspbase);
	}

psb_intel_pipe_set_base_exit:

	return ret;
}

#define CRTC_FLIP_TIMEOUT	100	/* ms */
#define MAX_FLIP_COUNTER	100

#define CRTC_PIPEA		0
#define CRTC_PIPEB		1
#define PIPEA_ENABLED		(1 << 0)
#define PIPEB_ENABLED		(1 << 1)

static void psb_flip_timer(unsigned long arg)
{
	struct drm_device *dev = (struct drm_device *) arg;
	struct drm_psb_private *dev_priv = (struct drm_psb_private *)(dev->dev_private);

	if (dev_priv->psb_vsync_handler)
		(dev_priv->psb_vsync_handler)(dev, dev_priv->ui32MainPipe);

	dev_priv->flip_counter--;
	if (dev_priv->vblanksEnabledForFlips && !dev_priv->pipeEnabled &&
			dev_priv->flip_counter) {
		mod_timer(&dev_priv->flip_timer, jiffies +
			 msecs_to_jiffies(CRTC_FLIP_TIMEOUT));
	}
}


/**
 * Sets the power management mode of the pipe and plane.
 *
 * This code should probably grow support for turning the cursor off and back
 * on appropriately at the same time as we're turning the pipe off/on.
 */
static void psb_intel_crtc_dpms(struct drm_crtc *crtc, int mode)
{
	struct drm_device *dev = crtc->dev;
	/* struct drm_i915_master_private *master_priv; */
	struct drm_psb_private *dev_priv = dev->dev_private; 
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
	int pipe = psb_intel_crtc->pipe;
	int dpll_reg = (pipe == 0) ? DPLL_A : DPLL_B;
	int dspcntr_reg = (pipe == 0) ? DSPACNTR : DSPBCNTR;
	int dspbase_reg = (pipe == 0) ? DSPABASE : DSPBBASE;
	int pipeconf_reg = (pipe == 0) ? PIPEACONF : PIPEBCONF;
	int pipestat_reg = (pipe == 0) ? PIPEASTAT : PIPEBSTAT;
	u32 temp;
	u32 target_pipe;

	/* XXX: When our outputs are all unaware of DPMS modes other than off
	 * and on, we should map those modes to DRM_MODE_DPMS_OFF in the CRTC.
	 */

	psb_intel_disable_self_refresh(dev);
	
	switch (mode) {
	case DRM_MODE_DPMS_ON:
	case DRM_MODE_DPMS_STANDBY:
	case DRM_MODE_DPMS_SUSPEND:
		DRM_DEBUG_KMS("Turning on display in dpms on pipe: %d\n", pipe);

		if (psb_intel_crtc->active)
			break;

		psb_intel_crtc->active = true;

		/* Enable the DPLL */
		temp = REG_READ(dpll_reg);
		if ((temp & DPLL_VCO_ENABLE) == 0) {
			REG_WRITE(dpll_reg, temp);
			REG_READ(dpll_reg);
			/* Wait for the clocks to stabilize. */
			udelay(150);
			REG_WRITE(dpll_reg, temp | DPLL_VCO_ENABLE);
			REG_READ(dpll_reg);
			/* Wait for the clocks to stabilize. */
			udelay(150);
			REG_WRITE(dpll_reg, temp | DPLL_VCO_ENABLE);
			REG_READ(dpll_reg);
			/* Wait for the clocks to stabilize. */
			udelay(150);
		}

		/* Jim Bish - switch plan and pipe per scott */
		/* Enable the plane */
		temp = REG_READ(dspcntr_reg);
		if ((temp & DISPLAY_PLANE_ENABLE) == 0) {
			REG_WRITE(dspcntr_reg,
				  temp | DISPLAY_PLANE_ENABLE);
			/* Flush the plane changes */
			REG_WRITE(dspbase_reg, REG_READ(dspbase_reg));
		}

		udelay(150);

		/* Enable the pipe */
		temp = REG_READ(pipeconf_reg);
		if ((temp & PIPEACONF_ENABLE) == 0)
			REG_WRITE(pipeconf_reg, temp | PIPEACONF_ENABLE);

		temp = REG_READ(pipestat_reg);
		temp &= ~(0xFFFF);
		temp |= PIPE_FIFO_UNDERRUN;
		REG_WRITE(pipestat_reg, temp);
		REG_READ(pipestat_reg);
		
		psb_intel_crtc_load_lut(crtc);

		dev_priv->pipeEnabled |= (1 << pipe);
		/* Give the overlay scaler a chance to enable
		 * if it's on this pipe */
		/* psb_intel_crtc_dpms_video(crtc, true); TODO */
		break;
	case DRM_MODE_DPMS_OFF:
		DRM_DEBUG_KMS("Turning off display in dpms on pipe: %d\n", pipe);
		if (!psb_intel_crtc->active)
			break;

		psb_intel_crtc->active = false;

		/* Give the overlay scaler a chance to disable
		 * if it's on this pipe */
		/* psb_intel_crtc_dpms_video(crtc, FALSE); TODO */

		/* Disable the VGA plane that we never use */
		REG_WRITE(VGACNTRL, VGA_DISP_DISABLE);

		drm_vblank_off(dev, pipe);

		if (!IS_I9XX(dev)) {
			/* Wait for vblank for the disable to take effect */
			psb_intel_wait_for_vblank(dev);
		}

		/* Next, disable display pipes */
		temp = REG_READ(pipeconf_reg);
		if ((temp & PIPEACONF_ENABLE) != 0) {
			REG_WRITE(pipeconf_reg, temp & ~PIPEACONF_ENABLE);
			REG_READ(pipeconf_reg);
		}

		/* Wait for vblank for the disable to take effect. */
		psb_intel_wait_for_vblank(dev);

		/* Disable display plane */
		temp = REG_READ(dspcntr_reg);
		if ((temp & DISPLAY_PLANE_ENABLE) != 0) {
			REG_WRITE(dspcntr_reg,
				  temp & ~DISPLAY_PLANE_ENABLE);
			/* Flush the plane changes */
			REG_WRITE(dspbase_reg, REG_READ(dspbase_reg));
			REG_READ(dspbase_reg);
		}

		temp = REG_READ(dpll_reg);
		if ((temp & DPLL_VCO_ENABLE) != 0) {
			REG_WRITE(dpll_reg, temp & ~DPLL_VCO_ENABLE);
			REG_READ(dpll_reg);
		}

		/* Wait for the clocks to turn off. */
		udelay(150);

		dev_priv->pipeEnabled &= ~(1 << pipe);
		break;
	}
	
	psb_intel_update_watermark(dev, crtc);

	if ((dev_priv->pipeEnabled == PIPEA_ENABLED) || 
			(dev_priv->pipeEnabled == PIPEB_ENABLED)) {
		target_pipe = CRTC_PIPEA;
		if (dev_priv->pipeEnabled == PIPEB_ENABLED)
			target_pipe = CRTC_PIPEB;
	} else {
		target_pipe = CRTC_PIPEB;
	}
	if (target_pipe != dev_priv->ui32MainPipe) {
		DRM_DEBUG_KMS("Main flip pipe is switched from %d to %d\n",
			dev_priv->ui32MainPipe, target_pipe);
		if (dev_priv->vblanksEnabledForFlips) {
			if (!dev->vblank_enabled[dev_priv->ui32MainPipe])
				psb_disable_vblank(dev, dev_priv->ui32MainPipe);
			psb_enable_vblank(dev, target_pipe);
		}
		dev_priv->ui32MainPipe = target_pipe;
	}
	if (mode == DRM_MODE_DPMS_OFF) {
		if (dev_priv->vblanksEnabledForFlips && !dev_priv->pipeEnabled) {
			dev_priv->flip_counter =  MAX_FLIP_COUNTER;
			mod_timer(&dev_priv->flip_timer, jiffies +
				 msecs_to_jiffies(CRTC_FLIP_TIMEOUT));
		}
	} else {
		dev_priv->flip_counter = 0;
		del_timer(&dev_priv->flip_timer);
		
	}
}

static void psb_intel_crtc_prepare(struct drm_crtc *crtc)
{
	struct drm_crtc_helper_funcs *crtc_funcs = crtc->helper_private;
	crtc_funcs->dpms(crtc, DRM_MODE_DPMS_OFF);
}

static void psb_intel_crtc_commit(struct drm_crtc *crtc)
{
	struct drm_crtc_helper_funcs *crtc_funcs = crtc->helper_private;
	crtc_funcs->dpms(crtc, DRM_MODE_DPMS_ON);
}

void psb_intel_encoder_prepare(struct drm_encoder *encoder)
{
	struct drm_encoder_helper_funcs *encoder_funcs =
	    encoder->helper_private;
	/* lvds has its own version of prepare see psb_intel_lvds_prepare */
	encoder_funcs->dpms(encoder, DRM_MODE_DPMS_OFF);
}

void psb_intel_encoder_commit(struct drm_encoder *encoder)
{
	struct drm_encoder_helper_funcs *encoder_funcs =
	    encoder->helper_private;
	/* lvds has its own version of commit see psb_intel_lvds_commit */
	encoder_funcs->dpms(encoder, DRM_MODE_DPMS_ON);
}

static bool psb_intel_crtc_mode_fixup(struct drm_crtc *crtc,
				  struct drm_display_mode *mode,
				  struct drm_display_mode *adjusted_mode)
{
	return true;
}


/**
 * Return the pipe currently connected to the panel fitter,
 * or -1 if the panel fitter is not present or not in use
 */
static int psb_intel_panel_fitter_pipe(struct drm_device *dev)
{
	u32 pfit_control;

	/* i830 doesn't have a panel fitter */
	if (IS_I830(dev))
		return -1;

	pfit_control = REG_READ(PFIT_CONTROL);

	/* See if the panel fitter is in use */
	if ((pfit_control & PFIT_ENABLE) == 0)
		return -1;

	/* 965 can place panel fitter on either pipe */
	if (IS_I965G(dev) || IS_MID(dev))
		return (pfit_control >> 29) & 0x3;

	/* older chips can only use pipe 1 */
	return 1;
}

static int cdv_intel_crtc_mode_set(struct drm_crtc *crtc,
			       struct drm_display_mode *mode,
			       struct drm_display_mode *adjusted_mode,
			       int x, int y,
			       struct drm_framebuffer *old_fb)
{
	struct drm_device *dev = crtc->dev;
	struct drm_psb_private *dev_priv = dev->dev_private; 
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
	int pipe = psb_intel_crtc->pipe;
	int dpll_reg = (pipe == 0) ? DPLL_A : DPLL_B;
	int dpll_md_reg = (psb_intel_crtc->pipe == 0) ? DPLL_A_MD : DPLL_B_MD;
	int dspcntr_reg = (pipe == 0) ? DSPACNTR : DSPBCNTR;
	int pipeconf_reg = (pipe == 0) ? PIPEACONF : PIPEBCONF;
	int htot_reg = (pipe == 0) ? HTOTAL_A : HTOTAL_B;
	int hblank_reg = (pipe == 0) ? HBLANK_A : HBLANK_B;
	int hsync_reg = (pipe == 0) ? HSYNC_A : HSYNC_B;
	int vtot_reg = (pipe == 0) ? VTOTAL_A : VTOTAL_B;
	int vblank_reg = (pipe == 0) ? VBLANK_A : VBLANK_B;
	int vsync_reg = (pipe == 0) ? VSYNC_A : VSYNC_B;
	int dspsize_reg = (pipe == 0) ? DSPASIZE : DSPBSIZE;
	int dsppos_reg = (pipe == 0) ? DSPAPOS : DSPBPOS;
	int pipesrc_reg = (pipe == 0) ? PIPEASRC : PIPEBSRC;
	int refclk;
	struct psb_intel_clock_t clock;
	u32 dpll = 0, dspcntr, pipeconf;
	bool ok;
	bool is_crt = false, is_lvds = false, is_tv = false;
	bool is_hdmi = false, is_dp = false;
	struct drm_mode_config *mode_config = &dev->mode_config;
	struct drm_connector *connector;
	const struct psb_intel_limit_t *limit;
	u32	ddi_select = 0;
	bool is_edp = false;

	DRM_DEBUG_KMS("cdv_intel_crtc_mode_set called for pipe %d\n", pipe);

	list_for_each_entry(connector, &mode_config->connector_list, head) {
		struct psb_intel_output *psb_intel_output =
		    to_psb_intel_output(connector);

		if (!connector->encoder
		    || connector->encoder->crtc != crtc)
			continue;

		switch (psb_intel_output->type) {
		case INTEL_OUTPUT_LVDS:
			is_lvds = true;
			break;
		case INTEL_OUTPUT_TVOUT:
			is_tv = true;
			break;
		case INTEL_OUTPUT_ANALOG:
			is_crt = true;
			break;
		case INTEL_OUTPUT_HDMI:
			is_hdmi = true;
			ddi_select = psb_intel_output->ddi_select;
			break;
		case INTEL_OUTPUT_DISPLAYPORT:
			is_dp = true;
			ddi_select = psb_intel_output->ddi_select;
			break;
		case INTEL_OUTPUT_EDP:
			is_edp = true;
			ddi_select = psb_intel_output->ddi_select;
			break;
		default:
			DRM_ERROR("Unsupported output type!\n");
			return 0;
		}
	}

	if (dev_priv->dplla_96mhz)
		/* low-end sku, 96/100 mhz */
		refclk = 96000;
	else
		/* high-end sku, 27/100 mhz */
		refclk = 27000;
	if (is_dp || is_edp) {
		/*
		 * Based on the spec the low-end SKU has only CRT/LVDS. So it is
		 * unnecessary to consider it for DP/eDP.
		 * On the high-end SKU, it will use the 27/100M reference clk
		 * for DP/eDP. When using SSC clock, the ref clk is 100MHz.Otherwise
		 * it will be 27MHz. From the VBIOS code it seems that the pipe A choose
		 * 27MHz for DP/eDP while the Pipe B chooses the 100MHz.
		 */ 
		if (pipe == 0)
			refclk = 27000;
		else
			refclk = 100000;
	}

	if (is_lvds && dev_priv->lvds_use_ssc) {
		refclk = dev_priv->lvds_ssc_freq * 1000;
		DRM_DEBUG_KMS("Use SSC reference clock %d Mhz\n", dev_priv->lvds_ssc_freq);
	}

	DRM_DEBUG_KMS("Use reference clock %d Khz\n", refclk);

	drm_mode_debug_printmodeline(adjusted_mode);

	limit = cdv_intel_limit(crtc, refclk);
	ok = limit->find_pll(limit, crtc, adjusted_mode->clock, refclk,
				 &clock);
	if (!ok) {
		DRM_ERROR("Couldn't find PLL settings for mode!\n");
		return 0;
	}

	dpll = DPLL_VGA_MODE_DIS;
	if (is_tv) {
		/* XXX: just matching BIOS for now */
/*	dpll |= PLL_REF_INPUT_TVCLKINBC; */
		dpll |= 3;
	}
#if 0
	else if (is_lvds)
		dpll |= PLLB_REF_INPUT_SPREADSPECTRUMIN;

#endif

	if (is_dp || is_edp) {
		psb_intel_dp_set_m_n(crtc, mode, adjusted_mode);
	} else {
		REG_WRITE(PIPE_GMCH_DATA_M(pipe), 0);
		REG_WRITE(PIPE_GMCH_DATA_N(pipe), 0);
		REG_WRITE(PIPE_DP_LINK_M(pipe), 0);
		REG_WRITE(PIPE_DP_LINK_N(pipe), 0);
	}
		
	dpll |= DPLL_SYNCLOCK_ENABLE;

	/* setup pipeconf */
	pipeconf = REG_READ(pipeconf_reg);
	pipeconf &= ~(PIPE_BPC_MASK);
	if (is_edp) {
		switch (dev_priv->edp.bpp) {
		case 24:
			pipeconf |= PIPE_8BPC;
			break;
		case 18:
			pipeconf |= PIPE_6BPC;
			break;
		case 30:
			pipeconf |= PIPE_10BPC;
			break;
		default:
			pipeconf |= PIPE_8BPC;
			break;
		}
	} else if (is_lvds) {
		/* the BPC will be 6 if it is 18-bit LVDS panel */
		if ((REG_READ(LVDS) & LVDS_A3_POWER_MASK) == LVDS_A3_POWER_UP)
			pipeconf |= PIPE_8BPC;
		else
			pipeconf |= PIPE_6BPC;
	} else
		pipeconf |= PIPE_8BPC;
			
		
	/* Set up the display plane register */
	dspcntr = DISPPLANE_GAMMA_ENABLE;

	if (pipe == 0)
		dspcntr |= DISPPLANE_SEL_PIPE_A;
	else
		dspcntr |= DISPPLANE_SEL_PIPE_B;

	dspcntr |= DISPLAY_PLANE_ENABLE;
	pipeconf |= PIPEACONF_ENABLE;

	REG_WRITE(dpll_reg, dpll);
        REG_READ(dpll_reg);

	psb_dpll_set_clock_cdv(dev, crtc, &clock, is_lvds, ddi_select);

        udelay(150);

	/* The LVDS pin pair needs to be on before the DPLLs are enabled.
	 * This is an exception to the general rule that mode_set doesn't turn
	 * things on.
	 */
	if (is_lvds) {
		u32 lvds = REG_READ(LVDS);

		lvds |= LVDS_PORT_EN | LVDS_A0A2_CLKA_POWER_UP |
			LVDS_PIPEB_SELECT;
		/* Set the B0-B3 data pairs corresponding to
		 * whether we're going to
		 * set the DPLLs for dual-channel mode or not.
		 */
		if (clock.p2 == 7)
			lvds |= LVDS_B0B3_POWER_UP | LVDS_CLKB_POWER_UP;
		else
			lvds &= ~(LVDS_B0B3_POWER_UP | LVDS_CLKB_POWER_UP);

		/* It would be nice to set 24 vs 18-bit mode (LVDS_A3_POWER_UP)
		 * appropriately here, but we need to look more
		 * thoroughly into how panels behave in the two modes.
		 */

		REG_WRITE(LVDS, lvds);
		REG_READ(LVDS);
	}

	dpll |= DPLL_VCO_ENABLE;

		
	/* Disable the panel fitter if it was on our pipe */
	if (psb_intel_panel_fitter_pipe(dev) == pipe)
		REG_WRITE(PFIT_CONTROL, 0);

	DRM_DEBUG_KMS("Mode for pipe %c:\n", pipe == 0 ? 'A' : 'B');
	drm_mode_debug_printmodeline(mode);

	REG_WRITE(dpll_reg,
                   (REG_READ(dpll_reg) & ~DPLL_LOCK) |
                   DPLL_VCO_ENABLE);
        REG_READ(dpll_reg);
	/* Wait for the clocks to stabilize. */
        udelay(150); /* 42 usec w/o calibration, 110 with.  rounded up. */

        if (!(REG_READ(dpll_reg) & DPLL_LOCK)) {
                DRM_ERROR("Failed to get DPLL lock\n");
                return -EBUSY;
        }

	if (IS_CDV(dev)) {
		int sdvo_pixel_multiply =
		    adjusted_mode->clock / mode->clock;
		REG_WRITE(dpll_md_reg,
			  (0 << DPLL_MD_UDI_DIVIDER_SHIFT) |
			  ((sdvo_pixel_multiply -
			    1) << DPLL_MD_UDI_MULTIPLIER_SHIFT));
	}

	REG_WRITE(htot_reg, (adjusted_mode->crtc_hdisplay - 1) |
		  ((adjusted_mode->crtc_htotal - 1) << 16));
	REG_WRITE(hblank_reg, (adjusted_mode->crtc_hblank_start - 1) |
		  ((adjusted_mode->crtc_hblank_end - 1) << 16));
	REG_WRITE(hsync_reg, (adjusted_mode->crtc_hsync_start - 1) |
		  ((adjusted_mode->crtc_hsync_end - 1) << 16));
	REG_WRITE(vtot_reg, (adjusted_mode->crtc_vdisplay - 1) |
		  ((adjusted_mode->crtc_vtotal - 1) << 16));
	REG_WRITE(vblank_reg, (adjusted_mode->crtc_vblank_start - 1) |
		  ((adjusted_mode->crtc_vblank_end - 1) << 16));
	REG_WRITE(vsync_reg, (adjusted_mode->crtc_vsync_start - 1) |
		  ((adjusted_mode->crtc_vsync_end - 1) << 16));
	/* pipesrc and dspsize control the size that is scaled from,
	 * which should always be the user's requested size.
	 */
	REG_WRITE(dspsize_reg,
		  ((mode->vdisplay - 1) << 16) | (mode->hdisplay - 1));
	REG_WRITE(dsppos_reg, 0);
	REG_WRITE(pipesrc_reg,
		  ((mode->hdisplay - 1) << 16) | (mode->vdisplay - 1));
	REG_WRITE(pipeconf_reg, pipeconf);
	REG_READ(pipeconf_reg);

	psb_intel_wait_for_vblank(dev);

	REG_WRITE(dspcntr_reg, dspcntr);

	/* Flush the plane changes */
	{
		struct drm_crtc_helper_funcs *crtc_funcs =
		    crtc->helper_private;
		crtc_funcs->mode_set_base(crtc, x, y, old_fb);
	}

	psb_intel_wait_for_vblank(dev);

	return 0;
}

/** Loads the palette/gamma unit for the CRTC with the prepared values */
void psb_intel_crtc_load_lut(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
	int palreg = PALETTE_A;
	int i;

	/* The clocks have to be on to load the palette. */
	if (!crtc->enabled)
		return;

	switch (psb_intel_crtc->pipe) {
	case 0:
		palreg = PALETTE_A;
		break;
	case 1:
		palreg = PALETTE_B;
		break;
	default:
		DRM_ERROR("Illegal Pipe Number. \n");
		return;
	}
	
	for (i = 0; i < 256; i++) {
		REG_WRITE(palreg + 4 * i,
			  ((psb_intel_crtc->lut_r[i] +
			    psb_intel_crtc->lut_adj[i]) << 16) |
			  ((psb_intel_crtc->lut_g[i] +
			    psb_intel_crtc->lut_adj[i]) << 8) |
			  (psb_intel_crtc->lut_b[i] +
			   psb_intel_crtc->lut_adj[i]));
	}
}

/**
 * Save HW states of giving crtc
 */
static void psb_intel_crtc_save(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	/* struct drm_psb_private *dev_priv =
			(struct drm_psb_private *)dev->dev_private; */
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
	struct psb_intel_crtc_state *crtc_state = psb_intel_crtc->crtc_state;
	int pipeA = (psb_intel_crtc->pipe == 0);
	uint32_t paletteReg;
	int i;

	DRM_DEBUG("\n");

	if (!crtc_state) {
		DRM_DEBUG("No CRTC state found\n");
		return;
	}

	crtc_state->saveDSPCNTR = REG_READ(pipeA ? DSPACNTR : DSPBCNTR);
	crtc_state->savePIPECONF = REG_READ(pipeA ? PIPEACONF : PIPEBCONF);
	crtc_state->savePIPESRC = REG_READ(pipeA ? PIPEASRC : PIPEBSRC);
	crtc_state->saveFP0 = REG_READ(pipeA ? FPA0 : FPB0);
	crtc_state->saveFP1 = REG_READ(pipeA ? FPA1 : FPB1);
	crtc_state->saveDPLL = REG_READ(pipeA ? DPLL_A : DPLL_B);
	crtc_state->saveHTOTAL = REG_READ(pipeA ? HTOTAL_A : HTOTAL_B);
	crtc_state->saveHBLANK = REG_READ(pipeA ? HBLANK_A : HBLANK_B);
	crtc_state->saveHSYNC = REG_READ(pipeA ? HSYNC_A : HSYNC_B);
	crtc_state->saveVTOTAL = REG_READ(pipeA ? VTOTAL_A : VTOTAL_B);
	crtc_state->saveVBLANK = REG_READ(pipeA ? VBLANK_A : VBLANK_B);
	crtc_state->saveVSYNC = REG_READ(pipeA ? VSYNC_A : VSYNC_B);
	crtc_state->saveDSPSTRIDE = REG_READ(pipeA ? DSPASTRIDE : DSPBSTRIDE);

	/*NOTE: DSPSIZE DSPPOS only for psb*/
	crtc_state->saveDSPSIZE = REG_READ(pipeA ? DSPASIZE : DSPBSIZE);
	crtc_state->saveDSPPOS = REG_READ(pipeA ? DSPAPOS : DSPBPOS);

	crtc_state->saveDSPBASE = REG_READ(pipeA ? DSPABASE : DSPBBASE);

	DRM_DEBUG("(%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x)\n",
			crtc_state->saveDSPCNTR,
			crtc_state->savePIPECONF,
			crtc_state->savePIPESRC,
			crtc_state->saveFP0,
			crtc_state->saveFP1,
			crtc_state->saveDPLL,
			crtc_state->saveHTOTAL,
			crtc_state->saveHBLANK,
			crtc_state->saveHSYNC,
			crtc_state->saveVTOTAL,
			crtc_state->saveVBLANK,
			crtc_state->saveVSYNC,
			crtc_state->saveDSPSTRIDE,
			crtc_state->saveDSPSIZE,
			crtc_state->saveDSPPOS,
			crtc_state->saveDSPBASE
		);

	paletteReg = pipeA ? PALETTE_A : PALETTE_B;
	for (i = 0; i < 256; ++i)
		crtc_state->savePalette[i] = REG_READ(paletteReg + (i << 2));
}

/**
 * Restore HW states of giving crtc
 */
static void psb_intel_crtc_restore(struct drm_crtc *crtc)
{
	struct drm_device *dev = crtc->dev;
	/* struct drm_psb_private * dev_priv =
				(struct drm_psb_private *)dev->dev_private; */
	struct psb_intel_crtc *psb_intel_crtc =  to_psb_intel_crtc(crtc);
	struct psb_intel_crtc_state *crtc_state = psb_intel_crtc->crtc_state;
	/* struct drm_crtc_helper_funcs * crtc_funcs = crtc->helper_private; */
	int pipeA = (psb_intel_crtc->pipe == 0);
	uint32_t paletteReg;
	int i;

	DRM_DEBUG("\n");

	if (!crtc_state) {
		DRM_DEBUG("No crtc state\n");
		return;
	}

	DRM_DEBUG(
		"current:(%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x)\n",
		REG_READ(pipeA ? DSPACNTR : DSPBCNTR),
		REG_READ(pipeA ? PIPEACONF : PIPEBCONF),
		REG_READ(pipeA ? PIPEASRC : PIPEBSRC),
		REG_READ(pipeA ? FPA0 : FPB0),
		REG_READ(pipeA ? FPA1 : FPB1),
		REG_READ(pipeA ? DPLL_A : DPLL_B),
		REG_READ(pipeA ? HTOTAL_A : HTOTAL_B),
		REG_READ(pipeA ? HBLANK_A : HBLANK_B),
		REG_READ(pipeA ? HSYNC_A : HSYNC_B),
		REG_READ(pipeA ? VTOTAL_A : VTOTAL_B),
		REG_READ(pipeA ? VBLANK_A : VBLANK_B),
		REG_READ(pipeA ? VSYNC_A : VSYNC_B),
		REG_READ(pipeA ? DSPASTRIDE : DSPBSTRIDE),
		REG_READ(pipeA ? DSPASIZE : DSPBSIZE),
		REG_READ(pipeA ? DSPAPOS : DSPBPOS),
		REG_READ(pipeA ? DSPABASE : DSPBBASE)
		);

	DRM_DEBUG(
		"saved: (%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x)\n",
		crtc_state->saveDSPCNTR,
		crtc_state->savePIPECONF,
		crtc_state->savePIPESRC,
		crtc_state->saveFP0,
		crtc_state->saveFP1,
		crtc_state->saveDPLL,
		crtc_state->saveHTOTAL,
		crtc_state->saveHBLANK,
		crtc_state->saveHSYNC,
		crtc_state->saveVTOTAL,
		crtc_state->saveVBLANK,
		crtc_state->saveVSYNC,
		crtc_state->saveDSPSTRIDE,
		crtc_state->saveDSPSIZE,
		crtc_state->saveDSPPOS,
		crtc_state->saveDSPBASE
		);


#if 0
	if (drm_helper_crtc_in_use(crtc))
		crtc_funcs->dpms(crtc, DRM_MODE_DPMS_OFF);


	if (psb_intel_panel_fitter_pipe(dev) == psb_intel_crtc->pipe) {
		REG_WRITE(PFIT_CONTROL, crtc_state->savePFITCTRL);
		DRM_DEBUG("write pfit_controle: %x\n", REG_READ(PFIT_CONTROL));
	}
#endif

	if (crtc_state->saveDPLL & DPLL_VCO_ENABLE) {
		REG_WRITE(pipeA ? DPLL_A : DPLL_B,
			crtc_state->saveDPLL & ~DPLL_VCO_ENABLE);
		REG_READ(pipeA ? DPLL_A : DPLL_B);
		DRM_DEBUG("write dpll: %x\n",
				REG_READ(pipeA ? DPLL_A : DPLL_B));
		udelay(150);
	}

	REG_WRITE(pipeA ? FPA0 : FPB0, crtc_state->saveFP0);
	REG_READ(pipeA ? FPA0 : FPB0);

	REG_WRITE(pipeA ? FPA1 : FPB1, crtc_state->saveFP1);
	REG_READ(pipeA ? FPA1 : FPB1);

	REG_WRITE(pipeA ? DPLL_A : DPLL_B, crtc_state->saveDPLL);
	REG_READ(pipeA ? DPLL_A : DPLL_B);
	udelay(150);

	REG_WRITE(pipeA ? HTOTAL_A : HTOTAL_B, crtc_state->saveHTOTAL);
	REG_WRITE(pipeA ? HBLANK_A : HBLANK_B, crtc_state->saveHBLANK);
	REG_WRITE(pipeA ? HSYNC_A : HSYNC_B, crtc_state->saveHSYNC);
	REG_WRITE(pipeA ? VTOTAL_A : VTOTAL_B, crtc_state->saveVTOTAL);
	REG_WRITE(pipeA ? VBLANK_A : VBLANK_B, crtc_state->saveVBLANK);
	REG_WRITE(pipeA ? VSYNC_A : VSYNC_B, crtc_state->saveVSYNC);
	REG_WRITE(pipeA ? DSPASTRIDE : DSPBSTRIDE, crtc_state->saveDSPSTRIDE);

	REG_WRITE(pipeA ? DSPASIZE : DSPBSIZE, crtc_state->saveDSPSIZE);
	REG_WRITE(pipeA ? DSPAPOS : DSPBPOS, crtc_state->saveDSPPOS);

	REG_WRITE(pipeA ? PIPEASRC : PIPEBSRC, crtc_state->savePIPESRC);
	REG_WRITE(pipeA ? DSPABASE : DSPBBASE, crtc_state->saveDSPBASE);
	REG_WRITE(pipeA ? PIPEACONF : PIPEBCONF, crtc_state->savePIPECONF);

	psb_intel_wait_for_vblank(dev);

	REG_WRITE(pipeA ? DSPACNTR : DSPBCNTR, crtc_state->saveDSPCNTR);
	REG_WRITE(pipeA ? DSPABASE : DSPBBASE, crtc_state->saveDSPBASE);

	psb_intel_wait_for_vblank(dev);

	paletteReg = pipeA ? PALETTE_A : PALETTE_B;
	for (i = 0; i < 256; ++i)
		REG_WRITE(paletteReg + (i << 2), crtc_state->savePalette[i]);
}

static int psb_intel_crtc_cursor_set(struct drm_crtc *crtc,
				 struct drm_file *file_priv,
				 uint32_t handle,
				 uint32_t width, uint32_t height)
{
	struct drm_device *dev = crtc->dev;
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
	struct psb_intel_mode_device *mode_dev = psb_intel_crtc->mode_dev;
	int pipe = psb_intel_crtc->pipe;
	uint32_t control = (pipe == 0) ? CURACNTR : CURBCNTR;
	uint32_t base = (pipe == 0) ? CURABASE : CURBBASE;
	uint32_t temp;
	size_t addr = 0;
	uint32_t page_offset;
	size_t size;
	void *bo;
	int ret;

	DRM_DEBUG("\n");

	/* if we want to turn of the cursor ignore width and height */
	if (!handle) {
		DRM_DEBUG("cursor off\n");
		/* turn off the cursor */
		temp = 0;
		temp |= CURSOR_MODE_DISABLE;

		REG_WRITE(control, temp);
		REG_WRITE(base, 0);

		/* unpin the old bo */
		if (psb_intel_crtc->cursor_bo) {
			mode_dev->bo_unpin_for_scanout(dev,
						       psb_intel_crtc->cursor_bo);
			psb_intel_crtc->cursor_bo = NULL;
		}

		if (psb_intel_crtc->cursor_handle) {
			psb_gtt_unmap_meminfo(dev, psb_intel_crtc->cursor_handle);
			psb_intel_crtc->cursor_handle = NULL;
		}

		return 0;
	}

	/* Currently we only support 64x64 cursors */
	if (width != 64 || height != 64) {
		DRM_ERROR("we currently only support 64x64 cursors\n");
		return -EINVAL;
	}

	bo = mode_dev->bo_from_handle(dev, file_priv, handle);
	if (!bo)
		return -ENOENT;

	ret = mode_dev->bo_pin_for_scanout(dev, bo);
	if (ret)
		return ret;
	size = mode_dev->bo_size(dev, bo);
	if (size < width * height * 4) {
		DRM_ERROR("buffer is to small\n");
		return -ENOMEM;
	}

	/*insert this bo into gtt*/
	DRM_DEBUG("%s: map meminfo for hw cursor. handle %x\n",
						__func__, handle);

	ret = psb_gtt_map_meminfo(dev, (IMG_HANDLE)handle, &page_offset);
	if (ret) {
		DRM_ERROR("Can not map meminfo to GTT. handle 0x%x\n", handle);
		return ret;
	}

	addr = page_offset << PAGE_SHIFT;


	psb_intel_crtc->cursor_addr = addr;

	temp = 0;
	/* set the pipe for the cursor */
	temp |= (pipe << 28);
	temp |= CURSOR_MODE_64_ARGB_AX | MCURSOR_GAMMA_ENABLE;

	REG_WRITE(control, temp);
	REG_WRITE(base, addr);

	/* unpin the old bo */
	if (psb_intel_crtc->cursor_bo && psb_intel_crtc->cursor_bo != bo)
		mode_dev->bo_unpin_for_scanout(dev, psb_intel_crtc->cursor_bo);

	psb_intel_crtc->cursor_bo = bo;

	if (psb_intel_crtc->cursor_handle && psb_intel_crtc->cursor_handle != (void *)handle)
		psb_gtt_unmap_meminfo(dev, psb_intel_crtc->cursor_handle);

	psb_intel_crtc->cursor_handle = (IMG_HANDLE)handle;

	return 0;
}

static int psb_intel_crtc_cursor_move(struct drm_crtc *crtc, int x, int y)
{
	struct drm_device *dev = crtc->dev;
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
	int pipe = psb_intel_crtc->pipe;
	uint32_t temp = 0;
	uint32_t adder;


	if (x < 0) {
		temp |= (CURSOR_POS_SIGN << CURSOR_X_SHIFT);
		x = -x;
	}
	if (y < 0) {
		temp |= (CURSOR_POS_SIGN << CURSOR_Y_SHIFT);
		y = -y;
	}

	temp |= ((x & CURSOR_POS_MASK) << CURSOR_X_SHIFT);
	temp |= ((y & CURSOR_POS_MASK) << CURSOR_Y_SHIFT);

	adder = psb_intel_crtc->cursor_addr;

	REG_WRITE((pipe == 0) ? CURAPOS : CURBPOS, temp);
	REG_WRITE((pipe == 0) ? CURABASE : CURBBASE, adder);

	return 0;
}

static void psb_intel_crtc_gamma_set(struct drm_crtc *crtc, u16 *red,
				 u16 *green, u16 *blue, uint32_t start, uint32_t size)
{
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
	int i;
	int end = (start + size > 256) ? 256 : start + size;

	for (i = start; i < end; i++) {
		psb_intel_crtc->lut_r[i] = red[i] >> 8;
		psb_intel_crtc->lut_g[i] = green[i] >> 8;
		psb_intel_crtc->lut_b[i] = blue[i] >> 8;
	}

	psb_intel_crtc_load_lut(crtc);
}

static int psb_crtc_set_config(struct drm_mode_set *set)
{
	int ret = 0;
	struct drm_device * dev = set->crtc->dev;
	struct drm_psb_private * dev_priv = dev->dev_private;

	if(!dev_priv->rpm_enabled)
		return drm_crtc_helper_set_config(set);

	pm_runtime_forbid(&dev->pdev->dev);

	ret = drm_crtc_helper_set_config(set);

	pm_runtime_allow(&dev->pdev->dev);

	return ret;
}

/* Returns the clock of the currently programmed mode of the given pipe. */
static int psb_intel_crtc_clock_get(struct drm_device *dev,
				struct drm_crtc *crtc)
{
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
	int pipe = psb_intel_crtc->pipe;
	u32 dpll;
	u32 fp;
	struct psb_intel_clock_t clock;
	bool is_lvds;

	dpll = REG_READ((pipe == 0) ? DPLL_A : DPLL_B);
	if ((dpll & DISPLAY_RATE_SELECT_FPA1) == 0)
		fp = REG_READ((pipe == 0) ? FPA0 : FPB0);
	else
		fp = REG_READ((pipe == 0) ? FPA1 : FPB1);
	is_lvds = (pipe == 1) && (REG_READ(LVDS) & LVDS_PORT_EN);

	clock.m1 = (fp & FP_M1_DIV_MASK) >> FP_M1_DIV_SHIFT;
	clock.m2 = (fp & FP_M2_DIV_MASK) >> FP_M2_DIV_SHIFT;
	clock.n = (fp & FP_N_DIV_MASK) >> FP_N_DIV_SHIFT;

	if (is_lvds) {
		clock.p1 =
		    ffs((dpll &
			 DPLL_FPA01_P1_POST_DIV_MASK_I830_LVDS) >>
			DPLL_FPA01_P1_POST_DIV_SHIFT);
		clock.p2 = 14;

		if ((dpll & PLL_REF_INPUT_MASK) ==
		    PLLB_REF_INPUT_SPREADSPECTRUMIN) {
			/* XXX: might not be 66MHz */
			i8xx_clock(66000, &clock);
		} else
			i8xx_clock(48000, &clock);
	} else {
		if (dpll & PLL_P1_DIVIDE_BY_TWO)
			clock.p1 = 2;
		else {
			clock.p1 =
			    ((dpll &
			      DPLL_FPA01_P1_POST_DIV_MASK_I830) >>
			     DPLL_FPA01_P1_POST_DIV_SHIFT) + 2;
		}
		if (dpll & PLL_P2_DIVIDE_BY_4)
			clock.p2 = 4;
		else
			clock.p2 = 2;

		i8xx_clock(48000, &clock);
	}

	/* XXX: It would be nice to validate the clocks, but we can't reuse
	 * i830PllIsValid() because it relies on the xf86_config connector
	 * configuration being accurate, which it isn't necessarily.
	 */

	return clock.dot;
}

/** Returns the currently programmed mode of the given pipe. */
struct drm_display_mode *psb_intel_crtc_mode_get(struct drm_device *dev,
					     struct drm_crtc *crtc)
{
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
	int pipe = psb_intel_crtc->pipe;
	struct drm_display_mode *mode;
	int htot;
	int hsync;
	int vtot;
	int vsync;

	htot = REG_READ((pipe == 0) ? HTOTAL_A : HTOTAL_B);
	hsync = REG_READ((pipe == 0) ? HSYNC_A : HSYNC_B);
	vtot = REG_READ((pipe == 0) ? VTOTAL_A : VTOTAL_B);
	vsync = REG_READ((pipe == 0) ? VSYNC_A : VSYNC_B);

	mode = kzalloc(sizeof(*mode), GFP_KERNEL);
	if (!mode)
		return NULL;

	mode->clock = psb_intel_crtc_clock_get(dev, crtc);
	mode->hdisplay = (htot & 0xffff) + 1;
	mode->htotal = ((htot & 0xffff0000) >> 16) + 1;
	mode->hsync_start = (hsync & 0xffff) + 1;
	mode->hsync_end = ((hsync & 0xffff0000) >> 16) + 1;
	mode->vdisplay = (vtot & 0xffff) + 1;
	mode->vtotal = ((vtot & 0xffff0000) >> 16) + 1;
	mode->vsync_start = (vsync & 0xffff) + 1;
	mode->vsync_end = ((vsync & 0xffff0000) >> 16) + 1;

	drm_mode_set_name(mode);
	drm_mode_set_crtcinfo(mode, 0);

	return mode;
}

static void psb_intel_crtc_destroy(struct drm_crtc *crtc)
{
	struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);

	kfree(psb_intel_crtc->crtc_state);
	drm_crtc_cleanup(crtc);
	kfree(psb_intel_crtc);
}

static const struct drm_crtc_helper_funcs cdv_intel_helper_funcs = {
	.dpms = psb_intel_crtc_dpms,
	.mode_fixup = psb_intel_crtc_mode_fixup,
	.mode_set = cdv_intel_crtc_mode_set,
	.mode_set_base = psb_intel_pipe_set_base,
	.prepare = psb_intel_crtc_prepare,
	.commit = psb_intel_crtc_commit,
};

const struct drm_crtc_funcs psb_intel_crtc_funcs = {
	.save = psb_intel_crtc_save,
	.restore = psb_intel_crtc_restore,
	.cursor_set = psb_intel_crtc_cursor_set,
	.cursor_move = psb_intel_crtc_cursor_move,
	.gamma_set = psb_intel_crtc_gamma_set,
	.set_config = psb_crtc_set_config,
	.destroy = psb_intel_crtc_destroy,
};

/*
 * Set the default value of cursor control and base register
 * to zero. This is a workaround for h/w defect on oaktrail
 */
void psb_intel_cursor_init(struct drm_device *dev, int pipe)
{
	uint32_t control;
	uint32_t base;
      
	switch (pipe) {
	case 0:
		control = CURACNTR;
		base = CURABASE;
		break;
	case 1:
		control = CURBCNTR;
		base = CURBBASE;
		break;
	case 2:
		control = CURCCNTR;
		base = CURCBASE;
		break;
	default:
		return;
	}

	REG_WRITE(control, 0);
	REG_WRITE(base, 0);
}

void psb_intel_crtc_init(struct drm_device *dev, int pipe,
			 struct psb_intel_mode_device *mode_dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct psb_intel_crtc *psb_intel_crtc;
	int i;
	uint16_t *r_base, *g_base, *b_base;

	PSB_DEBUG_ENTRY("\n");

	/* We allocate a extra array of drm_connector pointers
	 * for fbdev after the crtc */
	psb_intel_crtc = kzalloc(sizeof(struct psb_intel_crtc) +
				 (INTELFB_CONN_LIMIT * sizeof(struct drm_connector *)),
				 GFP_KERNEL);
	if (psb_intel_crtc == NULL)
		return;

	psb_intel_crtc->crtc_state = kzalloc(sizeof(struct psb_intel_crtc_state),
					     GFP_KERNEL);
	if (!psb_intel_crtc->crtc_state) {
		DRM_ERROR("Crtc state error: No memory\n");
		kfree(psb_intel_crtc);
		return;
	}

	drm_crtc_init(dev, &psb_intel_crtc->base, &psb_intel_crtc_funcs);

	drm_mode_crtc_set_gamma_size(&psb_intel_crtc->base, 256);
	psb_intel_crtc->pipe = pipe;
	psb_intel_crtc->plane = pipe;
	psb_intel_crtc->active = true;

	r_base = psb_intel_crtc->base.gamma_store;
	g_base = r_base + 256;
	b_base = g_base + 256;
	for (i = 0; i < 256; i++) {
		psb_intel_crtc->lut_r[i] = i;
		psb_intel_crtc->lut_g[i] = i;
		psb_intel_crtc->lut_b[i] = i;
		r_base[i] = i << 8;
		g_base[i] = i << 8;
		b_base[i] = i << 8;

		psb_intel_crtc->lut_adj[i] = 0;
	}

	psb_intel_crtc->mode_dev = mode_dev;
	psb_intel_crtc->cursor_addr = 0;

	drm_crtc_helper_add(&psb_intel_crtc->base,
			    &cdv_intel_helper_funcs);

	/* Setup the array of drm_connector pointer array */
	psb_intel_crtc->mode_set.crtc = &psb_intel_crtc->base;
	BUG_ON(pipe >= ARRAY_SIZE(dev_priv->plane_to_crtc_mapping) ||
	       dev_priv->plane_to_crtc_mapping[psb_intel_crtc->plane] != NULL);
	dev_priv->plane_to_crtc_mapping[psb_intel_crtc->plane] = &psb_intel_crtc->base;
	dev_priv->pipe_to_crtc_mapping[psb_intel_crtc->pipe] = &psb_intel_crtc->base;
	psb_intel_crtc->mode_set.connectors =
	    (struct drm_connector **) (psb_intel_crtc + 1);
	psb_intel_crtc->mode_set.num_connectors = 0;

	psb_intel_cursor_init(dev, pipe);

	if (pipe == 0) {
		init_timer(&dev_priv->flip_timer);
		dev_priv->flip_timer.function = psb_flip_timer;
		dev_priv->flip_timer.data = (unsigned long)dev;
		dev_priv->pipeEnabled = 0;
	}
}

int psb_intel_get_pipe_from_crtc_id(struct drm_device *dev, void *data,
				struct drm_file *file_priv)
{
	struct drm_psb_private *dev_priv = dev->dev_private;
	struct drm_psb_get_pipe_from_crtc_id_arg *pipe_from_crtc_id = data;
	struct drm_mode_object *drmmode_obj;
	struct psb_intel_crtc *crtc;

	if (!dev_priv) {
		DRM_ERROR("called with no initialization\n");
		return -EINVAL;
	}

	drmmode_obj = drm_mode_object_find(dev, pipe_from_crtc_id->crtc_id,
			DRM_MODE_OBJECT_CRTC);

	if (!drmmode_obj) {
		DRM_ERROR("no such CRTC id\n");
		return -EINVAL;
	}

	crtc = to_psb_intel_crtc(obj_to_crtc(drmmode_obj));
	pipe_from_crtc_id->pipe = crtc->pipe;

	return 0;
}

struct drm_crtc *psb_intel_get_crtc_from_pipe(struct drm_device *dev, int pipe)
{
	struct drm_crtc *crtc = NULL;

	list_for_each_entry(crtc, &dev->mode_config.crtc_list, head) {
		struct psb_intel_crtc *psb_intel_crtc = to_psb_intel_crtc(crtc);
		if (psb_intel_crtc->pipe == pipe)
			break;
	}
	return crtc;
}

int psb_intel_connector_clones(struct drm_device *dev, int type_mask)
{
	int index_mask = 0;
	struct drm_connector *connector;
	int entry = 0;

	list_for_each_entry(connector, &dev->mode_config.connector_list,
			    head) {
		struct psb_intel_output *psb_intel_output =
		    to_psb_intel_output(connector);
		if (type_mask & (1 << psb_intel_output->type))
			index_mask |= (1 << entry);
		entry++;
	}
	return index_mask;
}

void psb_intel_modeset_cleanup(struct drm_device *dev)
{
	struct drm_psb_private *dev_priv = dev->dev_private;

	timer_delete_sync(&dev_priv->flip_timer);

	drm_mode_config_cleanup(dev);
}


/* current intel driver doesn't take advantage of encoders
   always give back the encoder for the connector
*/
struct drm_encoder *psb_intel_best_encoder(struct drm_connector *connector)
{
	struct psb_intel_output *psb_intel_output =
					to_psb_intel_output(connector);

	return &psb_intel_output->enc;
}


