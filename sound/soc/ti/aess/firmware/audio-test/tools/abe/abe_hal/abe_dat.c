/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2010-2011 Texas Instruments Incorporated,
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
 * Copyright(c) 2010-2011 Texas Instruments Incorporated,
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
#include "abe_main.h"
#ifndef abe_dat_c
#define abe_dat_c
const u32 abe_firmware_array[ABE_FIRMWARE_MAX_SIZE] = {
#include "abe_firmware.c"
};
u32 abe_firmware_version_number;
/*
 * Kernel base
 */
void __iomem *io_base;
/*
 * global variable : saves stack area
 */
u16 MultiFrame[PROCESSING_SLOTS][TASKS_IN_SLOT];
ABE_SIODescriptor sio_desc;
ABE_SPingPongDescriptor desc_pp;
abe_satcdescriptor_aess atc_desc;
/*
 * automatic gain control of input mixer's gains
 */
u32 abe_compensated_mixer_gain;
u8 abe_muted_gains_indicator[MAX_NBGAIN_CMEM];
u32 abe_desired_gains_decibel[MAX_NBGAIN_CMEM];
u32 abe_muted_gains_decibel[MAX_NBGAIN_CMEM];
u32 abe_desired_gains_linear[MAX_NBGAIN_CMEM];
u32 abe_desired_ramp_delay_ms[MAX_NBGAIN_CMEM];
/*
 * HAL/FW ports status / format / sampling / protocol(call_back) / features
 *	/ gain / name
 */
u32 pdm_dl1_status;
u32 pdm_dl2_status;
u32 pdm_vib_status;
/*
 * HAL/FW ports status / format / sampling / protocol(call_back) / features
 *	/ gain / name
 */
