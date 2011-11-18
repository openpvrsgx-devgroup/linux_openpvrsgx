/*
 * FIXME: this is a blueprint for a Si47xx I2C driver
 * it is based on the WM8728 driver and
 * needs to be adapted for GTA04
 *
 * si47xx.c  --  Si47xx ALSA SoC Audio driver
 *
 * Copyright 2008 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "si47xx.h"

struct snd_soc_codec_device soc_codec_dev_si47xx;

/*
 * We can't read the WM8728 register space so we cache them instead.
 * Note that the defaults here aren't the physical defaults, we latch
 * the volume update bits, mute the output and enable infinite zero
 * detect.
 */
static const u16 si47xx_reg_defaults[] = {
	0x1ff,
	0x1ff,
	0x001,
	0x100,
};

static const DECLARE_TLV_DB_SCALE(si47xx_tlv, -12750, 50, 1);

static const struct snd_kcontrol_new si47xx_snd_controls[] = {

SOC_DOUBLE_R_TLV("Digital Playback Volume", SI47XX_DACLVOL, SI47XX_DACRVOL,
		 0, 255, 0, si47xx_tlv),

SOC_SINGLE("Deemphasis", SI47XX_DACCTL, 1, 1, 0),
};

/*
 * DAPM controls.
 */
static const struct snd_soc_dapm_widget si47xx_dapm_widgets[] = {
SND_SOC_DAPM_DAC("DAC", "HiFi Playback", SND_SOC_NOPM, 0, 0),
SND_SOC_DAPM_OUTPUT("VOUTL"),
SND_SOC_DAPM_OUTPUT("VOUTR"),
};

static const struct snd_soc_dapm_route intercon[] = {
	{"VOUTL", NULL, "DAC"},
	{"VOUTR", NULL, "DAC"},
};

static int si47xx_add_widgets(struct snd_soc_codec *codec)
{
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	snd_soc_dapm_new_controls(dapm, si47xx_dapm_widgets,
				  ARRAY_SIZE(si47xx_dapm_widgets));

	snd_soc_dapm_add_routes(dapm, intercon, ARRAY_SIZE(intercon));

	return 0;
}

static int si47xx_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	u16 mute_reg = snd_soc_read(codec, SI47XX_DACCTL);

	if (mute)
		snd_soc_write(codec, SI47XX_DACCTL, mute_reg | 1);
	else
		snd_soc_write(codec, SI47XX_DACCTL, mute_reg & ~1);

	return 0;
}

static int si47xx_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	u16 dac = snd_soc_read(codec, SI47XX_DACCTL);

	dac &= ~0x18;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		dac |= 0x10;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		dac |= 0x08;
		break;
	default:
		return -EINVAL;
	}

	snd_soc_write(codec, SI47XX_DACCTL, dac);

	return 0;
}

static int si47xx_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u16 iface = snd_soc_read(codec, SI47XX_IFCTL);

	/* Currently only I2S is supported by the driver, though the
	 * hardware is more flexible.
	 */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iface |= 1;
		break;
	default:
		return -EINVAL;
	}

	/* The hardware only support full slave mode */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		iface &= ~0x22;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		iface |=  0x20;
		iface &= ~0x02;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iface |= 0x02;
		iface &= ~0x20;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		iface |= 0x22;
		break;
	default:
		return -EINVAL;
	}

	snd_soc_write(codec, SI47XX_IFCTL, iface);
	return 0;
}

static int si47xx_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	u16 reg;
	int i;

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
	case SND_SOC_BIAS_STANDBY:
		if (codec->bias_level == SND_SOC_BIAS_OFF) {
			/* Power everything up... */
			reg = snd_soc_read(codec, SI47XX_DACCTL);
			snd_soc_write(codec, SI47XX_DACCTL, reg & ~0x4);

			/* ..then sync in the register cache. */
			for (i = 0; i < ARRAY_SIZE(si47xx_reg_defaults); i++)
				snd_soc_write(codec, i,
					     snd_soc_read(codec, i));
		}
		break;

	case SND_SOC_BIAS_OFF:
		reg = snd_soc_read(codec, SI47XX_DACCTL);
		snd_soc_write(codec, SI47XX_DACCTL, reg | 0x4);
		break;
	}
	codec->bias_level = level;
	return 0;
}

