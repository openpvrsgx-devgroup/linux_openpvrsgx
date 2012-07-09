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
 * abe_dbg_log - Log ABE trace inside circular buffer
 * @x: data to be logged
 * @y: data to be logged
 * @z: data to be logged
 * @t: data to be logged
 *  Parameter  :
 *
 *	abe_dbg_activity_log : global circular buffer holding the data
 *	abe_dbg_activity_log_write_pointer : circular write pointer
 *
 *	saves data in the log file
 */
void abe_dbg_log(u32 x, u32 y, u32 z, u32 t)
{
	u32 time_stamp, data;
	if (abe_dbg_activity_log_write_pointer >= (D_DEBUG_HAL_TASK_sizeof - 2))
		abe_dbg_activity_log_write_pointer = 0;
	/* copy in DMEM trace buffer and CortexA9 local buffer and a small 7
	   words circular buffer of the DMA trace ending with 0x55555555
	   (tag for last word) */
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM, D_loopCounter_ADDR,
		       (u32 *) &time_stamp, sizeof(time_stamp));
	abe_dbg_activity_log[abe_dbg_activity_log_write_pointer] = time_stamp;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, D_DEBUG_HAL_TASK_ADDR +
		       (abe_dbg_activity_log_write_pointer << 2),
		       (u32 *) &time_stamp, sizeof(time_stamp));
	abe_dbg_activity_log_write_pointer++;
	data = ((x & MAX_UINT8) << 24) | ((y & MAX_UINT8) << 16) |
		((z & MAX_UINT8) << 8)
		| (t & MAX_UINT8);
	abe_dbg_activity_log[abe_dbg_activity_log_write_pointer] = data;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, D_DEBUG_HAL_TASK_ADDR +
		       (abe_dbg_activity_log_write_pointer << 2),
		       (u32 *) &data, sizeof(data));
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM,
		       D_DEBUG_FIFO_HAL_ADDR +
		       ((abe_dbg_activity_log_write_pointer << 2) &
			(D_DEBUG_FIFO_HAL_sizeof - 1)), (u32 *) &data,
		       sizeof(data));
	data = ABE_DBG_MAGIC_NUMBER;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, D_DEBUG_FIFO_HAL_ADDR +
		       (((abe_dbg_activity_log_write_pointer +
			  1) << 2) & (D_DEBUG_FIFO_HAL_sizeof - 1)),
		       (u32 *) &data, sizeof(data));
	abe_dbg_activity_log_write_pointer++;
	if (abe_dbg_activity_log_write_pointer >= D_DEBUG_HAL_TASK_sizeof)
		abe_dbg_activity_log_write_pointer = 0;
}
/**
 * abe_debug_output_pins
 * @x: d
 *
 * set the debug output pins of AESS
 */
void abe_debug_output_pins(u32 x)
{
}
/**
 * abe_dbg_error_log -  Log ABE error
 * @x: error to log
 *
 * log the error codes
 */
void abe_dbg_error_log(u32 x)
{
	abe_dbg_log(x, MAX_UINT8, MAX_UINT8, MAX_UINT8);
}
/**
 * abe_debugger
 * @x: error to log
 *
 * log error for debugger
 */
void abe_debugger(u32 x)
{
}
/**
 * abe_load_embeddded_patterns
 *
 * load test patterns
 *
 *  S = power (2, 31) * 0.25;
 *  N =  4; B = 2; F=[1/N 1/N];
 *	gen_and_save('dbg_8k_2.txt',  B, F, N, S);
 *  N =  8; B = 2; F=[1/N 2/N];
 *	gen_and_save('dbg_16k_2.txt', B, F, N, S);
 *  N = 12; B = 2; F=[1/N 2/N];
 *	gen_and_save('dbg_48k_2.txt', B, F, N, S);
 *  N = 60; B = 2; F=[4/N 8/N];
 *	gen_and_save('dbg_amic.txt', B, F, N, S);
 *  N = 10; B = 6; F=[1/N 2/N 3/N 1/N 2/N 3/N];
 *	gen_and_save('dbg_dmic.txt', B, F, N, S);
*/
void abe_load_embeddded_patterns(void)
{
	u32 i;
#define patterns_96k_len 48
	const long patterns_96k[patterns_96k_len] = {
		1620480, 1452800,
		1452800, 838656,
		1186304, 0,
		838656, -838912,
		434176, -1453056,
		0, -1677824,
		-434432, -1453056,
		-838912, -838912,
		-1186560, -256,
		-1453056, 838656,
		-1620736, 1452800,
		-1677824, 1677568,
		-1620736, 1452800,
		-1453056, 838656,
		-1186560, 0,
		-838912, -838912,
		-434432, -1453056,
		-256, -1677824,
		434176, -1453056,
		838656, -838912,
		1186304, -256,
		1452800, 838656,
		1620480, 1452800,
		1677568, 1677568,
	};
#define patterns_48k_len 24
	const long patterns_48k[patterns_48k_len] = {
		1452800, 838656,
		838656, -838912,
		0, -1677824,
		-838912, -838912,
		-1453056, 838656,
		-1677824, 1677568,
		-1453056, 838656,
		-838912, -838912,
		-256, -1677824,
		838656, -838912,
		1452800, 838656,
		1677568, 1677568,
	};
#define patterns_24k_len 12
	const long patterns_24k[patterns_24k_len] = {
		838656, -838912,
		-838912, -838912,
		-1677824, 1677568,
		-838912, -838912,
		838656, -838912,
		1677568, 1677568,
	};
#define patterns_16k_len 8
	const long patterns_16k[patterns_16k_len] = {
		0, 0,
		-1677824, -1677824,
		-256, -256,
		1677568, 1677568,
	};
#define patterns_8k_len 4
	const long patterns_8k[patterns_8k_len] = {
		1677568, -1677824,
		1677568, 1677568,
	};
	for (i = 0; i < patterns_8k_len; i++)
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
			       (S_DBG_8K_PATTERN_ADDR * 8) + (i * 4),
			       (u32 *) (&(patterns_8k[i])), 4);
	for (i = 0; i < patterns_16k_len; i++)
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
			       (S_DBG_16K_PATTERN_ADDR * 8) + (i * 4),
			       (u32 *) (&(patterns_16k[i])), 4);
	for (i = 0; i < patterns_24k_len; i++)
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
			       (S_DBG_24K_PATTERN_ADDR * 8) + (i * 4),
			       (u32 *) (&(patterns_24k[i])), 4);
	for (i = 0; i < patterns_48k_len; i++)
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
			       (S_DBG_48K_PATTERN_ADDR * 8) + (i * 4),
			       (u32 *) (&(patterns_48k[i])), 4);
	for (i = 0; i < patterns_96k_len; i++)
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
			       (S_DBG_96K_PATTERN_ADDR * 8) + (i * 4),
			       (u32 *) (&(patterns_96k[i])), 4);
}
