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

// temporary hack until someone understands how to configure the I2C address in the
// board file and gets the driver loaded with I2C address 0x11
// all unused code and the #if USE_I2C_SPI should be removed as soon as I2C works

#define USE_I2C_SPI 1


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#define SI47XX_DACLVOL   0x00
#define SI47XX_DACRVOL   0x01
#define SI47XX_DACCTL    0x02
#define SI47XX_IFCTL     0x03

// FIXME: for the Si47xx we don't need the register cache mechanism
// FIXME: we must make sure that the digital clock (DCLK) is stable
// and the sample rate is set to 0Hz if we disable the clock!
// this happens when arecord is started/stopped, i.e. the clock is gated

/*
 * We can't read the WM8728 register space so we cache them instead.
 * Note that the defaults here aren't the physical defaults, we latch
 * the volume update bits, mute the output and enable infinite zero
 * detect.
 */
static const struct reg_default si47xx_reg_defaults[] = {
	{ 0, 0x1ff },
	{ 1, 0x1ff },
	{ 2, 0x001 },
	{ 3, 0x100 },
};

/* codec private data */
struct si47xx_priv {
	struct regmap *regmap;
};

static const DECLARE_TLV_DB_SCALE(si47xx_tlv, -12750, 50, 1);

static const struct snd_kcontrol_new si47xx_snd_controls[] = {
#if USE_I2C_SPI	// these controls call snd_soc_read(codev, ...)

SOC_DOUBLE_R_TLV("Digital Playback Volume", SI47XX_DACLVOL, SI47XX_DACRVOL,
		 0, 255, 0, si47xx_tlv),

SOC_SINGLE("Deemphasis", SI47XX_DACCTL, 1, 1, 0),
#endif
	/* code fragment
	 SOC_SINGLE("Frequency", SI47XX_DACCTL, 1, 1, 0),
	 */
};

/*
 * DAPM controls.
 */
static const struct snd_soc_dapm_widget si47xx_dapm_widgets[] = {
SND_SOC_DAPM_DAC("DAC", "HiFi Playback", SND_SOC_NOPM, 0, 0),
SND_SOC_DAPM_OUTPUT("VOUTL"),
SND_SOC_DAPM_OUTPUT("VOUTR"),
	/*
	 * here (???) we should add controls for
	 * - tuning the receiver
	 * - tuning the transmitter
	 * - scanning for stations
	 * - getting RSSI & SNR
	 * - RDS
	 * so that we can read/write them through the amixer get/set commands
	 */
};

static const struct snd_soc_dapm_route si47xx_intercon[] = {
	{"VOUTL", NULL, "DAC"},
	{"VOUTR", NULL, "DAC"},
};

static int si47xx_mute(struct snd_soc_dai *dai, int mute)
{
#if 0
	struct snd_soc_component *component = dai->component;
	u16 mute_reg = snd_soc_read(component, SI47XX_DACCTL);
	if (mute)
		snd_soc_write(component, SI47XX_DACCTL, mute_reg | 1);
	else
		snd_soc_write(component, SI47XX_DACCTL, mute_reg & ~1);
#endif
	return 0;
}

static int si47xx_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
#if 0
	struct snd_soc_component *component = dai->component;
	u16 dac = snd_soc_read(component, SI47XX_DACCTL);

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
	snd_soc_write(component, SI47XX_DACCTL, dac);
#endif
	return 0;
}

static int si47xx_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
#if 0
	struct snd_soc_component *component = codec_dai->component;
	u16 iface = snd_soc_read(component, SI47XX_IFCTL);

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
	snd_soc_write(component, SI47XX_IFCTL, iface);
#endif
	return 0;
}

static int si47xx_set_bias_level(struct snd_soc_component *component,
				 enum snd_soc_bias_level level)
{
#if 0
	struct si47xx_priv *si47xx = snd_soc_component_get_drvdata(component);
	u16 reg;

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
	case SND_SOC_BIAS_STANDBY:
		if (snd_soc_component_get_bias_level(component) == SND_SOC_BIAS_OFF) {
			/* Power everything up... */
			reg = snd_soc_read(component, SI47XX_DACCTL);
			snd_soc_write(component, SI47XX_DACCTL, reg & ~0x4);

			/* ..then sync in the register cache. */
			regcache_sync(si47xx->regmap);
		}
		break;

	case SND_SOC_BIAS_OFF:
		reg = snd_soc_read(component, SI47XX_DACCTL);
		snd_soc_write(component, SI47XX_DACCTL, reg | 0x4);
		break;
	}
#endif
	return 0;
}

// FIXME: adjust what the Si47xx chip needs...
#define SI47XX_RATES (SNDRV_PCM_RATE_8000_192000)

#define SI47XX_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
	SNDRV_PCM_FMTBIT_S24_LE)

static struct snd_soc_dai_ops si47xx_dai_ops = {
	.hw_params	= si47xx_hw_params,
#ifdef REVISIT
	.digital_mute	= si47xx_mute,
#endif
	.set_fmt	= si47xx_set_dai_fmt,
};

