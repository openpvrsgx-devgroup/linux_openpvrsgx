/*
 * ALSA SoC OMAP ABE driver
 *
 * Author:          Laurent Le Faucheur <l-le-faucheur@ti.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */
#if !PC_SIMULATION
#include "abe_main.h"
#include "abe_test.h"
void main (void)
{
        abe_dma_t dma_sink, dma_source;
        abe_data_format_t format;
        abe_uint32 base_address;
        abe_auto_check_data_format_translation ();
        abe_reset_hal ();
        abe_check_opp ();
        abe_check_dma ();
        /*
            To be added here :
            Device driver initialization:
                McPDM_DL : threshold=1, 6 slots activated
                DMIC : threshold=1, 6 microphones activated
                McPDM_UL : threshold=1, two microphones activated
        */
        /*  MM_DL INIT
            connect a DMA channel to MM_DL port (ATC FIFO)
		format.f = 48000;
		format.samp_format = STEREO_MSB;
            abe_connect_cbpr_dmareq_port (MM_DL_PORT, &format, ABE_CBPR0_IDX, &dma_sink);
            connect a Ping-Pong SDMA protocol to MM_DL port with Ping-Pong 576 stereo samples
		format.f = 48000;
		format.samp_format = STEREO_MSB;
            abe_connect_dmareq_ping_pong_port (MM_DL_PORT, &format, ABE_CBPR0_IDX, (576 * 4), &dma_sink);
            connect a Ping-Pong cache-flush protocol to MM_DL port with 50Hz (20ms) rate
        */
            abe_add_subroutine (&abe_irq_pingpong_player_id,
(abe_subroutine2) abe_default_irq_pingpong_player, SUB_0_PARAM, (abe_uint32*)0 );
            format.f = 48000;
format.samp_format = STEREO_MSB;
            /* ping-pong access to MM_DL at 48kHz Mono with 20ms packet sizes */
            #define N_SAMPLES ((int)(48000 * 0.020))
            abe_connect_irq_ping_pong_port (MM_DL_PORT, &format, abe_irq_pingpong_player_id,
N_SAMPLES, &base_address, PING_PONG_WITH_MCU_IRQ);
        /*  VX_DL INIT
            connect a DMA channel to VX_DL port (ATC FIFO)
        */
	format.f = 8000;
	format.samp_format = MONO_MSB;
        abe_connect_cbpr_dmareq_port (VX_DL_PORT, &format, ABE_CBPR1_IDX, &dma_sink);
        /*  VX_UL INIT
            connect a DMA channel to VX_UL port (ATC FIFO)
        */
	format.f = 8000;
	format.samp_format = MONO_MSB;
        abe_connect_cbpr_dmareq_port (VX_UL_PORT, &format, ABE_CBPR2_IDX, &dma_source);
        /* make the AE waking event to be the McPDM DMA requests */
        abe_write_event_generator (EVENT_MCPDM);
        abe_enable_data_transfer (MM_DL_PORT );
        abe_enable_data_transfer (VX_DL_PORT );
        abe_enable_data_transfer (VX_UL_PORT );
        abe_enable_data_transfer (PDM_UL_PORT);
        abe_enable_data_transfer (PDM_DL1_PORT);
}
#endif
/* ========================================================================== */
/**
* @fn ABE_AUTO_CHECK_DATA_FORMAT_TRANSLATION()
*/
/* ========================================================================= */
void abe_auto_check_data_format_translation (void)
{
#if 0
        abe_float test, f_decibel, f_linear, max_error;
        abe_uint32 data_mem;
#define C_20LOG2 ((abe_float)6.020599913)
	for (max_error = 0, f_decibel = -70; f_decibel < 30; f_decibel += 0.1) {
            f_linear = 0;
            abe_translate_gain_format (DECIBELS_TO_LINABE, f_decibel, &f_linear);
            abe_translate_to_xmem_format (ABE_CMEM, f_linear, &data_mem);
            abe_translate_to_xmem_format (ABE_SMEM, f_linear, &data_mem);
            abe_translate_to_xmem_format (ABE_DMEM, f_linear, &data_mem);
            test = (abe_float) pow (2, f_decibel/C_20LOG2);
            if ( (absolute(f_linear - test) / 1) > max_error)
                max_error = (absolute(f_linear - test) / 1);
            abe_translate_gain_format (LINABE_TO_DECIBELS, f_linear, &f_decibel);
            test = (abe_float) (C_20LOG2 * log(f_linear) / log(2));
            if ( (absolute(f_decibel - test) / 1) > max_error)
                max_error = (absolute(f_decibel - test) / 1);
            /* the observed max gain error is 0.0001 decibel  (0.002%) */
        }
	#if 0
	{
	FILE *outf;
	outf = fopen("buff.txt", "wt");
		for (f_decibel = -120; f_decibel < 31; f_decibel += 1) {
            f_linear = 0;
            abe_translate_gain_format (DECIBELS_TO_LINABE, f_decibel, &f_linear);
            abe_translate_to_xmem_format (ABE_CMEM, f_linear, &data_mem);
	    fprintf (outf, "\n 0x%08lX,  /* CMEM coding of %4d dB */", data_mem, (short)(f_decibel));
        }
	fclose (outf);
	}
	#endif
	#if 1
	{
	FILE *outf;
	outf = fopen("buff.txt", "wt");
		for (f_decibel = -120; f_decibel < 31; f_decibel += 1) {
            //f_linear = 0;
            //abe_translate_gain_format (DECIBELS_TO_LINABE, f_decibel, &f_linear);
            //abe_translate_to_xmem_format (ABE_SMEM, f_linear, &data_mem);
        fprintf (outf, "\n 0x%08lX,  /* SMEM coding of %4d dB */", (long)(0x40000L * pow(10, f_decibel/20.0)), (short)(f_decibel));
	    //fprintf (outf, "\n 0x%08lX,  /* SMEM coding of %4d dB */", data_mem, (short)(f_decibel));
        }
	fclose (outf);
	}
	#endif
#endif
}
#if 0
/* ========================================================================== */
/**
* @fn ABE_AUTO_CHECK_DATA_FORMAT_TRANSLATION()
*/
/* ========================================================================= */
void abe_check_opp (void)
{
#if PC_SIMULATION
        abe_use_case_id UC[] = {ABE_AUDIO_PLAYER_ON_HEADSET_OR_EARPHONE, ABE_RINGER_TONES, 0};
        abe_opp_t OPP;
        abe_hw_config_init_t CONFIG;
        abe_read_hardware_configuration (UC, &OPP,  &CONFIG); /* check HW config and OPP config */
#endif
}
/* ========================================================================== */
/**
* @fn ABE_AUTO_CHECK_DATA_FORMAT_TRANSLATION()
*/
/* ========================================================================= */
void abe_check_dma (void)
{
#if PC_SIMULATION
    abe_port_id id;
    abe_data_format_t format;
    abe_uint32 d;
    abe_dma_t dma_sink;
    /* check abe_read_port_address () */
    id = VX_UL_PORT;
    abe_read_port_address (id, &dma_sink);
	format.f = 8000;
	format.samp_format = STEREO_MSB;
    d = abe_dma_port_iter_factor (&format);
    d = abe_dma_port_iteration (&format);
    abe_connect_cbpr_dmareq_port (VX_DL_PORT, &format, ABE_CBPR1_IDX, &dma_sink);
    abe_read_port_address (id, &dma_sink);
#endif
}
/* ========================================================================== */
/**
* @fn ABE_CHECK_MIXERS_GAIN_UPDATE()
*/
/* ========================================================================= */
void abe_check_mixers_gain_update (void)
{
#if PC_SIMULATION
        abe_write_mixer (MIXDL1, GAIN_12dB,     RAMP_0MS,   MIX_DL1_INPUT_TONES);
        abe_write_mixer (MIXDL1, GAIN_12dB,     RAMP_1MS,   MIX_DL1_INPUT_VX_DL);
        abe_write_mixer (MIXDL1, GAIN_M12dB,    RAMP_2MS,   MIX_DL1_INPUT_MM_DL);
        abe_write_mixer (MIXDL1, GAIN_0dB,      RAMP_5MS,   MIX_DL1_INPUT_MM_UL2);
#endif
}
void abe_debug_and_non_regression (void)
{
#if PC_SIMULATION
	abe_uint32 i, j;
	abe_dma_t d;
	abe_set_ping_pong_buffer (MM_DL_PORT, 4);    //bug 23 & 24
	abe_read_next_ping_pong_buffer (MM_DL_PORT, &i, &j);
	abe_read_port_address (6, &d);
#endif
}
void abe_debug_check_mem (void)
{
#if 1
	abe_uint32 pmem[512], i;
	for (i = 0; i < 512; i++) {
		abe_block_copy (COPY_FROM_ABE_TO_HOST, ABE_PMEM, i *4 , &(pmem[i]),  4);
	}
	i = 0;
#endif
#if 0
	abe_uint32 cmem[512], i;
	for (i = 0; i < 512; i++) {
		abe_block_copy (COPY_FROM_ABE_TO_HOST, ABE_CMEM, i *4 , &(cmem[i]),  4);
	}
	i = 0;
#endif
#if 0
	static int flag;
	if (flag == 0) {
	abe_uint32 data[4] = {0x11111111L, 0x22222222L, 0x33333333L, 0x44444444L};
	abe_block_copy (COPY_FROM_HOST_TO_ABE, ABE_ATC,  0xf00, data, 8);   // @ = 0xF00/3  0x3C0 0X11111111
	abe_block_copy (COPY_FROM_HOST_TO_ABE, ABE_CMEM, 0x2000, data, 8);  // @ = 0x2000/4 0x800 0x111111
	abe_block_copy (COPY_FROM_HOST_TO_ABE, ABE_SMEM, 0x1100, data, 8);  // @ = 0x1100/8 0x220 0x2222221111111
	abe_block_copy (COPY_FROM_HOST_TO_ABE, ABE_DMEM, 0x5000, data, 8);  // @ = 0x5000(B) 0x1111
	flag = 0;
	}
#endif
}
#endif
