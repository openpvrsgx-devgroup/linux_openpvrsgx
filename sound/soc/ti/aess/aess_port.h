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

#ifndef _AESS_PORT_H_
#define _AESS_PORT_H_

#include "aess-fw.h"

struct omap_aess;

struct omap_aess_dma {
	void *data;
	u32 iter;
};

void omap_aess_reset_port(struct omap_aess *abe, u32 id);
void omap_aess_select_main_port(struct omap_aess *abe, u32 id);

void omap_aess_enable_data_transfer(struct omap_aess *aess, u32 id);
void omap_aess_disable_data_transfer(struct omap_aess *aess, u32 id);

int omap_aess_check_activity(struct omap_aess *aess);

void omap_aess_connect_cbpr_dmareq_port(struct omap_aess *aess, u32 id,
					struct omap_aess_data_format *f, u32 d,
				        struct omap_aess_dma *aess_dma);
void omap_aess_connect_serial_port(struct omap_aess *aess, u32 id,
				   struct omap_aess_data_format *f,
				   u32 mcbsp_id, struct omap_aess_dma *aess_dma);
int omap_aess_connect_irq_ping_pong_port(struct omap_aess *aess, u32 id,
					 struct omap_aess_data_format *f,
					 u32 subroutine_id, u32 size, u32 *sink,
					 u32 dsp_mcu_flag);

int omap_aess_set_ping_pong_buffer(struct omap_aess *aess, u32 port,
				   u32 n_bytes);
int omap_aess_read_next_ping_pong_buffer(struct omap_aess *aess, u32 port,
					 u32 *p, u32 *n);
int omap_aess_read_offset_from_ping_buffer(struct omap_aess *aess, u32 id,
					   u32 *n);

/* TODO: Should this to be moved to omap-aess-fw.c ?? */
void omap_aess_mono_mixer(struct omap_aess *aess, u32 id, u32 on_off);

#endif /* _AESS_PORT_H_ */
