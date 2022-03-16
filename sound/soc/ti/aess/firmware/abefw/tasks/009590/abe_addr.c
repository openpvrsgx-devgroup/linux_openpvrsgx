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
 * * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of Texas Instruments Incorporated nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
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

#include <abegen.h>

#include <abe_cm_linux.h>
#include <abe_dm_linux.h>
#include <abe_sm_linux.h>
#include <abe_functionsid.h>
#include <abe_define.h>
#include <abe_initxxx_labels.h>
#include <abe_taskid.h>

#include <sound/soc/ti/aess/aess-fw.h>

#include "abe-local.h"

static const u32 omap_function_id[] = {
	NULL_COPY_CFPID,
	S2D_STEREO_16_16_CFPID,
	S2D_MONO_MSB_CFPID,
	S2D_STEREO_MSB_CFPID,
	S2D_STEREO_RSHIFTED_16_CFPID,
	S2D_MONO_RSHIFTED_16_CFPID,
	D2S_STEREO_16_16_CFPID,
	D2S_MONO_MSB_CFPID,
	D2S_MONO_RSHIFTED_16_CFPID,
	D2S_STEREO_RSHIFTED_16_CFPID,
	D2S_STEREO_MSB_CFPID,
	COPY_DMIC_CFPID,
	COPY_MCPDM_DL_CFPID,
	COPY_MM_UL_CFPID,
	SPLIT_SMEM_CFPID,
	MERGE_SMEM_CFPID,
	SPLIT_TDM_CFPID,
	MERGE_TDM_CFPID,
	ROUTE_MM_UL_CFPID,
	IO_IP_CFPID,
	COPY_UNDERFLOW_CFPID,
	COPY_MCPDM_DL_HF_PDL1_CFPID,
	COPY_MCPDM_DL_HF_PDL2_CFPID,
	S2D_MONO_16_16_CFPID,
	D2S_MONO_16_16_CFPID,
	COPY_DMIC_NO_PRESCALE_CFPID
};

static const u32 omap_label_id[] = {
	ZERO_labelID,
	DMIC1_L_labelID,
	DMIC1_R_labelID,
	DMIC2_L_labelID,
	DMIC2_R_labelID,
	DMIC3_L_labelID,
	DMIC3_R_labelID,
	BT_UL_L_labelID,
	BT_UL_R_labelID,
	MM_EXT_IN_L_labelID,
	MM_EXT_IN_R_labelID,
	AMIC_L_labelID,
	AMIC_R_labelID,
	VX_REC_L_labelID,
	VX_REC_R_labelID,
	MCU_IRQ_FIFO_ptr_labelID,
	DMIC_ATC_PTR_labelID,
	MM_EXT_IN_labelID,
};

