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

#ifndef _AESS_FW_H_
#define _AESS_FW_H_

/*
 * HARDWARE AND PERIPHERAL DEFINITIONS
 */
/* holds the DMA req lines to the sDMA */
#define AESS_DMASTATUS_RAW 0x84

/* Direction=0 means input from AESS point of view */
#define AESS_ATC_DIRECTION_IN 0
/* Direction=1 means output from AESS point of view */
#define AESS_ATC_DIRECTION_OUT 1

/*
 * DMA requests
 */
/*Internal connection doesn't connect at AESS boundary */
#define External_DMA_0	0
/*Transmit request digital microphone */
#define DMIC_DMA_REQ	1
/*Multichannel PDM downlink */
#define McPDM_DMA_DL	2
/*Multichannel PDM uplink */
#define McPDM_DMA_UP	3
/*MCBSP module 1 - transmit request */
#define MCBSP1_DMA_TX	4
/*MCBSP module 1 - receive request */
#define MCBSP1_DMA_RX	5
/*MCBSP module 2 - transmit request */
#define MCBSP2_DMA_TX	6
/*MCBSP module 2 - receive request */
#define MCBSP2_DMA_RX	7
/*MCBSP module 3 - transmit request */
#define MCBSP3_DMA_TX	8
/*MCBSP module 3 - receive request */
#define MCBSP3_DMA_RX	9
/*McASP - Data transmit DMA request line */
#define McASP1_AXEVT	26
/*McASP - Data receive DMA request line */
#define McASP1_AREVT	29
/*DUMMY FIFO @@@ */
#define _DUMMY_FIFO_	30
/*DMA of the Circular buffer peripheral 0 */
#define CBPr_DMA_RTX0	32
/*DMA of the Circular buffer peripheral 1 */
#define CBPr_DMA_RTX1	33
/*DMA of the Circular buffer peripheral 2 */
#define CBPr_DMA_RTX2	34
/*DMA of the Circular buffer peripheral 3 */
#define CBPr_DMA_RTX3	35
/*DMA of the Circular buffer peripheral 4 */
#define CBPr_DMA_RTX4	36
/*DMA of the Circular buffer peripheral 5 */
#define CBPr_DMA_RTX5	37
/*DMA of the Circular buffer peripheral 6 */
#define CBPr_DMA_RTX6	38
/*DMA of the Circular buffer peripheral 7 */
#define CBPr_DMA_RTX7	39

/*
 * HARDWARE AND PERIPHERAL DEFINITIONS
 */
/* MM_DL */
#define AESS_CBPR0_IDX 0
/* VX_DL */
#define AESS_CBPR1_IDX 1
/* VX_UL */
#define AESS_CBPR2_IDX 2
/* MM_UL */
#define AESS_CBPR3_IDX 3
/* MM_UL2 */
#define AESS_CBPR4_IDX 4
/* TONES */
#define AESS_CBPR5_IDX 5
/* TDB */
#define AESS_CBPR6_IDX 6
/* DEBUG/CTL */
#define AESS_CBPR7_IDX 7
#define CIRCULAR_BUFFER_PERIPHERAL_R__0 (0x100 + AESS_CBPR0_IDX * 4)
#define CIRCULAR_BUFFER_PERIPHERAL_R__1 (0x100 + AESS_CBPR1_IDX * 4)
#define CIRCULAR_BUFFER_PERIPHERAL_R__2 (0x100 + AESS_CBPR2_IDX * 4)
#define CIRCULAR_BUFFER_PERIPHERAL_R__3 (0x100 + AESS_CBPR3_IDX * 4)
#define CIRCULAR_BUFFER_PERIPHERAL_R__4 (0x100 + AESS_CBPR4_IDX * 4)
#define CIRCULAR_BUFFER_PERIPHERAL_R__5 (0x100 + AESS_CBPR5_IDX * 4)
#define CIRCULAR_BUFFER_PERIPHERAL_R__6 (0x100 + AESS_CBPR6_IDX * 4)
#define CIRCULAR_BUFFER_PERIPHERAL_R__7 (0x100 + AESS_CBPR7_IDX * 4)
#define PING_PONG_WITH_MCU_IRQ	 1
#define PING_PONG_WITH_DSP_IRQ	 2
/*
 * INTERNAL DEFINITIONS
 */