// FIXME: adjust what the Si47xx chip needs...

#define SI47XX_RATES (SNDRV_PCM_RATE_8000_192000)

#define SI47XX_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE)

static struct snd_soc_dai_ops si47xx_dai_ops = {
	.hw_params	= si47xx_hw_params,
	.digital_mute	= si47xx_mute,
	.set_fmt	= si47xx_set_dai_fmt,
};

struct snd_soc_dai si47xx_dai = {
	.name = "Si47xx",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SI47XX_RATES,
		.formats = SI47XX_FORMATS,
	},
	.playback = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SI47XX_RATES,
		.formats = SI47XX_FORMATS,
	},
	.ops = &si47xx_dai_ops,
};
EXPORT_SYMBOL_GPL(si47xx_dai);

static int si47xx_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	si47xx_set_bias_level(codec, SND_SOC_BIAS_OFF);

	return 0;
}

static int si47xx_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	si47xx_set_bias_level(codec, codec->suspend_bias_level);

	return 0;
}

/*
 * initialise the Si47xx driver
 * register the mixer and dsp interfaces with the kernel
 */
static int si47xx_init(struct snd_soc_device *socdev,
		       enum snd_soc_control_type control)
{
	struct snd_soc_codec *codec = socdev->card->codec;
	int ret = 0;

	codec->name = "Si47xx";
	codec->owner = THIS_MODULE;
	codec->set_bias_level = si47xx_set_bias_level;
	codec->dai = &si47xx_dai;
	codec->num_dai = 1;
	codec->bias_level = SND_SOC_BIAS_OFF;
	codec->reg_cache_size = ARRAY_SIZE(si47xx_reg_defaults);
	codec->reg_cache = kmemdup(si47xx_reg_defaults,
				   sizeof(si47xx_reg_defaults),
				   GFP_KERNEL);
	if (codec->reg_cache == NULL)
		return -ENOMEM;

	ret = snd_soc_codec_set_cache_io(codec, 7, 9, control);
	if (ret < 0) {
		printk(KERN_ERR "si47xx: failed to configure cache I/O: %d\n",
		       ret);
		goto err;
	}

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "si47xx: failed to create pcms\n");
		goto err;
	}

	/* power on device */
	si47xx_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	snd_soc_add_controls(codec, si47xx_snd_controls,
				ARRAY_SIZE(si47xx_snd_controls));
	si47xx_add_widgets(codec);

	return ret;

err:
	kfree(codec->reg_cache);
	return ret;
}

static struct snd_soc_device *si47xx_socdev;

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)

/*
 * Si47xx 2 wire address is determined by GPIO5
 * state during powerup.
 *    low  = 0x1a
 *    high = 0x1b
 */

static int si47xx_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct snd_soc_device *socdev = si47xx_socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	int ret;

	i2c_set_clientdata(i2c, codec);
	codec->control_data = i2c;

	ret = si47xx_init(socdev, SND_SOC_I2C);
	if (ret < 0)
		pr_err("failed to initialise Si47xx\n");

	return ret;
}

static int si47xx_i2c_remove(struct i2c_client *client)
{
	struct snd_soc_codec *codec = i2c_get_clientdata(client);
	kfree(codec->reg_cache);
	return 0;
}

