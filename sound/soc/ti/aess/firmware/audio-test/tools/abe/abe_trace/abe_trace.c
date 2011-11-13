#include <stdio.h>
#include <string.h>

typedef struct abe_slot_trc {
	unsigned int slot;
	/* McPDM port information */
	char mcpdm_dl_read;
	char mcpdm_dl_write;
	char mcpdm_dl_flow;
	char mcpdm_ul_read;
	char mcpdm_ul_write;
	char mcpdm_ul_flow;
	/* Voice port information */
	char vx_ul_read;
	char vx_ul_write;
	char vx_ul_flow;
	char vx_dl_read;
	char vx_dl_write;
	char vx_dl_flow;
	/* BT port information */
	char bt_ul_read;
	char bt_ul_write;
	char bt_ul_flow;
	char bt_dl_read;
	char bt_dl_write;
	char bt_dl_flow;
	/* Multimedia DL port information */
	char mm_dl_read;
	char mm_dl_write;
	char mm_dl_flow;

	char slot_update;
};

void help(char *app_name)
{
	int i;

	printf("%s: <binary trace file>\n", app_name);
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
	slot->slot  = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
	slot->mcpdm_dl_read  = data[26];
	slot->mcpdm_dl_write = data[27];
	slot->mcpdm_dl_flow  = data[47];

	slot->mcpdm_ul_read  = data[6];
	slot->mcpdm_ul_write = data[7];
	slot->mcpdm_ul_flow  = data[37];

	slot->vx_dl_read  = data[18];
	slot->vx_dl_write = data[19];
	slot->vx_dl_flow  = data[43];

	slot->vx_ul_read  = data[14];
	slot->vx_ul_write = data[15];
	slot->vx_ul_flow  = data[41];

	slot->bt_dl_read  = data[24];
	slot->bt_dl_write = data[25];
	slot->bt_dl_flow  = data[46];

	slot->bt_ul_read  = data[8];
	slot->bt_ul_write = data[9];
	slot->bt_ul_flow  = data[38];

	slot->mm_dl_read  = data[16];
	slot->mm_dl_write = data[17];
	slot->mm_dl_flow  = data[42];



// Firmware Debug Trace is 96 bytes large and dumped to D_DEBUG_FIFO_ADDR
// byte [4] : index of DMIC ATC RD ptr
// byte [5] : index of DMIC ATC WR ptr
// byte [10] : index of MM_UL ATC RD ptr
// byte [11] : index of MM_UL ATC WR ptr
// byte [12] : index of MM_UL2 ATC RD ptr
// byte [13] : index of MM_UL2 ATC WR ptr
// byte [20] : index of TONES_DL ATC RD ptr
// byte [21] : index of TONES_DL ATC WR ptr
// byte [22] : index of VIB_DL ATC RD ptr
// byte [23] : index of VIB_DL ATC WR ptr
// byte [28] : index of MM_EXT_OUT ATC RD ptr
// byte [29] : index of MM_EXT_OUT ATC WR ptr
// byte [30] : index of MM_EXT_IN ATC RD ptr
// byte [31] : index of MM_EXT_IN ATC WR ptr
// byte [32] : index of TDM_OUT ATC RD ptr
// byte [33] : index of TDM_OUT ATC WR ptr
// byte [34] : index of TDM_IN ATC RD ptr
// byte [35] : index of TDM_IN ATC WR ptr
// byte [36] : index of DMIC flow_counter
// byte [39] : index of MM_UL flow_counter
// byte [40] : index of MM_UL2 flow_counter
// byte [44] : index of TONES_DL flow_counter
// byte [45] : index of VIB_DL flow_counter
// byte [48] : index of MM_EXT_OUT flow_counter
// byte [49] : index of MM_EXT_IN flow_counter
// byte [50] : index of TDM_OUT flow_counter
// byte [51] : index of TDM_IN flow_counter
/*
	printf("Slot %x:\n", slot->slot);
	printf("\t McPDM DL R=%d W=%d Flow:%d\n", slot->mcpdm_dl_read, slot->mcpdm_dl_write, slot->mcpdm_dl_flow);
	printf("\t McPDM UL R=%d W=%d Flow:%d\n", slot->mcpdm_ul_read, slot->mcpdm_ul_write, slot->mcpdm_ul_flow);
	printf("\t VX DL R=%d W=%d Flow:%d\n", slot->vx_dl_read, slot->vx_dl_write, slot->vx_dl_flow);
	printf("\t VX UL R=%d W=%d Flow:%d\n", slot->vx_ul_read, slot->vx_ul_write, slot->vx_ul_flow);

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
	char filename[100];
	char trace[128];
	int port, i, ret = 0;
	int prev_slot = 0;
	int slot = 0;
	int count = 0;

	if (argc < 2) {
		help(argv[0]);
		return 1;
	}

	strcpy(filename, argv[1]);

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
			printf("===> Slot Gap in trace %d slot:%d(%d)\n", (slot - prev_slot), slot, count);
			count = 0;
		}

                /* In case Count = 0 we have issue inside the trace so do not analyze the first slot after */
		if (count != 0) {
			/* Analyse scheduler synchro slot */
			if (trc_slot.slot_update == 1)
				printf("Scheduler slot removal  <---- (%d)\n", slot);
			else if (trc_slot.slot_update == 3)
				printf("Scheduler slot addition <---- (%d)\n", slot);

			printf("\t McPDM DL R=%03d(%d) W=%03d(%d) Flow:%d\n", 
				trc_slot.mcpdm_dl_read, (int)((trc_slot.mcpdm_dl_read - prev_trc_slot.mcpdm_dl_read) + 120) % 120,
				trc_slot.mcpdm_dl_write, (int)((trc_slot.mcpdm_dl_write - prev_trc_slot.mcpdm_dl_write) + 120) % 120,
				trc_slot.mcpdm_dl_flow);

			printf("\t McPDM UL R=%03d(%d) W=%03d(%d) Flow:%d\n", 
				trc_slot.mcpdm_ul_read, (int)((trc_slot.mcpdm_ul_read - prev_trc_slot.mcpdm_ul_read) + 120) % 120,
				trc_slot.mcpdm_ul_write, (int)((trc_slot.mcpdm_ul_write - prev_trc_slot.mcpdm_ul_write) + 120) % 120,
				trc_slot.mcpdm_ul_flow);

			printf("\t VX DL R=%03d(%d) W=%03d(%d) Flow:%d\n", 
				trc_slot.vx_dl_read, (int)((trc_slot.vx_dl_read - prev_trc_slot.vx_dl_read) + 120) % 120,
				trc_slot.vx_dl_write, (int)((trc_slot.vx_dl_write - prev_trc_slot.vx_dl_write) + 120) % 120,
				trc_slot.vx_dl_flow);

			printf("\t VX UL R=%03d(%d) W=%03d(%d) Flow:%d\n", 
				trc_slot.vx_ul_read, (int)((trc_slot.vx_ul_read - prev_trc_slot.vx_ul_read) + 120) % 120,
				trc_slot.vx_ul_write, (int)((trc_slot.vx_ul_write - prev_trc_slot.vx_ul_write) + 120) % 120,
				trc_slot.vx_ul_flow);
#if 0
			printf("\t BT DL R=%03d(%d) W=%03d(%d) Flow:%d\n", 
				trc_slot.bt_dl_read, (int)((trc_slot.bt_dl_read - prev_trc_slot.bt_dl_read) + 120) % 120,
				trc_slot.bt_dl_write, (int)((trc_slot.bt_dl_write - prev_trc_slot.bt_dl_write) + 120) % 120,
				trc_slot.bt_dl_flow);

			printf("\t BT UL R=%03d(%d) W=%03d(%d) Flow:%d\n", 
				trc_slot.bt_ul_read, (int)((trc_slot.bt_ul_read - prev_trc_slot.bt_ul_read) + 120) % 120,
				trc_slot.bt_ul_write, (int)((trc_slot.bt_ul_write - prev_trc_slot.bt_ul_write) + 120) % 120,
				trc_slot.bt_ul_flow);

			printf("\t MM DL R=%03d(%d) W=%03d(%d) Flow:%d\n", 
				trc_slot.mm_dl_read, (int)((trc_slot.mm_dl_read - prev_trc_slot.mm_dl_read) + 120) % 120,
				trc_slot.mm_dl_write, (int)((trc_slot.mm_dl_write - prev_trc_slot.mm_dl_write) + 120) % 120,
				trc_slot.mm_dl_flow);
#endif
		}
		/* Save current slot to previous slow for next comparaison */		
		prev_slot = slot;
		memcpy(&prev_trc_slot, &trc_slot, sizeof(struct abe_slot_trc));
	}

out:
	fclose(fp);
	return ret;
}
