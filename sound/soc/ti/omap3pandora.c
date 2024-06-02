// SPDX-License-Identifier: GPL-2.0-only
/*
 * omap3pandora.c  --  SoC audio for Pandora Handheld Console
 *
 * Author: Gražvydas Ignotas <notasas@gmail.com>
 */

#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>
#include <linux/of_gpio.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>

#include <asm/mach-types.h>
#include <linux/platform_data/asoc-ti-mcbsp.h>
#include <linux/mfd/twl4030-audio.h>

#include "omap-mcbsp.h"

struct omap3pandora_sound {
	struct gpio_desc *amp_gpio;
	struct regulator *amp_regulator;

	struct snd_pcm_substream *playback_stream;
	struct snd_pcm_substream *capture_stream;

	struct mutex sample_rate_lock; // protects all fields after
	unsigned int sample_rate;
	int sample_rate_users;
};

static struct snd_soc_dai_link omap3pandora_dai[];

static int omap3pandora_common_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct snd_soc_dai *codec_dai = snd_soc_rtd_to_codec(rtd, 0);
	struct snd_soc_dai *cpu_dai = snd_soc_rtd_to_cpu(rtd, 0);
	struct device *dev = rtd->dev;
	int ret;

	/* Set McBSP clock to external */
	ret = snd_soc_dai_set_sysclk(cpu_dai, OMAP_MCBSP_SYSCLK_CLKS_EXT,
				     256 * params_rate(params),
				     SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(dev, "cannot set McBSP clock to external: %d\n", ret);
		return ret;
	}

	ret = snd_soc_dai_set_clkdiv(cpu_dai, OMAP_MCBSP_CLKGDV, 8);
	if (ret < 0) {
		dev_err(dev, "cannot set McBSP clock divider: %d\n", ret);
		return ret;
	}

	return 0;
}

static inline int constrain_sample_rate(struct snd_pcm_substream *substream,
	unsigned int sample_rate)
{
	return snd_pcm_hw_constraint_single(substream->runtime,
					    SNDRV_PCM_HW_PARAM_RATE,
					    sample_rate);
}

static inline void relax_sample_rate(struct snd_pcm_substream *substream)
{
	const struct snd_interval default_sample_rate_range = {
		.min = substream->runtime->hw.rate_min,
		.max = substream->runtime->hw.rate_max,
		.openmin = 1,
		.openmax = 1,
	};

	*constrs_interval(&substream->runtime->hw_constraints,
			  SNDRV_PCM_HW_PARAM_RATE) =
		default_sample_rate_range;
}

static void release_sample_rate(struct omap3pandora_sound *ctx,
	struct snd_pcm_substream *substream)
{
	mutex_lock(&ctx->sample_rate_lock);

	if (ctx->sample_rate_users > 0)
		--ctx->sample_rate_users;
	if (ctx->sample_rate_users == 0)
		ctx->sample_rate = 0;

	relax_sample_rate(substream);

	mutex_unlock(&ctx->sample_rate_lock);
}

static int grab_sample_rate(struct omap3pandora_sound *ctx,
	struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct device *dev = rtd->dev;
	int ret;

	ret = mutex_lock_interruptible(&ctx->sample_rate_lock);
	if (ret)
		return ret;

	if (++ctx->sample_rate_users == 1)
		ctx->sample_rate = params_rate(params);

	mutex_unlock(&ctx->sample_rate_lock);

	ret = constrain_sample_rate(substream, ctx->sample_rate);
	if (ret < 0)
		goto err;

	ret = constrain_sample_rate(substream, params_rate(params));
	if (ret < 0) {
		dev_dbg(dev, "attempted to use sample rate different from other active substream; on the Pandora, this is impossible, as capture and playback use the same sample clock");
		goto err;
	}

	return 0;

err:
	release_sample_rate(ctx, substream);

	return ret;
}

