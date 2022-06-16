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
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/device.h>

#include "omap-aess-priv.h"
#include "aess_ini.h"
#include "aess_gain.h"
#include "aess_mem.h"
#include "aess_port.h"
#include "aess_utils.h"

/* nb of samples to route */
#define NBROUTE_UL 16

/**
 * omap_aess_reset_all_ports
 * @aess: Pointer on aess handle
 *
 * load default configuration for all features
 */
static void omap_aess_reset_all_ports(struct omap_aess *aess)
{
	u16 i;

	for (i = 0; i < OMAP_AESS_LAST_PHY_PORT_ID; i++)
		omap_aess_reset_port(aess, i);
}

/**
 * omap_aess_init_mem - Allocate Kernel space memory map for AESS
 * @aess: Pointer on aess handle
 * @fw_header: Pointer on the firmware header commit from the FS.
 *
 * Memory map of AESS memory space for PMEM/DMEM/SMEM/DMEM
 */
void omap_aess_init_mem(struct omap_aess *aess)
{
	u32 *fw_header = (u32*) aess->fw_config;
	struct omap_aess_mapping *fw_info = &aess->fw_info;
	int offset = 0;
	u32 count;

	dev_dbg(aess->dev, "DMEM bank at 0x%p\n",
		aess->io_base[OMAP_AESS_BANK_DMEM]);
	dev_dbg(aess->dev, "CMEM bank at 0x%p\n",
		aess->io_base[OMAP_AESS_BANK_CMEM]);
	dev_dbg(aess->dev, "SMEM bank at 0x%p\n",
		aess->io_base[OMAP_AESS_BANK_SMEM]);
	dev_dbg(aess->dev, "PMEM bank at 0x%p\n",
		aess->io_base[OMAP_AESS_BANK_PMEM]);
	dev_dbg(aess->dev, "AESS bank at 0x%p\n",
		aess->io_base[OMAP_AESS_BANK_IP]);

if(!fw_header)
{
	dev_err(aess->dev, "Missing firmware config\n");
	return;
}
	/* get mapping */
	count = fw_header[offset];
	dev_dbg(aess->dev, "Map %d items of size 0x%x at offset 0x%x\n", count,
		sizeof(struct omap_aess_addr), offset << 2);
	fw_info->map = (struct omap_aess_addr *)&fw_header[++offset];
	offset += (sizeof(struct omap_aess_addr) * count) / 4;

	/* get label IDs */
	count = fw_header[offset];
	dev_dbg(aess->dev, "Labels %d at offset 0x%x\n", count, offset << 2);
	fw_info->label_id = &fw_header[++offset];
	offset += count;

	/* get function IDs */
	count = fw_header[offset];
	dev_dbg(aess->dev, "Functions %d at offset 0x%x\n", count,
		offset << 2);
	fw_info->fct_id = &fw_header[++offset];
	offset += count;

	/* get tasks */
	count = fw_header[offset];
	dev_dbg(aess->dev, "Tasks %d of size 0x%x at offset 0x%x\n", count,
		sizeof(struct omap_aess_task), offset << 2);
	fw_info->nb_init_task = count;
	fw_info->init_table = (struct omap_aess_task *)&fw_header[++offset];
	offset += (sizeof(struct omap_aess_task) * count) / 4;

	/* get ports */
	count = fw_header[offset];
	dev_dbg(aess->dev, "Ports %d of size 0x%x at offset 0x%x\n", count,
		sizeof(struct omap_aess_port), offset << 2);
	fw_info->port = (struct omap_aess_port *)&fw_header[++offset];
	offset += (sizeof(struct omap_aess_port) * count) / 4;

	/* get ping pong port */
	dev_dbg(aess->dev, "Ping pong port at offset 0x%x\n", offset << 2);
	fw_info->ping_pong = (struct omap_aess_port *)&fw_header[offset];
	offset += sizeof(struct omap_aess_port) / 4;

	/* get DL1 mono mixer */
	dev_dbg(aess->dev, "DL1 mono mixer at offset 0x%x\n", offset << 2);
	fw_info->dl1_mono_mixer = (struct omap_aess_task *)&fw_header[offset];
	offset += (sizeof(struct omap_aess_task) / 4) * 2;

	/* get DL2 mono mixer */
	dev_dbg(aess->dev, "DL2 mono mixer at offset 0x%x\n", offset << 2);
	fw_info->dl2_mono_mixer = (struct omap_aess_task *)&fw_header[offset];
	offset += (sizeof(struct omap_aess_task) / 4) * 2;

	/* get AUDUL mono mixer */
	dev_dbg(aess->dev, "AUDUL mixer at offset 0x%x\n", offset << 2);
	fw_info->audul_mono_mixer = (struct omap_aess_task *)&fw_header[offset];
	offset += (sizeof(struct omap_aess_task) / 4) * 2;

	/* ASRC */
	dev_dbg(aess->dev, "ASRC at offset 0x%x\n", offset << 2);
	fw_info->asrc = &fw_header[offset];
}

