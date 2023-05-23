// SPDX-License-Identifier: GPL-2.0-only

/*
 * omap-abe-twl6040.c  --  SoC audio for TI OMAP based boards with ABE and
 *			   twl6040 codec
 *
 * This is a mix of upstream omap-abe-twl6040.c from 4.18ff with TI code
 * Author: H. N. Schaller <hns@goldelico.com> by porting to OMAP5 based Pyra-Handheld
 *
 * TI authors:
 * Authors: Misael Lopez Cruz <misael.lopez@ti.com>
 *          Peter Ujfalusi <peter.ujfalusi@ti.com>
 *          Liam Girdwood <lrg@ti.com>
 *          Sebastien Guiriec <s-guiriec@ti.com>
 *
 * Contact: Misael Lopez Cruz <misael.lopez@ti.com>
 *          Peter Ujfalusi <peter.ujfalusi@ti.com>
 *
 */

#include <linux/clk.h>
#include <linux/firmware.h>
#include <linux/mfd/twl6040.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include <sound/core.h>
#include <sound/jack.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include "aess/omap-aess.h"
#include "../codecs/twl6040.h"
#include "omap-dmic.h"
#include "omap-mcpdm.h"

#define AESS_FW_NAME   "omap_aess-adfw.bin"

// CHECKME: https://git.ti.com/cgit/ti-linux-kernel/ti-linux-kernel/tree/sound/soc/omap/omap-abe-twl6040.c?id=41b605f2887879d5e428928b197e24ffb44d9b82#n532

/* legacy links with CPU (userspace visible) */
SND_SOC_DAILINK_DEFS(link0,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_CODEC("twl6040-codec",
				      "twl6040-legacy")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(link1,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_CODEC("dmic-codec",
				      "dmic-hifi")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(link_mcbsp2,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),	// NOTE: of_node of mcbsp2 inserted by code
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));	// NOTE: of_node of aesss inserted by code

#if FIXME
// mcbsp1 & 3?
#endif

#if FIXME
/* do we need this??? */
SND_SOC_DAILINK_DEFS(link_mcasp,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_CODEC("spdif-dit",
				      "dit-hifi")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));
#endif

// CHECKME: https://git.ti.com/cgit/ti-linux-kernel/ti-linux-kernel/tree/sound/soc/omap/omap-abe-twl6040.c?id=41b605f2887879d5e428928b197e24ffb44d9b82#n566

/* Frontend DAIs - i.e. userspace visible interfaces (ALSA PCMs) */
SND_SOC_DAILINK_DEFS(link_fe_media1,
	DAILINK_COMP_ARRAY(COMP_CPU("MultiMedia1")),	/* must match with struct snd_soc_dai_driver omap_aess_dai in omap-aess-dai.c */
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("omap-pcm-audio")));	/* code will set .of_node later */

SND_SOC_DAILINK_DEFS(link_fe_media2,
	DAILINK_COMP_ARRAY(COMP_CPU("MultiMedia2")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("omap-pcm-audio")));

SND_SOC_DAILINK_DEFS(link_fe_tones,
	DAILINK_COMP_ARRAY(COMP_CPU("Tones")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("omap-pcm-audio")));

SND_SOC_DAILINK_DEFS(link_fe_voice,
	DAILINK_COMP_ARRAY(COMP_CPU("Voice")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("omap-pcm-audio")));

SND_SOC_DAILINK_DEFS(link_fe_modem,
	DAILINK_COMP_ARRAY(COMP_CPU("MODEM")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("aess")));

SND_SOC_DAILINK_DEFS(link_fe_lp,
	DAILINK_COMP_ARRAY(COMP_CPU("MultiMedia1 LP")),
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("aess")));

// CHECKME: https://git.ti.com/cgit/ti-linux-kernel/ti-linux-kernel/tree/sound/soc/omap/omap-abe-twl6040.c?id=41b605f2887879d5e428928b197e24ffb44d9b82#n609

/* Backend DAIs - i.e. dynamically matched interfaces, invisible to userspace */
SND_SOC_DAILINK_DEFS(link_be_mcpdm_dl1,
	DAILINK_COMP_ARRAY(COMP_CPU("mcpdm-abe")),
	DAILINK_COMP_ARRAY(COMP_CODEC("twl6040-codec",
				      "twl6040-dl1")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("aess")));

SND_SOC_DAILINK_DEFS(link_be_mcpdm_ul,
	DAILINK_COMP_ARRAY(COMP_CPU("mcpdm-abe")),
	DAILINK_COMP_ARRAY(COMP_CODEC("twl6040-codec",
				      "twl6040-ul")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("aess")));

SND_SOC_DAILINK_DEFS(link_be_mcpdm_dl2,
	DAILINK_COMP_ARRAY(COMP_CPU("mcpdm-abe")),
	DAILINK_COMP_ARRAY(COMP_CODEC("twl6040-codec",
				      "twl6040-dl2")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("aess")));

SND_SOC_DAILINK_DEFS(link_be_mcbsp1,
	DAILINK_COMP_ARRAY(COMP_CPU("omap-mcbsp.1")),	// NOTE: will be overwritten by OF node
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("aess")));

SND_SOC_DAILINK_DEFS(link_be_mcbsp2,
	DAILINK_COMP_ARRAY(COMP_CPU("omap-mcbsp.2")),	// NOTE: will be overwritten by OF node
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("aess")));

#if FIXME
SND_SOC_DAILINK_DEFS(link_be_mcbsp3,
	DAILINK_COMP_ARRAY(COMP_CPU("omap-mcbsp.3")),	// NOTE: will be overwritten by OF node
	DAILINK_COMP_ARRAY(COMP_DUMMY()),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("aess")));
#endif

SND_SOC_DAILINK_DEFS(link_be_dmic,
	DAILINK_COMP_ARRAY(COMP_CPU("dmic.0")),
#if FIXME
// dmic.1 dmic.2?
#endif
	DAILINK_COMP_ARRAY(COMP_CODEC("dmic-codec",
				      "dmic-hifi")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("aess")));