abe_port_t abe_port[LAST_PORT_ID];	/* list of ABE ports */
const abe_port_t abe_port_init[LAST_PORT_ID] = {
	/* Status Data Format Drift Call-Back Protocol+selector desc_addr;
	   buf_addr; buf_size; iter; irq_addr irq_data DMA_T $Features
	   reseted at start Port Name for the debug trace */
	/* DMIC */ {
		    OMAP_ABE_PORT_ACTIVITY_IDLE, {96000, SIX_MSB},
		    NODRIFT, NOCALLBACK, 1, (DMIC_ITER/6),
		    {
		     SNK_P, DMIC_PORT_PROT,
		     {{dmem_dmic, dmem_dmic_size, DMIC_ITER} }
		     },
		    {0, 0},
		    {EQDMIC, 0}, "DMIC"},
	/* PDM_UL */ {
		      OMAP_ABE_PORT_ACTIVITY_IDLE, {96000, STEREO_MSB},
		      NODRIFT, NOCALLBACK, smem_amic, (MCPDM_UL_ITER/2),
		      {
		       SNK_P, MCPDMUL_PORT_PROT,
		       {{dmem_amic, dmem_amic_size, MCPDM_UL_ITER} }
		       },
		      {0, 0},
		      {EQAMIC, 0}, "PDM_UL"},
	/* BT_VX_UL */ {
			OMAP_ABE_PORT_ACTIVITY_IDLE, {8000, STEREO_MSB},
			NODRIFT, NOCALLBACK, smem_bt_vx_ul_opp50, 1,
			{
			 SNK_P, SERIAL_PORT_PROT, {{
						   (MCBSP1_DMA_TX*ATC_SIZE),
						   dmem_bt_vx_ul,
						   dmem_bt_vx_ul_size,
						   (1*SCHED_LOOP_8kHz)
						   } }
			 },
			{0, 0}, {0}, "BT_VX_UL"},
	/* MM_UL */ {
		     OMAP_ABE_PORT_ACTIVITY_IDLE, {48000, STEREO_MSB},
		     NODRIFT, NOCALLBACK, smem_mm_ul, 1,
		     {
		      SRC_P, DMAREQ_PORT_PROT, {{
						(CBPr_DMA_RTX3*ATC_SIZE),
						dmem_mm_ul, dmem_mm_ul_size,
						(10*SCHED_LOOP_48kHz),
						ABE_DMASTATUS_RAW, (1 << 3)
						} }
		      },
		     {CIRCULAR_BUFFER_PERIPHERAL_R__3, 120},
		     {UPROUTE, 0}, "MM_UL"},
	/* MM_UL2 */ {
		      OMAP_ABE_PORT_ACTIVITY_IDLE, {48000, STEREO_MSB},
		      NODRIFT, NOCALLBACK, smem_mm_ul2, 1,
		      {
		       SRC_P, DMAREQ_PORT_PROT, {{
						 (CBPr_DMA_RTX4*ATC_SIZE),
						 dmem_mm_ul2, dmem_mm_ul2_size,
						 (2*SCHED_LOOP_48kHz),
						 ABE_DMASTATUS_RAW, (1 << 4)
						 } }
		       },
		      {CIRCULAR_BUFFER_PERIPHERAL_R__4, 24},
		      {UPROUTE, 0}, "MM_UL2"},
	/* VX_UL */ {
		     OMAP_ABE_PORT_ACTIVITY_IDLE, {8000, MONO_MSB},
		     NODRIFT, NOCALLBACK, smem_vx_ul, 1,
		     {
		      SRC_P, DMAREQ_PORT_PROT, {{
						(CBPr_DMA_RTX2*ATC_SIZE),
						dmem_vx_ul, dmem_vx_ul_size,
						(1*SCHED_LOOP_8kHz),
						ABE_DMASTATUS_RAW, (1 << 2)
						} }
		      }, {
			  CIRCULAR_BUFFER_PERIPHERAL_R__2, 2},
		     {ASRC2, 0}, "VX_UL"},
	/* MM_DL */ {
		     OMAP_ABE_PORT_ACTIVITY_IDLE, {48000, STEREO_MSB},
		     NODRIFT, NOCALLBACK, smem_mm_dl, 1,
		     {
		      SNK_P, PINGPONG_PORT_PROT, {{
						  (CBPr_DMA_RTX0*ATC_SIZE),
						  dmem_mm_dl, dmem_mm_dl_size,
						  (2*SCHED_LOOP_48kHz),
						  ABE_DMASTATUS_RAW, (1 << 0)
						  } }
		      },
		     {CIRCULAR_BUFFER_PERIPHERAL_R__0, 24},
		     {ASRC3, 0}, "MM_DL"},
	/* VX_DL */ {
		     OMAP_ABE_PORT_ACTIVITY_IDLE, {8000, MONO_MSB},
		     NODRIFT, NOCALLBACK, smem_vx_dl, 1,
		     {
		      SNK_P, DMAREQ_PORT_PROT, {{
						(CBPr_DMA_RTX1*ATC_SIZE),
						dmem_vx_dl, dmem_vx_dl_size,
						(1*SCHED_LOOP_8kHz),
						ABE_DMASTATUS_RAW, (1 << 1)
						} }
		      },
		     {CIRCULAR_BUFFER_PERIPHERAL_R__1, 2},
		     {ASRC1, 0}, "VX_DL"},
	/* TONES_DL */ {
			OMAP_ABE_PORT_ACTIVITY_IDLE, {48000, STEREO_MSB},
			NODRIFT, NOCALLBACK, smem_tones_dl, 1,
			{
			 SNK_P, DMAREQ_PORT_PROT, {{
						   (CBPr_DMA_RTX5*ATC_SIZE),
						   dmem_tones_dl,
						   dmem_tones_dl_size,
						   (2*SCHED_LOOP_48kHz),
						   ABE_DMASTATUS_RAW, (1 << 5)
						   } }
			 },
			{CIRCULAR_BUFFER_PERIPHERAL_R__5, 24},
			{0}, "TONES_DL"},
	/* VIB_DL */ {
		      OMAP_ABE_PORT_ACTIVITY_IDLE, {24000, STEREO_MSB},
		      NODRIFT, NOCALLBACK, smem_vib, 1,
		      {
		       SNK_P, DMAREQ_PORT_PROT, {{
						 (CBPr_DMA_RTX6*ATC_SIZE),
						 dmem_vib_dl, dmem_vib_dl_size,
						 (2*SCHED_LOOP_24kHz),
						 ABE_DMASTATUS_RAW, (1 << 6)
						 } }
		       },
		      {CIRCULAR_BUFFER_PERIPHERAL_R__6, 12},
		      {0}, "VIB_DL"},
	/* BT_VX_DL */ {
			OMAP_ABE_PORT_ACTIVITY_IDLE, {8000, MONO_MSB},
			NODRIFT, NOCALLBACK, smem_bt_vx_dl_opp50, 1,
			{
			 SRC_P, SERIAL_PORT_PROT, {{
						   (MCBSP1_DMA_RX*ATC_SIZE),
						   dmem_bt_vx_dl,
						   dmem_bt_vx_dl_size,
						   (1*SCHED_LOOP_8kHz),
						   } }
			 },
			{0, 0}, {0}, "BT_VX_DL"},
	/* PDM_DL */ {
		      OMAP_ABE_PORT_ACTIVITY_IDLE, {96000, SIX_MSB},
		      NODRIFT, NOCALLBACK, 1, (MCPDM_DL_ITER/6),
		      {SRC_P, MCPDMDL_PORT_PROT, {{dmem_mcpdm, dmem_mcpdm_size} } },
		      {0, 0},
		      {MIXDL1, EQ1, APS1, MIXDL2, EQ2L, EQ2R, APS2L, APS2R, 0},
		      "PDM_DL"},
	/* MM_EXT_OUT */
	{
	 OMAP_ABE_PORT_ACTIVITY_IDLE, {48000, STEREO_MSB},
	 NODRIFT, NOCALLBACK, smem_mm_ext_out, 1,
	 {
	  SRC_P, SERIAL_PORT_PROT, {{
				    (MCBSP1_DMA_TX*ATC_SIZE),
				    dmem_mm_ext_out, dmem_mm_ext_out_size,
				    (2*SCHED_LOOP_48kHz)
				    } }
	  }, {0, 0}, {0}, "MM_EXT_OUT"},
	/* MM_EXT_IN */
	{
	 OMAP_ABE_PORT_ACTIVITY_IDLE, {48000, STEREO_MSB},
	 NODRIFT, NOCALLBACK, smem_mm_ext_in_opp100, 1,
	 {
	  SNK_P, SERIAL_PORT_PROT, {{
				    (MCBSP1_DMA_RX*ATC_SIZE),
				    dmem_mm_ext_in, dmem_mm_ext_in_size,
				    (2*SCHED_LOOP_48kHz)
				    } }
	  },
	 {0, 0}, {0}, "MM_EXT_IN"},
	/* PCM3_TX */ {
		       OMAP_ABE_PORT_ACTIVITY_IDLE, {48000, STEREO_MSB},
		       NODRIFT, NOCALLBACK, 1, 1,
		       {
			SRC_P, TDM_SERIAL_PORT_PROT, {{
						      (MCBSP3_DMA_TX *
						       ATC_SIZE),
						      dmem_mm_ext_out,
						      dmem_mm_ext_out_size,
						      (2*SCHED_LOOP_48kHz)
						      } }
			},
		       {0, 0}, {0}, "TDM_OUT"},
	/* PCM3_RX */ {
		       OMAP_ABE_PORT_ACTIVITY_IDLE, {48000, STEREO_MSB},
		       NODRIFT, NOCALLBACK, 1, 1,
		       {
			SRC_P, TDM_SERIAL_PORT_PROT, {{
						      (MCBSP3_DMA_RX *
						       ATC_SIZE),
						      dmem_mm_ext_in,
						      dmem_mm_ext_in_size,
						      (2*SCHED_LOOP_48kHz)
						      } }
			},
		       {0, 0}, {0}, "TDM_IN"},
	/* SCHD_DBG_PORT */ {
			     OMAP_ABE_PORT_ACTIVITY_IDLE, {48000, MONO_MSB},
			     NODRIFT, NOCALLBACK, 1, 1,
			     {
			      SRC_P, DMAREQ_PORT_PROT, {{
							(CBPr_DMA_RTX7 *
							 ATC_SIZE),
							dmem_mm_trace,
							dmem_mm_trace_size,
							(2*SCHED_LOOP_48kHz),
							ABE_DMASTATUS_RAW,
							(1 << 4)
							} }
			      }, {CIRCULAR_BUFFER_PERIPHERAL_R__7, 24},
			     {FEAT_SEQ, FEAT_CTL, FEAT_GAINS, 0}, "SCHD_DBG"},
};
/* abe_port_init : smem content for DMIC/PDM must be 0 or Dummy_AM_labelID */
/*
 * Firmware features
 */
