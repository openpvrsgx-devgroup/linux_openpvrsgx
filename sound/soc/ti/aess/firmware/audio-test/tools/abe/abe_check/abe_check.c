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
#include <getopt.h>
#include <sys/stat.h>
#include <math.h>

#include "../abe_hal/abe_cm_linux.h"
#include "../abe_hal/abe_dm_linux.h"
#include "../abe_hal/abe_sm_linux.h"
#include "../abe_hal/abe_functionsid.h"


#define NAME_SIZE	30
#define TEXT_SIZE	20
#define NUM_TEXTS	10

#define MAX_PROFILES 	8  /* Number of supported profiles */
#define MAX_COEFFS 	25      /* Number of coefficients for profiles */

/* Scheduler table is an array of 25 x 8 x 2B */
#define SCHEDULER_SLOTS	25
#define SCHEDULER_IDXS	8

/* Option flags */
#define OPT_VERSION	0x0001
#define OPT_OPP		0x0002
#define OPT_SCHED	0x0004
#define OPT_BUFFERS	0x0008
#define OPT_GAINS	0x0010
#define OPT_ROUTES	0x0020
#define OPT_ATC		0x0040
#define OPT_IODESC	0x0080
#define OPT_PPDESC	0x0100
#define OPT_ALL		0xFFFF

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(x[0]))

#define OMAP_ABE_DMEM  0
#define OMAP_ABE_SMEM  1
#define OMAP_ABE_CMEM  2

#ifdef ARM
uint32_t *dmem = NULL;
uint32_t *smem = NULL;
uint32_t *cmem = NULL;
#else
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
#endif

struct buffer {
	char name[NAME_SIZE];
	uint32_t addr;
	int size;
	int bank;
};

#define VX_DL_16K_LIST		0x0001
#define VX_DL_8K_LIST		0x0002
#define BT_DL_8K_LIST		0x0004
#define BT_DL_16K_LIST		0x0008
#define MM_DL_LIST		0x0010
#define EARP_LIST		0x0020
#define IHF_LIST		0x0040
#define AMIC_UL_LIST		0x0080
#define BT_UL_8K_LIST		0x0100
#define BT_UL_16K_LIST		0x0200
#define VX_UL_8K_LIST		0x0400
#define VX_UL_16K_LIST		0x0800

#define BUFF_LIST_ALL		0x0FFF

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
	{
		.name = "MM_DL FIFO",
		.addr = OMAP_ABE_D_MM_DL_FIFO_ADDR,
		.size = OMAP_ABE_D_MM_DL_FIFO_SIZE,
		.bank = OMAP_ABE_DMEM,
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
	unsigned short dummy1;
	unsigned short dummy2;
	unsigned short split_addr1;
	unsigned short dummy3;
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
	{McPDM_DMA_DL,  PDM_DL_PORT},
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

#define OMAP_ABE_ID_TO_FCT_SIZE    21

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
		return "MCPDM_UL";
	case 0x0E:
	case 0x0F:
	case 0x10:
	case 0x11:
		return "RESERVED";
	case 0x3F:
		return "CBPr";
	}

	return "INVALID";
}