// from https://git.ti.com/cgit/ti-linux-kernel/ti-linux-kernel/tree/sound/soc/omap/omap-abe-twl6040.c?id=41b605f2887879d5e428928b197e24ffb44d9b82
// and https://kernel.googlesource.com/pub/scm/linux/kernel/git/lrg/asoc/+/omap/v3.10%5E2..omap/v3.10/

#if FIXME
// the stream_name may not be necessary to be initialized and can likely be removed
// or replaced by _name
#endif
#define SND_SOC_DAI_CONNECT(_name, _stream_name, _link) \
	.name = _name, .stream_name = _stream_name, SND_SOC_DAILINK_REG(_link)
#define SND_SOC_DAI_OPS(_ops, _init) \
	.ops = _ops, .init = _init
#define SND_SOC_DAI_IGNORE_SUSPEND .ignore_suspend = 1
#define SND_SOC_DAI_IGNORE_PMDOWN .ignore_pmdown_time = 1
#define SND_SOC_DAI_BE_LINK(_id, _fixup) \
	.id = _id, .be_hw_params_fixup = _fixup, .no_pcm = 1
#define SND_SOC_DAI_FE_LINK(_name, _link) \
	SND_SOC_DAI_CONNECT(_name, _name, _link), \
	.dynamic = 1
#define SND_SOC_DAI_FE_TRIGGER(_play, _capture) \
	.trigger = {_play, _capture }

#if FIXME
#define SND_SOC_DAI_LINK_NO_HOST	.no_host_mode = 1
#endif

static const struct snd_soc_ops omap_abe_dmic_ops;
static int omap_abe_dmic_init(struct snd_soc_pcm_runtime *rtd);

/* Digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link legacy_dmic_dai = {
	/* Legacy DMIC */
	SND_SOC_DAI_CONNECT("Legacy DMIC", "Legacy DMIC", link1),
	SND_SOC_DAI_OPS(&omap_abe_dmic_ops, omap_abe_dmic_init),
};

static int omap_abe_twl6040_init(struct snd_soc_pcm_runtime *rtd);
static const struct snd_soc_ops omap_abe_ops;

static struct snd_soc_dai_link legacy_mcpdm_dai = {
	/* Legacy McPDM */
// CHECKME: does this fit as .name?
	SND_SOC_DAI_CONNECT("Legacy McPDM", "Legacy McPDM", link0),
	SND_SOC_DAI_OPS(&omap_abe_ops, omap_abe_twl6040_init),
};

static const struct snd_soc_ops omap_abe_mcbsp_ops;

static struct snd_soc_dai_link legacy_mcbsp_dai = {
	/* Legacy McBSP */
	SND_SOC_DAI_CONNECT("Legacy McBSP", "Legacy McBSP", link_mcbsp2),
	SND_SOC_DAI_OPS(&omap_abe_mcbsp_ops, NULL),
	SND_SOC_DAI_IGNORE_SUSPEND,
};

#if FIXME
/* do we need this??? */
static struct snd_soc_dai_link legacy_mcasp_dai = {
	/* Legacy SPDIF */
	SND_SOC_DAI_CONNECT("Legacy SPDIF","Legacy SPDIF", link_mcasp),
	SND_SOC_DAI_IGNORE_SUSPEND,
};
#endif

static int omap_abe_twl6040_fe_init(struct snd_soc_pcm_runtime *rtd);

static struct snd_soc_dai_link abe_fe_dai[] = {

/*
 * Frontend DAIs - i.e. userspace visible interfaces (ALSA PCMs)
 */
{
	/* ABE Media Playback/Capture */
	SND_SOC_DAI_FE_LINK("OMAP ABE Media1", link_fe_media1),
	SND_SOC_DAI_FE_TRIGGER(SND_SOC_DPCM_TRIGGER_BESPOKE, SND_SOC_DPCM_TRIGGER_BESPOKE),
	SND_SOC_DAI_OPS(NULL, omap_abe_twl6040_fe_init),
	SND_SOC_DAI_IGNORE_PMDOWN,
	.dpcm_playback = 1,
	.dpcm_capture = 1,
},
{
	/* ABE Media Capture */
	SND_SOC_DAI_FE_LINK("OMAP ABE Media2", link_fe_media2),
	SND_SOC_DAI_FE_TRIGGER(SND_SOC_DPCM_TRIGGER_BESPOKE, SND_SOC_DPCM_TRIGGER_BESPOKE),
	SND_SOC_DAI_OPS(NULL, omap_abe_twl6040_fe_init),
	.dpcm_capture = 1,
},
{
	/* ABE Voice */
	SND_SOC_DAI_FE_LINK("OMAP ABE Voice", link_fe_voice),
	SND_SOC_DAI_FE_TRIGGER(SND_SOC_DPCM_TRIGGER_BESPOKE, SND_SOC_DPCM_TRIGGER_BESPOKE),
	.dpcm_playback = 1,
	.dpcm_capture = 1,
},
{
	/* ABE Tones */
	SND_SOC_DAI_FE_LINK("OMAP ABE Tones", link_fe_tones),
	SND_SOC_DAI_FE_TRIGGER(SND_SOC_DPCM_TRIGGER_BESPOKE, SND_SOC_DPCM_TRIGGER_BESPOKE),
	.dpcm_playback = 1,
},
{
	/* MODEM */
	SND_SOC_DAI_FE_LINK("OMAP ABE MODEM", link_fe_modem),
	SND_SOC_DAI_FE_TRIGGER(SND_SOC_DPCM_TRIGGER_BESPOKE, SND_SOC_DPCM_TRIGGER_BESPOKE),
	SND_SOC_DAI_OPS(NULL, omap_abe_twl6040_fe_init),
	SND_SOC_DAI_IGNORE_SUSPEND, SND_SOC_DAI_IGNORE_PMDOWN,
#if FIXME
	SND_SOC_DAI_LINK_NO_HOST,
#endif
	.dpcm_playback = 1,
	.dpcm_capture = 1,
},
{
	/* Low power ping - pong */
	SND_SOC_DAI_FE_LINK("OMAP ABE Media LP", link_fe_lp),
	SND_SOC_DAI_FE_TRIGGER(SND_SOC_DPCM_TRIGGER_BESPOKE, SND_SOC_DPCM_TRIGGER_BESPOKE),
	.dpcm_playback = 1,
},
};