abe_feature_t all_feature[MAXNBFEATURE];
const abe_feature_t all_feature_init[] = {
	/* ON_reset OFF READ WRITE STATUS INPUT OUTPUT SLOT/S OPP NAME */
	/* equalizer downlink path headset + earphone */
	/* EQ1 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq1,
	 c_write_eq1, 0, 0x1000, 0x1010, 2, 0, ABE_OPP25, " DLEQ1"},
	/* equalizer downlink path integrated handsfree LEFT */
	/* EQ2L */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq2,
	 c_write_eq2, 0, 0x1000, 0x1010, 2, 0, ABE_OPP100, " DLEQ2L"},
	/* equalizer downlink path integrated handsfree RIGHT */
	/* EQ2R */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP100, " DLEQ2R"},
	/* equalizer downlink path side-tone */
	/* EQSDT */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " EQSDT"},
	/* SRC+equalizer uplink DMIC 1st pair */
	/* EQDMIC1 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " EQDMIC1"},
	/* SRC+equalizer uplink DMIC 2nd pair */
	/* EQDMIC2 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " EQDMIC2"},
	/* SRC+equalizer uplink DMIC 3rd pair */
	/* EQDMIC3 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " EQDMIC3"},
	/* SRC+equalizer uplink AMIC */
	/* EQAMIC */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " EQAMIC"},
	/* Acoustic protection for headset */
	/* APS1 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP25, " APS1"},
	/* acoustic protection high-pass filter for handsfree "Left" */
	/* APS2 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP100, " APS2"},
	/* acoustic protection high-pass filter for handsfree "Right" */
	/* APS3 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP100, " APS3"},
	/* asynchronous sample-rate-converter for the downlink voice path */
	/* ASRC1 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " ASRC_VXDL"},
	/* asynchronous sample-rate-converter for the uplink voice path */
	/* ASRC2 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " ASRC_VXUL"},
	/* asynchronous sample-rate-converter for the multimedia player */
	/* ASRC3 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP100, " ASRC_MMDL"},
	/* asynchronous sample-rate-converter for the echo reference */
	/* ASRC4 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " ASRC_ECHO"},
	/* mixer of the headset and earphone path */
	/* MXDL1 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP25, " MIX_DL1"},
	/* mixer of the hands-free path */
	/* MXDL2 */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP100, " MIX_DL2"},
	/* mixer for uplink tone mixer */
	/* MXAUDUL */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " MXSAUDUL"},
	/* mixer for voice recording */
	/* MXVXREC */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " MXVXREC"},
	/* mixer for side-tone */
	/* MXSDT */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " MIX_SDT"},
	/* mixer for echo reference */
	/* MXECHO */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " MIX_ECHO"},
	/* router of the uplink path */
	/* UPROUTE */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP50, " DLEQ3"},
	/* all gains */
	/* GAINS */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP25, " DLEQ3"},
	/* active noise canceller */
	/* EANC */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP100, " DLEQ3"},
	/* sequencing queue of micro tasks */
	/* SEQ */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP25, " DLEQ3"},
	/* Phoenix control queue through McPDM */
	/* CTL */
	{
	 c_feat_init_eq, c_feat_init_eq, c_feat_read_eq3, c_write_eq3,
	 0, 0x1000, 0x1010, 2, 0, ABE_OPP25, " DLEQ3"},
};
/*
 * MEMORY MAPPING OF THE DMEM FIFOs
 */
/* DMEM port map */
u32 abe_map_dmem[LAST_PORT_ID];
u32 abe_map_dmem_secondary[LAST_PORT_ID];
/* DMEM port buffer sizes */
u32 abe_map_dmem_size[LAST_PORT_ID];
/*
 * AESS/ATC destination and source address translation (except McASPs)
 * from the original 64bits words address
 */