struct snd_soc_dai_driver si47xx_dai = {
	.name = "Si47xx",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SI47XX_RATES,
		.formats = SI47XX_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = SI47XX_RATES,
		.formats = SI47XX_FORMATS,
	},
	.ops = &si47xx_dai_ops,
};

static struct snd_soc_component_driver soc_component_dev_si47xx = {
	.set_bias_level = si47xx_set_bias_level,
	.controls = si47xx_snd_controls,
	.num_controls = ARRAY_SIZE(si47xx_snd_controls),
	.dapm_widgets = si47xx_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(si47xx_dapm_widgets),
	.dapm_routes = si47xx_intercon,
	.num_dapm_routes = ARRAY_SIZE(si47xx_intercon),
	.suspend_bias_off	= 1,
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
	.non_legacy_dai_naming	= 1,
};

static int si47xx_suspend(struct snd_soc_component *component)
{
	printk("si47xx_suspend\n");
	si47xx_set_bias_level(component, SND_SOC_BIAS_OFF);
	/* send power off command */
	return 0;
}

static int si47xx_resume(struct snd_soc_component *component)
{
	printk("si47xx_resume\n");
//	si47xx_set_bias_level(component, component->dapm.suspend_bias_level);
	/* send power on command */
	/* tune to current frequency */
	return 0;
}

static const struct of_device_id si47xx_of_match[] = {
	{ .compatible = "silicon-labs,si4721", },
	{ }
};
MODULE_DEVICE_TABLE(of, si47xx_of_match);

static const struct regmap_config si47xx_regmap = {
	.reg_bits = 7,
	.val_bits = 9,
	.max_register = SI47XX_IFCTL,

	.reg_defaults = si47xx_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(si47xx_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
};

#if defined(CONFIG_SPI_MASTER)
static int si47xx_spi_probe(struct spi_device *spi)
{
	struct si47xx_priv *si47xx;
	int ret;

	printk("si47xx_spi_probe\n");
	si47xx = devm_kzalloc(&spi->dev, sizeof(struct si47xx_priv),
			      GFP_KERNEL);
	if (si47xx == NULL)
		return -ENOMEM;

	si47xx->regmap = devm_regmap_init_spi(spi, &si47xx_regmap);
	if (IS_ERR(si47xx->regmap))
		return PTR_ERR(si47xx->regmap);

	spi_set_drvdata(spi, si47xx);

	ret = devm_snd_soc_register_component(&spi->dev,
			&soc_component_dev_si47xx, &si47xx_dai, 1);

	return ret;
}

static struct spi_driver si47xx_spi_driver = {
	.driver = {
		.name	= "si47xx_component_audio",
		.of_match_table = si47xx_of_match,
	},
	.probe		= si47xx_spi_probe,
};
#endif /* CONFIG_SPI_MASTER */

#if IS_ENABLED(CONFIG_I2C)
static int si47xx_i2c_probe(struct i2c_client *i2c,
				      const struct i2c_device_id *id)
{
	struct si47xx_priv *si47xx;
	int ret;

	printk("si47xx_i2c_probe\n");
	si47xx = devm_kzalloc(&i2c->dev, sizeof(struct si47xx_priv),
			      GFP_KERNEL);
	if (si47xx == NULL)
		return -ENOMEM;

	si47xx->regmap = devm_regmap_init_i2c(i2c, &si47xx_regmap);
	if (IS_ERR(si47xx->regmap))
		return PTR_ERR(si47xx->regmap);

	i2c_set_clientdata(i2c, si47xx);

	ret = devm_snd_soc_register_component(&i2c->dev,
			&soc_component_dev_si47xx, &si47xx_dai, 1);

	return ret;
}

static const struct i2c_device_id si47xx_i2c_id[] = {
	{ "si4705", 0 },
	{ "si4721", 1 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, si47xx_i2c_id);

static struct i2c_driver si47xx_i2c_driver = {
	.driver = {
		.name = "si47xx_component_audio",
		.of_match_table = si47xx_of_match,
	},
	.probe =    si47xx_i2c_probe,
	.id_table = si47xx_i2c_id,
};
#endif

static int __init si47xx_modinit(void)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_I2C)
	ret = i2c_add_driver(&si47xx_i2c_driver);
	if (ret != 0) {
		printk(KERN_ERR "Failed to register si47xx I2C driver: %d\n",
		       ret);
	}
#endif
#if defined(CONFIG_SPI_MASTER)
	ret = spi_register_driver(&si47xx_spi_driver);
	if (ret != 0) {
		printk(KERN_ERR "Failed to register si47xx SPI driver: %d\n",
		       ret);
	}
#endif
	return ret;
}
module_init(si47xx_modinit);

static void __exit si47xx_exit(void)
{
#if IS_ENABLED(CONFIG_I2C)
	i2c_del_driver(&si47xx_i2c_driver);
#endif
#if defined(CONFIG_SPI_MASTER)
	spi_unregister_driver(&si47xx_spi_driver);
#endif
}
module_exit(si47xx_exit);

MODULE_DESCRIPTION("ASoC Si47xx driver");
MODULE_AUTHOR("Mark Brown <broonie@opensource.wolfsonmicro.com>");
MODULE_LICENSE("GPL");