#if IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS)
static int omap_abe_twl6040_dl2_init(struct snd_soc_pcm_runtime *rtd);
static int omap_abe_twl6040_aess_init(struct snd_soc_pcm_runtime *rtd);
static int omap_mcpdm_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd, struct snd_pcm_hw_params *params);
static int omap_mcbsp_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd, struct snd_pcm_hw_params *params);
#endif

static struct snd_soc_dai_link abe_be_mcpdm_dai[] = {
#if IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS)
{
	/* McPDM DL1 - Headset */
	SND_SOC_DAI_CONNECT("McPDM-DL1", "twl6040-dl1", link_be_mcpdm_dl1),
	SND_SOC_DAI_BE_LINK(OMAP_AESS_BE_ID_PDM_DL1, omap_mcpdm_be_hw_params_fixup),
	SND_SOC_DAI_OPS(&omap_abe_ops, omap_abe_twl6040_aess_init),
	SND_SOC_DAI_IGNORE_SUSPEND, SND_SOC_DAI_IGNORE_PMDOWN,
	.dpcm_playback = 1,
},
{
	/* McPDM UL1 - Analog Capture (Headset) */
	SND_SOC_DAI_CONNECT("McPDM-UL1", "twl6040-ul", link_be_mcpdm_ul),
	SND_SOC_DAI_BE_LINK(OMAP_AESS_BE_ID_PDM_UL, omap_mcpdm_be_hw_params_fixup),
	SND_SOC_DAI_OPS(&omap_abe_ops, NULL),
	SND_SOC_DAI_IGNORE_SUSPEND, SND_SOC_DAI_IGNORE_PMDOWN,
	.dpcm_capture = 1,
},
{
	/* McPDM DL2 - Handsfree */
	SND_SOC_DAI_CONNECT("McPDM-DL2", "twl6040-dl2", link_be_mcpdm_dl2),
	SND_SOC_DAI_BE_LINK(OMAP_AESS_BE_ID_PDM_DL2, omap_mcpdm_be_hw_params_fixup),
	SND_SOC_DAI_OPS(&omap_abe_ops, omap_abe_twl6040_dl2_init),
	SND_SOC_DAI_IGNORE_SUSPEND, SND_SOC_DAI_IGNORE_PMDOWN,
	.dpcm_playback = 1,
},
#endif
};

#if IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS)
static struct snd_soc_dai_link abe_be_mcbsp1_dai = {
	/* McBSP 1 - Bluetooth */
	SND_SOC_DAI_CONNECT("McBSP-1", "mcbsp-1", link_be_mcbsp1),
	SND_SOC_DAI_BE_LINK(OMAP_AESS_BE_ID_BT_VX,	omap_mcbsp_be_hw_params_fixup),
	SND_SOC_DAI_OPS(&omap_abe_mcbsp_ops, NULL),
	SND_SOC_DAI_IGNORE_SUSPEND, SND_SOC_DAI_IGNORE_PMDOWN,
	.dpcm_playback = 1,
	.dpcm_capture = 1,
};

static struct snd_soc_dai_link abe_be_mcbsp2_dai = {
	/* McBSP 2 - MODEM or FM */
	SND_SOC_DAI_CONNECT("McBSP-2", "mcbsp-2", link_be_mcbsp2),
	SND_SOC_DAI_BE_LINK(OMAP_AESS_BE_ID_MM_FM,	omap_mcbsp_be_hw_params_fixup),
	SND_SOC_DAI_OPS(&omap_abe_mcbsp_ops, NULL),
	SND_SOC_DAI_IGNORE_SUSPEND, SND_SOC_DAI_IGNORE_PMDOWN,
	.dpcm_playback = 1,
	.dpcm_capture = 1,
};

#if FIXME
static struct snd_soc_dai_link abe_be_mcbsp3_dai = {
	/* McBSP 3 - MODEM */
	SND_SOC_DAI_CONNECT("McBSP-3", "mcbsp-3", link_be_mcbsp3),
	SND_SOC_DAI_BE_LINK(OMAP_AESS_BE_ID_MODEM,	omap_mcbsp_be_hw_params_fixup),
	SND_SOC_DAI_OPS(&omap_abe_mcbsp_ops, NULL),
	SND_SOC_DAI_IGNORE_SUSPEND, SND_SOC_DAI_IGNORE_PMDOWN,
	.dpcm_playback = 1,
	.dpcm_capture = 1,
};
#endif

#endif

static int omap_dmic_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd, struct snd_pcm_hw_params *params);

static struct snd_soc_dai_link abe_be_dmic_dai[] = {
{
	/* DMIC0 */
	SND_SOC_DAI_CONNECT("DMIC-0", "dmic-hifi", link_be_dmic),
	SND_SOC_DAI_BE_LINK(OMAP_AESS_BE_ID_DMIC0,	omap_dmic_be_hw_params_fixup),
	SND_SOC_DAI_OPS(&omap_abe_dmic_ops, NULL),
	.dpcm_capture = 1,
},
{
	/* DMIC1 */
	SND_SOC_DAI_CONNECT("DMIC-1", "dmic-hifi", link_be_dmic),
	SND_SOC_DAI_BE_LINK(OMAP_AESS_BE_ID_DMIC1,	omap_dmic_be_hw_params_fixup),
	SND_SOC_DAI_OPS(&omap_abe_dmic_ops, NULL),
	.dpcm_capture = 1,
},
{
	/* DMIC2 */
	SND_SOC_DAI_CONNECT("DMIC-2", "dmic-hifi", link_be_dmic),
	SND_SOC_DAI_BE_LINK(OMAP_AESS_BE_ID_DMIC2,	omap_dmic_be_hw_params_fixup),
	SND_SOC_DAI_OPS(&omap_abe_dmic_ops, NULL),
	.dpcm_capture = 1,
},
};

