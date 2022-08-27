/*
 * This is a driver for the W2CBW003 Bluetooth PCM interface
 * it does not control setup of bluetooth connection etc.
 *
 * w2cbw003.c
 *
 * based on wm8727 driver
 *
 *  Created on: 15-Oct-2009
 *      Author: neil.jones@imgtec.com
 *
 * Copyright (C) 2009 Imagination Technologies Ltd.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
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

static const struct snd_soc_dapm_widget w2cbw003_dapm_widgets[] = {
SND_SOC_DAPM_OUTPUT("VOUTL"),
SND_SOC_DAPM_OUTPUT("VOUTR"),
};

static const struct snd_soc_dapm_route w2cbw003_dapm_routes[] = {
	{ "VOUTL", NULL, "Playback" },
	{ "VOUTR", NULL, "Playback" },
};

/*
 * Note this is a simple chip with no configuration interface, sample rate is
 * determined automatically by examining the Master clock and Bit clock ratios
 */

// FIXME: adjust what the W2CBW003 PCM I/F supports...

#define W2CBW003_RATES  (SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 |\
			SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |\
			SNDRV_PCM_RATE_192000)


struct snd_soc_dai_driver w2cbw003_dai = {
	.name = "W2CBW003",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = W2CBW003_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = W2CBW003_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
};

struct snd_soc_component_driver soc_component_dev_w2cbw003 = {
	.dapm_widgets		= w2cbw003_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(w2cbw003_dapm_widgets),
	.dapm_routes		= w2cbw003_dapm_routes,
	.num_dapm_routes	= ARRAY_SIZE(w2cbw003_dapm_routes),
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
};

static int w2cbw003_platform_probe(struct platform_device *pdev)
{
	return devm_snd_soc_register_component(&pdev->dev,
			&soc_component_dev_w2cbw003, &w2cbw003_dai, 1);
}

static const struct of_device_id w2cbw003_component_of_match[] = {
	{ .compatible = "wi2wi,w2cbw003-codec", },
	{ .compatible = "ti,wl1271-codec", },
	{ .compatible = "ti,wl1835-codec", },
	{ .compatible = "ti,wl1837-codec", },
	{},
};
MODULE_DEVICE_TABLE(of, w2cbw003_component_of_match);

static struct platform_driver w2cbw003_component_driver = {
	.driver = {
			.name = "w2cbw003_component_audio",
			.of_match_table = of_match_ptr(w2cbw003_component_of_match),
	},

	.probe = w2cbw003_platform_probe,
};

module_platform_driver(w2cbw003_component_driver);

MODULE_DESCRIPTION("ASoC W2CBW003 driver");
MODULE_AUTHOR("Neil Jones");
MODULE_LICENSE("GPL");
