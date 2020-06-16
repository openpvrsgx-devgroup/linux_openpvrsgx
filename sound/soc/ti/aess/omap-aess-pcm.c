/*
 * ALSA SoC driver for OMAP4/5 AESS (Audio Engine Sub-System)
 *
 * Copyright (C) 2010-2013 Texas Instruments
 *
 * Authors: Liam Girdwood <lrg@ti.com>
 *          Misael Lopez Cruz <misael.lopez@ti.com>
 *          Sebastien Guiriec <s-guiriec@ti.com>
 *          Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 * Contact: Misael Lopez Cruz <misael.lopez@ti.com>
 *          Peter Ujfalusi <peter.ujfalusi@ti.com>
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

#include <linux/pm_runtime.h>


#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include "omap-aess-priv.h"
#include "omap-aess.h"
#include "aess_gain.h"
#include "aess_ini.h"
#include "aess_mem.h"
#include "aess_port.h"
#include "aess_utils.h"
#include "port_mgr.h"
#include "aess_opp.h"

/*
 * omap_aess_irq_data
 *
 * IRQ FIFO content declaration
 *	APS interrupts : IRQ_FIFO[31:28] = IRQtag_APS,
 *		IRQ_FIFO[27:16] = APS_IRQs, IRQ_FIFO[15:0] = loopCounter
 *	SEQ interrupts : IRQ_FIFO[31:28] OMAP_AESS_IRQTAG_COUNT,
 *		IRQ_FIFO[27:16] = Count_IRQs, IRQ_FIFO[15:0] = loopCounter
 *	Ping-Pong Interrupts : IRQ_FIFO[31:28] = OMAP_AESS_IRQTAG_PP,
 *		IRQ_FIFO[27:16] = PP_MCU_IRQ, IRQ_FIFO[15:0] = loopCounter
 */
struct omap_aess_irq_data {
	unsigned int counter:16;
	unsigned int data:12;
	unsigned int tag:4;
};

/* Ping pong buffer DMEM offset - we should read this from future FWs */
#define OMAP_AESS_BANK_DMEM_PP_OFFSET	0x4000

static const struct snd_pcm_hardware omap_aess_hardware = {
	.info			= SNDRV_PCM_INFO_MMAP |
				  SNDRV_PCM_INFO_MMAP_VALID |
				  SNDRV_PCM_INFO_INTERLEAVED |
				  SNDRV_PCM_INFO_BLOCK_TRANSFER |
				  SNDRV_PCM_INFO_PAUSE |
				  SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE |
				  SNDRV_PCM_FMTBIT_S32_LE,
	.period_bytes_min	= 4 * 1024,
	.period_bytes_max	= 24 * 1024,
	.periods_min		= 4,
	.periods_max		= 4,
	.buffer_bytes_max	= 24 * 1024 * 2,
};

static int aess_save_context(struct omap_aess *aess)
{
	/* mute gains not associated with FEs/BEs */
	omap_aess_mute_gain(aess, OMAP_AESS_MIXAUDUL_MM_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXAUDUL_TONES);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXAUDUL_VX_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXVXREC_TONES);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXVXREC_VX_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXVXREC_MM_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXVXREC_VX_UL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXECHO_DL1);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXECHO_DL2);

	/*
	 * mute gains associated with DL1 BE
	 * ideally, these gains should be muted/saved when BE is muted, but
	 * when ABE McPDM is started for DL1 or DL2, PDM_DL1 port gets enabled
	 * which prevents to mute these gains since two ports on DL1 path are
	 * active when mute is called for BT_VX_DL or MM_EXT_DL.
	 *
	 * These gains are not restored along with the context because they
	 * are properly unmuted/restored when any of the DL1 BEs is unmuted
	 */
	omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DL1_LEFT);
	omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DL1_RIGHT);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXSDT_DL);

	return 0;
}

/**
 * omap_aess_write_pdmdl_offset - write the desired offset on the DL1/DL2 paths
 * @aess: Pointer on aess handle
 * @path: DL1 or DL2 port
 * @offset_left: integer value that will be added on all PDM left samples
 * @offset_right: integer value that will be added on all PDM right samples
 *
 * Set AESS internal DC offset cancellation parameter for McPDM IP. Value
 * depends on TWL604x triming parameters.
 */
static void omap_aess_write_pdmdl_offset(struct omap_aess *aess, u32 path,
					 u32 offset_left, u32 offset_right)
{
	u32 offset[2];

	offset[0] = offset_right;
	offset[1] = offset_left;

	switch (path) {
	case 1:
		omap_aess_write_map(aess, OMAP_AESS_SMEM_DC_HS_ID, offset);
		break;
	case 2:
		omap_aess_write_map(aess, OMAP_AESS_SMEM_DC_HF_ID, offset);
		break;
	default:
		break;
	}
}

