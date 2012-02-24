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

#include <stdio.h>
#include <string.h>


#define MCPDM_DL_ID	0
#define MCPDM_UL_ID	1
#define VX_DL_ID	2
#define VX_UL_ID	3
#define BT_DL_ID	4
#define BT_UL_ID	5
#define MM_DL_ID	6
#define DMIC_ID		7
#define MM_EXT_IN_ID	8
#define MM_EXT_OUT_ID	9
#define MM_UL_ID	10
#define MM_UL2_ID	11
#define TONES_ID	12
#define VIB_DL_ID	13
#define MAX_PORT_ID	14

#define MCPDM_DL_TRACE_MASK	(0x0001<<0)
#define MCPDM_UL_TRACE_MASK	(0x0001<<1)
#define VX_DL_TRACE_MASK	(0x0001<<2)
#define VX_UL_TRACE_MASK	(0x0001<<3)
#define BT_DL_TRACE_MASK	(0x0001<<4)
#define BT_UL_TRACE_MASK	(0x0001<<5)
#define MM_DL_TRACE_MASK	(0x0001<<6)
#define DMIC_TRACE_MASK		(0x0001<<7)
#define MM_EXT_IN_TRACE_MASK	(0x0001<<8)
#define MM_EXT_OUT_TRACE_MASK	(0x0001<<9)
#define MM_UL_TRACE_MASK	(0x0001<<10)
#define MM_UL2_TRACE_MASK	(0x0001<<11)
#define TONES_TRACE_MASK	(0x0001<<12)
#define VIB_DL_TRACE_MASK	(0x0001<<13)

/* This struct is defining the different data associate to each port inside
 * the Audio Back End Firmware trace. All the ATC descriptor read and write
 * pointer are logged every 250 microseconds */
typedef struct abe_port_trc {
	char read;
	char write;
	char flow;
};

/* Full Audio Back End trace content
 * Block of up to 128 bytes can be outputted by the Audio Engine with a
 * DMA channel mapped on CBPr7. */
typedef struct abe_slot_trc {
	unsigned long slot;
	/* port information */
	struct abe_port_trc port[MAX_PORT_ID];
	char slot_update;
};

void help(char *app_name)
{
	int i;

	printf("%s: <binary trace file> <trace line size> <mask>\n", app_name);
	printf("\t<binary trace file>: binary trace file form the target\n");
	printf("\t<trace line size>: Size of the trace line (between 1 to 128)\n");
	printf("\t<mask>: Mask for analysis (0xFFFFF mean all)\n");
	printf("\t\t0x00001: McPDM DL\n");
	printf("\t\t0x00002: McPDM UL\n");
	printf("\t\t...\n");
}

/* generic function in order to analyze the delta between 2 traces.
 * Generally the ATC pointer are always moving in regulatar mode
 * between two consecutive trace. This function is printing the
 * evolution of the pointer in order to be able to check ATC
 * behavior */
void port_print(struct abe_port_trc *current, struct abe_port_trc *prev, char *app_name)
{
	printf("\t %s R=%03d(%d) W=%03d(%d) Flow:%d\n",
		app_name, current->read, (int)((current->read - prev->read) + 120) % 120,
		current->write, (int)((current->write - prev->write) + 120) % 120,
		current->flow);
}

/**
 * abe_debug_trace
 *   Read debug trace related to on Multiframe (250 microseconds)
 * @data: pointer on the 128 bytes of the trace
 * @slot: pointer on Output structure for the trace data 
 *
 * Return ABE Firwmare multiframe number.
 */
