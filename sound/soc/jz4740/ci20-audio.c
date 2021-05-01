/*
 * CI20 ASoC driver
 *
 * Copyright (c) 2013 Imagination Technologies
 * Author: Paul Burton <paul.burton@imgtec.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#include <sound/jack.h>
#include <sound/soc.h>

/* FIXME: convert to gpiod */
static int hp_mute_gpio;
static int hp_mic_sw_en_gpio;

static struct snd_soc_jack ci20_hp_jack;
static struct snd_soc_jack ci20_hdmi_jack;

static struct snd_soc_jack_pin ci20_hp_jack_pins[] = {
	{
		.pin = "Headphone Jack",
		.mask = SND_JACK_HEADPHONE,
	},
};

static int ci20_hp_jack_status_check(void *data);

static struct snd_soc_jack_gpio ci20_hp_jack_gpio = {
	.name = "headphone detect",
	.report = SND_JACK_HEADPHONE,
	.debounce_time = 200,
	.invert = 1,
	.jack_status_check = ci20_hp_jack_status_check,
};

static int ci20_hp_jack_status_check(void *data)
{
	int enable;

	enable = !gpio_get_value_cansleep(ci20_hp_jack_gpio.gpio);

	/*
	 * The headset type detection switch requires a rising edge on its
	 * enable pin to trigger the detection sequence.
	 */
	if (enable) {
		gpio_set_value_cansleep(hp_mic_sw_en_gpio, 1);
		return SND_JACK_HEADPHONE;
	} else {
		gpio_set_value_cansleep(hp_mic_sw_en_gpio, 0);
		return 0;
	}
}

static int ci20_hp_event(struct snd_soc_dapm_widget *widget,
	struct snd_kcontrol *ctrl, int event)
{
	gpio_set_value(hp_mute_gpio, !!SND_SOC_DAPM_EVENT_OFF(event));
	return 0;
}

static const struct snd_soc_dapm_widget ci20_widgets[] = {
	SND_SOC_DAPM_MIC("Mic", NULL),
	SND_SOC_DAPM_HP("Headphone Jack", ci20_hp_event),
	SND_SOC_DAPM_LINE("HDMI", NULL),
};

static const struct snd_soc_dapm_route ci20_routes[] = {
	{"Mic", NULL, "AIP2"},
	{"Headphone Jack", NULL, "AOHPL"},
	{"Headphone Jack", NULL, "AOHPR"},
	{"HDMI", NULL, "TX"},
};

static int ci20_audio_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = asoc_rtd_to_cpu(rtd, 0);
	unsigned int dai_fmt = rtd->dai_link->dai_fmt;
	int mclk, ret;

	switch (params_rate(params)) {
	case 8000:
	case 16000:
	case 24000:
	case 32000:
	case 48000:
	case 64000:
	case 96000:
		mclk = 12288000;
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		mclk = 11289600;
		break;
	default:
		return -EINVAL;
	}

	ret = snd_soc_dai_set_fmt(cpu_dai, dai_fmt);
	if (ret < 0) {
		dev_err(cpu_dai->dev, "failed to set cpu_dai fmt.\n");
		return ret;
	}

	ret = snd_soc_dai_set_sysclk(cpu_dai, 0, mclk, SND_SOC_CLOCK_OUT);
	if (ret < 0) {
		dev_err(cpu_dai->dev, "failed to set cpu_dai sysclk.\n");
		return ret;
	}

	return 0;
}

static int ci20_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *dai = asoc_rtd_to_codec(rtd, 0);
	struct snd_soc_component *codec = dai->component;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_card_jack_new(card, "Headphone Jack", SND_JACK_HEADPHONE,
			&ci20_hp_jack, ci20_hp_jack_pins, ARRAY_SIZE(ci20_hp_jack_pins));
	snd_soc_jack_add_gpios(&ci20_hp_jack, 1, &ci20_hp_jack_gpio);

	snd_soc_dapm_nc_pin(dapm, "AIP1");
	snd_soc_dapm_nc_pin(dapm, "AIP3");
	snd_soc_dapm_force_enable_pin(dapm, "Mic Bias");
	snd_soc_dapm_sync(dapm);

	return 0;
}

static int ci20_hdmi_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_card *card = rtd->card;
	struct snd_soc_dai *dai = asoc_rtd_to_codec(rtd, 0);
	struct snd_soc_component *codec = dai->component;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_dapm_enable_pin(dapm, "HDMI");

	/* Enable headphone jack detection */
	snd_soc_card_jack_new(card, "HDMI Jack", SND_JACK_LINEOUT,
			 &ci20_hdmi_jack, NULL, 0);

	/* Jack is connected (it just is) */
	snd_soc_jack_report(&ci20_hdmi_jack, SND_JACK_LINEOUT, SND_JACK_LINEOUT);
	return 0;
}

static struct snd_soc_ops ci20_audio_dai_ops = {
	.hw_params = ci20_audio_hw_params,
};

#define CI20_DAIFMT (SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF \
					| SND_SOC_DAIFMT_CBM_CFM)

static struct snd_soc_dai_link_component dai_link0_cpus[] = {
	{ .name = "jz4780-i2s", },
};

static struct snd_soc_dai_link_component dai_link0_platforms[] = {
	{ .name = "jz4780-i2s", },
};

static struct snd_soc_dai_link_component dai_link0_codecs[] = {
	{ .name = "jz4780-codec", .dai_name = "jz4780-hifi", },
};