/* 24 Q6.26 coefficients */
#define NBEQ1 25
/* 2x12 Q6.26 coefficients */
#define NBEQ2 13

/* sink / input port from Host point of view (or AESS for DMIC/McPDM/.. */
#define SNK_P AESS_ATC_DIRECTION_IN
/* source / ouptut port */
#define SRC_P AESS_ATC_DIRECTION_OUT

/* number of task/slot depending on the OPP value */
#define DOPPMODE32_OPP100	(0x00000010)
#define DOPPMODE32_OPP50	(0x0000000C)
#define DOPPMODE32_OPP25	(0x0000004)

/*
 * AESS CONST AREA FOR PERIPHERAL TUNING
 */
/* port idled IDLE_P */
#define OMAP_AESS_PORT_ACTIVITY_IDLE	1
/* port initialized, ready to be activated  */
#define OMAP_AESS_PORT_INITIALIZED	3
/* port activated RUN_P */
#define OMAP_AESS_PORT_ACTIVITY_RUNNING	2

#define NOPARAMETER 0
/* number of ATC access upon AMIC DMArequests, all the FIFOs are enabled */
#define MCPDM_UL_ITER 4
/* All the McPDM FIFOs are enabled simultaneously */
#define MCPDM_DL_ITER 24
/* All the DMIC FIFOs are enabled simultaneously */
#define DMIC_ITER 12
/* TBD later if needed */
#define MAX_PINGPONG_BUFFERS 2

/* OLD WAY */
#define c_feat_init_eq 1
#define c_feat_read_eq1 2
#define c_write_eq1 3
#define c_feat_read_eq2 4
#define c_write_eq2 5
#define c_feat_read_eq3 6
#define c_write_eq3 7
/* max number of gain to be controlled by HAL */
#define MAX_NBGAIN_CMEM 36

/* AESS memory bank IDs */
#define OMAP_AESS_BANK_DMEM	0
#define OMAP_AESS_BANK_CMEM	1
#define OMAP_AESS_BANK_SMEM	2
#define OMAP_AESS_BANK_PMEM	3
#define OMAP_AESS_BANK_IP	4

