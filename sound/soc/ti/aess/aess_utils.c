/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2010-2013 Texas Instruments Incorporated,
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.GPL.
 *
 * BSD LICENSE
 *
 * Copyright(c) 2010-2013 Texas Instruments Incorporated,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>

#include "omap-aess-priv.h"
#include "aess_utils.h"
#include "omap-aess.h"
#include "aess_gain.h"
#include "aess_mem.h"

/* AESS_MCU_IRQENABLE_SET/CLR (0x3c/0x40) bit field */
#define INT_MASK			0x01

/* AESS_DMAENABLE_SET/CLR (0x60/0x64) bit fields */
#define DMA_SELECT(x)			(x & 0xFF)

#define EVENT_SOURCE_DMA 0
#define EVENT_SOURCE_COUNTER 1

/* EVENT_GENERATOR_COUNTER COUNTER_VALUE bit field */

/* PLL output/desired sampling rate = (32768 * 6000)/96000 */
#define EVENT_GENERATOR_COUNTER_DEFAULT	(2048-1)
/* PLL output/desired sampling rate = (32768 * 6000)/88200 */
#define EVENT_GENERATOR_COUNTER_44100	(2228-1)

/* Offset cancellation */
#define OMAP_AESS_HS_DC_OFFSET_STEP	(1800 / 8)
#define OMAP_AESS_HF_DC_OFFSET_STEP	(4600 / 8)

/**
 * omap_aess_hw_configuration
 * @aess: Pointer on aess handle
 *
 * Initialize the AESS HW registers for MPU and DMA
 * request visibility.
 */
static void omap_aess_hw_configuration(struct omap_aess *aess)
{
	/* enable AESS auto gating (required to release all AESS clocks) */
	omap_aess_reg_write(aess, OMAP_AESS_AUTO_GATING_ENABLE, 1);
	/* enables the DMAreq from AESS AESS_DMAENABLE_SET = 255 */
	omap_aess_reg_write(aess, OMAP_AESS_DMAENABLE_SET, DMA_SELECT(0xff));
	/* enables the MCU IRQ from AESS to Cortex A9 */
	omap_aess_reg_write(aess, OMAP_AESS_MCU_IRQENABLE_SET, INT_MASK);
}

/**
 * omap_aess_disable_irq - disable MCU/DSP AESS interrupt
 * @aess: Pointer on abe handle
 *
 * This subroutine is disabling AESS MCU/DSP Irq
 */
void omap_aess_disable_irq(struct omap_aess *aess)
{
	/* disables the DMAreq from AESS AESS_DMAENABLE_CLR = 127
	 * DMA_Req7 will still be enabled as it is used for AESS trace */
	omap_aess_reg_write(aess, OMAP_AESS_DMAENABLE_CLR, DMA_SELECT(0x7f));
	/* disables the MCU IRQ from AESS to Cortex A9 */
	omap_aess_reg_write(aess, OMAP_AESS_MCU_IRQENABLE_CLR, INT_MASK);
}

/**
 * omap_aess_reset_hal - reset the AESS/HAL
 * @aess: Pointer on aess handle
 *
 * Operations : reset the AESS by reloading the static variables and
 * default AESS registers.
 * Called after a PRCM cold-start reset of AESS
 */
void omap_aess_reset_hal(struct omap_aess *aess)
{
	u32 i;

	/* IRQ & DBG circular read pointer in DMEM */
	aess->irq_dbg_read_ptr = 0;

	/* reset the default gain values */
	for (i = 0; i < MAX_NBGAIN_CMEM; i++) {
		struct omap_aess_gain *gain = &aess->gains[i];

		gain->muted_indicator = 0;
		gain->desired_decibel = (u32) GAIN_MUTE;
		gain->desired_linear = 0;
		gain->desired_ramp_delay_ms = 0;
		gain->muted_decibel = (u32) GAIN_TOOLOW;
	}
	omap_aess_hw_configuration(aess);
}

/**
 * omap_aess_write_event_generator - Selects event generator source
 * @aess: Pointer on abe handle
 * @e: Event Generation Counter, McPDM, DMIC or default.
 *
 * Loads the AESS event generator hardware source.
 * Indicates to the FW which data stream is the most important to preserve
 * in case all the streams are asynchronous.
 *
 * the Audio Engine will generaly use its own timer EVENT generator programmed
 * with the EVENT_COUNTER. The event counter will be tuned in order to deliver
 * a pulse frequency at 96 kHz.
 * The DPLL output at 100% OPP is MCLK = (32768kHz x6000) = 196.608kHz
 * The ratio is (MCLK/96000)+(1<<1) = 2050
 * (1<<1) in order to have the same speed at 50% and 100% OPP
 * (only 15 MSB bits are used at OPP50%)
 */