#define TOTAL_DAI_LINKS (4 + /* legacy DAIs */ \
	ARRAY_SIZE(abe_fe_dai) + \
	ARRAY_SIZE(abe_be_mcpdm_dai) + \
	2 + /* be_mcbsp DAIs */\
	ARRAY_SIZE(abe_be_dmic_dai))

struct abe_twl6040 {
	struct snd_soc_card card;
	struct snd_soc_dai_link dai_links[TOTAL_DAI_LINKS];
	int	jack_detection;	/* board can detect jack events */
	int	mclk_freq;	/* MCLK frequency speed for twl6040 */
	int	twl6040_power_mode;
	struct omap_aess	*aess;
};

static struct platform_device *dmic_codec_dev;
static struct platform_device *spdif_codec_dev;

static int omap_abe_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct snd_soc_dai *codec_dai = snd_soc_rtd_to_codec(rtd, 0);
	struct snd_soc_card *card = rtd->card;
	struct abe_twl6040 *priv = snd_soc_card_get_drvdata(card);
	int clk_id, freq;
	int ret;

	clk_id = twl6040_get_clk_id(codec_dai->component);
	if (clk_id == TWL6040_SYSCLK_SEL_HPPLL)
		freq = priv->mclk_freq;
	else if (clk_id == TWL6040_SYSCLK_SEL_LPPLL)
		freq = 32768;
	else
		return -EINVAL;

	/* set the codec mclk */
	ret = snd_soc_dai_set_sysclk(codec_dai, clk_id, freq,
				SND_SOC_CLOCK_IN);
	if (ret < 0)
		dev_err(card->dev, "can't set codec system clock\n");

	return ret;
}

static int omap_mcpdm_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
                                    struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_RATE);

	rate->min = rate->max = 96000;

	return 0;
}

static const struct snd_soc_ops omap_abe_ops = {
	.hw_params = omap_abe_hw_params,
};

static int omap_abe_dmic_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = snd_soc_rtd_to_cpu(rtd, 0);
	int ret = 0;

	ret = snd_soc_dai_set_sysclk(cpu_dai, OMAP_DMIC_SYSCLK_PAD_CLKS,
				     19200000, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(rtd->card->dev, "can't set DMIC in system clock\n");
		return ret;
	}
	ret = snd_soc_dai_set_sysclk(cpu_dai, OMAP_DMIC_ABE_DMIC_CLK, 2400000,
				     SND_SOC_CLOCK_OUT);
	if (ret < 0)
		dev_err(rtd->card->dev, "can't set DMIC output clock\n");

	return ret;
}

static int omap_mcbsp_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
				struct snd_pcm_hw_params *params)
{
	struct snd_interval *channels = hw_param_interval(params,
					SNDRV_PCM_HW_PARAM_CHANNELS);
	unsigned int be_id = rtd->dai_link->id;

	if (be_id == OMAP_AESS_BE_ID_MM_FM || be_id == OMAP_AESS_BE_ID_BT_VX)
		channels->min = 2;

	snd_mask_set(&params->masks[SNDRV_PCM_HW_PARAM_FORMAT -
				    SNDRV_PCM_HW_PARAM_FIRST_MASK],
				    SNDRV_PCM_FORMAT_S16_LE);
	return 0;
}

static int omap_dmic_be_hw_params_fixup(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_RATE);

	/* The ABE will covert the FE rate to 96k */
	rate->min = rate->max = 96000;

	snd_mask_set(&params->masks[SNDRV_PCM_HW_PARAM_FORMAT -
				    SNDRV_PCM_HW_PARAM_FIRST_MASK],
				    SNDRV_PCM_FORMAT_S32_LE);
	return 0;
}

static const struct snd_soc_ops omap_abe_dmic_ops = {
	.hw_params = omap_abe_dmic_hw_params,
};


/* Headset jack */
static struct snd_soc_jack hs_jack;

/*Headset jack detection DAPM pins */
static struct snd_soc_jack_pin hs_jack_pins[] = {
	{
		.pin = "Headset Mic",
		.mask = SND_JACK_MICROPHONE,
	},
	{
		.pin = "Headset Stereophone",
		.mask = SND_JACK_HEADPHONE,
	},
};

static int omap_abe_get_power_mode(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct abe_twl6040 *priv = snd_soc_card_get_drvdata(card);

	ucontrol->value.integer.value[0] = priv->twl6040_power_mode;

	return 0;
}

static int omap_abe_set_power_mode(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_card *card = snd_kcontrol_chip(kcontrol);
	struct abe_twl6040 *priv = snd_soc_card_get_drvdata(card);

	if (priv->twl6040_power_mode == ucontrol->value.integer.value[0])
		return 0;

	priv->twl6040_power_mode = ucontrol->value.integer.value[0];
#if IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS)
	omap_aess_pm_set_mode(priv->aess, priv->twl6040_power_mode);
#endif

	return 1;
}

static const char *power_texts[] = {"Low-Power", "High-Performance"};

static const struct soc_enum omap_abe_enum[] = {
	SOC_ENUM_SINGLE_EXT(2, power_texts),
};

static const struct snd_kcontrol_new omap_abe_controls[] = {
	SOC_ENUM_EXT("TWL6040 Power Mode", omap_abe_enum[0],
		omap_abe_get_power_mode, omap_abe_set_power_mode),
};

static const struct snd_soc_dapm_widget twl6040_dapm_widgets[] = {
	/* Outputs */
	SND_SOC_DAPM_HP("Headset Stereophone", NULL),
	SND_SOC_DAPM_SPK("Earphone Spk", NULL),
	SND_SOC_DAPM_SPK("Ext Spk", NULL),
	SND_SOC_DAPM_LINE("Line Out", NULL),
	SND_SOC_DAPM_SPK("Vibrator", NULL),

	/* Inputs */
	SND_SOC_DAPM_MIC("Headset Mic", NULL),
	SND_SOC_DAPM_MIC("Main Handset Mic", NULL),
	SND_SOC_DAPM_MIC("Sub Handset Mic", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),

	/* Digital microphones */
	SND_SOC_DAPM_MIC("Digital Mic", NULL),
};

