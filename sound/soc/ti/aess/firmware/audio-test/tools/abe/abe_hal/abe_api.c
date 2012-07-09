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
#include "abe_typedef.h"
#include "abe_initxxx_labels.h"
#include "abe_dbg.h"
/**
 * abe_reset_hal - reset the ABE/HAL
 * @rdev: regulator source
 * @constraints: constraints to apply
 *
 * Operations : reset the HAL by reloading the static variables and
 * default AESS registers.
 * Called after a PRCM cold-start reset of ABE
 */
abehal_status abe_reset_hal(void)
{
	u32 i;
	_log(id_reset_hal, 0, 0, 0);
	abe_dbg_output = TERMINAL_OUTPUT;
	abe_dbg_activity_log_write_pointer = 0;
	/* IRQ & DBG circular read pointer in DMEM */
	abe_irq_dbg_read_ptr = 0;
	/* PDM_DL enable/disable collisions */
	pdm_dl1_status = 0;
	pdm_dl2_status = 0;
	pdm_vib_status = 0;
	/* default = disable the mixer's adaptive gain control */
	abe_use_compensated_gain(0);
	/* reset the default gain values */
	for (i = 0; i < MAX_NBGAIN_CMEM; i++) {
		abe_muted_gains_indicator[i] = 0;
		abe_desired_gains_decibel[i] = (u32) GAIN_MUTE;
		abe_desired_gains_linear[i] = 0;
		abe_desired_ramp_delay_ms[i] = 0;
		abe_muted_gains_decibel[i] = (u32) GAIN_TOOLOW;
	}
	/* set debug mask to "enable all traces" */
	abe_dbg_mask = (abe_dbg_t) (0);
	abe_hw_configuration();
	return 0;
}
EXPORT_SYMBOL(abe_reset_hal);
/**
 * abe_load_fw_param - Load ABE Firmware memories
 * @PMEM: Pointer of Program memory data
 * @PMEM_SIZE: Size of PMEM data
 * @CMEM: Pointer of Coeffients memory data
 * @CMEM_SIZE: Size of CMEM data
 * @SMEM: Pointer of Sample memory data
 * @SMEM_SIZE: Size of SMEM data
 * @DMEM: Pointer of Data memory data
 * @DMEM_SIZE: Size of DMEM data
 *
 */
abehal_status abe_load_fw_param(u32 *ABE_FW)
{
	static u32 warm_boot;
	u32 event_gen;
	u32 pmem_size, dmem_size, smem_size, cmem_size;
	u32 *pmem_ptr, *dmem_ptr, *smem_ptr, *cmem_ptr, *fw_ptr;
	_log(id_load_fw_param, 0, 0, 0);
#if PC_SIMULATION
	/* the code is loaded from the Checkers */
#else
#define ABE_FW_OFFSET 5
	fw_ptr = ABE_FW;
	abe_firmware_version_number = *fw_ptr++;
	pmem_size = *fw_ptr++;
	cmem_size = *fw_ptr++;
	dmem_size = *fw_ptr++;
	smem_size = *fw_ptr++;
	pmem_ptr = fw_ptr;
	cmem_ptr = pmem_ptr + (pmem_size >> 2);
	dmem_ptr = cmem_ptr + (cmem_size >> 2);
	smem_ptr = dmem_ptr + (dmem_size >> 2);
	/* do not load PMEM */
	if (warm_boot) {
		/* Stop the event Generator */
		event_gen = 0;
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_ATC,
			       EVENT_GENERATOR_START, &event_gen, 4);
		/* Now we are sure the firmware is stalled */
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_CMEM, 0, cmem_ptr,
			       cmem_size);
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM, 0, smem_ptr,
			       smem_size);
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, 0, dmem_ptr,
			       dmem_size);
		/* Restore the event Generator status */
		event_gen = 1;
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_ATC,
			       EVENT_GENERATOR_START, &event_gen, 4);
	}
	else {
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_PMEM, 0, pmem_ptr,
			       pmem_size);
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_CMEM, 0, cmem_ptr,
			       cmem_size);
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM, 0, smem_ptr,
			       smem_size);
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, 0, dmem_ptr,
			       dmem_size);
	}
	warm_boot = 1;
#endif
	return 0;
}
EXPORT_SYMBOL(abe_load_fw_param);
/**
 * abe_reload_fw - Reload ABE Firmware after OFF mode
 */
abehal_status abe_reload_fw(u32 *ABE_FW)
{
	/* abe_load_fw_param((u32 *) abe_firmware_array);
	   with "warm_boot" forced to 0 */
	u32 event_gen;
	u32 pmem_size, dmem_size, smem_size, cmem_size;
	u32 *pmem_ptr, *dmem_ptr, *smem_ptr, *cmem_ptr, *fw_ptr;
	_log(id_load_fw_param, 0, 0, 1);
#if PC_SIMULATION
	/* the code is loaded from the Checkers */
#else
	fw_ptr = ABE_FW;
	abe_firmware_version_number = *fw_ptr++;
	pmem_size = *fw_ptr++;
	cmem_size = *fw_ptr++;
	dmem_size = *fw_ptr++;
	smem_size = *fw_ptr++;
	pmem_ptr = fw_ptr;
	cmem_ptr = pmem_ptr + (pmem_size >> 2);
	dmem_ptr = cmem_ptr + (cmem_size >> 2);
	smem_ptr = dmem_ptr + (dmem_size >> 2);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_PMEM, 0, pmem_ptr,
		       pmem_size);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_CMEM, 0, cmem_ptr,
		       cmem_size);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM, 0, smem_ptr,
		       smem_size);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, 0, dmem_ptr,
		       dmem_size);
#endif
	abe_build_scheduler_table();
	return 0;
}
EXPORT_SYMBOL(abe_reload_fw);
/**
 * abe_load_fw - Load ABE Firmware and initialize memories
 *
 */
abehal_status abe_load_fw(void)
{
	_log(id_load_fw, 0, 0, 0);
	abe_load_fw_param((u32 *) abe_firmware_array);
	abe_reset_all_ports();
	abe_build_scheduler_table();
	abe_reset_all_sequence();
	abe_select_main_port(PDM_DL_PORT);
	return 0;
}
EXPORT_SYMBOL(abe_load_fw);
/**
 * abe_read_hardware_configuration - Returns default
 *	HW periferals configuration
 * @u: use-case description list (pointer)
 * @o: opp mode (pointer)
 * @hw: pointer to the output HW structure
 *
 * Parameter :
 * U : use-case description list (pointer)
 * H : pointer to the output structure
 *
 * Operations :
 * Returns a structure with the HW thresholds compatible with
 *	the HAL/FW/AESS_ATC. Will be upgraded in FW06.
 */
abehal_status abe_read_hardware_configuration(u32 *u, u32 *o,
						abe_hw_config_init_t *hw)
{
	_log(id_read_hardware_configuration, (u32) u,
		(u32) u >> 8, (u32) u >> 16);
	abe_read_use_case_opp(u, o);
	/* 0: 96kHz 1:192kHz */
	hw->MCPDM_CTRL__DIV_SEL = 0;
	/* 0: no command in the FIFO,
	1: 6 data on each lines (with commands) */
	hw->MCPDM_CTRL__CMD_INT = 1;
	/* 0:MSB aligned 1:LSB aligned */
	hw->MCPDM_CTRL__PDMOUTFORMAT = 0;
	hw->MCPDM_CTRL__PDM_DN5_EN = 1;
	hw->MCPDM_CTRL__PDM_DN4_EN = 1;
	hw->MCPDM_CTRL__PDM_DN3_EN = 1;
	hw->MCPDM_CTRL__PDM_DN2_EN = 1;
	hw->MCPDM_CTRL__PDM_DN1_EN = 1;
	hw->MCPDM_CTRL__PDM_UP3_EN = 0;
	hw->MCPDM_CTRL__PDM_UP2_EN = 1;
	hw->MCPDM_CTRL__PDM_UP1_EN = 1;
	/* All the McPDM_DL FIFOs are enabled simultaneously */
	hw->MCPDM_FIFO_CTRL_DN__DN_TRESH = MCPDM_DL_ITER / 6;
	/* number of ATC access upon AMIC DMArequests, 2 the FIFOs channels
	   are enabled */
	hw->MCPDM_FIFO_CTRL_UP__UP_TRESH = MCPDM_UL_ITER / 2;
	/* 0:2.4MHz 1:3.84MHz */
	hw->DMIC_CTRL__DMIC_CLK_DIV = 0;
	/* 0:MSB aligned 1:LSB aligned */
	hw->DMIC_CTRL__DMICOUTFORMAT = 0;
	hw->DMIC_CTRL__DMIC_UP3_EN = 1;
	hw->DMIC_CTRL__DMIC_UP2_EN = 1;
	hw->DMIC_CTRL__DMIC_UP1_EN = 1;
	/* 1*(DMIC_UP1_EN+ 2+ 3)*2 OCP read access every 96/88.1 KHz. */
	hw->DMIC_FIFO_CTRL__DMIC_TRESH = DMIC_ITER / 6;
	/* MCBSP SPECIFICATION
	   RJUST = 00 Right justify data and zero fill MSBs in DRR[1,2]
	   RJUST = 01 Right justify data and sign extend it into the MSBs
	   in DRR[1,2]
	   RJUST = 10 Left justify data and zero fill LSBs in DRR[1,2]
	   MCBSPLP_RJUST_MODE_RIGHT_ZERO = 0x0,
	   MCBSPLP_RJUST_MODE_RIGHT_SIGN = 0x1,
	   MCBSPLP_RJUST_MODE_LEFT_ZERO = 0x2,
	   MCBSPLP_RJUST_MODE_MAX = MCBSPLP_RJUST_MODE_LEFT_ZERO
	 */
	hw->MCBSP_SPCR1_REG__RJUST = 2;
	/* 1=MONO, 2=STEREO, 3=TDM_3_CHANNELS, 4=TDM_4_CHANNELS, .... */
	hw->MCBSP_THRSH2_REG_REG__XTHRESHOLD = 1;
	/* 1=MONO, 2=STEREO, 3=TDM_3_CHANNELS, 4=TDM_4_CHANNELS, .... */
	hw->MCBSP_THRSH1_REG_REG__RTHRESHOLD = 1;
	/* Slimbus IP FIFO thresholds */
	hw->SLIMBUS_DCT_FIFO_SETUP_REG__SB_THRESHOLD = 1;
	/* 2050 gives about 96kHz */
	hw->AESS_EVENT_GENERATOR_COUNTER__COUNTER_VALUE =
		EVENT_GENERATOR_COUNTER_DEFAULT;
	/* 0: DMAreq, 1:Counter */
	hw->AESS_EVENT_SOURCE_SELECTION__SELECTION = 1;
	/* 5bits DMAreq selection */
	hw->AESS_AUDIO_ENGINE_SCHEDULER__DMA_REQ_SELECTION =
		ABE_ATC_MCPDMDL_DMA_REQ;
	/* THE famous EVENT timer ! */
	hw->HAL_EVENT_SELECTION = EVENT_TIMER;
	return 0;
}
EXPORT_SYMBOL(abe_read_hardware_configuration);
/**
 * abe_irq_processing - Process ABE interrupt
 *
 * This subroutine is call upon reception of "MA_IRQ_99 ABE_MPU_IRQ" Audio
 * back-end interrupt. This subroutine will check the ATC Hrdware, the
 * IRQ_FIFO from the AE and act accordingly. Some IRQ source are originated
 * for the delivery of "end of time sequenced tasks" notifications, some are
 * originated from the Ping-Pong protocols, some are generated from
 * the embedded debugger when the firmware stops on programmable break-points,
 * etc …
 */