/* AESS memory IDs in omap_aess_addr[] struct */
#define OMAP_AESS_DMEM_MULTIFRAME_ID			0
#define OMAP_AESS_DMEM_DMIC_UL_FIFO_ID			1
#define OMAP_AESS_DMEM_MCPDM_UL_FIFO_ID			2
#define OMAP_AESS_DMEM_BT_UL_FIFO_ID			3
#define OMAP_AESS_DMEM_MM_UL_FIFO_ID			4
#define OMAP_AESS_DMEM_MM_UL2_FIFO_ID			5
#define OMAP_AESS_DMEM_VX_UL_FIFO_ID			6
#define OMAP_AESS_DMEM_MM_DL_FIFO_ID			7
#define OMAP_AESS_DMEM_VX_DL_FIFO_ID			8
#define OMAP_AESS_DMEM_TONES_DL_FIFO_ID			9
#define OMAP_AESS_DMEM_MCASP_DL_FIFO_ID			10
#define OMAP_AESS_DMEM_BT_DL_FIFO_ID			11
#define OMAP_AESS_DMEM_MCPDM_DL_FIFO_ID			12
#define OMAP_AESS_DMEM_MM_EXT_OUT_FIFO_ID		13
#define OMAP_AESS_DMEM_MM_EXT_IN_FIFO_ID		14
#define OMAP_AESS_SMEM_DMIC0_96_48_DATA_ID		15
#define OMAP_AESS_SMEM_DMIC1_96_48_DATA_ID		16
#define OMAP_AESS_SMEM_DMIC2_96_48_DATA_ID		17
#define OMAP_AESS_SMEM_AMIC_96_48_DATA_ID		18
#define OMAP_AESS_SMEM_BT_UL_ID				19
#define OMAP_AESS_SMEM_BT_UL_8_48_HP_DATA_ID		20
#define OMAP_AESS_SMEM_BT_UL_8_48_LP_DATA_ID		21
#define OMAP_AESS_SMEM_BT_UL_16_48_HP_DATA_ID		22
#define OMAP_AESS_SMEM_BT_UL_16_48_LP_DATA_ID		23
#define OMAP_AESS_SMEM_MM_UL2_ID			24
#define OMAP_AESS_SMEM_MM_UL_ID				25
#define OMAP_AESS_SMEM_VX_UL_ID				26
#define OMAP_AESS_SMEM_VX_UL_48_8_HP_DATA_ID		27
#define OMAP_AESS_SMEM_VX_UL_48_8_LP_DATA_ID		28
#define OMAP_AESS_SMEM_VX_UL_48_16_HP_DATA_ID		29
#define OMAP_AESS_SMEM_VX_UL_48_16_LP_DATA_ID		30
#define OMAP_AESS_SMEM_MM_DL_ID				31
#define OMAP_AESS_SMEM_MM_DL_44P1_ID			32
#define OMAP_AESS_SMEM_MM_DL_44P1_XK_ID			33
#define OMAP_AESS_SMEM_VX_DL_ID				34
#define OMAP_AESS_SMEM_VX_DL_8_48_HP_DATA_ID		35
#define OMAP_AESS_SMEM_VX_DL_8_48_LP_DATA_ID		36
#define OMAP_AESS_SMEM_VX_DL_8_48_OSR_LP_DATA_ID	37
#define OMAP_AESS_SMEM_VX_DL_16_48_HP_DATA_ID		38
#define OMAP_AESS_SMEM_VX_DL_16_48_LP_DATA_ID		39
#define OMAP_AESS_SMEM_TONES_ID				40
#define OMAP_AESS_SMEM_TONES_44P1_ID			41
#define OMAP_AESS_SMEM_TONES_44P1_XK_ID			42
#define OMAP_AESS_SMEM_MCASP1_ID			43
#define OMAP_AESS_SMEM_BT_DL_ID				44
#define OMAP_AESS_SMEM_BT_DL_8_48_OSR_LP_DATA_ID	45
#define OMAP_AESS_SMEM_BT_DL_48_8_HP_DATA_ID		46
#define OMAP_AESS_SMEM_BT_DL_48_8_LP_DATA_ID		47
#define OMAP_AESS_SMEM_BT_DL_48_16_HP_DATA_ID		48
#define OMAP_AESS_SMEM_BT_DL_48_16_LP_DATA_ID		49
#define OMAP_AESS_SMEM_DL2_M_LR_EQ_DATA_ID		50
#define OMAP_AESS_SMEM_DL1_M_EQ_DATA_ID			51
#define OMAP_AESS_SMEM_EARP_48_96_LP_DATA_ID		52
#define OMAP_AESS_SMEM_IHF_48_96_LP_DATA_ID		53
#define OMAP_AESS_SMEM_DC_HS_ID				54
#define OMAP_AESS_SMEM_DC_HF_ID				55
#define OMAP_AESS_SMEM_SDT_F_DATA_ID			56
#define OMAP_AESS_SMEM_GTARGET1_ID			57
#define OMAP_AESS_SMEM_GCURRENT_ID			58
#define OMAP_AESS_CMEM_DL1_COEFS_ID			59
#define OMAP_AESS_CMEM_DL2_L_COEFS_ID			60
#define OMAP_AESS_CMEM_DL2_R_COEFS_ID			61
#define OMAP_AESS_CMEM_SDT_COEFS_ID			62
#define OMAP_AESS_CMEM_96_48_AMIC_COEFS_ID		63
#define OMAP_AESS_CMEM_96_48_DMIC_COEFS_ID		64
#define OMAP_AESS_CMEM_1_ALPHA_ID			65
#define OMAP_AESS_CMEM_ALPHA_ID				66
#define OMAP_AESS_DMEM_SLOT23_CTRL_ID			67
#define OMAP_AESS_DMEM_AUPLINKROUTING_ID		68
#define OMAP_AESS_DMEM_MAXTASKBYTESINSLOT_ID		69
#define OMAP_AESS_DMEM_PINGPONGDESC_ID			70
#define OMAP_AESS_DMEM_IODESCR_ID			71
#define OMAP_AESS_DMEM_MCUIRQFIFO_ID			72
#define OMAP_AESS_DMEM_PING_ID				73
#define OMAP_AESS_DMEM_DEBUG_FIFO_ID			74
#define OMAP_AESS_DMEM_DEBUG_FIFO_HAL_ID		75
#define OMAP_AESS_DMEM_DEBUG_HAL_TASK_ID		76
#define OMAP_AESS_DMEM_LOOPCOUNTER_ID			77
#define OMAP_AESS_DMEM_FWMEMINITDESCR_ID		78

