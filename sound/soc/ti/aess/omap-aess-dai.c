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

#include <linux/delay.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dpcm.h>
#include <sound/dmaengine_pcm.h>

#include "omap-aess-priv.h"
#include "omap-aess.h"
#include "aess_gain.h"
#include "aess_port.h"
#include "aess_utils.h"
#include "port_mgr.h"
#include "aess_opp.h"

/*
 * Stream DMA parameters
 */
static struct snd_dmaengine_dai_dma_data omap_aess_dma_params[7][2];

/* For DMA line lookup within aess->dma_lines array */
static int omap_aess_dma_mapping[7][2] = {
	{ 0, 3 },
	{ -EINVAL, 4 },
	{ 1, 2 },
	{ 5, -EINVAL },
	{ 6, -EINVAL },
	{ 1, 2 },
	{ 0, -EINVAL },
};

static int omap_aess_dl1_enabled(struct omap_aess *aess)
{
	/* DL1 path is common for PDM_DL1, BT_VX_DL and MM_EXT_DL */
	return omap_aess_port_is_enabled(aess, OMAP_AESS_BE_PORT_PDM_DL1) +
		omap_aess_port_is_enabled(aess, OMAP_AESS_BE_PORT_BT_VX_DL) +
		omap_aess_port_is_enabled(aess, OMAP_AESS_BE_PORT_MM_EXT_DL);
}

static void mute_be(struct snd_soc_pcm_runtime *be, struct snd_soc_dai *dai,
		    int stream)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);

	dev_dbg(be->dev, "%s: %s %d\n", __func__, be->cpu_dai->name, stream);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (be->dai_link->id) {
		case OMAP_AESS_BE_ID_PDM_DL1:
		case OMAP_AESS_BE_ID_BT_VX:
		case OMAP_AESS_BE_ID_MM_FM:
			/*
			 * DL1 Mixer->SDT Mixer and DL1 gain are common for
			 * PDM_DL1, BT_VX_DL and MM_EXT_DL, mute those gains
			 * only if the last active BE
			 */
			if (omap_aess_dl1_enabled(aess) == 1) {
				omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DL1_LEFT);
				omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DL1_RIGHT);
				omap_aess_mute_gain(aess, OMAP_AESS_MIXSDT_DL);
			}
			break;
		case OMAP_AESS_BE_ID_PDM_DL2:
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DL2_LEFT);
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DL2_RIGHT);
			break;
		case OMAP_AESS_BE_ID_MODEM:
			break;
		}
	} else {
		switch (be->dai_link->id) {
		case OMAP_AESS_BE_ID_PDM_UL:
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_AMIC_LEFT);
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_AMIC_RIGHT);
			break;
		case OMAP_AESS_BE_ID_BT_VX:
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_BTUL_LEFT);
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_BTUL_RIGHT);
			break;
		case OMAP_AESS_BE_ID_MM_FM:
		case OMAP_AESS_BE_ID_MODEM:
			break;
		case OMAP_AESS_BE_ID_DMIC0:
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DMIC1_LEFT);
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DMIC1_RIGHT);
			break;
		case OMAP_AESS_BE_ID_DMIC1:
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DMIC2_LEFT);
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DMIC2_RIGHT);
			break;
		case OMAP_AESS_BE_ID_DMIC2:
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DMIC3_LEFT);
			omap_aess_mute_gain(aess, OMAP_AESS_GAIN_DMIC3_RIGHT);
			break;
		}
	}
}

static void unmute_be(struct snd_soc_pcm_runtime *be,struct snd_soc_dai *dai,
		      int stream)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);

	dev_dbg(be->dev, "%s: %s %d\n", __func__, be->cpu_dai->name, stream);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (be->dai_link->id) {
		case OMAP_AESS_BE_ID_PDM_DL1:
		case OMAP_AESS_BE_ID_BT_VX:
		case OMAP_AESS_BE_ID_MM_FM:
			/*
			 * DL1 Mixer->SDT Mixer and DL1 gain are common for
			 * PDM_DL1, BT_VX_DL and MM_EXT_DL, unmute when any
			 * of them becomes active
			 */
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_DL1_LEFT);
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_DL1_RIGHT);
			omap_aess_unmute_gain(aess, OMAP_AESS_MIXSDT_DL);
			break;
		case OMAP_AESS_BE_ID_PDM_DL2:
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_DL2_LEFT);
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_DL2_RIGHT);
			break;
		case OMAP_AESS_BE_ID_MODEM:
			break;
		}
	} else {

		switch (be->dai_link->id) {
		case OMAP_AESS_BE_ID_PDM_UL:
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_AMIC_LEFT);
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_AMIC_RIGHT);
			break;
		case OMAP_AESS_BE_ID_BT_VX:
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_BTUL_LEFT);
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_BTUL_RIGHT);
			break;
		case OMAP_AESS_BE_ID_MM_FM:
		case OMAP_AESS_BE_ID_MODEM:
			break;
		case OMAP_AESS_BE_ID_DMIC0:
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_DMIC1_LEFT);
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_DMIC1_RIGHT);
			break;
		case OMAP_AESS_BE_ID_DMIC1:
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_DMIC2_LEFT);
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_DMIC2_RIGHT);
			break;
		case OMAP_AESS_BE_ID_DMIC2:
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_DMIC3_LEFT);
			omap_aess_unmute_gain(aess, OMAP_AESS_GAIN_DMIC3_RIGHT);
			break;
		}
	}
}