abehal_status abe_irq_processing(void)
{
	u32 abe_irq_dbg_write_ptr, i, cmem_src, sm_cm;
	abe_irq_data_t IRQ_data;
#define IrqFiFoMask ((D_McuIrqFifo_sizeof >> 2) - 1)
	_log(id_irq_processing, 0, 0, 0);
	/* extract the write pointer index from CMEM memory (INITPTR format) */
	/* CMEM address of the write pointer in bytes */
	cmem_src = MCU_IRQ_FIFO_ptr_labelID << 2;
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_CMEM, cmem_src,
			&sm_cm, sizeof(abe_irq_dbg_write_ptr));
	/* AESS left-pointer index located on MSBs */
	abe_irq_dbg_write_ptr = sm_cm >> 16;
	abe_irq_dbg_write_ptr &= 0xFF;
	/* loop on the IRQ FIFO content */
	for (i = 0; i < D_McuIrqFifo_sizeof; i++) {
		/* stop when the FIFO is empty */
		if (abe_irq_dbg_write_ptr == abe_irq_dbg_read_ptr)
			break;
		/* read the IRQ/DBG FIFO */
		abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM,
			       (D_McuIrqFifo_ADDR +
				(abe_irq_dbg_read_ptr << 2)),
			       (u32 *) &IRQ_data, sizeof(IRQ_data));
		abe_irq_dbg_read_ptr = (abe_irq_dbg_read_ptr + 1) & IrqFiFoMask;
		/* select the source of the interrupt */
		switch (IRQ_data.tag) {
		case IRQtag_APS:
			_log(id_irq_processing, IRQ_data.data, 0, 1);
			abe_irq_aps(IRQ_data.data);
			break;
		case IRQtag_PP:
			_log(id_irq_processing, 0, 0, 2);
			abe_irq_ping_pong();
			break;
		case IRQtag_COUNT:
			_log(id_irq_processing, IRQ_data.data, 0, 3);
			abe_irq_check_for_sequences(IRQ_data.data);
			break;
		default:
			break;
		}
	}
	abe_monitoring();
	return 0;
}
EXPORT_SYMBOL(abe_irq_processing);
/**
 * abe_clear_irq - clear ABE interrupt
 *
 * This subroutine is call to clear MCU Irq
 */
abehal_status abe_clear_irq(void)
{
	u32 clear_abe_irq = 1;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_ATC, ABE_MCU_IRQSTATUS,
			&clear_abe_irq, 4);
	return 0;
}
EXPORT_SYMBOL(abe_clear_irq);
/**
 * abe_select_main_port - Select stynchronization port for Event generator.
 * @id: audio port name
 *
 * tells the FW which is the reference stream for adjusting
 * the processing on 23/24/25 slots
 */
abehal_status abe_select_main_port(u32 id)
{
	u32 selection;
	_log(id_select_main_port, id, 0, 0);
	/* flow control */
	selection = D_IOdescr_ADDR + id * sizeof(ABE_SIODescriptor) +
		flow_counter_;
	/* when the main port is a sink port from AESS point of view
	   the sign the firmware task analysis must be changed  */
	selection &= 0xFFFFL;
	if (abe_port[id].protocol.direction == ABE_ATC_DIRECTION_IN)
		selection |= 0x80000;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, D_Slot23_ctrl_ADDR,
			&selection, 4);
	return 0;
}
/**
 * abe_write_event_generator - Selects event generator source
 * @e: Event Generation Counter, McPDM, DMIC or default.
 *
 * Loads the AESS event generator hardware source.
 * Loads the firmware parameters accordingly.
 * Indicates to the FW which data stream is the most important to preserve
 * in case all the streams are asynchronous.
 * If the parameter is "default", then HAL decides which Event source
 * is the best appropriate based on the opened ports.
 *
 * When neither the DMIC and the McPDM are activated, the AE will have
 * its EVENT generator programmed with the EVENT_COUNTER.
 * The event counter will be tuned in order to deliver a pulse frequency higher
 * than 96 kHz.
 * The DPLL output at 100% OPP is MCLK = (32768kHz x6000) = 196.608kHz
 * The ratio is (MCLK/96000)+(1<<1) = 2050
 * (1<<1) in order to have the same speed at 50% and 100% OPP
 * (only 15 MSB bits are used at OPP50%)
 */
abehal_status abe_write_event_generator(u32 e)
{
	u32 event, selection, counter, start;
	_log(id_write_event_generator, e, 0, 0);
	counter = EVENT_GENERATOR_COUNTER_DEFAULT;
	start = EVENT_GENERATOR_ON;
	abe_current_event_id = e;
	switch (e) {
	case EVENT_TIMER:
		selection = EVENT_SOURCE_COUNTER;
		event = 0;
		break;
	case EVENT_44100:
		selection = EVENT_SOURCE_COUNTER;
		event = 0;
		counter = EVENT_GENERATOR_COUNTER_44100;
		break;
	default:
		abe_dbg_param |= ERR_API;
		abe_dbg_error_log(ABE_BLOCK_COPY_ERR);
	}
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_ATC,
		       EVENT_GENERATOR_COUNTER, &counter, 4);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_ATC,
		       EVENT_SOURCE_SELECTION, &selection, 4);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_ATC,
		       EVENT_GENERATOR_START, &start, 4);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_ATC,
		       AUDIO_ENGINE_SCHEDULER, &event, 4);
	return 0;
}
EXPORT_SYMBOL(abe_write_event_generator);
/**
 * abe_stop_event_generator - Stops event generator source
 *
 * Stop the event genrator of AESS. No more event will be send to AESS engine.
 * Upper layer must wait 1/96kHz to be sure that engine reaches
 * the IDLE instruction.
 */
abehal_status abe_stop_event_generator(void)
{
	u32 event_gen;
	/* Stop the event Generator */
	event_gen = 0;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_ATC,
			EVENT_GENERATOR_START, &event_gen, 4);
	return 0;
}
EXPORT_SYMBOL(abe_stop_event_generator);
/**
 * abe_read_use_case_opp() - description for void abe_read_use_case_opp().
 *
 * returns the expected min OPP for a given use_case list
 */
abehal_status abe_read_use_case_opp(u32 *u, u32 *o)
{
	u32 opp, i;
	u32 *ptr = u;
#define MAX_READ_USE_CASE_OPP 10
#define OPP_25 1
#define OPP_50 2
#define OPP_100 4
	_log(id_read_use_case_opp, (u32) u, (u32) u >> 8, (u32) u >> 16);
	opp = i = 0;
	do {
		/* check for pointer errors */
		if (i > MAX_READ_USE_CASE_OPP) {
			abe_dbg_param |= ERR_API;
			abe_dbg_error_log(ABE_READ_USE_CASE_OPP_ERR);
			break;
		}
		/* check for end_of_list */
		if (*ptr <= 0)
			break;
		/* OPP selection based on current firmware implementation */
		switch (*ptr) {
		case ABE_AUDIO_PLAYER_ON_HEADSET_OR_EARPHONE:
			opp |= OPP_25;
			break;
		case ABE_DRIFT_MANAGEMENT_FOR_AUDIO_PLAYER:
			opp |= OPP_100;
			break;
		case ABE_DRIFT_MANAGEMENT_FOR_VOICE_CALL:
			opp |= OPP_100;
			break;
		case ABE_VOICE_CALL_ON_HEADSET_OR_EARPHONE_OR_BT:
			opp |= OPP_50;
			break;
		case ABE_MULTIMEDIA_AUDIO_RECORDER:
			opp |= OPP_50;
			break;
		case ABE_VIBRATOR_OR_HAPTICS:
			opp |= OPP_100;
			break;
		case ABE_VOICE_CALL_ON_HANDS_FREE_SPEAKER:
			opp |= OPP_100;
			break;
		case ABE_RINGER_TONES:
			opp |= OPP_100;
			break;
		case ABE_VOICE_CALL_WITH_EARPHONE_ACTIVE_NOISE_CANCELLER:
			opp |= OPP_100;
			break;
		default:
			break;
		}
		i++;
		ptr++;
	} while (*ptr != 0);
	if (opp & OPP_100)
		*o = ABE_OPP100;
	else if (opp & OPP_50)
		*o = ABE_OPP50;
	else
		*o = ABE_OPP25;
	return 0;
}
EXPORT_SYMBOL(abe_read_use_case_opp);
/**
 * abe_set_opp_processing - Set OPP mode for ABE Firmware
 * @opp: OOPP mode
 *
 * New processing network and OPP:
 * 0: Ultra Lowest power consumption audio player (no post-processing, no mixer)
 * 1: OPP 25% (simple multimedia features, including low-power player)
 * 2: OPP 50% (multimedia and voice calls)
 * 3: OPP100% ( multimedia complex use-cases)
 *
 * Rearranges the FW task network to the corresponding OPP list of features.
 * The corresponding AE ports are supposed to be set/reset accordingly before
 * this switch.
 *
 */
abehal_status abe_set_opp_processing(u32 opp)
{
	u32 dOppMode32, sio_desc_address, sio_desc_address_pp;
	_lock_enter;
	_log(id_set_opp_processing, opp, 0, 0);
	switch (opp) {
	case ABE_OPP25:
		/* OPP25% */
		dOppMode32 = DOPPMODE32_OPP25;
		break;
	case ABE_OPP50:
		/* OPP50% */
		dOppMode32 = DOPPMODE32_OPP50;
		break;
	default:
		abe_dbg_param |= ERR_API;
		abe_dbg_error_log(ABE_BLOCK_COPY_ERR);
	case ABE_OPP100:
		/* OPP100% */
		dOppMode32 = DOPPMODE32_OPP100;
		break;
	}
	/* Write Multiframe inside DMEM */
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM,
		       D_maxTaskBytesInSlot_ADDR, &dOppMode32, sizeof(u32));
	sio_desc_address = dmem_port_descriptors + (MM_EXT_IN_PORT *
						sizeof(ABE_SIODescriptor));
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM, sio_desc_address,
			(u32 *) &sio_desc, sizeof(sio_desc));
	if (dOppMode32 == DOPPMODE32_OPP100) {
		/* ASRC input buffer, size 40 */
		sio_desc.smem_addr1 = smem_mm_ext_in_opp100;
		/* Init MM_EXT_IN ASRC and enable its adaptation */
		abe_init_asrc_mm_ext_in(250);
	}
	else
		/* at OPP 50 or without ASRC */
		sio_desc.smem_addr1 = smem_mm_ext_in_opp50;		
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, sio_desc_address,
			(u32 *) &sio_desc, sizeof(sio_desc));
	
	sio_desc_address = dmem_port_descriptors + (BT_VX_UL_PORT *
						sizeof(ABE_SIODescriptor));
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM, sio_desc_address,
			(u32 *) &sio_desc, sizeof(sio_desc));
	if (abe_port[BT_VX_UL_PORT].format.f == 8000) {
		if (dOppMode32 == DOPPMODE32_OPP100)
				/* ASRC input buffer, size 40 */
			sio_desc.smem_addr1 = smem_bt_vx_ul_opp100;
		else
				/* at OPP 50 without ASRC */
			sio_desc.smem_addr1 = BT_UL_8k_labelID;
	} else {
		if (dOppMode32 == DOPPMODE32_OPP100)
				/* ASRC input buffer, size 40 */
			sio_desc.smem_addr1 = smem_bt_vx_ul_opp100;
		else
				/* at OPP 50 without ASRC */
			sio_desc.smem_addr1 = BT_UL_16k_labelID;
	}
		
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, sio_desc_address,
		       (u32 *) &sio_desc, sizeof(sio_desc));
	
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM, D_multiFrame_ADDR,
				(u32 *) MultiFrame, sizeof(MultiFrame));
	sio_desc_address = dmem_port_descriptors + (BT_VX_DL_PORT *
						sizeof(ABE_SIODescriptor));
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM, sio_desc_address,
		       (u32 *) &sio_desc, sizeof(sio_desc));
