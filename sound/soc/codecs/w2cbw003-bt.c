/*
 * FIXME: this is a blueprint for the W2CBW003 Bluetooth PCM interface
 * needs to be adapted for GTA04
 * CHECKME: can we use the generic AC97 or SPDIF driver instead of defining our own?
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
	.playback = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = W2CBW003_RATES,
		.formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
	},
};
EXPORT_SYMBOL_GPL(w2cbw003_dai);

static int w2cbw003_soc_probe(struct snd_soc_codec *codec)
{
	int ret = 0;

	/* register pcms */
	ret = snd_soc_new_ac97_codec(codec, &soc_ac97_ops, 0);
	if (ret < 0)
		printk(KERN_ERR "w2cbw003: failed to create pcms\n");

	return ret;
}

static int w2cbw003_soc_remove(struct snd_soc_codec *codec)
{
	if (codec == NULL)
		return 0;
	snd_soc_free_ac97_codec(codec);
	return 0;
}

struct snd_soc_codec_driver soc_codec_dev_w2cbw003 = {
	.probe = 	w2cbw003_soc_probe,
	.remove = 	w2cbw003_soc_remove,
// 	.write = ac97_write,
// 	.read = ac97_read,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_w2cbw003);


static __devinit int w2cbw003_platform_probe(struct platform_device *pdev)
{
// 	w2cbw003_dai.dev = &pdev->dev;
// 	return snd_soc_register_dai(&w2cbw003_dai);
	return snd_soc_register_codec(&pdev->dev,
			&soc_codec_dev_w2cbw003, &w2cbw003_dai, 1);
}

static int __devexit w2cbw003_platform_remove(struct platform_device *pdev)
{
// 	snd_soc_unregister_dai(&w2cbw003_dai);
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
	.remove = __devexit_p(w2cbw003_platform_remove),
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
