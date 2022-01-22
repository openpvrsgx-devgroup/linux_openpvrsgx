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

// SND_SOC_DAILINK_DEFS(name, cpu, codec, platform...)
// COMP_CODEC(_name, _dai_name)

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

// ab hier nicht integriert?
SND_SOC_DAILINK_DEFS(link_mcbsp,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_CODEC("snd-soc-dummy",
				      "snd-soc-dummy-dai")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(link_mcasp,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_CODEC("spdif-dit",
				      "dit-hifi")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(link_fe,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_CODEC("abc",
				      "def")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(link_be_mcpdm,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_CODEC("abc",
				      "def")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(link_be_mcbsp1,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_CODEC("abc",
				      "def")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(link_be_mcbsp2,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_CODEC("abc",
				      "def")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

SND_SOC_DAILINK_DEFS(link_be_dmic,
	DAILINK_COMP_ARRAY(COMP_EMPTY()),
	DAILINK_COMP_ARRAY(COMP_CODEC("abc",
				      "def")),
	DAILINK_COMP_ARRAY(COMP_EMPTY()));

struct abe_twl6040 {
	struct snd_soc_card card;
	struct snd_soc_dai_link dai_links[2];
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
	struct abe_twl6040 * priv = snd_soc_card_get_drvdata(card);

	if (priv->twl6040_power_mode == ucontrol->value.integer.value[0])
		return 0;

	priv->twl6040_power_mode = ucontrol->value.integer.value[0];
	omap_aess_pm_set_mode(priv->aess, priv->twl6040_power_mode);

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
	/* Routings for outputs */
// Destination Widget <=== Path Name <=== Source Widget
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

	/* Connections between twl6040 and ABE */
	{"Headset Playback", NULL, "PDM_DL1"},
	{"Handsfree Playback", NULL, "PDM_DL2"},
	{"PDM_UL1", NULL, "Analog Capture"},

	/* Bluetooth <--> ABE*/
	{"omap-mcbsp.1 Playback", NULL, "BT_VX_DL"},
	{"BT_VX_UL", NULL, "omap-mcbsp.1 Capture"},

	/* FM <--> ABE */
	{"omap-mcbsp.2 Playback", NULL, "MM_EXT_DL"},
	{"MM_EXT_UL", NULL, "omap-mcbsp.2 Capture"},
};

static int omap_abe_stream_event(struct snd_soc_dapm_context *dapm, int event)
{
	struct snd_soc_card *card = dapm->card;
	struct snd_soc_component *component = dapm->component;
	struct abe_twl6040 * priv = snd_soc_card_get_drvdata(card);

	int gain;

	/*
	 * set DL1 gains dynamically according to the active output
	 * (Headset, Earpiece) and HSDAC power mode
	 */

	gain = twl6040_get_dl1_gain(component) * 100;

	omap_aess_set_dl1_gains(priv->aess, gain, gain);

	return 0;
}

static int omap_abe_twl6040_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_component *component = snd_soc_rtd_to_codec(rtd, 0)->component;
	struct snd_soc_card *card = rtd->card;
	struct abe_twl6040 *priv = snd_soc_card_get_drvdata(card);
	int hs_trim;
	u32 hsotrim, left_offset, right_offset, step_mV;
	int ret;

	/*
	 * Configure McPDM offset cancellation based on the HSOTRIM value from
	 * twl6040.
	 */
	hs_trim = twl6040_get_trim_value(component, TWL6040_TRIM_HSOTRIM);
	omap_mcpdm_configure_dn_offsets(rtd, TWL6040_HSF_TRIM_LEFT(hs_trim),
					TWL6040_HSF_TRIM_RIGHT(hs_trim));

	// FIXME: omap_abe_stream_event.stream_event has disappeared in v5.4
	// card->dapm.stream_event = omap_abe_stream_event;

#if 0	// REVISIT
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
		hsotrim = twl6040_get_trim_value(component, TWL6040_TRIM_HSOTRIM);
		right_offset = TWL6040_HSF_TRIM_RIGHT(hsotrim);
		left_offset = TWL6040_HSF_TRIM_LEFT(hsotrim);

		step_mV = twl6040_get_hs_step_size(component);
		omap_aess_dc_set_hs_offset(priv->aess, left_offset,
					   right_offset, step_mV);

		/* ABE power control */
		ret = snd_soc_add_card_controls(card, omap_abe_controls,
						ARRAY_SIZE(omap_abe_controls));
		if (ret)
			return ret;
	}

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

static const struct snd_soc_dapm_widget dmic_dapm_widgets[] = {
	SND_SOC_DAPM_MIC("Digital Mic 0", NULL),
	SND_SOC_DAPM_MIC("Digital Mic 1", NULL),
	SND_SOC_DAPM_MIC("Digital Mic 2", NULL),
};

static const struct snd_soc_dapm_route dmic_audio_map[] = {
	{"DMic", NULL, "Digital Mic"},
	{"Digital Mic", NULL, "Digital Mic1 Bias"},

	/* Digital Mics: DMic0, DMic1, DMic2 with bias */
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

static int omap_abe_dmic_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_dapm_context *dapm = &rtd->card->dapm;
	struct snd_soc_card *card = rtd->card;
	struct abe_twl6040 * priv = snd_soc_card_get_drvdata(card);
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

#define ADD_DAILINK(_card, _link, _name, _stream, _ofnode, _init, _ops) { \
	_card->dai_link[_card->num_links].name = _name; \
	_card->dai_link[_card->num_links].stream_name = _stream; \
	_card->dai_link[_card->num_links].cpus = _link##_cpus; \
	_card->dai_link[_card->num_links].num_cpus = ARRAY_SIZE(_link##_cpus); \
	_card->dai_link[_card->num_links].cpus->of_node = _ofnode; \
	_card->dai_link[_card->num_links].platforms = _link##_platforms; \
	_card->dai_link[_card->num_links].num_platforms = ARRAY_SIZE(_link##_platforms); \
	_card->dai_link[_card->num_links].platforms->of_node = _ofnode; \
	_card->dai_link[_card->num_links].codecs = _link##_codecs; \
	_card->dai_link[_card->num_links].num_codecs = ARRAY_SIZE(_link##_codecs); \
	_card->dai_link[_card->num_links].init = _init; \
	_card->dai_link[_card->num_links].ops = &_ops; \
	_card->num_links++; \
	}

/* called after loading firmware */
static int omap_abe_add_aess_dai_links(struct snd_soc_card *card)
{
	struct abe_twl6040 * priv = snd_soc_card_get_drvdata(card);
	struct device_node *node = card->dev->of_node;
	struct device_node *dai_node;
	int ret;

	/* FIXME: add DAI links for AE */
	return 0;
}

static int omap_abe_add_legacy_dai_links(struct snd_soc_card *card)
{
	struct abe_twl6040 * priv = snd_soc_card_get_drvdata(card);
	struct device_node *node = card->dev->of_node;
	struct device_node *dai_node;
	int ret;

	/* FIXME: */
	return 0;
}

#if IS_BUILTIN(CONFIG_SND_OMAP_SOC_OMAP_ABE_TWL6040)
static void omap_abe_fw_ready(const struct firmware *fw, void *context)
{
	struct platform_device *pdev = (struct platform_device *)context;
	struct snd_soc_card *card = &omap_abe_card;
	struct abe_twl6040 * priv = snd_soc_card_get_drvdata(card);
	int ret;

	if (unlikely(!fw))
		dev_warn(&pdev->dev, "%s firmware is not loaded.\n",
			 AESS_FW_NAME);

	priv->aess = omap_aess_get_handle();
	if (!priv->aess)
		dev_err(&pdev->dev, "AESS is not yet available\n");

	ret = omap_aess_load_firmware(priv->aess, AESS_FW_NAME);
	if (ret) {
		dev_err(&pdev->dev, "%s firmware was not loaded.\n",
			AESS_FW_NAME);
		omap_aess_put_handle(priv->aess);
		priv->aess = NULL;
	}

	/* Release the FW here. */
	release_firmware(fw);

	if (priv->aess) {
		ret = omap_abe_add_aess_dai_links(card);
		if (ret < 0)
			return;
	}

	ret = omap_abe_add_legacy_dai_links(card);
	if (ret < 0)
		return;

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if (ret)
		dev_err(&pdev->dev, "devm_snd_soc_register_card() failed: %d\n",
			ret);
	return;
}

#else /* !IS_BUILTIN(CONFIG_SND_OMAP_SOC_OMAP_ABE_TWL6040) */
static int omap_abe_load_fw(struct snd_soc_card *card)
{
	struct abe_twl6040 * priv = snd_soc_card_get_drvdata(card);
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
		return ret;
	}

	ret = omap_aess_load_firmware(priv->aess, AESS_FW_NAME);
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
	release_firmware(fw);

	return ret;
}
#endif /* IS_BUILTIN(CONFIG_SND_OMAP_SOC_OMAP_ABE_TWL6040) */

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
	card->dapm_routes = audio_map;
	card->num_dapm_routes = ARRAY_SIZE(audio_map);
	card->dai_link = priv->dai_links;
	snd_soc_card_set_drvdata(card, priv);

	if (snd_soc_of_parse_card_name(card, "ti,model")) {
		dev_err(&pdev->dev, "Card name is not provided\n");
		return -ENODEV;
	}

	ret = snd_soc_of_parse_audio_routing(card, "ti,audio-routing");
	if (ret) {
		dev_err(&pdev->dev, "Error while parsing DAPM routing\n");
		return ret;
	}

	dai_node = of_parse_phandle(node, "ti,mcpdm", 0);
	if (!dai_node) {
		dev_err(&pdev->dev, "McPDM node is not provided\n");
		return -EINVAL;
	}

	ADD_DAILINK(card, link0, "DMIC", "TWL6040", dai_node, omap_abe_twl6040_init, omap_abe_ops);

	dai_node = of_parse_phandle(node, "ti,dmic", 0);
	if (dai_node)
		ADD_DAILINK(card, link1, "TWL6040", "DMIC Capture", dai_node, omap_abe_dmic_init, omap_abe_dmic_ops);

	dai_node = of_parse_phandle(node, "ti,mcbsp", 0);
	if (dai_node)
		ADD_DAILINK(link_mcbsp, "TWL6040", "DMIC Capture", dai_node, omap_abe_dmic_init, omap_abe_dmic_ops);

	dai_node = of_parse_phandle(node, "ti,mcasp", 0);
	if (dai_node)
		ADD_DAILINK(link_mcasp, "TWL6040", "DMIC Capture", dai_node, omap_abe_dmic_init, omap_abe_dmic_ops);

	dai_node = of_parse_phandle(node, "ti,mcbsp2", 0);
	if (dai_node)
		ADD_DAILINK(link_mcasp, "TWL6040", "DMIC Capture", dai_node, omap_abe_dmic_init, omap_abe_dmic_ops);

	dai_node = of_parse_phandle(node, "ti,aess", 0);
	if (dai_node) {
		ADD_DAILINK(link_fe, "TWL6040", "DMIC Capture", dai_node, omap_abe_dmic_init, omap_abe_dmic_ops);
		ADD_DAILINK(link_be_mcpdm, "TWL6040", "DMIC Capture", dai_node, omap_abe_dmic_init, omap_abe_dmic_ops);
		ADD_DAILINK(link_be_mcbsp1, "TWL6040", "DMIC Capture", dai_node, omap_abe_dmic_init, omap_abe_dmic_ops);
		ADD_DAILINK(link_be_mcbsp2, "TWL6040", "DMIC Capture", dai_node, omap_abe_dmic_init, omap_abe_dmic_ops);
		ADD_DAILINK(link_be_dmic, "TWL6040", "DMIC Capture", dai_node, omap_abe_dmic_init, omap_abe_dmic_ops);
	}

	priv->jack_detection = of_property_read_bool(node, "ti,jack-detection");
	of_property_read_u32(node, "ti,mclk-freq", &priv->mclk_freq);
	if (!priv->mclk_freq) {
		dev_err(&pdev->dev, "MCLK frequency not provided\n");
		return -EINVAL;
	}

	card->fully_routed = 1;

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
		/* card is registered after successful firmware load */
		return ret;
#endif
	}

	ret = omap_abe_add_legacy_dai_links(card);
	if (ret < 0)
		return ret;

	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(&pdev->dev, "card registration failed: %d\n", ret);
		return ret;
	}

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