static void aess_reconfigure_profile(struct omap_aess *aess)
{
	int i;

	omap_aess_set_router_configuration(aess, (u32 *)aess->mixer.route_ul);

	/* DC offset cancellation setting */
	if (aess->dc_offset.power_mode)
		omap_aess_write_pdmdl_offset(aess, 1, aess->dc_offset.hsl * 2,
					     aess->dc_offset.hsr * 2);
	else
		omap_aess_write_pdmdl_offset(aess, 1, aess->dc_offset.hsl,
					     aess->dc_offset.hsr);

	omap_aess_write_pdmdl_offset(aess, 2, aess->dc_offset.hfl,
				     aess->dc_offset.hfr);

	aess_mixer_set_equ_profile(aess, OMAP_AESS_CMEM_DL1_COEFS_ID,
				   aess->equ.dl1.profile);
	aess_mixer_set_equ_profile(aess, OMAP_AESS_CMEM_DL2_L_COEFS_ID,
				   aess->equ.dl2l.profile);
	aess_mixer_set_equ_profile(aess, OMAP_AESS_CMEM_DL2_R_COEFS_ID,
				   aess->equ.dl2r.profile);
	aess_mixer_set_equ_profile(aess, OMAP_AESS_CMEM_SDT_COEFS_ID,
				   aess->equ.sdt.profile);
	aess_mixer_set_equ_profile(aess, OMAP_AESS_CMEM_96_48_AMIC_COEFS_ID,
				   aess->equ.amic.profile);
	aess_mixer_set_equ_profile(aess, OMAP_AESS_CMEM_96_48_DMIC_COEFS_ID,
				   aess->equ.dmic.profile);

	for (i = 0; i < OMAP_AESS_NUM_MONO_MIXERS; i++)
		aess_mixer_enable_mono(aess, OMAP_AESS_MIX_DL1_MONO + i,
				       aess->mixer.mono[i]);
}

static int aess_restore_context(struct omap_aess *aess)
{
	int ret;

	if (aess->device_scale) {
		ret = aess->device_scale(aess->dev, aess->dev,
				aess->opp.freqs[OMAP_AESS_OPP_50]);
		if (ret) {
			dev_err(aess->dev, "failed to scale to OPP 50\n");
			return ret;
		}
	}

	/* unmute gains not associated with FEs/BEs */
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXAUDUL_MM_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXAUDUL_TONES);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXAUDUL_VX_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXVXREC_TONES);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXVXREC_VX_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXVXREC_MM_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXVXREC_VX_UL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXECHO_DL1);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXECHO_DL2);

	aess_reconfigure_profile(aess);

	return 0;
}

static void aess_irq_pingpong_subroutine(struct omap_aess *aess)
{
	struct snd_pcm_substream *substream;
	u32 dst, n_bytes;

	omap_aess_read_next_ping_pong_buffer(aess, OMAP_AESS_PHY_PORT_MM_DL,
					     &dst, &n_bytes);
	omap_aess_set_ping_pong_buffer(aess, OMAP_AESS_PHY_PORT_MM_DL, n_bytes);

	/* 1st IRQ does not signal completed period */
	if (aess->pingpong.first_irq) {
		aess->pingpong.first_irq = 0;
	} else {
		substream = omap_aess_port_get_substream(aess,
						OMAP_AESS_FE_PORT_MM_DL_LP);
		if (substream)
			snd_pcm_period_elapsed(substream);
	}
}

#define OMAP_AESS_IRQTAG_COUNT		0x000c
#define OMAP_AESS_IRQTAG_PP		0x000d
#define OMAP_AESS_D_MCUIRQFIFO_SIZE	0x40
#define OMAP_AESS_IRQ_FIFO_MASK		((OMAP_AESS_D_MCUIRQFIFO_SIZE >> 2) - 1)

