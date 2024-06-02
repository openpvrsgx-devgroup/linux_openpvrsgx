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
/**
* abe_fprintf
* @line: character line to be printed
*
* Print ABE debug messages.
*/
/**
 * abe_read_feature_from_port
 * @x: d
 *
 * TBD
 *
 */
void abe_read_feature_from_port(u32 x)
{
}
/**
 * abe_write_feature_to_port
 * @x: d
 *
 * TBD
 *
 */
void abe_write_feature_to_port(u32 x)
{
}
/**
 * abe_read_fifo
 * @x: d
 *
 * TBD
 */
void abe_read_fifo(u32 x)
{
}
/**
 * abe_write_fifo
 * @mem_bank: currently only ABE_DMEM supported
 * @addr: FIFO descriptor address ( descriptor fields : READ ptr, WRITE ptr,
 * FIFO START_ADDR, FIFO END_ADDR)
 * @data: data to write to FIFO
 * @number: number of 32-bit words to write to DMEM FIFO
 *
 * write DMEM FIFO and update FIFO descriptor,
 * it is assumed that FIFO descriptor is located in DMEM
 */
void abe_write_fifo(u32 memory_bank, u32 descr_addr, u32 *data, u32 nb_data32)
{
	u32 fifo_addr[4];
	u32 i;
	/* read FIFO descriptor from DMEM */
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM, descr_addr,
		       &fifo_addr[0], 4 * sizeof(u32));
	/* WRITE ptr < FIFO start address */
	if (fifo_addr[1] < fifo_addr[2])
		abe_dbg_error_log(ABE_FW_FIFO_WRITE_PTR_ERR);
	/* WRITE ptr > FIFO end address */
	if (fifo_addr[1] > fifo_addr[3])
		abe_dbg_error_log(ABE_FW_FIFO_WRITE_PTR_ERR);
	switch (memory_bank) {
	case ABE_DMEM:
		for (i = 0; i < nb_data32; i++) {
			abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM,
				       (s32) fifo_addr[1], (u32 *) (data + i),
				       4);
			/* increment WRITE pointer */
			fifo_addr[1] = fifo_addr[1] + 4;
			if (fifo_addr[1] > fifo_addr[3])
				fifo_addr[1] = fifo_addr[2];
			if (fifo_addr[1] == fifo_addr[0])
				abe_dbg_error_log(ABE_FW_FIFO_WRITE_PTR_ERR);
		}
		/* update WRITE pointer in DMEM */
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, descr_addr +
			       sizeof(u32), &fifo_addr[1], 4);
		break;
	default:
		/* printf("ERROR: Only DMEM FIFO write is supported \n"); */
		break;
	}
}
/**
 * abe_monitoring
 *
 * checks the internal status of ABE and HAL
 */
void abe_monitoring(void)
{
	abe_dbg_param = 0;
}
/**
 * abe_format_switch
 * @f: port format
 * @iter: port iteration
 * @mulfac: multiplication factor
 *
 * translates the sampling and data length to ITER number for the DMA
 * and the multiplier factor to apply during data move with DMEM
 *
 */
void abe_format_switch(abe_data_format_t *f, u32 *iter, u32 *mulfac)
{
	u32 n_freq;
#if FW_SCHED_LOOP_FREQ == 4000
	switch (f->f) {
		/* nb of samples processed by scheduling loop */
	case 8000:
		n_freq = 2;
		break;
	case 16000:
		n_freq = 4;
		break;
	case 24000:
		n_freq = 6;
		break;
	case 44100:
		n_freq = 12;
		break;
	case 96000:
		n_freq = 24;
		break;
	default/*case 48000 */ :
		n_freq = 12;
		break;
	}
#else
	/* erroneous cases */
	n_freq = 0;
#endif
	switch (f->samp_format) {
	case MONO_MSB:
	case MONO_RSHIFTED_16:
	case STEREO_16_16:
		*mulfac = 1;
		break;
	case STEREO_MSB:
	case STEREO_RSHIFTED_16:
		*mulfac = 2;
		break;
	case THREE_MSB:
		*mulfac = 3;
		break;
	case FOUR_MSB:
		*mulfac = 4;
		break;
	case FIVE_MSB:
		*mulfac = 5;
		break;
	case SIX_MSB:
		*mulfac = 6;
		break;
	case SEVEN_MSB:
		*mulfac = 7;
		break;
	case EIGHT_MSB:
		*mulfac = 8;
		break;
	case NINE_MSB:
		*mulfac = 9;
		break;
	default:
		*mulfac = 1;
		break;
	}
	*iter = (n_freq * (*mulfac));
	if (f->samp_format == MONO_16_16)
		*iter /= 2;
}
/**
 * abe_dma_port_iteration
 * @f: port format
 *
 * translates the sampling and data length to ITER number for the DMA
 */