struct omap_aess_addr {
	int bank;
	unsigned int offset;
	unsigned int bytes;
};

/* AESS copy function IDs */
#define OMAP_AESS_COPY_FCT_NULL_ID			0
#define OMAP_AESS_COPY_FCT_S2D_STEREO_16_16_ID		1
#define OMAP_AESS_COPY_FCT_S2D_MONO_MSB_ID		2
#define OMAP_AESS_COPY_FCT_S2D_STEREO_MSB_ID		3
#define OMAP_AESS_COPY_FCT_S2D_STEREO_RSHIFTED_16_ID	4
#define OMAP_AESS_COPY_FCT_S2D_MONO_RSHIFTED_16_ID	5
#define OMAP_AESS_COPY_FCT_D2S_STEREO_16_16_ID		6
#define OMAP_AESS_COPY_FCT_D2S_MONO_MSB_ID		7
#define OMAP_AESS_COPY_FCT_D2S_MONO_RSHIFTED_16_ID	8
#define OMAP_AESS_COPY_FCT_D2S_STEREO_RSHIFTED_16_ID	9
#define OMAP_AESS_COPY_FCT_D2S_STEREO_MSB_ID		10
#define OMAP_AESS_COPY_FCT_DMIC_ID			11
#define OMAP_AESS_COPY_FCT_MCPDM_DL_ID			12
#define OMAP_AESS_COPY_FCT_MM_UL_ID			13
#define OMAP_AESS_COPY_FCT_SPLIT_SMEM_ID		14
#define OMAP_AESS_COPY_FCT_MERGE_SMEM_ID		15
#define OMAP_AESS_COPY_FCT_SPLIT_TDM_ID			16
#define OMAP_AESS_COPY_FCT_MERGE_TDM_ID			17
#define OMAP_AESS_COPY_FCT_ROUTE_MM_UL_ID		18
#define OMAP_AESS_COPY_FCT_IO_IP_ID			19
#define OMAP_AESS_COPY_FCT_COPY_UNDERFLOW_ID		20
#define OMAP_AESS_COPY_FCT_COPY_MCPDM_DL_HF_PDL1_ID	21
#define OMAP_AESS_COPY_FCT_COPY_MCPDM_DL_HF_PDL2_ID	22
#define OMAP_AESS_COPY_FCT_S2D_MONO_16_16_ID		23
#define OMAP_AESS_COPY_FCT_D2S_MONO_16_16_ID		24
#define OMAP_AESS_COPY_FCT_DMIC_NO_PRESCALE_ID		25

