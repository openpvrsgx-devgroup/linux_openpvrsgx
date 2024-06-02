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
/**
 * abe_fprintf
	 *
	 *  Parameter  :
	 *      character line to be printed
	 *
	 *  Operations :
	 *
	 *  Return value :
	 *      None.
	 */
void abe_fprintf(char *line);
/*
 *  ABE_READ_FEATURE_FROM_PORT
 *
 *  Parameter  :
 *      x : d
 *
 *  Operations :
 *
 *
 *  Return value :
 *
 */
void abe_read_feature_from_port(u32 x);
/*
 *  ABE_WRITE_FEATURE_TO_PORT
 *
 *  Parameter  :
 *      x : d
 *
 *  Operations :
 *
 *
 *  Return value :
 *
 */
void abe_write_feature_to_port(u32 x);
/*
 *  ABE_READ_FIFO
 *
 *  Parameter  :
 *      x : d
 *
 *  Operations :
 *
 *
 *  Return value :
 *
 */
void abe_read_fifo(u32 x);
/*
 *  ABE_WRITE_FIFO
 *
 *  Parameter  :
 *      mem_bank : currently only ABE_DMEM supported
 *	addr : FIFO descriptor address ( descriptor fields : READ ptr,
 *	WRITE ptr, FIFO START_ADDR, FIFO END_ADDR)
 *	data to write to FIFO
 *	number of 32-bit words to write to DMEM FIFO
 *
 *  Operations :
 *	write DMEM FIFO and update FIFO descriptor, it is assumed that FIFO
 *	descriptor is located in DMEM
 *
 *  Return value :
 *      none
 */
void abe_write_fifo(u32 mem_bank, u32 addr, u32 *data, u32 nb_data32);
/*
 *  ABE_BLOCK_COPY
 *
 *  Parameter  :
 *      direction of the data move (Read/Write)
 *      memory bank among PMEM, DMEM, CMEM, SMEM, ATC/IO
 *      address of the memory copy (byte addressing)
 *      long pointer to the data
 *      number of data to move
 *
 *  Operations :
 *      block data move
 *
 *  Return value :
 *      none
 */
void abe_block_copy(u32 direction, u32 memory_bank, u32 address, u32 *data,
		    u32 nb);
/*
 *  ABE_RESET_MEM
 *
 *  Parameter  :
 *      memory bank among DMEM, SMEM
 *      address of the memory copy (byte addressing)
 *      number of data to move
 *
 *  Operations :
 *      reset memory
 *
 *  Return value :
 *      none
 */
void abe_reset_mem(u32 memory_bank, u32 address, u32 nb_bytes);