u32 abe_dma_port_iteration(abe_data_format_t *f)
{
	u32 iter, mulfac;
	abe_format_switch(f, &iter, &mulfac);
	return iter;
}
/**
 * abe_dma_port_iter_factor
 * @f: port format
 *
 * returns the multiplier factor to apply during data move with DMEM
 */
u32 abe_dma_port_iter_factor(abe_data_format_t *f)
{
	u32 iter, mulfac;
	abe_format_switch(f, &iter, &mulfac);
	return mulfac;
}
/**
 * abe_dma_port_copy_subroutine_id
 *
 * @port_id: ABE port ID
 *
 * returns the index of the function doing the copy in I/O tasks
 */
u32 abe_dma_port_copy_subroutine_id(u32 port_id)
{
	u32 sub_id;
	if (abe_port[port_id].protocol.direction == ABE_ATC_DIRECTION_IN) {
		switch (abe_port[port_id].format.samp_format) {
		case MONO_MSB:
			sub_id = D2S_MONO_MSB_CFPID;
			break;
		case MONO_RSHIFTED_16:
			sub_id = D2S_MONO_RSHIFTED_16_CFPID;
			break;
		case STEREO_RSHIFTED_16:
			sub_id = D2S_STEREO_RSHIFTED_16_CFPID;
			break;
		case STEREO_16_16:
			sub_id = D2S_STEREO_16_16_CFPID;
			break;
		case MONO_16_16:
			sub_id = D2S_MONO_16_16_CFPID;
			break;
		case STEREO_MSB:
			sub_id = D2S_STEREO_MSB_CFPID;
			break;
		case SIX_MSB:
			if (port_id == DMIC_PORT) {
				sub_id = COPY_DMIC_CFPID;
				break;
			}
		default:
			sub_id = NULL_COPY_CFPID;
			break;
		}
	} else {
		switch (abe_port[port_id].format.samp_format) {
		case MONO_MSB:
			sub_id = S2D_MONO_MSB_CFPID;
			break;
		case MONO_RSHIFTED_16:
			sub_id = S2D_MONO_RSHIFTED_16_CFPID;
			break;
		case STEREO_RSHIFTED_16:
			sub_id = S2D_STEREO_RSHIFTED_16_CFPID;
			break;
		case STEREO_16_16:
			sub_id = S2D_STEREO_16_16_CFPID;
			break;
		case MONO_16_16:
			sub_id = S2D_MONO_16_16_CFPID;
			break;
		case STEREO_MSB:
			sub_id = S2D_STEREO_MSB_CFPID;
			break;
		case SIX_MSB:
			if (port_id == PDM_DL_PORT) {
				if (abe_mcpdm_path == 1)
					sub_id = COPY_MCPDM_DL_CFPID;					// Separate headset and handsfree path
				else if (abe_mcpdm_path == 2)
					sub_id = COPY_MCPDM_DL_HF_PDL1_CFPID;			// OPP25 on handsfree path and mute headset
				else if (abe_mcpdm_path == 3)
					sub_id = COPY_MCPDM_DL_HF_PDL2_CFPID;			// OPP25 on headset and handsfree at the same time
				else
					sub_id = COPY_MCPDM_DL_CFPID;					// Default case, headset path		
				break;
			}
			if (port_id == MM_UL_PORT) {
				sub_id = COPY_MM_UL_CFPID;
				break;
			}
		case THREE_MSB:
		case FOUR_MSB:
		case FIVE_MSB:
		case SEVEN_MSB:
		case EIGHT_MSB:
		case NINE_MSB:
			sub_id = COPY_MM_UL_CFPID;
			break;
		default:
			sub_id = NULL_COPY_CFPID;
			break;
		}
	}
	return sub_id;
}
/**
 * abe_int_2_float
 * returns a mantissa on 16 bits and the exponent
 * 0x4000.0000 leads to M=0x4000 X=15
 * 0x0004.0000 leads to M=0x4000 X=4
 * 0x0000.0001 leads to M=0x4000 X=-14
 *
 */