int omap_aess_write_event_generator(struct omap_aess *aess, u32 e)
{
	u32 event, selection;
	u32 counter = EVENT_GENERATOR_COUNTER_DEFAULT;

	switch (e) {
	case EVENT_STOP:
		omap_aess_reg_write(aess, OMAP_AESS_EVENT_GENERATOR_START, 0);
		return 0;
	case EVENT_44100:
		counter = EVENT_GENERATOR_COUNTER_44100;
		/* Fall through */
	case EVENT_TIMER:
		selection = EVENT_SOURCE_COUNTER;
		event = 0;
		break;
	default:
		dev_err(aess->dev, "Bad event generator selection (%u)\n", e);
		return -EINVAL;
	}
	omap_aess_reg_write(aess, OMAP_AESS_EVENT_GENERATOR_COUNTER, counter);
	omap_aess_reg_write(aess, OMAP_AESS_EVENT_SOURCE_SELECTION, selection);
	omap_aess_reg_write(aess, OMAP_AESS_EVENT_GENERATOR_START, 1);
	omap_aess_reg_write(aess, OMAP_AESS_AUDIO_ENGINE_SCHEDULER, event);
	return 0;
}

/**
 * omap_aess_wakeup - Wakeup AESS
 * @aess: Pointer on aess handle
 *
 * Wakeup AESS in case of retention
 */
void omap_aess_wakeup(struct omap_aess *aess)
{
	/* Restart event generator */
	omap_aess_write_event_generator(aess, EVENT_TIMER);

	/* reconfigure DMA Req and MCU Irq visibility */
	omap_aess_hw_configuration(aess);
}

/**
 * omap_aess_set_router_configuration
 * @aess: Pointer on aess handle
 * @param: list of output index of the route
 *
 * The uplink router takes its input from DMIC (6 samples), AMIC (2 samples)
 * and PORT1/2 (2 stereo ports). Each sample will be individually stored in
 * an intermediate table of 10 elements.
 *
 * Example of router table parameter for voice uplink with phoenix microphones
 *
 * indexes 0 .. 9 = MM_UL description (digital MICs and MMEXTIN)
 *	DMIC1_L_labelID, DMIC1_R_labelID, DMIC2_L_labelID, DMIC2_R_labelID,
 *	MM_EXT_IN_L_labelID, MM_EXT_IN_R_labelID, ZERO_labelID, ZERO_labelID,
 *	ZERO_labelID, ZERO_labelID,
 * indexes 10 .. 11 = MM_UL2 description (recording on DMIC3)
 *	DMIC3_L_labelID, DMIC3_R_labelID,
 * indexes 12 .. 13 = VX_UL description (VXUL based on PDMUL data)
 *	AMIC_L_labelID, AMIC_R_labelID,
 * indexes 14 .. 15 = RESERVED (NULL)
 *	ZERO_labelID, ZERO_labelID,
 */
void omap_aess_set_router_configuration(struct omap_aess *aess, u32 *param)
{
	omap_aess_write_map(aess, OMAP_AESS_DMEM_AUPLINKROUTING_ID, param);
}

u32 omap_aess_get_label_data(struct omap_aess *aess, int index)
{
	return aess->fw_info.label_id[index];
}

void omap_aess_dc_set_hs_offset(struct omap_aess *aess, int left, int right,
				int step_mV)
{
	if (left >= 8)
		left -= 16;
	aess->dc_offset.hsl = OMAP_AESS_HS_DC_OFFSET_STEP * left * step_mV;

	if (right >= 8)
		right -= 16;
	aess->dc_offset.hsr = OMAP_AESS_HS_DC_OFFSET_STEP * right * step_mV;
}
EXPORT_SYMBOL(omap_aess_dc_set_hs_offset);

void omap_aess_dc_set_hf_offset(struct omap_aess *aess, int left, int right)
{
	if (left >= 8)
		left -= 16;
	aess->dc_offset.hfl = OMAP_AESS_HF_DC_OFFSET_STEP * left;

	if (right >= 8)
		right -= 16;
	aess->dc_offset.hfr = OMAP_AESS_HF_DC_OFFSET_STEP * right;
}
EXPORT_SYMBOL(omap_aess_dc_set_hf_offset);

void omap_aess_set_dl1_gains(struct omap_aess *aess, int left, int right)
{
	omap_aess_write_gain(aess, OMAP_AESS_GAIN_DL1_LEFT, left);
	omap_aess_write_gain(aess, OMAP_AESS_GAIN_DL1_RIGHT, right);
}
EXPORT_SYMBOL(omap_aess_set_dl1_gains);