static void enable_be_port(struct snd_soc_pcm_runtime *be,
			   struct snd_soc_dai *dai, int stream)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);
	struct omap_aess_data_format format;

	dev_dbg(be->dev, "%s: %s %d\n", __func__, be->cpu_dai->name, stream);

	switch (be->dai_link->id) {
	case OMAP_AESS_BE_ID_PDM_DL1:
	case OMAP_AESS_BE_ID_PDM_DL2:
	case OMAP_AESS_BE_ID_PDM_UL:
		/* McPDM is a special case, handled by McPDM driver */
		break;
	case OMAP_AESS_BE_ID_BT_VX:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {

			/* port can only be configured if it's not running */
			if (omap_aess_port_is_enabled(aess,
						OMAP_AESS_BE_PORT_BT_VX_DL))
				return;

			/* BT_DL connection to McBSP 1 ports */
			format.f = 8000;
			format.samp_format = OMAP_AESS_FORMAT_STEREO_RSHIFTED_16;
			omap_aess_connect_serial_port(aess,
						      OMAP_AESS_PHY_PORT_BT_VX_DL,
						      &format, MCBSP1_TX, NULL);
			omap_aess_port_enable(aess, OMAP_AESS_BE_PORT_BT_VX_DL);
		} else {

			/* port can only be configured if it's not running */
			if (omap_aess_port_is_enabled(aess,
						OMAP_AESS_BE_PORT_BT_VX_UL))
				return;

			/* BT_UL connection to McBSP 1 ports */
			format.f = 8000;
			format.samp_format = OMAP_AESS_FORMAT_STEREO_RSHIFTED_16;
			omap_aess_connect_serial_port(aess,
						      OMAP_AESS_PHY_PORT_BT_VX_UL,
						      &format, MCBSP1_RX, NULL);
			omap_aess_port_enable(aess, OMAP_AESS_BE_PORT_BT_VX_UL);
		}
		break;
	case OMAP_AESS_BE_ID_MM_FM:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {

			/* port can only be configured if it's not running */
			if (omap_aess_port_is_enabled(aess,
						OMAP_AESS_BE_PORT_MM_EXT_DL))
				return;

			/* MM_EXT connection to McBSP 2 ports */
			format.f = 48000;
			format.samp_format = OMAP_AESS_FORMAT_STEREO_RSHIFTED_16;
			omap_aess_connect_serial_port(aess,
						      OMAP_AESS_PHY_PORT_MM_EXT_OUT,
						      &format, MCBSP2_TX, NULL);
			omap_aess_port_enable(aess,
					      OMAP_AESS_BE_PORT_MM_EXT_DL);
		} else {

			/* port can only be configured if it's not running */
			if (omap_aess_port_is_enabled(aess,
						OMAP_AESS_BE_PORT_MM_EXT_UL))
				return;

			/* MM_EXT connection to McBSP 2 ports */
			format.f = 48000;
			format.samp_format = OMAP_AESS_FORMAT_STEREO_RSHIFTED_16;
			omap_aess_connect_serial_port(aess,
						      OMAP_AESS_PHY_PORT_MM_EXT_IN,
						      &format, MCBSP2_RX, NULL);
			omap_aess_port_enable(aess, OMAP_AESS_BE_PORT_MM_EXT_UL);
		}
		break;
	case OMAP_AESS_BE_ID_DMIC0:
		omap_aess_port_enable(aess, OMAP_AESS_BE_PORT_DMIC0);
		break;
	case OMAP_AESS_BE_ID_DMIC1:
		omap_aess_port_enable(aess, OMAP_AESS_BE_PORT_DMIC1);
		break;
	case OMAP_AESS_BE_ID_DMIC2:
		omap_aess_port_enable(aess, OMAP_AESS_BE_PORT_DMIC2);
		break;
	}
}

static void enable_fe_port(struct snd_pcm_substream *substream,
			   struct snd_soc_dai *dai, int stream)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);

	dev_dbg(fe->dev, "%s: %s %d\n", __func__, dai->name, stream);

	switch (dai->id) {
	case OMAP_AESS_DAI_MM1:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_port_enable(aess, OMAP_AESS_FE_PORT_MM_DL1);
		else
			omap_aess_port_enable(aess, OMAP_AESS_FE_PORT_MM_UL1);
		break;
	case OMAP_AESS_DAI_MM1_LP:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			omap_aess_port_enable(aess, OMAP_AESS_FE_PORT_MM_DL_LP);
			omap_aess_port_set_substream(aess,
						     OMAP_AESS_FE_PORT_MM_DL_LP,
						     substream);
		}
		break;
	case OMAP_AESS_DAI_MM2_CAPTURE:
		if (stream == SNDRV_PCM_STREAM_CAPTURE)
			omap_aess_port_enable(aess, OMAP_AESS_FE_PORT_MM_UL2);
		break;
	case OMAP_AESS_DAI_MODEM:
	case OMAP_AESS_DAI_VOICE:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_port_enable(aess, OMAP_AESS_FE_PORT_VX_DL);
		else
			omap_aess_port_enable(aess, OMAP_AESS_FE_PORT_VX_UL);
		break;
	case OMAP_AESS_DAI_TONES:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_port_enable(aess, OMAP_AESS_FE_PORT_TONES);
		break;
	}
}

