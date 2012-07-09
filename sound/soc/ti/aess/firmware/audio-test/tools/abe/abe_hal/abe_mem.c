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
#if 0	
/*---------------------------------------------*/
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
/*---------------------------------------------*/
#else
#if PC_SIMULATION
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif
#endif
/**
 * abe_block_copy
 * @direction: direction of the data move (Read/Write)
 * @memory_bamk:memory bank among PMEM, DMEM, CMEM, SMEM, ATC/IO
 * @address: address of the memory copy (byte addressing)
 * @data: pointer to the data to transfer
 * @nb_bytes: number of data to move
 *
 * Memory transfer to/from ABE to MPU
 */
void abe_block_copy(u32 direction, u32 memory_bank, u32 address,
		    u32 *data, u32 nb_bytes)
{
#if PC_SIMULATION
	extern void target_server_read_pmem(u32 address, u32 *data,
					    u32 nb_words_32bits);
	extern void target_server_write_pmem(u32 address, u32 *data,
					     u32 nb_words_32bits);
	extern void target_server_read_cmem(u32 address, u32 *data,
					    u32 nb_words_32bits);
	extern void target_server_write_cmem(u32 address, u32 *data,
					     u32 nb_words_32bits);
	extern void target_server_read_atc(u32 address, u32 *data,
					   u32 nb_words_32bits);
	extern void target_server_write_atc(u32 address, u32 *data,
					    u32 nb_words_32bits);
	extern void target_server_read_smem(u32 address_48bits, u32 *data,
					    u32 nb_words_48bits);
	extern void target_server_write_smem(u32 address_48bits, u32 *data,
					     u32 nb_words_48bits);
	extern void target_server_read_dmem(u32 address_byte, u32 *data,
					    u32 nb_byte);
	extern void target_server_write_dmem(u32 address_byte, u32 *data,
					     u32 nb_byte);
	u32 *smem_tmp, smem_offset, smem_base, nb_words48;
	u32 remaining, index;
	if (direction == COPY_FROM_HOST_TO_ABE) {
		switch (memory_bank) {
		case ABE_PMEM:
			target_server_write_pmem(address / 4, data,
						 nb_bytes / 4);
			break;
		case ABE_CMEM:
			target_server_write_cmem(address / 4, data,
						 nb_bytes / 4);
			break;
		case ABE_ATC:
			target_server_write_atc(address / 4, data,
						nb_bytes / 4);
			break;
		case ABE_SMEM:
			nb_words48 = (nb_bytes + 7) >> 3;
			/* temporary buffer manages the OCP access to
			   32bits boundaries */
			smem_tmp = (u32 *) malloc(nb_bytes + 64);
			/* address is on SMEM 48bits lines boundary */
			smem_base = address - (address & 7);
			target_server_read_smem(smem_base / 8, smem_tmp,
						2 + nb_words48);
			smem_offset = address & 7;
			memcpy(&(smem_tmp[smem_offset >> 2]), data, nb_bytes);
			target_server_write_smem(smem_base / 8, smem_tmp,
						 2 + nb_words48);
			free(smem_tmp);
			break;
		case ABE_DMEM:
			if (nb_bytes > 0x1000) {
				remaining = nb_bytes;
				index = 0;
				while (remaining > 0x1000) {
					target_server_write_dmem(address,
								 &(data[index]),
								 0x1000);
					address += 0x1000;
					index += (0x1000 >> 2);
					remaining -= 0x1000;
				}
				target_server_write_dmem(address,
							 &(data[index]),
							 remaining);
			} else
				target_server_write_dmem(address, data,
							 nb_bytes);
			break;
		default:
			abe_dbg_param |= ERR_LIB;
			abe_dbg_error_log(ABE_BLOCK_COPY_ERR);
			break;
		}
	} else {
		switch (memory_bank) {
		case ABE_PMEM:
			target_server_read_pmem(address / 4, data,
						nb_bytes / 4);
			break;
		case ABE_CMEM:
			target_server_read_cmem(address / 4, data,
						nb_bytes / 4);
			break;
		case ABE_ATC:
			target_server_read_atc(address / 4, data, nb_bytes / 4);
			break;
		case ABE_SMEM:
			nb_words48 = (nb_bytes + 7) >> 3;
			/* temporary buffer manages the OCP access to
			   32bits boundaries */
			smem_tmp = (u32 *) malloc(nb_bytes + 64);
			/* address is on SMEM 48bits lines boundary */
			smem_base = address - (address & 7);
			target_server_read_smem(smem_base / 8, smem_tmp,
						2 + nb_words48);
			smem_offset = address & 7;
			memcpy(data, &(smem_tmp[smem_offset >> 2]), nb_bytes);
			free(smem_tmp);
			break;
		case ABE_DMEM:
			target_server_read_dmem(address, data, nb_bytes);
			break;
		default:
			abe_dbg_param |= ERR_LIB;
			abe_dbg_error_log(ABE_BLOCK_COPY_ERR);
			break;
		}
	}
#else
	u32 i;
	u32 base_address = 0, *src_ptr, *dst_ptr, n;
	switch (memory_bank) {
	case ABE_PMEM:
		base_address = (u32) io_base + ABE_PMEM_BASE_OFFSET_MPU;
		break;
	case ABE_CMEM:
		base_address = (u32) io_base + ABE_CMEM_BASE_OFFSET_MPU;
		break;
	case ABE_SMEM:
		base_address = (u32) io_base + ABE_SMEM_BASE_OFFSET_MPU;
		break;
	case ABE_DMEM:
		base_address = (u32) io_base + ABE_DMEM_BASE_OFFSET_MPU;
		break;
	case ABE_ATC:
		base_address = (u32) io_base + ABE_ATC_BASE_OFFSET_MPU;
		break;
	default:
		base_address = (u32) io_base + ABE_SMEM_BASE_OFFSET_MPU;
		abe_dbg_param |= ERR_LIB;
		abe_dbg_error_log(ABE_BLOCK_COPY_ERR);
		break;
	}
	if (direction == COPY_FROM_HOST_TO_ABE) {
		dst_ptr = (u32 *) (base_address + address);
		src_ptr = (u32 *) data;
	} else {
		dst_ptr = (u32 *) data;
		src_ptr = (u32 *) (base_address + address);
	}
	n = (nb_bytes / 4);
	for (i = 0; i < n; i++)
		*dst_ptr++ = *src_ptr++;
#endif
}
/**
 * abe_reset_mem
 *
 * @memory_bank: memory bank among DMEM, SMEM
 * @address: address of the memory copy (byte addressing)
 * @nb_bytes: number of data to move
 *
 * Reset ABE memory
 */
