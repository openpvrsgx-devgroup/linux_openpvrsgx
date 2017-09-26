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

#ifndef _AESS_MEM_H_
#define _AESS_MEM_H_

#include <asm/io.h>
#include <linux/pm_runtime.h>

struct omap_aess;

static inline void omap_aess_write(struct omap_aess *aess, int bank, u32 offset,
				   void *src, size_t bytes)
{
	if (pm_runtime_active(aess->dev))
		memcpy((void __force *)(aess->io_base[bank] + offset), src,
		       bytes);
}

static inline void omap_aess_read(struct omap_aess *aess, int bank, u32 offset,
				  void *dest, size_t bytes)
{
	if (pm_runtime_active(aess->dev))
		memcpy(dest, (void __force *)(aess->io_base[bank] + offset),
		       bytes);
}

static inline u32 omap_aess_reg_read(struct omap_aess *aess, u32 offset)
{
	if (pm_runtime_active(aess->dev))
		return __raw_readl(aess->io_base[OMAP_AESS_BANK_IP] + offset);

	return 0;
}

static inline void omap_aess_reg_write(struct omap_aess *aess, u32 offset,
				       u32 val)
{
	if (pm_runtime_active(aess->dev))
		__raw_writel(val, (aess->io_base[OMAP_AESS_BANK_IP] + offset));
}

static inline void *omap_aess_clear(struct omap_aess *aess, int bank,
				    u32 offset, size_t bytes)
{
	if (pm_runtime_active(aess->dev))
		return memset((void __force *)(aess->io_base[bank] + offset), 0,
			      bytes);
	return NULL;
}

/* struct omap_aess_addr based operations */
#define omap_aess_mem_write(aess, addr, src) \
	omap_aess_write(aess, addr.bank, addr.offset, src, addr.bytes)

#define omap_aess_mem_read(aess, addr, dst) \
	omap_aess_read(aess, addr.bank, addr.offset, dst, addr.bytes)

#define omap_aess_mem_reset(aess, addr) \
	omap_aess_clear(aess, addr.bank, addr.offset, addr.bytes)

/* Operations via aess->fw_info.map[] */
#define omap_aess_write_map(aess, map_id, src) \
	omap_aess_write(aess, aess->fw_info.map[map_id].bank, \
			aess->fw_info.map[map_id].offset, src, \
			aess->fw_info.map[map_id].bytes)

#define omap_aess_reset_map(aess, map_id) \
	omap_aess_clear(aess, aess->fw_info.map[map_id].bank, \
			aess->fw_info.map[map_id].offset, \
			aess->fw_info.map[map_id].bytes)

#endif /*_AESS_MEM_H_*/
