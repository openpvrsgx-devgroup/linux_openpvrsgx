/*
 * Copyright (C) 2011 Intel Corporation
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <drm/drmP.h>

#include "psb_intel_bios.h"
#include "psb_drv.h"
#include "psb_intel_drv.h"
#include "psb_intel_reg.h"


int
psb_sb_read(struct drm_device *dev, u32 reg, u32 *val)
{
	int ret;

	ret = wait_for((REG_READ(SB_PCKT) & SB_BUSY) == 0, 1000);
	if (ret) {
		DRM_ERROR("timeout waiting for SB to idle before read\n");
		return ret;
	}

	REG_WRITE(SB_ADDR, reg);
	REG_WRITE(SB_PCKT,
		   SET_FIELD(SB_OPCODE_READ, SB_OPCODE) |
		   SET_FIELD(SB_DEST_DPLL, SB_DEST) |
		   SET_FIELD(0xf, SB_BYTE_ENABLE));

	ret = wait_for((REG_READ(SB_PCKT) & SB_BUSY) == 0, 1000);
	if (ret) {
		DRM_ERROR("timeout waiting for SB to idle after read\n");
		return ret;
	}

	*val = REG_READ(SB_DATA);

	return 0;
}

int
psb_sb_write(struct drm_device *dev, u32 reg, u32 val)
{
	int ret;
	static bool dpio_debug = false;
	u32 temp;

	if (dpio_debug) {
		if (psb_sb_read(dev, reg, &temp) == 0)
			DRM_DEBUG_KMS("0x%08x: 0x%08x (before)\n", reg, temp);
		DRM_DEBUG_KMS("0x%08x: 0x%08x\n", reg, val);
	}

	ret = wait_for((REG_READ(SB_PCKT) & SB_BUSY) == 0, 1000);
	if (ret) {
		DRM_ERROR("timeout waiting for SB to idle before write\n");
		return ret;
	}

	REG_WRITE(SB_ADDR, reg);
	REG_WRITE(SB_DATA, val);
	REG_WRITE(SB_PCKT,
		   SET_FIELD(SB_OPCODE_WRITE, SB_OPCODE) |
		   SET_FIELD(SB_DEST_DPLL, SB_DEST) |
		   SET_FIELD(0xf, SB_BYTE_ENABLE));

	ret = wait_for((REG_READ(SB_PCKT) & SB_BUSY) == 0, 1000);
	if (ret) {
		DRM_ERROR("timeout waiting for SB to idle after write\n");
		return ret;
	}

	if (dpio_debug) {
		if (psb_sb_read(dev, reg, &temp) == 0)
			DRM_DEBUG_KMS("0x%08x: 0x%08x (after)\n", reg, temp);
	}

	return 0;
}

/* Reset the DPIO configuration register.  The BIOS does this at every
 * mode set.
 */
void
psb_sb_reset(struct drm_device *dev)
{

	REG_WRITE(DPIO_CFG, 0);
	REG_READ(DPIO_CFG);
	REG_WRITE(DPIO_CFG, DPIO_MODE_SELECT_0 | DPIO_CMN_RESET_N);
}

static void
psb_print_clock(struct psb_intel_clock_t *clock)
{
	DRM_DEBUG_KMS("CDV clock: n %d m1 %d m2 %d p1 %d p2 %d dot %d vco %d m %d p %d\n",
		      clock->n, clock->m1, clock->m2, clock->p1, clock->p2, clock->dot,
		      clock->vco, clock->m, clock->p);
}

/* Unlike most Intel display engines, on Cedarview the DPLL registers
 * are behind this sideband bus.  They must be programmed while the
 * DPLL reference clock is on in the DPLL control register, but before
 * the DPLL is enabled in the DPLL control register.
 */
int
psb_dpll_set_clock_cdv(struct drm_device *dev, struct drm_crtc *crtc,
		       struct psb_intel_clock_t *clock, bool is_lvds, u32 ddi_select)
{
	struct psb_intel_crtc *psb_crtc =
				to_psb_intel_crtc(crtc);
	int pipe = psb_crtc->pipe;
	u32 m, n_vco, p;
	int ret = 0;
	int dpll_reg = (pipe == 0) ? DPLL_A : DPLL_B;
	int ref_sfr = (pipe == 0) ? SB_REF_DPLLA : SB_REF_DPLLB;
	u32 ref_value;
	u32 lane_reg, lane_value;

	psb_print_clock(clock);

	psb_sb_reset(dev);

	REG_WRITE(dpll_reg, DPLL_SYNCLOCK_ENABLE | DPLL_VGA_MODE_DIS);

	udelay(100);

	/* Follow the BIOS and write the REF/SFR Register. Hardcoded value */
	psb_sb_read(dev, SB_REF_SFR(pipe), &ref_value);
	ref_value = 0x68A701;
	psb_sb_write(dev, SB_REF_SFR(pipe), ref_value);
	
	/* We don't know what the other fields of these regs are, so
	 * leave them in place.
	 */
	/* 
	 * The BIT 14:13 of 0x8010/0x8030 is used to select the ref clk
	 * for the pipe A/B. Display spec 1.06 has wrong definition.
	 * Correct definition is like below:
	 *
	 * refclka mean use clock from same PLL
	 *
	 * if DPLLA sets 01 and DPLLB sets 01, they use clock from their pll
	 *
	 * if DPLLA sets 01 and DPLLB sets 02, both use clk from DPLLA
	 *
	 */  
	ret = psb_sb_read(dev, ref_sfr, &ref_value);
	if (ret)
		return ret;
	ref_value &= ~(REF_CLK_MASK);