static void disable_be_port(struct snd_soc_pcm_runtime *be,
			    struct snd_soc_dai *dai, int stream)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);

	dev_dbg(be->dev, "%s: %s %d\n", __func__, be->cpu_dai->name, stream);

	switch (be->dai_link->id) {
	/* McPDM is a special case, handled by McPDM driver */
	case OMAP_AESS_BE_ID_PDM_DL1:
	case OMAP_AESS_BE_ID_PDM_DL2:
	case OMAP_AESS_BE_ID_PDM_UL:
		break;
	case OMAP_AESS_BE_ID_BT_VX:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_port_disable(aess,
					       OMAP_AESS_BE_PORT_BT_VX_DL);
		else
			omap_aess_port_disable(aess,
					       OMAP_AESS_BE_PORT_BT_VX_UL);
		break;
	case OMAP_AESS_BE_ID_MM_FM:
	case OMAP_AESS_BE_ID_MODEM:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_port_disable(aess,
					       OMAP_AESS_BE_PORT_MM_EXT_DL);
		else
			omap_aess_port_disable(aess,
					       OMAP_AESS_BE_PORT_MM_EXT_UL);
		break;
	case OMAP_AESS_BE_ID_DMIC0:
		omap_aess_port_disable(aess, OMAP_AESS_BE_PORT_DMIC0);
		break;
	case OMAP_AESS_BE_ID_DMIC1:
		omap_aess_port_disable(aess, OMAP_AESS_BE_PORT_DMIC1);
		break;
	case OMAP_AESS_BE_ID_DMIC2:
		omap_aess_port_disable(aess, OMAP_AESS_BE_PORT_DMIC2);
		break;
	}
}

static void disable_fe_port(struct snd_pcm_substream *substream,
			    struct snd_soc_dai *dai, int stream)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);

	dev_dbg(fe->dev, "%s: %s %d\n", __func__, dai->name, stream);

	switch (dai->id) {
	case OMAP_AESS_DAI_MM1:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_port_disable(aess, OMAP_AESS_FE_PORT_MM_DL1);
		else
			omap_aess_port_disable(aess, OMAP_AESS_FE_PORT_MM_UL1);
		break;
	case OMAP_AESS_DAI_MM1_LP:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK) {
			omap_aess_port_disable(aess,
					       OMAP_AESS_FE_PORT_MM_DL_LP);
			omap_aess_port_set_substream(aess,
						    OMAP_AESS_FE_PORT_MM_DL_LP,
						    NULL);
		}
		break;
	case OMAP_AESS_DAI_MM2_CAPTURE:
		if (stream == SNDRV_PCM_STREAM_CAPTURE)
			omap_aess_port_disable(aess, OMAP_AESS_FE_PORT_MM_UL2);
		break;
	case OMAP_AESS_DAI_MODEM:
	case OMAP_AESS_DAI_VOICE:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_port_disable(aess, OMAP_AESS_FE_PORT_VX_DL);
		else
			omap_aess_port_disable(aess, OMAP_AESS_FE_PORT_VX_UL);
		break;
	case OMAP_AESS_DAI_TONES:
		if (stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_port_disable(aess, OMAP_AESS_FE_PORT_TONES);
		break;
	}
}

static void mute_fe_port_capture(struct snd_soc_pcm_runtime *fe,
				 struct snd_soc_dai *dai, int mute)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);

	dev_dbg(fe->dev, "%s: %s FE %s\n", __func__, mute ? "mute" : "unmute",
		fe->dai_link->name);

	switch (fe->cpu_dai->id) {
	case OMAP_AESS_DAI_MM2_CAPTURE:
		if (mute) {
			omap_aess_mute_gain(aess, OMAP_AESS_MIXDL1_MM_UL2);
			omap_aess_mute_gain(aess, OMAP_AESS_MIXDL2_MM_UL2);
		} else {
			omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL1_MM_UL2);
			omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL2_MM_UL2);
		}
		break;
	case OMAP_AESS_DAI_MODEM:
	case OMAP_AESS_DAI_VOICE:
		if (mute) {
			omap_aess_mute_gain(aess, OMAP_AESS_MIXSDT_UL);
			omap_aess_mute_gain(aess, OMAP_AESS_MIXAUDUL_UPLINK);
		} else {
			omap_aess_unmute_gain(aess, OMAP_AESS_MIXSDT_UL);
			omap_aess_unmute_gain(aess, OMAP_AESS_MIXAUDUL_UPLINK);
		}
		break;
	case OMAP_AESS_DAI_MM1:
	default:
		break;
	}
}