int abe_debug_trace(char *data, struct abe_slot_trc *slot)
{
	slot->slot  = (unsigned char)data[0] | ((unsigned char)(data[1]) << 8) |
		((unsigned char)(data[2]) << 16) | ((unsigned char)(data[3]) << 24);
//	printf("%x %x %x %x \n",(unsigned char)data[0], (unsigned char)data[1], (unsigned char)data[2], (unsigned char)data[3]);

	slot->port[MCPDM_DL_ID].read  = data[26];
	slot->port[MCPDM_DL_ID].write = data[27];
	slot->port[MCPDM_DL_ID].flow  = data[47];

	slot->port[MCPDM_UL_ID].read  = data[6];
	slot->port[MCPDM_UL_ID].write = data[7];
	slot->port[MCPDM_UL_ID].flow  = data[37];

	slot->port[VX_DL_ID].read  = data[18];
	slot->port[VX_DL_ID].write = data[19];
	slot->port[VX_DL_ID].flow  = data[43];

	slot->port[VX_UL_ID].read  = data[14];
	slot->port[VX_UL_ID].write = data[15];
	slot->port[VX_UL_ID].flow  = data[41];

	slot->port[BT_DL_ID].read  = data[24];
	slot->port[BT_DL_ID].write = data[25];
	slot->port[BT_DL_ID].flow  = data[46];

	slot->port[BT_UL_ID].read  = data[8];
	slot->port[BT_UL_ID].write = data[9];
	slot->port[BT_UL_ID].flow  = data[38];

	slot->port[MM_DL_ID].read  = data[16];
	slot->port[MM_DL_ID].write = data[17];
	slot->port[MM_DL_ID].flow  = data[42];

	slot->port[DMIC_ID].read  = data[4];
	slot->port[DMIC_ID].write = data[5];
	slot->port[DMIC_ID].flow  = data[36];

	slot->port[MM_EXT_IN_ID].read  = data[30];
	slot->port[MM_EXT_IN_ID].write = data[31];
	slot->port[MM_EXT_IN_ID].flow  = data[49];

	slot->port[MM_EXT_OUT_ID].read  = data[28];
	slot->port[MM_EXT_OUT_ID].write = data[29];
	slot->port[MM_EXT_OUT_ID].flow  = data[48];

	slot->port[MM_UL_ID].read  = data[10];
	slot->port[MM_UL_ID].write = data[11];
	slot->port[MM_UL_ID].flow  = data[39];

	slot->port[MM_UL2_ID].read  = data[12];
	slot->port[MM_UL2_ID].write = data[13];
	slot->port[MM_UL2_ID].flow  = data[40];

	slot->port[TONES_ID].read  = data[20];
	slot->port[TONES_ID].write = data[21];
	slot->port[TONES_ID].flow  = data[44];

	slot->port[VIB_DL_ID].read  = data[22];
	slot->port[VIB_DL_ID].write = data[23];
	slot->port[VIB_DL_ID].flow  = data[45];

// Firmware Debug Trace is 96 bytes large and dumped to D_DEBUG_FIFO_ADDR
// byte [32] : index of TDM_OUT ATC RD ptr
// byte [33] : index of TDM_OUT ATC WR ptr
// byte [34] : index of TDM_IN ATC RD ptr
// byte [35] : index of TDM_IN ATC WR ptr
// byte [50] : index of TDM_OUT flow_counter
// byte [51] : index of TDM_IN flow_counter
/*

	if (data[74] != 2)
		printf("***************\n SLOT Update %d \n", data[74]);
*/
	slot->slot_update  = data[74];
	
	return (slot->slot);
}

