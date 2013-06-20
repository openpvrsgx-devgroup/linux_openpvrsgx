/*
 * Tis is a driver for the W2CBW003 Bluetooth PCM interface
 *
 * w2cbw003.c
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
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "w2cbw003-bt.h"
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

struct snd_soc_codec_driver soc_codec_dev_w2cbw003;


static int w2cbw003_platform_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_w2cbw003, &w2cbw003_dai, 1);
}

static int w2cbw003_platform_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

MODULE_ALIAS("platform:w2cbw003_codec_audio");

static struct platform_driver w2cbw003_codec_driver = {
	.driver = {
			.name = "w2cbw003_codec_audio",
			.owner = THIS_MODULE,
	},

	.probe = w2cbw003_platform_probe,
	.remove = w2cbw003_platform_remove,
};

static int __init w2cbw003_init(void)
{
	return platform_driver_register(&w2cbw003_codec_driver);
}
module_init(w2cbw003_init);

static void __exit w2cbw003_exit(void)
{
	platform_driver_unregister(&w2cbw003_codec_driver);
}
module_exit(w2cbw003_exit);

MODULE_DESCRIPTION("ASoC W2CBW003 driver");
MODULE_AUTHOR("Neil Jones");
MODULE_LICENSE("GPL");