void abe_reset_mem(u32 memory_bank, u32 address, u32 nb_bytes)
{
#if PC_SIMULATION
	extern void target_server_write_smem(u32 address_48bits,
					     u32 *data, u32 nb_words_48bits);
	extern void target_server_write_dmem(u32 address_byte,
					     u32 *data, u32 nb_byte);
	u32 *smem_tmp, *data, smem_offset, smem_base, nb_words48;
	data = (u32 *) calloc(nb_bytes, 1);
	switch (memory_bank) {
	case ABE_SMEM:
		nb_words48 = (nb_bytes + 7) >> 3;
		/* temporary buffer manages the OCP access to 32bits
		   boundaries */
		smem_tmp = (u32 *) malloc(nb_bytes + 64);
		/* address is on SMEM 48bits lines boundary */
		smem_base = address - (address & 7);
		target_server_read_smem(smem_base / 8, smem_tmp,
					2 + nb_words48);
		smem_offset = address & 7;
		memcpy(&(smem_tmp[smem_offset >> 2]), data, nb_bytes);
		target_server_write_smem(smem_base / 8, smem_tmp,
					 2 + nb_words48);
		free(smem_tmp);
		break;
	case ABE_CMEM:
		target_server_write_cmem(address / 4, data, nb_bytes / 4);
		break;
	case ABE_DMEM:
		target_server_write_dmem(address, data, nb_bytes);
		break;
	default:
		abe_dbg_param |= ERR_LIB;
		abe_dbg_error_log(ABE_BLOCK_COPY_ERR);
	}
	free(data);
#else
	u32 i;
	u32 *dst_ptr, n;
	u32 base_address = 0;
	switch (memory_bank) {
	case ABE_SMEM:
		base_address = (u32) io_base + ABE_SMEM_BASE_OFFSET_MPU;
		break;
	case ABE_DMEM:
		base_address = (u32) io_base + ABE_DMEM_BASE_OFFSET_MPU;
		break;
	case ABE_CMEM:
		base_address = (u32) io_base + ABE_CMEM_BASE_OFFSET_MPU;
		break;
	}
	dst_ptr = (u32 *) (base_address + address);
	n = (nb_bytes / 4);
	for (i = 0; i < n; i++)
		*dst_ptr++ = 0;
#endif
}
