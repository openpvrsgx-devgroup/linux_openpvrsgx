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
#define ENABLE_DEFAULT_PLAYERS 1
/**
 * abe_default_irq_pingpong_player
 *
 * generates data for the cache-flush buffer MODE 16+16
 */
void abe_default_irq_pingpong_player(void)
{
#if ENABLE_DEFAULT_PLAYERS
/* ping-pong access to MM_DL at 48kHz Mono with 20ms packet sizes */
#define N_SAMPLES_MAX ((int)(1024))
	static s32 idx;
	u32 i, dst, n_samples, n_bytes;
	s32 temp[N_SAMPLES_MAX], audio_sample;
#define DATA_SIZE 20	/* t = [0:N-1]/N; x = round(16383*sin(2*pi*t)) */
	const s32 audio_pattern[DATA_SIZE] = {
		0, 5063, 9630, 13254, 15581, 16383, 15581, 13254, 9630,
		5063, 0, -5063, -9630, -13254, -15581, -16383, -15581,
		-13254, -9630, -5063
	};
#if 0
#define DATA_SIZE 8
	const s32 audio_pattern[DATA_SIZE] = {
		0, 11585, 16384, 11585, 0, -11586, -16384, -11586
	};
#define DATA_SIZE 12
	const s32 audio_pattern[DATA_SIZE] = {
		0, 8191, 14188, 16383, 14188, 8191, 0,
		-8192, -14188, -16383, -14188, -8192
	};
	const s32 audio_pattern[8] = {
		16383, 16383, 16383, 16383, -16384, -16384, -16384, -16384
	};
#endif
	/* read the address of the Pong buffer */
	abe_read_next_ping_pong_buffer(MM_DL_PORT, &dst, &n_bytes);
	/* each stereo sample weights 4 bytes (format 16|16) */
	n_samples = n_bytes / 4;
	/* generate a test pattern */
	for (i = 0; i < n_samples; i++) {
		audio_sample = audio_pattern[idx];
		idx = (idx >= (DATA_SIZE - 1)) ? 0 : (idx + 1);
		/* format 16|16 */
		temp[i] = ((audio_sample << 16) + audio_sample);
	}
	/* copy the pattern (flush it) to DMEM pointer update
	 * not necessary here because the buffer size do not
	 * change from one ping to the other pong
	 */
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, dst,
		       (u32 *) &(temp[0]), n_bytes);
	abe_set_ping_pong_buffer(MM_DL_PORT, n_bytes);
#endif
}
/**
 * abe_default_irq_pingpong_player_32bits
 *
 * generates data for the cache-flush buffer  MODE 32 BITS
 * Return value:
 * None.
 */
void abe_default_irq_pingpong_player_32bits(void)
{
#if ENABLE_DEFAULT_PLAYERS
	/* ping-pong access to MM_DL at 48kHz Mono with 20ms packet sizes */
	static s32 idx;
	u32 i, dst, n_samples, n_bytes;
	s32 temp[N_SAMPLES_MAX], audio_sample;
#define DATA_SIZE 20	/*  t = [0:N-1]/N; x = round(16383*sin(2*pi*t)) */
	const s32 audio_pattern[DATA_SIZE] = {
		0, 5063, 9630, 13254, 15581, 16383, 15581, 13254,
		9630, 5063, 0, -5063, -9630, -13254, -15581, -16383,
		-15581, -13254, -9630, -5063
	};
	/* read the address of the Pong buffer */
	abe_read_next_ping_pong_buffer(MM_DL_PORT, &dst, &n_bytes);
	/* each stereo sample weights 8 bytes (format 32|32) */
	n_samples = n_bytes / 8;
	/* generate a test pattern */
	for (i = 0; i < n_samples; i++) {
		/* circular addressing */
		audio_sample = audio_pattern[idx];
		idx = (idx >= (DATA_SIZE - 1)) ? 0 : (idx + 1);
		temp[i * 2 + 0] = (audio_sample << 16);
		temp[i * 2 + 1] = (audio_sample << 16);
	}
	abe_set_ping_pong_buffer(MM_DL_PORT, 0);
	/* copy the pattern (flush it) to DMEM pointer update
	 * not necessary here because the buffer size do not
	 * change from one ping to the other pong
	 */
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, dst,
		       (u32 *) &(temp[0]), n_bytes);
	abe_set_ping_pong_buffer(MM_DL_PORT, n_bytes);
#endif
}
/**
 * abe_rshifted16_irq_pingpong_player_32bits
 *
 * generates data for the cache-flush buffer  MODE 32 BITS
 * Return value:
 * None.
 */
void abe_rshifted16_irq_pingpong_player_32bits(void)
{
#if ENABLE_DEFAULT_PLAYERS
	/* ping-pong access to MM_DL at 48kHz Mono with 20ms packet sizes */
	static s32 idx;
	u32 i, dst, n_samples, n_bytes;
	s32 temp[N_SAMPLES_MAX], audio_sample;
#define DATA_SIZE 20	/*  t = [0:N-1]/N; x = round(16383*sin(2*pi*t)) */
	const s32 audio_pattern[DATA_SIZE] = {
		0, 5063, 9630, 13254, 15581, 16383, 15581, 13254,
		9630, 5063, 0, -5063, -9630, -13254, -15581, -16383,
		-15581, -13254, -9630, -5063
	};
	/* read the address of the Pong buffer */
	abe_read_next_ping_pong_buffer(MM_DL_PORT, &dst, &n_bytes);
	/* each stereo sample weights 8 bytes (format 32|32) */
	n_samples = n_bytes / 8;
	/* generate a test pattern */
	for (i = 0; i < n_samples; i++) {
		/* circular addressing */
		audio_sample = audio_pattern[idx];
		idx = (idx >= (DATA_SIZE - 1)) ? 0 : (idx + 1);
		temp[i * 2 + 0] = audio_sample;
		temp[i * 2 + 1] = audio_sample;
	}
	abe_set_ping_pong_buffer(MM_DL_PORT, 0);
	/* copy the pattern (flush it) to DMEM pointer update
	 * not necessary here because the buffer size do not
	 * change from one ping to the other pong
	 */
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, dst,
		       (u32 *) &(temp[0]), n_bytes);
	abe_set_ping_pong_buffer(MM_DL_PORT, n_bytes);