/* AESS buffer IDs */
#define OMAP_AESS_BUFFER_ZERO_ID		0
#define OMAP_AESS_BUFFER_DMIC1_L_ID		1
#define OMAP_AESS_BUFFER_DMIC1_R_ID		2
#define OMAP_AESS_BUFFER_DMIC2_L_ID		3
#define OMAP_AESS_BUFFER_DMIC2_R_ID		4
#define OMAP_AESS_BUFFER_DMIC3_L_ID		5
#define OMAP_AESS_BUFFER_DMIC3_R_ID		6
#define OMAP_AESS_BUFFER_BT_UL_L_ID		7
#define OMAP_AESS_BUFFER_BT_UL_R_ID		8
#define OMAP_AESS_BUFFER_MM_EXT_IN_L_ID		9
#define OMAP_AESS_BUFFER_MM_EXT_IN_R_ID		10
#define OMAP_AESS_BUFFER_AMIC_L_ID		11
#define OMAP_AESS_BUFFER_AMIC_R_ID		12
#define OMAP_AESS_BUFFER_VX_REC_L_ID		13
#define OMAP_AESS_BUFFER_VX_REC_R_ID		14
#define OMAP_AESS_BUFFER_MCU_IRQ_FIFO_PTR_ID	15
#define OMAP_AESS_BUFFER_DMIC_ATC_PTR_ID	16
#define OMAP_AESS_BUFFER_MM_EXT_IN_ID		17

struct omap_aess_io_desc {
	/* 0 */
	u16 drift_asrc;
	/* 2 */
	u16 drift_io;
	/* 4 "Function index" of XLS sheet "Functions" */
	u8 io_type_idx;
	/* 5 1 = MONO or Stereo1616, 2= STEREO, ... */
	u8 samp_size;
	/* 6 drift "issues" for ASRC */
	s16 flow_counter;
	/* 8 address for IRQ or DMArequests */
	u16 hw_ctrl_addr;
	/* 10 DMA request bit-field or IRQ (DSP/MCU) */
	u8 atc_irq_data;
	/* 11 0 = Read, 3 = Write */
	u8 direction_rw;
	/* 12 */
	u8 repeat_last_samp;
	/* 13 12 at 48kHz, ... */
	u8 nsamp;
	/* 14 nsamp x samp_size */
	u8 x_io;
	/* 15 ON = 0x80, OFF = 0x00 */
	u8 on_off;
	/* 16 For TDM purpose */
	u16 split_addr1;
	/* 18 */
	u16 split_addr2;
	/* 20 */
	u16 split_addr3;
	/* 22 */
	u8 before_f_index;
	/* 23 */
	u8 after_f_index;
	/* 24 SM/CM INITPTR field */
	u16 smem_addr1;
	/* 26 in bytes */
	u16 atc_address1;
	/* 28 DMIC_ATC_PTR, MCPDM_UL_ATC_PTR, ... */
	u16 atc_pointer_saved1;
	/* 30 samp_size (except in TDM) */
	u8 data_size1;
	/* 31 "Function index" of XLS sheet "Functions" */
	u8 copy_f_index1;
	/* 32 For TDM purpose */
	u16 smem_addr2;
	/* 34 */
	u16 atc_address2;
	/* 36 */
	u16 atc_pointer_saved2;
	/* 38 */
	u8 data_size2;
	/* 39 */
	u8 copy_f_index2;
};

/*
 *	SAMPLES TYPE
 *
 *	mono 16 bit sample LSB aligned, 16 MSB bits are unused;
 *	mono right shifted to 16bits LSBs on a 32bits DMEM FIFO for McBSP
 *	TX purpose;
 *	mono sample MSB aligned (16/24/32bits);
 *	two successive mono samples in one 32bits container;
 *	Two L/R 16bits samples in a 32bits container;
 *	Two channels defined with two MSB aligned samples;
 *	Three channels defined with three MSB aligned samples (MIC);
 *	Four channels defined with four MSB aligned samples (MIC);
 *	. . .
 *	Eight channels defined with eight MSB aligned samples (MIC);
 */