static void mute_fe_port_playback(struct snd_soc_pcm_runtime *fe,
				  struct snd_soc_dai *dai, int mute)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);

	dev_dbg(fe->dev, "%s: %s FE %s\n", __func__, mute ? "mute" : "unmute",
		fe->dai_link->name);

	switch (fe->cpu_dai->id) {
	case OMAP_AESS_DAI_MM1:
	case OMAP_AESS_DAI_MM1_LP:
		if (mute) {
			omap_aess_mute_gain(aess, OMAP_AESS_MIXDL1_MM_DL);
			omap_aess_mute_gain(aess, OMAP_AESS_MIXDL2_MM_DL);
		} else {
			omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL1_MM_DL);
			omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL2_MM_DL);
		}
		break;
	case OMAP_AESS_DAI_VOICE:
	case OMAP_AESS_DAI_MODEM:
		if (mute) {
			omap_aess_mute_gain(aess, OMAP_AESS_MIXDL1_VX_DL);
			omap_aess_mute_gain(aess, OMAP_AESS_MIXDL2_VX_DL);
		} else {
			omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL1_VX_DL);
			omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL2_VX_DL);
		}
		break;
	case OMAP_AESS_DAI_TONES:
		if (mute) {
			omap_aess_mute_gain(aess, OMAP_AESS_MIXDL1_TONES);
			omap_aess_mute_gain(aess, OMAP_AESS_MIXDL2_TONES);
		} else {
			omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL1_TONES);
			omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL2_TONES);
		}
		break;
	default:
		break;
	}
}

static void mute_fe_port(struct snd_pcm_substream *substream,
			 struct snd_soc_dai *dai, int stream)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;

	dev_dbg(fe->dev, "%s: %s %d\n", __func__, dai->name, stream);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		mute_fe_port_playback(fe, dai, 1);
	else
		mute_fe_port_capture(fe, dai, 1);
}

static void unmute_fe_port(struct snd_pcm_substream *substream,
			   struct snd_soc_dai *dai, int stream)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;

	dev_dbg(fe->dev, "%s: %s %d\n", __func__, dai->name, stream);

	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		mute_fe_port_playback(fe, dai, 0);
	else
		mute_fe_port_capture(fe, dai, 0);
}

static void capture_trigger(struct snd_pcm_substream *substream,
			    struct snd_soc_dai *dai, int cmd)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;
	int stream = substream->stream;
	struct list_head *be_clients = &fe->dpcm[stream].be_clients;
	struct snd_soc_dpcm *dpcm;
	struct snd_pcm_substream *be_substream;
	enum snd_soc_dpcm_state state;

	dev_dbg(fe->dev, "%s: %s %d\n", __func__, fe->cpu_dai->name, stream);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:

		/* mute and enable BE ports */
		list_for_each_entry(dpcm, be_clients, list_be) {
			struct snd_soc_pcm_runtime *be = dpcm->be;

			/* does this trigger() apply to this BE and stream ? */
			if (!snd_soc_dpcm_be_can_update(fe, be, stream))
				continue;

			/* we only start BEs that are prepared or stopped */
			state = snd_soc_dpcm_be_get_state(be, stream);
			if ((state != SND_SOC_DPCM_STATE_PREPARE) &&
			    (state != SND_SOC_DPCM_STATE_STOP))
				continue;

			be_substream = snd_soc_dpcm_get_substream(be, stream);

			/* mute the BE port */
			mute_be(be, dai, stream);

			/* enable the BE port */
			enable_be_port(be, dai, stream);

			/* DAI work must be started at least 250us after AESS */
			udelay(250);

			/* trigger the BE port */
			snd_soc_dai_trigger(be_substream, cmd, be->cpu_dai);

			snd_soc_dpcm_be_set_state(be, stream,
						  SND_SOC_DPCM_STATE_START);
		}

		/* does this trigger() apply to the FE ? */
		if (snd_soc_dpcm_fe_can_update(fe, stream)) {
			/* Enable Frontend sDMA  */
			snd_soc_platform_trigger(substream, cmd, fe->platform);

			enable_fe_port(substream, dai, stream);
			/* unmute FE port */
			unmute_fe_port(substream, dai, stream);
		}

		/* Restore AESS GAINS AMIC */
		list_for_each_entry(dpcm, be_clients, list_be) {
			struct snd_soc_pcm_runtime *be = dpcm->be;

			/* does this trigger() apply to this BE and stream ? */
			if (!snd_soc_dpcm_be_can_update(fe, be, stream))
				continue;

			/* unmute this BE port */
			unmute_be(be, dai, stream);
		}
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		/* Enable sDMA */
		snd_soc_platform_trigger(substream, cmd, fe->platform);
		enable_fe_port(substream, dai, stream);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/* Disable sDMA */
		disable_fe_port(substream, dai, stream);
		snd_soc_platform_trigger(substream, cmd, fe->platform);
		break;
	case SNDRV_PCM_TRIGGER_STOP:

		/* does this trigger() apply to the FE ? */
		if (snd_soc_dpcm_fe_can_update(fe, stream)) {
			/* mute FE port */
			mute_fe_port(substream, dai, stream);
			/* Disable sDMA */
			disable_fe_port(substream, dai, stream);
			snd_soc_platform_trigger(substream, cmd, fe->platform);
		}

		/* disable BE ports */
		list_for_each_entry(dpcm, be_clients, list_be) {
			struct snd_soc_pcm_runtime *be = dpcm->be;

			/* does this trigger() apply to this BE and stream ? */
			if (!snd_soc_dpcm_be_can_update(fe, be, stream))
				continue;

			/* we dont need to stop BEs that are already stopped */
			state = snd_soc_dpcm_be_get_state(be, stream);
			if (state != SND_SOC_DPCM_STATE_START)
				continue;

			/* only stop if last running user */
			if (!snd_soc_dpcm_can_be_free_stop(fe, be, stream))
				continue;

			be_substream = snd_soc_dpcm_get_substream(be, stream);

			/* mute the BE port */
			mute_be(be, dai, stream);

			/* disable the BE port */
			disable_be_port(be, dai, stream);

			/* DAI work must be stopped at least 250us after AESS */
			udelay(250);

			/* trigger BE port */
			snd_soc_dai_trigger(be_substream, cmd, be->cpu_dai);

			snd_soc_dpcm_be_set_state(be, stream,
						  SND_SOC_DPCM_STATE_STOP);
		}
		break;
	default:
		break;
	}
}