/**
 * omap_aess_load_fw_param - Load the AESS FW inside AESS memories
 * @aess: Pointer on aess handle
 * @data: Pointer on the AESS firmware (after the header)
 *
 * Load the different AESS memories PMEM/DMEM/SMEM/DMEM
 */
static void omap_aess_load_fw_param(struct omap_aess *aess)
{
	u32 pmem_size, dmem_size, smem_size, cmem_size;
	u32 *pmem_ptr, *dmem_ptr, *smem_ptr, *cmem_ptr;

	/* Take FW memories banks sizes */
	aess->firmware_version_number = aess->fw_hdr.version;
	pmem_size = aess->fw_hdr.pmem_size;
	cmem_size = aess->fw_hdr.cmem_size;
	dmem_size = aess->fw_hdr.dmem_size;
	smem_size = aess->fw_hdr.smem_size;
	pmem_ptr = (u32 *) aess->fw_data;
	cmem_ptr = pmem_ptr + (pmem_size >> 2);
	dmem_ptr = cmem_ptr + (cmem_size >> 2);
	smem_ptr = dmem_ptr + (dmem_size >> 2);

	omap_aess_write(aess, OMAP_AESS_BANK_PMEM, 0, pmem_ptr, pmem_size);
	omap_aess_write(aess, OMAP_AESS_BANK_CMEM, 0, cmem_ptr, cmem_size);
	omap_aess_write(aess, OMAP_AESS_BANK_SMEM, 0, smem_ptr, smem_size);
	omap_aess_write(aess, OMAP_AESS_BANK_DMEM, 0, dmem_ptr, dmem_size);
}

/**
 * omap_aess_build_scheduler_table
 * @aess: Pointer on aess handle
 *
 * Initialize Audio Engine scheduling table for AESS internal
 * processing. The content of the scheduling table is provided
 * by the firmware header. It can be changed according to the
 * AESS graph.
 */
static void omap_aess_build_scheduler_table(struct omap_aess *aess)
{
	struct omap_aess_task *task;
	u16 uplink_mux[NBROUTE_UL];
	int i, n;

	/* Initialize default scheduling table */
	memset(aess->MultiFrame, 0, sizeof(aess->MultiFrame));

	for (i = 0; i < aess->fw_info.nb_init_task; i++) {
		task = &aess->fw_info.init_table[i];
		aess->MultiFrame[task->frame][task->slot] = task->task;
	}

	omap_aess_write_map(aess, OMAP_AESS_DMEM_MULTIFRAME_ID,
			    aess->MultiFrame);

	/* reset the uplink router */
	n = aess->fw_info.map[OMAP_AESS_DMEM_AUPLINKROUTING_ID].bytes >> 1;
	if (unlikely(n > NBROUTE_UL)) {
		dev_err(aess->dev, "Too many uplink route (%d)\n", n);
		n = NBROUTE_UL;
	}
	for (i = 0; i < n; i++)
		uplink_mux[i] = omap_aess_get_label_data(aess,
						OMAP_AESS_BUFFER_ZERO_ID);

	omap_aess_write_map(aess, OMAP_AESS_DMEM_AUPLINKROUTING_ID, uplink_mux);
}

/**
 * omap_aess_load_fw - Load AESS Firmware and initialize memories
 * @aess: Pointer on aess handle
 * @firmware: Pointer on the AESS firmware (after the header)
 *
 */
void omap_aess_load_fw(struct omap_aess *aess)
{
	omap_aess_load_fw_param(aess);
	omap_aess_reset_all_ports(aess);
	omap_aess_init_gains(aess);
	omap_aess_init_gain_ramp(aess);
	omap_aess_build_scheduler_table(aess);
	omap_aess_select_main_port(aess, OMAP_AESS_PHY_PORT_PDM_DL);
}

/**
 * omap_aess_reload_fw - Reload AESS Firmware after OFF mode
 * @aess: Pointer on aess handle
 * @firmware: Pointer on the AESS firmware (after the header)
 */
void omap_aess_reload_fw(struct omap_aess *aess)
{
	omap_aess_load_fw_param(aess);
	omap_aess_init_gain_ramp(aess);
	omap_aess_build_scheduler_table(aess);
	/* IRQ circular read pointer in DMEM */
	aess->irq_dbg_read_ptr = 0;
	/* Restore Gains not managed by the drivers */
	omap_aess_write_gain(aess, OMAP_AESS_GAIN_SPLIT_LEFT, GAIN_0dB);
	omap_aess_write_gain(aess, OMAP_AESS_GAIN_SPLIT_RIGHT, GAIN_0dB);
}