const u32 abe_atc_dstid[ABE_ATC_DESC_SIZE >> 3] = {
	/* DMA_0 DMIC PDM_DL PDM_UL McB1TX McB1RX McB2TX McB2RX 0 .. 7 */
	0, 0, 12, 0, 1, 0, 2, 0,
	/* McB3TX McB3RX SLIMT0 SLIMT1 SLIMT2 SLIMT3 SLIMT4 SLIMT5 8 .. 15 */
	3, 0, 4, 5, 6, 7, 8, 9,
	/* SLIMT6 SLIMT7 SLIMR0 SLIMR1 SLIMR2 SLIMR3 SLIMR4 SLIMR5 16 .. 23 */
	10, 11, 0, 0, 0, 0, 0, 0,
	/* SLIMR6 SLIMR7 McASP1X ----- ----- McASP1R ----- ----- 24 .. 31 */
	0, 0, 14, 0, 0, 0, 0, 0,
	/* CBPrT0 CBPrT1 CBPrT2 CBPrT3 CBPrT4 CBPrT5 CBPrT6 CBPrT7 32 .. 39 */
	63, 63, 63, 63, 63, 63, 63, 63,
	/* CBP_T0 CBP_T1 CBP_T2 CBP_T3 CBP_T4 CBP_T5 CBP_T6 CBP_T7 40 .. 47 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* CBP_T8 CBP_T9 CBP_T10 CBP_T11 CBP_T12 CBP_T13 CBP_T14
	   CBP_T15 48 .. 63 */
	0, 0, 0, 0, 0, 0, 0, 0,
};
const u32 abe_atc_srcid[ABE_ATC_DESC_SIZE >> 3] = {
	/* DMA_0 DMIC PDM_DL PDM_UL McB1TX McB1RX McB2TX McB2RX 0 .. 7 */
	0, 12, 0, 13, 0, 1, 0, 2,
	/* McB3TX McB3RX SLIMT0 SLIMT1 SLIMT2 SLIMT3 SLIMT4 SLIMT5 8 .. 15 */
	0, 3, 0, 0, 0, 0, 0, 0,
	/* SLIMT6 SLIMT7 SLIMR0 SLIMR1 SLIMR2 SLIMR3 SLIMR4 SLIMR5 16 .. 23 */
	0, 0, 4, 5, 6, 7, 8, 9,
	/* SLIMR6 SLIMR7 McASP1X ----- ----- McASP1R ----- ----- 24 .. 31 */
	10, 11, 0, 0, 0, 14, 0, 0,
	/* CBPrT0 CBPrT1 CBPrT2 CBPrT3 CBPrT4 CBPrT5 CBPrT6 CBPrT7 32 .. 39 */
	63, 63, 63, 63, 63, 63, 63, 63,
	/* CBP_T0 CBP_T1 CBP_T2 CBP_T3 CBP_T4 CBP_T5 CBP_T6 CBP_T7 40 .. 47 */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* CBP_T8 CBP_T9 CBP_T10 CBP_T11 CBP_T12 CBP_T13 CBP_T14
	   CBP_T15 48 .. 63 */
	0, 0, 0, 0, 0, 0, 0, 0,
};
/*
 * preset default routing configurations
 * This is given as implementation EXAMPLES
 * the programmer uses "abe_set_router_configuration" with its own tables
	*/
