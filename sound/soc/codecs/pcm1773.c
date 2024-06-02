// SPDX-License-Identifier: GPL-2.0-only
/*
 * pcm1773.c -- codec for the simple PCM1773 output codec from TI
 *
 * Shamelessly cobbled together from sound/soc/ti/omap3pandora.c and a few
 * other codec drivers in sound/soc/codecs/
 *
 * Author: Grond <grond66@riseup.net>
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>

struct pcm1773 {
	struct regulator *regulator;
	bool has_enable_gpio;
	int enable_gpio;
};

static int pcm1773_dac_event(struct snd_soc_dapm_widget *w,
			     struct snd_kcontrol *k, int event)
{
	struct snd_soc_component *component = w->dapm->component;
	struct pcm1773 *ctx = snd_soc_component_get_drvdata(component);
	struct device *dev = component->dev;
	int ret;

	/*
	 * The PCM1773 DAC datasheet requires 1ms delay between switching
	 * VCC power on/off and /PD pin high/low
	 */
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		if (ctx->regulator) {
			ret = regulator_enable(ctx->regulator);
			if (ret) {
				dev_err(dev, "Failed to power DAC: %d\n", ret);
				return ret;
			}
			mdelay(1);
		}

		if (ctx->has_enable_gpio)
			gpio_set_value(ctx->enable_gpio, 1);
	} else {
		if (ctx->has_enable_gpio)
			gpio_set_value(ctx->enable_gpio, 0);

		if (ctx->regulator) {
			mdelay(1);
			regulator_disable(ctx->regulator);
		}
	}

	return 0;
}

static const struct snd_soc_dapm_widget pcm1773_dapm_widgets[] = {
	SND_SOC_DAPM_DAC_E("PCM1773 DAC", "HiFi Playback", SND_SOC_NOPM,
			   0, 0, pcm1773_dac_event,
			   SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
};

static const struct snd_soc_dapm_route pcm1773_dapm_routes[] = {
	/* tell DAPM that the main stream flows to the PCM1773 */
	{"PCM1773 DAC", NULL, "PCM1773 IN"},
};

static struct snd_soc_dai_driver pcm1773_dai = {
	.name = "pcm1773-hifi",
	.playback = {
		.stream_name = "PCM1773 IN",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_48000,
		// [TODO] these really should be BE, per the data sheet but for
		// some reason the omap-mcbsp driver claims only to support LE.
		// investigate
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
};

static int pcm1773_probe(struct snd_soc_component *component)
{
	struct pcm1773 *ctx = NULL;
	struct device *dev = component->dev;
	struct device_node *of_node = dev->of_node;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;
	snd_soc_component_set_drvdata(component, ctx);

	if (!of_node) {
		dev_dbg(dev, "DT data not present for pcm1773-codec; assuming hardware handles regulators and chip enable appropriately");
		return 0;
	}

	ctx->enable_gpio = of_get_named_gpio(of_node, "enable-gpio", 0);
	if (ctx->enable_gpio <= 0) {
		ctx->enable_gpio = 0;
		dev_warn(dev, "invalid GPIO specification for enable-gpio");
	} else {
		ctx->has_enable_gpio = true;
		dev_dbg(dev, "got enable-gpio %d", ctx->enable_gpio);

		ret = gpio_request(ctx->enable_gpio, "dac_enable");
		if (ret >= 0)
			ret = gpio_direction_output(ctx->enable_gpio, 0);
		if (ret < 0) {
			dev_err(dev, "failed to reserve GPIO %d as an output",
				ctx->enable_gpio);
			return ret;
		}
	}

	ctx->regulator = regulator_get(dev, "vcc");
	if (IS_ERR(ctx->regulator)) {
		ctx->regulator = NULL;
		dev_warn(dev, "cannot get regulator 'vcc'");
	}

	return 0;
}

static void pcm1773_remove(struct snd_soc_component *component)
{
	struct pcm1773 *ctx = snd_soc_component_get_drvdata(component);

	if (ctx->regulator)
		regulator_put(ctx->regulator);

	if (ctx->has_enable_gpio)
		gpio_free(ctx->enable_gpio);
}

static const struct snd_soc_component_driver soc_component_dev_pcm1773 = {
	.probe = pcm1773_probe,
	.remove = pcm1773_remove,
	.dapm_widgets = pcm1773_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(pcm1773_dapm_widgets),
	.dapm_routes = pcm1773_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(pcm1773_dapm_routes),
};

static int pcm1773_codec_probe(struct platform_device *pdev)
{
	return devm_snd_soc_register_component(&pdev->dev,
		&soc_component_dev_pcm1773,
		&pcm1773_dai, 1);
}

static const struct of_device_id pcm1773_of_match[] = {
	{
		.compatible = "ti,pcm1773",
	},
	{ },
};
MODULE_DEVICE_TABLE(of, pcm1773_of_match);

static struct platform_driver pcm1773_codec_driver = {
	.probe = pcm1773_codec_probe,
	.driver = {
		.name = "pcm1773-codec",
		.of_match_table = pcm1773_of_match,
	},
};

module_platform_driver(pcm1773_codec_driver);

MODULE_DESCRIPTION("ASoC codec driver PCM1773");
MODULE_AUTHOR("Grond");
MODULE_LICENSE("GPL");