static const struct omap_aess_addr omap_aess_map[] = {
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_MULTIFRAME_ADDR,
		.bytes = OMAP_ABE_D_MULTIFRAME_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_DMIC_UL_FIFO_ADDR,
		.bytes = OMAP_ABE_D_DMIC_UL_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset =  OMAP_ABE_D_MCPDM_UL_FIFO_ADDR,
		.bytes = OMAP_ABE_D_MCPDM_UL_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_BT_UL_FIFO_ADDR,
		.bytes = OMAP_ABE_D_BT_UL_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_MM_UL_FIFO_ADDR,
		.bytes = OMAP_ABE_D_MM_UL_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_MM_UL2_FIFO_ADDR,
		.bytes = OMAP_ABE_D_MM_UL2_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_VX_UL_FIFO_ADDR,
		.bytes = OMAP_ABE_D_VX_UL_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_MM_DL_FIFO_ADDR,
		.bytes = OMAP_ABE_D_MM_DL_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_VX_DL_FIFO_ADDR,
		.bytes = OMAP_ABE_D_VX_DL_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_TONES_DL_FIFO_ADDR,
		.bytes = OMAP_ABE_D_TONES_DL_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_MCASP1_FIFO_ADDR,
		.bytes = OMAP_ABE_D_MCASP1_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_BT_DL_FIFO_ADDR,
		.bytes = OMAP_ABE_D_BT_DL_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset =  OMAP_ABE_D_MCPDM_DL_FIFO_ADDR,
		.bytes = OMAP_ABE_D_MCPDM_DL_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset =  OMAP_ABE_D_MM_EXT_OUT_FIFO_ADDR,
		.bytes = OMAP_ABE_D_MM_EXT_OUT_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset =  OMAP_ABE_D_MM_EXT_IN_FIFO_ADDR,
		.bytes = OMAP_ABE_D_MM_EXT_IN_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_DMIC0_96_48_DATA_ADDR,
		.bytes = OMAP_ABE_S_DMIC0_96_48_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_DMIC1_96_48_DATA_ADDR,
		.bytes = OMAP_ABE_S_DMIC1_96_48_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_DMIC2_96_48_DATA_ADDR,
		.bytes = OMAP_ABE_S_DMIC2_96_48_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_AMIC_96_48_DATA_ADDR,
		.bytes = OMAP_ABE_S_AMIC_96_48_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_BT_UL_ADDR,
		.bytes = OMAP_ABE_S_BT_UL_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_BT_UL_8_48_HP_DATA_ADDR,
		.bytes = OMAP_ABE_S_BT_UL_8_48_HP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_BT_UL_8_48_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_BT_UL_8_48_LP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_BT_UL_16_48_HP_DATA_ADDR,
		.bytes = OMAP_ABE_S_BT_UL_16_48_HP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_BT_UL_16_48_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_BT_UL_16_48_LP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_MM_UL2_ADDR,
		.bytes = OMAP_ABE_S_MM_UL2_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_MM_UL_ADDR,
		.bytes = OMAP_ABE_S_MM_UL_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_VX_UL_ADDR,
		.bytes = OMAP_ABE_S_VX_UL_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_VX_UL_48_8_HP_DATA_ADDR,
		.bytes = OMAP_ABE_S_VX_UL_48_8_HP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_VX_UL_48_8_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_VX_UL_48_8_LP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_VX_UL_48_16_HP_DATA_ADDR,
		.bytes = OMAP_ABE_S_VX_UL_48_16_HP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_VX_UL_48_16_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_VX_UL_48_16_LP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_MM_DL_ADDR,
		.bytes = OMAP_ABE_S_MM_DL_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_MM_DL_44P1_ADDR,
		.bytes = OMAP_ABE_S_MM_DL_44P1_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_MM_DL_44P1_XK_ADDR,
		.bytes = OMAP_ABE_S_MM_DL_44P1_XK_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_VX_DL_ADDR,
		.bytes = OMAP_ABE_S_VX_DL_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_VX_DL_8_48_HP_DATA_ADDR,
		.bytes = OMAP_ABE_S_VX_DL_8_48_HP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_VX_DL_8_48_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_VX_DL_8_48_LP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_VX_DL_8_48_OSR_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_VX_DL_8_48_OSR_LP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_VX_DL_16_48_HP_DATA_ADDR,
		.bytes = OMAP_ABE_S_VX_DL_16_48_HP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_VX_DL_16_48_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_VX_DL_16_48_LP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_TONES_ADDR,
		.bytes = OMAP_ABE_S_TONES_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_TONES_44P1_ADDR,
		.bytes = OMAP_ABE_S_TONES_44P1_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_TONES_44P1_XK_ADDR,
		.bytes = OMAP_ABE_S_TONES_44P1_XK_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_MCASP1_ADDR,
		.bytes = OMAP_ABE_S_MCASP1_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_BT_DL_ADDR,
		.bytes = OMAP_ABE_S_BT_DL_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_BT_DL_8_48_OSR_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_BT_DL_8_48_OSR_LP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_BT_DL_48_8_HP_DATA_ADDR,
		.bytes = OMAP_ABE_S_BT_DL_48_8_HP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_BT_DL_48_8_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_BT_DL_48_8_LP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_BT_DL_48_16_HP_DATA_ADDR,
		.bytes = OMAP_ABE_S_BT_DL_48_16_HP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_BT_DL_48_16_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_BT_DL_48_16_LP_DATA_SIZE,
	},

	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_DL2_M_LR_EQ_DATA_ADDR,
		.bytes = OMAP_ABE_S_DL2_M_LR_EQ_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_DL1_M_EQ_DATA_ADDR,
		.bytes = OMAP_ABE_S_DL1_M_EQ_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_EARP_48_96_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_EARP_48_96_LP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_IHF_48_96_LP_DATA_ADDR,
		.bytes = OMAP_ABE_S_IHF_48_96_LP_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_DC_HS_ADDR,
		.bytes = OMAP_ABE_S_DC_HS_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_DC_HF_ADDR,
		.bytes = OMAP_ABE_S_DC_HF_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_SDT_F_DATA_ADDR,
		.bytes = OMAP_ABE_S_SDT_F_DATA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_GTARGET1_ADDR,
		.bytes = OMAP_ABE_S_GTARGET1_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_SMEM,
		.offset = OMAP_ABE_S_GCURRENT_ADDR,
		.bytes = OMAP_ABE_S_GCURRENT_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_CMEM,
		.offset = OMAP_ABE_C_DL1_COEFS_ADDR,
		.bytes = OMAP_ABE_C_DL1_COEFS_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_CMEM,
		.offset = OMAP_ABE_C_DL2_L_COEFS_ADDR,
		.bytes = OMAP_ABE_C_DL2_L_COEFS_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_CMEM,
		.offset = OMAP_ABE_C_DL2_R_COEFS_ADDR,
		.bytes = OMAP_ABE_C_DL2_R_COEFS_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_CMEM,
		.offset = OMAP_ABE_C_SDT_COEFS_ADDR,
		.bytes = OMAP_ABE_C_SDT_COEFS_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_CMEM,
		.offset = OMAP_ABE_C_96_48_AMIC_COEFS_ADDR,
		.bytes = OMAP_ABE_C_96_48_AMIC_COEFS_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_CMEM,
		.offset = OMAP_ABE_C_96_48_DMIC_COEFS_ADDR,
		.bytes = OMAP_ABE_C_96_48_DMIC_COEFS_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_CMEM,
		.offset = OMAP_ABE_C_1_ALPHA_ADDR,
		.bytes = OMAP_ABE_C_1_ALPHA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_CMEM,
		.offset = OMAP_ABE_C_ALPHA_ADDR,
		.bytes = OMAP_ABE_C_ALPHA_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_SLOT23_CTRL_ADDR,
		.bytes = OMAP_ABE_D_SLOT23_CTRL_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_AUPLINKROUTING_ADDR,
		.bytes = OMAP_ABE_D_AUPLINKROUTING_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_MAXTASKBYTESINSLOT_ADDR,
		.bytes = 4,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_PINGPONGDESC_ADDR,
		.bytes = 4,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_IODESCR_ADDR,
		.bytes = 4,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_MCUIRQFIFO_ADDR,
		.bytes = 4,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_PING_ADDR,
		.bytes = OMAP_ABE_D_PING_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_DEBUG_FIFO_ADDR,
		.bytes = OMAP_ABE_D_DEBUG_FIFO_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_DEBUG_FIFO_HAL_ADDR,
		.bytes = OMAP_ABE_D_DEBUG_FIFO_HAL_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_DEBUG_HAL_TASK_ADDR,
		.bytes = OMAP_ABE_D_DEBUG_HAL_TASK_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_LOOPCOUNTER_ADDR,
		.bytes = OMAP_ABE_D_LOOPCOUNTER_SIZE,
	},
	{
		.bank = OMAP_AESS_BANK_DMEM,
		.offset = OMAP_ABE_D_FWMEMINITDESCR_ADDR,
		.bytes = OMAP_ABE_D_FWMEMINITDESCR_SIZE,
	},
};