#define ABE_TASK_ID(ID) (D_tasksList_ADDR + sizeof(ABE_STask)*(ID))
#define TASK_BT_DL_48_8_SLT 14
#define TASK_BT_DL_48_8_IDX 4
 	if (abe_port[BT_VX_DL_PORT].format.f == 8000) {
		if (dOppMode32 == DOPPMODE32_OPP100) {
			MultiFrame[TASK_BT_DL_48_8_SLT][TASK_BT_DL_48_8_IDX] =
				ABE_TASK_ID(C_ABE_FW_TASK_BT_DL_48_8_OPP100);
			sio_desc.smem_addr1 = BT_DL_8k_opp100_labelID;
		} else {
			MultiFrame[TASK_BT_DL_48_8_SLT][TASK_BT_DL_48_8_IDX] =
				ABE_TASK_ID(C_ABE_FW_TASK_BT_DL_48_8_FIR);
			sio_desc.smem_addr1 = BT_DL_8k_labelID;
		}
	} else {
		if (dOppMode32 == DOPPMODE32_OPP100) {
			MultiFrame[TASK_BT_DL_48_8_SLT][TASK_BT_DL_48_8_IDX] =
				ABE_TASK_ID(C_ABE_FW_TASK_BT_DL_48_16_OPP100);
			sio_desc.smem_addr1 = BT_DL_16k_opp100_labelID;
		} else {
			MultiFrame[TASK_BT_DL_48_8_SLT][TASK_BT_DL_48_8_IDX] =
				ABE_TASK_ID(C_ABE_FW_TASK_BT_DL_48_16);
			sio_desc.smem_addr1 = BT_DL_16k_labelID;
		}
	}
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, D_multiFrame_ADDR,
				(u32 *) MultiFrame, sizeof(MultiFrame));
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, sio_desc_address,
		       (u32 *) &sio_desc, sizeof(sio_desc));
	
	if (dOppMode32 == DOPPMODE32_OPP100) {
				/* Init BT_VX_UL ASRC and enable its adaptation */
			abe_init_asrc_bt_ul(250);
				/* Init BT_VX_DL ASRC and enable its adaptation */
			abe_init_asrc_bt_dl(-250);
	}
	
	return 0;	
}
EXPORT_SYMBOL(abe_set_opp_processing);
/**
 * abe_set_ping_pong_buffer
 * @port: ABE port ID
 * @n_bytes: Size of Ping/Pong buffer
 *
 * Updates the next ping-pong buffer with "size" bytes copied from the
 * host processor. This API notifies the FW that the data transfer is done.
 */
abehal_status abe_set_ping_pong_buffer(u32 port, u32 n_bytes)
{
	u32 sio_pp_desc_address, struct_offset, n_samples, datasize,
		base_and_size, *src;
	_log(id_set_ping_pong_buffer, port, n_bytes, n_bytes >> 8);
	/* ping_pong is only supported on MM_DL */
	if (port != MM_DL_PORT) {
		abe_dbg_param |= ERR_API;
		abe_dbg_error_log(ABE_PARAMETER_ERROR);
	}
	/* translates the number of bytes in samples */
	/* data size in DMEM words */
	datasize = abe_dma_port_iter_factor(&((abe_port[port]).format));
	/* data size in bytes */
	datasize = datasize << 2;
	n_samples = n_bytes / datasize;
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM, D_PingPongDesc_ADDR,
			(u32 *) &desc_pp, sizeof(desc_pp));
	/*
	 * read the port SIO descriptor and extract the current pointer
	 * address after reading the counter
	 */
	if ((desc_pp.counter & 0x1) == 0) {
		struct_offset = (u32) &(desc_pp.nextbuff0_BaseAddr) -
			(u32) &(desc_pp);
		base_and_size = desc_pp.nextbuff0_BaseAddr;
	}
	else {
		struct_offset = (u32) &(desc_pp.nextbuff1_BaseAddr) -
			(u32) &(desc_pp);
		base_and_size = desc_pp.nextbuff1_BaseAddr;
	}
	base_and_size = (base_and_size & 0xFFFFL) + (n_samples << 16);
	sio_pp_desc_address = D_PingPongDesc_ADDR + struct_offset;
	src = &base_and_size;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, sio_pp_desc_address,
			(u32 *) &base_and_size, sizeof(u32));
	return 0;
}
EXPORT_SYMBOL(abe_set_ping_pong_buffer);
/**
 * abe_read_next_ping_pong_buffer
 * @port: ABE portID
 * @p: Next buffer address (pointer)
 * @n: Next buffer size (pointer)
 *
 * Tell the next base address of the next ping_pong Buffer and its size
 */
abehal_status abe_read_next_ping_pong_buffer(u32 port, u32 *p, u32 *n)
{
	u32 sio_pp_desc_address;
	_log(id_read_next_ping_pong_buffer, port, 0, 0);
	/* ping_pong is only supported on MM_DL */
	if (port != MM_DL_PORT) {
		abe_dbg_param |= ERR_API;
		abe_dbg_error_log(ABE_PARAMETER_ERROR);
	}
	/* read the port SIO descriptor and extract the current pointer
	   address after reading the counter */
	sio_pp_desc_address = D_PingPongDesc_ADDR;
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM, sio_pp_desc_address,
			(u32 *) &desc_pp, sizeof(ABE_SPingPongDescriptor));
	if ((desc_pp.counter & 0x1) == 0) {
		_log(id_read_next_ping_pong_buffer, port, 0, 0);
		*p = desc_pp.nextbuff0_BaseAddr;
	}
	else {
		_log(id_read_next_ping_pong_buffer, port, 1, 0);
		*p = desc_pp.nextbuff1_BaseAddr;
	}
	/* translates the number of samples in bytes */
	*n = abe_size_pingpong;
	return 0;
}
EXPORT_SYMBOL(abe_read_next_ping_pong_buffer);
/**
 * abe_init_ping_pong_buffer
 * @id: ABE port ID
 * @size_bytes:size of the ping pong
 * @n_buffers:number of buffers (2 = ping/pong)
 * @p:returned address of the ping-pong list of base addresses
 *	(byte offset from DMEM start)
 *
 * Computes the base address of the ping_pong buffers
 */
abehal_status abe_init_ping_pong_buffer(u32 id, u32 size_bytes, u32 n_buffers,
					u32 *p)
{
	u32 i, dmem_addr;
	_log(id_init_ping_pong_buffer, id, size_bytes, n_buffers);
	/* ping_pong is supported in 2 buffers configuration right now but FW
	   is ready for ping/pong/pung/pang... */
	if (id != MM_DL_PORT || n_buffers > MAX_PINGPONG_BUFFERS) {
		abe_dbg_param |= ERR_API;
		abe_dbg_error_log(ABE_PARAMETER_ERROR);
	}
	for (i = 0; i < n_buffers; i++) {
		dmem_addr = dmem_ping_pong_buffer + (i * size_bytes);
		/* base addresses of the ping pong buffers in U8 unit */
		abe_base_address_pingpong[i] = dmem_addr;
	}
	/* global data */
	abe_size_pingpong = size_bytes;
	*p = (u32) dmem_ping_pong_buffer;
	return 0;
}
EXPORT_SYMBOL(abe_init_ping_pong_buffer);
/**
 * abe_read_offset_from_ping_buffer
 * @id: ABE port ID
 * @n:  returned address of the offset
 *	from the ping buffer start address (in samples)
 *
 * Computes the current firmware ping pong read pointer location,
 * expressed in samples, as the offset from the start address of ping buffer.
 */
abehal_status abe_read_offset_from_ping_buffer(u32 id, u32 *n)
{
	u32 sio_pp_desc_address;
	/* ping_pong is only supported on MM_DL */
	if (MM_DL_PORT != id) {
		abe_dbg_param |= ERR_API;
		abe_dbg_error_log(ABE_PARAMETER_ERROR);
	}
	else {
		/* read the port SIO ping pong descriptor */
		sio_pp_desc_address = D_PingPongDesc_ADDR;
		abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM,
			       sio_pp_desc_address, (u32 *) &desc_pp,
			       sizeof(ABE_SPingPongDescriptor));
		/* extract the current ping pong buffer read pointer based on
		   the value of the counter */
		if ((desc_pp.counter & 0x1) == 0) {
			/* the next is buffer0, hence the current is buffer1 */
			if ((abe_port[MM_DL_PORT].format.samp_format ==
			     MONO_MSB)
			    | (abe_port[MM_DL_PORT].format.samp_format ==
			       MONO_RSHIFTED_16)
			    | (abe_port[MM_DL_PORT].format.samp_format ==
			       STEREO_16_16)) {
				*n = abe_size_pingpong / 4 +
					desc_pp.nextbuff1_Samples -
					desc_pp.workbuff_Samples;
			}
			else if ((abe_port[MM_DL_PORT].format.samp_format ==
				    STEREO_MSB)
				   | (abe_port[MM_DL_PORT].format.samp_format ==
				      STEREO_RSHIFTED_16)) {
				*n = abe_size_pingpong / 8 +
					desc_pp.nextbuff1_Samples -
					desc_pp.workbuff_Samples;
			}
			else {
				abe_dbg_param |= ERR_API;
				abe_dbg_error_log(ABE_PARAMETER_ERROR);
			}
		}
		else {
			/* the next is buffer1, hence the current is buffer0 */
			*n = desc_pp.nextbuff0_Samples -
				desc_pp.workbuff_Samples;
		}
	}
	return 0;
}
EXPORT_SYMBOL(abe_read_offset_from_ping_buffer);
/**
 * abe_plug_subroutine
 * @id: returned sequence index after plugging a new subroutine
 * @f: subroutine address to be inserted
 * @n: number of parameters of this subroutine
 * @params: pointer on parameters
 *
 * register a list of subroutines for call-back purpose
 */
abehal_status abe_plug_subroutine(u32 *id, abe_subroutine2 f, u32 n,
				  u32 *params)
{
	_log(id_plug_subroutine, (u32) (*id), (u32) f, n);
	abe_add_subroutine(id, (abe_subroutine2) f, n, (u32 *) params);
	return 0;
}
EXPORT_SYMBOL(abe_plug_subroutine);
/**
 * abe_set_sequence_time_accuracy
 * @fast: fast counter
 * @slow: slow counter
 *
 */
