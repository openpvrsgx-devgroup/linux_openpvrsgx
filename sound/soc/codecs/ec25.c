// SPDX-License-Identifier: GPL-2.0-only
/*
 * This is a simple driver for the GTM601 Voice PCM interface
 *
 * Copyright (C) 2015 Goldelico GmbH
 *
 * Author: Marek Belisko <marek@goldelico.com>
 *
 * Based on gtm601.c driver
 */

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>

static const struct snd_soc_dapm_widget ec25_dapm_widgets[] = {
	SND_SOC_DAPM_OUTPUT("AOUT"),
	SND_SOC_DAPM_INPUT("AIN"),
};

static const struct snd_soc_dapm_route ec25_dapm_routes[] = {
	{ "AOUT", NULL, "Playback" },
	{ "Capture", NULL, "AIN" },
};

static struct snd_soc_dai_driver ec25_dai = {
	.name = "ec25",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 1,
		.rates = SNDRV_PCM_RATE_8000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 1,
		.rates = SNDRV_PCM_RATE_8000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
};

static const struct snd_soc_component_driver soc_component_dev_ec25 = {
	.dapm_widgets		= ec25_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ec25_dapm_widgets),
	.dapm_routes		= ec25_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(ec25_dapm_routes),
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
	.non_legacy_dai_naming	= 1,
};

static int ec25_platform_probe(struct platform_device *pdev)
{
	return devm_snd_soc_register_component(&pdev->dev,
			&soc_component_dev_ec25, &ec25_dai, 1);
}

#if defined(CONFIG_OF)
static const struct of_device_id ec25_codec_of_match[] = {
	{ .compatible = "quectel,ec25", },
	{},
};
MODULE_DEVICE_TABLE(of, ec25_codec_of_match);
#endif

static struct platform_driver ec25_codec_driver = {
	.driver = {
		.name = "ec25",
		.of_match_table = of_match_ptr(ec25_codec_of_match),
	},
	.probe = ec25_platform_probe,
};

module_platform_driver(ec25_codec_driver);

MODULE_DESCRIPTION("ASoC ec25 driver");
MODULE_AUTHOR("Marek Belisko <marek@goldelico.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ec25");