static irqreturn_t aess_irq_handler(int irq, void *dev_id)
{
	struct omap_aess *aess = dev_id;
	struct omap_aess_pingppong *pp = &aess->pingpong;
	struct omap_aess_addr addr;
	struct omap_aess_irq_data IRQ_data;
	u32 aess_irq_dbg_write_ptr, i, cmem_src, sm_cm;

	pm_runtime_get_sync(aess->dev);

	/* Clear interrupts */
	omap_aess_reg_write(aess, OMAP_AESS_MCU_IRQSTATUS, 0x01);

	/*
	 * extract the write pointer index from CMEM memory (INITPTR format)
	 * CMEM address of the write pointer in bytes
	 */
	cmem_src = omap_aess_get_label_data(aess,
				OMAP_AESS_BUFFER_MCU_IRQ_FIFO_PTR_ID) << 2;
	omap_aess_read(aess, OMAP_AESS_BANK_CMEM, cmem_src, &sm_cm,
		       sizeof(sm_cm));
	/* AESS left-pointer index located on MSBs */
	aess_irq_dbg_write_ptr = sm_cm >> 16;
	aess_irq_dbg_write_ptr &= 0xFF;
	/* loop on the IRQ FIFO content */
	for (i = 0; i < OMAP_AESS_D_MCUIRQFIFO_SIZE; i++) {
		/* stop when the FIFO is empty */
		if (aess_irq_dbg_write_ptr == aess->irq_dbg_read_ptr)
			break;
		/* read the IRQ/DBG FIFO */
		memcpy(&addr, &aess->fw_info.map[OMAP_AESS_DMEM_MCUIRQFIFO_ID],
		       sizeof(struct omap_aess_addr));
		addr.offset += (aess->irq_dbg_read_ptr << 2);
		addr.bytes = sizeof(IRQ_data);
		omap_aess_mem_read(aess, addr, (u32 *)&IRQ_data);
		aess->irq_dbg_read_ptr =
			  (aess->irq_dbg_read_ptr + 1) & OMAP_AESS_IRQ_FIFO_MASK;
		/* select the source of the interrupt */
		switch (IRQ_data.tag) {
		case OMAP_AESS_IRQTAG_PP:
			/*
			 * first IRQ doesn't represent a buffer transference
			 * completion
			 */
			if (!pp->first_irq)
				pp->buf_id = (pp->buf_id + 1) & 0x03;

			aess_irq_pingpong_subroutine(aess);

			break;
		case OMAP_AESS_IRQTAG_COUNT:
			break;
		default:
			break;
		}

	}

	pm_runtime_put_sync_suspend(aess->dev);
	return IRQ_HANDLED;
}

static int omap_aess_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *dai = asoc_rtd_to_cpu(rtd, 0);
	struct snd_soc_component *component = dai->component;
	struct omap_aess *aess = snd_soc_component_get_drvdata(component);

	mutex_lock(&aess->mutex);

	dev_dbg(dai->dev, "%s: %s\n", __func__, dai->name);

	if (dai->id == OMAP_AESS_DAI_MM1_LP)
		snd_soc_set_runtime_hwparams(substream, &omap_aess_hardware);

	pm_runtime_get_sync(aess->dev);

	if (!aess->active++) {
		aess->opp.level = 0;
		aess_restore_context(aess);
		aess_opp_set_level(aess, 100);
		omap_aess_wakeup(aess);
	}

	mutex_unlock(&aess->mutex);
	return 0;
}

static int omap_aess_pcm_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct snd_soc_dai *dai = asoc_rtd_to_cpu(rtd, 0);
	struct snd_soc_component *component = dai->component;
	struct omap_aess *aess = snd_soc_component_get_drvdata(component);
	struct omap_aess_data_format format;
	size_t period_size;
	u32 dst;
	int ret = 0;

	mutex_lock(&aess->mutex);

	dev_dbg(dai->dev, "%s: %s\n", __func__, dai->name);

	if (dai->id != OMAP_AESS_DAI_MM1_LP)
		goto out;

	format.f = params_rate(params);
	if (params_format(params) == SNDRV_PCM_FORMAT_S32_LE)
		format.samp_format = OMAP_AESS_FORMAT_STEREO_MSB;
	else
		format.samp_format = OMAP_AESS_FORMAT_STEREO_16_16;

	period_size = params_period_bytes(params);

	/* Connect a Ping-Pong cache-flush protocol to MM_DL port */
	omap_aess_connect_irq_ping_pong_port(aess, OMAP_AESS_PHY_PORT_MM_DL,
					     &format, 0, period_size, &dst,
					     PING_PONG_WITH_MCU_IRQ);

	/* Memory mapping for hw params */
	runtime->dma_area  = (void __force *)(aess->io_base[0] + dst);
	runtime->dma_addr  = 0;
	runtime->dma_bytes = period_size * 4;

	/* Need to set the first buffer in order to get interrupt */
	omap_aess_set_ping_pong_buffer(aess, OMAP_AESS_PHY_PORT_MM_DL,
				       period_size);
out:
	mutex_unlock(&aess->mutex);
	return ret;
}