#define OMAP_AESS_FORMAT_MONO_MSB		1
#define OMAP_AESS_FORMAT_MONO_RSHIFTED_16	2
#define OMAP_AESS_FORMAT_STEREO_RSHIFTED_16	3
#define OMAP_AESS_FORMAT_STEREO_16_16		4
#define OMAP_AESS_FORMAT_STEREO_MSB		5
#define OMAP_AESS_FORMAT_THREE_MSB		6
#define OMAP_AESS_FORMAT_FOUR_MSB		7
#define OMAP_AESS_FORMAT_FIVE_MSB		8
#define OMAP_AESS_FORMAT_SIX_MSB		9
#define OMAP_AESS_FORMAT_SEVEN_MSB		10
#define OMAP_AESS_FORMAT_EIGHT_MSB		11
#define OMAP_AESS_FORMAT_NINE_MSB		12
#define OMAP_AESS_FORMAT_TEN_MSB		13
#define OMAP_AESS_FORMAT_MONO_16_16		14

/*
 *	PORT PROTOCOL TYPE - abe_port_protocol_switch_id
 */
#define OMAP_AESS_PROTOCOL_SERIAL	2
#define OMAP_AESS_PROTOCOL_TDM		3
#define OMAP_AESS_PROTOCOL_DMIC		4
#define OMAP_AESS_PROTOCOL_MCPDMDL	5
#define OMAP_AESS_PROTOCOL_MCPDMUL	6
#define OMAP_AESS_PROTOCOL_PINGPONG	7
#define OMAP_AESS_PROTOCOL_DMAREQ	8

/*
 *	PORT IDs, this list is aligned with the FW data mapping
 */
#define OMAP_AESS_PHY_PORT_DMIC		0
#define OMAP_AESS_PHY_PORT_PDM_UL	1
#define OMAP_AESS_PHY_PORT_BT_VX_UL	2
#define OMAP_AESS_PHY_PORT_MM_UL	3
#define OMAP_AESS_PHY_PORT_MM_UL2	4
#define OMAP_AESS_PHY_PORT_VX_UL	5
#define OMAP_AESS_PHY_PORT_MM_DL	6
#define OMAP_AESS_PHY_PORT_VX_DL	7
#define OMAP_AESS_PHY_PORT_TONES_DL	8
#define OMAP_AESS_PHY_PORT_MCASP_DL	9
#define OMAP_AESS_PHY_PORT_BT_VX_DL	10
#define OMAP_AESS_PHY_PORT_PDM_DL	11
#define OMAP_AESS_PHY_PORT_MM_EXT_OUT	12
#define OMAP_AESS_PHY_PORT_MM_EXT_IN	13
#define OMAP_AESS_PHY_PORT_TDM_DL	14
#define OMAP_AESS_PHY_PORT_TDM_UL	15
#define OMAP_AESS_PHY_PORT_DEBUG	16
#define OMAP_AESS_LAST_PHY_PORT_ID	17

#define FEAT_MIXDL1         14
#define FEAT_MIXDL2         15
#define FEAT_MIXAUDUL       16
#define FEAT_GAINS          21
#define FEAT_GAINS_DMIC1    22
#define FEAT_GAINS_DMIC2    23
#define FEAT_GAINS_DMIC3    24
#define FEAT_GAINS_AMIC     25
#define FEAT_GAIN_BTUL      29

/* abe_mixer_id */
#define MIXDL1 FEAT_MIXDL1
#define MIXDL2 FEAT_MIXDL2
#define MIXAUDUL FEAT_MIXAUDUL
/*
 *	GAIN IDs
 */
#define GAINS_DMIC1     FEAT_GAINS_DMIC1
#define GAINS_DMIC2     FEAT_GAINS_DMIC2
#define GAINS_DMIC3     FEAT_GAINS_DMIC3
#define GAINS_AMIC      FEAT_GAINS_AMIC
#define GAINS_BTUL      FEAT_GAIN_BTUL