const abe_router_t abe_router_ul_table_preset[NBROUTE_CONFIG][NBROUTE_UL] = {
	/* VOICE UPLINK WITH PHOENIX MICROPHONES - UPROUTE_CONFIG_AMIC */
	{
	/* 0 .. 9 = MM_UL */
	 DMIC1_L_labelID, DMIC1_R_labelID, DMIC2_L_labelID, DMIC2_R_labelID,
	 MM_EXT_IN_L_labelID, MM_EXT_IN_R_labelID, AMIC_L_labelID,
	 AMIC_L_labelID,
	 ZERO_labelID, ZERO_labelID,
	/* 10 .. 11 = MM_UL2 */
	 AMIC_L_labelID, AMIC_L_labelID,
	/* 12 .. 13 = VX_UL */
	 AMIC_L_labelID, AMIC_R_labelID,
	/* 14 .. 15 = RESERVED */
	 ZERO_labelID, ZERO_labelID,
	 },
	/* VOICE UPLINK WITH THE FIRST DMIC PAIR - UPROUTE_CONFIG_DMIC1 */
	{
	/* 0 .. 9 = MM_UL */
	 DMIC2_L_labelID, DMIC2_R_labelID, DMIC3_L_labelID, DMIC3_R_labelID,
	 DMIC1_L_labelID, DMIC1_R_labelID, ZERO_labelID, ZERO_labelID,
	 ZERO_labelID, ZERO_labelID,
	/* 10 .. 11 = MM_UL2 */
	 DMIC1_L_labelID, DMIC1_R_labelID,
	/* 12 .. 13 = VX_UL */
	 DMIC1_L_labelID, DMIC1_R_labelID,
	/* 14 .. 15 = RESERVED */
	 ZERO_labelID, ZERO_labelID,
	 },
	/* VOICE UPLINK WITH THE SECOND DMIC PAIR - UPROUTE_CONFIG_DMIC2 */
	{
	/* 0 .. 9 = MM_UL */
	 DMIC3_L_labelID, DMIC3_R_labelID, DMIC1_L_labelID, DMIC1_R_labelID,
	 DMIC2_L_labelID, DMIC2_R_labelID, ZERO_labelID, ZERO_labelID,
	 ZERO_labelID, ZERO_labelID,
	/* 10 .. 11 = MM_UL2 */
	 DMIC2_L_labelID, DMIC2_R_labelID,
	/* 12 .. 13 = VX_UL */
	 DMIC2_L_labelID, DMIC2_R_labelID,
	/* 14 .. 15 = RESERVED */
	 ZERO_labelID, ZERO_labelID,
	 },
	/* VOICE UPLINK WITH THE LAST DMIC PAIR - UPROUTE_CONFIG_DMIC3 */
	{
	/* 0 .. 9 = MM_UL */
	 AMIC_L_labelID, AMIC_R_labelID, DMIC2_L_labelID, DMIC2_R_labelID,
	 DMIC3_L_labelID, DMIC3_R_labelID, ZERO_labelID, ZERO_labelID,
	 ZERO_labelID, ZERO_labelID,
	/* 10 .. 11 = MM_UL2 */
	 DMIC3_L_labelID, DMIC3_R_labelID,
	/* 12 .. 13 = VX_UL */
	 DMIC3_L_labelID, DMIC3_R_labelID,
	/* 14 .. 15 = RESERVED */
	 ZERO_labelID, ZERO_labelID,
	 },
	/* VOICE UPLINK WITH THE BT - UPROUTE_CONFIG_BT */
	{
	/* 0 .. 9 = MM_UL */
	 BT_UL_L_labelID, BT_UL_R_labelID, DMIC2_L_labelID, DMIC2_R_labelID,
	 DMIC3_L_labelID, DMIC3_R_labelID, DMIC1_L_labelID, DMIC1_R_labelID,
	 ZERO_labelID, ZERO_labelID,
	/* 10 .. 11 = MM_UL2 */
	 AMIC_L_labelID, AMIC_R_labelID,
	/* 12 .. 13 = VX_UL */
	 BT_UL_L_labelID, BT_UL_R_labelID,
	/* 14 .. 15 = RESERVED */
	 ZERO_labelID, ZERO_labelID,
	 },
	/* VOICE UPLINK WITH THE BT - UPROUTE_ECHO_MMUL2 */
	{
	/* 0 .. 9 = MM_UL */
	 MM_EXT_IN_L_labelID, MM_EXT_IN_R_labelID, BT_UL_L_labelID,
	 BT_UL_R_labelID, AMIC_L_labelID, AMIC_R_labelID,
	 ZERO_labelID, ZERO_labelID, ZERO_labelID, ZERO_labelID,
	/* 10 .. 11 = MM_UL2 */
	 EchoRef_L_labelID, EchoRef_R_labelID,
	/* 12 .. 13 = VX_UL */
	 AMIC_L_labelID, AMIC_L_labelID,
	/* 14 .. 15 = RESERVED */
	 ZERO_labelID, ZERO_labelID,
	 },
};
/* all default routing configurations */
abe_router_t abe_router_ul_table[NBROUTE_CONFIG_MAX][NBROUTE_UL];
/*
 * ABE SUBROUTINES AND SEQUENCES
 */
/*
const abe_seq_t abe_seq_array [MAXNBSEQUENCE] [MAXSEQUENCESTEPS] =
 {{0, 0, 0, 0}, {-1, 0, 0, 0} },
 {{0, 0, 0, 0}, {-1, 0, 0, 0} },
const seq_t setup_hw_sequence2 [ ] = { 0, C_AE_FUNC1, 0, 0, 0, 0,
 -1, C_CALLBACK1, 0, 0, 0, 0 };
const abe_subroutine2 abe_sub_array [MAXNBSUBROUTINE] =
 abe_init_atc, 0, 0,
 abe_init_atc, 0, 0,
 typedef double (*PtrFun) (double);
PtrFun pFun;
pFun = sin;
   y = (* pFun) (x);
*//* mask, { time id param tag1} */
const abe_sequence_t seq_null = {
	NOMASK, {CL_M1, 0, {0, 0, 0, 0}, 0}, {CL_M1, 0, {0, 0, 0, 0}, 0}
};
/* table of new subroutines called in the sequence */
abe_subroutine2 abe_all_subsubroutine[MAXNBSUBROUTINE];
/* number of parameters per calls */
u32 abe_all_subsubroutine_nparam[MAXNBSUBROUTINE];
/* index of the subroutine */
u32 abe_subroutine_id[MAXNBSUBROUTINE];
/* paramters of the subroutine (if any) */
u32 *abe_all_subroutine_params[MAXNBSUBROUTINE];
u32 abe_subroutine_write_pointer;
/* table of all sequences */
abe_sequence_t abe_all_sequence[MAXNBSEQUENCE];
u32 abe_sequence_write_pointer;
/* current number of pending sequences (avoids to look in the table) */
u32 abe_nb_pending_sequences;
/* pending sequences due to ressource collision */
u32 abe_pending_sequences[MAXNBSEQUENCE];
/* mask of unsharable ressources among other sequences */
u32 abe_global_sequence_mask;
/* table of active sequences */
abe_seq_t abe_active_sequence[MAXACTIVESEQUENCE][MAXSEQUENCESTEPS];
/* index of the plugged subroutine doing ping-pong cache-flush DMEM accesses */
u32 abe_irq_pingpong_player_id;
/* index of the plugged subroutine doing acoustics protection adaptation */
u32 abe_irq_aps_adaptation_id;
/* base addresses of the ping pong buffers in bytes addresses */
u32 abe_base_address_pingpong[MAX_PINGPONG_BUFFERS];
/* size of each ping/pong buffers */
u32 abe_size_pingpong;
/* number of ping/pong buffer being used */
u32 abe_nb_pingpong;
/* current EVENT */
u32 abe_current_event_id;
/*
 * ABE CONST AREA FOR PARAMETERS TRANSLATION
 */