/* Default scheduling table for AESS (load after boot and OFF mode) */
static const struct omap_aess_task init_table[] = {
	{
		.frame = 0,
		.slot = 3,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_DL_8),
	},
	{
		.frame = 1,
		.slot = 3,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_VX_DL_8_48_FIR),
	},
	{
		.frame = 1,
		.slot = 6,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DL2Mixer),
	},
	{
		.frame = 2,
		.slot = 0,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DL1Mixer),
	},
	{
		.frame = 2,
		.slot = 1,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_SDTMixer),
	},
	{
		.frame = 3,
		.slot = 0,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DL1_GAIN),
	},
	{
		.frame = 3,
		.slot = 6,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DL2_GAIN),
	},
	{
		.frame = 3,
		.slot = 7,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DL2_EQ),
	},
	{
		.frame = 4,
		.slot = 0,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DL1_EQ),
	},
	{
		.frame = 4,
		.slot = 1,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_VXRECMixer),
	},
	{
		.frame = 4,
		.slot = 3,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_VXREC_SPLIT),
	},
	{
		.frame = 5,
		.slot = 1,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_EARP_48_96_LP),
	},
	{
		.frame = 6,
		.slot = 0,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_EARP_48_96_LP),
	},
	{
		.frame = 6,
		.slot = 4,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_EchoMixer),
	},
	{
		.frame = 6,
		.slot = 5,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_BT_UL_SPLIT),
	},
	{
		.frame = 7,
		.slot = 3,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DBG_SYNC),
	},
	{
		.frame = 7,
		.slot = 5,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_ECHO_REF_SPLIT),
	},
	{
		.frame = 8,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DMIC1_96_48_LP),
	},
	{
		.frame = 8,
		.slot = 4,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DMIC1_SPLIT),
	},
	{
		.frame = 9,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DMIC2_96_48_LP),
	},
	{
		.frame = 9,
		.slot = 4,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DMIC2_SPLIT),
	},
	{
		.frame = 9,
		.slot = 7,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_IHF_48_96_LP),
	},
	{
		.frame = 10,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DMIC3_96_48_LP),
	},
	{
		.frame = 10,
		.slot = 4,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DMIC3_SPLIT),
	},
	{
		.frame = 10,
		.slot = 7,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_IHF_48_96_LP),
	},
	{
		.frame = 11,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_AMIC_96_48_LP),
	},
	{
		.frame = 11,
		.slot = 4,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_AMIC_SPLIT),
	},
	{
		.frame = 12,
		.slot = 3,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_VX_UL_ROUTING),
	},
	{
		.frame = 12,
		.slot = 4,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_ULMixer),
	},
	{
		.frame = 12,
		.slot = 5,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_VX_UL_48_8),
	},
	{
		.frame = 13,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_MM_UL2_ROUTING),
	},
	{
		.frame = 13,
		.slot = 3,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_SideTone),
	},
	{
		.frame = 14,
		.slot = 4,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_BT_DL_48_8),
	},
	{
		.frame = 16,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_UL_8),
	},
	{
		.frame = 17,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_BT_UL_8_48),
	},
	{
		.frame = 21,
		.slot = 1,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DEBUGTRACE_VX_ASRCs),
	},
	{
		.frame = 21,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_CHECK_IIR_RIGHT_8K),
	},
	{
		.frame = 22,
		.slot = 0,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DEBUG_IRQFIFO),
	},
	{
		.frame = 22,
		.slot = 1,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_INIT_FW_MEMORY),
	},
	{
		.frame = 22,
		.slot = 4,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_MM_EXT_IN_SPLIT),
	},
	{
		.frame = 23,
		.slot = 0,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_GAIN_UPDATE),
	},
	{
		.frame = 23,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_CHECK_IIR_LEFT_8K),
	},
};

static const struct omap_aess_task aess_dl1_mono_mixer[2] = {
	{
		.frame = 2,
		.slot = 0,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DL1Mixer),
	},
	{
		.frame = 2,
		.slot = 0,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DL1Mixer_dual_mono),
	},
};

static const struct omap_aess_task aess_dl2_mono_mixer[2] = {
	{
		.frame = 1,
		.slot = 6,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DL2Mixer),
	},
	{
		.frame = 1,
		.slot = 6,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_DL2Mixer_dual_mono),
	},
};

static const struct omap_aess_task aess_audul_mono_mixer[2] = {
	{
		.frame = 12,
		.slot = 4,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_ULMixer),
	},
	{
		.frame = 12,
		.slot = 4,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_ULMixer_dual_mono),
	},
};


static const struct omap_aess_task aess_8k_check[2] = {
	{
		.frame = 21,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_CHECK_IIR_RIGHT_8K),
	},
	{
		.frame = 23,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_CHECK_IIR_LEFT_8K),
	},
};