static int omap3pandora_playback_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct snd_soc_card *card = rtd->card;
	struct omap3pandora_sound *ctx = snd_soc_card_get_drvdata(card);
	struct device *dev = rtd->dev;
	struct snd_soc_pcm_runtime *tmp_rtd;
	struct snd_soc_pcm_runtime *twl4030_rtd = NULL;
	int ret;

	/*
	 * We need to set the APLL clock rate on the TWL4030 because it feeds
	 * both the DAI of the PCM1773. So find the appropriate RTD and call
	 * the TWL's .set_sysclk callback through there. Ugly, but it must be
	 * done.
	 */
	for_each_card_rtds(card, tmp_rtd)
		if (tmp_rtd->dai_link == &omap3pandora_dai[1]) {
			twl4030_rtd = tmp_rtd;
			break;
		}
	if (!twl4030_rtd) {
		dev_err(dev, "cannot find TWL4030 runtime data to set APLL rate\n");
		return -EINVAL;
	}

	ret = snd_soc_dai_set_sysclk(snd_soc_rtd_to_codec(twl4030_rtd, 0),
				     TWL4030_CLOCK_APLL, params_rate(params),
				     SND_SOC_CLOCK_OUT);
	if (ret) {
		dev_err(dev, "cannot set TWL4030 APLL rate via set_sysclk interface: %d\n",
			ret);
		return ret;
	}

	if (!ctx->playback_stream) {
		ctx->playback_stream = substream;
		ret = grab_sample_rate(ctx, substream, params);
		if (ret)
			return ret;
	}

	return omap3pandora_common_hw_params(substream, params);
}

static int omap3pandora_capture_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct snd_soc_dai *codec_dai = snd_soc_rtd_to_codec(rtd, 0);
	struct device *dev = rtd->dev;
	struct snd_soc_card *card = rtd->card;
	struct omap3pandora_sound *ctx = snd_soc_card_get_drvdata(card);
	int ret;

	/* Set the codec system clock for DAC and ADC */
	ret = snd_soc_dai_set_sysclk(codec_dai, 0, 26000000,
				     SND_SOC_CLOCK_IN);
	if (ret < 0) {
		dev_err(dev, "cannot set TWL4030 system clock: %d\n", ret);
		return ret;
	}

	if (!ctx->capture_stream) {
		ctx->capture_stream = substream;
		ret = grab_sample_rate(ctx, substream, params);
		if (ret)
			return ret;
	}

	return omap3pandora_common_hw_params(substream, params);
}

static int omap3pandora_playback_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct snd_soc_card *card = rtd->card;
	struct omap3pandora_sound *ctx = snd_soc_card_get_drvdata(card);

	if (ctx->playback_stream)
		release_sample_rate(ctx, substream);
	ctx->playback_stream = NULL;

	return 0;
}

static int omap3pandora_common_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct snd_soc_card *card = rtd->card;
	struct omap3pandora_sound *ctx = snd_soc_card_get_drvdata(card);
	unsigned int sample_rate = ctx->sample_rate;

	if (!sample_rate)
		return 0;

	return constrain_sample_rate(substream, sample_rate);
}

static int omap3pandora_capture_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct snd_soc_card *card = rtd->card;
	struct omap3pandora_sound *ctx = snd_soc_card_get_drvdata(card);

	if (ctx->capture_stream)
		release_sample_rate(ctx, substream);
	ctx->capture_stream = NULL;

	return 0;
}

static int omap3pandora_hp_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *k, int event)
{
	struct snd_soc_card *card = w->dapm->card;
	struct device *dev = card->dev;
	struct omap3pandora_sound *ctx =
		snd_soc_card_get_drvdata(card);
	int ret;

	if (SND_SOC_DAPM_EVENT_ON(event)) {
		ret = regulator_enable(ctx->amp_regulator);
		if (ret) {
			dev_err(dev, "error enabling amplifier regulator: %d\n", ret);
			return ret;
		}

		gpiod_set_value(ctx->amp_gpio, 1);
	} else {
		gpiod_set_value(ctx->amp_gpio, 0);

		ret = regulator_disable(ctx->amp_regulator);
		if (ret) {
			dev_err(dev, "error disabling amplifier regulator: %d\n", ret);
			return ret;
		}
	}

	return 0;
}

/*
 * Audio paths on Pandora board:
 *
 *  |O| ---> PCM DAC +-> AMP -> Headphone Jack
 *  |M|         A    +--------> Line Out
 *  |A| <~~clk~~+
 *  |P| <--- TWL4030 <--------- Line In and MICs
 */