abehal_status abe_set_sequence_time_accuracy(u32 fast, u32 slow)
{
	u32 data;
	_log(id_set_sequence_time_accuracy, fast, slow, 0);
	data = minimum(MAX_UINT16, fast / FW_SCHED_LOOP_FREQ_DIV1000);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, D_fastCounter_ADDR,
		       &data, sizeof(data));
	data = minimum(MAX_UINT16, slow / FW_SCHED_LOOP_FREQ_DIV1000);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, D_slowCounter_ADDR,
		       &data, sizeof(data));
	return 0;
}
EXPORT_SYMBOL(abe_set_sequence_time_accuracy);
/**
 * abe_reset_port
 * @id: ABE port ID
 *
 * stop the port activity and reload default parameters on the associated
 * processing features.
 * Clears the internal AE buffers.
 */
abehal_status abe_reset_port(u32 id)
{
	_log(id_reset_port, id, 0, 0);
	abe_port[id] = ((abe_port_t *) abe_port_init)[id];
	return 0;
}
EXPORT_SYMBOL(abe_reset_port);
/**
 * abe_read_remaining_data
 * @id:	ABE port_ID
 * @n: size pointer to the remaining number of 32bits words
 *
 * computes the remaining amount of data in the buffer.
 */
abehal_status abe_read_remaining_data(u32 port, u32 *n)
{
	u32 sio_pp_desc_address;
	_log(id_read_remaining_data, port, 0, 0);
	/*
	 * read the port SIO descriptor and extract the
	 * current pointer address after reading the counter
	 */
	sio_pp_desc_address = D_PingPongDesc_ADDR;
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM, sio_pp_desc_address,
			(u32 *) &desc_pp, sizeof(ABE_SPingPongDescriptor));
	*n = desc_pp.workbuff_Samples;
	return 0;
}
EXPORT_SYMBOL(abe_read_remaining_data);
/**
 * abe_disable_data_transfer
 * @id: ABE port id
 *
 * disables the ATC descriptor and stop IO/port activities
 * disable the IO task (@f = 0)
 * clear ATC DMEM buffer, ATC enabled
 */
abehal_status abe_disable_data_transfer(u32 id)
{
	abe_port_protocol_t *protocol;
	_log(id_disable_data_transfer, id, 0, 0);
	/* there is only one PDM_DL physical port shared
	   with DL1/DL2/VIB. Here is a check for the need to stop
	   PDM_DL if some activity is already on */
	if (id == PDM_DL1_PORT) {
		pdm_dl1_status = 0;
		if (pdm_dl2_status || pdm_vib_status)
			return 0;
		else
			id = PDM_DL_PORT;
	}
	if (id == PDM_DL2_PORT) {
		pdm_dl2_status = 0;
		if (pdm_dl1_status || pdm_vib_status)
			return 0;
		else
			id = PDM_DL_PORT;
	}
	if (id == PDM_VIB_PORT) {
		pdm_vib_status = 0;
		if (pdm_dl1_status || pdm_dl2_status)
			return 0;
		else
			id = PDM_DL_PORT;
	}
	/* MM_DL managed in ping-pong */
	if (id == MM_DL_PORT) {
		protocol = &(abe_port[MM_DL_PORT].protocol);
		if (protocol->protocol_switch == PINGPONG_PORT_PROT)
			abe_disable_pp_io_task(MM_DL_PORT);
	}
	/* local host variable status= "port is running" */
	abe_port[id].status = OMAP_ABE_PORT_ACTIVITY_IDLE;
	/* disable DMA requests */
	abe_disable_dma_request(id);
	/* disable ATC transfers */
	abe_init_atc(id);
	abe_clean_temporary_buffers(id);
	/* select the main port based on the desactivation of this port */
	abe_decide_main_port();
	return 0;
}
EXPORT_SYMBOL(abe_disable_data_transfer);
/**
 * abe_enable_data_transfer
 * @ip: ABE port id
 *
 * enables the ATC descriptor
 * reset ATC pointers
 * enable the IO task (@f <> 0)
 */
abehal_status abe_enable_data_transfer(u32 id)
{
	abe_port_protocol_t *protocol;
	abe_data_format_t format;
	_log(id_enable_data_transfer, id, 0, 0);
	/* there is only one PDM_DL physical port shared
	   with DL1/DL2/VIB. Here is a check for the need to enable
	   PDM_DL when some activity is already on */
	if (id == PDM_DL1_PORT) {
		id = PDM_DL_PORT;
		if (pdm_dl1_status == 1)
			return 0;
		else
			pdm_dl1_status = 1;
	}
	if (id == PDM_DL2_PORT) {
		id = PDM_DL_PORT;
		if (pdm_dl2_status == 1)
			return 0;
		else
			pdm_dl2_status = 1;
	}
	if (id == PDM_VIB_PORT) {
		id = PDM_DL_PORT;
		if (pdm_vib_status == 1)
			return 0;
		else
			pdm_vib_status = 1;
	}
	abe_clean_temporary_buffers(id);
	if (id == PDM_UL_PORT) {
		/* initializes the ABE ATC descriptors in DMEM - MCPDM_UL */
		protocol = &(abe_port[PDM_UL_PORT].protocol);
		format = abe_port[PDM_UL_PORT].format;
		abe_init_atc(PDM_UL_PORT);
		abe_init_io_tasks(PDM_UL_PORT, &format, protocol);
	}
	if (id == PDM_DL_PORT) {
		/* initializes the ABE ATC descriptors in DMEM - MCPDM_DL */
		protocol = &(abe_port[PDM_DL_PORT].protocol);
		format = abe_port[PDM_DL_PORT].format;
		abe_init_atc(PDM_DL_PORT);
		abe_init_io_tasks(PDM_DL_PORT, &format, protocol);
	}
	/* MM_DL managed in ping-pong */
	if (id == MM_DL_PORT) {
		protocol = &(abe_port[MM_DL_PORT].protocol);
		if (protocol->protocol_switch == PINGPONG_PORT_PROT)
			abe_enable_pp_io_task(MM_DL_PORT);
	}
	if (id == DMIC_PORT) {
		/* one DMIC port enabled = all DMICs enabled,
		 * since there is a single DMIC path for all DMICs */
		protocol = &(abe_port[DMIC_PORT].protocol);
		format = abe_port[DMIC_PORT].format;
		abe_init_atc(DMIC_PORT);
		abe_init_io_tasks(DMIC_PORT, &format, protocol);
	}
	if (id == VX_UL_PORT) {
		protocol = &(abe_port[VX_UL_PORT].protocol);
		if (abe_port[VX_DL_PORT].status == OMAP_ABE_PORT_ACTIVITY_RUNNING) {
		/* VX_DL port already started, hence no need to initialize ASRC */
		} else {
			/* Init VX_UL ASRC & VX_DL ASRC and enable its adaptation */
			
			/* Test if CBPR mode: if ok then ne drift on ASRC*/
			if (protocol->protocol_switch == DMAREQ_PORT_PROT){
				abe_init_asrc_vx_ul(0);				/* No drift to make ASRC neutral for io task*/
				abe_init_asrc_vx_dl(0);				/* No drift to make ASRC neutral for io task*/
			
			}else{
				/* McBSP mode: ARSC for VXDL and VXUL launched at the same time with drift*/	
				abe_init_asrc_vx_ul(-250);
				abe_init_asrc_vx_dl(250);
			}
		}
	}
	if (id == VX_DL_PORT) {
		protocol = &(abe_port[VX_DL_PORT].protocol);
		if (abe_port[VX_UL_PORT].status == OMAP_ABE_PORT_ACTIVITY_RUNNING) {
		/* VX_UL port already started, hence no need to initialize ASRC */
		} else {
			/* Init VX_UL ASRC & VX_DL ASRC and enable its adaptation */
			/* Test if CBPR mode: if ok then ne drift on ASRC*/
			if (protocol->protocol_switch == DMAREQ_PORT_PROT){
				abe_init_asrc_vx_ul(0);				/* No drift to make ASRC neutral for io task*/
				abe_init_asrc_vx_dl(0);				/* No drift to make ASRC neutral for io task*/
			
			}else{
				/* McBSP mode: ARSC for VXDL and VXUL launched at the same time with drift*/	
				abe_init_asrc_vx_ul(-250);
				abe_init_asrc_vx_dl(250);
			}
		}
	}
	/* local host variable status= "port is running" */
	abe_port[id].status = OMAP_ABE_PORT_ACTIVITY_RUNNING;
	/* enable DMA requests */
	abe_enable_dma_request(id);
	/* select the main port based on the activation of this new port */
	abe_decide_main_port();
	return 0;
}
EXPORT_SYMBOL(abe_enable_data_transfer);
/**
 * abe_connect_cbpr_dmareq_port
 * @id: port name
 * @f: desired data format
 * @d: desired dma_request line (0..7)
 * @a: returned pointer to the base address of the CBPr register and number of
 *	samples to exchange during a DMA_request.
 *
 * enables the data echange between a DMA and the ABE through the
 *	CBPr registers of AESS.
 */
abehal_status abe_connect_cbpr_dmareq_port(u32 id, abe_data_format_t *f, u32 d,
					   abe_dma_t *returned_dma_t)
{
	_log(id_connect_cbpr_dmareq_port, id, f->f, f->samp_format);
	abe_port[id] = ((abe_port_t *) abe_port_init)[id];
	(abe_port[id]).format = (*f);
	abe_port[id].protocol.protocol_switch = DMAREQ_PORT_PROT;
	abe_port[id].protocol.p.prot_dmareq.iter = abe_dma_port_iteration(f);
	abe_port[id].protocol.p.prot_dmareq.dma_addr = ABE_DMASTATUS_RAW;
	abe_port[id].protocol.p.prot_dmareq.dma_data = (1 << d);
	/* load the dma_t with physical information from AE memory mapping */
	abe_init_dma_t(id, &((abe_port[id]).protocol));
	abe_port[id].status = OMAP_ABE_PORT_ACTIVITY_IDLE;
	/* load the ATC descriptors - disabled */
	abe_init_atc(id);
	/* load the micro-task parameters */
	abe_init_io_tasks(id, &((abe_port[id]).format),&((abe_port[id]).protocol));
	/* return the dma pointer address */
	abe_read_port_address(id, returned_dma_t);
	return 0;
}
EXPORT_SYMBOL(abe_connect_cbpr_dmareq_port);
/**
 * abe_connect_dmareq_ping_pong_port
 * @id: port name
 * @f: desired data format
 * @d: desired dma_request line (0..7)
 * @s: half-buffer (ping) size
 * @a: returned pointer to the base address of the ping-pong buffer and number
 * of samples to exchange during a DMA_request.
 *
 * enables the data echanges between a DMA and a direct access to
 * the DMEM memory of ABE. On each dma_request activation the DMA will exchange
 * "s" bytes and switch to the "pong" buffer for a new buffer exchange.
 */
