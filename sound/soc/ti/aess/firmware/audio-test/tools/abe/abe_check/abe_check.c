/*

  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2010-2011 Texas Instruments Incorporated,
  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution
  in the file called LICENSE.GPL.

  BSD LICENSE

  Copyright(c) 2010-2011 Texas Instruments Incorporated,
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Texas Instruments Incorporated nor the names of
      its contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

#include "abe_cm_addr.h"
#include "abe_dm_addr.h"
#include "abe_sm_addr.h"
#include "abe_functionsid.h"


#define NAME_SIZE	30
#define TEXT_SIZE	20
#define NUM_TEXTS	10

#define MAX_PROFILES 	8  /* Number of supported profiles */
#define MAX_COEFFS 	25      /* Number of coefficients for profiles */


#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(x[0]))

#define OMAP_ABE_DMEM  0
#define OMAP_ABE_SMEM  1
#define OMAP_ABE_CMEM  2

#if 1
uint32_t dmem[] = {
#include "dmem.txt"
};

uint32_t smem[] = {
#include "smem.txt"
};

uint32_t cmem[] = {
#include "cmem.txt"
};
#else
uint32_t dmem[] = {
#include "dump_dmem.txt"
};

uint32_t smem[] = {
#include "dump_smem.txt"
};

uint32_t cmem[] = {
#include "dump_cmem.txt"
};
#endif


#define NAME_SIZE 30

struct buffer {
	char name[NAME_SIZE];
	uint32_t addr;
	int size;
	int bank;
};