static const struct snd_soc_dapm_widget omap3pandora_dapm_widgets[] = {
	SND_SOC_DAPM_PGA_E("Headphone Amplifier", SND_SOC_NOPM,
			   0, 0, NULL, 0, omap3pandora_hp_event,
			   SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_LINE("Line Out", NULL),

	SND_SOC_DAPM_MIC("Mic (internal)", NULL),
	SND_SOC_DAPM_MIC("Mic (external)", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
};

static const struct snd_soc_dapm_route omap3pandora_map[] = {
	/*
	 * The APLL signal produced by the TWL4030 is routed to the PCM1773 (in
	 * addition to supplying the clock for the McBSPs) so we need to make
	 * sure that APLL gets enabled before the PCM1773 begins operation.
	 */
	{"PCM1773 DAC", NULL, "APLL Enable"},

	{"Headphone Amplifier", NULL, "PCM1773 DAC"},
	{"Line Out", NULL, "PCM1773 DAC"},
	{"Headphone Jack", NULL, "Headphone Amplifier"},

	{"AUXL", NULL, "Line In"},
	{"AUXR", NULL, "Line In"},

	{"MAINMIC", NULL, "Mic (internal)"},
	{"Mic (internal)", NULL, "Mic Bias 1"},

	{"SUBMIC", NULL, "Mic (external)"},
	{"Mic (external)", NULL, "Mic Bias 2"},
};

static int omap3pandora_out_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_dapm_context *dapm = &rtd->card->dapm;

	/* All TWL4030 output pins are floating */
	snd_soc_dapm_nc_pin(dapm, "EARPIECE");
	snd_soc_dapm_nc_pin(dapm, "PREDRIVEL");
	snd_soc_dapm_nc_pin(dapm, "PREDRIVER");
	snd_soc_dapm_nc_pin(dapm, "HSOL");
	snd_soc_dapm_nc_pin(dapm, "HSOR");
	snd_soc_dapm_nc_pin(dapm, "CARKITL");
	snd_soc_dapm_nc_pin(dapm, "CARKITR");
	snd_soc_dapm_nc_pin(dapm, "HFL");
	snd_soc_dapm_nc_pin(dapm, "HFR");
	snd_soc_dapm_nc_pin(dapm, "VIBRA");

	return 0;
}

static int omap3pandora_in_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_dapm_context *dapm = &rtd->card->dapm;

	/* Not comnnected */
	snd_soc_dapm_nc_pin(dapm, "HSMIC");
	snd_soc_dapm_nc_pin(dapm, "CARKITMIC");
	snd_soc_dapm_nc_pin(dapm, "DIGIMIC0");
	snd_soc_dapm_nc_pin(dapm, "DIGIMIC1");

	return 0;
}

static const struct snd_soc_ops omap3pandora_playback_ops = {
	.startup = omap3pandora_common_startup,
	.hw_params = omap3pandora_playback_hw_params,
	.hw_free = omap3pandora_playback_hw_free,
};

static const struct snd_soc_ops omap3pandora_capture_ops = {
	.startup = omap3pandora_common_startup,
	.hw_params = omap3pandora_capture_hw_params,
	.hw_free = omap3pandora_capture_hw_free,
};

/* Digital audio interface glue - connects codec <--> CPU */
#if IS_BUILTIN(CONFIG_SND_SOC_OMAP3_PANDORA)
SND_SOC_DAILINK_DEFS(out,
	DAILINK_COMP_ARRAY(COMP_CPU("omap-mcbsp.2")),
	DAILINK_COMP_ARRAY(COMP_CODEC("pcm1773-codec", "pcm1773-hifi")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("omap-mcbsp.2")));

SND_SOC_DAILINK_DEFS(in,
	DAILINK_COMP_ARRAY(COMP_CPU("omap-mcbsp.4")),
	DAILINK_COMP_ARRAY(COMP_CODEC("twl4030-codec", "twl4030-hifi")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("omap-mcbsp.4")));