/* Reverse of 'abe_format_switch' for 'multfac' */
char *get_format_name_from_iterfactor(int factor)
{
	switch (factor) {
	case 1:
		return "MONO_MSB, MONO_RSHIFTED_16, STEREO_16_16";
	case 2:
		return "STEREO_MSB, STEREO_RSHIFTED_16";
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
	case 0x3F:
		return "CBPr";
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

void print_release(void)
{
	uint32_t release;
	int maj, min, rev;

	release = dmem[OMAP_ABE_D_VERSION_ADDR/4];
	rev = release & 0xF;
	min = (release >> 4) & 0xFF;
	maj = (release >> 12) & 0xFF;
	printf("ABE release: %02x.%02x.%1x\n\n", maj, min, rev);
}

void print_opp(void)
{
	uint32_t task_slot = dmem[OMAP_ABE_D_MAXTASKBYTESINSLOT_ADDR/4];

	printf("OPP: ");

	switch (task_slot) {
	case 0x04:
		printf("25\n\n");
		break;
	case 0x0C:
		printf("50\n\n");
		break;
	case 0x10:
		printf("100\n\n");
		break;
	default:
		printf("UNKNOWN\n\n");
		break;
	}
}

void print_atc_desc(struct atc_desc *atc_desc, int dma_req)
{
	printf("|---------------------------------|\n");
	printf("| ATC Descriptor %-16s |\n", get_dma_req_name(dma_req));
	printf("|---------------------------------|\n");
	printf("| %-13s | 0x%02x            |\n", "RDPT", atc_desc->rdpt);
	printf("| %-13s | 0x%1x             |\n", "RESERVED0", atc_desc->reserved0);
	printf("| %-13s | 0x%02x            |\n", "CBSIZE", atc_desc->cbsize);
	printf("| %-13s | 0x%1x             |\n", "IRQDEST", atc_desc->irqdest);
	printf("| %-13s | 0x%1x             |\n", "CBERR", atc_desc->cberr);
	printf("| %-13s | 0x%02x            |\n", "RESERVED1", atc_desc->reserved1);
	printf("| %-13s | 0x%1x             |\n", "CBDIR", atc_desc->cbdir);
	printf("| %-13s | 0x%1x             |\n", "NW", atc_desc->nw);
	printf("| %-13s | 0x%02x            |\n", "WRPT", atc_desc->wrpt);
	printf("| %-13s | 0x%1x             |\n", "RESERVED2", atc_desc->reserved2);
	printf("| %-13s | 0x%03x           |\n", "BADD", atc_desc->badd);
	printf("| %-13s | 0x%02x            |\n", "ITER", atc_desc->iter);
	printf("| %-13s | 0x%02x            |\n", "SRCID", atc_desc->srcid);
	printf("| %-13s | 0x%02x            |\n", "DESTID", atc_desc->destid);
	printf("| %-13s | 0x%1x             |\n", "DESEN", atc_desc->desen);
	printf("|---------------------------------|\n\n");
}

void interpret_atc_desc(struct atc_desc *atc_desc, int dma_req)
{
	printf("|-------------------------------------------------|\n");
	printf("| ATC Descriptor Decoded %-24s |\n", get_dma_req_name(dma_req));
	printf("|-------------------------------------------------|\n");
	printf("| %-30s | 0x%02x           |\n", "Read pointer relative address", atc_desc->rdpt);
	printf("| %-30s | 0x%02x           |\n", "Circular buffer size", atc_desc->cbsize);
	printf("| %-30s | %-14s |\n", "IRQ destination", atc_desc->irqdest ? "MCU" : "DSP");
	printf("| %-30s | %-3d            |\n", "Report circular buffer errors", atc_desc->cberr);
	printf("| %-30s | %-3s (wrt AESS) |\n", "Circular buffer direction", atc_desc->cbdir ? "Out" : "In");
	printf("| %-30s | 0x%02x           |\n", "Write pointer relative address", atc_desc->wrpt);
	printf("| %-30s | 0x4018%04x     |\n", "Base address", atc_desc->badd << 4);
	printf("| %-30s | %-14d |\n", "Iteration", atc_desc->iter);
	printf("| %-30s | %-14s |\n", "Source ID peripheral", get_atc_source_name(atc_desc->srcid));
	printf("| %-30s | %-14s |\n", "Destination ID peripheral", get_atc_dest_name(atc_desc->destid));
	printf("| %-30s | %-14s |\n", "Descriptor activation", atc_desc->desen ? "Active" : "Not Active");
	printf("|-------------------------------------------------|\n\n");
}

void print_ping_pong_desc(struct ping_pong_desc *pp_desc)
{
	printf("|---------------------------------|\n");
	printf("| Ping Pong Descriptor            |\n");
	printf("|---------------------------------|\n");
	printf("| %-22s | %-6d |\n", "drift_asrc", pp_desc->drift_asrc);
	printf("| %-22s | %-6d |\n", "drift_io", pp_desc->drift_io);
	printf("| %-22s | 0x%04x |\n", "hw_ctrl_addr", pp_desc->hw_ctrl_addr);
	printf("| %-22s | %-6d |\n", "copy_func_index", pp_desc->copy_func_index);
	printf("| %-22s | %-6d |\n", "x_io", pp_desc->x_io);
	printf("| %-22s | %-6d |\n", "data_size", pp_desc->data_size);
	printf("| %-22s | 0x%02x   |\n", "smem_addr", pp_desc->smem_addr);
	printf("| %-22s | 0x%02x   |\n", "atc_irq_data", pp_desc->atc_irq_data);
	printf("| %-22s | %-6d |\n", "counter", pp_desc->counter);
	printf("| %-22s | 0x%04x |\n", "workbuff_baseaddr", pp_desc->workbuff_baseaddr);
	printf("| %-22s | %-6d |\n", "workbuff_samples", pp_desc->workbuff_samples);
	printf("| %-22s | 0x%04x |\n", "nextbuff0_baseaddr", pp_desc->nextbuff0_baseaddr);
	printf("| %-22s | %-6d |\n", "nextbuff0_samples", pp_desc->nextbuff0_samples);
	printf("| %-22s | 0x%04x |\n", "nextbuff1_baseaddr", pp_desc->nextbuff1_baseaddr);
	printf("| %-22s | %-6d |\n", "nextbuff1_samples", pp_desc->nextbuff1_samples);
	printf("|---------------------------------|\n\n");
}

void interpret_ping_pong_desc(struct ping_pong_desc *pp_desc)
{
	int frame_bytes = pp_desc->data_size << 2;

	printf("|-----------------------------------------------------------------|\n");
	printf("| Decoded Ping Pong Descriptor                                    |\n");
	printf("|-----------------------------------------------------------------|\n");
	printf("| %-24s | 0x4908%04x%-26s |\n", "Ping base address", pp_desc->nextbuff0_baseaddr, "");
	printf("| %-24s | 0x4908%04x%-26s |\n", "Pong base address", pp_desc->nextbuff1_baseaddr, "");
	printf("| %-24s | %-36d |\n", "Ping-Pong period bytes", pp_desc->nextbuff0_samples * frame_bytes);
	/* Ping-pong desc samples term refers to period frames */
	printf("| %-24s | %-36d |\n", "Ping-Pong period frames", pp_desc->nextbuff0_samples);

	if (pp_desc->workbuff_baseaddr == pp_desc->nextbuff0_baseaddr)
		printf("| %-24s | %-36s |\n", "Current buffer", "Ping");
	else if (pp_desc->workbuff_baseaddr == pp_desc->nextbuff1_baseaddr)
		printf("| %-24s | %-36s |\n", "Current buffer", "Pong");
	else
		printf("| %-24s | INVALID (0x4908%04x)%-16s |\n", "Current buffer",pp_desc->workbuff_baseaddr, "");

	printf("| %-24s | %-36d |\n", "Remaining samples", pp_desc->workbuff_samples);
	printf("| %-24s | %-36d |\n", "Iterations", pp_desc->x_io * pp_desc->data_size);
	printf("| %-24s | %-36s |\n", "MCU IRQ register", get_reg_name(pp_desc->hw_ctrl_addr));
	printf("| %-24s | 0x%02x%-32s |\n", "MCU IRQ data", pp_desc->atc_irq_data, "");
	printf("|-----------------------------------------------------------------|\n");

	if (pp_desc->nextbuff0_samples != pp_desc->nextbuff1_samples)
		printf(" Note: Ping size (%d B) does not match pong size (%d B)\n",
		       pp_desc->nextbuff0_samples * frame_bytes,
		       pp_desc->nextbuff1_samples * frame_bytes);

	printf("\n");
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
	printf("|---------------------------------|\n");
	printf("| IO Descriptor %-17s |\n", get_port_name(port));
	printf("|---------------------------------|\n");
	printf("| %-22s | %-6d |\n", "drift_asrc", io_desc->drift_asrc);
	printf("| %-22s | %-6d |\n", "drift_io", io_desc->drift_io);
	printf("| %-22s | %-6d |\n", "io_type_idx", io_desc->io_type_idx);
	printf("| %-22s | 0x%02x   |\n", "samp_size", io_desc->samp_size);
	printf("| %-22s | %-6d |\n", "flow_counter", io_desc->flow_counter);
	printf("| %-22s | 0x%04x |\n", "hw_ctrl_addr", io_desc->hw_ctrl_addr);
	printf("| %-22s | 0x%02x   |\n", "atc_irq_data", io_desc->atc_irq_data);
	printf("| %-22s | 0x%02x   |\n", "direction_rw", io_desc->direction_rw);
	printf("| %-22s | 0x%02x   |\n", "repeat_last_samp", io_desc->repeat_last_samp);
	printf("| %-22s | %-6d |\n", "nsamp", io_desc->nsamp);
	printf("| %-22s | %-6d |\n", "x_io", io_desc->x_io);
	printf("| %-22s | 0x%02x   |\n", "on_off", io_desc->on_off);
	printf("| %-22s | 0x%04x |\n", "split_addr1", io_desc->split_addr1);
	printf("| %-22s | 0x%04x |\n", "split_addr2", io_desc->split_addr2);
	printf("| %-22s | 0x%04x |\n", "split_addr3", io_desc->split_addr3);
	printf("| %-22s | 0x%02x   |\n", "before_f_index", io_desc->before_f_index);
	printf("| %-22s | 0x%02x   |\n", "after_f_index", io_desc->after_f_index);
	printf("| %-22s | 0x%04x |\n", "smem_addr1", io_desc->smem_addr1);
	printf("| %-22s | 0x%04x |\n", "atc_address1", io_desc->atc_address1);
	printf("| %-22s | 0x%04x |\n", "atc_pointer_saved1", io_desc->atc_pointer_saved1);
	printf("| %-22s | 0x%02x   |\n", "data_size1", io_desc->data_size1);
	printf("| %-22s | %-6d |\n", "copy_f_index1", io_desc->copy_f_index1);
	printf("| %-22s | 0x%04x |\n", "smem_addr2", io_desc->smem_addr2);
	printf("| %-22s | 0x%04x |\n", "atc_address2", io_desc->atc_address2);
	printf("| %-22s | 0x%04x |\n", "atc_pointer_saved2", io_desc->atc_pointer_saved2);
	printf("| %-22s | 0x%02x   |\n", "data_size2", io_desc->data_size2);
	printf("| %-22s | %-6d |\n", "copy_f_index2", io_desc->copy_f_index2);
	printf("|---------------------------------|\n\n");
}

void interpret_io_desc(struct io_desc *io_desc, int port)
{
	printf("|-------------------------------------------------------------------|\n");
	printf("| Decoded IO Descriptor %-43s |\n", get_port_name(port));
	printf("|-------------------------------------------------------------------|\n");
	printf("| %-22s | %-40d |\n", "IO type", io_desc->io_type_idx);
	printf("| %-22s | %-40s |\n", "Direction", io_desc->direction_rw ? "Write" : "Read");
	printf("| %-22s | %-40s |\n", "State", io_desc->on_off ? "On" : "Off");
	printf("| %-22s | %-40s |\n", "MCU IRQ register", get_reg_name(io_desc->hw_ctrl_addr));
	printf("| %-22s | 0x%02x%-36s |\n", "MCU IRQ data", io_desc->atc_irq_data, "");
	printf("| %-22s | %-40d |\n", "Number of samples", io_desc->nsamp);
	printf("| %-22s | %-40s |\n", "Format", get_format_name_from_iterfactor(io_desc->samp_size));
	printf("| %-22s | %-40s |\n", "Configuration 1", "");
	printf("| %-22s | %-40s |\n", "Index", id_to_fct[io_desc->copy_f_index1]);
	printf("| %-22s | %-40d |\n", "Data size", io_desc->data_size1);
	printf("| %-22s | 0x%04x%-34s |\n", "SMEM address", io_desc->smem_addr1, "");
	printf("| %-22s | %-27s (0x4908%04x) |\n", "ATC address", get_dma_req_name(io_desc->atc_address1 / 8), io_desc->atc_address1);
	printf("| %-22s | 0x%04x%-34s |\n", "ATC pointer saved", io_desc->atc_pointer_saved1, "");
	printf("| %-22s | %-40s |\n", "Configuration 1", "");
	printf("| %-22s | %-40d |\n", "Index", io_desc->copy_f_index2);
	printf("| %-22s | %-40d |\n", "Data size", io_desc->data_size2);
	printf("| %-22s | 0x%04x%-34s |\n", "SMEM address", io_desc->smem_addr2, "");
	printf("| %-22s | %-27s (0x4908%04x) |\n", "ATC address", get_dma_req_name(io_desc->atc_address2 / 8), io_desc->atc_address2);
	printf("| %-22s | 0x%04x%-34s |\n", "ATC pointer saved", io_desc->atc_pointer_saved2, "");
	printf("|-------------------------------------------------------------------|\n\n");
}

int abe_print_gain()
{
	int i;
	uint32_t target, current;
	uint32_t *p_data = smem;

	printf("|----------------------------------------|\n");
	printf("|        %-4s        | %-7s | %-7s |\n", "Gain", "Target", "Current");
	printf("|--------------------|---------|---------|\n");

	for (i = 0; i <36; i++) {
		target = p_data[OMAP_ABE_S_GTARGET1_ADDR/4+i];
		current = p_data[OMAP_ABE_S_GCURRENT_ADDR/4+i];
		if ((target != 0) && (20.0 * log10f(current/262144.0) > -115)) {
			//printf("Debug %x %x", p_data[OMAP_ABE_S_GTARGET1_ADDR/4+i], p_data[OMAP_ABE_S_GCURRENT_ADDR/4+i]);
			printf("| %-18s | %7.2f | %7.2f |\n",
				gain_list[i],
				20.0 * log10f(target/262144.0),
				20.0 * log10f(current/262144.0));
		}
	}

	printf("|----------------------------------------|\n\n");
}

int abe_print_route()
{
	int i;
	uint32_t *p_data = dmem;

//	printf("VX Route, %x, %x", dmem[OMAP_ABE_D_AUPLINKROUTING]);
}

int task_to_id(int task)
{
	int id;

	/* ABE_STask size is 16 bytes */
	if (task)
		id = (task - OMAP_ABE_D_TASKSLIST_ADDR) / 16;
	else
		id = -1;

	return id;
}

void print_scheduler_table(void)
{
	int addr, id, i, j;
	uint32_t val;

	printf("|-----------------------------------------------------|\n");
	printf("|                  Scheduling Table                   |\n");
	printf("|-----------------------------------------------------|\n");
	printf("| S\\I ");
	for (j = 0; j < SCHEDULER_IDXS; j++)
		printf("| %3d ", j + 1);
	printf("|\n");
	printf("|-----|-----|-----|-----|-----|-----|-----|-----|-----|\n");

	for (i = 0; i < SCHEDULER_SLOTS; i++) {
		printf("| %3d ", i + 1);
		for (j = 0; j < SCHEDULER_IDXS; j++) {
			addr = OMAP_ABE_D_MULTIFRAME_ADDR + (i * SCHEDULER_IDXS + j) * 2;
			val = dmem[addr / 4];

			if (addr % 4)
				id = task_to_id((val >> 16) & 0xFFFF);
			else
				id = task_to_id(val & 0xFFFF);

			if (id > 0)
				printf("| %3d ", id);
			else
				printf("|  -  ");
		}
		printf("|\n");
	}
	printf("|-----------------------------------------------------|\n\n");
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

	if ((buf->bank == OMAP_ABE_SMEM) || (buf->bank == OMAP_ABE_DMEM))
		printf("| %-25s | %4d | %15.06f | %15.06f | %15.06f | %15.06f |\n",
			buf->name, buf->size/4, minl, maxl, minr, maxr);
	else
		printf("| %-25s | %4d | %15.04f | %15.04f | %15s | %15s |\n",
			buf->name, buf->size/4, min, max, "-", "-");

}

void print_buffers(int buffer_list)
{
	int i;

	printf("|------------------------------------------------------");
	printf("----------------------------------------------------|\n");
	printf("| %25s | %4s | %33s | %33s |\n",
		"", "",
		"              Left               ",
		"              Right              ");
        printf("| %25s | %4s |"
		"-----------------------------------|"
		"-----------------------------------|\n",
		"          Buffer         ", "Size");
        printf("| %25s | %4s | %15s | %15s | %15s | %15s |\n",
		"", "",
		"     Min     ", "     Max     ",
		"     Min     ", "     Max     ");
	printf("|---------------------------|------|-----------------|");
	printf("-----------------|-----------------|-----------------|\n");

	/* AMIC */
	if (buffer_list & AMIC_UL_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Analog Microphone", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(amic_ul_list); i++)
			abe_parse_buffer(&amic_ul_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	/* VX UL 16 kHz */
	if (buffer_list & VX_UL_16K_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Voice Uplink 16kHz", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(vx_ul_16k_list); i++)
			abe_parse_buffer(&vx_ul_16k_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	/* VX DL 16kHz */
	if (buffer_list & VX_DL_16K_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Voice Downlink 16kHz", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(vx_dl_16k_list); i++)
			abe_parse_buffer(&vx_dl_16k_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	/* VX UL 8kHz */
	if (buffer_list & VX_UL_8K_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Voice Uplink 8kHz", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(vx_ul_8k_list); i++)
			abe_parse_buffer(&vx_ul_8k_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	/* VX DL 8kHz */
	if (buffer_list & VX_DL_8K_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Voice Downlink 8kHz", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(vx_dl_8k_list); i++)
			abe_parse_buffer(&vx_dl_8k_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	/* EAR */
	if (buffer_list & EARP_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Earpiece", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(earp_list); i++)
			abe_parse_buffer(&earp_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	/* IHF */
	if (buffer_list & IHF_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Hands Free", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(ihf_list); i++)
			abe_parse_buffer(&ihf_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	/* MM DL */
	if (buffer_list & MM_DL_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Multimedia Downlink", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(mm_dl_list); i++)
			abe_parse_buffer(&mm_dl_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	/* BT UL 8kHz */
	if (buffer_list & BT_UL_8K_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Bluetooth Uplink 8kHz", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(bt_ul_8k_list); i++)
			abe_parse_buffer(&bt_ul_8k_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	/* BT DL 8kHz */
	if (buffer_list & BT_DL_8K_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Bluetooth Downlink 8kHz", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(bt_dl_8k_list); i++)
			abe_parse_buffer(&bt_dl_8k_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	/* BT UL 16kHz */
	if (buffer_list & BT_UL_16K_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Bluetooth Uplink 16kHz", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(bt_ul_16k_list); i++)
			abe_parse_buffer(&bt_ul_16k_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	/* BT DL 16kHz */
	if (buffer_list & BT_DL_16K_LIST) {
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"Bluetooth Downlink 16kHz", "", "", "", "", "");
		for (i = 0; i < ARRAY_SIZE(bt_dl_16k_list); i++)
			abe_parse_buffer(&bt_dl_16k_list[i]);
		printf("| %-25s | %4s | %15s | %15s | %15s | %15s |\n",
			"", "", "", "", "", "");
	}

	printf("|------------------------------------------------------");
	printf("----------------------------------------------------|\n\n");
}

#ifdef ARM

void get_abe_data(const char *name, uint32_t **data)
{
	char filename[128];
	int fd;
	int size, bytes;

	sprintf(filename, "/sys/kernel/debug/omap-abe/%s", name);

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "failed to open %s : %d\n", filename, fd);
		exit(fd);
	}

	size = lseek(fd, 0, SEEK_END);
	if (size < 0) {
		fprintf(stderr, "failed to get size for %s : %d\n", filename, size);
		exit(size);
	}
	lseek(fd, 0, SEEK_SET);

	fprintf(stdout, "loaded %s size %d\n", filename, size);
	*data = malloc(size);
	if (*data == NULL)
		exit(-ENOMEM);

	bytes = read(fd, *data, size);
	if (bytes != size) {
		fprintf(stderr, "failed to get read %s : %d\n", filename, bytes);
		exit(size);
	}
	close(fd);
}

#endif

void help(char *name)
{
	printf("Usage: %s [OPTION]\n", name);
	printf("-v --version   ABE version\n");
	printf("-o --opp       AESS OPP\n");
	printf("-b --buffers   AESS internal buffers stats\n");
	printf("-g --gains     gains which are not muted\n");
	printf("-r --routes    active routes\n");
	printf("-a --atc       active ATC descriptors\n");
	printf("-i --iodesc    IO descriptors\n");
	printf("-p --ppdesc    Ping-Pong descriptor\n");
	printf("-h --help      help\n");
}

static const struct option long_options[] = {
	{"version", no_argument, 0, 'v'},
	{"opp", no_argument, 0, 'o'},
	{"sched", no_argument, 0, 's'},
	{"buffers", no_argument, 0, 'b'},
	{"gains", no_argument, 0, 'g'},
	{"routes", no_argument, 0, 'r'},
	{"atc", no_argument, 0, 'a'},
	{"iodesc", no_argument, 0, 'i'},
	{"ppdesc", no_argument, 0, 'p'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0},
};

static const char short_options[] = "vosbgraiph";

int main(int argc, char *argv[])
{
	struct atc_desc atc_desc;
        struct io_desc io_desc;
	struct ping_pong_desc pp_desc;
	int i, j;
	signed char c;
	int opt = 0;

#ifdef ARM

	get_abe_data("cmem", &cmem);
	get_abe_data("dmem", &dmem);
	get_abe_data("smem", &smem);

#endif

	while ((c = getopt_long(argc, argv, short_options, long_options, 0)) != -1) {

		switch (c) {
		case 'v':
			opt |= OPT_VERSION;
			break;
		case 'o':
			opt |= OPT_OPP;
			break;
		case 's':
			opt |= OPT_SCHED;
			break;
		case 'b':
			opt |= OPT_BUFFERS;
			break;
		case 'g':
			opt |= OPT_GAINS;
			break;
		case 'r':
			opt |= OPT_ROUTES;
			break;
		case 'a':
			opt |= OPT_ATC;
			break;
		case 'i':
			opt |= OPT_IODESC;
			break;
		case 'p':
			opt |= OPT_PPDESC;
			break;
		case 'h':
			help(argv[0]);
			return 0;
		default:
			help(argv[0]);
			return 1;
		}
	}

	/* dump all options if no arg */
	if (argc == 1)
		opt = OPT_ALL;

	if (opt & OPT_VERSION)
		print_release();

	if (opt & OPT_OPP)
		print_opp();

	if (opt & OPT_SCHED)
		print_scheduler_table();

	if (opt & OPT_BUFFERS)
		print_buffers(BUFF_LIST_ALL);

	if (opt & OPT_GAINS)
		abe_print_gain();

	if (opt & OPT_ROUTES)
		abe_print_route();

	for (i = 0; i < NUM_SUPPORTED_DMA_REQS; i++) {
		parse_atc_desc(&atc_desc, i);
		if (atc_desc.desen) {
			if (opt & OPT_ATC) {
				print_atc_desc(&atc_desc, i);
				interpret_atc_desc(&atc_desc, i);
			}
			for (j = 0; j < OMAP_ABE_ATC_TO_PORT_SIZE; j++) {
				if (atc_to_io[j][0] == i) {
					if (opt & OPT_IODESC) {
						parse_io_desc(&io_desc, atc_to_io[j][1]);
						print_io_desc(&io_desc, atc_to_io[j][1]);
						interpret_io_desc(&io_desc, atc_to_io[j][1]);
					}
				}
			}
		}
	}

	if (opt & OPT_PPDESC) {
		parse_ping_pong_desc(&pp_desc);
		print_ping_pong_desc(&pp_desc);
		interpret_ping_pong_desc(&pp_desc);
	}

	return 0;
}