abehal_status abe_connect_dmareq_ping_pong_port(u32 id, abe_data_format_t *f,
						u32 d, u32 s,
						abe_dma_t *returned_dma_t)
{
	abe_dma_t dma1;
	_log(id_connect_dmareq_ping_pong_port, id, f->f, f->samp_format);
	/* ping_pong is only supported on MM_DL */
	if (id != MM_DL_PORT) {
		abe_dbg_param |= ERR_API;
		abe_dbg_error_log(ABE_PARAMETER_ERROR);
	}
	/* declare PP buffer and prepare the returned dma_t */
	abe_init_ping_pong_buffer(MM_DL_PORT, s, 2,
				  (u32 *) &(returned_dma_t->data));
	abe_port[id] = ((abe_port_t *) abe_port_init)[id];
	(abe_port[id]).format = (*f);
	(abe_port[id]).protocol.protocol_switch = PINGPONG_PORT_PROT;
	(abe_port[id]).protocol.p.prot_pingpong.buf_addr =
		dmem_ping_pong_buffer;
	(abe_port[id]).protocol.p.prot_pingpong.buf_size = s;
	(abe_port[id]).protocol.p.prot_pingpong.irq_addr = ABE_DMASTATUS_RAW;
	(abe_port[id]).protocol.p.prot_pingpong.irq_data = (1 << d);
	abe_port[id].status = OMAP_ABE_PORT_ACTIVITY_IDLE;
	/* load the micro-task parameters DESC_IO_PP */
	abe_init_io_tasks(id, &((abe_port[id]).format),
			  &((abe_port[id]).protocol));
	/* load the dma_t with physical information from AE memory mapping */
	abe_init_dma_t(id, &((abe_port[id]).protocol));
	dma1.data = (u32 *) (abe_port[id].dma.data +
		ABE_DEFAULT_BASE_ADDRESS_L3 + ABE_DMEM_BASE_OFFSET_MPU);
	dma1.iter = abe_port[id].dma.iter;
	*returned_dma_t = dma1;
	return 0;
}
EXPORT_SYMBOL(abe_connect_dmareq_ping_pong_port);
/**
 * abe_connect_irq_ping_pong_port
 * @id: port name
 * @f: desired data format
 * @I: index of the call-back subroutine to call
 * @s: half-buffer (ping) size
 * @p: returned base address of the first (ping) buffer)
 *
 * enables the data echanges between a direct access to the DMEM
 * memory of ABE using cache flush. On each IRQ activation a subroutine
 * registered with "abe_plug_subroutine" will be called. This subroutine
 * will generate an amount of samples, send them to DMEM memory and call
 * "abe_set_ping_pong_buffer" to notify the new amount of samples in the
 * pong buffer.
 */
abehal_status abe_connect_irq_ping_pong_port(u32 id, abe_data_format_t *f,
					     u32 subroutine_id, u32 size,
					     u32 *sink, u32 dsp_mcu_flag)
{
	_log(id_connect_irq_ping_pong_port, id, f->f, f->samp_format);
	/* ping_pong is only supported on MM_DL */
	if (id != MM_DL_PORT) {
		abe_dbg_param |= ERR_API;
		abe_dbg_error_log(ABE_PARAMETER_ERROR);
	}
	abe_port[id] = ((abe_port_t *) abe_port_init)[id];
	(abe_port[id]).format = (*f);
	(abe_port[id]).protocol.protocol_switch = PINGPONG_PORT_PROT;
	(abe_port[id]).protocol.p.prot_pingpong.buf_addr =
		dmem_ping_pong_buffer;
	(abe_port[id]).protocol.p.prot_pingpong.buf_size = size;
	(abe_port[id]).protocol.p.prot_pingpong.irq_data = (1);
	abe_init_ping_pong_buffer(MM_DL_PORT, size, 2, sink);
	if (dsp_mcu_flag == PING_PONG_WITH_MCU_IRQ)
		(abe_port[id]).protocol.p.prot_pingpong.irq_addr =
			ABE_MCU_IRQSTATUS_RAW;
	if (dsp_mcu_flag == PING_PONG_WITH_DSP_IRQ)
		(abe_port[id]).protocol.p.prot_pingpong.irq_addr =
			ABE_DSP_IRQSTATUS_RAW;
	abe_port[id].status = OMAP_ABE_PORT_ACTIVITY_IDLE;
	/* load the ATC descriptors - disabled */
	abe_init_atc(id);
	/* load the micro-task parameters */
	abe_init_io_tasks(id, &((abe_port[id]).format), &((abe_port[id]).protocol));
	*sink = (abe_port[id]).protocol.p.prot_pingpong.buf_addr;
	return 0;
}
EXPORT_SYMBOL(abe_connect_irq_ping_pong_port);
/**
 * abe_connect_serial_port()
 * @id: port name
 * @f: data format
 * @i: peripheral ID (McBSP #1, #2, #3)
 *
 * Operations : enables the data echanges between a McBSP and an ATC buffer in
 * DMEM. This API is used connect 48kHz McBSP streams to MM_DL and 8/16kHz
 * voice streams to VX_UL, VX_DL, BT_VX_UL, BT_VX_DL. It abstracts the
 * abe_write_port API.
 */
abehal_status abe_connect_serial_port(u32 id, abe_data_format_t *f,
				      u32 mcbsp_id)
{
	u32 UC_NULL[] = { 0 };
	u32 OPP;
	abe_hw_config_init_t CONFIG;
	_log(id_connect_serial_port, id, f->samp_format, mcbsp_id);
	abe_port[id] = ((abe_port_t *) abe_port_init)[id];
	(abe_port[id]).format = (*f);
	(abe_port[id]).protocol.protocol_switch = SERIAL_PORT_PROT;
	/* McBSP peripheral connected to ATC */
	(abe_port[id]).protocol.p.prot_serial.desc_addr = mcbsp_id*ATC_SIZE;
	/* check the iteration of ATC */
	abe_read_hardware_configuration(UC_NULL, &OPP, &CONFIG);
	(abe_port[id]).protocol.p.prot_serial.iter =
		abe_dma_port_iter_factor(f);
	abe_port[id].status = OMAP_ABE_PORT_ACTIVITY_IDLE;
	/* load the ATC descriptors - disabled */
	abe_init_atc(id);
	/* load the micro-task parameters */
	abe_init_io_tasks(id, &((abe_port[id]).format), &((abe_port[id]).protocol));
	return 0;
}
EXPORT_SYMBOL(abe_connect_serial_port);
/**
 * abe_connect_slimbus_port
 * @id: port name
 * @f: data format
 * @i: peripheral ID (McBSP #1, #2, #3)
 * @j: peripheral ID (McBSP #1, #2, #3)
 *
 * enables the data echanges between 1/2 SB and an ATC buffers in
 * DMEM.
 */
abehal_status abe_connect_slimbus_port(u32 id, abe_data_format_t *f,
				       u32 sb_port1, u32 sb_port2)
{
	u32 UC_NULL[] = {
		0
	};
	u32 OPP;
	abe_hw_config_init_t CONFIG;
	u32 iter;
	_log(id_connect_slimbus_port, id, f->samp_format, sb_port2);
	abe_port[id] = ((abe_port_t *) abe_port_init)[id];
	(abe_port[id]).format = (*f);
	(abe_port[id]).protocol.protocol_switch = SLIMBUS_PORT_PROT;
	/* SB1 peripheral connected to ATC */
	(abe_port[id]).protocol.p.prot_slimbus.desc_addr1 = sb_port1*ATC_SIZE;
	/* SB2 peripheral connected to ATC */
	(abe_port[id]).protocol.p.prot_slimbus.desc_addr2 = sb_port2*ATC_SIZE;
	/* check the iteration of ATC */
	abe_read_hardware_configuration(UC_NULL, &OPP, &CONFIG);
	iter = CONFIG.SLIMBUS_DCT_FIFO_SETUP_REG__SB_THRESHOLD;
	/* SLIMBUS iter should be 1 */
	(abe_port[id]).protocol.p.prot_serial.iter = iter;
	abe_port[id].status = OMAP_ABE_PORT_ACTIVITY_IDLE;
	/* load the ATC descriptors - disabled */
	abe_init_atc(id);
	/* load the micro-task parameters */
	abe_init_io_tasks(id, &((abe_port[id]).format), &((abe_port[id]).protocol));
	return 0;
}
EXPORT_SYMBOL(abe_connect_slimbus_port);
/**
 * abe_connect_tdm_port
 * @id: port name
 * @f: data format
 * @i: peripheral ID (McBSP #1, #2, #3)
 * @j: peripheral ID (McBSP #1, #2, #3)
 *
 * enables the data echanges between TDM McBSP ATC buffers in
 * DMEM and 1/2 SMEM buffers
 */
abehal_status abe_connect_tdm_port(u32 id, abe_data_format_t *f, u32 mcbsp_id)
{
	u32 UC_NULL[] = { 0 };
	u32 OPP;
	abe_hw_config_init_t CONFIG;
	u32 iter;
	_log(id_connect_tdm_port, id, f->samp_format, mcbsp_id);
	abe_port[id] = ((abe_port_t *) abe_port_init)[id];
	(abe_port[id]).format = (*f);
	(abe_port[id]).protocol.protocol_switch = TDM_SERIAL_PORT_PROT;
	/* McBSP peripheral connected to ATC */
	(abe_port[id]).protocol.p.prot_serial.desc_addr = mcbsp_id*ATC_SIZE;
	/* check the iteration of ATC */
	abe_read_hardware_configuration(UC_NULL, &OPP, &CONFIG);
	if (abe_port[id].protocol.direction == ABE_ATC_DIRECTION_IN)
		iter = CONFIG.MCBSP_THRSH1_REG_REG__RTHRESHOLD;
	else
		iter = CONFIG.MCBSP_THRSH2_REG_REG__XTHRESHOLD;
	/* McBSP iter should be 1 */
	(abe_port[id]).protocol.p.prot_serial.iter = iter;
	abe_port[id].status = OMAP_ABE_PORT_ACTIVITY_IDLE;
	/* load the ATC descriptors - disabled */
	abe_init_atc(id);
	/* load the micro-task parameters */
	abe_init_io_tasks(id, &((abe_port[id]).format), &((abe_port[id]).protocol));
	return 0;
}
EXPORT_SYMBOL(abe_connect_tdm_port);
/**
 * abe_read_port_address
 * @dma: output pointer to the DMA iteration and data destination pointer
 *
 * This API returns the address of the DMA register used on this audio port.
 * Depending on the protocol being used, adds the base address offset L3
 * (DMA) or MPU (ARM)
 */