static struct snd_soc_dai_link_component dai_link1_cpus[] = {
	{ .name = "jz4780-i2s", },
};

static struct snd_soc_dai_link_component dai_link1_platforms[] = {
	{ .name = "jz4780-i2s", },
};

static struct snd_soc_dai_link_component dai_link1_codecs[] = {
	{ .name = /* "dw-hdmi-audio" */ "hdmi-audio-codec.3.auto", .dai_name = /* "dw-hdmi-hifi" */ "i2s-hifi", },
};

static struct snd_soc_dai_link ci20_dai_link[] = {
	{
		.name = "ci20",
		.stream_name = "headphones",
		.cpus = dai_link0_cpus,
		.num_cpus = ARRAY_SIZE(dai_link0_cpus),
		.platforms = dai_link0_platforms,
		.num_platforms = ARRAY_SIZE(dai_link0_platforms),
		.codecs = dai_link0_codecs,
		.num_codecs = ARRAY_SIZE(dai_link0_codecs),
		.init = ci20_init,
		.ops = &ci20_audio_dai_ops,
		.dai_fmt = CI20_DAIFMT,
	},
	{
		.name = "ci20 HDMI",
		.stream_name = "hdmi",
		.cpus = dai_link1_cpus,
		.num_cpus = ARRAY_SIZE(dai_link1_cpus),
		.platforms = dai_link1_platforms,
		.num_platforms = ARRAY_SIZE(dai_link1_platforms),
		.codecs = dai_link1_codecs,
		.num_codecs = ARRAY_SIZE(dai_link1_codecs),
		.init = ci20_hdmi_init,
		.ops = &ci20_audio_dai_ops,
		.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS,
	}
};

static struct snd_soc_card ci20_audio_card = {
	.name = "ci20",
	.dai_link = ci20_dai_link,
	.num_links = ARRAY_SIZE(ci20_dai_link),

	.dapm_widgets = ci20_widgets,
	.num_dapm_widgets = ARRAY_SIZE(ci20_widgets),
	.dapm_routes = ci20_routes,
	.num_dapm_routes = ARRAY_SIZE(ci20_routes),
};

static const struct of_device_id ingenic_asoc_ci20_dt_ids[] = {
	{ .compatible = "ingenic,ci20-audio", },
	{ }
};
MODULE_DEVICE_TABLE(of, ingenic_asoc_ci20_dt_ids);

static int ingenic_asoc_ci20_probe(struct platform_device *pdev)
{
	int ret;

	struct snd_soc_card *card = &ci20_audio_card;
	struct device_node *np = pdev->dev.of_node;
	struct device_node *codec, *i2s;

	card->dev = &pdev->dev;

	i2s = of_parse_phandle(np, "ingenic,i2s-controller", 0);
	codec = of_parse_phandle(np, "ingenic,codec", 0);

	if (!i2s || !codec) {
		dev_warn(&pdev->dev,
			 "Phandle not found for i2s/codecs, using defaults\n");
	} else {
		dev_dbg(&pdev->dev, "Setting dai_link parameters\n");
		ci20_dai_link[0].cpus[0].of_node = i2s;
		ci20_dai_link[0].cpus[0].name = NULL;
		ci20_dai_link[1].cpus[0].of_node = i2s;
		ci20_dai_link[1].cpus[0].name = NULL;
		ci20_dai_link[0].platforms[0].of_node = i2s;
		ci20_dai_link[0].platforms[0].name = NULL;
		ci20_dai_link[1].platforms[0].of_node = i2s;
		ci20_dai_link[1].platforms[0].name = NULL;
		ci20_dai_link[0].codecs[0].of_node = codec;
		ci20_dai_link[0].codecs[0].name = NULL;
	}

/* FIXME: convert to gpiod and add error handling */

	ci20_hp_jack_gpio.gpio = of_get_named_gpio(np, "ingenic,hp-det-gpio", 0);
	hp_mute_gpio = of_get_named_gpio(np, "ingenic,hp-mute-gpio", 0);
	hp_mic_sw_en_gpio = of_get_named_gpio(np, "ingenic,mic-detect-gpio", 0);

	gpio_direction_output(hp_mute_gpio, 1);
	gpio_direction_output(hp_mic_sw_en_gpio, 0);

	ret = devm_snd_soc_register_card(&pdev->dev, card);
	if ((ret) && (ret != -EPROBE_DEFER))
		dev_err(&pdev->dev, "snd_soc_register_card() failed:%d\n", ret);
	return ret;
}

static int ingenic_asoc_ci20_remove(struct platform_device *pdev)
{
	snd_soc_jack_free_gpios(&ci20_hp_jack, 1, &ci20_hp_jack_gpio);

	return 0;
}

static struct platform_driver ingenic_ci20_audio_driver = {
	.driver = {
		.name = "ingenic-ci20-audio",
		.owner = THIS_MODULE,
		.of_match_table = ingenic_asoc_ci20_dt_ids,
	},
	.probe = ingenic_asoc_ci20_probe,
	.remove = ingenic_asoc_ci20_remove,
};

module_platform_driver(ingenic_ci20_audio_driver);

MODULE_AUTHOR("Paul Burton <paul.burton@imgtec.com>");
MODULE_DESCRIPTION("ci20/JZ4780 ASoC driver");
MODULE_LICENSE("GPL");