void abe_int_2_float16(u32 data, u32 *mantissa, u32 *exp)
{
	u32 i;
	*exp = 0;
	*mantissa = 0;
	for (i = 0; i < 32; i++) {
		if ((1 << i) > data)
			break;
	}
	*exp = i - 15;
	*mantissa = (*exp > 0) ? data >> (*exp) : data << (*exp);
}
/**
 * abe_gain_offset
 * returns the offset to firmware data structures
 *
 */
void abe_gain_offset(u32 id, u32 *mixer_offset)
{
	switch (id) {
	default:
	case GAINS_DMIC1:
		*mixer_offset = dmic1_gains_offset;
		break;
	case GAINS_DMIC2:
		*mixer_offset = dmic2_gains_offset;
		break;
	case GAINS_DMIC3:
		*mixer_offset = dmic3_gains_offset;
		break;
	case GAINS_AMIC:
		*mixer_offset = amic_gains_offset;
		break;
	case GAINS_DL1:
		*mixer_offset = dl1_gains_offset;
		break;
	case GAINS_DL2:
		*mixer_offset = dl2_gains_offset;
		break;
	case GAINS_SPLIT:
		*mixer_offset = splitters_gains_offset;
		break;
	case MIXDL1:
		*mixer_offset = mixer_dl1_offset;
		break;
	case MIXDL2:
		*mixer_offset = mixer_dl2_offset;
		break;
	case MIXECHO:
		*mixer_offset = mixer_echo_offset;
		break;
	case MIXSDT:
		*mixer_offset = mixer_sdt_offset;
		break;
	case MIXVXREC:
		*mixer_offset = mixer_vxrec_offset;
		break;
	case MIXAUDUL:
		*mixer_offset = mixer_audul_offset;
		break;
	case GAINS_BTUL:
		*mixer_offset = btul_gains_offset;
		break;
	}
}
/**
 * abe_decide_main_port - Select stynchronization port for Event generator.
 * @id: audio port name
 *
 * tells the FW which is the reference stream for adjusting
 * the processing on 23/24/25 slots
 *
 * takes the first port in a list which is slave on the data interface
 */
u32 abe_valid_port_for_synchro(u32 id)
{
	if ((abe_port[id].protocol.protocol_switch ==
	     DMAREQ_PORT_PROT) ||
	    (abe_port[id].protocol.protocol_switch ==
	     PINGPONG_PORT_PROT) ||
	    (abe_port[id].status != OMAP_ABE_PORT_ACTIVITY_RUNNING))
		return 0;
	else
		return 1;
}
void abe_decide_main_port(void)
{
	u32 id, id_not_found;
	id_not_found = 1;
	for (id = 0; id < LAST_PORT_ID - 1; id++) {
		if (abe_valid_port_for_synchro(abe_port_priority[id])) {
			id_not_found = 0;
			break;
		}
	}
	/* if no port is currently activated, the default one is PDM_DL */
	if (id_not_found)
		abe_select_main_port(PDM_DL_PORT);
	else
		abe_select_main_port(abe_port_priority[id]);
}