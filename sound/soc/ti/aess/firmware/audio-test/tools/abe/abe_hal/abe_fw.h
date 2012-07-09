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
#ifndef _ABE_FW_H_
#define _ABE_FW_H_
#include "abe_cm_addr.h"
#include "abe_sm_addr.h"
#include "abe_dm_addr.h"
#include "abe_typedef.h"
/*
 * GLOBAL DEFINITION
 */
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
#define TASKS_IN_SLOT 8
/*
 * DMEM AREA - SCHEDULER
 */
#define dmem_mm_trace	D_DEBUG_FIFO_ADDR
#define dmem_mm_trace_size ((D_DEBUG_FIFO_ADDR_END-D_DEBUG_FIFO_ADDR+1)/4)
#define ATC_SIZE 8		/* 8 bytes per descriptors */
typedef struct
{
	unsigned rdpt: 7;	/* first 32bits word of the descriptor */
	unsigned reserved0: 1;
	unsigned cbsize: 7;
	unsigned irqdest: 1;
	unsigned cberr: 1;
	unsigned reserved1: 5;
	unsigned cbdir: 1;
	unsigned nw: 1;
	unsigned wrpt: 7;
	unsigned reserved2: 1;
	unsigned badd: 12;	/* second 32bits word of the descriptor */
	unsigned iter: 7;	/* iteration field overlaps 16-bit boundary */
	unsigned srcid: 6;
	unsigned destid: 6;
	unsigned desen: 1;
} abe_satcdescriptor_aess;
/*
 * table of scheduler tasks :
 * char scheduler_table[24 x 4] : four bytes used at OPP100%
 */
#define dmem_scheduler_table	D_multiFrame_ADDR
#define dmem_eanc_task_pointer	D_pFastLoopBack_ADDR
/*
 * OPP value :
 * pointer increment steps in the scheduler table
 */
#define dmem_scheduler_table_step	D_taskStep_ADDR
/*
 * table of scheduler tasks (max 64) :
 * char task_descriptors[64 x 8] : eight bytes per task
 * TASK INDEX, INITPTR 1,2,3, INITREG, Loop Counter, Reserved 1,2
 */
#define dmem_task_descriptor	D_tasksList_ADDR
/*
 * I/O DESCRIPTORS
 */
#define dmem_port_descriptors	D_IOdescr_ADDR
/* ping_pong_t descriptors table
 * structure of 8 bytes:
 * uint16 base_address1
 * uint16 size1 (16bits address format)
 * uint16 base_address2
 * uint16 size2
 * } ping_pong_t
 * ping_pong_t dmem_ping_pong_t [8]
 */
/* U8 address */
#define dmem_ping_pong_buffer	D_PING_ADDR
/*
 * IRQ mask used with ports with IRQ (DMA or host)
 * uint32 dmem_irq_masks [8]
 */
#define dmem_irq_masks	D_IRQMask_ADDR
/*
 * tables of to the 8 FIFO sequences (delayed commands) holding 12bytes tasks
 * in the format
 * structure {
 * 1) Down counter delay on 16bits, decremented on each scheduler period
 * 2) Code on 8 bits for the type of operation to execute : call or data move.
 * 3) Three 16bits parameters (for data move example example : source/
 * destination/counter)
 * 4) Three bytes reserved
 * } seq_fw_task_t
 *
 * structure {
 * uint32 : base address(MSB) + read pointer(LSB)
 * uint32 : max address (MSB) + write pointer (LSB)
 * } FIFO_generic;
 * seq_fw_task_t FIFO_CONTENT [8]; 96 bytes
 *
 * FIFO_SEQ dmem_fifo_sequences [8]; all FIFO sequences
 */
#define dmem_fifo_sequences	D_DCFifo_ADDR
#define dmem_fifo_sequences_descriptors	D_DCFifoDesc_ADDR
/*
 * IRQ FIFOs
 *
 * structure {
 * uint32 : base address(MSB) + read pointer(LSB)
 * uint32 : max address (MSB) + write pointer (LSB)
 * uint32 IRQ_CODES [6];
 * } dmem_fifo_irq_mcu; 32 bytes
 * } dmem_fifo_irq_dsp; 32 bytes
 */