static const struct omap_aess_task aess_16k_check[2] = {
	{
		.frame = 21,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_CHECK_IIR_RIGHT_16K),
	},
	{
		.frame = 23,
		.slot = 2,
		.task = ABE_TASK_ID(C_ABE_FW_TASK_CHECK_IIR_LEFT_16K),
	},
};

static const struct omap_aess_port abe_port_init_pp = {
	/* MM_DL */
	.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
	.format = {
		.f = 48000,
		.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
	},
	.task = {
		.nb_task = 1,
		.task = {
			{
				.frame = 18,
				.slot = 0,
				.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_PING_PONG),
			},
		},
	},
	.tsk_freq = {
		{},
		{},
		{
			/* MM DL 44.1 kHz */
			.task = {
				.nb_task = 1,
				.smem = MM_DL_44P1_WPTR_labelID,
				.task = {
					{
						.frame = 18,
						.slot = 1,
						.task = ABE_TASK_ID(C_ABE_FW_TASK_SRC44P1_MMDL_PP),
					},
				},
			},
		},
		{
			/* MM DL 48 kHz */
			.task = {
				.nb_task = 1,
				.smem = MM_DL_labelID,
				.task = {
					{
						.frame = 18,
						.slot = 1,
						.task = 0,
					},
				},
			},
		},
	},
};

/* one scheduler loop = 4kHz = 12 samples at 48kHz */
#define FW_SCHED_LOOP_FREQ	4000
/* one scheduler loop = 4kHz = 12 samples at 48kHz */
#define FW_SCHED_LOOP_FREQ_DIV1000	(FW_SCHED_LOOP_FREQ/1000)
#define EVENT_FREQUENCY 96000
#define SLOTS_IN_SCHED_LOOP (96000/FW_SCHED_LOOP_FREQ)
#define SCHED_LOOP_8kHz (8000/FW_SCHED_LOOP_FREQ)
#define SCHED_LOOP_16kHz (16000/FW_SCHED_LOOP_FREQ)
#define SCHED_LOOP_24kHz (24000/FW_SCHED_LOOP_FREQ)
#define SCHED_LOOP_48kHz (48000/FW_SCHED_LOOP_FREQ)

#define dmem_mm_trace	OMAP_ABE_D_DEBUG_FIFO_ADDR
#define dmem_mm_trace_size ((OMAP_ABE_D_DEBUG_FIFO_SIZE)/4)
#define dmem_dmic OMAP_ABE_D_DMIC_UL_FIFO_ADDR
#define dmem_dmic_size (OMAP_ABE_D_DMIC_UL_FIFO_SIZE/4)
#define dmem_amic OMAP_ABE_D_MCPDM_UL_FIFO_ADDR
#define dmem_amic_size (OMAP_ABE_D_MCPDM_UL_FIFO_SIZE/4)
#define dmem_mcpdm OMAP_ABE_D_MCPDM_DL_FIFO_ADDR
#define dmem_mcpdm_size (OMAP_ABE_D_MCPDM_DL_FIFO_SIZE/4)
#define dmem_mm_ul OMAP_ABE_D_MM_UL_FIFO_ADDR
#define dmem_mm_ul_size (OMAP_ABE_D_MM_UL_FIFO_SIZE/4)
#define dmem_mm_ul2 OMAP_ABE_D_MM_UL2_FIFO_ADDR
#define dmem_mm_ul2_size (OMAP_ABE_D_MM_UL2_FIFO_SIZE/4)
#define dmem_mm_dl OMAP_ABE_D_MM_DL_FIFO_ADDR
#define dmem_mm_dl_size (OMAP_ABE_D_MM_DL_FIFO_SIZE/4)
#define dmem_vx_dl OMAP_ABE_D_VX_DL_FIFO_ADDR
#define dmem_vx_dl_size (OMAP_ABE_D_VX_DL_FIFO_SIZE/4)
#define dmem_vx_ul OMAP_ABE_D_VX_UL_FIFO_ADDR
#define dmem_vx_ul_size (OMAP_ABE_D_VX_UL_FIFO_SIZE/4)
#define dmem_tones_dl OMAP_ABE_D_TONES_DL_FIFO_ADDR
#define dmem_tones_dl_size (OMAP_ABE_D_TONES_DL_FIFO_SIZE/4)
#define dmem_mm_ext_out OMAP_ABE_D_MM_EXT_OUT_FIFO_ADDR
#define dmem_mm_ext_out_size (OMAP_ABE_D_MM_EXT_OUT_FIFO_SIZE/4)
#define dmem_mm_ext_in OMAP_ABE_D_MM_EXT_IN_FIFO_ADDR
#define dmem_mm_ext_in_size (OMAP_ABE_D_MM_EXT_IN_FIFO_SIZE/4)
#define dmem_bt_vx_dl OMAP_ABE_D_BT_DL_FIFO_ADDR
#define dmem_bt_vx_dl_size (OMAP_ABE_D_BT_DL_FIFO_SIZE/4)
#define dmem_bt_vx_ul OMAP_ABE_D_BT_UL_FIFO_ADDR
#define dmem_bt_vx_ul_size (OMAP_ABE_D_BT_UL_FIFO_SIZE/4)
#define dmem_mcasp1 OMAP_ABE_D_MCASP1_FIFO_ADDR
#define dmem_mcasp1_size (OMAP_ABE_D_MCASP1_FIFO_SIZE/4)