struct buffer vx_dl_16k_list[] = {
	{
		.name = "VX_DL FIFO",
		.addr = OMAP_ABE_D_VX_DL_FIFO_ADDR,
		.size = OMAP_ABE_D_VX_DL_FIFO_SIZE,
		.bank = OMAP_ABE_DMEM,
	},
	{
		.name = "Voice DL 16 kHz",
		.addr = OMAP_ABE_S_VOICE_16K_DL_ADDR,
		.size = OMAP_ABE_S_VOICE_16K_DL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_DL 16 -> 48 LP",
		.addr = OMAP_ABE_S_VX_DL_16_48_LP_DATA_ADDR,
		.size = OMAP_ABE_S_VX_DL_16_48_LP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_DL 16 -> 48 HP",
		.addr = OMAP_ABE_S_VX_DL_16_48_HP_DATA_ADDR,
		.size = OMAP_ABE_S_VX_DL_16_48_HP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_DL",
		.addr = OMAP_ABE_S_VX_DL_ADDR,
		.size = OMAP_ABE_S_VX_DL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
};

struct buffer vx_dl_8k_list[] = {
	{
		.name = "VX_DL FIFO",
		.addr = OMAP_ABE_D_VX_DL_FIFO_ADDR,
		.size = OMAP_ABE_D_VX_DL_FIFO_SIZE,
		.bank = OMAP_ABE_DMEM,
	},
	{
		.name = "Voice DL 8 kHz",
		.addr = OMAP_ABE_S_VOICE_8K_DL_ADDR,
		.size = OMAP_ABE_S_VOICE_8K_DL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "ASRC VX DL",
		.addr = OMAP_ABE_S_XINASRC_DL_VX_ADDR,
		.size = OMAP_ABE_S_XINASRC_DL_VX_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_DL 8 -> 48 FIR LP",
		.addr = OMAP_ABE_S_VX_DL_8_48_OSR_LP_DATA_ADDR,
		.size = OMAP_ABE_S_VX_DL_8_48_OSR_LP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_DL 8 -> 48 HP",
		.addr = OMAP_ABE_S_VX_DL_8_48_HP_DATA_ADDR,
		.size = OMAP_ABE_S_VX_DL_8_48_HP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_DL",
		.addr = OMAP_ABE_S_VX_DL_ADDR,
		.size = OMAP_ABE_S_VX_DL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
};

struct buffer bt_dl_8k_list[] = {
	{
		.name = "BT_DL",
		.addr = OMAP_ABE_S_BT_DL_ADDR,
		.size = OMAP_ABE_S_BT_DL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
#if 0
	{
		.name = "BT_DL 8 -> 48 LP",
		.addr = OMAP_ABE_S_BT_DL_8_48_OSR_LP_DATA_ADDR,/*OMAP_ABE_S_BT_DL_48_8_LP_DATA_ADDR,*/
		.size = OMAP_ABE_S_BT_DL_8_48_OSR_LP_DATA_SIZE, /*OMAP_ABE_S_BT_DL_48_8_LP_DATA_SIZE,*/
		.bank = OMAP_ABE_SMEM,
	},
#else
	{
		.name = "BT_DL 8 -> 48 LP",
		.addr = OMAP_ABE_S_BT_DL_48_8_LP_DATA_ADDR,
		.size = OMAP_ABE_S_BT_DL_48_8_LP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
#endif
	{
		.name = "BT_DL 8 -> 48 HP",
		.addr = OMAP_ABE_S_BT_DL_48_8_HP_DATA_ADDR,
		.size = OMAP_ABE_S_BT_DL_48_8_HP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_DL 8 kHz",
		.addr = OMAP_ABE_S_BT_DL_8K_ADDR,
		.size = OMAP_ABE_S_BT_DL_8K_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_DL FIFO",
		.addr = OMAP_ABE_D_BT_DL_FIFO_ADDR,
		.size = OMAP_ABE_D_BT_DL_FIFO_SIZE,
		.bank = OMAP_ABE_DMEM,
	},
};

struct buffer bt_dl_16k_list[] = {
	{
		.name = "BT_DL",
		.addr = OMAP_ABE_S_BT_DL_ADDR,
		.size = OMAP_ABE_S_BT_DL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_DL 16 -> 48 LP",
		.addr = OMAP_ABE_S_BT_DL_48_16_LP_DATA_ADDR,
		.size = OMAP_ABE_S_BT_DL_48_16_LP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_DL 16 -> 48 HP",
		.addr = OMAP_ABE_S_BT_DL_48_16_HP_DATA_ADDR,
		.size = OMAP_ABE_S_BT_DL_48_16_HP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_DL 16 kHz",
		.addr = OMAP_ABE_S_BT_DL_16K_ADDR,
		.size = OMAP_ABE_S_BT_DL_16K_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_DL FIFO",
		.addr = OMAP_ABE_D_BT_DL_FIFO_ADDR,
		.size = OMAP_ABE_D_BT_DL_FIFO_SIZE,
		.bank = OMAP_ABE_DMEM,
	},
};

struct buffer mm_dl_list[] = {
	{
		.name = "MM DL",
		.addr = OMAP_ABE_S_MM_DL_ADDR,
		.size = OMAP_ABE_S_MM_DL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
};

struct buffer earp_list[] = {
	{
		.name = "DL1 M Out",
		.addr = OMAP_ABE_S_DL1_M_OUT_ADDR,
		.size = OMAP_ABE_S_DL1_M_OUT_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "DL1 EQ",
		.addr = OMAP_ABE_S_DL1_M_EQ_DATA_ADDR,
		.size = OMAP_ABE_S_DL1_M_EQ_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "EARP LP 48 -> 96",
		.addr = OMAP_ABE_S_EARP_48_96_LP_DATA_ADDR,
		.size = OMAP_ABE_S_EARP_48_96_LP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "McPDM Out1",
		.addr = OMAP_ABE_S_MCPDM_OUT1_ADDR,
		.size = OMAP_ABE_S_MCPDM_OUT1_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
};

struct buffer ihf_list[] = {
	{
		.name = "DL2 M Out",
		.addr = OMAP_ABE_S_DL2_M_OUT_ADDR,
		.size = OMAP_ABE_S_DL2_M_OUT_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "DL2 EQ",
		.addr = OMAP_ABE_S_DL2_M_LR_EQ_DATA_ADDR,
		.size = OMAP_ABE_S_DL2_M_LR_EQ_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "IHF LP 48 -> 96",
		.addr = OMAP_ABE_S_IHF_48_96_LP_DATA_ADDR,
		.size = OMAP_ABE_S_IHF_48_96_LP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "McPDM Out2",
		.addr = OMAP_ABE_S_MCPDM_OUT1_ADDR,
		.size = OMAP_ABE_S_MCPDM_OUT1_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
};

struct buffer amic_ul_list[] = {
	{
		.name = "McPDM_UL FIFO",
		.addr = OMAP_ABE_D_MCPDM_UL_FIFO_ADDR,
		.size = OMAP_ABE_D_MCPDM_UL_FIFO_SIZE,
		.bank = OMAP_ABE_DMEM,
	},
	{
		.name = "AMIC_96k",
		.addr = OMAP_ABE_S_AMIC_96K_ADDR,
		.size = OMAP_ABE_S_AMIC_96K_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "AMIC_96_48 EQ",
		.addr = OMAP_ABE_S_AMIC_96_48_DATA_ADDR,
		.size = OMAP_ABE_S_AMIC_96_48_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "AMIC",
		.addr = OMAP_ABE_S_AMIC_ADDR,
		.size = OMAP_ABE_S_AMIC_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
};

struct buffer bt_ul_8k_list[] = {
	{
		.name = "BT_UL FIFO",
		.addr = OMAP_ABE_D_BT_UL_FIFO_ADDR,
		.size = OMAP_ABE_D_BT_UL_FIFO_SIZE,
		.bank = OMAP_ABE_DMEM,
	},
	{
		.name = "BT_UL 8 kHz",
		.addr = OMAP_ABE_S_BT_UL_8K_ADDR,
		.size = OMAP_ABE_S_BT_UL_8K_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_UL 8 -> 48 LP",
		.addr = OMAP_ABE_S_BT_UL_8_48_LP_DATA_ADDR,
		.size = OMAP_ABE_S_BT_UL_8_48_LP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_UL 8 -> 48 HP",
		.addr = OMAP_ABE_S_BT_UL_8_48_HP_DATA_ADDR,
		.size = OMAP_ABE_S_BT_UL_8_48_HP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_UL",
		.addr = OMAP_ABE_S_BT_UL_ADDR,
		.size = OMAP_ABE_S_BT_UL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
};

struct buffer bt_ul_16k_list[] = {
	{
		.name = "BT_UL FIFO",
		.addr = OMAP_ABE_D_BT_UL_FIFO_ADDR,
		.size = OMAP_ABE_D_BT_UL_FIFO_SIZE,
		.bank = OMAP_ABE_DMEM,
	},
	{
		.name = "BT_UL 16 kHz",
		.addr = OMAP_ABE_S_BT_UL_16K_ADDR,
		.size = OMAP_ABE_S_BT_UL_16K_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_UL 16 -> 48 LP",
		.addr = OMAP_ABE_S_BT_UL_16_48_LP_DATA_ADDR,
		.size = OMAP_ABE_S_BT_UL_16_48_LP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_UL 16 -> 48 HP",
		.addr = OMAP_ABE_S_BT_UL_16_48_HP_DATA_ADDR,
		.size = OMAP_ABE_S_BT_UL_16_48_HP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "BT_UL",
		.addr = OMAP_ABE_S_BT_UL_ADDR,
		.size = OMAP_ABE_S_BT_UL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
};

struct buffer vx_ul_16k_list[] = {
	{
		.name = "VX_UL FIFO",
		.addr = OMAP_ABE_D_VX_UL_FIFO_ADDR,
		.size = OMAP_ABE_D_VX_UL_FIFO_SIZE,
		.bank = OMAP_ABE_DMEM,
	},
	{
		.name = "Voice UL 16 kHz",
		.addr = OMAP_ABE_S_VOICE_16K_UL_ADDR,
		.size = OMAP_ABE_S_VOICE_16K_UL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_UL 16 -> 48 LP",
		.addr = OMAP_ABE_S_VX_UL_48_16_LP_DATA_ADDR,
		.size = OMAP_ABE_S_VX_UL_48_16_LP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_UL 16 -> 48 HP",
		.addr = OMAP_ABE_S_VX_UL_48_16_HP_DATA_ADDR,
		.size = OMAP_ABE_S_VX_UL_48_16_HP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_UL",
		.addr = OMAP_ABE_S_VX_UL_ADDR,
		.size = OMAP_ABE_S_VX_UL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
};

struct buffer vx_ul_8k_list[] = {
	{
		.name = "VX_UL",
		.addr = OMAP_ABE_S_VX_UL_ADDR,
		.size = OMAP_ABE_S_VX_UL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_UL 8 -> 48 LP",
		.addr = OMAP_ABE_S_VX_UL_48_8_LP_DATA_ADDR,
		.size = OMAP_ABE_S_VX_UL_48_8_LP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_UL 8 -> 48 HP",
		.addr = OMAP_ABE_S_VX_UL_48_8_HP_DATA_ADDR,
		.size = OMAP_ABE_S_VX_UL_48_8_HP_DATA_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "Voice UL 8 kHz",
		.addr = OMAP_ABE_S_VOICE_8K_UL_ADDR,
		.size = OMAP_ABE_S_VOICE_8K_UL_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "ASRC VX UL",
		.addr = OMAP_ABE_S_XINASRC_UL_VX_ADDR,
		.size = OMAP_ABE_S_XINASRC_UL_VX_SIZE,
		.bank = OMAP_ABE_SMEM,
	},
	{
		.name = "VX_UL FIFO",
		.addr = OMAP_ABE_D_VX_UL_FIFO_ADDR,
		.size = OMAP_ABE_D_VX_UL_FIFO_SIZE,
		.bank = OMAP_ABE_DMEM,
	},
};

#define NB_GAIN 38

char *gain_list[NB_GAIN] = {
	"DMIC1",
	"DMIC1",
	"DMIC2",
	"DMIC2",
	"DMIC3",
	"DMIC3",
	"AMIC",
	"AMIC",
	"DL1 Left",
	"DL1 Right",
	"DL2 Left",
	"DL2 Right",
	"SPLIT",
	"SPLIT",
	"Mix DL1 Multimedia",
	"Mix DL1 Uplink",
	"Mix DL1 Voice DL",
	"Mix DL1 Tones",
	"Mix DL2 Multimedia",
	"Mix DL2 Uplink",
	"Mix DL2 Voice DL",
	"Mix DL2 Tones",
	"Echo DL2",
	"Echo DL1",
	"SDT Uplink",
	"SDT Downlink",
	"VxRec Multimedia",
	"VxRec Tones",
	"VxRec Voice UL",
	"VxRec Voice DL",
	"UL Mix Multimdeia",
	"UL Mix Tones",
	"UL Mix Uplink",
	"UL Mix Voice",
	"UL Left",
	"UL Right",
	"BT UL Left",
	"BT UL Right"
};


struct atc_desc {
	unsigned rdpt:7;
	unsigned reserved0:1;
	unsigned cbsize:7;
	unsigned irqdest:1;
	unsigned cberr:1;
	unsigned reserved1:5;
	unsigned cbdir:1;
	unsigned nw:1;
	unsigned wrpt:7;
	unsigned reserved2:1;
	unsigned badd:12;
	unsigned iter:7;
	unsigned srcid:6;
	unsigned destid:6;
	unsigned desen:1;
};

struct io_desc {
	unsigned short drift_asrc;
	unsigned short drift_io;
	unsigned char io_type_idx;
	unsigned char samp_size;
	short flow_counter;
	unsigned short hw_ctrl_addr;
	unsigned char atc_irq_data;
	unsigned char direction_rw;
	unsigned char repeat_last_samp;
	unsigned char nsamp;
	unsigned char x_io;
	unsigned char on_off;
	unsigned short split_addr1;
	unsigned short split_addr2;
	unsigned short split_addr3;
	unsigned char before_f_index;
	unsigned char after_f_index;
	unsigned short smem_addr1;
	unsigned short atc_address1;
	unsigned short atc_pointer_saved1;
	unsigned char data_size1;
	unsigned char copy_f_index1;
	unsigned short smem_addr2;
	unsigned short atc_address2;
	unsigned short atc_pointer_saved2;
	unsigned char data_size2;
	unsigned char copy_f_index2;
};

struct ping_pong_desc {
	unsigned short drift_asrc;
	unsigned short drift_io;
	unsigned short hw_ctrl_addr;
	unsigned char copy_func_index;
	unsigned char x_io;
	unsigned char data_size;
	unsigned char smem_addr;
	unsigned char atc_irq_data;
	unsigned char counter;
	unsigned short workbuff_baseaddr;
	unsigned short workbuff_samples;
	unsigned short nextbuff0_baseaddr;
	unsigned short nextbuff0_samples;
	unsigned short nextbuff1_baseaddr;
	unsigned short nextbuff1_samples;
};

#define DMIC_PORT		0
#define PDM_UL_PORT		1
#define BT_VX_UL_PORT		2
#define MM_UL_PORT		3
#define MM_UL2_PORT		4
#define VX_UL_PORT		5
#define MM_DL_PORT		6
#define VX_DL_PORT		7
#define TONES_DL_PORT		8
#define VIB_DL_PORT		9
#define BT_VX_DL_PORT		10
#define PDM_DL_PORT		11
#define MM_EXT_OUT_PORT		12
#define MM_EXT_IN_PORT		13
#define TDM_DL_PORT		14
#define TDM_UL_PORT		15
#define DEBUG_PORT		16
#define PING_PONG_PORT		17
#define NUM_SUPPORTED_PORTS	18
#define NO_PORT			-1

#define External_DMA_0		0
#define DMIC_DMA_REQ		1
#define McPDM_DMA_DL		2
#define McPDM_DMA_UP		3
#define MCBSP1_DMA_TX		4
#define MCBSP1_DMA_RX		5
#define MCBSP2_DMA_TX		6
#define MCBSP2_DMA_RX		7
#define MCBSP3_DMA_TX		8
#define MCBSP3_DMA_RX		9
#define SLIMBUS1_DMA_TX0	10
#define SLIMBUS1_DMA_TX1	11
#define SLIMBUS1_DMA_TX2	12
#define SLIMBUS1_DMA_TX3	13
#define SLIMBUS1_DMA_TX4	14
#define SLIMBUS1_DMA_TX5	15
#define SLIMBUS1_DMA_TX6	16
#define SLIMBUS1_DMA_TX7	17
#define SLIMBUS1_DMA_RX0	18
#define SLIMBUS1_DMA_RX1	19
#define SLIMBUS1_DMA_RX2	20
#define SLIMBUS1_DMA_RX3	21
#define SLIMBUS1_DMA_RX4	21
#define SLIMBUS1_DMA_RX5	23
#define SLIMBUS1_DMA_RX6	24
#define SLIMBUS1_DMA_RX7	25
#define McASP1_AXEVT		26
#define McASP1_AREVT		29
#define DUMMY_FIFO		30
#define CBPr_DMA_RTX0		32
#define CBPr_DMA_RTX1		33
#define CBPr_DMA_RTX2		34
#define CBPr_DMA_RTX3		35
#define CBPr_DMA_RTX4		36
#define CBPr_DMA_RTX5		37
#define CBPr_DMA_RTX6		38
#define CBPr_DMA_RTX7		39
#define NUM_SUPPORTED_DMA_REQS	40


#define DMIC_PORT		0
#define PDM_UL_PORT		1
#define BT_VX_UL_PORT		2
#define MM_UL_PORT		3
#define MM_UL2_PORT		4
#define VX_UL_PORT		5
#define MM_DL_PORT		6
#define VX_DL_PORT		7
#define TONES_DL_PORT		8
#define VIB_DL_PORT		9
#define BT_VX_DL_PORT		10
#define PDM_DL_PORT		11
#define MM_EXT_OUT_PORT		12
#define MM_EXT_IN_PORT		13
#define TDM_DL_PORT		14
#define TDM_UL_PORT		15
#define DEBUG_PORT		16
#define PING_PONG_PORT		17
#define NUM_SUPPORTED_PORTS	18
#define NO_PORT			-1


#define OMAP_ABE_ATC_TO_PORT_SIZE    14

int atc_to_io[OMAP_ABE_ATC_TO_PORT_SIZE][2] = {
	{DMIC_DMA_REQ,  DMIC_PORT},
	{McPDM_DMA_DL,  PDM_UL_PORT},
	{McPDM_DMA_UP,  PDM_UL_PORT},
	{MCBSP1_DMA_TX, BT_VX_DL_PORT},
	{MCBSP1_DMA_RX, BT_VX_UL_PORT},
	{MCBSP2_DMA_TX, VX_UL_PORT},
	{MCBSP2_DMA_RX, VX_DL_PORT},
	{CBPr_DMA_RTX0, MM_DL_PORT},
	{CBPr_DMA_RTX1, VX_DL_PORT},
	{CBPr_DMA_RTX2, VX_UL_PORT},
	{CBPr_DMA_RTX3, MM_UL_PORT},
	{CBPr_DMA_RTX4, MM_UL2_PORT},
	{CBPr_DMA_RTX5, TONES_DL_PORT},
	{CBPr_DMA_RTX6, VIB_DL_PORT},
};

#define OMAP_ABE_ID_TO_FCT_SIZE    20

char *id_to_fct[OMAP_ABE_ID_TO_FCT_SIZE] = {
	"NULL",
	"S2D_STEREO_16_16_CFPID",
	"S2D_MONO_MSB_CFPID",
	"S2D_STEREO_MSB_CFPID",
	"S2D_STEREO_RSHIFTED_16_CFPID",
	"S2D_MONO_RSHIFTED_16_CFPID",
	"D2S_STEREO_16_16_CFPID",
	"D2S_MONO_MSB_CFPID",
	"D2S_MONO_RSHIFTED_16_CFPID",
	"D2S_STEREO_RSHIFTED_16_CFPID",
	"D2S_STEREO_MSB_CFPID",
	"COPY_DMIC_CFPID",
	"COPY_MCPDM_DL_CFPID",
	"COPY_MM_UL_CFPID",
	"SPLIT_SMEM_CFPID",
	"MERGE_SMEM_CFPID",
	"SPLIT_TDM_CFPID",
	"MERGE_TDM_CFPID",
	"ROUTE_MM_UL_CFPID",
	"IO_IP_CFPID",
	"COPY_UNDERFLOW_CFPID",
};

char *supported_ports[NUM_SUPPORTED_PORTS] = {
	"DMIC_PORT",
	"PDM_UL_PORT",
	"BT_VX_UL_PORT",
	"MM_UL_PORT",
	"MM_UL2_PORT",
	"VX_UL_PORT",
	"MM_DL_PORT",
	"VX_DL_PORT",
	"TONES_DL_PORT",
	"VIB_DL_PORT",
	"BT_VX_DL_PORT",
	"PDM_DL_PORT",
	"MM_EXT_OUT_PORT",
	"MM_EXT_IN_PORT",
	"TDM_DL_PORT",
	"TDM_UL_PORT",
	"DEBUG_PORT",
	"PING_PONG_PORT",
};

char *supported_dma_reqs[NUM_SUPPORTED_DMA_REQS] = {
	"External_DMA_0",
	"DMIC_DMA_REQ",
	"McPDM_DMA_DL",
	"McPDM_DMA_UP",
	"MCBSP1_DMA_TX",
	"MCBSP1_DMA_RX",
	"MCBSP2_DMA_TX",
	"MCBSP2_DMA_RX",
	"MCBSP3_DMA_TX",
	"MCBSP3_DMA_RX",
	"SLIMBUS1_DMA_TX0",
	"SLIMBUS1_DMA_TX1",
	"SLIMBUS1_DMA_TX2",
	"SLIMBUS1_DMA_TX3",
	"SLIMBUS1_DMA_TX4",
	"SLIMBUS1_DMA_TX5",
	"SLIMBUS1_DMA_TX6",
	"SLIMBUS1_DMA_TX7",
	"SLIMBUS1_DMA_RX0",
	"SLIMBUS1_DMA_RX1",
	"SLIMBUS1_DMA_RX2",
	"SLIMBUS1_DMA_RX3",
	"SLIMBUS1_DMA_RX4",
	"SLIMBUS1_DMA_RX5",
	"SLIMBUS1_DMA_RX6",
	"SLIMBUS1_DMA_RX7",
	"McASP1_AXEVT",
	NULL,
	NULL,
	"McASP1_AREVT",
	"DUMMY_FIFO",
	NULL,
	"CBPr_DMA_RTX0",
	"CBPr_DMA_RTX1",
	"CBPr_DMA_RTX2",
	"CBPr_DMA_RTX3",
	"CBPr_DMA_RTX4",
	"CBPr_DMA_RTX5",
	"CBPr_DMA_RTX6",
	"CBPr_DMA_RTX7",
};

char *get_reg_name(short offset)
{
	switch (offset) {
	case 0x0000:
		return "AESS_REVISION";
	case 0x0010:
		return "AESS_SYSCONFIG";
	case 0x0024:
		return "AESS_MCU_IRQ_STATUS_RAW";
	case 0x0028:
		return "AESS_MCU_IRQ_STATUS";
	case 0x003C:
		return "AESS_MCU_IRQ_ENABLE_SET";
	case 0x0040:
		return "AESS_MCU_IRQ_ENABLE_CLR";
	case 0x004C:
		return "AESS_DSP_IRQ_STATUS_RAW";
	case 0x0050:
		return "AESS_DSP_IRQ_STATUS";
	case 0x0054:
		return "AESS_DSP_IRQ_ENABLE_SET";
	case 0x0058:
		return "AESS_DSP_IRQ_ENABLE_CLR";
	case 0x0060:
		return "AESS_DMAENABLE_SET";
	case 0x0064:
		return "AESS_DMAENABLE_CLR";
	case 0x0068:
		return "AESS_EVENT_GENERATOR_COUNTER";
	case 0x006C:
		return "AESS_EVENT_GENERATOR_START";
	case 0x0070:
		return "AESS_EVENT_GENERATOR_SELECTION";
	case 0x0074:
		return "AESS_EVENT_AUDIO_ENGINE_SCHEDULER";
	case 0x007C:
		return "AESS_AUTO_GATING_ENABLE";
	case 0x0084:
		return "AESS_DMASTATUS_RAW";
	case 0x0088:
		return "AESS_DMASTATUS";
	case 0x0100:
		return "AESS_CIRCULAR_BUFFER_PERIPHERAL_R_0";
	case 0x0104:
		return "AESS_CIRCULAR_BUFFER_PERIPHERAL_R_1";
	case 0x0108:
		return "AESS_CIRCULAR_BUFFER_PERIPHERAL_R_2";
	case 0x010C:
		return "AESS_CIRCULAR_BUFFER_PERIPHERAL_R_3";
	case 0x0110:
		return "AESS_CIRCULAR_BUFFER_PERIPHERAL_R_4";
	case 0x0114:
		return "AESS_CIRCULAR_BUFFER_PERIPHERAL_R_5";
	case 0x0118:
		return "AESS_CIRCULAR_BUFFER_PERIPHERAL_R_6";
	case 0x011C:
		return "AESS_CIRCULAR_BUFFER_PERIPHERAL_R_7";
	}

	return "UNKNOWN";
}

char *get_port_name(int port)
{
	if (port >= NUM_SUPPORTED_PORTS)
		return "INVALID";

	return supported_ports[port];
}

int get_dma_req(int port)
{
	switch (port) {
	case DMIC_PORT:
		return 1;
	case PDM_UL_PORT:
		return 3;
	case BT_VX_UL_PORT:
		return 0;
	case MM_UL_PORT:
		return 35;
	case MM_UL2_PORT:
		return 36;
	case VX_UL_PORT:
		return 34;
	case MM_DL_PORT:
		return 32;
	case VX_DL_PORT:
		return 33;
	case TONES_DL_PORT:
		return 37;
	case VIB_DL_PORT:
		return 38;
	case BT_VX_DL_PORT:
		return 0;
	case PDM_DL_PORT:
		return 2;
	case MM_EXT_OUT_PORT:
		return 0;
	case MM_EXT_IN_PORT:
		return 0;
	}

	return 0;
}

char *get_format_name(int format)
{
	switch (format) {
	case 1:
		return "MONO_MSB";
	case 2:
		return "MONO_RSHIFTED_16";
	case 3:
		return "STEREO_RSHIFTED_16";
	case 4:
		return "STEREO_16_16";
	case 5:
		return "STEREO_MSB";
	case 6:
		return "THREE_MSB";
	case 7:
		return "FOUR_MSB";
	case 8:
		return "FIVE_MSB";
	case 9:
		return "SIX_MSB";
	case 10:
		return "SEVEN_MSB";
	case 11:
		return "EIGHT_MSB";
	case 12:
		return "NINE_MSB";
	case 13:
		return "TEN_MSB";
	}

	return "INVALID";
}

char *get_atc_source_name(int source)
{
	switch (source) {
	case 0x00:
		return "DMEM access";
	case 0x01:
		return "MCBSP1_RX";
	case 0x02:
		return "MCBSP2_RX";
	case 0x03:
		return "MCBSP3_RX";
	case 0x04:
		return "SLIMBUS1_RX0";
	case 0x05:
		return "SLIMBUS1_RX1";
	case 0x06:
		return "SLIMBUS1_RX2";
	case 0x07:
		return "SLIMBUS1_RX3";
	case 0x08:
		return "SLIMBUS1_RX4";
	case 0x09:
		return "SLIMBUS1_RX5";
	case 0x0A:
		return "SLIMBUS1_RX6";
	case 0x0B:
		return "SLIMBUS1_RX7";
	case 0x0C:
		return "DMIC_UP";
	case 0x0D:
		return "MCPDM_DL";
	case 0x0E:
	case 0x0F:
	case 0x10:
	case 0x11:
		return "RESERVED";
	}

	return "INVALID";
}

/* Reverse of 'abe_format_switch' for 'multfac' */
char *get_format_name_from_iterfactor(int factor)
{
	switch (factor) {
	case 1:
		return "MONO_MSB, MONO_RSHIFTED_16 or STEREO_16_16";
	case 2:
		return "STEREO_MSB or STEREO_RSHIFTED_16";
	case 3:
		return "THREE_MSB";
	case 4:
		return "FOUR_MSB";
	case 5:
		return "FIVE_MSB";
	case 6:
		return "SIX_MSB";
	case 7:
		return "SEVEN_MSB";
	case 8:
		return "EIGHT_MSB";
	case 9:
		return "NINE_MSB";
	}

	return "INVALID";
}

char *get_atc_dest_name(int dest)
{
	switch (dest) {
	case 0x00:
		return "DMEM access";
	case 0x01:
		return "MCBSP1_TX";
	case 0x02:
		return "MCBSP2_TX";
	case 0x03:
		return "MCBSP3_TX";
	case 0x04:
		return "SLIMBUS1_TX0";
	case 0x05:
		return "SLIMBUS1_TX1";
	case 0x06:
		return "SLIMBUS1_TX2";
	case 0x07:
		return "SLIMBUS1_TX3";
	case 0x08:
		return "SLIMBUS1_TX4";
	case 0x09:
		return "SLIMBUS1_TX5";
	case 0x0A:
		return "SLIMBUS1_TX6";
	case 0x0B:
		return "SLIMBUS1_TX7";
	case 0x0C:
		return "MCPDM_DL";
	case 0x0D:
	case 0x0E:
	case 0x0F:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
		return "RESERVED";
	case 0x15:
		return "MCPDM_ON";
	}

	return "INVALID";
}

char *get_dma_req_name(int dma_req)
{
	if (dma_req >= NUM_SUPPORTED_DMA_REQS)
		return "INVALID";

	if (!supported_dma_reqs[dma_req])
		return "UNKNOWN";

	return supported_dma_reqs[dma_req];
}
void print_atc_desc(struct atc_desc *atc_desc, int dma_req)
{
	printf("*****************************************\n");
	printf(" ATC Descriptor %s\n", get_dma_req_name(dma_req));
	printf("*****************************************\n");
	printf("RDPT     : 0x%02x\n", atc_desc->rdpt);
	printf("RESERVED0: 0x%1x\n", atc_desc->reserved0);
	printf("CBSIZE   : 0x%02x\n", atc_desc->cbsize);
	printf("IRQDEST  : 0x%1x\n", atc_desc->irqdest);
	printf("CBERR    : 0x%1x\n", atc_desc->cberr);
	printf("RESERVED1: 0x%02x\n", atc_desc->reserved1);
	printf("CBDIR    : 0x%1x\n", atc_desc->cbdir);
	printf("NW       : 0x%1x\n", atc_desc->nw);
	printf("WRPT     : 0x%02x\n", atc_desc->wrpt);
	printf("RESERVED2: 0x%1x\n", atc_desc->reserved2);
	printf("BADD     : 0x%03x\n", atc_desc->badd);
	printf("ITER     : 0x%02x\n", atc_desc->iter);
	printf("SRCID    : 0x%02x\n", atc_desc->srcid);
	printf("DESTID   : 0x%02x\n", atc_desc->destid);
	printf("DESEN    : 0x%1x\n", atc_desc->desen);
}

void interpret_atc_desc(struct atc_desc *atc_desc, int dma_req)
{
	printf("*****************************************\n");
	printf(" Decoded ATC Descriptor %s\n", get_dma_req_name(dma_req));
	printf("*****************************************\n");
	printf("Read pointer relative address : 0x%02x\n", atc_desc->rdpt);
	printf("Circular buffer size          : 0x%02x\n", atc_desc->cbsize);
	printf("IRQ destination               : %s\n", atc_desc->irqdest ? "MCU" : "DSP");
	printf("Report circular buffer errors : %d\n", atc_desc->cberr);
	printf("Circular buffer direction     : %s (wrt AESS)\n", atc_desc->cbdir ? "Out" : "In");
	printf("Write pointer relative address: 0x%02x\n", atc_desc->wrpt);
	printf("Base address                  : 0x%02x\n", atc_desc->badd);
	printf("Iteration                     : 0x%02x (%d)\n", atc_desc->iter, atc_desc->iter);
	printf("Source ID peripheral          : %s\n", get_atc_source_name(atc_desc->srcid));
	printf("Destination ID peripheral     : %s\n", get_atc_dest_name(atc_desc->destid));
	printf("Descriptor activation         : %s\n", atc_desc->desen ? "Active" : "Not Active");
}

void print_ping_pong_desc(struct ping_pong_desc *pp_desc)
{
	printf("**********************\n");
	printf(" Ping Pong Descriptor \n");
	printf("**********************\n");
	printf("drift_asrc        : %d\n", pp_desc->drift_asrc);
	printf("drift_io          : %d\n", pp_desc->drift_io);
	printf("hw_ctrl_addr      : 0x%04x\n", pp_desc->hw_ctrl_addr);
	printf("copy_func_index   : %d\n", pp_desc->copy_func_index);
	printf("x_io              : %d\n", pp_desc->x_io);
	printf("data_size         : %d\n", pp_desc->data_size);
	printf("smem_addr         : 0x%02x\n", pp_desc->smem_addr);
	printf("atc_irq_data      : 0x%02x\n", pp_desc->atc_irq_data);
	printf("counter           : %d\n", pp_desc->counter);
	printf("workbuff_baseaddr : %04x\n", pp_desc->workbuff_baseaddr);
	printf("workbuff_samples  : %04x\n", pp_desc->workbuff_samples);
	printf("nextbuff0_baseaddr: %04x\n", pp_desc->nextbuff0_baseaddr);
	printf("nextbuff0_samples : %04x\n", pp_desc->nextbuff0_samples);
	printf("nextbuff1_baseaddr: %04x\n", pp_desc->nextbuff1_baseaddr);
	printf("nextbuff1_samples : %04x\n", pp_desc->nextbuff1_samples);
}

void interpret_ping_pong_desc(struct ping_pong_desc *pp_desc)
{
	printf("******************************\n");
	printf(" Decoded Ping Pong Descriptor \n");
	printf("******************************\n");
	printf("Ping base address    : 0x4908%04x\n", pp_desc->nextbuff0_baseaddr);
	printf("Pong base address    : 0x4908%04x\n", pp_desc->nextbuff1_baseaddr);
	if (pp_desc->nextbuff0_samples != pp_desc->nextbuff1_samples)
		printf("Ping size (%d B) doesn't match pong size (%d B)\n",
			pp_desc->nextbuff0_samples * 4, pp_desc->nextbuff1_samples * 4);
	else
		printf("Ping-Pong buffer size: %d B (%d samples)\n",
			pp_desc->nextbuff0_samples * 8, pp_desc->nextbuff0_samples * 2);
	if (pp_desc->workbuff_baseaddr == pp_desc->nextbuff0_baseaddr)
		printf("Current buffer       : Ping\n");
	else if (pp_desc->workbuff_baseaddr == pp_desc->nextbuff1_baseaddr)
		printf("Current buffer       : Pong\n");
	else
		printf("Current buffer       : invalid (0x4908%04x)\n", pp_desc->workbuff_baseaddr);
	printf("Remaining samples    : %d\n", pp_desc->workbuff_samples);
	if (pp_desc->data_size == 0)
		printf("Channels             : Mono\n");
	else if (pp_desc->data_size == 1)
		printf("Channels             : Stereo\n");
	else
		printf("Channels             : invalid (%d)\n", pp_desc->data_size);
	printf("Iterations           : %d\n", pp_desc->x_io * pp_desc->data_size);
	printf("MCU IRQ register     : %s (0x%04x)\n", get_reg_name(pp_desc->hw_ctrl_addr), pp_desc->hw_ctrl_addr);
	printf("MCU IRQ data         : 0x%02x\n", pp_desc->atc_irq_data);
}

int parse_atc_desc(struct atc_desc *atc_desc, int dma_req)
{
	memcpy(atc_desc, &dmem[(OMAP_ABE_D_ATCDESCRIPTORS_ADDR + sizeof(struct atc_desc)*dma_req)/4], sizeof(struct atc_desc));

	return 0;
}
int parse_io_desc(struct io_desc *io_desc, int port)
{
	memcpy(io_desc, &dmem[(OMAP_ABE_D_IODESCR_ADDR + sizeof(struct io_desc)*port)/4],sizeof(struct io_desc));

	return 0;
}

int parse_ping_pong_desc(struct ping_pong_desc *pp_desc)
{
	memcpy(pp_desc, &dmem[OMAP_ABE_D_PINGPONGDESC_ADDR/4],sizeof(struct ping_pong_desc));
	return 0;
}

void print_io_desc(struct io_desc *io_desc, int port)
{
	printf("**********************************\n");
	printf(" IO Descriptor %s\n", get_port_name(port));
	printf("**********************************\n");
	printf("drift_asrc        : %d\n", io_desc->drift_asrc);
	printf("drift_io          : %d\n", io_desc->drift_io);
	printf("io_type_idx       : %d\n", io_desc->io_type_idx);
	printf("samp_size         : 0x%02x\n", io_desc->samp_size);
	printf("flow_counter      : %d\n", io_desc->flow_counter);
	printf("hw_ctrl_addr      : 0x%04x\n", io_desc->hw_ctrl_addr);
	printf("atc_irq_data      : 0x%02x\n", io_desc->atc_irq_data);
	printf("direction_rw      : 0x%02x\n", io_desc->direction_rw);
	printf("repeat_last_samp  : 0x%02x\n", io_desc->repeat_last_samp);
	printf("nsamp             : %d\n", io_desc->nsamp);
	printf("x_io              : %d\n", io_desc->x_io);
	printf("on_off            : 0x%02x\n", io_desc->on_off);
	printf("split_addr1       : 0x%04x\n", io_desc->split_addr1);
	printf("split_addr2       : 0x%04x\n", io_desc->split_addr2);
	printf("split_addr3       : 0x%04x\n", io_desc->split_addr3);
	printf("before_f_index    : 0x%02x\n", io_desc->before_f_index);
	printf("after_f_index     : 0x%02x\n", io_desc->after_f_index);
	printf("smem_addr1        : 0x%04x\n", io_desc->smem_addr1);
	printf("atc_address1      : 0x%04x\n", io_desc->atc_address1);
	printf("atc_pointer_saved1: 0x%04x\n", io_desc->atc_pointer_saved1);
	printf("data_size1        : 0x%02x\n", io_desc->data_size1);
	printf("copy_f_index1     : %d\n", io_desc->copy_f_index1);
	printf("smem_addr2        : 0x%04x\n", io_desc->smem_addr2);
	printf("atc_address2      : 0x%04x\n", io_desc->atc_address2);
	printf("atc_pointer_saved2: 0x%04x\n", io_desc->atc_pointer_saved2);
	printf("data_size2        : 0x%02x\n", io_desc->data_size2);
	printf("copy_f_index2     : %d\n", io_desc->copy_f_index2);
}

void interpret_io_desc(struct io_desc *io_desc, int port)
{
	printf("*****************************************\n");
	printf(" Decoded IO Descriptor %s\n", get_port_name(port));
	printf("*****************************************\n");
	printf("IO type          : %d (%s)\n", io_desc->io_type_idx, id_to_fct[io_desc->io_type_idx]);
	printf("Direction        : %s\n", io_desc->direction_rw ? "Write" : "Read");
	printf("State            : %s\n", io_desc->on_off ? "On" : "Off");
	printf("MCU IRQ register : %s (0x%04x)\n", get_reg_name(io_desc->hw_ctrl_addr), io_desc->hw_ctrl_addr);
	printf("MCU IRQ data     : 0x%02x\n", io_desc->atc_irq_data);
	printf("Number of samples: %d\n", io_desc->nsamp);
	printf("Format           : %s\n", get_format_name_from_iterfactor(io_desc->samp_size));
	printf("- Configuration 1 -\n");
	printf("Index            : %d (%s)\n", io_desc->copy_f_index1, id_to_fct[io_desc->copy_f_index1]);
	printf("Data size        : 0x%04x\n", io_desc->data_size1);
	printf("SMEM address     : 0x%04x\n", io_desc->smem_addr1);
	printf("ATC address      : %s (0x4908%04x)\n", get_dma_req_name(io_desc->atc_address1 / 8), io_desc->atc_address1);
	printf("ATC pointer saved: 0x%04x\n", io_desc->atc_pointer_saved1);
	printf("- Configuration 2 -\n");
	printf("Index            : %d (%s)\n", io_desc->copy_f_index2, id_to_fct[io_desc->copy_f_index2]);
	printf("Data size        : 0x%04x\n", io_desc->data_size2);
	printf("SMEM address     : 0x%04x\n", io_desc->smem_addr2);
	printf("ATC address      : %s (0x4908%04x)\n", get_dma_req_name(io_desc->atc_address2 / 8), io_desc->atc_address2);
	printf("ATC pointer saved: 0x%04x\n", io_desc->atc_pointer_saved2);
	if (io_desc->data_size1 != io_desc->data_size2)
		printf("Data size1 (0x%04x) doesn't match size2 (0x%04x)\n", io_desc->data_size1, io_desc->data_size2);
}



int abe_print_gain()
{
	int i;
	uint32_t *p_data = smem;
	uint32_t target, current;

	for (i = 0; i <36; i++) {
		target = p_data[OMAP_ABE_S_GTARGET1_ADDR/4+i];
		current = p_data[OMAP_ABE_S_GCURRENT_ADDR/4+i];
		if ((target != 0) && (20.0 * log10f(current/262144.0) > -115)) {
			//printf("Debug %x %x", p_data[OMAP_ABE_S_GTARGET1_ADDR/4+i], p_data[OMAP_ABE_S_GCURRENT_ADDR/4+i]);
			printf("Gain:%s, Target: %2.2f, ", gain_list[i], 20.0 * log10f(target/262144.0));
			printf("Current: %2.2f\n", 20.0 * log10f(current/262144.0));
		}
	}
}

int abe_print_route()
{
	int i;
	uint32_t *p_data = dmem;

//	printf("VX Route, %x, %x", dmem[OMAP_ABE_D_AUPLINKROUTING]);
}


int abe_parse_buffer(struct buffer *buf)
{
	int i;
	float data, max, min;
	float maxl, minl, maxr, minr;
	uint32_t *p_data;
	
	max = maxl = maxr = 0.0;
	min = minr = minl = 32768.0;

	switch (buf->bank) {
	case OMAP_ABE_DMEM:
		p_data = dmem;
		break;
	case OMAP_ABE_SMEM:
	default:
		p_data = smem;
		break;
	case OMAP_ABE_CMEM:
		p_data = cmem;
		break;
	}

	for (i = 0; i< buf->size/4; i++) {
		switch (buf->bank) {
		case OMAP_ABE_DMEM:
			if ((p_data[buf->addr/4+i] & 0x8000) ==  0x8000) {
				data = (p_data[buf->addr/4+i] & 0x0000ffff)/256.0;
				data = data - 256.0;
			} else
				data = p_data[buf->addr/4+i]/256.0;
			break;
		case OMAP_ABE_SMEM:
			if ((p_data[buf->addr/4+i] & 0x800000) ==  0x800000) {
				data = p_data[buf->addr/4+i]/65536.0;
				data = data - 256.0;
			} else
				data = p_data[buf->addr/4+i]/65536.0;
			break;
		case OMAP_ABE_CMEM:
			break;
		}

		if ((buf->bank == OMAP_ABE_SMEM) ||
		    (buf->bank == OMAP_ABE_DMEM)) {
			if (i%2 == 0) {
				if (fabsf(data) > maxl)
					maxl = fabsf(data);
				if (fabsf(data) < minl)
					minl = fabsf(data);
			} else {
				if (fabsf(data) > maxr)
					maxr = fabsf(data);
				if (fabsf(data) < minr)
					minr = fabsf(data);
			}
		}
//		printf("\tData (%d): %x, %f\n", buf->addr/4+i, p_data[buf->addr/4+i], data);

		if (fabsf(data) > max)
			max = fabsf(data);
		if (fabsf(data) < min)
			min = fabsf(data);
	}

	if (buf->bank == OMAP_ABE_SMEM)
		printf("Buf: % 20s, Size % 3d, Min: %04.06f, Max: %04.06f, Min: %04.06f, Max: %04.06f\n",
			buf->name, buf->size/4, minl, maxl, minr, maxr);
	else if (buf->bank == OMAP_ABE_DMEM)
		printf("Buf: % 20s, Size % 3d, Min: %04.06f, Max: %04.06f, Min: %04.06f, Max: %04.06f\n",
			buf->name, buf->size/4, minl, maxl, minr, maxr);
	else
		printf("Buf: % 20s, Size % 3d, Min: %04.06f, Max: %04.06f\n", buf->name, buf->size/4, min, max);

}

int main(int argc, char *argv[])
{
	struct atc_desc atc_desc;
        struct io_desc io_desc;
	struct ping_pong_desc pp_desc;
	int i, j, err = 0;

	if (argc != 1) {
		printf("%s: outfile\n", argv[0]);
		return 0;
	}

	printf("ABE release: %x\n", dmem[OMAP_ABE_D_VERSION_ADDR/4]);
	printf("   OPP: %x\n", dmem[OMAP_ABE_D_MAXTASKBYTESINSLOT_ADDR/4]);

	/* AMIC */
	for (i = 0; i < ARRAY_SIZE(amic_ul_list); i++)
		abe_parse_buffer(&amic_ul_list[i]);
	printf("\n");

#if 0
	/* Vx UL 16 kHz */
	for (i = 0; i < ARRAY_SIZE(vx_ul_16k_list); i++)
		abe_parse_buffer(&vx_ul_16k_list[i]);
	printf("\n");

	/* Vx DL 16 kHz */
	for (i = 0; i < ARRAY_SIZE(vx_dl_16k_list); i++)
		abe_parse_buffer(&vx_dl_16k_list[i]);
	printf("\n");
#else
	/* Vx UL 8 kHz */
	for (i = 0; i < ARRAY_SIZE(vx_ul_8k_list); i++)
		abe_parse_buffer(&vx_ul_8k_list[i]);
	printf("\n");

	/* Vx DL 8 kHz */
	for (i = 0; i < ARRAY_SIZE(vx_dl_8k_list); i++)
		abe_parse_buffer(&vx_dl_8k_list[i]);
	printf("\n");
#endif
	/* EAR */
	for (i = 0; i < ARRAY_SIZE(earp_list); i++)
		abe_parse_buffer(&earp_list[i]);
	printf("\n");

	/* IHF */
	for (i = 0; i < ARRAY_SIZE(ihf_list); i++)
		abe_parse_buffer(&ihf_list[i]);
	printf("\n");

	/* BT UL 8k */
	for (i = 0; i < ARRAY_SIZE(bt_ul_8k_list); i++)
		abe_parse_buffer(&bt_ul_8k_list[i]);
	printf("\n");

	/* BT DL 8k */
	for (i = 0; i < ARRAY_SIZE(bt_dl_8k_list); i++)
		abe_parse_buffer(&bt_dl_8k_list[i]);
	printf("\n");

	abe_print_gain();
	abe_print_route();


	for (i = 0; i < NUM_SUPPORTED_DMA_REQS; i++) {
		parse_atc_desc(&atc_desc, i);
		if (atc_desc.desen) {
			print_atc_desc(&atc_desc, i);
			interpret_atc_desc(&atc_desc, i);
			for (j = 0; j < OMAP_ABE_ATC_TO_PORT_SIZE; j++) {
				if (atc_to_io[j][0] == i) {
					parse_io_desc(&io_desc, atc_to_io[j][1]);
					print_io_desc(&io_desc, atc_to_io[j][1]);
					interpret_io_desc(&io_desc, atc_to_io[j][1]);
				}
			}
		}
	}

	parse_ping_pong_desc(&pp_desc);
	print_ping_pong_desc(&pp_desc);
	interpret_ping_pong_desc(&pp_desc);


	return err;
}