int main(int argc, char *argv[])
{
	FILE *fp;
	struct abe_slot_trc trc_slot;
	struct abe_slot_trc prev_trc_slot;
	char trace[128];
	char filename[100];
	int port, i, ret = 0;
	int prev_slot = 0;
	int slot = 0;
	int count = 0;
	size_t abe_trace_size = 128;
	int mask = 0x0000;

	if (argc < 4) {
		help(argv[0]);
		return 1;
	}

	strcpy(filename, argv[1]);
	abe_trace_size = atoi(argv[2]);
	sscanf(argv[3], "0x%x", &mask);

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		printf("couldn't open binary file '%s'\n", filename);
		return 1;
	}

	/* Read first slot */
	if (fread(trace, sizeof(char), 128, fp) != 128)
		return;
	prev_slot = abe_debug_trace(trace, &prev_trc_slot);

	/* Loop on all slots inside the trace */
	while (fread(trace, sizeof(char), 128, fp) == 128) {
		/* Reformat debug trace information */
		slot = abe_debug_trace(trace, &trc_slot);
		count++;

		/* Check for trace gaps */
		if (prev_slot != (slot - 1)) {
			printf("===> Slot Gap in trace %d slot:%d(%d)(0x%08x)%x\n", (slot - prev_slot), slot, count, slot, prev_slot);
			count = 0;
		}

                /* In case Count = 0 we have issue inside the trace so do not analyze the first slot after */
		if (count != 0) {
			/* Analyse scheduler synchro slot */
			if (trc_slot.slot_update == 1)
				printf("Scheduler slot removal  <---- (%d)\n", slot);
			else if (trc_slot.slot_update == 3)
				printf("Scheduler slot addition <---- (%d)\n", slot);

			if ((mask & MCPDM_DL_TRACE_MASK) == MCPDM_DL_TRACE_MASK)
				port_print(&trc_slot.port[MCPDM_DL_ID], &prev_trc_slot.port[MCPDM_DL_ID], "McPDM DL");

			if ((mask & MCPDM_UL_TRACE_MASK) == MCPDM_UL_TRACE_MASK)
				port_print(&trc_slot.port[MCPDM_UL_ID], &prev_trc_slot.port[MCPDM_UL_ID], "McPDM UL");

			if ((mask & VX_DL_TRACE_MASK) == VX_DL_TRACE_MASK)
				port_print(&trc_slot.port[VX_DL_ID], &prev_trc_slot.port[VX_DL_ID], "VX DL");

			if ((mask & VX_UL_TRACE_MASK) == VX_UL_TRACE_MASK)
				port_print(&trc_slot.port[VX_UL_ID], &prev_trc_slot.port[VX_UL_ID], "VX UL");

			if ((mask & BT_DL_TRACE_MASK) == BT_DL_TRACE_MASK)
				port_print(&trc_slot.port[BT_DL_ID], &prev_trc_slot.port[BT_DL_ID], "BT DL");

			if ((mask & BT_UL_TRACE_MASK) == BT_UL_TRACE_MASK)
				port_print(&trc_slot.port[BT_UL_ID], &prev_trc_slot.port[BT_UL_ID], "BT UL");

			if ((mask & MM_DL_TRACE_MASK) == MM_DL_TRACE_MASK)
				port_print(&trc_slot.port[MM_DL_ID], &prev_trc_slot.port[MM_DL_ID], "MM DL");

			if ((mask & DMIC_TRACE_MASK) == DMIC_TRACE_MASK)
				port_print(&trc_slot.port[DMIC_ID], &prev_trc_slot.port[DMIC_ID], "DMIC DL");

			if ((mask & MM_EXT_IN_TRACE_MASK) == MM_EXT_IN_TRACE_MASK)
				port_print(&trc_slot.port[MM_EXT_IN_ID], &prev_trc_slot.port[MM_EXT_IN_ID], "MM EXT IN");

			if ((mask & MM_EXT_OUT_TRACE_MASK) == MM_EXT_OUT_TRACE_MASK)
				port_print(&trc_slot.port[MM_EXT_OUT_ID], &prev_trc_slot.port[MM_EXT_OUT_ID], "MM EXT_OUT");

			if ((mask & MM_UL_TRACE_MASK) == MM_UL_TRACE_MASK)
				port_print(&trc_slot.port[MM_UL_ID], &prev_trc_slot.port[MM_UL_ID], "MM UL");

			if ((mask & MM_UL2_TRACE_MASK) == MM_UL2_TRACE_MASK)
				port_print(&trc_slot.port[MM_UL2_ID], &prev_trc_slot.port[MM_UL2_ID], "MM UL2");

			if ((mask & TONES_TRACE_MASK) == TONES_TRACE_MASK)
				port_print(&trc_slot.port[TONES_ID], &prev_trc_slot.port[TONES_ID], "TONES");

			if ((mask & VIB_DL_TRACE_MASK) == VIB_DL_TRACE_MASK)
				port_print(&trc_slot.port[VIB_DL_ID], &prev_trc_slot.port[VIB_DL_ID], "VIB DL");

		}
		/* Save current slot to previous slow for next comparaison */		
		prev_slot = slot;
		memcpy(&prev_trc_slot, &trc_slot, sizeof(struct abe_slot_trc));
	}

out:
	fclose(fp);
	return ret;
}