	/* use DPLL_A for pipeB on CRT/HDMI */

	if (pipe == 1 && !is_lvds && !(ddi_select & DP_MASK)) {
		DRM_DEBUG_KMS("use DPLLA for pipe B\n");
		ref_value |= REF_CLK_DPLLA;
	} else {
		DRM_DEBUG_KMS("use their DPLL for pipe A/B\n");
		ref_value |= REF_CLK_DPLL;
	}
	ret = psb_sb_write(dev, ref_sfr, ref_value);
	if (ret)
		return ret;
	
	ret = psb_sb_read(dev, SB_M(pipe), &m);
	if (ret)
		return ret;
	m &= ~SB_M_DIVIDER_MASK;
	m |= ((clock->m2) << SB_M_DIVIDER_SHIFT);
	ret = psb_sb_write(dev, SB_M(pipe), m);
	if (ret)
		return ret;

	ret = psb_sb_read(dev, SB_N_VCO(pipe), &n_vco);
	if (ret)
		return ret;

	/* Follow the BIOS to program the N_DIVIDER REG */
	n_vco &= 0xFFFF;
	n_vco |= 0x107;
	n_vco &= ~(SB_N_VCO_SEL_MASK |
		   SB_N_DIVIDER_MASK |
		   SB_N_CB_TUNE_MASK);

	n_vco |= ((clock->n) << SB_N_DIVIDER_SHIFT);

	if (clock->vco < 2250000) {
		n_vco |= (2 << SB_N_CB_TUNE_SHIFT);
		n_vco |= (0 << SB_N_VCO_SEL_SHIFT);
	} else if (clock->vco < 2750000) {
		n_vco |= (1 << SB_N_CB_TUNE_SHIFT);
		n_vco |= (1 << SB_N_VCO_SEL_SHIFT);
	} else if (clock->vco < 3300000) {
		n_vco |= (0 << SB_N_CB_TUNE_SHIFT);
		n_vco |= (2 << SB_N_VCO_SEL_SHIFT);
	} else {
		n_vco |= (0 << SB_N_CB_TUNE_SHIFT);
		n_vco |= (3 << SB_N_VCO_SEL_SHIFT);
	}

	ret = psb_sb_write(dev, SB_N_VCO(pipe), n_vco);
	if (ret)
		return ret;

	ret = psb_sb_read(dev, SB_P(pipe), &p);
	if (ret)
		return ret;
	p &= ~(SB_P2_DIVIDER_MASK | SB_P1_DIVIDER_MASK);
	p |= SET_FIELD(clock->p1, SB_P1_DIVIDER);
	switch (clock->p2) {
	case 5:
		p |= SET_FIELD(SB_P2_5, SB_P2_DIVIDER);
		break;
	case 10:
		p |= SET_FIELD(SB_P2_10, SB_P2_DIVIDER);
		break;
	case 14:
		p |= SET_FIELD(SB_P2_14, SB_P2_DIVIDER);
		break;
	case 7:
		p |= SET_FIELD(SB_P2_7, SB_P2_DIVIDER);
		break;
	default:
		DRM_ERROR("Bad P2 clock: %d\n", clock->p2);
		return -EINVAL;
	}
	ret = psb_sb_write(dev, SB_P(pipe), p);
	if (ret)
		return ret;

	/* Issue Title: 
	 *     DP/HDMI lanes were dead during cold temperature.
	 *
	 * Reason for this failure: 
	 *     Reference Clk required to power these lanes were not sufficient enough
	 *     and hence in cold temperature the lanes were failing. Now EV team came
	 *     up with a work around which utilizes a different alt ref path and to 
	 *     disable Op Amp (which was previously generating reference clk) and 
	 *     enable Alt_Ref Path.
	 */
	ref_value = 0x14100300;
	ret = psb_sb_write(dev, SB_ALT_REF(pipe), ref_value);
	if (ret)
		return ret;

	ref_value = 0x1FFB0010;
	ret = psb_sb_write(dev, SB_MISC_DFX(pipe), ref_value);
	if (ret)
		return ret;

	ref_value = 0x10100300;
	ret = psb_sb_write(dev, SB_ALT_REF(pipe), ref_value);
	if (ret)
		return ret;

	if (ddi_select) {
		if ((ddi_select & DDI_MASK) == DDI0_SELECT) {
			lane_reg = PSB_LANE0;
			psb_sb_read(dev, lane_reg, &lane_value);
			lane_value &= ~(LANE_PLL_MASK);
			lane_value |= LANE_PLL_ENABLE | LANE_PLL_PIPE(pipe);
			psb_sb_write(dev, lane_reg, lane_value);

			lane_reg = PSB_LANE1;
			psb_sb_read(dev, lane_reg, &lane_value);
			lane_value &= ~(LANE_PLL_MASK);
			lane_value |= LANE_PLL_ENABLE | LANE_PLL_PIPE(pipe);
			psb_sb_write(dev, lane_reg, lane_value);
		} else {
			lane_reg = PSB_LANE2;
			psb_sb_read(dev, lane_reg, &lane_value);
			lane_value &= ~(LANE_PLL_MASK);
			lane_value |= LANE_PLL_ENABLE | LANE_PLL_PIPE(pipe);
			psb_sb_write(dev, lane_reg, lane_value);

			lane_reg = PSB_LANE3;
			psb_sb_read(dev, lane_reg, &lane_value);
			lane_value &= ~(LANE_PLL_MASK);
			lane_value |= LANE_PLL_ENABLE | LANE_PLL_PIPE(pipe);
			psb_sb_write(dev, lane_reg, lane_value);
		}
	}

	return 0;
}