/*
 *	EVENT GENERATORS - abe_event_id
 */
#define EVENT_TIMER	0
#define EVENT_44100	1
#define EVENT_STOP	2

/*
 *	SERIAL PORTS IDs - abe_mcbsp_id
 */
#define MCBSP1_TX MCBSP1_DMA_TX
#define MCBSP1_RX MCBSP1_DMA_RX
#define MCBSP2_TX MCBSP2_DMA_TX
#define MCBSP2_RX MCBSP2_DMA_RX
#define MCBSP3_TX MCBSP3_DMA_TX
#define MCBSP3_RX MCBSP3_DMA_RX

/*
 *	SERIAL PORTS IDs - abe_mcasp_id
 */
#define MCASP1_TX	McASP1_AXEVT
#define MCASP1_RX	McASP1_AREVT

/*
 *	DATA_FORMAT_T
 *
 *	used in port declaration
 */
struct omap_aess_data_format {
	/* Sampling frequency of the stream */
	u32 f;
	/* Sample format type  */
	u32 samp_format;
};

/*
 *	PORT_PROTOCOL_T
 *
 *	port declaration
 */
struct omap_aess_port_protocol {
	/* Direction=0 means input from AESS point of view */
	u32 direction;
	/* Protocol type (switch) during the data transfers */
	u32 protocol_switch;
	union {
		/* McBSP/McASP peripheral connected to ATC */
		struct {
			u32 desc_addr;
			/* Address of ATC McBSP/McASP descriptor's in bytes */
			u32 buf_addr;
			/* DMEM address in bytes */
			u32 buf_size;
			/* ITERation on each DMAreq signals */
			u32 iter;
		} prot_serial;
		/* DMIC peripheral connected to ATC */
		struct {
			/* DMEM address in bytes */
			u32 buf_addr;
			/* DMEM buffer size in bytes */
			u32 buf_size;
			/* Number of activated DMIC */
			u32 nbchan;
		} prot_dmic;
		/* McPDMDL peripheral connected to ATC */
		struct {
			/* DMEM address in bytes */
			u32 buf_addr;
			/* DMEM size in bytes */
			u32 buf_size;
			/* Control allowed on McPDM DL */
			u32 control;
		} prot_mcpdmdl;
		/* McPDMUL peripheral connected to ATC */
		struct {
			/* DMEM address size in bytes */
			u32 buf_addr;
			/* DMEM buffer size size in bytes */
			u32 buf_size;
		} prot_mcpdmul;
		/* Ping-Pong interface to the Host using cache-flush */
		struct {
			/* Address of ATC descriptor's */
			u32 desc_addr;
			/* DMEM buffer base address in bytes */
			u32 buf_addr;
			/* DMEM size in bytes for each ping and pong buffers */
			u32 buf_size;
			/* IRQ address (either DMA (0) MCU (1) or DSP(2)) */
			u32 irq_addr;
			/* IRQ data content loaded in the AESS IRQ register */
			u32 irq_data;
			/* Call-back function upon IRQ reception */
			u32 callback;
		} prot_pingpong;
		/* DMAreq line to CBPr */
		struct {
			/* Address of ATC descriptor's */
			u32 desc_addr;
			/* DMEM buffer address in bytes */
			u32 buf_addr;
			/* DMEM buffer size size in bytes */
			u32 buf_size;
			/* ITERation on each DMAreq signals */
			u32 iter;
			/* DMAreq address */
			u32 dma_addr;
			/* DMA/AESS = 1 << #DMA */
			u32 dma_data;
		} prot_dmareq;
		/* Circular buffer - direct addressing to DMEM */
		struct {
			/* DMEM buffer base address in bytes */
			u32 buf_addr;
			/* DMEM buffer size in bytes */
			u32 buf_size;
			/* DMAreq address */
			u32 dma_addr;
			/* DMA/AESS = 1 << #DMA */
			u32 dma_data;
		} prot_circular_buffer;
	} p;
};