static const struct snd_soc_dapm_route audio_map[] = {
	/* Routings for outputs: Destination Widget <=== Path Name <=== Source Widget */
	{"Headset Stereophone", NULL, "HSOL"},
	{"Headset Stereophone", NULL, "HSOR"},

	{"Earphone Spk", NULL, "EP"},

	{"Ext Spk", NULL, "HFL"},
	{"Ext Spk", NULL, "HFR"},

	{"Line Out", NULL, "AUXL"},
	{"Line Out", NULL, "AUXR"},

	{"Vibrator", NULL, "VIBRAL"},
	{"Vibrator", NULL, "VIBRAR"},

	/* Routings for inputs */
	{"HSMIC", NULL, "Headset Mic"},
	{"Headset Mic", NULL, "Headset Mic Bias"},

	{"MAINMIC", NULL, "Main Handset Mic"},
	{"Main Handset Mic", NULL, "Main Mic Bias"},

	{"SUBMIC", NULL, "Sub Handset Mic"},
	{"Sub Handset Mic", NULL, "Main Mic Bias"},

	{"AFML", NULL, "Line In"},
	{"AFMR", NULL, "Line In"},
};

#if IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS)
static const struct snd_soc_dapm_route aess_audio_map[] = {
	/* Connections between twl6040 and ABE */
	{"Headset Playback", NULL, "PDM_DL1"},
	{"Handsfree Playback", NULL, "PDM_DL2"},
	{"PDM_UL1", NULL, "Capture"},

	/* Bluetooth <--> ABE*/
	{"40122000.mcbsp Playback", NULL, "BT_VX_DL"},
	{"BT_VX_UL", NULL, "40122000.mcbsp Capture"},

	/* FM <--> ABE */
	{"40124000.mcbsp Playback", NULL, "MM_EXT_DL"},
	{"MM_EXT_UL", NULL, "40124000.mcbsp Capture"},

#if FIXME	/* for direct modem access? Likely needs firmware modifications. */
	/* Modem <--> ABE*/
	{"40126000.mcbsp Playback", NULL, "PH_EXT_DL"},
	{"PH_EXT_UL", NULL, "40126000.mcbsp Capture"},
#endif
};
#endif

static int omap_abe_stream_event(struct snd_soc_dapm_context *dapm, int event)
{
	struct snd_soc_card *card = dapm->card;
	struct snd_soc_component *component = dapm->component;
	struct abe_twl6040 *priv = snd_soc_card_get_drvdata(card);

	int gain;

	/*
	 * set DL1 gains dynamically according to the active output
	 * (Headset, Earpiece) and HSDAC power mode
	 */

	gain = twl6040_get_dl1_gain(component) * 100;

#if IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS)
	omap_aess_set_dl1_gains(priv->aess, gain, gain);
#endif

	return 0;
}

static int omap_abe_twl6040_dl2_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_component *component = asoc_rtd_to_codec(rtd, 0)->component;
	struct snd_soc_card *card = rtd->card;
	struct abe_twl6040 *priv = snd_soc_card_get_drvdata(card);
	u32 hfotrim, left_offset, right_offset;

	/* DC offset cancellation computation */
	hfotrim = twl6040_get_trim_value(component, TWL6040_TRIM_HFOTRIM);
	right_offset = TWL6040_HSF_TRIM_RIGHT(hfotrim);
	left_offset = TWL6040_HSF_TRIM_LEFT(hfotrim);

#if IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS)
	omap_aess_dc_set_hf_offset(priv->aess, left_offset, right_offset);
#endif

	return 0;
}
static int omap_abe_twl6040_fe_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_component *component = asoc_rtd_to_codec(rtd, 0)->component;
	struct snd_soc_card *card = rtd->card;
	struct abe_twl6040 *priv = snd_soc_card_get_drvdata(card);

#if FIXME	// FIXME: what should that do in modern code?
//	card_data->abe_platform = component;
#endif

	return 0;
}

static int omap_abe_twl6040_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_component *component = snd_soc_rtd_to_codec(rtd, 0)->component;
	struct snd_soc_card *card = rtd->card;
	struct abe_twl6040 *priv = snd_soc_card_get_drvdata(card);
	int hsotrim;
	int ret;

	/*
	 * Configure McPDM offset cancellation based on the HSOTRIM value from
	 * twl6040.
	 */
	hsotrim = twl6040_get_trim_value(component, TWL6040_TRIM_HSOTRIM);
	omap_mcpdm_configure_dn_offsets(rtd, TWL6040_HSF_TRIM_LEFT(hsotrim),
					TWL6040_HSF_TRIM_RIGHT(hsotrim));

	/* Headset jack detection only if it is supported */
	if (priv->jack_detection) {
		ret = snd_soc_card_jack_new_pins(rtd->card, "Headset Jack",
						 SND_JACK_HEADSET, &hs_jack,
						 hs_jack_pins,
						 ARRAY_SIZE(hs_jack_pins));
		if (ret)
			return ret;

		twl6040_hs_jack_detect(component, &hs_jack, SND_JACK_HEADSET);
	}

	return 0;
}

#if IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS)
static int omap_abe_twl6040_aess_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_component *component = asoc_rtd_to_codec(rtd, 0)->component;
	struct snd_soc_card *card = rtd->card;
	struct abe_twl6040 *priv = snd_soc_card_get_drvdata(card);
	u32 hfotrim, left_offset, right_offset, step_mV;
	int ret;

#if FIXME	// dapm.stream_event has disappeared in v5.4

	card->dapm.stream_event = omap_abe_stream_event;

	// what is the replacement?
	// maybe: component->driver->stream_event = omap_abe_stream_event;
/* better in abe_probe? */