static void playback_trigger(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai, int cmd)
{
	struct snd_soc_pcm_runtime *fe = substream->private_data;
	int stream = substream->stream;
	struct list_head *be_clients = &fe->dpcm[stream].be_clients;
	struct snd_soc_dpcm *dpcm;
	struct snd_pcm_substream *be_substream;
	enum snd_soc_dpcm_state state;

	dev_dbg(fe->dev, "%s: %s %d\n", __func__, fe->cpu_dai->name, stream);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:

		/* mute and enable ports */
		list_for_each_entry(dpcm, be_clients, list_be) {
			struct snd_soc_pcm_runtime *be = dpcm->be;

			/* does this trigger() apply to the FE ? */
			if (!snd_soc_dpcm_be_can_update(fe, be, stream))
				continue;

			/* we only start BEs that are prepared or stopped */
			state = snd_soc_dpcm_be_get_state(be, stream);
			if ((state != SND_SOC_DPCM_STATE_PREPARE) &&
			    (state != SND_SOC_DPCM_STATE_STOP))
				continue;

			be_substream = snd_soc_dpcm_get_substream(be, stream);

			/* mute BE port */
			mute_be(be, dai, stream);

			/* enabled BE port */
			enable_be_port(be, dai, stream);

			/* DAI work must be started at least 250us after AESS */
			udelay(250);

			/* trigger BE port */
			snd_soc_dai_trigger(be_substream, cmd, be->cpu_dai);

			/* unmute the BE port */
			unmute_be(be, dai, stream);

			snd_soc_dpcm_be_set_state(be, stream,
						  SND_SOC_DPCM_STATE_START);
		}

		/* does this trigger() apply to the FE ? */
		if (snd_soc_dpcm_fe_can_update(fe, stream)) {

			/* Enable Frontend sDMA  */
			snd_soc_platform_trigger(substream, cmd, fe->platform);
			enable_fe_port(substream, dai, stream);
		}

		/* unmute FE port (sensitive to runtime udpates) */
		unmute_fe_port(substream, dai, stream);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		/* Enable Frontend sDMA  */
		snd_soc_platform_trigger(substream, cmd, fe->platform);
		enable_fe_port(substream, dai, stream);

		/* unmute FE port */
		unmute_fe_port(substream, dai, stream);
		break;
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		/* mute FE port */
		mute_fe_port(substream, dai, stream);
		/* disable Frontend sDMA  */
		disable_fe_port(substream, dai, stream);
		snd_soc_platform_trigger(substream, cmd, fe->platform);

		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
		/* does this trigger() apply to the FE ? */
		if (snd_soc_dpcm_fe_can_update(fe, stream)) {
			/* mute FE port (sensitive to runtime udpates) */
			mute_fe_port(substream, dai, stream);
			/* disable the transfer */
			disable_fe_port(substream, dai, stream);
			snd_soc_platform_trigger(substream, cmd, fe->platform);

		}

		/* disable BE ports */
		list_for_each_entry(dpcm, be_clients, list_be) {
			struct snd_soc_pcm_runtime *be = dpcm->be;

			/* does this trigger() apply to this BE and stream ? */
			if (!snd_soc_dpcm_be_can_update(fe, be, stream))
				continue;

			/* we dont need to stop BEs that are already stopped */
			state = snd_soc_dpcm_be_get_state(be, stream);
			if (state != SND_SOC_DPCM_STATE_START)
				continue;

			/* only stop if last running user */
			if (!snd_soc_dpcm_can_be_free_stop(fe, be, stream))
				continue;

			be_substream = snd_soc_dpcm_get_substream(be, stream);

			/* mute the BE port */
			mute_be(be, dai, stream);

			/* disable the BE */
			disable_be_port(be, dai, stream);

			/* DAI work must be stopped at least 250us after AESS */
			udelay(250);

			/*  trigger the BE port */
			snd_soc_dai_trigger(be_substream, cmd, be->cpu_dai);

			snd_soc_dpcm_be_set_state(be, stream,
						  SND_SOC_DPCM_STATE_STOP);
		}
		break;
	default:
		break;
	}
}