#else /* IS_BUILTIN(CONFIG_SND_SOC_OMAP3_PANDORA) */
SND_SOC_DAILINK_DEFS(out,
	DAILINK_COMP_ARRAY(COMP_CPU("49022000.mcbsp")),
	DAILINK_COMP_ARRAY(COMP_CODEC("pcm1773-codec", "pcm1773-hifi")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("49022000.mcbsp")));

SND_SOC_DAILINK_DEFS(in,
	DAILINK_COMP_ARRAY(COMP_CPU("49026000.mcbsp")),
	DAILINK_COMP_ARRAY(COMP_CODEC("twl4030-codec", "twl4030-hifi")),
	DAILINK_COMP_ARRAY(COMP_PLATFORM("49026000.mcbsp")));
#endif /* IS_BUILTIN(CONFIG_SND_SOC_OMAP3_PANDORA) */

static struct snd_soc_dai_link omap3pandora_dai[] = {
	{
		.name = "PCM1773",
		.stream_name = "HiFi Out",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.ops = &omap3pandora_playback_ops,
		.init = omap3pandora_out_init,
		.playback_only = 1,
		SND_SOC_DAILINK_REG(out),
	}, {
		.name = "TWL4030",
		.stream_name = "Line/Mic In",
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBS_CFS,
		.ops = &omap3pandora_capture_ops,
		.init = omap3pandora_in_init,
		.capture_only = 1,
		SND_SOC_DAILINK_REG(in),
	}
};

/* SoC card */
static struct snd_soc_card snd_soc_card_omap3pandora = {
	.name = "omap3pandora",
	.owner = THIS_MODULE,
	.dai_link = omap3pandora_dai,
	.num_links = ARRAY_SIZE(omap3pandora_dai),

	.dapm_widgets = omap3pandora_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(omap3pandora_dapm_widgets),
	.dapm_routes = omap3pandora_map,
	.num_dapm_routes = ARRAY_SIZE(omap3pandora_map),
	.owner = THIS_MODULE,
};

static int omap3pandora_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct snd_soc_card *card = &snd_soc_card_omap3pandora;
	struct omap3pandora_sound *ctx;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx) {
		dev_dbg(dev, "cannot allocate space for runtime data\n");
		return -ENOMEM;
	}

	mutex_init(&ctx->sample_rate_lock);

	ctx->amp_gpio = devm_gpiod_get(dev, "amp", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->amp_gpio)) {
		dev_err(dev, "cannot find amplifier gpio");
		return PTR_ERR(ctx->amp_gpio);
	}

	ctx->amp_regulator = devm_regulator_get(dev, "amp");
	if (IS_ERR(ctx->amp_regulator)) {
		ret = PTR_ERR(ctx->amp_regulator);
		dev_err(dev, "Failed to request regulator for amplifier power: %d\n", ret);
		return PTR_ERR(ctx->amp_regulator);
	}

	card->dev = dev;
	ret = snd_soc_register_card(card);
	if (ret) {
		dev_err(dev, "Failed to register sound card: %d\n", ret);
		return ret;
	}

	snd_soc_card_set_drvdata(card, ctx);

	return 0;
}

static int omap3pandora_remove(struct platform_device *pdev)
{
	struct snd_soc_card *card = platform_get_drvdata(pdev);
	struct omap3pandora_sound *ctx = snd_soc_card_get_drvdata(card);

	snd_soc_unregister_card(card);

	mutex_destroy(&ctx->sample_rate_lock);

	return 0;
}

static const struct of_device_id omap3pandora_of_match[] = {
	{ .compatible = "openpandora,omap3pandora-sound", },
	{ },
};
MODULE_DEVICE_TABLE(of, omap3pandora_of_match);

static struct platform_driver omap3pandora_driver = {
	.driver = {
		.name = "omap3pandora-sound",
		.of_match_table = omap3pandora_of_match,
	},
	.probe = omap3pandora_probe,
	.remove = omap3pandora_remove,
};

module_platform_driver(omap3pandora_driver);

MODULE_AUTHOR("Grazvydas Ignotas <notasas@gmail.com>");
MODULE_DESCRIPTION("ALSA SoC OMAP3 Pandora");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:omap3pandora-sound");