/*
 * ATC BUFFERS + IO TASKS SMEM buffers
 */
#define smem_amic	AMIC_96_labelID

/* managed directly by the router */
#define smem_mm_ul MM_UL_labelID
/* managed directly by the router */
#define smem_mm_ul2 MM_UL2_labelID
#define smem_mm_dl MM_DL_labelID
#define smem_vx_dl	IO_VX_DL_ASRC_labelID	/* Voice_16k_DL_labelID */
#define smem_vx_ul Voice_8k_UL_labelID
#define smem_tones_dl Tones_labelID
#define smem_mm_ext_out DL1_GAIN_out_labelID
/*IO_MM_EXT_IN_ASRC_labelID	 ASRC input buffer, size 40 */
#define smem_mm_ext_in_opp100 IO_MM_EXT_IN_ASRC_labelID
/* at OPP 50 without ASRC */
#define smem_mm_ext_in_opp50 MM_EXT_IN_labelID
#define smem_bt_vx_dl_opp50 BT_DL_8k_labelID
/*BT_DL_8k_opp100_labelID  ASRC output buffer, size 40 */
#define smem_bt_vx_dl_opp100 BT_DL_8k_opp100_labelID
#define smem_bt_vx_ul_opp50 BT_UL_8k_labelID
/*IO_BT_UL_ASRC_labelID	 ASRC input buffer, size 40 */
#define smem_bt_vx_ul_opp100 IO_BT_UL_ASRC_labelID


#define ATC_SIZE 8		/* 8 bytes per descriptors */


/*
 * HAL/FW ports status / format / sampling / protocol(call_back) / features
 *	/ gain / name
 */