static int omap_aess_hwrule_size_step(struct snd_pcm_hw_params *params,
				      struct snd_pcm_hw_rule *rule)
{
	unsigned int rate = params_rate(params);

	/* 44.1kHz has the same iteration number as 48kHz */
	rate = (rate == 44100) ? 48000 : rate;

	/* AESS requires chunks of 250us worth of data */
	return snd_interval_step(hw_param_interval(params, rule->var),
				 rate / 4000);
}

static char *dma_names[OMAP_AESS_DMA_RESOURCES] = {
	"fifo0", "fifo1", "fifo2", "fifo3", "fifo4", "fifo5", "fifo6", "fifo7" };

static int omap_aess_dai_startup(struct snd_pcm_substream *substream,
				 struct snd_soc_dai *dai)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);
	int dma_index = omap_aess_dma_mapping[dai->id][substream->stream];
	struct snd_dmaengine_dai_dma_data *dma_data;

	dev_dbg(dai->dev, "%s: %s\n", __func__, dai->name);

	mutex_lock(&aess->mutex);
	aess->dai.num_active++;

	switch (dai->id) {
	case OMAP_AESS_DAI_MODEM:
		break;
	case OMAP_AESS_DAI_MM1_LP:
		snd_pcm_hw_constraint_step(substream->runtime, 0,
					   SNDRV_PCM_HW_PARAM_BUFFER_BYTES,
					   1024);
		break;
	default:
		/*
		 * Period and buffer size must be aligned with the Audio Engine
		 * processing loop which is 250 us long
		 */
		snd_pcm_hw_rule_add(substream->runtime, 0,
				    SNDRV_PCM_HW_PARAM_PERIOD_SIZE,
				    omap_aess_hwrule_size_step, NULL,
				    SNDRV_PCM_HW_PARAM_PERIOD_SIZE, -1);
		snd_pcm_hw_rule_add(substream->runtime, 0,
				    SNDRV_PCM_HW_PARAM_BUFFER_SIZE,
				    omap_aess_hwrule_size_step, NULL,
				    SNDRV_PCM_HW_PARAM_BUFFER_SIZE, -1);
		break;
	}

	mutex_unlock(&aess->mutex);

	dma_data = &omap_aess_dma_params[dai->id][substream->stream];
	dma_data->filter_data = dma_names[dma_index];
	dma_data->addr_width = DMA_SLAVE_BUSWIDTH_4_BYTES;

	snd_soc_dai_set_dma_data(dai, substream, dma_data);

	return 0;
}