#define dmem_fifo_irq_mcu_descriptor	D_McuIrqFifoDesc_ADDR
#define dmem_fifo_irq_dsp_descriptor	D_DspIrqFifoDesc_ADDR
#define dmem_fifo_irq_mcu	D_McuIrqFifo_ADDR
#define dmem_fifo_irq_dsp	D_DspIrqFifo_ADDR
/*
 * remote debugger exchange buffer
 * uint32 dmem_debug_ae2hal [32]
 * uint32 dmem_debug_hal2ae [32]
 */
#define dmem_debug_ae2hal	D_DebugAbe2hal_ADDR
#define dmem_debug_hal2ae	D_Debug_hal2abe_ADDR
/*
 * DMEM address of the ASRC ppm drift parameter for ASRCs (voice and multimedia
 * paths)
 * uint32 smem_asrc(x)_drift
 */
#define dmem_asrc1_drift	D_ASRC1drift_ADDR
#define dmem_asrc2_drift	D_ASRC2drift_ADDR
/*
 * DMEM indexes of the router uplink paths
 * uint8 dmem_router_index [8]
 */
/* OC: TBD +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* #define dmem_router_index */
/*
 * analog control circular buffer commands to Phoenix
 * structure {
 * uint32 : base address(MSB) + read pointer(LSB)
 * uint32 : max address (MSB) + write pointer (LSB)
 * uint32 FIFO_CONTENT [6];
 * } dmem_commands_to_phoenix; 32 bytes
 */
#define dmem_commands_to_phoenix		D_Cmd2PhenixFifo_ADDR
#define dmem_commands_to_phoenix_descriptor	D_Cmd2PhenixFifoDesc_ADDR
/*
 * analog control circular buffer commands from Phoenix (status line)
 * structure {
 * uint32 : base address(MSB) + read pointer(LSB)
 * uint32 : max address (MSB) + write pointer (LSB)
 * uint32 FIFO_CONTENT [6];
 * } dmem_commands_to_phoenix; 32 bytes
 */
#define dmem_commands_from_phoenix		D_StatusFromPhenixFifo_ADDR
#define dmem_commands_from_phoenix_descriptor	D_StatusFromPhenixFifoDesc_ADDR
/*
 * DEBUG mask
 * uint16 dmem_debug_trace_mask
 * each bit of this word enables a type a trace in the debug circular buffer
 */
/* OC: TBD +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
/* #define dmem_debug_trace_mask */
/*
 * DEBUG circular buffer
 * structure {
 * uint32 : base address(MSB) + read pointer(LSB)
 * uint32 : max address (MSB) + write pointer (LSB)
 * uint32 FIFO_CONTENT [14]; = TIMESTAMP + CODE
 * } dmem_debug_trace_buffer; 64 bytes
 * should be much larger (depends on the DMEM mapping...)
 */
#define dmem_debug_trace_buffer
#define dmem_debug_trace_fifo	D_debugFifo_ADDR
#define dmem_debug_trace_descriptor	D_debugFifoDesc_ADDR
/*
 * Infinite counter incremented on each sheduler periods (~250 us)
 * uint16 dmem_debug_time_stamp
 */
#define dmem_debug_time_stamp	D_loopCounter_ADDR
/*
 * ATC BUFFERS + IO TASKS SMEM buffers
 */