const u32 abe_db2lin_table[sizeof_db2lin_table] = {
	0x00000000,		/* SMEM coding of -120 dB */
	0x00000000,		/* SMEM coding of -119 dB */
	0x00000000,		/* SMEM coding of -118 dB */
	0x00000000,		/* SMEM coding of -117 dB */
	0x00000000,		/* SMEM coding of -116 dB */
	0x00000000,		/* SMEM coding of -115 dB */
	0x00000000,		/* SMEM coding of -114 dB */
	0x00000000,		/* SMEM coding of -113 dB */
	0x00000000,		/* SMEM coding of -112 dB */
	0x00000000,		/* SMEM coding of -111 dB */
	0x00000000,		/* SMEM coding of -110 dB */
	0x00000000,		/* SMEM coding of -109 dB */
	0x00000001,		/* SMEM coding of -108 dB */
	0x00000001,		/* SMEM coding of -107 dB */
	0x00000001,		/* SMEM coding of -106 dB */
	0x00000001,		/* SMEM coding of -105 dB */
	0x00000001,		/* SMEM coding of -104 dB */
	0x00000001,		/* SMEM coding of -103 dB */
	0x00000002,		/* SMEM coding of -102 dB */
	0x00000002,		/* SMEM coding of -101 dB */
	0x00000002,		/* SMEM coding of -100 dB */
	0x00000002,		/* SMEM coding of -99 dB */
	0x00000003,		/* SMEM coding of -98 dB */
	0x00000003,		/* SMEM coding of -97 dB */
	0x00000004,		/* SMEM coding of -96 dB */
	0x00000004,		/* SMEM coding of -95 dB */
	0x00000005,		/* SMEM coding of -94 dB */
	0x00000005,		/* SMEM coding of -93 dB */
	0x00000006,		/* SMEM coding of -92 dB */
	0x00000007,		/* SMEM coding of -91 dB */
	0x00000008,		/* SMEM coding of -90 dB */
	0x00000009,		/* SMEM coding of -89 dB */
	0x0000000A,		/* SMEM coding of -88 dB */
	0x0000000B,		/* SMEM coding of -87 dB */
	0x0000000D,		/* SMEM coding of -86 dB */
	0x0000000E,		/* SMEM coding of -85 dB */
	0x00000010,		/* SMEM coding of -84 dB */
	0x00000012,		/* SMEM coding of -83 dB */
	0x00000014,		/* SMEM coding of -82 dB */
	0x00000017,		/* SMEM coding of -81 dB */
	0x0000001A,		/* SMEM coding of -80 dB */
	0x0000001D,		/* SMEM coding of -79 dB */
	0x00000021,		/* SMEM coding of -78 dB */
	0x00000025,		/* SMEM coding of -77 dB */
	0x00000029,		/* SMEM coding of -76 dB */
	0x0000002E,		/* SMEM coding of -75 dB */
	0x00000034,		/* SMEM coding of -74 dB */
	0x0000003A,		/* SMEM coding of -73 dB */
	0x00000041,		/* SMEM coding of -72 dB */
	0x00000049,		/* SMEM coding of -71 dB */
	0x00000052,		/* SMEM coding of -70 dB */
	0x0000005D,		/* SMEM coding of -69 dB */
	0x00000068,		/* SMEM coding of -68 dB */
	0x00000075,		/* SMEM coding of -67 dB */
	0x00000083,		/* SMEM coding of -66 dB */
	0x00000093,		/* SMEM coding of -65 dB */
	0x000000A5,		/* SMEM coding of -64 dB */
	0x000000B9,		/* SMEM coding of -63 dB */
	0x000000D0,		/* SMEM coding of -62 dB */
	0x000000E9,		/* SMEM coding of -61 dB */
	0x00000106,		/* SMEM coding of -60 dB */
	0x00000126,		/* SMEM coding of -59 dB */
	0x0000014A,		/* SMEM coding of -58 dB */
	0x00000172,		/* SMEM coding of -57 dB */
	0x0000019F,		/* SMEM coding of -56 dB */
	0x000001D2,		/* SMEM coding of -55 dB */
	0x0000020B,		/* SMEM coding of -54 dB */
	0x0000024A,		/* SMEM coding of -53 dB */
	0x00000292,		/* SMEM coding of -52 dB */
	0x000002E2,		/* SMEM coding of -51 dB */
	0x0000033C,		/* SMEM coding of -50 dB */
	0x000003A2,		/* SMEM coding of -49 dB */
	0x00000413,		/* SMEM coding of -48 dB */
	0x00000492,		/* SMEM coding of -47 dB */
	0x00000521,		/* SMEM coding of -46 dB */
	0x000005C2,		/* SMEM coding of -45 dB */
	0x00000676,		/* SMEM coding of -44 dB */
	0x0000073F,		/* SMEM coding of -43 dB */
	0x00000822,		/* SMEM coding of -42 dB */
	0x00000920,		/* SMEM coding of -41 dB */
	0x00000A3D,		/* SMEM coding of -40 dB */
	0x00000B7D,		/* SMEM coding of -39 dB */
	0x00000CE4,		/* SMEM coding of -38 dB */
	0x00000E76,		/* SMEM coding of -37 dB */
	0x0000103A,		/* SMEM coding of -36 dB */
	0x00001235,		/* SMEM coding of -35 dB */
	0x0000146E,		/* SMEM coding of -34 dB */
	0x000016EC,		/* SMEM coding of -33 dB */
	0x000019B8,		/* SMEM coding of -32 dB */
	0x00001CDC,		/* SMEM coding of -31 dB */
	0x00002061,		/* SMEM coding of -30 dB */
	0x00002455,		/* SMEM coding of -29 dB */
	0x000028C4,		/* SMEM coding of -28 dB */
	0x00002DBD,		/* SMEM coding of -27 dB */
	0x00003352,		/* SMEM coding of -26 dB */
	0x00003995,		/* SMEM coding of -25 dB */
	0x0000409C,		/* SMEM coding of -24 dB */
	0x0000487E,		/* SMEM coding of -23 dB */
	0x00005156,		/* SMEM coding of -22 dB */
	0x00005B43,		/* SMEM coding of -21 dB */
	0x00006666,		/* SMEM coding of -20 dB */
	0x000072E5,		/* SMEM coding of -19 dB */
	0x000080E9,		/* SMEM coding of -18 dB */
	0x000090A4,		/* SMEM coding of -17 dB */
	0x0000A24B,		/* SMEM coding of -16 dB */
	0x0000B618,		/* SMEM coding of -15 dB */
	0x0000CC50,		/* SMEM coding of -14 dB */
	0x0000E53E,		/* SMEM coding of -13 dB */
	0x00010137,		/* SMEM coding of -12 dB */
	0x0001209A,		/* SMEM coding of -11 dB */
	0x000143D1,		/* SMEM coding of -10 dB */
	0x00016B54,		/* SMEM coding of -9 dB */
	0x000197A9,		/* SMEM coding of -8 dB */
	0x0001C967,		/* SMEM coding of -7 dB */
	0x00020137,		/* SMEM coding of -6 dB */
	0x00023FD6,		/* SMEM coding of -5 dB */
	0x00028619,		/* SMEM coding of -4 dB */
	0x0002D4EF,		/* SMEM coding of -3 dB */
	0x00032D64,		/* SMEM coding of -2 dB */
	0x000390A4,		/* SMEM coding of -1 dB */
	0x00040000,		/* SMEM coding of 0 dB */
	0x00047CF2,		/* SMEM coding of 1 dB */
	0x00050923,		/* SMEM coding of 2 dB */
	0x0005A670,		/* SMEM coding of 3 dB */
	0x000656EE,		/* SMEM coding of 4 dB */
	0x00071CF5,		/* SMEM coding of 5 dB */
	0x0007FB26,		/* SMEM coding of 6 dB */
	0x0008F473,		/* SMEM coding of 7 dB */
	0x000A0C2B,		/* SMEM coding of 8 dB */
	0x000B4606,		/* SMEM coding of 9 dB */
	0x000CA62C,		/* SMEM coding of 10 dB */
	0x000E314A,		/* SMEM coding of 11 dB */
	0x000FEC9E,		/* SMEM coding of 12 dB */
	0x0011DE0A,		/* SMEM coding of 13 dB */
	0x00140C28,		/* SMEM coding of 14 dB */
	0x00167E60,		/* SMEM coding of 15 dB */
	0x00193D00,		/* SMEM coding of 16 dB */
	0x001C515D,		/* SMEM coding of 17 dB */
	0x001FC5EB,		/* SMEM coding of 18 dB */
	0x0023A668,		/* SMEM coding of 19 dB */
	0x00280000,		/* SMEM coding of 20 dB */
	0x002CE178,		/* SMEM coding of 21 dB */
	0x00325B65,		/* SMEM coding of 22 dB */
	0x00388062,		/* SMEM coding of 23 dB */
	0x003F654E,		/* SMEM coding of 24 dB */
	0x00472194,		/* SMEM coding of 25 dB */
	0x004FCF7C,		/* SMEM coding of 26 dB */
	0x00598C81,		/* SMEM coding of 27 dB */
	0x006479B7,		/* SMEM coding of 28 dB */
	0x0070BC3D,		/* SMEM coding of 29 dB */
	0x007E7DB9,		/* SMEM coding of 30 dB */
};
const u32 abe_1_alpha_iir[64] = {
	0x040002, 0x040002, 0x040002, 0x040002,	/* 0 */
	0x50E955, 0x48CA65, 0x40E321, 0x72BE78,	/* 1 [ms] */
	0x64BA68, 0x57DF14, 0x4C3D60, 0x41D690,	/* 2 */
	0x38A084, 0x308974, 0x297B00, 0x235C7C,	/* 4 */
	0x1E14B0, 0x198AF0, 0x15A800, 0x125660,	/* 8 */
	0x0F82A0, 0x0D1B5C, 0x0B113C, 0x0956CC,	/* 16 */
	0x07E054, 0x06A3B8, 0x059844, 0x04B680,	/* 32 */
	0x03F80C, 0x035774, 0x02D018, 0x025E0C,	/* 64 */
	0x7F8057, 0x6B482F, 0x5A4297, 0x4BEECB,	/* 128 */
	0x3FE00B, 0x35BAA7, 0x2D3143, 0x2602AF,	/* 256 */
	0x1FF803, 0x1AE2FB, 0x169C9F, 0x13042B,	/* 512 */
	0x0FFE03, 0x0D72E7, 0x0B4F4F, 0x0982CB,	/* 1.024 [s] */
	0x07FF83, 0x06B9CF, 0x05A7E7, 0x04C193,	/* 2.048 */
	0x03FFE3, 0x035CFF, 0x02D403, 0x0260D7,	/* 4.096 */
	0x01FFFB, 0x01AE87, 0x016A07, 0x01306F,	/* 8.192 */
	0x00FFFF, 0x00D743, 0x00B503, 0x009837,
};
const u32 abe_alpha_iir[64] = {
	0x000000, 0x000000, 0x000000, 0x000000,	/* 0 */
	0x5E2D58, 0x6E6B3C, 0x7E39C0, 0x46A0C5,	/* 1 [ms] */
	0x4DA2CD, 0x541079, 0x59E151, 0x5F14B9,	/* 2 */
	0x63AFC1, 0x67BB45, 0x6B4281, 0x6E51C1,	/* 4 */
	0x70F5A9, 0x733A89, 0x752C01, 0x76D4D1,	/* 8 */
	0x783EB1, 0x797251, 0x7A7761, 0x7B549D,	/* 16 */
	0x7C0FD5, 0x7CAE25, 0x7D33DD, 0x7DA4C1,	/* 32 */
	0x7E03FD, 0x7E5449, 0x7E97F5, 0x7ED0F9,	/* 64 */
	0x7F0101, 0x7F2971, 0x7F4B7D, 0x7F6825,	/* 128 */
	0x7F8041, 0x7F948D, 0x7FA59D, 0x7FB3FD,	/* 256 */
	0x7FC011, 0x7FCA3D, 0x7FD2C9, 0x7FD9F9,	/* 512 */
	0x7FE005, 0x7FE51D, 0x7FE961, 0x7FECFD,	/* 1.024 [s] */
	0x7FF001, 0x7FF28D, 0x7FF4B1, 0x7FF67D,	/* 2.048 */
	0x7FF801, 0x7FF949, 0x7FFA59, 0x7FFB41,	/* 4.096 */
	0x7FFC01, 0x7FFCA5, 0x7FFD2D, 0x7FFDA1,	/* 8.192 */
	0x7FFE01, 0x7FFE51, 0x7FFE95, 0x7FFED1,
};
/*
 * ABE_DEBUG DATA
 */