static int omap_aess_dai_hw_params(struct snd_pcm_substream *substream,
				   struct snd_pcm_hw_params *params,
				   struct snd_soc_dai *dai)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);
	struct snd_dmaengine_dai_dma_data *dma_data;
	struct omap_aess_data_format format;
	struct omap_aess_dma dma_params;

	dev_dbg(dai->dev, "%s: %s\n", __func__, dai->name);

	dma_data = snd_soc_dai_get_dma_data(dai, substream);
	switch (params_channels(params)) {
	case 1:
		if (params_format(params) == SNDRV_PCM_FORMAT_S16_LE)
			format.samp_format = OMAP_AESS_FORMAT_MONO_16_16;
		else
			format.samp_format = OMAP_AESS_FORMAT_MONO_MSB;
		break;
	case 2:
		if (params_format(params) == SNDRV_PCM_FORMAT_S16_LE)
			format.samp_format = OMAP_AESS_FORMAT_STEREO_16_16;
		else
			format.samp_format = OMAP_AESS_FORMAT_STEREO_MSB;
		break;
	case 3:
		format.samp_format = OMAP_AESS_FORMAT_THREE_MSB;
		break;
	case 4:
		format.samp_format = OMAP_AESS_FORMAT_FOUR_MSB;
		break;
	case 5:
		format.samp_format = OMAP_AESS_FORMAT_FIVE_MSB;
		break;
	case 6:
		format.samp_format = OMAP_AESS_FORMAT_SIX_MSB;
		break;
	case 7:
		format.samp_format = OMAP_AESS_FORMAT_SEVEN_MSB;
		break;
	case 8:
		format.samp_format = OMAP_AESS_FORMAT_EIGHT_MSB;
		break;
	default:
		dev_err(dai->dev, "%d channels not supported",
			params_channels(params));
		return -EINVAL;
	}

	format.f = params_rate(params);

	switch (dai->id) {
	case OMAP_AESS_DAI_MM1:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_connect_cbpr_dmareq_port(aess,
					OMAP_AESS_PHY_PORT_MM_DL, &format,
					AESS_CBPR0_IDX, &dma_params);
		else
			omap_aess_connect_cbpr_dmareq_port(aess,
					OMAP_AESS_PHY_PORT_MM_UL, &format,
					AESS_CBPR3_IDX, &dma_params);
		break;
	case OMAP_AESS_DAI_MM1_LP:
		return 0;
		break;
	case OMAP_AESS_DAI_MM2_CAPTURE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			return -EINVAL;
		else
			omap_aess_connect_cbpr_dmareq_port(aess,
					OMAP_AESS_PHY_PORT_MM_UL2, &format,
					AESS_CBPR4_IDX, &dma_params);
		break;
	case OMAP_AESS_DAI_VOICE:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_connect_cbpr_dmareq_port(aess,
					OMAP_AESS_PHY_PORT_VX_DL, &format,
					AESS_CBPR1_IDX, &dma_params);
		else
			omap_aess_connect_cbpr_dmareq_port(aess,
					OMAP_AESS_PHY_PORT_VX_UL, &format,
					AESS_CBPR2_IDX, &dma_params);
		break;
	case OMAP_AESS_DAI_TONES:
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_connect_cbpr_dmareq_port(aess,
					OMAP_AESS_PHY_PORT_TONES_DL, &format,
					AESS_CBPR5_IDX, &dma_params);
		else
			return -EINVAL;
		break;
	case OMAP_AESS_DAI_MODEM:

		/* MODEM is special case where data IO is performed by McBSP2
		 * directly onto VX_DL and VX_UL (instead of SDMA).
		 */
		format.samp_format = OMAP_AESS_FORMAT_STEREO_RSHIFTED_16;

		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			omap_aess_connect_serial_port(aess,
					OMAP_AESS_PHY_PORT_VX_DL, &format,
					MCBSP2_RX, &dma_params);
		else
			omap_aess_connect_serial_port(aess,
					OMAP_AESS_PHY_PORT_VX_UL, &format,
					MCBSP2_TX, &dma_params);
		break;
	default:
		dev_err(dai->dev, "port %d not supported\n", dai->id);
		return -EINVAL;
	}

	/* configure frontend SDMA data */
	dma_data->addr = (unsigned long)dma_params.data;
	dma_data->maxburst = dma_params.iter;

	return 0;
}


static int omap_aess_dai_bespoke_trigger(struct snd_pcm_substream *substream,
					 int cmd, struct snd_soc_dai *dai)
{
	dev_dbg(dai->dev, "%s: %s cmd %d\n", __func__, dai->name, cmd);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		playback_trigger(substream, dai, cmd);
	else
		capture_trigger(substream, dai, cmd);

	return 0;
}


static void omap_aess_dai_shutdown(struct snd_pcm_substream *substream,
				   struct snd_soc_dai *dai)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);
	int err;

	dev_dbg(dai->dev, "%s: %s\n", __func__, dai->name);

	mutex_lock(&aess->mutex);
	/* shutdown the AESS if last user */
	if (!aess->active && !omap_aess_check_activity(aess)) {
		aess_set_opp_processing(aess, OMAP_AESS_OPP_25);
		aess->opp.level = 25;
		omap_aess_write_event_generator(aess, EVENT_STOP);
		udelay(250);
		if (aess->device_scale) {
			err = aess->device_scale(aess->dev, aess->dev,
						 aess->opp.freqs[0]);
			if (err)
				dev_err(aess->dev,
					"failed to scale to lowest OPP\n");
		}
	}

	aess->dai.num_active--;
	mutex_unlock(&aess->mutex);
}

#ifdef CONFIG_PM
static int omap_aess_dai_suspend(struct snd_soc_dai *dai)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);

	dev_dbg(dai->dev, "%s: %s active %d\n", __func__,
		dai->name, dai->active);

	if (!dai->active)
		return 0;

	if (++aess->dai.num_suspended < aess->dai.num_active)
		return 0;

	omap_aess_mute_gain(aess, OMAP_AESS_MIXSDT_UL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXSDT_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXAUDUL_MM_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXAUDUL_TONES);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXAUDUL_UPLINK);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXAUDUL_VX_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXVXREC_TONES);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXVXREC_VX_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXVXREC_MM_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXVXREC_VX_UL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXDL1_MM_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXDL1_MM_UL2);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXDL1_VX_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXDL1_TONES);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXDL2_MM_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXDL2_MM_UL2);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXDL2_VX_DL);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXDL2_TONES);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXECHO_DL1);
	omap_aess_mute_gain(aess, OMAP_AESS_MIXECHO_DL2);

	return 0;
}