abehal_status abe_read_port_address(u32 port, abe_dma_t *dma2)
{
	abe_dma_t_offset dma1;
	u32 protocol_switch;
	_log(id_read_port_address, port, 0, 0);
	dma1 = (abe_port[port]).dma;
	protocol_switch = abe_port[port].protocol.protocol_switch;
	switch (protocol_switch) {
	case PINGPONG_PORT_PROT:
		/* return the base address of the buffer in L3 and L4 spaces */
		(*dma2).data = (void *)(dma1.data +
			ABE_DEFAULT_BASE_ADDRESS_L3 + ABE_DMEM_BASE_OFFSET_MPU);
		(*dma2).l3_dmem = (void *)(dma1.data +
			ABE_DEFAULT_BASE_ADDRESS_L3 + ABE_DMEM_BASE_OFFSET_MPU);
		(*dma2).l4_dmem = (void *)(dma1.data +
			ABE_DEFAULT_BASE_ADDRESS_L4 + ABE_DMEM_BASE_OFFSET_MPU);
		break;
	case DMAREQ_PORT_PROT:
		/* return the CBPr(L3), DMEM(L3), DMEM(L4) address */
		(*dma2).data = (void *)(dma1.data +
			ABE_DEFAULT_BASE_ADDRESS_L3 + ABE_ATC_BASE_OFFSET_MPU);
		(*dma2).l3_dmem =
			(void *)((abe_port[port]).protocol.p.prot_dmareq.buf_addr +
			ABE_DEFAULT_BASE_ADDRESS_L3 + ABE_DMEM_BASE_OFFSET_MPU);
		(*dma2).l4_dmem =
			(void *)((abe_port[port]).protocol.p.prot_dmareq.buf_addr +
			ABE_DEFAULT_BASE_ADDRESS_L4 + ABE_DMEM_BASE_OFFSET_MPU);
		break;
	default:
		break;
	}
	(*dma2).iter = (dma1.iter);
	return 0;
}
EXPORT_SYMBOL(abe_read_port_address);
/*
 *  ABE_SELECT_DATA_SOURCE
 *
 *  Parameter  :
 *	port id where data are exchanged
 *	data_cource_id among:
 *      SRC_DL1_MIXER_OUTPUT (DL1_M_labelID)
 *      SRC_SDT_MIXER_OUTPUT (SDT_M_labelID)
 *      SRC_DL1_GAIN_OUTPUT (DL1_GAIN_out_labelID)
 *      SRC_DL1_EQ_OUTPUT (DL1_EQ_labelID)
 *      SRC_DL2_GAIN_OUTPUT (DL2_GAIN_out_labelID)
 *      SRC_DL2_EQ_OUTPUT (DL2_EQ_labelID)
 *      SRC_MM_DL (MM_DL_labelID)
 *      SRC_TONES_DL  (Tones_labelID)
 *      SRC_VX_DL (VX_DL_labelID)
 *      SRC_VX_UL (VX_UL_labelID)
 *      SRC_MM_UL2 (MM_UL2_labelID)
 *      SRC_MM_UL (MM_UL_labelID)
 *
 *  Operations :
 *
 *  Return value :
 *      None.
 */
abehal_status abe_select_data_source(u32 port_id, u32 smem_source)
{
	u32 sio_desc_address;
	sio_desc_address = dmem_port_descriptors +
		(port_id * sizeof(ABE_SIODescriptor));
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM,
		       sio_desc_address, (u32 *) &sio_desc, sizeof(sio_desc));
	sio_desc.smem_addr1 = (u16) smem_source;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM,
		       sio_desc_address, (u32 *) &sio_desc, sizeof(sio_desc));
	return 0;
}
EXPORT_SYMBOL(abe_select_data_source);
/**
 * abe_write_equalizer
 * @id: name of the equalizer
 * @param : equalizer coefficients
 *
 * Load the coefficients in CMEM.
 */
abehal_status abe_write_equalizer(u32 id, abe_equ_t *param)
{
	u32 eq_offset, length, *src, eq_mem, eq_mem_len;
	_log(id_write_equalizer, id, 0, 0);
	switch (id) {
	default:
	case EQ1:
		eq_offset = C_DL1_Coefs_ADDR;
		eq_mem = S_DL1_M_EQ_data_ADDR;
		eq_mem_len = S_DL1_M_EQ_data_sizeof;
		break;
	case EQ2L:
		eq_offset = C_DL2_L_Coefs_ADDR;
		eq_mem = S_DL2_M_LR_EQ_data_ADDR;
		eq_mem_len = S_DL2_M_LR_EQ_data_sizeof;
		break;
	case EQ2R:
		eq_offset = C_DL2_R_Coefs_ADDR;
		eq_mem = S_DL2_M_LR_EQ_data_ADDR;
		eq_mem_len = S_DL2_M_LR_EQ_data_sizeof;
		break;
	case EQSDT:
		eq_offset = C_SDT_Coefs_ADDR;
		eq_mem = S_SDT_F_data_ADDR;
		eq_mem_len = S_SDT_F_data_sizeof;
		break;
	case EQAMIC:
		eq_offset = C_96_48_AMIC_Coefs_ADDR;
		eq_mem = S_AMIC_96_48_data_ADDR;
		eq_mem_len = S_AMIC_96_48_data_sizeof;
		break;
	case EQDMIC:
		eq_offset = C_96_48_DMIC_Coefs_ADDR;
		eq_mem = S_DMIC0_96_48_data_ADDR;
		eq_mem_len = S_DMIC0_96_48_data_sizeof;
		/* three DMIC are clear at the same time DMIC0 DMIC1 DMIC2 */
		eq_mem_len *= 3;
		break;
	}
	length = param->equ_length;
	src = (u32 *) ((param->coef).type1);
	/* translate in bytes */
	eq_offset <<= 2;
	/* reset SMEM buffers before the coefficients are loaded */
	abe_reset_mem(ABE_SMEM, eq_mem << 3, eq_mem_len << 3);
	/* translate in bytes */
	length <<= 2;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_CMEM, eq_offset, src, length);
	/* reset SMEM buffers after the coefficients are loaded */
	abe_reset_mem(ABE_SMEM, eq_mem << 3, eq_mem_len << 3);
	return 0;
}
EXPORT_SYMBOL(abe_write_equalizer);
/**
 * abe_write_asrc
 * @id: name of the port
 * @param: drift value to compensate [ppm]
 *
 * Load the drift variables to the FW memory. This API can be called only
 * when the corresponding port has been already opened and the ASRC has
 * been correctly initialized with API abe_init_asrc_... If this API is
 * used such that the drift has been changed from positive to negative drift
 * or vice versa, there will be click in the output signal. Loading the drift
 * value with zero disables the feature.
 */
abehal_status abe_write_asrc(u32 port, s32 dppm)
{
	s32 dtempvalue, adppm, drift_sign, drift_sign_addr, alpha_params_addr;
	s32 alpha_params[3];
	_log(id_write_asrc, port, dppm, dppm >> 8);
	/*
	 * x = ppm
	 *
	 * - 1000000/x must be multiple of 16
	 * - deltaalpha = round(2^20*x*16/1000000)=round(2^18/5^6*x) on 22 bits.
	 *      then shifted by 2bits
	 * - minusdeltaalpha
	 * - oneminusepsilon = 1-deltaalpha/2.
	 *
	 * ppm = 250
	 * - 1000000/250=4000
	 * - deltaalpha = 4194.3 ~ 4195 => 0x00418c
	 */
	/* examples for -6250 ppm */
	/* atempvalue32[1] = -1;  d_driftsign */
	/* atempvalue32[3] = 0x00066668;  d_deltaalpha */
	/* atempvalue32[4] = 0xfff99998;  d_minusdeltaalpha */
	/* atempvalue32[5] = 0x003ccccc;  d_oneminusepsilon */
	/* example for 100 ppm */
	/* atempvalue32[1] = 1;* d_driftsign */
	/* atempvalue32[3] = 0x00001a38;  d_deltaalpha */
	/* atempvalue32[4] = 0xffffe5c8;  d_minusdeltaalpha */
	/* atempvalue32[5] = 0x003ccccc;  d_oneminusepsilon */
	/* compute new value for the ppm */
	if (dppm >= 0) {
		/* d_driftsign */
		drift_sign = 1;
		adppm = dppm;
	}
	else {
		/* d_driftsign */
		drift_sign = -1;
		adppm = (-1 * dppm);
	}
	if (dppm == 0) {
		/* delta_alpha */
		alpha_params[0] = 0;
		/* minusdelta_alpha */
		alpha_params[1] = 0;
		/* one_minusepsilon */
		alpha_params[2] = 0x003ffff0;
	}
	else {
		dtempvalue = (adppm << 4) + adppm - ((adppm * 3481L) / 15625L);
		/* delta_alpha */
		alpha_params[0] = dtempvalue << 2;
		/* minusdelta_alpha */
		alpha_params[1] = (-dtempvalue) << 2;
		/* one_minusepsilon */
		alpha_params[2] = (0x00100000 - (dtempvalue / 2)) << 2;
	}
	switch (port) {
	/* asynchronous sample-rate-converter for the uplink voice path */
	case VX_DL_PORT:
		drift_sign_addr = D_AsrcVars_DL_VX_ADDR + (1 * sizeof(s32));
		alpha_params_addr = D_AsrcVars_DL_VX_ADDR + (3 * sizeof(s32));
		break;
	/* asynchronous sample-rate-converter for the downlink voice path */
	case VX_UL_PORT:
		drift_sign_addr = D_AsrcVars_UL_VX_ADDR + (1 * sizeof(s32));
		alpha_params_addr = D_AsrcVars_UL_VX_ADDR + (3 * sizeof(s32));
		break;
	/* asynchronous sample-rate-converter for the BT_UL path */
	case BT_VX_UL_PORT:
		drift_sign_addr = D_AsrcVars_BT_UL_ADDR + (1 * sizeof(s32));
		alpha_params_addr = D_AsrcVars_BT_UL_ADDR + (3 * sizeof(s32));
		break;
	/* asynchronous sample-rate-converter for the BT_DL path */
	case BT_VX_DL_PORT:
		drift_sign_addr = D_AsrcVars_BT_DL_ADDR + (1 * sizeof(s32));
		alpha_params_addr = D_AsrcVars_BT_DL_ADDR + (3 * sizeof(s32));
		break;
	default:
	/* asynchronous sample-rate-converter for the MM_EXT_IN path */
	case MM_EXT_IN_PORT:
		drift_sign_addr = D_AsrcVars_MM_EXT_IN_ADDR + (1 * sizeof(s32));
		alpha_params_addr =
			D_AsrcVars_MM_EXT_IN_ADDR + (3 * sizeof(s32));
		break;
	}
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, drift_sign_addr,
		       (u32 *) &drift_sign, 4);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, alpha_params_addr,
		       (u32 *) &alpha_params[0], 12);
	return 0;
}
EXPORT_SYMBOL(abe_write_asrc);
/**
 * abe_write_aps
 * @id: name of the aps filter
 * @param: table of filter coefficients
 *
 * Load the filters and thresholds coefficients in FW memory. This AP
 * can be called when the corresponding APS is not activated. After
 * reloading the firmware the default coefficients corresponds to "no APS
 * activated".
 * Loading all the coefficients value with zero disables the feature.
 */
abehal_status abe_write_aps(u32 id, abe_aps_t *param)
{
	_log(id_write_aps, id, 0, 0);
	return 0;
}
EXPORT_SYMBOL(abe_write_aps);
/**
 * abe_use_compensated_gain
 * @on_off:
 *
 * Selects the automatic Mixer's gain management
 * on_off = 1 allows the "abe_write_gain" to adjust the overall
 * gains of the mixer to be tuned not to create saturation
 */
abehal_status abe_use_compensated_gain(u32 on_off)
{
	abe_compensated_mixer_gain = on_off;
	return 0;
}
EXPORT_SYMBOL(abe_use_compensated_gain);
/**
 * abe_mute_gain
 * Parameters:
 *	mixer id
 *	sub-port id
 *
 */
abehal_status abe_mute_gain(u32 id, u32 p)
{
	u32 mixer_offset, f_g, ramp, gain_saved;
	/* save the input parameters for mute/unmute */
	abe_gain_offset(id, &mixer_offset);
	if (abe_muted_gains_decibel[mixer_offset + p] == GAIN_TOOLOW) {
		ramp = abe_desired_ramp_delay_ms[mixer_offset + p];
		f_g = (u32) GAIN_MUTE;
		gain_saved = abe_desired_gains_decibel[mixer_offset + p];
		/* mute the gain */
		abe_write_gain(id, f_g, ramp, p);
		abe_muted_gains_decibel[mixer_offset + p] = gain_saved;
	}
	return 0;
}
EXPORT_SYMBOL(abe_mute_gain);
/**
 * abe_unmute_gain
 * Parameters:
 *	mixer id
 *	sub-port id
 *
 */