#define dmem_dmic D_DMIC_UL_FIFO_ADDR
#define dmem_dmic_size (D_DMIC_UL_FIFO_sizeof/4)
#define dmem_amic D_McPDM_UL_FIFO_ADDR
#define dmem_amic_size (D_McPDM_UL_FIFO_sizeof/4)
#define smem_amic	AMIC_96_labelID
#define dmem_mcpdm D_McPDM_DL_FIFO_ADDR
#define dmem_mcpdm_size (D_McPDM_DL_FIFO_sizeof/4)
#define dmem_mm_ul D_MM_UL_FIFO_ADDR
#define dmem_mm_ul_size (D_MM_UL_FIFO_sizeof/4)
/* managed directly by the router */
#define smem_mm_ul MM_UL_labelID
#define dmem_mm_ul2 D_MM_UL2_FIFO_ADDR
#define dmem_mm_ul2_size (D_MM_UL2_FIFO_sizeof/4)
/* managed directly by the router */
#define smem_mm_ul2 MM_UL2_labelID
#define dmem_mm_dl D_MM_DL_FIFO_ADDR
#define dmem_mm_dl_size (D_MM_DL_FIFO_sizeof/4)
#define smem_mm_dl MM_DL_labelID
#define dmem_vx_dl D_VX_DL_FIFO_ADDR
#define dmem_vx_dl_size (D_VX_DL_FIFO_sizeof/4)
#define smem_vx_dl	IO_VX_DL_ASRC_labelID	/* Voice_16k_DL_labelID */
#define dmem_vx_ul D_VX_UL_FIFO_ADDR
#define dmem_vx_ul_size (D_VX_UL_FIFO_sizeof/4)
#define smem_vx_ul Voice_8k_UL_labelID
#define dmem_tones_dl D_TONES_DL_FIFO_ADDR
#define dmem_tones_dl_size (D_TONES_DL_FIFO_sizeof/4)
#define smem_tones_dl Tones_labelID
#define dmem_vib_dl D_VIB_DL_FIFO_ADDR
#define dmem_vib_dl_size (D_VIB_DL_FIFO_sizeof/4)
#define smem_vib IO_VIBRA_DL_labelID
#define dmem_mm_ext_out D_MM_EXT_OUT_FIFO_ADDR
#define dmem_mm_ext_out_size (D_MM_EXT_OUT_FIFO_sizeof/4)
#define smem_mm_ext_out DL1_GAIN_out_labelID
#define dmem_mm_ext_in D_MM_EXT_IN_FIFO_ADDR
#define dmem_mm_ext_in_size (D_MM_EXT_IN_FIFO_sizeof/4)
/*IO_MM_EXT_IN_ASRC_labelID	 ASRC input buffer, size 40 */
#define smem_mm_ext_in_opp100 IO_MM_EXT_IN_ASRC_labelID
/* at OPP 50 without ASRC */
#define smem_mm_ext_in_opp50 MM_EXT_IN_labelID
#define dmem_bt_vx_dl D_BT_DL_FIFO_ADDR
#define dmem_bt_vx_dl_size (D_BT_DL_FIFO_sizeof/4)
#define smem_bt_vx_dl_opp50 BT_DL_8k_labelID
/*BT_DL_8k_opp100_labelID  ASRC output buffer, size 40 */
#define smem_bt_vx_dl_opp100 BT_DL_8k_opp100_labelID
#define dmem_bt_vx_ul D_BT_UL_FIFO_ADDR
#define dmem_bt_vx_ul_size (D_BT_UL_FIFO_sizeof/4)
#define smem_bt_vx_ul_opp50 BT_UL_8k_labelID
/*IO_BT_UL_ASRC_labelID	 ASRC input buffer, size 40 */
#define smem_bt_vx_ul_opp100 IO_BT_UL_ASRC_labelID
/*
 * INITPTR / INITREG AREA
 */
/*
 * POINTER - used for the port descriptor programming
 * corresponds to 8bits addresses to the INITPTR area
 *
 * List from ABE_INITxxx_labels.h
 */
#define ptr_ul_rec
#define ptr_vx_dl
#define ptr_mm_dl
#define ptr_mm_ext
#define ptr_tones
#define ptr_vibra2
/*
 * SMEM AREA
 */
/*
 * PHOENIX OFFSET in SMEM
 * used to subtract a DC offset on the headset path
 * (power consumption optimization)
 */