static int omap_aess_dai_resume(struct snd_soc_dai *dai)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);

	dev_dbg(dai->dev, "%s: %s active %d\n", __func__,
		dai->name, dai->active);

	if (!dai->active)
		return 0;

	if (aess->dai.num_suspended-- < aess->dai.num_active)
		return 0;

	omap_aess_unmute_gain(aess, OMAP_AESS_MIXSDT_UL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXSDT_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXAUDUL_MM_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXAUDUL_TONES);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXAUDUL_UPLINK);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXAUDUL_VX_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXVXREC_TONES);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXVXREC_VX_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXVXREC_MM_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXVXREC_VX_UL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL1_MM_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL1_MM_UL2);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL1_VX_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL1_TONES);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL2_MM_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL2_MM_UL2);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL2_VX_DL);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXDL2_TONES);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXECHO_DL1);
	omap_aess_unmute_gain(aess, OMAP_AESS_MIXECHO_DL2);

	return 0;
}
#else
#define omap_aess_dai_suspend	NULL
#define omap_aess_dai_resume		NULL
#endif

static int omap_aess_dai_probe(struct snd_soc_dai *dai)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);
	int ret = 0;
	int i;

	for (i = 0; i < OMAP_AESS_LAST_LOGICAL_PORT_ID; i++) {
		/* Skip McPDM DL1 and UL1 since they are handled differently */
		if (i == OMAP_AESS_BE_PORT_PDM_DL1 ||
		    i == OMAP_AESS_BE_PORT_PDM_UL1)
			continue;

		ret = omap_aess_port_open(aess, i);
		if (ret)
			goto err_port;
	}

	return 0;

err_port:
	for (--i; i >= 0; i--) {
		/* Skip McPDM DL1 and UL1 since they are handled differently */
		if (i == OMAP_AESS_BE_PORT_PDM_DL1 ||
		    i == OMAP_AESS_BE_PORT_PDM_UL1)
			continue;

		omap_aess_port_close(aess, i);
	}

	return ret;
}

static int omap_aess_dai_remove(struct snd_soc_dai *dai)
{
	struct omap_aess *aess = snd_soc_dai_get_drvdata(dai);
	int i;

	for (i = 0; i < OMAP_AESS_LAST_LOGICAL_PORT_ID; i++) {
		/* Skip McPDM DL1 and UL1 since they are handled differently */
		if (i == OMAP_AESS_BE_PORT_PDM_DL1 ||
		    i == OMAP_AESS_BE_PORT_PDM_UL1)
			continue;

		omap_aess_port_close(aess, i);
	}

	return 0;
}

static struct snd_soc_dai_ops omap_aess_dai_ops = {
	.startup	= omap_aess_dai_startup,
	.shutdown	= omap_aess_dai_shutdown,
	.hw_params	= omap_aess_dai_hw_params,
	.bespoke_trigger = omap_aess_dai_bespoke_trigger,
};

struct snd_soc_dai_driver omap_aess_dai[] = {
	{	/* Multimedia Playback and Capture */
		.name = "MultiMedia1",
		.id = OMAP_AESS_DAI_MM1,
		.probe = omap_aess_dai_probe,
		.remove = omap_aess_dai_remove,
		.suspend = omap_aess_dai_suspend,
		.resume = omap_aess_dai_resume,
		.playback = {
			.stream_name = "MM1 Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "MM1 Capture",
			.channels_min = 1,
			.channels_max = 6,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &omap_aess_dai_ops,
	},
	{	/* Multimedia Capture */
		.name = "MultiMedia2",
		.id = OMAP_AESS_DAI_MM2_CAPTURE,
		.suspend = omap_aess_dai_suspend,
		.resume = omap_aess_dai_resume,
		.capture = {
			.stream_name = "MM2 Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &omap_aess_dai_ops,
	},
	{	/* Voice Playback and Capture */
		.name = "Voice",
		.id = OMAP_AESS_DAI_VOICE,
		.suspend = omap_aess_dai_suspend,
		.resume = omap_aess_dai_resume,
		.playback = {
			.stream_name = "Voice Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
				 SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "Voice Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |
				 SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &omap_aess_dai_ops,
	},
	{	/* Tones Playback */
		.name = "Tones",
		.id = OMAP_AESS_DAI_TONES,
		.suspend = omap_aess_dai_suspend,
		.resume = omap_aess_dai_resume,
		.playback = {
			.stream_name = "Tones Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &omap_aess_dai_ops,
	},
	{	/* MODEM Voice Playback and Capture */
		.name = "MODEM",
		.id = OMAP_AESS_DAI_MODEM,
		.suspend = omap_aess_dai_suspend,
		.resume = omap_aess_dai_resume,
		.playback = {
			.stream_name = "Modem Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.capture = {
			.stream_name = "Modem Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &omap_aess_dai_ops,
	},
	{	/* Low Power HiFi Playback */
		.name = "MultiMedia1 LP",
		.id = OMAP_AESS_DAI_MM1_LP,
		.suspend = omap_aess_dai_suspend,
		.resume = omap_aess_dai_resume,
		.playback = {
			.stream_name = "MMLP Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE |
				   SNDRV_PCM_FMTBIT_S32_LE,
		},
		.ops = &omap_aess_dai_ops,
	},
};