abehal_status abe_unmute_gain(u32 id, u32 p)
{
	u32 mixer_offset, f_g, ramp;
	/* restore the input parameters for mute/unmute */
	abe_gain_offset(id, &mixer_offset);
	if (abe_muted_gains_decibel[mixer_offset + p] != GAIN_TOOLOW) {
		f_g = abe_muted_gains_decibel[mixer_offset + p];
		ramp = abe_desired_ramp_delay_ms[mixer_offset + p];
		abe_muted_gains_decibel[mixer_offset + p] = (u32) GAIN_TOOLOW;
		/* unmute the gain */
		abe_write_gain(id, f_g, ramp, p);	
	}
	return 0;
}
EXPORT_SYMBOL(abe_unmute_gain);
/**
 * abe_write_gain
 * @id: gain name or mixer name
 * @f_g: list of input gains of the mixer
 * @ramp: gain ramp speed factor
 * @p: list of ports corresponding to the above gains
 *
 * Loads the gain coefficients to FW memory. This API can be called when
 * the corresponding MIXER is not activated. After reloading the firmware
 * the default coefficients corresponds to "all input and output mixer's gain
 * in mute state". A mixer is disabled with a network reconfiguration
 * corresponding to an OPP value.
 */
abehal_status abe_write_gain(u32 id, s32 f_g, u32 ramp, u32 p)
{
	u32 lin_g, sum_g, mixer_target, mixer_offset, i, mean_gain, mean_exp;
	u32 new_gain_linear[4];
	s32 gain_index;
	u32 alpha, beta;
	u32 ramp_index;
	_log(id_write_gain, id, f_g, p);
	gain_index = ((f_g - min_mdb) / 100);
	gain_index = maximum(gain_index, 0);
	gain_index = minimum(gain_index, sizeof_db2lin_table);
	lin_g = abe_db2lin_table[gain_index];
	abe_gain_offset(id, &mixer_offset);
	if (abe_muted_gains_decibel[mixer_offset + p] != GAIN_TOOLOW) {
		abe_muted_gains_decibel[mixer_offset + p] = f_g;
		goto write_g_end;
	}
	/* save the input parameters for mute/unmute */
	abe_desired_gains_linear[mixer_offset + p] = lin_g;
	abe_desired_gains_decibel[mixer_offset + p] = f_g;
	abe_desired_ramp_delay_ms[mixer_offset + p] = ramp;
	/* SMEM word32 address */
	mixer_target = (S_GTarget1_ADDR << 1);
	mixer_target += mixer_offset;
	mixer_target += p;
	/* translate coef address in Bytes */
	mixer_target <<= 2;
	if (abe_compensated_mixer_gain) {
		switch (id) {
		case MIXDL1:
		case MIXDL2:
		case MIXVXREC:
		case MIXAUDUL:
			/* compute the sum of the gain of the mixer */
			for (sum_g = i = 0; i < 4; i++)
				sum_g += abe_desired_gains_linear[mixer_offset +
								  i];
			/* lets avoid a division by 0 */
			if (sum_g == 0)
				break;
			/* if the sum is OK with less than 1, then
			   do not weight the gains */
			if (sum_g < 0x00040000) {	/* REMOVE HARD CONST */
			/* recompute all gains from original desired values */
				sum_g = 0x00040000;
			}
			/* translate it in Q16 format for the later division */
			abe_int_2_float16(sum_g, &mean_gain, &mean_exp);
			mean_exp = 10 - mean_exp;
			for (i = 0; i < 4; i++) {
			/* new gain = desired gain divided by sum of gains */
				new_gain_linear[i] =
					(abe_desired_gains_linear
					 [mixer_offset + i]
					 << 8) / mean_gain;
				new_gain_linear[i] = (mean_exp > 0) ?
					new_gain_linear[i] << mean_exp :
					new_gain_linear[i] >> mean_exp;
			}
			/* load the whole adpated S_G_Target SMEM MIXER table */
			abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
				       mixer_target - (p << 2),
				       new_gain_linear, (4 * sizeof(lin_g)));
			break;
		default:
			/* load the S_G_Target SMEM table */
			abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
				       mixer_target,
				       (u32 *) &lin_g, sizeof(lin_g));
			break;
		}
	}
	else {
		/* load the S_G_Target SMEM table */
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
			mixer_target, (u32 *) &lin_g, sizeof(lin_g));
	}
	ramp = maximum(minimum(RAMP_MAXLENGTH, ramp), RAMP_MINLENGTH);
	/* ramp data should be interpolated in the table instead */
	ramp_index = 3;
	if ((RAMP_2MS <= ramp) && (ramp < RAMP_5MS))
	ramp_index = 8;
	if ((RAMP_5MS <= ramp) && (ramp < RAMP_50MS))
		ramp_index = 24;
	if ((RAMP_50MS <= ramp) && (ramp < RAMP_500MS))
		ramp_index = 36;
	if (ramp > RAMP_500MS)
		ramp_index = 48;
	beta = abe_alpha_iir[ramp_index];
	alpha = abe_1_alpha_iir[ramp_index];
	/* CMEM word32 address */
	mixer_target = C_1_Alpha_ADDR;
	/* a pair of gains is updated once in the firmware */
	mixer_target += (p + mixer_offset) >> 1;
	/* translate coef address in Bytes */
	mixer_target <<= 2;
	/* load the ramp delay data */
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_CMEM, mixer_target,
		       (u32 *) &alpha, sizeof(alpha));
	/* CMEM word32 address */
	mixer_target = C_Alpha_ADDR;
	/* a pair of gains is updated once in the firmware */
	mixer_target += (p + mixer_offset) >> 1;
	/* translate coef address in Bytes */
	mixer_target <<= 2;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_CMEM, mixer_target,
		       (u32 *) &beta, sizeof(beta));
     write_g_end:
	return 0;
}
EXPORT_SYMBOL(abe_write_gain);
/**
 * abe_write_mixer
 * @id: name of the mixer
 * @param: input gains and delay ramp of the mixer
 * @p: port corresponding to the above gains
 *
 * Load the gain coefficients in FW memory. This API can be called when
 * the corresponding MIXER is not activated. After reloading the firmware
 * the default coefficients corresponds to "all input and output mixer's
 * gain in mute state". A mixer is disabled with a network reconfiguration
 * corresponding to an OPP value.
 */
abehal_status abe_write_mixer(u32 id, s32 f_g, u32 f_ramp, u32 p)
{
	_log(id_write_mixer, id, f_ramp, p);
	abe_write_gain(id, f_g, f_ramp, p);
	return 0;
}
EXPORT_SYMBOL(abe_write_mixer);
/**
 * abe_read_gain
 * @id: name of the mixer
 * @param: list of input gains of the mixer
 * @p: list of port corresponding to the above gains
 *
 */
abehal_status abe_read_gain(u32 id, u32 *f_g, u32 p)
{
	u32 mixer_target, mixer_offset, i;
	_log(id_read_gain, id, (u32) f_g, p);
	abe_gain_offset(id, &mixer_offset);
	if (abe_muted_gains_decibel[mixer_offset + p] != GAIN_TOOLOW) {
		*f_g = abe_muted_gains_decibel[mixer_offset + p];
		goto found;
	}
	/* SMEM word32 address */
	mixer_target = (S_GTarget1_ADDR << 1);
	mixer_target += mixer_offset;
	mixer_target += p;
	/* translate coef address in Bytes */
	mixer_target <<= 2;
	/* load the S_G_Target SMEM table */
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_SMEM, mixer_target,
		       (u32 *) f_g, sizeof(*f_g));
	/* translate the linear gain to milliBel */
	for (i = 0; i < sizeof_db2lin_table; i++) {
		if (abe_db2lin_table[i] == *f_g) {
			*f_g = (i * 100) + min_mdb;
			goto found;
		}
	}
	*f_g = min_mdb;
	return -1;
found:
	return 0;
}
EXPORT_SYMBOL(abe_read_gain);
/**
 * abe_read_mixer
 * @id: name of the mixer
 * @param: gains of the mixer
 * @p: port corresponding to the above gains
 *
 * Load the gain coefficients in FW memory. This API can be called when
 * the corresponding MIXER is not activated. After reloading the firmware
 * the default coefficients corresponds to "all input and output mixer's
 * gain in mute state". A mixer is disabled with a network reconfiguration
 * corresponding to an OPP value.
 */
abehal_status abe_read_mixer(u32 id, u32 *f_g, u32 p)
{
	_log(id_read_mixer, id, 0, p);
	abe_read_gain(id, f_g, p);
	return 0;
}
EXPORT_SYMBOL(abe_read_mixer);
/**
 * abe_mono_mixer
 * @id: name of the mixer (MIXDL1 or MIXDL2)
 * on_off: enable\disable flag
 *
 * This API Programs DL1Mixer or DL2Mixer to output mono data
 * on both left and right data paths.
 */
abehal_status abe_mono_mixer(u32 id, u32 on_off)
{
#define ABE_TASK_ID(ID) (D_tasksList_ADDR + sizeof(ABE_STask)*(ID))
#define TASK_DL2Mixer_SLT 1
#define TASK_DL2Mixer_IDX 6
#define TASK_DL1Mixer_SLT 2
#define TASK_DL1Mixer_IDX 0
	
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM, D_multiFrame_ADDR,
				(u32 *) MultiFrame, sizeof(MultiFrame));
	
	switch (id) {
		case MIXDL1:
			if (on_off)
				MultiFrame[TASK_DL1Mixer_SLT][TASK_DL1Mixer_IDX] =
					ABE_TASK_ID(C_ABE_FW_TASK_DL1Mixer_dual_mono);
			else
				MultiFrame[TASK_DL1Mixer_SLT][TASK_DL1Mixer_IDX] =
					ABE_TASK_ID(C_ABE_FW_TASK_DL1Mixer);
			break;
		case MIXDL2:
			if (on_off)
				MultiFrame[TASK_DL2Mixer_SLT][TASK_DL2Mixer_IDX] =
					ABE_TASK_ID(C_ABE_FW_TASK_DL2Mixer_dual_mono);
			else
				MultiFrame[TASK_DL2Mixer_SLT][TASK_DL2Mixer_IDX] =
					ABE_TASK_ID(C_ABE_FW_TASK_DL2Mixer);
			break;
		default:
			break;
	}
			
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, D_multiFrame_ADDR,
				(u32 *) MultiFrame, sizeof(MultiFrame));
	return 0;
}
EXPORT_SYMBOL(abe_mono_mixer);
/**
 * abe_set_router_configuration
 * @Id: name of the router
 * @Conf: id of the configuration
 * @param: list of output index of the route
 *
 * The uplink router takes its input from DMIC (6 samples), AMIC (2 samples)
 * and PORT1/2 (2 stereo ports). Each sample will be individually stored in
 * an intermediate table of 10 elements.
 *
 * Example of router table parameter for voice uplink with phoenix microphones
 *
 * indexes 0 .. 9 = MM_UL description (digital MICs and MMEXTIN)
 *	DMIC1_L_labelID, DMIC1_R_labelID, DMIC2_L_labelID, DMIC2_R_labelID,
 *	MM_EXT_IN_L_labelID, MM_EXT_IN_R_labelID, ZERO_labelID, ZERO_labelID,
 *	ZERO_labelID, ZERO_labelID,
 * indexes 10 .. 11 = MM_UL2 description (recording on DMIC3)
 *	DMIC3_L_labelID, DMIC3_R_labelID,
 * indexes 12 .. 13 = VX_UL description (VXUL based on PDMUL data)
 *	AMIC_L_labelID, AMIC_R_labelID,
 * indexes 14 .. 15 = RESERVED (NULL)
 *	ZERO_labelID, ZERO_labelID,
 */