static int omap_aess_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *dai = asoc_rtd_to_cpu(rtd, 0);
	struct snd_soc_component *component = dai->component;
	struct omap_aess *aess = snd_soc_component_get_drvdata(component);

	dev_dbg(dai->dev, "%s: %s\n", __func__, dai->name);

	mutex_lock(&aess->mutex);
	/*
	 * TODO: do we need to set OPP here ? e.g.
	 * Startup() -> OPP100
	 * omap_aess_pcm_prepare() -> OPP0 (as widgets are not updated
	 *			      until after)
	 * stream_event() -> OPP25/50/100 (correct value based on widgets)
	 * aess_opp_recalc_level(aess);
	 */
	mutex_unlock(&aess->mutex);
	return 0;
}

static int omap_aess_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *dai = asoc_rtd_to_cpu(rtd, 0);
	struct snd_soc_component *component = dai->component;
	struct omap_aess *aess = snd_soc_component_get_drvdata(component);

	dev_dbg(dai->dev, "%s: %s\n", __func__, dai->name);

	mutex_lock(&aess->mutex);

	if (dai->id != OMAP_AESS_DAI_MM1_LP) {
	}

	if (!--aess->active) {
		omap_aess_disable_irq(aess);
		aess_save_context(aess);
		omap_aess_pm_shutdown(aess);
	} else {
		/* Only scale OPP level
		 * if AESS is still active */
		aess_opp_recalc_level(aess);
	}
	pm_runtime_put_sync(aess->dev);

	mutex_unlock(&aess->mutex);
	return 0;
}

static int omap_aess_pcm_mmap(struct snd_pcm_substream *substream,
			      struct vm_area_struct *vma)
{
	struct snd_soc_pcm_runtime  *rtd = substream->private_data;
	struct snd_soc_dai *dai = asoc_rtd_to_cpu(rtd, 0);
	struct snd_soc_component *component = dai->component;
	struct omap_aess *aess = snd_soc_component_get_drvdata(component);
	int offset, size, err;

	if (dai->id != OMAP_AESS_DAI_MM1_LP)
		return -EINVAL;

	vma->vm_flags |= VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	size = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;

	err = io_remap_pfn_range(vma, vma->vm_start, (aess->dmem_l4 +
			OMAP_AESS_BANK_DMEM_PP_OFFSET + offset) >> PAGE_SHIFT,
			size, vma->vm_page_prot);
	if (err)
		return -EAGAIN;

	return 0;
}

static snd_pcm_uframes_t omap_aess_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *dai = asoc_rtd_to_cpu(rtd, 0);
	struct snd_soc_component *component = dai->component;
	struct omap_aess *aess = snd_soc_component_get_drvdata(component);
	snd_pcm_uframes_t offset = 0;
	u32 pingpong;

	if (!aess->pingpong.first_irq) {
		omap_aess_read_offset_from_ping_buffer(aess,
						       OMAP_AESS_PHY_PORT_MM_DL,
						       &pingpong);
		offset = (snd_pcm_uframes_t)pingpong;
	}

	return offset;
}

static struct snd_pcm_ops omap_aess_pcm_ops = {
	.open           = omap_aess_pcm_open,
	.hw_params	= omap_aess_pcm_hw_params,
	.prepare	= omap_aess_pcm_prepare,
	.close	        = omap_aess_pcm_close,
	.pointer	= omap_aess_pcm_pointer,
	.mmap		= omap_aess_pcm_mmap,
};