struct omap_aess_port abe_port_init[OMAP_AESS_LAST_PHY_PORT_ID] = {
	/* Status Data Format Drift Call-Back Protocol+selector desc_addr;
	   buf_addr; buf_size; iter; irq_addr irq_data DMA_T $Features
	   reseted at start Port Name for the debug trace */
	{
		/* DMIC */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 96000,
			.samp_format = OMAP_AESS_FORMAT_SIX_MSB,
		},
		.smem_buffer1 = 1,
		.smem_buffer2 = (DMIC_ITER/6),
		.protocol = {
			.direction = SNK_P,
			.protocol_switch = OMAP_AESS_PHY_PORT_DMIC,
			.p = {
				.prot_dmic = {
					.buf_addr = dmem_dmic,
					.buf_size = dmem_dmic_size,
					.nbchan = DMIC_ITER,
				},
			},
		},
		.dma = {
			.data = 0,
			.iter = 0,
		},
		.task = {
			.nb_task = 2,
			.task = {
				{
					.frame = 2,
					.slot = 5,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_DMIC),
				},
				{
					.frame = 14,
					.slot = 3,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_DMIC),
				},
			},
		},
	},
	{
		/* PDM_UL */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 96000,
			.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
		},
		.smem_buffer1 = smem_amic,
		.smem_buffer2 = (MCPDM_UL_ITER/2),
		.protocol = {
			.direction = SNK_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_MCPDMUL,
			.p = {
				.prot_mcpdmul = {
					.buf_addr = dmem_amic,
					.buf_size = dmem_amic_size,
				},
			},
		},
		.dma = {
			.data = 0,
			.iter = 0,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 5,
					.slot = 2,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_PDM_UL),
				},
			},
		},
	},
	{
		/* BT_VX_UL */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 8000,
			.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
		},
		.smem_buffer1 = smem_bt_vx_ul_opp50,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SNK_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_SERIAL,
			.p = {
				.prot_serial = {
					.desc_addr = (MCBSP1_DMA_TX*ATC_SIZE),
					.buf_addr = dmem_bt_vx_ul,
					.buf_size = dmem_bt_vx_ul_size,
					.iter = (1*SCHED_LOOP_8kHz),
				},
			},
		},
		.dma = {
			.data = 0,
			.iter = 0,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 15,
					.slot = 3,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_BT_VX_UL),
				},
			},
		},
		.tsk_freq = {
			{
				/* BT-UL 8 kHz */
				.task = {
					.nb_task = 1,
					.smem = BT_UL_8k_labelID,
					.task = {
						{
							.frame = 17,
							.slot = 2,
							.task = ABE_TASK_ID(C_ABE_FW_TASK_BT_UL_8_48),
						},
					},
				},
			},
			{
				/* BT-UL 16 kHz */
				.task = {
					.nb_task = 1,
					.smem = BT_DL_16k_labelID,
					.task = {
						{
							.frame = 17,
							.slot = 2,
							.task = ABE_TASK_ID(C_ABE_FW_TASK_BT_UL_16_48),
						},
					},
				},
			},
			{},
			{
				/* BT-UL 48 kHz */
				.task = {
					.nb_task = 2,
					.smem = BT_UL_labelID,
					.task = {
						{
							.frame = 15,
							.slot = 6,
							.task = 0,
						},
						{
							.frame = 17,
							.slot = 2,
							.task = 0,
						},
					},
				},
			},
		},

	},
	{
		/* MM_UL */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 48000,
			.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
		},
		.smem_buffer1 = smem_mm_ul,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SRC_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_DMAREQ,
			.p = {
				.prot_dmareq = {
					.desc_addr = (CBPr_DMA_RTX3*ATC_SIZE),
					.buf_addr = dmem_mm_ul,
					.buf_size = dmem_mm_ul_size,
					.iter = (10*SCHED_LOOP_48kHz),
					.dma_addr = AESS_DMASTATUS_RAW,
					.dma_data = (1 << 3),
				},
			},
		},
		.dma = {
			.data = CIRCULAR_BUFFER_PERIPHERAL_R__3,
			.iter = 120,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 19,
					.slot = 6,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_MM_UL),
				},
			},
		},
	},
	{
		/* MM_UL2 */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 48000,
			.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
		},
		.smem_buffer1 = smem_mm_ul2,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SRC_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_DMAREQ,
			.p = {
				.prot_dmareq = {
					.desc_addr = (CBPr_DMA_RTX4*ATC_SIZE),
					.buf_addr = dmem_mm_ul2,
					.buf_size = dmem_mm_ul2_size,
					.iter = (2*SCHED_LOOP_48kHz),
					.dma_addr = AESS_DMASTATUS_RAW,
					.dma_data = (1 << 4),
				},
			},
		},
		.dma = {
			.data = CIRCULAR_BUFFER_PERIPHERAL_R__4,
			.iter = 24,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 17,
					.slot = 3,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_MM_UL2),
				},
			},
		},
	},
	{
		/* VX_UL */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 8000,
			.samp_format = OMAP_AESS_FORMAT_MONO_MSB,
		},
		.smem_buffer1 = smem_vx_ul,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SRC_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_DMAREQ,
			.p = {
				.prot_dmareq = {
					.desc_addr = (CBPr_DMA_RTX2*ATC_SIZE),
					.buf_addr = dmem_vx_ul,
					.buf_size = dmem_vx_ul_size,
					.iter = (1*SCHED_LOOP_8kHz),
					.dma_addr = AESS_DMASTATUS_RAW,
					.dma_data = (1 << 2),
				},
			},
		},
		.dma = {
			.data = CIRCULAR_BUFFER_PERIPHERAL_R__2,
			.iter = 2,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 16,
					.slot = 3,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_VX_UL),
				},
			},
		},
		.tsk_freq = {
			{
				/* Vx-UL 8 kHz */
				.task = {
					.nb_task = 1,
					.smem = Voice_8k_UL_labelID,
					.task = {
						{
							.frame = 12,
							.slot = 5,
							.task = ABE_TASK_ID(C_ABE_FW_TASK_VX_UL_48_8),
						},
					},
				},
				.asrc = {
					.serial = {
						.nb_task = 2,
						.task = {
							{
								.frame = 0,
								.slot = 3,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_DL_8_SIB),
							},
							{
								.frame = 16,
								.slot = 2,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_UL_8),
							},
						},
					},
					.cbpr = {
						.nb_task = 0,
						.task = {
							{
								.frame = 16,
								.slot = 2,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_UL_8),
							},
						},
					},
				},
			},
			{
				/* Vx-UL 16 kHz */
				.task = {
					.nb_task = 1,
					.smem = Voice_16k_UL_labelID,
					.task = {
						{
							.frame = 12,
							.slot = 5,
							.task = ABE_TASK_ID(C_ABE_FW_TASK_VX_UL_48_16),
						},
					},
				},
				.asrc = {
					.serial = {
						.nb_task = 2,
						.task = {
							{
								.frame = 0,
								.slot = 3,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_DL_16_SIB),
							},
							{
								.frame = 16,
								.slot = 2,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_UL_16),
							},
						},
					},
					.cbpr = {
						.nb_task = 1,
						.task = {
							{
								.frame = 16,
								.slot = 2,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_UL_16),
							},
						},
					},
				},
			},
			{},
			{
				/* Vx-UL 48 kHz */
				.task = {
					.nb_task = 2,
					.smem = VX_UL_M_labelID,
					.task = {
						{
							.frame = 16,
							.slot = 2,
							.task = 0,
						},
						{
							.frame = 12,
							.slot = 5,
							.task = 0,
						},
					},
				},
			},
		},
	},
	{
		/* MM_DL */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 48000,
			.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
		},
		.smem_buffer1 = smem_mm_dl,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SNK_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_DMAREQ,
			.p = {
				.prot_dmareq = {
					.desc_addr = (CBPr_DMA_RTX0*ATC_SIZE),
					.buf_addr = dmem_mm_dl,
					.buf_size = dmem_mm_dl_size,
					.iter = (2*SCHED_LOOP_48kHz),
					.dma_addr = AESS_DMASTATUS_RAW,
					.dma_data = (1 << 0),
				},
			},
		},
		.dma = {
			.data = CIRCULAR_BUFFER_PERIPHERAL_R__0,
			.iter = 24,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 18,
					.slot = 0,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_MM_DL),
				},
			},
		},
		.tsk_freq = {
			{},
			{},
			{
				/* MM DL 44.1 kHz */
				.task = {
					.nb_task = 1,
					.smem = MM_DL_44P1_WPTR_labelID,
					.task = {
						{
							.frame = 18,
							.slot = 1,
							.task = ABE_TASK_ID(C_ABE_FW_TASK_SRC44P1_MMDL),
						},
					},
				},
			},
			{
				/* MM DL 48 kHz */
				.task = {
					.nb_task = 1,
					.smem = MM_DL_labelID,
					.task = {
						{
							.frame = 18,
							.slot = 1,
							.task = 0,
						},
					},
				},
			},
		},