abehal_status abe_set_router_configuration(u32 id, u32 k, u32 *param)
{
	_log(id_set_router_configuration, id, (u32) param, (u32) param >> 8);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM,
			       D_aUplinkRouting_ADDR,
			       param, D_aUplinkRouting_sizeof);
	return 0;
}
EXPORT_SYMBOL(abe_set_router_configuration);
/**
 * ABE_READ_DEBUG_TRACE
 *
 * Parameters :
 * @data: data destination pointer
 * @n	: max number of read data
 *
 * Operations :
 * Reads the AE circular data pointer that holds pairs of debug data +
 * timestamps, and stores the pairs, via linear addressing, to the parameter
 * pointer.
 * Stops the copy when the max parameter is reached or when the FIFO is empty.
 *
 * Return value :
 * None.
 */
abehal_status abe_read_debug_trace(u32 *data, u32 *n)
{
	_log(id_select_data_source, 0, 0, 0);
	return 0;
}
EXPORT_SYMBOL(abe_read_debug_trace);
/**
 * abe_connect_debug_trace
 * @dma2:pointer to the DMEM trace buffer
 *
 * returns the address and size of the real-time debug trace buffer,
 * the content of which will vary from one firmware release to another
 */
abehal_status abe_connect_debug_trace(abe_dma_t *dma2)
{
	_log(id_connect_debug_trace, 0, 0, 0);
	/* return the base address of the ping buffer in L3 and L4 spaces */
	(*dma2).data = (void *)(D_DEBUG_FIFO_ADDR +
		ABE_DEFAULT_BASE_ADDRESS_L3 + ABE_DMEM_BASE_OFFSET_MPU);
	(*dma2).l3_dmem = (void *)(D_DEBUG_FIFO_ADDR +
		ABE_DEFAULT_BASE_ADDRESS_L3 + ABE_DMEM_BASE_OFFSET_MPU);
	(*dma2).l4_dmem = (void *)(D_DEBUG_FIFO_ADDR +
		ABE_DEFAULT_BASE_ADDRESS_L4 + ABE_DMEM_BASE_OFFSET_MPU);
	(*dma2).iter = (D_DEBUG_FIFO_sizeof + D_DEBUG_FIFO_HAL_sizeof)>>2;
	return 0;
}
EXPORT_SYMBOL(abe_connect_debug_trace);
/**
 * abe_set_debug_trace
 * @debug: debug ID from a list to be defined
 *
 * loads a mask which filters the debug trace to dedicated types of data
 */
abehal_status abe_set_debug_trace(abe_dbg_t debug)
{
	_log(id_set_debug_trace, 0, 0, 0);
	abe_dbg_mask = debug;
	return 0;
}
EXPORT_SYMBOL(abe_set_debug_trace);
/**
 * abe_remote_debugger_interface
 *
 * interpretation of the UART stream from the remote debugger commands.
 * The commands consist in setting break points, loading parameter
 */
abehal_status abe_remote_debugger_interface(u32 n, u8 *p)
{
	_log(id_remote_debugger_interface, n, 0, 0);
	return 0;
}
EXPORT_SYMBOL(abe_remote_debugger_interface);
/**
 * abe_enable_test_pattern
 *
 */
abehal_status abe_enable_test_pattern(u32 smem_id, u32 on_off)
{
	u16 dbg_on, dbg_off, idx_patch, task_patch, addr_patch;
	u32 patch, task32;
	_log(id_enable_test_pattern, on_off, smem_id, smem_id >> 8);
	switch (smem_id) {
	case DBG_PATCH_AMIC:
		dbg_on = DBG_48K_PATTERN_labelID;
		dbg_off = AMIC_labelID;
		task_patch = C_ABE_FW_TASK_AMIC_SPLIT;
		idx_patch = 1;
		break;
	case DBG_PATCH_DMIC1:
		dbg_on = DBG_48K_PATTERN_labelID;
		dbg_off = DMIC1_labelID;
		task_patch = C_ABE_FW_TASK_DMIC1_SPLIT;
		idx_patch = 1;
		break;
	case DBG_PATCH_DMIC2:
		dbg_on = DBG_48K_PATTERN_labelID;
		dbg_off = DMIC2_labelID;
		task_patch = C_ABE_FW_TASK_DMIC2_SPLIT;
		idx_patch = 1;
		break;
	case DBG_PATCH_DMIC3:
		dbg_on = DBG_48K_PATTERN_labelID;
		dbg_off = DMIC3_labelID;
		task_patch = C_ABE_FW_TASK_DMIC3_SPLIT;
		idx_patch = 1;
		break;
	case DBG_PATCH_VX_REC:
		dbg_on = DBG_48K_PATTERN_labelID;
		dbg_off = VX_REC_labelID;
		task_patch = C_ABE_FW_TASK_VXREC_SPLIT;
		idx_patch = 1;
		break;
	case DBG_PATCH_BT_UL:
		dbg_on = DBG_48K_PATTERN_labelID;
		dbg_off = BT_UL_labelID;
		task_patch = C_ABE_FW_TASK_BT_UL_SPLIT;
		idx_patch = 1;
		break;
	case DBG_PATCH_MM_DL:
		dbg_on = DBG_48K_PATTERN_labelID;
		dbg_off = MM_DL_labelID;
		task_patch = C_ABE_FW_TASK_MM_SPLIT;
		idx_patch = 1;
		break;
	case DBG_PATCH_VIBRA:
		dbg_on = DBG_48K_PATTERN_labelID;
		dbg_off = VIBRA_labelID;
		task_patch = C_ABE_FW_TASK_VIBRA_SPLIT;
		idx_patch = 1;
		break;
	case DBG_PATCH_MM_EXT_IN:
		dbg_on = DBG_48K_PATTERN_labelID;
		dbg_off = MM_EXT_IN_labelID;
		task_patch = C_ABE_FW_TASK_MM_EXT_IN_SPLIT;
		idx_patch = 1;
		break;
	case DBG_PATCH_MM_DL_MIXDL1:
		dbg_on = DBG_48K_PATTERN_labelID;
		dbg_off = AMIC_labelID;
		task_patch = C_ABE_FW_TASK_DL1Mixer;
		idx_patch = 1;
		break;
	case DBG_PATCH_MM_DL_MIXDL2:
		dbg_on = DBG_48K_PATTERN_labelID;
		dbg_off = AMIC_labelID;
		task_patch = C_ABE_FW_TASK_DL2Mixer;
		idx_patch = 1;
		break;
	default:
		return 0;
	}
	patch = (on_off != 0) ? dbg_on : dbg_off;
	/* address is on 16bits boundary */
	addr_patch = D_tasksList_ADDR + (16 * task_patch) + (2 * idx_patch);
	/* read on 32bits words' boundary */
	abe_block_copy(COPY_FROM_ABE_TO_HOST, ABE_DMEM, addr_patch & (~0x03),
		       &task32, 4);
	if (addr_patch & 0x03)
		task32 = (0x0000FFFFL & task32) | (patch << 16);
	else
		task32 = (0xFFFF0000L & task32) | (0x0000FFFF & patch);
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_DMEM, addr_patch & (~0x03),
		       &task32, 4);
	return 0;
}
EXPORT_SYMBOL(abe_enable_test_pattern);
/**
 * abe_wakeup - Wakeup ABE
 *
 * Wakeup ABE in case of retention
 */
abehal_status abe_wakeup(void)
{
	/* Restart event generator */
	abe_write_event_generator(EVENT_TIMER);
	/* reconfigure DMA Req and MCU Irq visibility */
	abe_hw_configuration();
	return 0;
}
EXPORT_SYMBOL(abe_wakeup);
/**
 * abe_disable_irq - disable MCU/DSP ABE interrupt
 *
 * This subroutine is disabling ABE MCU/DSP Irq
 */
abehal_status abe_disable_irq(void)
{
	u32 atc_reg;
	/* disables the DMAreq from AESS AESS_DMAENABLE_CLR = 127
	 * DMA_Req7 will still be enabled as it is used for ABE trace */
	atc_reg = 0x7F;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_ATC, 0x64, &atc_reg, 4);
	/* disables the MCU IRQ from AESS to Cortex A9 */
	atc_reg = 0x01;
	abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_ATC, 0x40, &atc_reg, 4);
	return 0;
}
EXPORT_SYMBOL(abe_disable_irq);
/**
 * abe_check_activity - Check if some ABE activity.
 *
 * Check if any ABE ports are running.
 * return 1: still activity on ABE
 * return 0: no more activity on ABE. Event generator can be stopped
 *
 */
u32 abe_check_activity(void)
{
	u32 i;
	u32 ret;
	ret = 0;
	for (i = 0; i < (LAST_PORT_ID - 1); i++) {
		if (abe_port[abe_port_priority[i]].status ==
				OMAP_ABE_PORT_ACTIVITY_RUNNING)
			break;
	}
	if (i < (LAST_PORT_ID - 1))
		ret = 1;
	return ret;
}
EXPORT_SYMBOL(abe_check_activity);
/**
 * abe_init_mem - Allocate Kernel space memory map for ABE
 *
 * Memory map of ABE memory space for PMEM/DMEM/SMEM/DMEM
 */
void abe_init_mem(void __iomem *_io_base)
{
	io_base = _io_base;
}
EXPORT_SYMBOL(abe_init_mem);
/**
 * abe_write_pdmdl_offset - write the desired offset on the DL1/DL2 paths
 *
 * Parameters:
 *   path: 1 for the DL1 ABE path, 2 for the DL2 ABE path
 *   offset_left: integer value that will be added on all PDM left samples
 *   offset_right: integer value that will be added on all PDM right samples
 *
 */
void abe_write_pdmdl_offset(u32 path, u32 offset_left, u32 offset_right)
{
	switch(path) {
	case 1:
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
			4 + (S_DC_HS_ADDR << 3), &offset_left, sizeof(u32));
			abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
			(S_DC_HS_ADDR<<3), &offset_right, sizeof(u32));
			break;
	case 2:
		abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
			4 + (S_DC_HF_ADDR << 3), &offset_left, sizeof(u32));
			abe_block_copy(COPY_FROM_HOST_TO_ABE, ABE_SMEM,
			(S_DC_HF_ADDR<<3), &offset_right, sizeof(u32));
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(abe_write_pdmdl_offset);
/**
 * abe_write_select_pdm_output - Select the path for OPP25 route input
 *
 * Parameters:
 *   path: 1 for output om headset, 2 for output on handsfree,
 *	 3 for output on headset and handsfree at the same time
 *
 */
void abe_write_select_pdm_output(u32 path)
{
	abe_mcpdm_path = path;
}
EXPORT_SYMBOL(abe_write_select_pdm_output);