/* OC: exact usage to be detailled */
#define smem_phoenix_offset	S_PhoenixOffset_ADDR
/*
 * EQUALIZERS Z AREA
 * used to reset the filter memory - IIR-8 (max)
 * int24 stereo smem_equ(x) [8x2 + 1]
 */
#define smem_equ1	S_EQU1_data_ADDR
#define smem_equ2	S_EQU2_data_ADDR
#define smem_equ3	S_EQU3_data_ADDR
#define smem_equ4	S_EQU4_data_ADDR
#define smem_sdt	S_SDT_data_ADDR
/*
 * GAIN SMEM on PORT
 * int32 smem_G0 [18] : desired gain on the ports
 * format of G0 = 6 bits left shifted desired gain in linear 24bits format
 * int24 stereo G0 [18] = G0
 * int24 stereo GI [18] current value of the gain in the same format of G0
 * List of smoothed gains :
 * 6 DMIC 0 1 2 3 4 5
 * 2 AMIC L R
 * 4 PORT1/2_RX L R
 * 2 MM_EXT L R
 * 2 MM_VX_DL L R
 * 2 IHF L R
 * ---------------
 * 18 = TOTAL
 */
#if 0
#define smem_g0	S_GTarget_ADDR		/* [9] 2 gains in 1 SM address */
#define smem_g1	S_GCurrent_ADDR		/* [9] 2 gains in 1 SM address */
#endif
/*
 * COEFFICIENTS AREA
 */
/*
 * delay coefficients used in the IIR-1 filters
 * int24 cmem_gain_delay_iir1[9 x 2] (a, (1-a))
 *
 * 3 for 6 DMIC 0 1 2 3 4 5
 * 1 for 2 AMIC L R
 * 2 for 4 PORT1/2_RX L R
 * 1 for 2 MM_EXT L R
 * 1 for 2 MM_VX_DL L R
 * 1 for 2 IHF L R
 */
#define cmem_gain_alpha	C_Alpha_ADDR
#define cmem_gain_1_alpha	C_1_Alpha_ADDR
/*
 * gain controls
 */
#define GAIN_LEFT_OFFSET 0
#define GAIN_RIGHT_OFFSET 1
/* stereo gains */
#define dmic1_gains_offset 0
#define dmic2_gains_offset 2
#define dmic3_gains_offset 4
#define amic_gains_offset 6
#define dl1_gains_offset 8
#define dl2_gains_offset 10
#define splitters_gains_offset 12
#define mixer_dl1_offset 14
#define mixer_dl2_offset 18
#define mixer_echo_offset 22
#define mixer_sdt_offset 24
#define mixer_vxrec_offset 26
#define mixer_audul_offset 30
#define btul_gains_offset 34
/*
 * DMIC SRC 96->48
 * the filter is changed depending on the decimatio ratio used (16/25/32/40)
 * int32 cmem_src2_dmic [6] IIR with 2 coefs in the recursive part and 4 coefs
 * in the direct part
 */
#define cmem_src2_dmic
/*
 * EANC coefficients
 * structure of :
 * 20 Q6.26 coef for the FIR
 * 16 Q6.26 coef for the IIR
 * 1 Q6.26 coef for Lambda
 */
#define cmem_eanc_coef_fir
#define cmem_eanc_coef_iir
#define cmem_eanc_coef_lambda
/*
 * EQUALIZERS - SDT - COEF AREA
 * int24 cmem_equ(x) [8x2+1]
 */
#define cmem_equ1	C_EQU1_data_ADDR
#define cmem_equ2	C_EQU2_data_ADDR
#define cmem_equ3	C_EQU3_data_ADDR
#define cmem_equ4	C_EQU4_data_ADDR
#define cmem_sdt	C_SDT_data_ADDR
/*
 * APS - COEF AREA
 * int24 cmem_aps(x) [16]
 */
#define cmem_aps1
#define cmem_aps2
#define cmem_aps3
/*
 * DITHER - COEF AREA
 * int24 cmem_dither(x) [4]
 */
#define cmem_dither
#endif/* _ABE_FW_H_ */