static const struct i2c_device_id si47xx_i2c_id[] = {
	{ "si47xx", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, si47xx_i2c_id);

static struct i2c_driver si47xx_i2c_driver = {
	.driver = {
		.name = "Si47xx I2C Codec",
		.owner = THIS_MODULE,
	},
	.probe =    si47xx_i2c_probe,
	.remove =   si47xx_i2c_remove,
	.id_table = si47xx_i2c_id,
};

static int si47xx_add_i2c_device(struct platform_device *pdev,
				 const struct si47xx_setup_data *setup)
{
	struct i2c_board_info info;
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	int ret;

	ret = i2c_add_driver(&si47xx_i2c_driver);
	if (ret != 0) {
		dev_err(&pdev->dev, "can't add i2c driver\n");
		return ret;
	}

	memset(&info, 0, sizeof(struct i2c_board_info));
	info.addr = setup->i2c_address;
	strlcpy(info.type, "si47xx", I2C_NAME_SIZE);

	adapter = i2c_get_adapter(setup->i2c_bus);
	if (!adapter) {
		dev_err(&pdev->dev, "can't get i2c adapter %d\n",
			setup->i2c_bus);
		goto err_driver;
	}

	client = i2c_new_device(adapter, &info);
	i2c_put_adapter(adapter);
	if (!client) {
		dev_err(&pdev->dev, "can't add i2c device at 0x%x\n",
			(unsigned int)info.addr);
		goto err_driver;
	}

	return 0;

err_driver:
	i2c_del_driver(&si47xx_i2c_driver);
	return -ENODEV;
}
#endif

#if defined(CONFIG_SPI_MASTER)
static int __devinit si47xx_spi_probe(struct spi_device *spi)
{
	struct snd_soc_device *socdev = si47xx_socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	int ret;

	codec->control_data = spi;

	ret = si47xx_init(socdev, SND_SOC_SPI);
	if (ret < 0)
		dev_err(&spi->dev, "failed to initialise Si47xx\n");

	return ret;
}

static int __devexit si47xx_spi_remove(struct spi_device *spi)
{
	return 0;
}

MODULE_ALIAS("platform:si47xx_codec_audio");

static struct spi_driver si47xx_spi_driver = {
	.driver = {
		.name	= "si47xx_codec_audio",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe		= si47xx_spi_probe,
	.remove		= __devexit_p(si47xx_spi_remove),
};
#endif /* CONFIG_SPI_MASTER */

static int si47xx_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct si47xx_setup_data *setup;
	struct snd_soc_codec *codec;
	int ret = 0;

	setup = socdev->codec_data;
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	socdev->card->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	si47xx_socdev = socdev;
	ret = -ENODEV;

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	if (setup->i2c_address) {
		ret = si47xx_add_i2c_device(pdev, setup);
	}
#endif
#if defined(CONFIG_SPI_MASTER)
	if (setup->spi) {
		ret = spi_register_driver(&si47xx_spi_driver);
		if (ret != 0)
			printk(KERN_ERR "can't add spi driver");
	}
#endif

	if (ret != 0)
		kfree(codec);

	return ret;
}

/* power down chip */
static int si47xx_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	if (codec->control_data)
		si47xx_set_bias_level(codec, SND_SOC_BIAS_OFF);

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	i2c_unregister_device(codec->control_data);
	i2c_del_driver(&si47xx_i2c_driver);
#endif
#if defined(CONFIG_SPI_MASTER)
	spi_unregister_driver(&si47xx_spi_driver);
#endif
	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_si47xx = {
	.probe = 	si47xx_probe,
	.remove = 	si47xx_remove,
	.suspend = 	si47xx_suspend,
	.resume =	si47xx_resume,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_si47xx);

static int __init si47xx_modinit(void)
{
	return snd_soc_register_dai(&si47xx_dai);
}
module_init(si47xx_modinit);

static void __exit si47xx_exit(void)
{
	snd_soc_unregister_dai(&si47xx_dai);
}
module_exit(si47xx_exit);

MODULE_DESCRIPTION("ASoC Si47xx driver");
MODULE_AUTHOR("Mark Brown <broonie@opensource.wolfsonmicro.com>");
MODULE_LICENSE("GPL");