/*
			{
				.task = {
					.nb_task = 0,
					.smem = ,
					.task = {
					},
				},
				.asrc = {
					.serial = {
						.nb_task = 0,
						.task = {
						},
					},
					.cbpr = {
						.nb_task = 0,
						.task = {
						},
					},
				},
			},
*/
	},
	{
		/* VX_DL */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 8000,
			.samp_format = OMAP_AESS_FORMAT_MONO_MSB,
		},
		.smem_buffer1 = smem_vx_dl,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SNK_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_DMAREQ,
			.p = {
				.prot_dmareq = {
					.desc_addr = (CBPr_DMA_RTX1*ATC_SIZE),
					.buf_addr = dmem_vx_dl,
					.buf_size = dmem_vx_dl_size,
					.iter = (1*SCHED_LOOP_8kHz),
					.dma_addr = AESS_DMASTATUS_RAW,
					.dma_data = (1 << 1),
				},
			},
		},
		.dma = {
			.data = CIRCULAR_BUFFER_PERIPHERAL_R__1,
			.iter = 2,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 22,
					.slot = 2,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_VX_DL),
				},
			},
		},
		.tsk_freq = {
			{
				/* Vx-DL 8 kHz */
				.task = {
					.nb_task = 1,
					.smem = IO_VX_DL_ASRC_labelID,
					.task = {
						{
							.frame = 1,
							.slot = 3,
							.task = ABE_TASK_ID(C_ABE_FW_TASK_VX_DL_8_48_FIR),
						},
					},
				},
				.asrc = {
					.serial = {
						.nb_task = 2,
						.task = {
							{
								.frame = 0,
								.slot = 3,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_DL_8),
							},
							{
								.frame = 16,
								.slot = 2,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_UL_8_SIB),
							},
						},
					},
					.cbpr = {
						.nb_task = 1,
						.task = {
							{
								.frame = 0,
								.slot = 3,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_DL_8),
							},
						},
					},
				},
			},
			{
				/* Vx-DL 16 kHz */
				.task = {
					.nb_task = 1,
					.smem = IO_VX_DL_ASRC_labelID,
					.task = {
						{
							.frame = 1,
							.slot = 3,
							.task = ABE_TASK_ID(C_ABE_FW_TASK_VX_DL_16_48),
						},
					},
				},
				.asrc = {
					.serial = {
						.nb_task = 2,
						.task = {
							{
								.frame = 0,
								.slot = 3,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_DL_16),
							},
							{
								.frame = 16,
								.slot = 2,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_UL_16_SIB),
							},
						},
					},
					.cbpr = {
						.nb_task = 1,
						.task = {
							{
								.frame = 0,
								.slot = 3,
								.task = ABE_TASK_ID(C_ABE_FW_TASK_ASRC_VX_DL_16),
							},
						},
					},
				},
			},
			{},
			{
				/* Vx-DL 48 kHz */
				.task = {
					.nb_task = 2,
					.smem = VX_DL_labelID,
					.task = {
						{
							.frame = 0,
							.slot = 3,
							.task = 0,
						},
						{
							.frame = 1,
							.slot = 3,
							.task = 0,
						},
					},
				},
			},
		},
	},
	{
		/* TONES_DL */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 48000,
			.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
		},
		.smem_buffer1 = smem_tones_dl,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SNK_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_DMAREQ,
			.p = {
				.prot_dmareq = {
					.desc_addr = (CBPr_DMA_RTX5*ATC_SIZE),
					.buf_addr = dmem_tones_dl,
					.buf_size = dmem_tones_dl_size,
					.iter = (2*SCHED_LOOP_48kHz),
					.dma_addr = AESS_DMASTATUS_RAW,
					.dma_data = (1 << 5),
				},
			},
		},
		.dma = {
			.data = CIRCULAR_BUFFER_PERIPHERAL_R__5,
			.iter = 24,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 20,
					.slot = 0,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_TONES_DL),
				},
			},
		},
		.tsk_freq = {
			{},
			{},
			{
				/* Tones DL 44.1 kHz */
				.task = {
					.nb_task = 1,
					.smem = TONES_44P1_WPTR_labelID,
					.task = {
						{
							.frame = 20,
							.slot = 1,
							.task = ABE_TASK_ID(C_ABE_FW_TASK_SRC44P1_TONES_1211),
						},
					},
				},
			},
			{
				/* Tones DL 48 kHz */
				.task = {
					.nb_task = 1,
					.smem = Tones_labelID,
					.task = {
						{
							.frame = 20,
							.slot = 1,
							.task = 0,
						},
					},
				},
			},
		},

	},
	{
		/* McASP_DL */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 48000,
			.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
		},
		.smem_buffer1 = DL1_GAIN_out_labelID,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SRC_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_SERIAL,
			.p = {
				.prot_serial = {
					.desc_addr = (MCASP1_TX*ATC_SIZE),
					.buf_addr = dmem_mcasp1,
					.buf_size = dmem_mcasp1_size,
					.iter = (2*SCHED_LOOP_48kHz),
				},
			},
		},
		.dma = {
			.data = 0,
			.iter = 0,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 1,
					.slot = 7,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_McASP1),
				},
			},
		},
	},
	{
		/* BT_VX_DL */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 8000,
			.samp_format = OMAP_AESS_FORMAT_MONO_MSB,
		},
		.smem_buffer1 = smem_bt_vx_dl_opp50,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SRC_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_SERIAL,
			.p = {
				.prot_serial = {
					.desc_addr = (MCBSP1_DMA_RX*ATC_SIZE),
					.buf_addr = dmem_bt_vx_dl,
					.buf_size = dmem_bt_vx_dl_size,
					.iter = (1*SCHED_LOOP_8kHz),
				},
			},
		},
		.dma = {
			.data = 0,
			.iter = 0,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 13,
					.slot = 5,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_BT_VX_DL),
				},
			},
		},
		.tsk_freq = {
			{
				/* BT-DL 8 kHz */
				.task = {
					.nb_task = 1,
					.smem = BT_DL_8k_labelID,
					.task = {
						{
							.frame = 14,
							.slot = 4,
							.task = ABE_TASK_ID(C_ABE_FW_TASK_BT_DL_48_8),
						},
					},
				},
			},
			{
				/* BT-DL 16 kHz */
				.task = {
					.nb_task = 1,
					.smem = BT_DL_16k_labelID,
					.task = {
						{
							.frame = 14,
							.slot = 4,
							.task = ABE_TASK_ID(C_ABE_FW_TASK_BT_DL_48_16),
						},
					},
				},
			},
			{},
			{
				/* BT-DL 48 kHz */
				.task = {
					.nb_task = 1,
					.smem = DL1_GAIN_out_labelID,
					.task = {
						{
							.frame = 14,
							.slot = 4,
							.task = 0,
						},
					},
				},
			},
		},
	},
	{
		/* PDM_DL */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 96000,
			.samp_format = OMAP_AESS_FORMAT_SIX_MSB,
		},
		.smem_buffer1 = 1,
		.smem_buffer2 = (MCPDM_DL_ITER/6),
		.protocol = {
			.direction = SRC_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_MCPDMDL,
			.p = {
				.prot_mcpdmdl = {
					.buf_addr = dmem_mcpdm,
					.buf_size = dmem_mcpdm_size,
				},
			},
		},
		.dma = {
			.data = 0,
			.iter = 0,
		},
		.task = {
			.nb_task = 2,
			.task = {
				{
					.frame = 7,
					.slot = 0,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_PDM_DL),
				},
				{
					.frame = 19,
					.slot = 0,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_PDM_DL),
				},
			},
		},
	},
	{
		/* MM_EXT_OUT */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 48000,
			.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
		},
		.smem_buffer1 = smem_mm_ext_out,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SRC_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_SERIAL,
			.p = {
				.prot_serial = {
					.desc_addr = (MCBSP1_DMA_TX*ATC_SIZE),
					.buf_addr = dmem_mm_ext_out,
					.buf_size = dmem_mm_ext_out_size,
					.iter = (2*SCHED_LOOP_48kHz),
				},
			},
		},
		.dma = {
			.data = 0,
			.iter = 0,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 15,
					.slot = 0,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_MM_EXT_OUT),
				},
			},
		},
	},
	{
		/* MM_EXT_IN */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 48000,
			.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
		},
		.smem_buffer1 = smem_mm_ext_in_opp100,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SNK_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_SERIAL,
			.p = {
				.prot_serial = {
					.desc_addr = (MCBSP1_DMA_RX*ATC_SIZE),
					.buf_addr = dmem_mm_ext_in,
					.buf_size = dmem_mm_ext_in_size,
					.iter = (2*SCHED_LOOP_48kHz),
				},
			},
		},
		.dma = {
			.data = 0,
			.iter = 0,
		},
		.task = {
			.nb_task = 1,
			.task = {
				{
					.frame = 21,
					.slot = 3,
					.task = ABE_TASK_ID(C_ABE_FW_TASK_IO_MM_EXT_IN),
				},
			},
		},
	},
	{
		/* PCM3_TX */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 48000,
			.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
		},
		.smem_buffer1 = 1,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SRC_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_SERIAL,
			.p = {
				.prot_serial = {
					.desc_addr = (MCBSP3_DMA_TX * ATC_SIZE),
					.buf_addr = dmem_mm_ext_out,
					.buf_size = dmem_mm_ext_out_size,
					.iter = (2*SCHED_LOOP_48kHz),
				},
			},
		},
		.dma = {
			.data = 0,
			.iter = 0,
		},
		.task = {
			.nb_task = 0,
		},
	},
	{
		/* PCM3_RX */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 48000,
			.samp_format = OMAP_AESS_FORMAT_STEREO_MSB,
		},
		.smem_buffer1 = 1,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SRC_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_SERIAL,
			.p = {
				.prot_serial = {
					.desc_addr = (MCBSP3_DMA_RX * ATC_SIZE),
					.buf_addr = dmem_mm_ext_in,
					.buf_size = dmem_mm_ext_in_size,
					.iter = (2*SCHED_LOOP_48kHz),
				},
			},
		},
		.dma = {
			.data = 0,
			.iter = 0,
		},
		.task = {
			.nb_task = 0,
		},
	},
	{
		/* SCHD_DBG_PORT */
		.status = OMAP_AESS_PORT_ACTIVITY_IDLE,
		.format = {
			.f = 48000,
			.samp_format = OMAP_AESS_FORMAT_MONO_MSB,
		},
		.smem_buffer1 = 1,
		.smem_buffer2 = 1,
		.protocol = {
			.direction = SRC_P,
			.protocol_switch = OMAP_AESS_PROTOCOL_DMAREQ,
			.p = {
				.prot_dmareq = {
					.desc_addr = (CBPr_DMA_RTX7 * ATC_SIZE),
					.buf_addr = dmem_mm_trace,
					.buf_size = dmem_mm_trace_size,
					.iter = (2*SCHED_LOOP_48kHz),
					.dma_addr = AESS_DMASTATUS_RAW,
					.dma_data = (1 << 4),
				},
			},
		},
		.dma = {
			.data = CIRCULAR_BUFFER_PERIPHERAL_R__7,
			.iter = 24,
		},
		.task = {
			.nb_task = 0,
		},
	},
};