static const struct snd_soc_component_driver something = {
	.stream_event = omap_abe_stream_event;
};

	ret = devm_snd_soc_register_component(dev, &something,
					      &some_dai, 1);

#endif

#if FIXME	// REVISIT
	/* allow audio paths from the audio modem to run during suspend */
	snd_soc_dapm_ignore_suspend(&card->dapm, "Ext Spk");
// AFML/AFMR belong to the codec twl6040.c and will not be found by &card->dapm
// which is some generic context but not the one pointing to the codec
	snd_soc_dapm_ignore_suspend(&card->dapm, "AFML");
	snd_soc_dapm_ignore_suspend(&card->dapm, "AFMR");
	snd_soc_dapm_ignore_suspend(&card->dapm, "Headset Mic");
	snd_soc_dapm_ignore_suspend(&card->dapm, "Headset Stereophone");
#endif

	/* DC offset cancellation computation only if ABE is enabled */
	if (priv->aess) {
		hfotrim = twl6040_get_trim_value(component, TWL6040_TRIM_HFOTRIM);
		right_offset = TWL6040_HSF_TRIM_RIGHT(hfotrim);
		left_offset = TWL6040_HSF_TRIM_LEFT(hfotrim);

		step_mV = twl6040_get_hs_step_size(component);

		omap_aess_dc_set_hs_offset(priv->aess, left_offset,
					   right_offset, step_mV);

		/* ABE power control */
		ret = snd_soc_add_card_controls(card, omap_abe_controls,
						ARRAY_SIZE(omap_abe_controls));
		if (ret)
			return ret;
	}

#if FIXME	// can we add the aess routes here?
	if (priv->aess) {
		ret = snd_soc_dapm_add_routes(&card->dapm, aess_audio_map,
					ARRAY_SIZE(aess_audio_map));
	}
#endif
	return 0;
}
#endif

static const struct snd_soc_dapm_route dmic_audio_map[] = {
	{"DMic", NULL, "Digital Mic"},
	{"Digital Mic", NULL, "Digital Mic1 Bias"},
	{"Digital Mic1 Bias", NULL, "Digital Mic 0"},

	{"DMic1", NULL, "omap-dmic-abe Capture"},
	{"omap-dmic-abe Capture", NULL, "Digital Mic1 Bias"},
	{"Digital Mic1 Bias", NULL, "Digital Mic 1"},

	{"DMic2", NULL, "omap-dmic-abe Capture"},
	{"omap-dmic-abe Capture", NULL, "Digital Mic1 Bias"},
	{"Digital Mic1 Bias", NULL, "Digital Mic 2"},
};

static const struct snd_soc_dapm_widget dmic_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Digital Mic 0", NULL),
	SND_SOC_DAPM_MIC("Digital Mic 1", NULL),
	SND_SOC_DAPM_MIC("Digital Mic 2", NULL),
};

static int omap_abe_dmic_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_dapm_context *dapm = &rtd->card->dapm;
	int ret;

	ret = snd_soc_dapm_new_controls(dapm, dmic_dapm_widgets,
					ARRAY_SIZE(dmic_dapm_widgets));
	if (ret)
		return ret;

	ret = snd_soc_dapm_add_routes(dapm, dmic_audio_map,
					ARRAY_SIZE(dmic_audio_map));
	if (ret < 0)
		return ret;

	snd_soc_dapm_ignore_suspend(dapm, "Digital Mic 0");
	snd_soc_dapm_ignore_suspend(dapm, "Digital Mic 1");
	snd_soc_dapm_ignore_suspend(dapm, "Digital Mic 2");

	return 0;
}

static int snd_soc_card_new_dai_links(struct snd_soc_card *card,
	struct snd_soc_dai_link *new, int count)
{
	struct snd_soc_dai_link *links;
	size_t bytes;

	bytes = (count + card->num_links) * sizeof(struct snd_soc_dai_link);
	links = devm_kzalloc(card->dev, bytes, GFP_KERNEL);
	if (!links)
		return -ENOMEM;

	if (card->num_links > 0) {
		memcpy(links, card->dai_link,
			card->num_links * sizeof(struct snd_soc_dai_link));
		devm_kfree(card->dev, card->dai_link);
	}
	memcpy(links + card->num_links, new,
		count * sizeof(struct snd_soc_dai_link));
	card->dai_link = links;
	card->num_links += count;

	return 0;
}

/*
 * helper to fill the dynamic _card->dai_link array from
 * static array of struct snd_soc_dai_link and insert of_node if defined
 */

static int snd_soc_card_new_dai_links_with_nodes(struct snd_soc_card *card,
	struct snd_soc_dai_link *new, int count, struct device_node *cpu_of_node,
	struct device_node *platform_of_node)
{
	int ret;
	int i;

	ret = snd_soc_card_new_dai_links(card, new, count);
	if (ret < 0)
		return ret;

	for (i = 0; i < count; i++) {
		if (cpu_of_node) {
			card->dai_link[card->num_links-i-1].cpus[0].name = NULL;
			card->dai_link[card->num_links-i-1].cpus[0].dai_name = NULL;
			card->dai_link[card->num_links-i-1].cpus[0].of_node = cpu_of_node;
		}
		if (platform_of_node) {
			card->dai_link[card->num_links-i-1].platforms[0].name = NULL;
			card->dai_link[card->num_links-i-1].platforms[0].of_node = platform_of_node;
		}
	}

	return 0;
}