/*
 * IRQ and trace pointer in DMEM:
 * FW updates a write pointer at "MCU_IRQ_FIFO_ptr_labelID",
 * the read pointer is in HAL
 */
u32 abe_irq_dbg_read_ptr;
/*
 * General circular buffer used to trace APIs calls and AE activity.
 */
u32 abe_dbg_activity_log[D_DEBUG_HAL_TASK_sizeof];
u32 abe_dbg_activity_log_write_pointer;
u32 abe_dbg_mask;
/*
 * Global variable holding parameter errors
 */
u32 abe_dbg_param;
/*
 * Output of messages selector
 */
u32 abe_dbg_output;
/* Variable to switch path for headset or handsfree at OPP25*/
u32 abe_mcpdm_path = 1;			/* Headset path default*/
/*
 * last parameters
 */
#define SIZE_PARAM 10
u32 param1[SIZE_PARAM];
u32 param2[SIZE_PARAM];
u32 param3[SIZE_PARAM];
u32 param4[SIZE_PARAM];
u32 param5[SIZE_PARAM];
/*
 * MAIN PORT SELECTION
 */
const u32 abe_port_priority[LAST_PORT_ID - 1] = {
	PDM_DL_PORT,
	PDM_UL_PORT,
	MM_EXT_OUT_PORT,
	MM_EXT_IN_PORT,
	TDM_DL_PORT,
	TDM_UL_PORT,
	DMIC_PORT,
	MM_UL_PORT,
	MM_UL2_PORT,
	MM_DL_PORT,
	TONES_DL_PORT,
	VX_UL_PORT,
	VX_DL_PORT,
	BT_VX_DL_PORT,
	BT_VX_UL_PORT,
	VIB_DL_PORT,
};
/*
 * ABE CONST AREA FOR DMIC DECIMATION FILTERS
 */
