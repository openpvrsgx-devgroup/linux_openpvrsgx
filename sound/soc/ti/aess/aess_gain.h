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

#ifndef _AESS_GAIN_H_
#define _AESS_GAIN_H_

#define GAIN_MAXIMUM	3000L
#define GAIN_24dB	2400L
#define GAIN_18dB	1800L
#define GAIN_12dB	1200L
#define GAIN_6dB	600L
/* default gain = 1 */
#define GAIN_0dB	0L
#define GAIN_M6dB	(-600L)
#define GAIN_M7dB	(-700L)
#define GAIN_M12dB	(-1200L)
#define GAIN_M18dB	(-1800L)
#define GAIN_M24dB	(-2400L)
#define GAIN_M30dB	(-3000L)
#define GAIN_M40dB	(-4000L)
#define GAIN_M50dB	(-5000L)
/* muted gain = -120 decibels */
#define GAIN_MUTE	(-12000L)
#define GAIN_TOOLOW	(-13000L)
#define RAMP_MINLENGTH	0L
/* ramp_t is in milli- seconds */
#define RAMP_0MS	0L
#define RAMP_1MS	1L
#define RAMP_2MS	2L
#define RAMP_5MS	5L
#define RAMP_10MS	10L
#define RAMP_20MS	20L
#define RAMP_50MS	50L
#define RAMP_100MS	100L
#define RAMP_200MS	200L
#define RAMP_500MS	500L
#define RAMP_1000MS	1000L
#define RAMP_MAXLENGTH	10000L

struct omap_aess;

void omap_aess_init_gains(struct omap_aess *aess);
void omap_aess_write_gain(struct omap_aess *aess, u32 id, s32 f_g);
int omap_aess_read_gain(struct omap_aess *aess, u32 id, u32 *f_g);
#define omap_aess_write_mixer(aess, id, f_g) omap_aess_write_gain(aess, id, f_g)
#define omap_aess_read_mixer(aess, id, f_g) omap_aess_read_gain(aess, id, f_g)

void omap_aess_reset_gain_mixer(struct omap_aess *aess, u32 id);

void omap_aess_init_gain_ramp(struct omap_aess *aess);

void omap_aess_enable_gain(struct omap_aess *aess, u32 id);
void omap_aess_disable_gain(struct omap_aess *aess, u32 id);

void omap_aess_mute_gain(struct omap_aess *aess, u32 id);
void omap_aess_unmute_gain(struct omap_aess *aess, u32 id);

#endif /* _AESS_GAIN_H_ */