static int omap_abe_add_legacy_dai_links(struct snd_soc_card *card)
{
	struct device_node *node = card->dev->of_node;
	struct device_node *dai_node;
	int ret;

	dai_node = of_parse_phandle(node, "ti,mcpdm", 0);
	if (!dai_node) {
		dev_err(card->dev, "McPDM node is not provided\n");
		return -EINVAL;
	}

	/* Add the Legacy McPDM */
	ret = snd_soc_card_new_dai_links_with_nodes(card, &legacy_mcpdm_dai, 1, dai_node, dai_node);
	if (ret < 0)
		return ret;

	/* Add the Legacy McBSP(2) */
	dai_node = of_parse_phandle(node, "ti,mcbsp2", 0);
	ret = snd_soc_card_new_dai_links_with_nodes(card, &legacy_mcbsp_dai, 1, dai_node, dai_node);
	if (ret < 0)
		return ret;

#if FIXME	// is already added somewhere? Or not?
	/* Add the Legacy McASP */
	dai_node = of_parse_phandle(node, "ti,mcasp", 0);
	ret = snd_soc_card_new_dai_links_with_nodes(card, &legacy_mcasp_dai, 1, dai_node, dai_node);
	if (ret < 0)
		return ret;
#endif

	/* Add the Legacy DMICs */
	dai_node = of_parse_phandle(node, "ti,dmic", 0);
	if (dai_node)
		ret = snd_soc_card_new_dai_links_with_nodes(card, &legacy_dmic_dai, 1, dai_node, dai_node);

	return 0;
}

#if IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS)

/* called after loading firmware */
static int omap_abe_add_aess_dai_links(struct snd_soc_card *card)
{
	struct device_node *node = card->dev->of_node;
	struct device_node *aess_node;
	struct device_node *dai_node;
	int ret;

	aess_node = of_parse_phandle(node, "ti,aess", 0);

	ret = snd_soc_card_new_dai_links_with_nodes(card, abe_fe_dai, ARRAY_SIZE(abe_fe_dai), NULL, aess_node);
	if (ret < 0)
		return ret;

#if FIXME	// not done in https://git.goldelico.com/?p=letux-kernel.git;a=blob;f=sound/soc/omap/omap-abe-twl6040.c;h=208e93cdde40944d6e717e5a1db0e56877516d05;hb=refs/heads/omap-audio-3.15#l694
	dai_node = of_parse_phandle(node, "ti,mcpdm", 0);
#endif
	ret = snd_soc_card_new_dai_links_with_nodes(card, abe_be_mcpdm_dai, ARRAY_SIZE(abe_be_mcpdm_dai), NULL, aess_node);
	if (ret < 0)
		return ret;

	dai_node = of_parse_phandle(node, "ti,mcbsp1", 0);
	ret = snd_soc_card_new_dai_links_with_nodes(card, &abe_be_mcbsp1_dai, 1, dai_node, aess_node);
	if (ret < 0)
		return ret;

	dai_node = of_parse_phandle(node, "ti,mcbsp2", 0);
	ret = snd_soc_card_new_dai_links_with_nodes(card, &abe_be_mcbsp2_dai, 1, dai_node, aess_node);
	if (ret < 0)
		return ret;

#if FIXME	// this requires a ti,dmic node and fails if it is missing rather than using an internal fallback
	dai_node = of_parse_phandle(node, "ti,dmic", 0);
	ret = snd_soc_card_new_dai_links_with_nodes(card, abe_be_dmic_dai, ARRAY_SIZE(abe_be_dmic_dai), dai_node, aess_node);
	if (ret < 0)
		return ret;
#endif

	return 0;
}

#if IS_BUILTIN(CONFIG_SND_OMAP_SOC_OMAP_ABE_TWL6040)
static void omap_abe_fw_ready(const struct firmware *fw, void *context)
{
	struct platform_device *pdev = (struct platform_device *)context;
	struct snd_soc_card *card = &omap_abe_card;
	struct abe_twl6040 *priv = snd_soc_card_get_drvdata(card);
	int ret;

	if (unlikely(!fw))
		dev_warn(&pdev->dev, "%s firmware is not loaded.\n",
			 AESS_FW_NAME);

	priv->aess = omap_aess_get_handle();
	if (!priv->aess) {
		dev_err(&pdev->dev, "AESS is not yet available\n");
		return;
	}

	ret = omap_aess_load_firmware(priv->aess, fw);
	if (ret) {
		dev_err(&pdev->dev, "%s firmware was not loaded.\n",
			AESS_FW_NAME);
		omap_aess_put_handle(priv->aess);
		priv->aess = NULL;
	}

	/* Release the FW here. */
#if FIXME // if we do this some problem I do not remember arises...
	release_firmware(fw);
#endif
	ret = omap_abe_add_legacy_dai_links(card);
	if (ret < 0)
		return;

	ret = omap_abe_add_aess_dai_links(card);
	if (ret < 0)
		return;

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret)
		dev_err(&pdev->dev, "card registration failed after successful firmware load: %d\n",
			ret);

#if FIXME
// can we move that to omap_abe_twl6040_init?
#endif
	if (priv->aess) {
		ret = snd_soc_dapm_add_routes(&card->dapm, aess_audio_map,
				ARRAY_SIZE(aess_audio_map));
		if (ret) {
			dev_err(&pdev->dev, "could not add AESS routes: %d\n", ret);
			return ret;
		}
	}

	return;
}

#else /* !IS_BUILTIN(CONFIG_SND_OMAP_SOC_OMAP_ABE_TWL6040) */
static int omap_abe_load_fw(struct snd_soc_card *card)
{
	struct abe_twl6040 *priv = snd_soc_card_get_drvdata(card);
	const struct firmware *fw;
	int ret;

	priv->aess = omap_aess_get_handle();
	if (!priv->aess) {
		dev_err(card->dev, "AESS is not yet available\n");
		return -EPROBE_DEFER;
	}

	ret = request_firmware(&fw, AESS_FW_NAME, card->dev);
	if (ret) {
		dev_err(card->dev, "FW request failed: %d\n", ret);
		omap_aess_put_handle(priv->aess);
		priv->aess = NULL;
		return ret;
	}

	ret = omap_aess_load_firmware(priv->aess, fw);
	if (ret) {
		dev_err(card->dev, "%s firmware was not loaded.\n",
			AESS_FW_NAME);
		omap_aess_put_handle(priv->aess);
		priv->aess = NULL;
		ret = 0;
	}

	if (priv->aess)
		ret = omap_abe_add_aess_dai_links(card);

	/* Release the FW here. */
#if FIXME
// oops - why???
// and: what about error paths? We should have sort of devm_request_firmware
//	release_firmware(fw);
#endif

	return ret;
}
#endif /* IS_BUILTIN(CONFIG_SND_OMAP_SOC_OMAP_ABE_TWL6040) */
#endif /* IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS) */