static int omap_aess_pcm_probe(struct snd_soc_component *component)
{
	struct omap_aess *aess = snd_soc_component_get_drvdata(component);
	int ret = 0, i;

	pm_runtime_enable(aess->dev);
	pm_runtime_irq_safe(aess->dev);

	ret = snd_soc_tplg_component_load(component, &soc_tplg_ops,
					  aess->fw, 0);
	if (ret < 0) {
		dev_err(component->dev, "request for AESS FW failed %d\n", ret);
#if 0
/*
 * fw loading fails because header size seems to be wrong
 *	[    7.404714] aess 401f1000.aess: ASoC: invalid header size for type 0 at offset 0x0 size 0x1a5e8.
 */

		goto out;
#endif
	}

	ret = request_threaded_irq(aess->irq, NULL,
				   aess_irq_handler, IRQF_ONESHOT, "AESS",
				   aess);
	if (ret) {
		dev_err(component->dev, "request for AESS IRQ %d failed %d\n",
			aess->irq, ret);
		goto out;
	}

#if 0	// failed firmware load leads to missing/wrong opp and failed omap_aess_init_mem
	ret = aess_opp_init_initial_opp(aess);
	if (ret < 0) {
		dev_info(component->dev, "No OPP definition\n");
		ret = 0;
	}
	/* aess_clk has to be enabled to access hal register.
	 * Disable the clk after it has been used.
	 */
	pm_runtime_get_sync(aess->dev);

	omap_aess_init_mem(aess, aess->fw_config);

	omap_aess_reset_hal(aess);

	/* ZERO_labelID should really be 0 */
	for (i = 0; i < OMAP_AESS_ROUTES_UL + 2; i++)
		aess->mixer.route_ul[i] = omap_aess_get_label_data(aess,
						      OMAP_AESS_BUFFER_ZERO_ID);

	omap_aess_load_fw(aess, aess->fw_data);

	/* "tick" of the audio engine */
	omap_aess_write_event_generator(aess, EVENT_TIMER);

	/* Stop the engine */
	omap_aess_write_event_generator(aess, EVENT_STOP);
	omap_aess_disable_irq(aess);

	pm_runtime_put_sync(aess->dev);
#endif
	aess_init_debugfs(aess);

out:
	if (ret)
		pm_runtime_disable(aess->dev);

	return ret;
}

static void omap_aess_pcm_remove(struct snd_soc_component *component)
{
	struct omap_aess *aess = snd_soc_component_get_drvdata(component);

	free_irq(aess->irq, aess);
	aess_cleanup_debugfs(aess);
	pm_runtime_disable(aess->dev);
}

/* TODO: map IO directly into AESS memories */
static unsigned int omap_aess_oppwidget_read(struct snd_soc_component *component,
					     unsigned int reg)
{
	struct omap_aess *aess = snd_soc_component_get_drvdata(component);

	if (reg > OMAP_AESS_NUM_DAPM_REG)
		return 0;

	dev_dbg(component->dev, "read R%d (Ox%x) = 0x%x\n", reg, reg,
		aess->opp.widget[reg]);
	return aess->opp.widget[reg];
}

static int omap_aess_oppwidget_write(struct snd_soc_component *component,
				     unsigned int reg, unsigned int val)
{
	struct omap_aess *aess = snd_soc_component_get_drvdata(component);

	if (reg > OMAP_AESS_NUM_DAPM_REG)
		return 0;

	aess->opp.widget[reg] = val;
	dev_dbg(component->dev, "write R%d (Ox%x) = 0x%x\n", reg, reg, val);
	return 0;
}

static int omap_aess_pcm_stream_event(struct snd_soc_component *component,
				      int event)
{
	struct omap_aess *aess = snd_soc_component_get_drvdata(component);

	if (aess->active) {
		dev_dbg(aess->dev, "opp: stream event %d\n", event);
		aess_opp_recalc_level(aess);
	}

	return 0;
}

#ifdef CONFIG_PM
static int omap_aess_pcm_suspend(struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "%s: %s active %d\n", __func__,
		dai->name, snd_soc_dai_active(dai));

	return 0;
}

static int omap_aess_pcm_resume(struct snd_soc_dai *dai)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);
	int ret = 0;

	dev_dbg(dai->dev, "%s: %s active %d\n", __func__,
		dai->name, snd_soc_dai_active(dai));

	if (!snd_soc_dai_active(dai))
		return 0;

#ifdef FIXME	// mechanism does no longer exist
	/* context retained, no need to restore */
	if (aess->get_context_lost_count &&
	    aess->get_context_lost_count(aess->dev) == aess->context_lost)
		return 0;
	aess->context_lost = aess->get_context_lost_count(aess->dev);
#endif
	pm_runtime_get_sync(aess->dev);
	if (aess->device_scale) {
		ret = aess->device_scale(aess->dev, aess->dev,
					 aess->opp.freqs[OMAP_AESS_OPP_50]);
		if (ret) {
			dev_err(aess->dev, "failed to scale to OPP 50\n");
			goto out;
		}
	}

	omap_aess_reload_fw(aess, aess->fw_data);

	aess_reconfigure_profile(aess);
out:
	pm_runtime_put_sync(aess->dev);
	return ret;
}
#else
#define omap_aess_pcm_suspend	NULL
#define omap_aess_pcm_resume	NULL
#endif

struct snd_soc_component_driver omap_aess_platform = {
//	.ops		= &omap_aess_pcm_ops,
	.probe		= omap_aess_pcm_probe,
	.remove		= omap_aess_pcm_remove,
	.read		= omap_aess_oppwidget_read,
	.write		= omap_aess_oppwidget_write,
	.stream_event	= omap_aess_pcm_stream_event,
};