/* const s32 abe_dmic_40 [C_98_48_LP_Coefs_sizeof] = {
	-4119413, -192384, -341428, -348088, -151380, 151380, 348088,
	341428, 192384, 4119415, 1938156, -6935719, 775202, -1801934,
	2997698, -3692214, 3406822, -2280190, 1042982 };
const s32 abe_dmic_32 [C_98_48_LP_Coefs_sizeof] = {
	-4119413, -192384, -341428, -348088, -151380, 151380, 348088,
	341428, 192384, 4119415, 1938156, -6935719, 775202, -1801934,
	2997698, -3692214, 3406822, -2280190, 1042982 };
const s32 abe_dmic_25 [C_98_48_LP_Coefs_sizeof] = {
	-4119413, -192384, -341428, -348088, -151380, 151380, 348088,
	341428, 192384, 4119415, 1938156, -6935719, 775202, -1801934,
	2997698, -3692214, 3406822, -2280190, 1042982 };
const s32 abe_dmic_16 [C_98_48_LP_Coefs_sizeof] = {
	-4119413, -192384, -341428, -348088, -151380, 151380, 348088,
	341428, 192384, 4119415, 1938156, -6935719, 775202, -1801934,
	2997698, -3692214, 3406822, -2280190, 1042982 };
*/
#endif/* abe_dat_c */