static int omap_abe_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct snd_soc_card *card;
	struct device_node *dai_node;
	struct abe_twl6040 *priv;
	int ret = 0;

	if (!node) {
		dev_err(&pdev->dev, "of node is missing.\n");
		return -ENODEV;
	}

	priv = devm_kzalloc(&pdev->dev, sizeof(struct abe_twl6040), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	card = &priv->card;
	card->dev = &pdev->dev;
	card->owner = THIS_MODULE;
	card->dapm_widgets = twl6040_dapm_widgets;
	card->num_dapm_widgets = ARRAY_SIZE(twl6040_dapm_widgets);
	card->dai_link = priv->dai_links;
	snd_soc_card_set_drvdata(card, priv);

	if (snd_soc_of_parse_card_name(card, "ti,model")) {
		dev_err(&pdev->dev, "Card name is not provided\n");
		return -ENODEV;
	}


	/* mapping (static) */
	ret = snd_soc_of_parse_audio_routing(card, "ti,audio-routing");
	if (ret) {
		dev_err(&pdev->dev, "Error while parsing DAPM routing\n");
		return ret;
	}

	/* fall back to default routing (static) */
	if (card->num_of_dapm_routes == 0) {
		card->dapm_routes = audio_map;	/* default map */
		card->num_dapm_routes = ARRAY_SIZE(audio_map);
	}

	priv->jack_detection = of_property_read_bool(node, "ti,jack-detection");
	of_property_read_u32(node, "ti,mclk-freq", &priv->mclk_freq);
	if (!priv->mclk_freq) {
		dev_err(&pdev->dev, "MCLK frequency not provided\n");
		return -EINVAL;
	}

#if FIXME	/*
		 * we must not set fully_routed since this disables
		 * dapm_update_widget_flags() to set the is_ep flags
		 * for BE:OUT if AESS is loaded
		 * Not sure, if this is the right workaround especially
		 * if there is no AESS (compiled or firmware available)
		 */

	card->fully_routed = 1;
#endif

#if IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS)
	dai_node = of_parse_phandle(node, "ti,aess", 0);

	if (dai_node) {
		/* When ABE is in use the AESS needs firmware */
#if IS_BUILTIN(CONFIG_SND_OMAP_SOC_OMAP_ABE_TWL6040)
		ret = request_firmware_nowait(THIS_MODULE, 1, AESS_FW_NAME,
				      &pdev->dev, GFP_KERNEL, pdev,
				      omap_abe_fw_ready);
#else
		ret = omap_abe_load_fw(card);
#endif
		if (ret < 0)
			/* warn only but continue */
			dev_warn(&pdev->dev, "Failed to load firmware %s: %d\n",
				AESS_FW_NAME, ret);

#if IS_BUILTIN(CONFIG_SND_OMAP_SOC_OMAP_ABE_TWL6040)
		/* card is already registered after successful firmware load */
		return ret;
#endif
	}
#endif

	ret = omap_abe_add_legacy_dai_links(card);
	if (ret < 0)
		return ret;

#if FIXME
/* can we replace by devm_snd_soc_register_component and register the stream event here? */
#endif
	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret) {
		dev_err(&pdev->dev, "card registration failed: %d\n", ret);
		return ret;
	}

#if IS_ENABLED(CONFIG_SND_SOC_OMAP_AESS)
#if FIXME
// can we move that to omap_abe_twl6040_init?
#endif
	if (priv->aess) {
		ret = snd_soc_dapm_add_routes(&card->dapm, aess_audio_map,
					ARRAY_SIZE(aess_audio_map));

		if (ret) {
			dev_err(&pdev->dev, "could not add AESS routes: %d\n", ret);
			return ret;
		}
	}
#endif

	return ret;
}

static const struct of_device_id omap_abe_of_match[] = {
	{ .compatible = "ti,abe-twl6040", },
	{ },
};
MODULE_DEVICE_TABLE(of, omap_abe_of_match);

static struct platform_driver omap_abe_driver = {
	.driver = {
		.name = "omap-abe-twl6040",
		.pm = &snd_soc_pm_ops,
		.of_match_table = omap_abe_of_match,
	},
	.probe = omap_abe_probe,
};

static int __init omap_abe_init(void)
{
	int ret;

	dmic_codec_dev = platform_device_register_simple("dmic-codec", -1, NULL,
							 0);
	if (IS_ERR(dmic_codec_dev)) {
		pr_err("%s: dmic-codec device registration failed\n", __func__);
		return PTR_ERR(dmic_codec_dev);
	}

	spdif_codec_dev = platform_device_register_simple("spdif-dit", -1,
				NULL, 0);
	if (IS_ERR(spdif_codec_dev)) {
		pr_err("%s: spdif-dit device registration failed\n", __func__);
		platform_device_unregister(dmic_codec_dev);
		return PTR_ERR(spdif_codec_dev);
	}

	ret = platform_driver_register(&omap_abe_driver);
	if (ret) {
		pr_err("%s: platform driver registration failed\n", __func__);
		platform_device_unregister(spdif_codec_dev);
		platform_device_unregister(dmic_codec_dev);
	}

	return ret;
}
module_init(omap_abe_init);

static void __exit omap_abe_exit(void)
{
	platform_driver_unregister(&omap_abe_driver);
	platform_device_unregister(dmic_codec_dev);
	platform_device_unregister(spdif_codec_dev);
}
module_exit(omap_abe_exit);

MODULE_AUTHOR("Misael Lopez Cruz <misael.lopez@ti.com>");
MODULE_AUTHOR("H. Nikolaus Schaller <hns@goldelico.com>");
MODULE_DESCRIPTION("ALSA SoC for OMAP boards with ABE and twl6040 codec");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:omap-abe-twl6040");