#endif
}
/**
 * abe_1616_irq_pingpong_player_1616bits
 *
 * generates data for the cache-flush buffer  MODE 16+16 BITS
 * Return value:
 * None.
 */
void abe_1616_irq_pingpong_player_1616bits(void)
{
#if ENABLE_DEFAULT_PLAYERS
	/* ping-pong access to MM_DL at 48kHz Mono with 20ms packet sizes */
	static s32 idx;
	u32 i, dst, n_samples, n_bytes;
	s32 temp[N_SAMPLES_MAX], audio_sample;
#define DATA_SIZE 20	/*  t = [0:N-1]/N; x = round(16383*sin(2*pi*t)) */
	const s32 audio_pattern[DATA_SIZE] = {
		0, 5063, 9630, 13254, 15581, 16383, 15581, 13254,
		9630, 5063, 0, -5063, -9630, -13254, -15581, -16383,
		-15581, -13254, -9630, -5063
	};
	/* read the address of the Pong buffer */
	abe_read_next_ping_pong_buffer(MM_DL_PORT, &dst, &n_bytes);
	/* each stereo sample weights 4 bytes (format 16+16) */
	n_samples = n_bytes / 4;
	/* generate a test pattern */
	for (i = 0; i < n_samples; i++) {
		/* circular addressing */
		audio_sample = audio_pattern[idx];
		idx = (idx >= (DATA_SIZE - 1)) ? 0 : (idx + 1);
		temp[i] = (audio_sample << 16) | (audio_sample & 0x0000FFFF);
		//temp[i] = (i << 16)| ((i  )& 0x0000FFFF);
	}
	abe_set_ping_pong_buffer(MM_DL_PORT, 0);
	/* copy the pattern (flush it) to DMEM pointer update
	 * not necessary here because the buffer size do not
	 * change from one ping to the other pong
	 */
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, dst,
		       (u32 *) &(temp[0]), n_bytes);
	abe_set_ping_pong_buffer(MM_DL_PORT, n_bytes);
#endif
}
/**
 * abe_1616_irq_pingpong_player_mono_1616bits
 *
 * generates data for the cache-flush buffer  MODE MONO 16+16 BITS
 * Return value:
 * None.
 */
void abe_1616_irq_pingpong_player_mono_1616bits(void)
{
#if ENABLE_DEFAULT_PLAYERS
	/* ping-pong access to MM_DL at 48kHz Mono with 20ms packet sizes */
	static s32 idx;
	u32 i, dst, n_samples, n_bytes;
	s32 temp[N_SAMPLES_MAX], audio_sample, audio_sample2;
#define DATA_SIZE 20	/*  t = [0:N-1]/N; x = round(16383*sin(2*pi*t)) */
	const s32 audio_pattern[DATA_SIZE] = {
		0, 5063, 9630, 13254, 15581, 16383, 15581, 13254,
		9630, 5063, 0, -5063, -9630, -13254, -15581, -16383,
		-15581, -13254, -9630, -5063
	};
	/* read the address of the Pong buffer */
	abe_read_next_ping_pong_buffer(MM_DL_PORT, &dst, &n_bytes);
	/* each dual mono sample weights 4 bytes (format 16+16) */
	n_samples = n_bytes / 4;
	/* generate a test pattern */
	for (i = 0; i < n_samples; i++) {
		/* circular addressing */
		audio_sample = audio_pattern[idx];
		audio_sample2 = audio_pattern[idx+1];
		idx = (idx >= (DATA_SIZE - 2)) ? 0 : (idx + 2);			// -2 because of idx+1
		temp[i] = (audio_sample << 16) | (audio_sample2 & 0x0000FFFF);
		//temp[i] = (2*i << 16)| ((2*i +1 )& 0x0000FFFF);
	}
	/* copy the pattern (flush it) to DMEM pointer update
	 * not necessary here because the buffer size do not
	 * change from one ping to the other pong
	 */
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, dst,
		       (u32 *) &(temp[0]), n_bytes);
	abe_set_ping_pong_buffer(MM_DL_PORT, n_bytes);
#endif
}
/**
 * abe_default_irq_aps_adaptation
 *
 * updates the APS filter and gain
 */
void abe_default_irq_aps_adaptation(void)
{
}
/**
 * abe_read_sys_clock
 * @time: pointer to the system clock
 *
 * returns the current time indication for the LOG
 */
void abe_read_sys_clock(u32 *time)
{
	static u32 clock;
	*time = clock;
	clock++;
}
/**
 * abe_aps_tuning
 *
 * Tune APS parameters
 *
 */
void abe_aps_tuning(void)
{
}