struct omap_aess_dma_offset {
	/* Offset to the first address of the */
	u32 data;
	/* number of iterations for the DMA data moves. */
	u32 iter;
};

/*
 *	AESS_PORT_T status / format / sampling / protocol(call_back) /
 *	features / gain / name ..
 *
 */

struct omap_aess_task {
	u8 frame;
	u8 slot;
	u16 task;
};

struct omap_aess_init_task {
	u32 nb_task;
	struct omap_aess_task task[2];
};

struct omap_aess_io_task {
	u32 nb_task;
	u32 smem;
	struct omap_aess_task task[2];
};

struct omap_aess_port_type {
	struct omap_aess_init_task serial;
	struct omap_aess_init_task cbpr;
};

struct omap_aess_asrc_port {
	struct omap_aess_io_task task;
	struct omap_aess_port_type asrc;
};

struct omap_aess_port {
	/* running / idled */
	u16 status;
	/* Sample format type  */
	struct omap_aess_data_format format;
	/* IO tasks buffers */
	u16 smem_buffer1;
	u16 smem_buffer2;
	struct omap_aess_port_protocol protocol;
	/* pointer and iteration counter of the xDMA */
	struct omap_aess_dma_offset dma;
	struct omap_aess_init_task task;
	struct omap_aess_asrc_port tsk_freq[4];
};

#define OMAP_AESS_GAIN_DMIC1_LEFT    0
#define OMAP_AESS_GAIN_DMIC1_RIGHT   1
#define OMAP_AESS_GAIN_DMIC2_LEFT    2
#define OMAP_AESS_GAIN_DMIC2_RIGHT   3
#define OMAP_AESS_GAIN_DMIC3_LEFT    4
#define OMAP_AESS_GAIN_DMIC3_RIGHT   5
#define OMAP_AESS_GAIN_AMIC_LEFT     6
#define OMAP_AESS_GAIN_AMIC_RIGHT    7
#define OMAP_AESS_GAIN_DL1_LEFT      8
#define OMAP_AESS_GAIN_DL1_RIGHT     9
#define OMAP_AESS_GAIN_DL2_LEFT     10
#define OMAP_AESS_GAIN_DL2_RIGHT    11
#define OMAP_AESS_GAIN_SPLIT_LEFT   12
#define OMAP_AESS_GAIN_SPLIT_RIGHT  13
#define OMAP_AESS_MIXDL1_MM_DL      14
#define OMAP_AESS_MIXDL1_MM_UL2     15
#define OMAP_AESS_MIXDL1_VX_DL      16
#define OMAP_AESS_MIXDL1_TONES      17
#define OMAP_AESS_MIXDL2_MM_DL      18
#define OMAP_AESS_MIXDL2_MM_UL2     19
#define OMAP_AESS_MIXDL2_VX_DL      20
#define OMAP_AESS_MIXDL2_TONES      21
#define OMAP_AESS_MIXECHO_DL1       22
#define OMAP_AESS_MIXECHO_DL2       23
#define OMAP_AESS_MIXSDT_UL         24
#define OMAP_AESS_MIXSDT_DL         25
#define OMAP_AESS_MIXVXREC_MM_DL    26
#define OMAP_AESS_MIXVXREC_TONES    27
#define OMAP_AESS_MIXVXREC_VX_UL    28
#define OMAP_AESS_MIXVXREC_VX_DL    29
#define OMAP_AESS_MIXAUDUL_MM_DL    30
#define OMAP_AESS_MIXAUDUL_TONES    31
#define OMAP_AESS_MIXAUDUL_UPLINK   32
#define OMAP_AESS_MIXAUDUL_VX_DL    33
#define OMAP_AESS_GAIN_BTUL_LEFT    34
#define OMAP_AESS_GAIN_BTUL_RIGHT   35

#endif /* _AESS_FW_H_ */
